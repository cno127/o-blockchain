// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MEASUREMENT_O_MEASUREMENT_DB_H
#define BITCOIN_MEASUREMENT_O_MEASUREMENT_DB_H

#include <dbwrapper.h>
#include <measurement/measurement_system.h>
#include <sync.h>
#include <uint256.h>

#include <memory>
#include <optional>
#include <vector>
#include <string>

namespace OMeasurement {

/** Database key prefixes for measurement data */
static constexpr uint8_t DB_WATER_PRICE = 'w';        // Water price measurements
static constexpr uint8_t DB_EXCHANGE_RATE = 'e';      // Exchange rate measurements
static constexpr uint8_t DB_INVITE = 'i';              // Measurement invitations
static constexpr uint8_t DB_VALIDATED_URL = 'u';      // Validated URLs for bots
static constexpr uint8_t DB_DAILY_AVERAGE = 'd';      // Daily averages by currency
static constexpr uint8_t DB_WATER_BY_CURRENCY = 'W';  // Index: currency -> measurement IDs
static constexpr uint8_t DB_EXCHANGE_BY_PAIR = 'E';   // Index: currency pair -> measurement IDs
static constexpr uint8_t DB_INVITE_BY_USER = 'I';     // Index: user -> invite IDs
static constexpr uint8_t DB_MEASUREMENT_STATS = 's';  // Statistics
static constexpr uint8_t DB_MEASUREMENT_VERSION = 'v'; // Database version

/** Measurement Database - Persistent storage for water price and exchange rate data */
class CMeasurementDB {
private:
    std::unique_ptr<CDBWrapper> m_db;
    mutable RecursiveMutex m_db_mutex;
    
public:
    explicit CMeasurementDB(size_t cache_size, bool memory_only = false, bool wipe_data = false);
    ~CMeasurementDB();
    
    // ===== Water Price Measurement Operations =====
    
    /** Write water price measurement to database */
    bool WriteWaterPrice(const uint256& measurement_id, const WaterPriceMeasurement& measurement);
    
    /** Read water price measurement from database */
    std::optional<WaterPriceMeasurement> ReadWaterPrice(const uint256& measurement_id) const;
    
    /** Check if water price measurement exists */
    bool HasWaterPrice(const uint256& measurement_id) const;
    
    /** Erase water price measurement */
    bool EraseWaterPrice(const uint256& measurement_id);
    
    /** Get water prices for currency in time range */
    std::vector<WaterPriceMeasurement> GetWaterPricesInRange(
        const std::string& currency, int64_t start_time, int64_t end_time) const;
    
    /** Get all water prices for currency (last N days) */
    std::vector<WaterPriceMeasurement> GetRecentWaterPrices(
        const std::string& currency, int days) const;
    
    // ===== Exchange Rate Measurement Operations =====
    
    /** Write exchange rate measurement to database */
    bool WriteExchangeRate(const uint256& measurement_id, const ExchangeRateMeasurement& measurement);
    
    /** Read exchange rate measurement from database */
    std::optional<ExchangeRateMeasurement> ReadExchangeRate(const uint256& measurement_id) const;
    
    /** Check if exchange rate measurement exists */
    bool HasExchangeRate(const uint256& measurement_id) const;
    
    /** Erase exchange rate measurement */
    bool EraseExchangeRate(const uint256& measurement_id);
    
    /** Get exchange rates for currency pair in time range */
    std::vector<ExchangeRateMeasurement> GetExchangeRatesInRange(
        const std::string& from_currency, const std::string& to_currency,
        int64_t start_time, int64_t end_time) const;
    
    /** Get recent exchange rates for currency pair */
    std::vector<ExchangeRateMeasurement> GetRecentExchangeRates(
        const std::string& from_currency, const std::string& to_currency, int days) const;
    
    // ===== Measurement Invitation Operations =====
    
    /** Write measurement invitation to database */
    bool WriteInvite(const uint256& invite_id, const MeasurementInvite& invite);
    
    /** Read measurement invitation from database */
    std::optional<MeasurementInvite> ReadInvite(const uint256& invite_id) const;
    
    /** Check if invite exists */
    bool HasInvite(const uint256& invite_id) const;
    
    /** Mark invite as used (SECURITY: prevent replay) */
    bool MarkInviteUsed(const uint256& invite_id);
    
    /** Erase invite */
    bool EraseInvite(const uint256& invite_id);
    
    /** Get all invites for a user */
    std::vector<MeasurementInvite> GetUserInvites(const CPubKey& user) const;
    
    /** Get active (unused, not expired) invites */
    std::vector<MeasurementInvite> GetActiveInvites() const;
    
    // ===== Validated URL Operations =====
    
    /** Write validated URL to database */
    bool WriteValidatedURL(const uint256& url_id, const ValidatedURL& url);
    
    /** Read validated URL from database */
    std::optional<ValidatedURL> ReadValidatedURL(const uint256& url_id) const;
    
    /** Get all active URLs for measurement type and currency */
    std::vector<ValidatedURL> GetActiveURLs(MeasurementType type, const std::string& currency) const;
    
    /** Update URL reliability score */
    bool UpdateURLReliability(const uint256& url_id, double reliability_score);
    
    /** Mark URL as inactive */
    bool DeactivateURL(const uint256& url_id);
    
    // ===== Daily Average Operations =====
    
    /** Write daily average to database */
    bool WriteDailyAverage(const std::string& currency, const std::string& date, const DailyAverage& average);
    
    /** Read daily average from database */
    std::optional<DailyAverage> ReadDailyAverage(const std::string& currency, const std::string& date) const;
    
    /** Get daily averages for currency in date range */
    std::vector<DailyAverage> GetDailyAveragesInRange(
        const std::string& currency, const std::string& start_date, const std::string& end_date) const;
    
    /** Get recent daily averages */
    std::vector<DailyAverage> GetRecentDailyAverages(const std::string& currency, int days) const;
    
    // ===== Batch Operations =====
    
    /** Batch write water prices */
    bool BatchWriteWaterPrices(const std::vector<std::pair<uint256, WaterPriceMeasurement>>& batch);
    
    /** Batch write exchange rates */
    bool BatchWriteExchangeRates(const std::vector<std::pair<uint256, ExchangeRateMeasurement>>& batch);
    
    /** Batch write invites */
    bool BatchWriteInvites(const std::vector<std::pair<uint256, MeasurementInvite>>& batch);
    
    // ===== Query Operations =====
    
    /** Find measurements by submitter */
    std::vector<uint256> FindMeasurementsBySubmitter(const CPubKey& submitter, MeasurementType type) const;
    
    /** Find unvalidated measurements */
    std::vector<uint256> FindUnvalidatedMeasurements(MeasurementType type) const;
    
    /** Find measurements by block height range */
    std::vector<uint256> FindMeasurementsByHeight(int start_height, int end_height, MeasurementType type) const;
    
    // ===== Statistics =====
    
    /** Get total water price measurement count */
    size_t GetWaterPriceCount() const;
    
    /** Get total exchange rate measurement count */
    size_t GetExchangeRateCount() const;
    
    /** Get total invite count */
    size_t GetInviteCount() const;
    
    /** Get active invite count */
    size_t GetActiveInviteCount() const;
    
    /** Get validated URL count */
    size_t GetValidatedURLCount() const;
    
    /** Get measurements count by currency */
    std::map<std::string, size_t> GetMeasurementCountByCurrency() const;
    
    // ===== Maintenance =====
    
    /** Prune old measurements before cutoff timestamp */
    bool PruneOldMeasurements(int64_t cutoff_timestamp);
    
    /** Prune expired invites */
    bool PruneExpiredInvites(int64_t current_time);
    
    /** Prune inactive URLs */
    bool PruneInactiveURLs();
    
    /** Compact the database */
    void Compact();
    
    /** Get database size estimate */
    size_t EstimateSize() const;
    
    /** Get database path */
    std::optional<fs::path> StoragePath() const;
    
    // ===== Backup/Restore =====
    
    /** Export measurements to file (for backup) */
    bool ExportMeasurements(const fs::path& export_path, MeasurementType type) const;
    
    /** Import measurements from file (for restore) */
    bool ImportMeasurements(const fs::path& import_path, MeasurementType type);
    
    /** Verify database integrity */
    bool VerifyIntegrity() const;
};

/** Global measurement database instance */
extern std::unique_ptr<CMeasurementDB> g_measurement_db;

} // namespace OMeasurement

#endif // BITCOIN_MEASUREMENT_O_MEASUREMENT_DB_H

