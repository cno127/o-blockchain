#ifndef BITCOIN_CONSENSUS_MEASUREMENT_READINESS_H
#define BITCOIN_CONSENSUS_MEASUREMENT_READINESS_H

#include <string>
#include <map>
#include <optional>
#include <sync.h>
#include <consensus/amount.h>


namespace OConsensus {

// Minimum requirements for starting measurements
static constexpr int MIN_USERS_FOR_WATER_PRICE_MEASUREMENTS = 100;  // Mature network
static constexpr int BOOTSTRAP_MIN_USERS = 10;  // Bootstrap mode (early blocks)
static constexpr int BOOTSTRAP_HEIGHT_THRESHOLD = 10000;  // Switch to full threshold at block 10,000
static constexpr CAmount MIN_COINS_FOR_EXCHANGE_RATE_MEASUREMENTS = 100000 * COIN; // 100,000 O coins

/**
 * @brief Manages measurement readiness conditions for O currencies.
 *
 * This class determines when it's appropriate to start sending measurement invitations
 * based on user count and coin supply thresholds.
 */
class MeasurementReadinessManager {
private:
    mutable RecursiveMutex cs_readiness;
    
    // User count tracking: o_currency -> user_count
    std::map<std::string, int> m_user_counts GUARDED_BY(cs_readiness);
    
    // Coin supply tracking: o_currency -> total_supply
    std::map<std::string, CAmount> m_coin_supplies GUARDED_BY(cs_readiness);
    
    // Measurement readiness status: o_currency -> readiness_status
    std::map<std::string, std::string> m_readiness_status GUARDED_BY(cs_readiness);
    
    // Last update timestamps: o_currency -> timestamp
    std::map<std::string, int64_t> m_last_updates GUARDED_BY(cs_readiness);

public:
    MeasurementReadinessManager();
    
    /**
     * @brief Update user count for an O currency.
     * @param o_currency The O currency code (e.g., "OUSD").
     * @param user_count The current user count.
     */
    void UpdateUserCount(const std::string& o_currency, int user_count);
    
    /**
     * @brief Update coin supply for an O currency.
     * @param o_currency The O currency code (e.g., "OUSD").
     * @param total_supply The current total supply in satoshis.
     */
    void UpdateCoinSupply(const std::string& o_currency, CAmount total_supply);
    
    /**
     * @brief Check if water price measurements are ready for an O currency.
     * @param o_currency The O currency code.
     * @param height Current block height (for bootstrap mode)
     * @return True if ready (10+ users in bootstrap, 100+ users after), false otherwise.
     */
    bool IsWaterPriceMeasurementReady(const std::string& o_currency, int height = 0) const;
    
    /**
     * @brief Check if exchange rate measurements are ready for an O currency.
     * @param o_currency The O currency code.
     * @return True if ready (100,000+ coins), false otherwise.
     */
    bool IsExchangeRateMeasurementReady(const std::string& o_currency) const;
    
    /**
     * @brief Get readiness status for an O currency.
     * @param o_currency The O currency code.
     * @return Readiness status string.
     */
    std::string GetReadinessStatus(const std::string& o_currency) const;
    
    /**
     * @brief Get user count for an O currency.
     * @param o_currency The O currency code.
     * @return User count, or 0 if not tracked.
     */
    int GetUserCount(const std::string& o_currency) const;
    
    /**
     * @brief Get coin supply for an O currency.
     * @param o_currency The O currency code.
     * @return Coin supply in satoshis, or 0 if not tracked.
     */
    CAmount GetCoinSupply(const std::string& o_currency) const;
    
    /**
     * @brief Get readiness statistics for all O currencies.
     * @return Map with comprehensive statistics.
     */
    std::map<std::string, int> GetReadinessStatistics() const;
    
    /**
     * @brief Get readiness status for a specific O currency with details.
     * @param o_currency The O currency code.
     * @return Map with detailed readiness information.
     */
    std::map<std::string, std::string> GetDetailedReadinessStatus(const std::string& o_currency) const;
    
    /**
     * @brief Get minimum user count required for water price measurements.
     * @return Minimum user count threshold.
     */
    int GetMinimumUsersForWaterPrice() const { return MIN_USERS_FOR_WATER_PRICE_MEASUREMENTS; }
    
    /**
     * @brief Get minimum coin supply required for exchange rate measurements.
     * @return Minimum coin supply threshold in satoshis.
     */
    CAmount GetMinimumCoinsForExchangeRate() const { return MIN_COINS_FOR_EXCHANGE_RATE_MEASUREMENTS; }
    
    /**
     * @brief Check if all measurement types are ready for an O currency.
     * @param o_currency The O currency code.
     * @return True if both water price and exchange rate measurements are ready.
     */
    bool IsFullyReady(const std::string& o_currency) const;
    
    /**
     * @brief Get list of O currencies ready for water price measurements.
     * @return Vector of O currency codes that are ready.
     */
    std::vector<std::string> GetReadyForWaterPriceMeasurements() const;
    
    /**
     * @brief Get list of O currencies ready for exchange rate measurements.
     * @return Vector of O currency codes that are ready.
     */
    std::vector<std::string> GetReadyForExchangeRateMeasurements() const;
    
    /**
     * @brief Get list of O currencies fully ready for all measurements.
     * @return Vector of O currency codes that are fully ready.
     */
    std::vector<std::string> GetFullyReadyCurrencies() const;

private:
    /**
     * @brief Update readiness status for an O currency.
     * @param o_currency The O currency code.
     */
    void UpdateReadinessStatus(const std::string& o_currency);
    
    /**
     * @brief Get readiness status string based on conditions.
     * @param water_price_ready Whether water price measurements are ready.
     * @param exchange_rate_ready Whether exchange rate measurements are ready.
     * @return Readiness status string.
     */
    std::string GetReadinessStatusString(bool water_price_ready, bool exchange_rate_ready) const;
};

/** Global measurement readiness manager */
extern MeasurementReadinessManager g_measurement_readiness_manager;

} // namespace OConsensus

#endif // BITCOIN_CONSENSUS_MEASUREMENT_READINESS_H
