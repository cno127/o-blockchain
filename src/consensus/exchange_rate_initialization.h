// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_EXCHANGE_RATE_INITIALIZATION_H
#define BITCOIN_CONSENSUS_EXCHANGE_RATE_INITIALIZATION_H

#include <consensus/amount.h>
#include <measurement/measurement_system.h>
#include <map>
#include <string>
#include <optional>

namespace OConsensus {

/** Exchange Rate Initialization Manager */
class ExchangeRateInitializationManager {
public:
    ExchangeRateInitializationManager();
    
    /** Initialize all O currency exchange rates with theoretical water prices */
    void InitializeExchangeRates();
    
    /** Get current exchange rate (theoretical or measured) */
    std::optional<double> GetExchangeRate(const std::string& o_currency, const std::string& fiat_currency) const;
    
    /** Check if we have enough measurements to switch from theoretical to measured rates */
    bool HasMinimumMeasurements(const std::string& o_currency, const std::string& fiat_currency) const;
    
    /** Get the minimum number of measurements required before switching to measured rates */
    int GetMinimumMeasurementsThreshold() const { return MIN_MEASUREMENTS_FOR_RATE; }
    
    /** Get exchange rate status (theoretical, measured, or insufficient_data) */
    std::string GetExchangeRateStatus(const std::string& o_currency, const std::string& fiat_currency) const;
    
    /** Get all initialized exchange rates */
    std::map<std::string, double> GetAllExchangeRates() const;
    
    /** Get exchange rate statistics */
    std::map<std::string, int> GetExchangeRateStatistics() const;
    
    /** Update exchange rate based on new measurements */
    void UpdateExchangeRate(const std::string& o_currency, const std::string& fiat_currency, double new_rate);
    
    /** Check if exchange rate is initialized */
    bool IsExchangeRateInitialized(const std::string& o_currency, const std::string& fiat_currency) const;

private:
    // Minimum number of measurements required before switching from theoretical to measured rates
    static constexpr int MIN_MEASUREMENTS_FOR_RATE = 10;
    
    // Exchange rates: o_currency -> fiat_currency -> rate
    std::map<std::string, std::map<std::string, double>> m_exchange_rates;
    
    // Exchange rate status: o_currency -> fiat_currency -> status
    std::map<std::string, std::map<std::string, std::string>> m_exchange_rate_status;
    
    // Measurement counts: o_currency -> fiat_currency -> count
    std::map<std::string, std::map<std::string, int>> m_measurement_counts;
    
    /** Initialize exchange rate for a specific O currency */
    void InitializeExchangeRateForCurrency(const std::string& o_currency);
    
    /** Get theoretical exchange rate from water price */
    double GetTheoreticalExchangeRate(const std::string& o_currency) const;
    
    /** Get measured exchange rate from measurement system */
    std::optional<double> GetMeasuredExchangeRate(const std::string& o_currency, const std::string& fiat_currency) const;
    
    /** Get measurement count for a currency pair */
    int GetMeasurementCount(const std::string& o_currency, const std::string& fiat_currency) const;
};

/** Global exchange rate initialization manager */
extern ExchangeRateInitializationManager g_exchange_rate_init_manager;

} // namespace OConsensus

#endif // BITCOIN_CONSENSUS_EXCHANGE_RATE_INITIALIZATION_H
