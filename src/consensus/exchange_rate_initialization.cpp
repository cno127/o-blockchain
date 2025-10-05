// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/exchange_rate_initialization.h>
#include <measurement/measurement_system.h>
#include <logging.h>
#include <util/time.h>

using namespace OMeasurement;

namespace OConsensus {

ExchangeRateInitializationManager g_exchange_rate_init_manager;

ExchangeRateInitializationManager::ExchangeRateInitializationManager() {
    LogPrintf("O Exchange Rate Init: Initialized exchange rate initialization manager\n");
}

void ExchangeRateInitializationManager::InitializeExchangeRates() {
    LogPrintf("O Exchange Rate Init: Initializing exchange rates with theoretical water prices\n");
    
    // List of O currencies to initialize
    std::vector<std::string> o_currencies = {
        "OUSD", "OEUR", "OJPY", "OGBP", "OAUD", "OCAD", "OCHF", "OCNY", "OINR", "OBRL",
        "OKRW", "OMXN", "OSGD", "OHKD", "ONOK", "OSEK", "ODKK", "OPLN", "OCZK", "OHUF",
        "OILS", "OZAR", "OTRY", "ORUB", "OIDR", "OMYR", "OPHP", "OTHB", "OVND", "OEGP"
    };
    
    for (const auto& o_currency : o_currencies) {
        InitializeExchangeRateForCurrency(o_currency);
    }
    
    LogPrintf("O Exchange Rate Init: Initialized %d O currency exchange rates\n", 
              static_cast<int>(o_currencies.size()));
}

void ExchangeRateInitializationManager::InitializeExchangeRateForCurrency(const std::string& o_currency) {
    // Get the corresponding fiat currency
    std::string fiat_currency = g_measurement_system.GetCorrespondingFiatCurrency(o_currency);
    
    if (fiat_currency.empty()) {
        LogPrintf("O Exchange Rate Init: No corresponding fiat currency for %s\n", o_currency.c_str());
        return;
    }
    
    // Get theoretical exchange rate from water price
    double theoretical_rate = GetTheoreticalExchangeRate(o_currency);
    
    if (theoretical_rate > 0) {
        // Initialize with theoretical rate (initialization fallback)
        m_exchange_rates[o_currency][fiat_currency] = theoretical_rate;
        m_exchange_rate_status[o_currency][fiat_currency] = "theoretical_initialization";
        m_measurement_counts[o_currency][fiat_currency] = 0;
        m_last_measurement_times[o_currency][fiat_currency] = 0; // No measurements yet
        
        LogPrintf("O Exchange Rate Init: %s/%s = %.4f (theoretical initialization, based on water price)\n",
                  o_currency.c_str(), fiat_currency.c_str(), theoretical_rate);
    } else {
        // Fallback to 1:1 if no water price data available (initialization fallback)
        m_exchange_rates[o_currency][fiat_currency] = 1.0;
        m_exchange_rate_status[o_currency][fiat_currency] = "theoretical_initialization_fallback";
        m_measurement_counts[o_currency][fiat_currency] = 0;
        m_last_measurement_times[o_currency][fiat_currency] = 0; // No measurements yet
        
        LogPrintf("O Exchange Rate Init: %s/%s = 1.0000 (theoretical initialization fallback, no water price data)\n",
                  o_currency.c_str(), fiat_currency.c_str());
    }
}

std::optional<double> ExchangeRateInitializationManager::GetExchangeRate(const std::string& o_currency, const std::string& fiat_currency) const {
    auto o_it = m_exchange_rates.find(o_currency);
    if (o_it == m_exchange_rates.end()) {
        return std::nullopt;
    }
    
    auto fiat_it = o_it->second.find(fiat_currency);
    if (fiat_it == o_it->second.end()) {
        return std::nullopt;
    }
    
    return fiat_it->second;
}

bool ExchangeRateInitializationManager::HasMinimumMeasurements(const std::string& o_currency, const std::string& fiat_currency) const {
    int measurement_count = GetMeasurementCount(o_currency, fiat_currency);
    return measurement_count >= MIN_MEASUREMENTS_FOR_RATE;
}

std::string ExchangeRateInitializationManager::GetExchangeRateStatus(const std::string& o_currency, const std::string& fiat_currency) const {
    auto o_it = m_exchange_rate_status.find(o_currency);
    if (o_it == m_exchange_rate_status.end()) {
        return "not_initialized";
    }
    
    auto fiat_it = o_it->second.find(fiat_currency);
    if (fiat_it == o_it->second.end()) {
        return "not_initialized";
    }
    
    return fiat_it->second;
}

std::map<std::string, double> ExchangeRateInitializationManager::GetAllExchangeRates() const {
    std::map<std::string, double> all_rates;
    
    for (const auto& [o_currency, fiat_rates] : m_exchange_rates) {
        for (const auto& [fiat_currency, rate] : fiat_rates) {
            std::string key = o_currency + "/" + fiat_currency;
            all_rates[key] = rate;
        }
    }
    
    return all_rates;
}

std::map<std::string, int> ExchangeRateInitializationManager::GetExchangeRateStatistics() const {
    std::map<std::string, int> stats;
    
    int theoretical_count = 0;
    int measured_count = 0;
    int insufficient_data_count = 0;
    
    for (const auto& [o_currency, fiat_statuses] : m_exchange_rate_status) {
        for (const auto& [fiat_currency, status] : fiat_statuses) {
            if (status == "theoretical" || status == "theoretical_fallback") {
                theoretical_count++;
            } else if (status == "measured") {
                measured_count++;
            } else {
                insufficient_data_count++;
            }
        }
    }
    
    stats["theoretical"] = theoretical_count;
    stats["measured"] = measured_count;
    stats["insufficient_data"] = insufficient_data_count;
    stats["total"] = theoretical_count + measured_count + insufficient_data_count;
    
    return stats;
}

void ExchangeRateInitializationManager::UpdateExchangeRate(const std::string& o_currency, const std::string& fiat_currency, double new_rate) {
    // Increment measurement count
    m_measurement_counts[o_currency][fiat_currency]++;
    
    // Record measurement timestamp
    int64_t current_time = GetTime();
    m_measurement_history[o_currency][fiat_currency].push_back(current_time);
    m_last_measurement_times[o_currency][fiat_currency] = current_time;
    
    int measurement_count = m_measurement_counts[o_currency][fiat_currency];
    
    if (measurement_count >= MIN_MEASUREMENTS_FOR_RATE) {
        // Switch to measured rate
        m_exchange_rates[o_currency][fiat_currency] = new_rate;
        m_exchange_rate_status[o_currency][fiat_currency] = "measured";
        
        LogPrintf("O Exchange Rate Init: %s/%s = %.4f (measured, %d measurements)\n",
                  o_currency.c_str(), fiat_currency.c_str(), new_rate, measurement_count);
    } else {
        // Still using theoretical rate, but update measurement count
        LogPrintf("O Exchange Rate Init: %s/%s still theoretical, %d/%d measurements\n",
                  o_currency.c_str(), fiat_currency.c_str(), measurement_count, MIN_MEASUREMENTS_FOR_RATE);
    }
}

bool ExchangeRateInitializationManager::IsExchangeRateInitialized(const std::string& o_currency, const std::string& fiat_currency) const {
    auto o_it = m_exchange_rates.find(o_currency);
    if (o_it == m_exchange_rates.end()) {
        return false;
    }
    
    auto fiat_it = o_it->second.find(fiat_currency);
    return fiat_it != o_it->second.end();
}

double ExchangeRateInitializationManager::GetTheoreticalExchangeRate(const std::string& o_currency) const {
    // Use the measurement system's theoretical exchange rate function
    return g_measurement_system.GetTheoreticalExchangeRate(o_currency);
}

std::optional<double> ExchangeRateInitializationManager::GetMeasuredExchangeRate(const std::string& o_currency, const std::string& fiat_currency) const {
    // Get the average exchange rate from the measurement system
    // This would need to be implemented in the measurement system
    // For now, return nullopt as placeholder
    return std::nullopt;
}

int ExchangeRateInitializationManager::GetMeasurementCount(const std::string& o_currency, const std::string& fiat_currency) const {
    auto o_it = m_measurement_counts.find(o_currency);
    if (o_it == m_measurement_counts.end()) {
        return 0;
    }
    
    auto fiat_it = o_it->second.find(fiat_currency);
    if (fiat_it == o_it->second.end()) {
        return 0;
    }
    
    return fiat_it->second;
}

bool ExchangeRateInitializationManager::DetectCurrencyDisappearance(const std::string& o_currency, const std::string& fiat_currency) const {
    // Check if we have measurement history
    auto o_it = m_measurement_history.find(o_currency);
    if (o_it == m_measurement_history.end()) {
        return false; // No measurements yet, not disappearing
    }
    
    auto fiat_it = o_it->second.find(fiat_currency);
    if (fiat_it == o_it->second.end()) {
        return false; // No measurements yet, not disappearing
    }
    
    const auto& timestamps = fiat_it->second;
    if (timestamps.size() < 5) {
        return false; // Need at least 5 measurements to detect trend
    }
    
    // Check if last measurement was more than 30 days ago
    int64_t current_time = GetTime();
    int64_t last_measurement = m_last_measurement_times.at(o_currency).at(fiat_currency);
    int64_t days_since_last = (current_time - last_measurement) / (24 * 60 * 60);
    
    if (days_since_last > 30) {
        LogPrintf("O Exchange Rate Init: %s/%s detected as disappearing (no measurements for %d days)\n",
                  o_currency.c_str(), fiat_currency.c_str(), static_cast<int>(days_since_last));
        return true;
    }
    
    // Check for progressive decrease in measurement frequency
    // Look at last 10 measurements and check if frequency is decreasing
    if (timestamps.size() >= 10) {
        std::vector<int64_t> recent_timestamps(timestamps.end() - 10, timestamps.end());
        
        // Calculate average interval between measurements
        int64_t total_interval = 0;
        for (size_t i = 1; i < recent_timestamps.size(); i++) {
            total_interval += (recent_timestamps[i] - recent_timestamps[i-1]);
        }
        int64_t avg_interval = total_interval / (recent_timestamps.size() - 1);
        
        // If average interval is more than 7 days, consider it disappearing
        if (avg_interval > (7 * 24 * 60 * 60)) {
            LogPrintf("O Exchange Rate Init: %s/%s detected as disappearing (avg interval: %d days)\n",
                      o_currency.c_str(), fiat_currency.c_str(), static_cast<int>(avg_interval / (24 * 60 * 60)));
            return true;
        }
    }
    
    return false;
}

std::string ExchangeRateInitializationManager::GetMeasurementTrend(const std::string& o_currency, const std::string& fiat_currency) const {
    // Check if we have measurement history
    auto o_it = m_measurement_history.find(o_currency);
    if (o_it == m_measurement_history.end()) {
        return "no_measurements";
    }
    
    auto fiat_it = o_it->second.find(fiat_currency);
    if (fiat_it == o_it->second.end()) {
        return "no_measurements";
    }
    
    const auto& timestamps = fiat_it->second;
    if (timestamps.size() < 3) {
        return "insufficient_data";
    }
    
    // Check if currency is disappearing
    if (DetectCurrencyDisappearance(o_currency, fiat_currency)) {
        return "disappearing";
    }
    
    // Check recent measurement frequency
    int64_t current_time = GetTime();
    int64_t last_measurement = m_last_measurement_times.at(o_currency).at(fiat_currency);
    int64_t days_since_last = (current_time - last_measurement) / (24 * 60 * 60);
    
    if (days_since_last > 7) {
        return "decreasing";
    } else if (days_since_last <= 1) {
        return "active";
    } else {
        return "stable";
    }
}

} // namespace OConsensus
