// Copyright (c) 2024 The Bitcoin Core developers
// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/currency_exchange.h>
#include <consensus/amount.h>
#include <consensus/o_amount.h>
#include <measurement/measurement_system.h>
#include <hash.h>
#include <logging.h>
#include <random.h>
#include <util/time.h>
#include <util/strencodings.h>

#include <algorithm>
#include <cmath>

// Global instance
CurrencyExchangeManager g_currency_exchange_manager;

// ===== CurrencyExchange Implementation =====

uint256 CurrencyExchange::GetHash() const {
    HashWriter ss{};
    ss << exchange_id << from_user << to_user << from_currency << to_currency
       << from_amount << to_amount << static_cast<int64_t>(exchange_rate * 1000000) << timestamp;
    return ss.GetHash();
}

bool CurrencyExchange::IsValid() const {
    if (exchange_id.IsNull()) return false;
    if (from_currency.empty() || to_currency.empty()) return false;
    if (from_currency == to_currency) return false;
    if (from_amount <= 0 || to_amount <= 0) return false;
    if (exchange_rate <= 0.0) return false;
    if (timestamp <= 0) return false;
    return true;
}

std::string CurrencyExchange::ToString() const {
    return strprintf("Exchange[%s]: %s %.2f -> %s %.2f (rate: %.6f)",
                     exchange_id.ToString().substr(0, 8),
                     from_currency, static_cast<double>(from_amount) / OAmount::O_COIN,
                     to_currency, static_cast<double>(to_amount) / OAmount::O_COIN,
                     exchange_rate);
}

// ===== CurrencyExchangeManager Implementation =====

CurrencyExchangeManager::CurrencyExchangeManager() {
    m_stats.total_exchanges = 0;
    m_stats.total_volume = 0;
}

CurrencyExchangeManager::~CurrencyExchangeManager() = default;

std::optional<CurrencyExchange> CurrencyExchangeManager::CreateExchange(
    const CPubKey& from_user,
    const CPubKey& to_user,
    const std::string& from_currency,
    const std::string& to_currency,
    CAmount from_amount,
    const std::string& memo) {
    
    // Validate inputs
    if (from_user.size() == 0 || to_user.size() == 0) {
        LogPrintf("O Exchange: Invalid user keys\n");
        return std::nullopt;
    }
    
    if (from_currency.empty() || to_currency.empty()) {
        LogPrintf("O Exchange: Invalid currency codes\n");
        return std::nullopt;
    }
    
    if (from_currency == to_currency) {
        LogPrintf("O Exchange: Cannot exchange same currency\n");
        return std::nullopt;
    }
    
    if (from_amount < ExchangeConfig::MIN_EXCHANGE_AMOUNT) {
        LogPrintf("O Exchange: Amount too small: %d (min: %d)\n", 
                  from_amount, ExchangeConfig::MIN_EXCHANGE_AMOUNT);
        return std::nullopt;
    }
    
    if (from_amount > ExchangeConfig::MAX_EXCHANGE_AMOUNT) {
        LogPrintf("O Exchange: Amount too large: %d (max: %d)\n", 
                  from_amount, ExchangeConfig::MAX_EXCHANGE_AMOUNT);
        return std::nullopt;
    }
    
    if (!IsCurrencySupported(from_currency) || !IsCurrencySupported(to_currency)) {
        LogPrintf("O Exchange: Unsupported currency: %s -> %s\n", 
                  from_currency.c_str(), to_currency.c_str());
        return std::nullopt;
    }
    
    // Get current exchange rate
    auto current_rate = GetCurrentExchangeRate(from_currency, to_currency);
    if (!current_rate.has_value()) {
        LogPrintf("O Exchange: No exchange rate available for %s -> %s\n", 
                  from_currency.c_str(), to_currency.c_str());
        return std::nullopt;
    }
    
    // Create exchange
    CurrencyExchange exchange;
    exchange.exchange_id = GetRandHash();
    exchange.from_user = from_user;
    exchange.to_user = to_user;
    exchange.from_currency = from_currency;
    exchange.to_currency = to_currency;
    exchange.from_amount = from_amount;
    exchange.exchange_rate = current_rate.value();
    exchange.to_amount = CalculateExchangeAmount(from_amount, exchange.exchange_rate);
    exchange.timestamp = GetTime();
    exchange.memo = memo;
    exchange.is_executed = false;
    exchange.is_validated = false;
    
    // Validate exchange
    if (!exchange.IsValid()) {
        LogPrintf("O Exchange: Created exchange is invalid\n");
        return std::nullopt;
    }
    
    // Store exchange
    m_exchanges[exchange.exchange_id] = exchange;
    
    LogPrintf("O Exchange: Created exchange %s\n", exchange.ToString().c_str());
    
    return exchange;
}

bool CurrencyExchangeManager::ValidateExchangeRate(
    const std::string& from_currency,
    const std::string& to_currency,
    double proposed_rate) const {
    
    if (proposed_rate <= 0.0) {
        return false;
    }
    
    // Get measured rate from measurement system
    // The measurement system already guarantees quality through its invitation system
    // and Gaussian filtering, so we trust its average calculation
    auto measured_rate = OMeasurement::g_measurement_system.GetAverageExchangeRate(
        from_currency, to_currency, 7); // Last 7 days
    
    if (!measured_rate.has_value()) {
        LogPrintf("O Exchange: No measured rate for %s -> %s (measurement system has no data)\n", 
                  from_currency.c_str(), to_currency.c_str());
        return false;
    }
    
    // Check deviation
    double deviation = GetRateDeviation(from_currency, to_currency, proposed_rate);
    if (deviation > ExchangeConfig::MAX_RATE_DEVIATION) {
        LogPrintf("O Exchange: Rate deviation too high: %.3f%% (max: %.3f%%)\n", 
                  deviation * 100.0, ExchangeConfig::MAX_RATE_DEVIATION * 100.0);
        return false;
    }
    
    return true;
}

std::optional<double> CurrencyExchangeManager::GetCurrentExchangeRate(
    const std::string& from_currency,
    const std::string& to_currency) const {
    
    // If both are O currencies, calculate cross-O rate
    if (IsOCurrency(from_currency) && IsOCurrency(to_currency)) {
        return CalculateOCurrencyExchangeRate(from_currency, to_currency);
    }
    
    // If one is O currency and one is fiat, validate the pair
    if (IsOCurrency(from_currency) && !IsOCurrency(to_currency)) {
        // Validate O currency to corresponding fiat currency pair
        if (!OMeasurement::g_measurement_system.IsValidOCurrencyToFiatPair(from_currency, to_currency)) {
            LogPrintf("O Exchange: Invalid currency pair %s/%s - must be O currency to corresponding fiat\n",
                      from_currency.c_str(), to_currency.c_str());
            return std::nullopt;
        }
    }
    
    // Try to get from measurement system first
    auto measured_rate = OMeasurement::g_measurement_system.GetAverageExchangeRate(
        from_currency, to_currency, 7); // Last 7 days
    
    if (measured_rate.has_value()) {
        return measured_rate.value();
    }
    
    // Fallback to stored rate
    std::string rate_key = MakeRateKey(from_currency, to_currency);
    auto it = m_exchange_rates.find(rate_key);
    if (it != m_exchange_rates.end() && !IsRateExpired(rate_key)) {
        return it->second;
    }
    
    return std::nullopt;
}

bool CurrencyExchangeManager::ExecuteExchange(const uint256& exchange_id, const CTransaction& tx) {
    auto it = m_exchanges.find(exchange_id);
    if (it == m_exchanges.end()) {
        LogPrintf("O Exchange: Exchange not found: %s\n", exchange_id.ToString().c_str());
        return false;
    }
    
    auto& exchange = it->second;
    
    if (exchange.is_executed) {
        LogPrintf("O Exchange: Exchange already executed: %s\n", exchange_id.ToString().c_str());
        return false;
    }
    
    // Validate transaction
    if (!ValidateExchangeTransaction(tx)) {
        LogPrintf("O Exchange: Invalid transaction for exchange: %s\n", exchange_id.ToString().c_str());
        return false;
    }
    
    // Execute exchange
    exchange.is_executed = true;
    exchange.tx_hash = tx.GetHash();
    exchange.is_validated = true;
    
    // Update statistics
    UpdateStatistics(exchange);
    
    LogPrintf("O Exchange: Executed exchange %s\n", exchange.ToString().c_str());
    
    return true;
}

bool CurrencyExchangeManager::ValidateExchangeTransaction(const CTransaction& tx) const {
    // Basic transaction validation
    if (tx.vin.empty() || tx.vout.empty()) {
        return false;
    }
    
    // Check if transaction has proper structure for exchange
    // This is a simplified validation - in practice, you'd check:
    // 1. Input amounts match exchange from_amount
    // 2. Output amounts match exchange to_amount
    // 3. Proper script types for multi-currency
    // 4. Signatures are valid
    
    return true;
}

std::optional<CurrencyExchange> CurrencyExchangeManager::GetExchange(const uint256& exchange_id) const {
    auto it = m_exchanges.find(exchange_id);
    if (it == m_exchanges.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::vector<CurrencyExchange> CurrencyExchangeManager::GetUserExchanges(const CPubKey& user) const {
    std::vector<CurrencyExchange> user_exchanges;
    
    for (const auto& [id, exchange] : m_exchanges) {
        if (exchange.from_user == user || exchange.to_user == user) {
            user_exchanges.push_back(exchange);
        }
    }
    
    return user_exchanges;
}

std::vector<CurrencyExchange> CurrencyExchangeManager::GetExchangesInRange(
    int64_t start_time, int64_t end_time) const {
    
    std::vector<CurrencyExchange> range_exchanges;
    
    for (const auto& [id, exchange] : m_exchanges) {
        if (exchange.timestamp >= start_time && exchange.timestamp <= end_time) {
            range_exchanges.push_back(exchange);
        }
    }
    
    return range_exchanges;
}

void CurrencyExchangeManager::UpdateExchangeRate(
    const std::string& from_currency,
    const std::string& to_currency,
    double rate,
    int64_t timestamp) {
    
    std::string rate_key = MakeRateKey(from_currency, to_currency);
    m_exchange_rates[rate_key] = rate;
    m_rate_timestamps[rate_key] = timestamp;
    
    LogPrintf("O Exchange: Updated rate %s -> %s: %.6f\n", 
              from_currency.c_str(), to_currency.c_str(), rate);
}

bool CurrencyExchangeManager::IsExchangeRateValid(
    const std::string& from_currency,
    const std::string& to_currency) const {
    
    std::string rate_key = MakeRateKey(from_currency, to_currency);
    auto it = m_exchange_rates.find(rate_key);
    
    if (it == m_exchange_rates.end()) {
        return false;
    }
    
    return !IsRateExpired(rate_key);
}

double CurrencyExchangeManager::GetRateDeviation(
    const std::string& from_currency,
    const std::string& to_currency,
    double proposed_rate) const {
    
    auto measured_rate = GetCurrentExchangeRate(from_currency, to_currency);
    if (!measured_rate.has_value()) {
        return 1.0; // 100% deviation if no rate available
    }
    
    double deviation = std::abs(proposed_rate - measured_rate.value()) / measured_rate.value();
    return deviation;
}

std::map<std::string, int64_t> CurrencyExchangeManager::GetExchangeStatistics() const {
    std::map<std::string, int64_t> stats;
    stats["total_exchanges"] = m_stats.total_exchanges;
    stats["total_volume"] = m_stats.total_volume;
    
    for (const auto& [pair, count] : m_stats.exchanges_by_pair) {
        stats["exchanges_" + pair] = count;
    }
    
    return stats;
}

std::map<std::string, CAmount> CurrencyExchangeManager::GetExchangeVolumeByPair() const {
    return m_stats.volume_by_pair;
}

CAmount CurrencyExchangeManager::GetDailyExchangeVolume(const std::string& date) const {
    // Parse date and get exchanges for that day
    int64_t start_time = 0; // TODO: Parse date
    int64_t end_time = start_time + 86400; // 24 hours
    
    CAmount daily_volume = 0;
    auto exchanges = GetExchangesInRange(start_time, end_time);
    
    for (const auto& exchange : exchanges) {
        daily_volume += exchange.from_amount;
    }
    
    return daily_volume;
}

CAmount CurrencyExchangeManager::CalculateExchangeAmount(
    CAmount from_amount,
    double exchange_rate) const {
    
    return static_cast<CAmount>(static_cast<double>(from_amount) * exchange_rate);
}

std::string CurrencyExchangeManager::FormatExchangeRate(double rate) const {
    return strprintf("%.6f", rate);
}

bool CurrencyExchangeManager::IsCurrencySupported(const std::string& currency) const {
    auto supported = GetSupportedCurrencies();
    return std::find(supported.begin(), supported.end(), currency) != supported.end();
}

std::vector<std::pair<std::string, std::string>> CurrencyExchangeManager::GetSupportedPairs() const {
    std::vector<std::pair<std::string, std::string>> pairs;
    auto currencies = GetSupportedCurrencies();
    
    for (size_t i = 0; i < currencies.size(); ++i) {
        for (size_t j = i + 1; j < currencies.size(); ++j) {
            pairs.emplace_back(currencies[i], currencies[j]);
            pairs.emplace_back(currencies[j], currencies[i]);
        }
    }
    
    return pairs;
}

void CurrencyExchangeManager::PruneOldData(int64_t cutoff_time) {
    auto it = m_exchanges.begin();
    while (it != m_exchanges.end()) {
        if (it->second.timestamp < cutoff_time) {
            it = m_exchanges.erase(it);
        } else {
            ++it;
        }
    }
    
    // Also prune old rates
    auto rate_it = m_rate_timestamps.begin();
    while (rate_it != m_rate_timestamps.end()) {
        if (rate_it->second < cutoff_time) {
            m_exchange_rates.erase(rate_it->first);
            rate_it = m_rate_timestamps.erase(rate_it);
        } else {
            ++rate_it;
        }
    }
}

void CurrencyExchangeManager::ClearAllData() {
    m_exchanges.clear();
    m_exchange_rates.clear();
    m_rate_timestamps.clear();
    m_stats = ExchangeStats{};
}

// ===== Private Helper Functions =====

std::string CurrencyExchangeManager::MakeRateKey(const std::string& from, const std::string& to) const {
    return from + "_" + to;
}

bool CurrencyExchangeManager::IsRateExpired(const std::string& rate_key) const {
    auto it = m_rate_timestamps.find(rate_key);
    if (it == m_rate_timestamps.end()) {
        return true;
    }
    
    int64_t current_time = GetTime();
    int64_t expiry_time = it->second + (ExchangeConfig::RATE_VALIDITY_HOURS * 3600);
    
    return current_time > expiry_time;
}

void CurrencyExchangeManager::UpdateStatistics(const CurrencyExchange& exchange) {
    m_stats.total_exchanges++;
    m_stats.total_volume += exchange.from_amount;
    
    std::string pair = exchange.from_currency + "_" + exchange.to_currency;
    m_stats.exchanges_by_pair[pair]++;
    m_stats.volume_by_pair[pair] += exchange.from_amount;
}

std::vector<std::string> CurrencyExchangeManager::GetSupportedCurrencies() const {
    return {
        "OUSD", "OEUR", "OJPY", "OGBP", "OCNY", "OCAD", "OAUD", "OCHF", "ONZD",
        "OSEK", "ONOK", "ODKK", "OPLN", "OCZK", "OHUF", "OKRW", "OSGD", "OHKD",
        "OTWD", "OTHB", "OMYR", "OIDR", "OPHP", "OVND", "OINR", "OBRL", "ORUB",
        "OZAR", "OTRY", "OEGP", "OSAR", "OAED", "OILS", "OQAR", "OKWD", "OBHD",
        "OOMR", "OJOD", "OLBP", "OMAD", "OTND", "ODZD", "OMRO", "OLYD", "OXOF",
        "OXAF", "OXPF", "OALL", "OAMD", "OAZN", "OBYN", "OBGN", "OBIF", "OKHR",
        "OKGS", "OKZT", "OLAK", "OLSL", "OLTL", "OMDL", "OMKD", "OMNT", "ORON",
        "ORSD", "OTJS", "OTMT", "OUAH", "OUZS", "OXDR", "OZWL"
    };
}

// ===== Cross-O Currency Exchange Rate Calculation =====

std::optional<double> CurrencyExchangeManager::CalculateOCurrencyExchangeRate(
    const std::string& from_o_currency,
    const std::string& to_o_currency) const {
    
    if (!IsOCurrency(from_o_currency) || !IsOCurrency(to_o_currency)) {
        LogPrintf("O Exchange: Both currencies must be O currencies for cross-O calculation\n");
        return std::nullopt;
    }
    
    if (from_o_currency == to_o_currency) {
        return 1.0; // Same currency, 1:1 rate
    }
    
    // Get corresponding fiat currencies
    std::string from_fiat = GetCorrespondingFiatCurrency(from_o_currency);
    std::string to_fiat = GetCorrespondingFiatCurrency(to_o_currency);
    
    // Get O currency to fiat rates (should be 1:1 if stable)
    auto from_o_to_fiat_rate = GetCurrentExchangeRate(from_o_currency, from_fiat);
    auto to_o_to_fiat_rate = GetCurrentExchangeRate(to_o_currency, to_fiat);
    
    if (!from_o_to_fiat_rate.has_value() || !to_o_to_fiat_rate.has_value()) {
        LogPrintf("O Exchange: Missing O currency to fiat rates for cross-O calculation\n");
        return std::nullopt;
    }
    
    // Get fiat exchange rate
    auto fiat_exchange_rate = GetFiatExchangeRate(from_fiat, to_fiat);
    if (!fiat_exchange_rate.has_value()) {
        LogPrintf("O Exchange: Missing fiat exchange rate %s/%s\n", 
                  from_fiat.c_str(), to_fiat.c_str());
        return std::nullopt;
    }
    
    // Calculate: OUSD/OEUR = (OUSD/USD) * (USD/EUR) / (OEUR/EUR)
    double cross_o_rate = from_o_to_fiat_rate.value() * fiat_exchange_rate.value() / to_o_to_fiat_rate.value();
    
    LogPrintf("O Exchange: Cross-O rate %s/%s = %.6f "
              "(via %s:%.4f * %s/%s:%.4f / %s:%.4f)\n",
              from_o_currency.c_str(), to_o_currency.c_str(), cross_o_rate,
              from_o_currency.c_str(), from_o_to_fiat_rate.value(),
              from_fiat.c_str(), to_fiat.c_str(), fiat_exchange_rate.value(),
              to_o_currency.c_str(), to_o_to_fiat_rate.value());
    
    return cross_o_rate;
}

std::optional<double> CurrencyExchangeManager::GetFiatExchangeRate(
    const std::string& from_fiat,
    const std::string& to_fiat) const {
    
    if (from_fiat == to_fiat) {
        return 1.0;
    }
    
    // Try to get from external sources (this would be implemented with real API calls)
    // For now, return a placeholder that would come from external financial APIs
    
    LogPrintf("O Exchange: Getting fiat exchange rate %s/%s from external sources\n",
              from_fiat.c_str(), to_fiat.c_str());
    
    // TODO: Implement actual external API calls to get real fiat exchange rates
    // This would typically call services like:
    // - CoinGecko API
    // - Fixer.io
    // - ExchangeRate-API
    // - Central bank APIs
    
    // Placeholder implementation
    std::map<std::string, double> placeholder_rates = {
        {"USD_EUR", 0.85},
        {"EUR_USD", 1.18},
        {"USD_JPY", 110.0},
        {"JPY_USD", 0.0091},
        {"USD_GBP", 0.73},
        {"GBP_USD", 1.37},
        {"EUR_JPY", 129.4},
        {"JPY_EUR", 0.0077},
        {"EUR_GBP", 0.86},
        {"GBP_EUR", 1.16},
        {"USD_CAD", 1.25},
        {"CAD_USD", 0.80},
        {"USD_AUD", 1.35},
        {"AUD_USD", 0.74}
    };
    
    std::string rate_key = from_fiat + "_" + to_fiat;
    auto it = placeholder_rates.find(rate_key);
    if (it != placeholder_rates.end()) {
        return it->second;
    }
    
    LogPrintf("O Exchange: No fiat exchange rate available for %s/%s\n",
              from_fiat.c_str(), to_fiat.c_str());
    return std::nullopt;
}

bool CurrencyExchangeManager::IsOCurrency(const std::string& currency) const {
    return currency.length() > 1 && currency[0] == 'O' && std::isupper(currency[1]);
}

std::string CurrencyExchangeManager::GetCorrespondingFiatCurrency(const std::string& o_currency) const {
    if (IsOCurrency(o_currency)) {
        return o_currency.substr(1); // Remove 'O' prefix
    }
    return o_currency;
}
