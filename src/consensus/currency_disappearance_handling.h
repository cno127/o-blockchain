// Copyright (c) 2024 The Bitcoin Core developers
// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_CURRENCY_DISAPPEARANCE_HANDLING_H
#define BITCOIN_CONSENSUS_CURRENCY_DISAPPEARANCE_HANDLING_H

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <serialize.h>
#include <consensus/amount.h>
#include <consensus/o_amount.h>
#include <consensus/currency_lifecycle.h>

/** Fallback Data Source Types */
enum class FallbackSourceType {
    PRIMARY,        // Real-time exchange rate APIs
    SECONDARY,      // Historical data with time decay
    TERTIARY,       // Regional currency proxies
    EMERGENCY       // Global water price averages
};

/** Fallback Data Source */
struct FallbackDataSource {
    FallbackSourceType type;
    std::string source_name;
    std::string currency;
    double rate;
    int64_t timestamp;
    double confidence;      // 0.0 to 1.0
    bool is_active;
    
    FallbackDataSource()
        : type(FallbackSourceType::PRIMARY), source_name(), currency(), rate(0.0),
          timestamp(0), confidence(1.0), is_active(true) {}
    
    SERIALIZE_METHODS(FallbackDataSource, obj) {
        uint8_t type_val = static_cast<uint8_t>(obj.type);
        READWRITE(type_val, obj.source_name, obj.currency, obj.rate,
                  obj.timestamp, obj.confidence, obj.is_active);
        if (ser_action.ForRead()) obj.type = static_cast<FallbackSourceType>(type_val);
    }
};

/** Currency Disappearance Handler */
class CurrencyDisappearanceHandler {
public:
    CurrencyDisappearanceHandler();
    ~CurrencyDisappearanceHandler();
    
    // ===== Fallback Data Management =====
    
    /** Add fallback data source */
    void AddFallbackSource(const std::string& currency, const FallbackDataSource& source);
    
    /** Get fallback data for currency */
    std::optional<FallbackDataSource> GetFallbackData(const std::string& currency, 
                                                     FallbackSourceType preferred_type = FallbackSourceType::PRIMARY) const;
    
    /** Get best available data for currency */
    std::optional<FallbackDataSource> GetBestAvailableData(const std::string& currency) const;
    
    /** Update fallback data source */
    void UpdateFallbackSource(const std::string& currency, const std::string& source_name, 
                             double rate, double confidence);
    
    /** Remove fallback data source */
    void RemoveFallbackSource(const std::string& currency, const std::string& source_name);
    
    // ===== Currency Disappearance Scenarios =====
    
    /** Handle Scenario 1: Currency replaced by existing currency */
    bool HandleCurrencyReplacement(const std::string& old_currency, 
                                  const std::string& new_currency,
                                  double conversion_rate,
                                  const std::string& reason,
                                  int height);
    
    /** Handle Scenario 2: Currency replaced by new currency */
    bool HandleNewCurrencyReplacement(const std::string& old_currency,
                                    const std::string& new_currency,
                                    double conversion_rate,
                                    const std::string& reason,
                                    int height);
    
    /** Handle Scenario 3: Currency becomes O_ONLY */
    bool HandleOOnlyConversion(const std::string& currency,
                              const std::string& reason,
                              int height);
    
    // ===== O_ONLY Currency Validation =====
    
    /** Validate O_ONLY currency stability */
    bool ValidateOOnlyCurrency(const std::string& currency, 
                              double water_price, 
                              double exchange_rate) const;
    
    /** Check if O_ONLY currency is stable */
    bool IsOOnlyCurrencyStable(const std::string& currency) const;
    
    /** Get O_ONLY stability metrics */
    std::pair<double, double> GetOOnlyStabilityMetrics(const std::string& currency) const;
    
    /** Update O_ONLY stability status */
    void UpdateOOnlyStability(const std::string& currency,
                             double water_price,
                             double exchange_rate);
    
    // ===== Fallback Rate Calculation =====
    
    /** Calculate fallback exchange rate */
    std::optional<double> CalculateFallbackExchangeRate(const std::string& from_currency,
                                                       const std::string& to_currency) const;
    
    /** Calculate fallback water price */
    std::optional<double> CalculateFallbackWaterPrice(const std::string& currency) const;
    
    /** Get regional currency proxy */
    std::optional<std::string> GetRegionalCurrencyProxy(const std::string& currency) const;
    
    /** Calculate global water price average */
    double CalculateGlobalWaterPriceAverage() const;
    
    // ===== Emergency Mechanisms =====
    
    /** Trigger emergency stabilization for O_ONLY currency */
    bool TriggerEmergencyStabilization(const std::string& currency, int height) const;
    
    /** Check if emergency stabilization is needed */
    bool IsEmergencyStabilizationNeeded(const std::string& currency) const;
    
    /** Get emergency stabilization amount */
    CAmount GetEmergencyStabilizationAmount(const std::string& currency) const;
    
    // ===== Data Quality Management =====
    
    /** Check data quality for currency */
    double GetDataQualityScore(const std::string& currency) const;
    
    /** Get data freshness for currency */
    int64_t GetDataFreshness(const std::string& currency) const;
    
    /** Check if data is stale */
    bool IsDataStale(const std::string& currency, int64_t max_age_seconds = 86400) const;
    
    /** Get currencies with poor data quality */
    std::vector<std::string> GetCurrenciesWithPoorDataQuality(double min_quality_score = 0.5) const;
    
    // ===== Regional Currency Mapping =====
    
    /** Get regional currency groups */
    std::map<std::string, std::vector<std::string>> GetRegionalCurrencyGroups() const;
    
    /** Get currency region */
    std::optional<std::string> GetCurrencyRegion(const std::string& currency) const;
    
    /** Get regional proxy for currency */
    std::optional<std::string> GetRegionalProxy(const std::string& currency) const;
    
    // ===== Statistics and Reporting =====
    
    /** Get fallback usage statistics */
    std::map<std::string, int64_t> GetFallbackUsageStatistics() const;
    
    /** Get currency disappearance statistics */
    std::map<std::string, int64_t> GetDisappearanceStatistics() const;
    
    /** Get O_ONLY currency statistics */
    std::map<std::string, int64_t> GetOOnlyStatistics() const;
    
    /** Get data quality statistics */
    std::map<std::string, double> GetDataQualityStatistics() const;
    
    // ===== Utility Functions =====
    
    /** Check if currency has fallback data */
    bool HasFallbackData(const std::string& currency) const;
    
    /** Get all currencies with fallback data */
    std::vector<std::string> GetCurrenciesWithFallbackData() const;
    
    /** Get fallback source type description */
    std::string GetFallbackSourceDescription(FallbackSourceType type) const;
    
    /** Check if currency is in emergency mode */
    bool IsCurrencyInEmergencyMode(const std::string& currency) const;
    
    // ===== Data Management =====
    
    /** Prune old fallback data */
    void PruneOldFallbackData(int64_t cutoff_timestamp);
    
    /** Clear all fallback data */
    void ClearAllFallbackData();
    
    /** Backup fallback data */
    void BackupFallbackData(const std::string& backup_path) const;
    
    /** Restore fallback data */
    bool RestoreFallbackData(const std::string& backup_path);

private:
    // Storage
    std::map<std::string, std::vector<FallbackDataSource>> m_fallback_sources;
    std::map<std::string, std::pair<double, double>> m_o_only_stability; // water_price, exchange_rate
    std::map<std::string, int64_t> m_fallback_usage_count;
    std::map<std::string, int64_t> m_disappearance_count;
    
    // Regional currency mapping
    std::map<std::string, std::string> m_currency_regions;
    std::map<std::string, std::vector<std::string>> m_regional_groups;
    
    // Statistics
    struct DisappearanceStats {
        int64_t total_fallback_sources;
        int64_t active_fallback_sources;
        int64_t o_only_currencies;
        int64_t emergency_stabilizations;
        int64_t total_fallback_usage;
    } m_stats;
    
    // Helper functions
    void UpdateStatistics();
    void LogFallbackUsage(const std::string& currency, FallbackSourceType type);
    void LogCurrencyDisappearance(const std::string& currency, const std::string& reason);
    bool ValidateFallbackData(const FallbackDataSource& source) const;
    double CalculateTimeDecay(int64_t timestamp, int64_t current_time) const;
    std::optional<std::string> FindBestRegionalProxy(const std::string& currency) const;
    void InitializeRegionalMappings();
};

/** Global currency disappearance handler instance */
extern CurrencyDisappearanceHandler g_currency_disappearance_handler;

#endif // BITCOIN_CONSENSUS_CURRENCY_DISAPPEARANCE_HANDLING_H
