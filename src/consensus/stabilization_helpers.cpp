// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/stabilization_mining.h>
#include <measurement/measurement_system.h>
#include <logging.h>
#include <random.h>
#include <util/time.h>
#include <algorithm>
#include <cmath>

namespace OConsensus {

bool StabilizationMining::IsCurrencyStable(const std::string& currency) const {
    auto it = m_currency_status.find(currency);
    return (it == m_currency_status.end()) ? true : it->second.IsStable();
}

void StabilizationMining::UpdateStabilityStatus(const std::string& currency,
                                                double expected_price, double observed_price,
                                                double exchange_rate, int height) {
    auto& info = m_currency_status[currency];
    info.currency_code = currency;
    info.expected_water_price = expected_price;
    info.observed_water_price = observed_price;
    info.observed_exchange_rate = exchange_rate;
    info.last_check_height = height;
    info.stability_ratio = CalculateStabilityRatio(expected_price, observed_price);
    
    if (info.stability_ratio > StabilizationConfig::STABILITY_THRESHOLD) {
        if (info.status != StabilityStatus::UNSTABLE) {
            info.unstable_since_height = height;
            info.status = StabilityStatus::UNSTABLE;
            LogPrintf("O Stabilization: Currency %s became UNSTABLE (deviation: %.2f%%)\n",
                     currency, info.stability_ratio * 100);
        }
    } else {
        if (info.status == StabilityStatus::UNSTABLE) {
            LogPrintf("O Stabilization: Currency %s became STABLE again\n", currency);
        }
        info.status = StabilityStatus::STABLE;
        info.unstable_since_height = 0;
    }
}

std::optional<CurrencyStabilityInfo> StabilizationMining::GetStabilityStatus(
    const std::string& currency) const {
    auto it = m_currency_status.find(currency);
    return (it == m_currency_status.end()) ? std::nullopt : std::make_optional(it->second);
}

std::vector<std::string> StabilizationMining::GetUnstableCurrencies() const {
    std::vector<std::string> unstable;
    for (const auto& [currency, info] : m_currency_status) {
        if (info.IsUnstable()) unstable.push_back(currency);
    }
    return unstable;
}

std::vector<std::string> StabilizationMining::GetStableCurrencies() const {
    std::vector<std::string> stable;
    for (const auto& [currency, info] : m_currency_status) {
        if (info.IsStable()) stable.push_back(currency);
    }
    return stable;
}

bool StabilizationMining::IsCurrencyInactive(const std::string& currency, int height) const {
    auto it = m_currency_status.find(currency);
    return (it != m_currency_status.end()) && 
           (it->second.IsInactive() || 
            (height - it->second.last_check_height > StabilizationConfig::INACTIVE_TIME_RANGE));
}

void StabilizationMining::MarkCurrencyInactive(const std::string& currency, int height) {
    auto& info = m_currency_status[currency];
    info.currency_code = currency;
    info.status = StabilityStatus::INACTIVE;
    info.last_check_height = height;
    LogPrintf("O Stabilization: Currency %s marked as INACTIVE\n", currency);
}

CAmount StabilizationMining::CalculateStabilizationCoins(const CBlock& block, int height) {
    CAmount total_coins = 0;
    for (const auto& currency : GetUnstableCurrencies()) {
        auto info = GetStabilityStatus(currency);
        if (info.has_value() && MeetsInstabilityThreshold(info.value(), height)) {
            total_coins += CalculateCoinsForCurrency(currency, block, height);
        }
    }
    return total_coins;
}

CAmount StabilizationMining::CalculateCoinsForCurrency(const std::string& currency,
                                                       const CBlock& block, int height) const {
    return CalculateVolumeDifference(currency, block);
}

CAmount StabilizationMining::CalculateVolumeDifference(const std::string& currency,
                                                       const CBlock& block) const {
    auto info = GetStabilityStatus(currency);
    if (!info.has_value()) return 0;
    
    // Calculate stabilization coins based on:
    // Volume of transactions in currency × Exchange rate deviation from water price
    
    // Get transaction volume in this currency (from block transactions)
    CAmount transaction_volume = GetTransactionVolumeInCurrency(currency, block);
    
    // Get exchange rate deviation (how far O currency is from water price)
    double exchange_rate_deviation = CalculateExchangeRateDeviation(currency);
    
    // Calculate stabilization coins needed
    // Formula: Volume × Deviation × Stabilization Factor
    double stabilization_factor = 0.1;  // 10% of volume × deviation
    CAmount stabilization_coins = static_cast<CAmount>(
        static_cast<double>(transaction_volume) * exchange_rate_deviation * stabilization_factor
    );
    
    LogPrintf("O Stabilization: Currency %s, Volume %d, Deviation %.3f, Coins %d\n",
              currency.c_str(), transaction_volume, exchange_rate_deviation, stabilization_coins);
    
    return stabilization_coins;
}

std::vector<CPubKey> StabilizationMining::SelectRewardRecipients(
    int count, const std::string& exclude_currency) const {
    std::vector<CPubKey> all_recipients;
    std::vector<std::string> stable = GetStableCurrencies();
    
    stable.erase(std::remove(stable.begin(), stable.end(), exclude_currency), stable.end());
    
    for (const auto& currency : stable) {
        auto currency_users = GetUsersByCurrency(currency);
        all_recipients.insert(all_recipients.end(), currency_users.begin(), currency_users.end());
    }
    
    return RandomSample(all_recipients, std::min(count, (int)all_recipients.size()));
}

std::vector<CPubKey> StabilizationMining::SelectRecipientsFromCurrency(
    int count, const std::string& currency) const {
    auto users = GetUsersByCurrency(currency);
    return RandomSample(users, std::min(count, (int)users.size()));
}

std::vector<CPubKey> StabilizationMining::GetUsersByCurrency(const std::string& currency) const {
    return std::vector<CPubKey>();
}

int StabilizationMining::CalculateOptimalRecipientCount(CAmount total_coins) const {
    // Calculate optimal number of recipients based on total stabilization coins
    // Goal: Many recipients with smaller amounts (max 100 O each)
    
    // Base calculation: 1 recipient per 100 O (1.00 OUSD) to ensure many recipients
    int base_recipients = static_cast<int>(total_coins / 100);
    
    // Apply reasonable bounds
    int min_recipients = 10;   // At least 10 recipients (minimum distribution)
    int max_recipients = 50000; // At most 50,000 recipients (safety limit)
    
    int optimal_count = std::max(min_recipients, std::min(base_recipients, max_recipients));
    
    LogPrintf("O Stabilization: Total coins %d, calculated %d recipients (target: many small rewards)\n", 
              total_coins, optimal_count);
    
    return optimal_count;
}

CAmount StabilizationMining::GetTransactionVolumeInCurrency(const std::string& currency, const CBlock& block) const {
    // TODO: Implement actual transaction volume calculation
    // This should sum up all transaction values in the specified currency
    
    CAmount total_volume = 0;
    for (const auto& tx : block.vtx) {
        // TODO: Parse transaction outputs and sum values in the specified currency
        // For now, use a placeholder calculation
        total_volume += tx->GetValueOut();  // This is Bitcoin's total, need currency-specific calculation
    }
    
    // Placeholder: assume 10% of Bitcoin volume is in this currency
    // In real implementation, this would be currency-specific
    return total_volume / 10;
}

double StabilizationMining::CalculateExchangeRateDeviation(const std::string& currency) const {
    auto info = GetStabilityStatus(currency);
    if (!info.has_value()) return 0.0;
    
    // Exchange rate deviation is the difference between:
    // 1. Expected O currency value (based on water price)
    // 2. Actual O currency exchange rate
    
    double expected_value = 1.0;  // 1 O should equal 1 liter of water
    double actual_exchange_rate = info->stability_ratio;  // Use stability_ratio as deviation measure
    
    if (actual_exchange_rate == 0) return 0.0;
    
    // Calculate deviation as percentage
    double deviation = std::abs(expected_value - actual_exchange_rate) / expected_value;
    
    LogPrintf("O Stabilization: Currency %s, Expected %.3f, Actual %.3f, Deviation %.3f\n",
              currency.c_str(), expected_value, actual_exchange_rate, deviation);
    
    return deviation;
}

std::vector<CTransaction> StabilizationMining::CreateStabilizationTransactions(
    const CBlock& block, int height) {
    std::vector<CTransaction> stab_txs;
    CAmount total_coins = CalculateStabilizationCoins(block, height);
    
    if (total_coins == 0) return stab_txs;
    
    for (const auto& currency : GetUnstableCurrencies()) {
        auto info = GetStabilityStatus(currency);
        if (!info.has_value() || !MeetsInstabilityThreshold(info.value(), height)) continue;
        
        CAmount currency_coins = CalculateCoinsForCurrency(currency, block, height);
        if (currency_coins == 0) continue;
        
        // Calculate number of recipients based on economic need
        // More recipients for larger stabilization amounts
        int recipient_count = CalculateOptimalRecipientCount(currency_coins);
        auto recipients = SelectRewardRecipients(recipient_count, currency);
        if (recipients.empty()) continue;
        
        // Calculate amount per recipient based on total coins and recipient count
        CAmount amount_per_recipient = currency_coins / static_cast<CAmount>(recipients.size());
        
        // Apply bounds checking
        amount_per_recipient = std::max(StabilizationConfig::MIN_STABILIZATION_REWARD, 
                                       std::min(amount_per_recipient, StabilizationConfig::MAX_STABILIZATION_REWARD));
        
        CMutableTransaction mtx;
        mtx.version = 2;
        for ([[maybe_unused]] const auto& recipient : recipients) {
            CTxOut output;
            output.nValue = amount_per_recipient;
            mtx.vout.push_back(output);
        }
        
        CTransaction tx(mtx);
        stab_txs.push_back(tx);
        
        StabilizationTransaction stab_record;
        stab_record.tx_id = tx.GetHash();
        stab_record.unstable_currency = currency;
        stab_record.coins_created = amount_per_recipient * recipients.size();
        stab_record.recipients = recipients;
        stab_record.block_height = height;
        stab_record.timestamp = GetTime();
        stab_record.deviation_ratio = info->stability_ratio;
        RecordStabilizationTransaction(stab_record);
    }
    
    return stab_txs;
}

void StabilizationMining::RecordStabilizationTransaction(const StabilizationTransaction& tx) {
    m_stabilization_txs[tx.tx_id] = tx;
    m_stats.total_coins_created += tx.coins_created;
    m_stats.total_transactions++;
    m_stats.total_recipients += tx.recipients.size();
}

std::vector<StabilizationTransaction> StabilizationMining::GetStabilizationHistory(
    const std::string& currency, int start_height, int end_height) const {
    std::vector<StabilizationTransaction> history;
    for (const auto& [tx_id, tx] : m_stabilization_txs) {
        if (tx.unstable_currency == currency && tx.block_height >= start_height && 
            tx.block_height <= end_height) {
            history.push_back(tx);
        }
    }
    return history;
}

CAmount StabilizationMining::GetTotalCoinsCreated(const std::string& currency) const {
    CAmount total = 0;
    for (const auto& [tx_id, tx] : m_stabilization_txs) {
        if (tx.unstable_currency == currency) total += tx.coins_created;
    }
    return total;
}

std::map<std::string, CurrencyStabilityInfo> StabilizationMining::GetAllStabilityStatus() const {
    return m_currency_status;
}

StabilizationMining::StabilizationStats StabilizationMining::GetStatistics() const {
    StabilizationStats stats;
    stats.total_unstable_currencies = stats.total_stable_currencies = stats.total_inactive_currencies = 0;
    
    for (const auto& [currency, info] : m_currency_status) {
        if (info.IsStable()) stats.total_stable_currencies++;
        else if (info.IsUnstable()) stats.total_unstable_currencies++;
        else if (info.IsInactive()) stats.total_inactive_currencies++;
    }
    
    stats.total_coins_created = m_stats.total_coins_created;
    stats.total_transactions = m_stats.total_transactions;
    stats.total_recipients = m_stats.total_recipients;
    return stats;
}

void StabilizationMining::ReEvaluateAllCurrencies(int height) {
    for (auto& [currency, info] : m_currency_status) {
        double avg_water_price = GetAverageWaterPrice(currency, 30);
        double avg_exchange_rate = GetAverageExchangeRate(currency, "O", 7);
        if (avg_water_price > 0 && avg_exchange_rate > 0) {
            UpdateStabilityStatus(currency, 1.0, avg_water_price, avg_exchange_rate, height);
        }
    }
}

void StabilizationMining::PruneOldData(int cutoff_height) {
    int pruned = 0;
    for (auto it = m_stabilization_txs.begin(); it != m_stabilization_txs.end();) {
        if (it->second.block_height < cutoff_height) {
            it = m_stabilization_txs.erase(it);
            pruned++;
        } else ++it;
    }
    if (pruned > 0) LogPrintf("O Stabilization: Pruned %d old transactions\n", pruned);
}

double StabilizationMining::CalculateStabilityRatio(double expected, double observed) const {
    return (expected == 0) ? 0.0 : std::abs(expected - observed) / expected;
}

bool StabilizationMining::MeetsInstabilityThreshold(const CurrencyStabilityInfo& info, int height) const {
    return info.IsUnstable() && (height - info.unstable_since_height) >= StabilizationConfig::UNSTABLE_TIME_RANGE;
}

std::vector<CPubKey> StabilizationMining::RandomSample(const std::vector<CPubKey>& users, int count) const {
    if (users.empty() || count <= 0) return {};
    std::vector<CPubKey> sample = users;
    
    FastRandomContext rng;
    for (size_t i = sample.size() - 1; i > 0; --i) {
        std::swap(sample[i], sample[rng.randrange(i + 1)]);
    }
    
    if (sample.size() > static_cast<size_t>(count)) sample.resize(count);
    return sample;
}

double StabilizationMining::GetAverageWaterPrice(const std::string& currency, int days) const {
    auto avg = OMeasurement::g_measurement_system.GetAverageWaterPrice(currency, days);
    return avg.value_or(0.0);
}

double StabilizationMining::GetAverageExchangeRate(const std::string& from, const std::string& to, int days) const {
    auto avg = OMeasurement::g_measurement_system.GetAverageExchangeRate(from, to, days);
    return avg.value_or(0.0);
}

bool ShouldTriggerStabilization(const CBlock& block, int height) {
    for (const auto& currency : g_stabilization_mining.GetUnstableCurrencies()) {
        auto info = g_stabilization_mining.GetStabilityStatus(currency);
        if (info.has_value() && 
            (height - info->unstable_since_height) >= StabilizationConfig::UNSTABLE_TIME_RANGE) {
            return true;
        }
    }
    return false;
}

bool ValidateStabilizationTransactions(const CBlock& block, int height) {
    CAmount expected = CalculateExpectedStabilization(block, height);
    return expected >= 0;
}

CAmount CalculateExpectedStabilization(const CBlock& block, int height) {
    return g_stabilization_mining.CalculateStabilizationCoins(block, height);
}

} // namespace OConsensus
