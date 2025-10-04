// Copyright (c) 2024 The Bitcoin Core developers
// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_CURRENCY_LIFECYCLE_H
#define BITCOIN_CONSENSUS_CURRENCY_LIFECYCLE_H

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <serialize.h>
#include <consensus/amount.h>
#include <consensus/o_amount.h>

/** Currency Lifecycle Status */
enum class CurrencyStatus {
    ACTIVE,         // Normal operation with data collection
    REPLACED,       // Replaced by existing currency (user migration)
    NEW_CURRENCY,   // Replaced by new currency (system update needed)
    O_ONLY,         // Only O currency used (water price = 1 O, exchange rate = 1:1)
    DEPRECATED      // Currency discontinued without replacement
};

/** Currency Replacement Information */
struct CurrencyReplacement {
    std::string old_currency;        // Original currency code
    std::string new_currency;        // Replacement currency code
    double conversion_rate;          // Conversion rate (old to new)
    int64_t replacement_height;      // Block height when replacement occurred
    int64_t migration_deadline;      // Deadline for user migration
    std::string reason;              // Reason for replacement
    bool is_mandatory;               // Whether migration is mandatory
    
    CurrencyReplacement()
        : old_currency(), new_currency(), conversion_rate(1.0),
          replacement_height(0), migration_deadline(0), reason(), is_mandatory(false) {}
    
    SERIALIZE_METHODS(CurrencyReplacement, obj) {
        READWRITE(obj.old_currency, obj.new_currency, obj.conversion_rate,
                  obj.replacement_height, obj.migration_deadline, obj.reason, obj.is_mandatory);
    }
};

/** Currency Lifecycle Information */
struct CurrencyLifecycleInfo {
    std::string currency_code;
    CurrencyStatus status;
    int64_t status_change_height;
    std::string status_reason;
    std::optional<CurrencyReplacement> replacement_info;
    int64_t last_data_height;        // Last height with valid data
    int64_t data_dry_period;         // Blocks since last data
    bool is_water_price_stable;      // Water price = 1 O (for O_ONLY currencies)
    bool is_exchange_rate_stable;    // Exchange rate = 1:1 (for O_ONLY currencies)
    
    CurrencyLifecycleInfo()
        : currency_code(), status(CurrencyStatus::ACTIVE), status_change_height(0),
          status_reason(), last_data_height(0), data_dry_period(0),
          is_water_price_stable(true), is_exchange_rate_stable(true) {}
    
    SERIALIZE_METHODS(CurrencyLifecycleInfo, obj) {
        uint8_t status_val = static_cast<uint8_t>(obj.status);
        READWRITE(obj.currency_code, status_val, obj.status_change_height,
                  obj.status_reason, obj.replacement_info, obj.last_data_height,
                  obj.data_dry_period, obj.is_water_price_stable, obj.is_exchange_rate_stable);
        if (ser_action.ForRead()) obj.status = static_cast<CurrencyStatus>(status_val);
    }
    
    bool IsActive() const { return status == CurrencyStatus::ACTIVE; }
    bool IsReplaced() const { return status == CurrencyStatus::REPLACED; }
    bool IsNewCurrency() const { return status == CurrencyStatus::NEW_CURRENCY; }
    bool IsOOnly() const { return status == CurrencyStatus::O_ONLY; }
    bool IsDeprecated() const { return status == CurrencyStatus::DEPRECATED; }
};

/** Currency Lifecycle Manager */
class CurrencyLifecycleManager {
public:
    CurrencyLifecycleManager();
    ~CurrencyLifecycleManager();
    
    // ===== Currency Status Management =====
    
    /** Get currency lifecycle information */
    std::optional<CurrencyLifecycleInfo> GetCurrencyInfo(const std::string& currency) const;
    
    /** Update currency status */
    void UpdateCurrencyStatus(const std::string& currency, CurrencyStatus status, 
                             const std::string& reason, int height);
    
    /** Check if currency is active */
    bool IsCurrencyActive(const std::string& currency) const;
    
    /** Get all active currencies */
    std::vector<std::string> GetActiveCurrencies() const;
    
    /** Get all O_ONLY currencies */
    std::vector<std::string> GetOOnlyCurrencies() const;
    
    // ===== Currency Replacement Scenarios =====
    
    /** Scenario 1: Currency replaced by existing currency */
    void ReplaceCurrencyWithExisting(const std::string& old_currency,
                                    const std::string& new_currency,
                                    double conversion_rate,
                                    const std::string& reason,
                                    int height);
    
    /** Scenario 2: Currency replaced by new currency */
    void ReplaceCurrencyWithNew(const std::string& old_currency,
                               const std::string& new_currency,
                               double conversion_rate,
                               const std::string& reason,
                               int height);
    
    /** Scenario 3: Currency becomes O_ONLY */
    void ConvertToOOnly(const std::string& currency,
                       const std::string& reason,
                       int height);
    
    /** Get replacement information */
    std::optional<CurrencyReplacement> GetReplacementInfo(const std::string& currency) const;
    
    // ===== O_ONLY Currency Handling =====
    
    /** Check if currency is O_ONLY */
    bool IsOOnlyCurrency(const std::string& currency) const;
    
    /** Validate O_ONLY currency stability */
    bool ValidateOOnlyStability(const std::string& currency, 
                               double water_price, 
                               double exchange_rate) const;
    
    /** Get O_ONLY stability status */
    std::pair<bool, bool> GetOOnlyStabilityStatus(const std::string& currency) const;
    
    /** Update O_ONLY stability status */
    void UpdateOOnlyStability(const std::string& currency,
                             bool water_price_stable,
                             bool exchange_rate_stable);
    
    // ===== Data Availability Management =====
    
    /** Update data availability for currency */
    void UpdateDataAvailability(const std::string& currency, int height);
    
    /** Check if currency has recent data */
    bool HasRecentData(const std::string& currency, int height, int max_blocks_old = 144) const;
    
    /** Get currencies with stale data */
    std::vector<std::string> GetCurrenciesWithStaleData(int height, int max_blocks_old = 144) const;
    
    // ===== Migration Management =====
    
    /** Get currencies requiring migration */
    std::vector<std::string> GetCurrenciesRequiringMigration(int height) const;
    
    /** Get all currency information */
    std::vector<CurrencyLifecycleInfo> GetAllCurrencyInfo() const;
    
    /** Check if migration deadline passed */
    bool IsMigrationDeadlinePassed(const std::string& currency, int height) const;
    
    /** Get migration deadline for currency */
    std::optional<int64_t> GetMigrationDeadline(const std::string& currency) const;
    
    // ===== Statistics and Reporting =====
    
    /** Get currency lifecycle statistics */
    std::map<std::string, int64_t> GetLifecycleStatistics() const;
    
    /** Get currency status distribution */
    std::map<CurrencyStatus, int> GetStatusDistribution() const;
    
    /** Get replacement statistics */
    std::map<std::string, int64_t> GetReplacementStatistics() const;
    
    // ===== Utility Functions =====
    
    /** Get currency status description */
    std::string GetStatusDescription(CurrencyStatus status) const;
    
    /** Get currency status reason */
    std::string GetStatusReason(const std::string& currency) const;
    
    /** Check if currency needs system update */
    bool NeedsSystemUpdate(const std::string& currency) const;
    
    /** Get all currencies needing system updates */
    std::vector<std::string> GetCurrenciesNeedingUpdates() const;
    
    // ===== Data Management =====
    
    /** Prune old lifecycle data */
    void PruneOldData(int cutoff_height);
    
    /** Clear all data (for testing) */
    void ClearAllData();

private:
    // Storage
    std::map<std::string, CurrencyLifecycleInfo> m_currency_lifecycle;
    std::map<std::string, CurrencyReplacement> m_replacements;
    
    // Statistics
    struct LifecycleStats {
        int64_t total_currencies;
        int64_t active_currencies;
        int64_t replaced_currencies;
        int64_t o_only_currencies;
        int64_t deprecated_currencies;
        int64_t total_replacements;
    } m_stats;
    
    // Helper functions
    void UpdateStatistics();
    void LogStatusChange(const std::string& currency, CurrencyStatus old_status, 
                        CurrencyStatus new_status, const std::string& reason);
    bool ValidateReplacement(const std::string& old_currency, const std::string& new_currency) const;
    void NotifyUsersOfReplacement(const std::string& old_currency, const std::string& new_currency);
};

/** Global currency lifecycle manager instance */
extern CurrencyLifecycleManager g_currency_lifecycle_manager;

#endif // BITCOIN_CONSENSUS_CURRENCY_LIFECYCLE_H
