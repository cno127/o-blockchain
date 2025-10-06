// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_STABILIZATION_MINING_H
#define BITCOIN_CONSENSUS_STABILIZATION_MINING_H

#include <consensus/amount.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <pubkey.h>
#include <uint256.h>
#include <serialize.h>

#include <map>
#include <string>
#include <vector>
#include <optional>

namespace OConsensus {

/** Currency Stability Status */
enum class StabilityStatus : uint8_t {
    STABLE = 0,
    UNSTABLE = 1,
    INACTIVE = 2,
    UNKNOWN = 3
};

/** Currency Stability Information */
struct CurrencyStabilityInfo {
    std::string currency_code;
    double expected_water_price;     // Expected price based on O coin definition (1 liter = 1 O)
    double observed_water_price;     // Observed price from measurements
    double observed_exchange_rate;   // Exchange rate with O coin or reference currency
    double stability_ratio;          // |expected - observed| / expected
    StabilityStatus status;
    int64_t unstable_since_height;   // Block height when became unstable (0 if stable)
    int64_t last_check_height;       // Last block height checked
    int64_t measurement_count;       // Number of measurements used
    
    CurrencyStabilityInfo()
        : currency_code(), expected_water_price(1.0), observed_water_price(0.0),
          observed_exchange_rate(0.0), stability_ratio(0.0), 
          status(StabilityStatus::UNKNOWN), unstable_since_height(0),
          last_check_height(0), measurement_count(0) {}
    
    SERIALIZE_METHODS(CurrencyStabilityInfo, obj) {
        uint8_t status_val = static_cast<uint8_t>(obj.status);
        READWRITE(obj.currency_code, obj.expected_water_price, obj.observed_water_price,
                  obj.observed_exchange_rate, obj.stability_ratio, status_val,
                  obj.unstable_since_height, obj.last_check_height, obj.measurement_count);
        if (ser_action.ForRead()) obj.status = static_cast<StabilityStatus>(status_val);
    }
    
    bool IsStable() const { return status == StabilityStatus::STABLE; }
    bool IsUnstable() const { return status == StabilityStatus::UNSTABLE; }
    bool IsInactive() const { return status == StabilityStatus::INACTIVE; }
};

/** Stabilization Mining Configuration */
namespace StabilizationConfig {
    static constexpr double STABILITY_THRESHOLD = 0.10;        // 10% deviation threshold
    static constexpr int UNSTABLE_TIME_RANGE = 144;            // ~1 day in blocks to confirm instability
    static constexpr int INACTIVE_TIME_RANGE = 144 * 30;       // ~30 days without exchange rates
    static constexpr int MIN_EXCHANGE_REPORTS = 10;            // Minimum exchange rate reports per day
    static constexpr CAmount MIN_STABILIZATION_REWARD = 100;   // 1.00 O minimum per recipient
    static constexpr CAmount MAX_STABILIZATION_REWARD = 10000; // 100.00 O maximum per recipient
    static constexpr int MIN_MEASUREMENTS_REQUIRED = 10;       // Minimum measurements for stability check
    // Note: No MAX_RECIPIENTS_PER_BLOCK - recipients determined by economic need
}

/** Stabilization Transaction Record */
struct StabilizationTransaction {
    uint256 tx_id;
    std::string unstable_currency;
    CAmount coins_created;
    std::vector<CPubKey> recipients;
    int block_height;
    int64_t timestamp;
    double deviation_ratio;
    
    StabilizationTransaction()
        : tx_id(), unstable_currency(), coins_created(0), block_height(0),
          timestamp(0), deviation_ratio(0.0) {}
    
    SERIALIZE_METHODS(StabilizationTransaction, obj) {
        READWRITE(obj.tx_id, obj.unstable_currency, obj.coins_created, obj.recipients,
                  obj.block_height, obj.timestamp, obj.deviation_ratio);
    }
};

/** Stabilization Mining Manager */
class StabilizationMining {
public:
    StabilizationMining();
    
    // ===== Currency Stability Detection =====
    
    /** Check if a currency is stable */
    bool IsCurrencyStable(const std::string& currency) const;
    
    /** Update currency stability status */
    void UpdateStabilityStatus(const std::string& currency, 
                               double expected_price,
                               double observed_price,
                               double exchange_rate,
                               int height);
    
    /** Get stability status for a currency */
    std::optional<CurrencyStabilityInfo> GetStabilityStatus(const std::string& currency) const;
    
    /** Get all unstable currencies */
    std::vector<std::string> GetUnstableCurrencies() const;
    
    /** Get all stable currencies */
    std::vector<std::string> GetStableCurrencies() const;
    
    /** Check if currency is inactive (no exchange rate data) */
    bool IsCurrencyInactive(const std::string& currency, int height) const;
    
    /** Mark currency as inactive */
    void MarkCurrencyInactive(const std::string& currency, int height);
    
    // ===== Stabilization Coin Creation =====
    
    /** Calculate stabilization coins to create for a block */
    CAmount CalculateStabilizationCoins(const CBlock& block, int height);
    
    /** Calculate coins to create for a specific currency's instability */
    CAmount CalculateCoinsForCurrency(const std::string& currency, 
                                      const CBlock& block,
                                      int height) const;
    
    /** Calculate total volume difference (expected vs observed) */
    CAmount CalculateVolumeDifference(const std::string& currency,
                                      const CBlock& block) const;
    
    // ===== Recipient Selection =====
    
    /** Select random authenticated users from stable currency regions */
    std::vector<CPubKey> SelectRewardRecipients(int count, 
                                                const std::string& exclude_currency = "") const;
    
    /** Select recipients specifically from a stable currency */
    std::vector<CPubKey> SelectRecipientsFromCurrency(int count,
                                                      const std::string& currency) const;
    
    /** Get users by their birth currency (from user consensus system) */
    std::vector<CPubKey> GetUsersByCurrency(const std::string& currency) const;
    
    // ===== Stabilization Transactions =====
    
    /** Create stabilization transactions for a block */
    std::vector<CTransaction> CreateStabilizationTransactions(const CBlock& block, int height);
    
    /** Record stabilization transaction */
    void RecordStabilizationTransaction(const StabilizationTransaction& tx);
    
    /** Get stabilization history */
    std::vector<StabilizationTransaction> GetStabilizationHistory(
        const std::string& currency, int start_height, int end_height) const;
    
    /** Get total coins created for a currency */
    CAmount GetTotalCoinsCreated(const std::string& currency) const;
    
    // ===== Statistics =====
    
    /** Get stability statistics */
    std::map<std::string, CurrencyStabilityInfo> GetAllStabilityStatus() const;
    
    /** Get stabilization statistics */
    struct StabilizationStats {
        int64_t total_unstable_currencies;
        int64_t total_stable_currencies;
        int64_t total_inactive_currencies;
        CAmount total_coins_created;
        int64_t total_transactions;
        int64_t total_recipients;
    };
    
    StabilizationStats GetStatistics() const;
    
    /** Re-evaluate all currency stability statuses */
    void ReEvaluateAllCurrencies(int height);
    
    /** Prune old stability data */
    void PruneOldData(int cutoff_height);
    
    /** Calculate optimal number of recipients based on stabilization amount */
    int CalculateOptimalRecipientCount(CAmount total_coins) const;
    
    /** Get transaction volume in specific currency */
    CAmount GetTransactionVolumeInCurrency(const std::string& currency, const CBlock& block) const;
    
    /** Calculate exchange rate deviation from water price */
    double CalculateExchangeRateDeviation(const std::string& currency) const;

private:
    // Storage
    std::map<std::string, CurrencyStabilityInfo> m_currency_status;
    std::map<uint256, StabilizationTransaction> m_stabilization_txs;
    
    // Statistics
    struct InternalStats {
        CAmount total_coins_created;
        int64_t total_transactions;
        int64_t total_recipients;
    } m_stats;
    
    // Helper functions
    double CalculateStabilityRatio(double expected, double observed) const;
    bool MeetsInstabilityThreshold(const CurrencyStabilityInfo& info, int height) const;
    std::vector<CPubKey> RandomSample(const std::vector<CPubKey>& users, int count) const;
    
    /** Calculate dynamic stabilization factor based on volatility level */
    double CalculateDynamicStabilizationFactor(double stability_ratio, const std::string& currency) const;
    
    /** Integration with measurement system */
    double GetAverageWaterPrice(const std::string& currency, int days) const;
    double GetAverageExchangeRate(const std::string& from, const std::string& to, int days) const;
};

/** Global stabilization mining instance */
extern StabilizationMining g_stabilization_mining;

/** Helper functions for validation integration */

/** Check if block should trigger stabilization mining */
bool ShouldTriggerStabilization(const CBlock& block, int height);

/** Validate stabilization transactions in block */
bool ValidateStabilizationTransactions(const CBlock& block, int height);

/** Calculate expected stabilization amount for block */
CAmount CalculateExpectedStabilization(const CBlock& block, int height);

/** O_ONLY Currency Stability Integration */

/** Check if O_ONLY currency is unstable */
bool IsOOnlyCurrencyUnstable(const std::string& currency);

/** Get stabilization amount for O_ONLY currency */
CAmount GetOOnlyStabilizationAmount(const std::string& currency);

/** Get list of unstable O_ONLY currencies */
std::vector<std::string> GetUnstableOOnlyCurrencies();

} // namespace OConsensus

#endif // BITCOIN_CONSENSUS_STABILIZATION_MINING_H
