// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/measurement_readiness.h>
#include <logging.h>
#include <util/time.h>
#include <util/moneystr.h>

namespace OConsensus {

MeasurementReadinessManager g_measurement_readiness_manager;

MeasurementReadinessManager::MeasurementReadinessManager() {
    LogPrintf("O Measurement Readiness Manager: Initialized.\n");
}

void MeasurementReadinessManager::UpdateUserCount(const std::string& o_currency, int user_count) {
    LOCK(cs_readiness);
    
    m_user_counts[o_currency] = user_count;
    m_last_updates[o_currency] = GetTime();
    
    // Update readiness status
    UpdateReadinessStatus(o_currency);
    
    LogPrintf("O Measurement Readiness: %s user count updated to %d\n", 
              o_currency.c_str(), user_count);
}

void MeasurementReadinessManager::UpdateCoinSupply(const std::string& o_currency, CAmount total_supply) {
    LOCK(cs_readiness);
    
    m_coin_supplies[o_currency] = total_supply;
    m_last_updates[o_currency] = GetTime();
    
    // Update readiness status
    UpdateReadinessStatus(o_currency);
    
    LogPrintf("O Measurement Readiness: %s coin supply updated to %s\n", 
              o_currency.c_str(), FormatMoney(total_supply));
}

bool MeasurementReadinessManager::IsWaterPriceMeasurementReady(const std::string& o_currency, int height) const {
    LOCK(cs_readiness);
    
    auto it = m_user_counts.find(o_currency);
    if (it == m_user_counts.end()) {
        return false;
    }
    
    int user_count = it->second;
    
    // Bootstrap mode: Lower threshold for early blocks
    if (height < BOOTSTRAP_HEIGHT_THRESHOLD) {
        bool ready = user_count >= BOOTSTRAP_MIN_USERS;
        if (ready && height % 100 == 0) {  // Log periodically
            LogPrintf("O Measurement Readiness: %s in BOOTSTRAP mode (height %d) - %d users (threshold: %d)\n",
                     o_currency.c_str(), height, user_count, BOOTSTRAP_MIN_USERS);
        }
        return ready;
    }
    
    // Mature mode: Full threshold
    return user_count >= MIN_USERS_FOR_WATER_PRICE_MEASUREMENTS;
}

bool MeasurementReadinessManager::IsExchangeRateMeasurementReady(const std::string& o_currency) const {
    LOCK(cs_readiness);
    
    auto it = m_coin_supplies.find(o_currency);
    if (it == m_coin_supplies.end()) {
        return false;
    }
    
    return it->second >= MIN_COINS_FOR_EXCHANGE_RATE_MEASUREMENTS;
}

std::string MeasurementReadinessManager::GetReadinessStatus(const std::string& o_currency) const {
    LOCK(cs_readiness);
    
    auto it = m_readiness_status.find(o_currency);
    if (it == m_readiness_status.end()) {
        return "not_tracked";
    }
    
    return it->second;
}

int MeasurementReadinessManager::GetUserCount(const std::string& o_currency) const {
    LOCK(cs_readiness);
    
    auto it = m_user_counts.find(o_currency);
    if (it == m_user_counts.end()) {
        return 0;
    }
    
    return it->second;
}

CAmount MeasurementReadinessManager::GetCoinSupply(const std::string& o_currency) const {
    LOCK(cs_readiness);
    
    auto it = m_coin_supplies.find(o_currency);
    if (it == m_coin_supplies.end()) {
        return 0;
    }
    
    return it->second;
}

std::map<std::string, int> MeasurementReadinessManager::GetReadinessStatistics() const {
    LOCK(cs_readiness);
    
    std::map<std::string, int> stats;
    
    int total_currencies = m_user_counts.size();
    int water_price_ready = 0;
    int exchange_rate_ready = 0;
    int fully_ready = 0;
    
    for (const auto& pair : m_user_counts) {
        const std::string& o_currency = pair.first;
        
        bool wp_ready = IsWaterPriceMeasurementReady(o_currency);
        bool er_ready = IsExchangeRateMeasurementReady(o_currency);
        
        if (wp_ready) water_price_ready++;
        if (er_ready) exchange_rate_ready++;
        if (wp_ready && er_ready) fully_ready++;
    }
    
    stats["total_currencies_tracked"] = total_currencies;
    stats["water_price_ready_count"] = water_price_ready;
    stats["exchange_rate_ready_count"] = exchange_rate_ready;
    stats["fully_ready_count"] = fully_ready;
    stats["minimum_users_for_water_price"] = MIN_USERS_FOR_WATER_PRICE_MEASUREMENTS;
    stats["minimum_coins_for_exchange_rate"] = MIN_COINS_FOR_EXCHANGE_RATE_MEASUREMENTS / COIN; // Convert to O coins
    
    return stats;
}

std::map<std::string, std::string> MeasurementReadinessManager::GetDetailedReadinessStatus(const std::string& o_currency) const {
    LOCK(cs_readiness);
    
    std::map<std::string, std::string> result;
    
    result["o_currency"] = o_currency;
    result["user_count"] = std::to_string(GetUserCount(o_currency));
    result["coin_supply"] = FormatMoney(GetCoinSupply(o_currency));
    result["water_price_ready"] = IsWaterPriceMeasurementReady(o_currency) ? "true" : "false";
    result["exchange_rate_ready"] = IsExchangeRateMeasurementReady(o_currency) ? "true" : "false";
    result["fully_ready"] = IsFullyReady(o_currency) ? "true" : "false";
    result["readiness_status"] = GetReadinessStatus(o_currency);
    
    // Add progress information
    int user_count = GetUserCount(o_currency);
    CAmount coin_supply = GetCoinSupply(o_currency);
    
    double user_progress = (double)user_count / MIN_USERS_FOR_WATER_PRICE_MEASUREMENTS * 100.0;
    double coin_progress = (double)coin_supply / MIN_COINS_FOR_EXCHANGE_RATE_MEASUREMENTS * 100.0;
    
    result["user_progress_percent"] = std::to_string(user_progress);
    result["coin_progress_percent"] = std::to_string(coin_progress);
    
    // Add last update time
    auto it = m_last_updates.find(o_currency);
    if (it != m_last_updates.end()) {
        result["last_updated"] = std::to_string(it->second);
    }
    
    return result;
}

bool MeasurementReadinessManager::IsFullyReady(const std::string& o_currency) const {
    return IsWaterPriceMeasurementReady(o_currency) && IsExchangeRateMeasurementReady(o_currency);
}

std::vector<std::string> MeasurementReadinessManager::GetReadyForWaterPriceMeasurements() const {
    LOCK(cs_readiness);
    
    std::vector<std::string> ready_currencies;
    
    for (const auto& pair : m_user_counts) {
        const std::string& o_currency = pair.first;
        if (IsWaterPriceMeasurementReady(o_currency)) {
            ready_currencies.push_back(o_currency);
        }
    }
    
    return ready_currencies;
}

std::vector<std::string> MeasurementReadinessManager::GetReadyForExchangeRateMeasurements() const {
    LOCK(cs_readiness);
    
    std::vector<std::string> ready_currencies;
    
    for (const auto& pair : m_coin_supplies) {
        const std::string& o_currency = pair.first;
        if (IsExchangeRateMeasurementReady(o_currency)) {
            ready_currencies.push_back(o_currency);
        }
    }
    
    return ready_currencies;
}

std::vector<std::string> MeasurementReadinessManager::GetFullyReadyCurrencies() const {
    LOCK(cs_readiness);
    
    std::vector<std::string> fully_ready_currencies;
    
    for (const auto& pair : m_user_counts) {
        const std::string& o_currency = pair.first;
        if (IsFullyReady(o_currency)) {
            fully_ready_currencies.push_back(o_currency);
        }
    }
    
    return fully_ready_currencies;
}

void MeasurementReadinessManager::UpdateReadinessStatus(const std::string& o_currency) {
    LOCK(cs_readiness);
    
    bool water_price_ready = IsWaterPriceMeasurementReady(o_currency);
    bool exchange_rate_ready = IsExchangeRateMeasurementReady(o_currency);
    
    std::string status = GetReadinessStatusString(water_price_ready, exchange_rate_ready);
    m_readiness_status[o_currency] = status;
    
    LogPrintf("O Measurement Readiness: %s status updated to %s\n", 
              o_currency.c_str(), status.c_str());
}

std::string MeasurementReadinessManager::GetReadinessStatusString(bool water_price_ready, bool exchange_rate_ready) const {
    if (water_price_ready && exchange_rate_ready) {
        return "fully_ready";
    } else if (water_price_ready) {
        return "water_price_ready";
    } else if (exchange_rate_ready) {
        return "exchange_rate_ready";
    } else {
        return "not_ready";
    }
}

} // namespace OConsensus
