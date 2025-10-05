// Copyright (c) 2024 The Bitcoin Core developers
// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_CURRENCY_EXCHANGE_H
#define BITCOIN_CONSENSUS_CURRENCY_EXCHANGE_H

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <serialize.h>
#include <primitives/transaction.h>
#include <pubkey.h>

/** Currency Exchange Configuration */
namespace ExchangeConfig {
    static constexpr double MAX_RATE_DEVIATION = 0.01;        // 1% maximum deviation from measured rate
    static constexpr int MIN_CONFIRMATIONS = 1;               // Minimum confirmations for exchange
    static constexpr int64_t MIN_EXCHANGE_AMOUNT = 100;       // 1.00 O minimum exchange amount
    static constexpr int64_t MAX_EXCHANGE_AMOUNT = 100000000; // 1,000,000.00 O maximum exchange amount
    static constexpr int RATE_VALIDITY_HOURS = 24;            // Exchange rate valid for 24 hours
    // Note: No MIN_MEASUREMENTS_FOR_RATE - measurement system guarantees quality through invitation system
}

/** Currency Exchange Transaction Structure */
struct CurrencyExchange {
    uint256 exchange_id;
    CPubKey from_user;
    CPubKey to_user;
    std::string from_currency;        // Source currency (e.g., "OUSD")
    std::string to_currency;          // Target currency (e.g., "OEUR")
    CAmount from_amount;              // Amount in source currency
    CAmount to_amount;                // Amount in target currency
    double exchange_rate;             // Rate used for exchange
    int64_t timestamp;                // Unix timestamp
    int block_height;                 // Block height when created
    bool is_executed;                 // Whether exchange was executed
    bool is_validated;                // Whether exchange was validated
    std::string memo;                 // Optional memo
    uint256 tx_hash;                  // Transaction hash
    
    CurrencyExchange()
        : exchange_id(), from_user(), to_user(), from_currency(), to_currency(),
          from_amount(0), to_amount(0), exchange_rate(0.0), timestamp(0),
          block_height(0), is_executed(false), is_validated(false), tx_hash() {}
    
    SERIALIZE_METHODS(CurrencyExchange, obj) {
        READWRITE(obj.exchange_id, obj.from_user, obj.to_user, obj.from_currency, obj.to_currency,
                  obj.from_amount, obj.to_amount, obj.exchange_rate, obj.timestamp,
                  obj.block_height, obj.is_executed, obj.is_validated, obj.memo, obj.tx_hash);
    }
    
    uint256 GetHash() const;
    bool IsValid() const;
    std::string ToString() const;
};

/** Currency Exchange Manager */
class CurrencyExchangeManager {
public:
    CurrencyExchangeManager();
    ~CurrencyExchangeManager();
    
    // ===== Exchange Creation =====
    
    /** Create a new currency exchange */
    std::optional<CurrencyExchange> CreateExchange(
        const CPubKey& from_user,
        const CPubKey& to_user,
        const std::string& from_currency,
        const std::string& to_currency,
        CAmount from_amount,
        const std::string& memo = "");
    
    /** Validate exchange rate against measured rates */
    bool ValidateExchangeRate(
        const std::string& from_currency,
        const std::string& to_currency,
        double proposed_rate) const;
    
    /** Get current exchange rate between currencies */
    std::optional<double> GetCurrentExchangeRate(
        const std::string& from_currency,
        const std::string& to_currency) const;
    
    // ===== Exchange Execution =====
    
    /** Execute a currency exchange */
    bool ExecuteExchange(const uint256& exchange_id, const CTransaction& tx);
    
    /** Validate exchange transaction */
    bool ValidateExchangeTransaction(const CTransaction& tx) const;
    
    /** Get exchange by ID */
    std::optional<CurrencyExchange> GetExchange(const uint256& exchange_id) const;
    
    /** Get all exchanges for a user */
    std::vector<CurrencyExchange> GetUserExchanges(const CPubKey& user) const;
    
    /** Get exchanges in time range */
    std::vector<CurrencyExchange> GetExchangesInRange(
        int64_t start_time, int64_t end_time) const;
    
    // ===== Rate Management =====
    
    /** Update exchange rate from measurement system */
    void UpdateExchangeRate(
        const std::string& from_currency,
        const std::string& to_currency,
        double rate,
        int64_t timestamp);
    
    /** Check if exchange rate is valid */
    bool IsExchangeRateValid(
        const std::string& from_currency,
        const std::string& to_currency) const;
    
    /** Get rate deviation from measured rate */
    double GetRateDeviation(
        const std::string& from_currency,
        const std::string& to_currency,
        double proposed_rate) const;
    
    // ===== Statistics =====
    
    /** Get exchange statistics */
    std::map<std::string, int64_t> GetExchangeStatistics() const;
    
    /** Get volume by currency pair */
    std::map<std::string, CAmount> GetExchangeVolumeByPair() const;
    
    /** Get daily exchange volume */
    CAmount GetDailyExchangeVolume(const std::string& date) const;
    
    // ===== Utility Functions =====
    
    /** Calculate exchange amount */
    CAmount CalculateExchangeAmount(
        CAmount from_amount,
        double exchange_rate) const;
    
    /** Format exchange rate */
    std::string FormatExchangeRate(double rate) const;
    
    /** Check if currencies are supported */
    bool IsCurrencySupported(const std::string& currency) const;
    
    /** Get supported currency pairs */
    std::vector<std::pair<std::string, std::string>> GetSupportedPairs() const;
    
    // ===== Cross-O Currency Exchange Rate Calculation =====
    
    /** Calculate exchange rate between two O currencies via fiat currency bridge */
    std::optional<double> CalculateOCurrencyExchangeRate(
        const std::string& from_o_currency,
        const std::string& to_o_currency) const;
    
    /** Get fiat exchange rate between two fiat currencies */
    std::optional<double> GetFiatExchangeRate(
        const std::string& from_fiat,
        const std::string& to_fiat) const;
    
    /** Check if currency is an O currency */
    bool IsOCurrency(const std::string& currency) const;
    
    /** Get corresponding fiat currency for O currency */
    std::string GetCorrespondingFiatCurrency(const std::string& o_currency) const;
    
    // ===== Data Management =====
    
    /** Prune old exchange data */
    void PruneOldData(int64_t cutoff_time);
    
    /** Clear all data (for testing) */
    void ClearAllData();

private:
    // Storage
    std::map<uint256, CurrencyExchange> m_exchanges;
    std::map<std::string, double> m_exchange_rates;  // Key: "from_to" (e.g., "OUSD_OEUR")
    std::map<std::string, int64_t> m_rate_timestamps; // When rate was last updated
    
    // Statistics
    struct ExchangeStats {
        int64_t total_exchanges;
        int64_t total_volume;
        std::map<std::string, int64_t> exchanges_by_pair;
        std::map<std::string, CAmount> volume_by_pair;
    } m_stats;
    
    // Helper functions
    std::string MakeRateKey(const std::string& from, const std::string& to) const;
    bool IsRateExpired(const std::string& rate_key) const;
    void UpdateStatistics(const CurrencyExchange& exchange);
    std::vector<std::string> GetSupportedCurrencies() const;
};

/** Global currency exchange manager instance */
extern CurrencyExchangeManager g_currency_exchange_manager;

#endif // BITCOIN_CONSENSUS_CURRENCY_EXCHANGE_H
