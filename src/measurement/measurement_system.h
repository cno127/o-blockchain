// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MEASUREMENT_MEASUREMENT_SYSTEM_H
#define BITCOIN_MEASUREMENT_MEASUREMENT_SYSTEM_H

#include <primitives/transaction.h>
#include <pubkey.h>
#include <uint256.h>
#include <consensus/amount.h>
#include <serialize.h>

#include <map>
#include <vector>
#include <optional>
#include <string>
#include <set>

namespace OMeasurement {

/** Measurement Type Enumeration */
enum class MeasurementType : uint8_t {
    WATER_PRICE = 0,
    EXCHANGE_RATE = 1,
    WATER_PRICE_OFFLINE_VALIDATION = 2,
    EXCHANGE_RATE_OFFLINE_VALIDATION = 3,
    URL_SUBMISSION = 4,
    ONLINE_WATER_PRICE_VALIDATION = 5,
    ONLINE_EXCHANGE_VALIDATION = 6
};

/** Water Price Measurement Structure */
struct WaterPriceMeasurement {
    uint256 measurement_id;
    CPubKey submitter;
    std::string currency_code;       // USD, EUR, JPY, etc.
    int64_t price;                   // Price in smallest unit (cents, etc.)
    std::string location;            // Geographic location
    std::string proof_image_hash;    // IPFS or SHA256 hash of proof image
    int64_t timestamp;               // Unix timestamp
    int block_height;                // Block height when submitted
    bool is_validated;               // Whether validated by other users
    std::vector<CPubKey> validators; // Users who validated this measurement
    double confidence_score;         // Confidence score (0.0 to 1.0)
    uint256 invite_id;               // Invitation that prompted this measurement
    
    WaterPriceMeasurement() 
        : measurement_id(), submitter(), currency_code(), price(0), location(),
          proof_image_hash(), timestamp(0), block_height(0), is_validated(false),
          confidence_score(0.0), invite_id() {}
    
    SERIALIZE_METHODS(WaterPriceMeasurement, obj) {
        READWRITE(obj.measurement_id, obj.submitter, obj.currency_code, obj.price,
                  obj.location, obj.proof_image_hash, obj.timestamp, obj.block_height,
                  obj.is_validated, obj.validators, obj.confidence_score, obj.invite_id);
    }
    
    uint256 GetHash() const;
};

/** Exchange Rate Measurement Structure */
struct ExchangeRateMeasurement {
    uint256 measurement_id;
    CPubKey submitter;
    std::string from_currency;       // Source currency (e.g., USD)
    std::string to_currency;         // Target currency (e.g., EUR)
    double exchange_rate;            // Exchange rate value
    std::string source_url;          // Source URL for verification
    std::string proof_image_hash;    // IPFS or SHA256 hash of proof
    int64_t timestamp;               // Unix timestamp
    int block_height;                // Block height when submitted
    bool is_validated;               // Whether validated
    std::vector<CPubKey> validators; // Validators
    uint256 invite_id;               // Invitation ID
    
    ExchangeRateMeasurement()
        : measurement_id(), submitter(), from_currency(), to_currency(),
          exchange_rate(0.0), source_url(), proof_image_hash(), timestamp(0),
          block_height(0), is_validated(false), invite_id() {}
    
    SERIALIZE_METHODS(ExchangeRateMeasurement, obj) {
        READWRITE(obj.measurement_id, obj.submitter, obj.from_currency, obj.to_currency,
                  obj.exchange_rate, obj.source_url, obj.proof_image_hash, obj.timestamp,
                  obj.block_height, obj.is_validated, obj.validators, obj.invite_id);
    }
    
    uint256 GetHash() const;
};

/** Measurement Invitation Structure */
struct MeasurementInvite {
    uint256 invite_id;
    CPubKey invited_user;
    MeasurementType type;
    std::string currency_code;       // Optional: specific currency for measurement
    int64_t created_at;              // Unix timestamp when created
    int64_t expires_at;              // Unix timestamp when expires
    bool is_used;                    // Whether invite has been used
    bool is_expired;                 // Whether invite has expired
    int block_height;                // Block height when created
    
    MeasurementInvite()
        : invite_id(), invited_user(), type(MeasurementType::WATER_PRICE),
          currency_code(), created_at(0), expires_at(0), is_used(false),
          is_expired(false), block_height(0) {}
    
    SERIALIZE_METHODS(MeasurementInvite, obj) {
        uint8_t type_val = static_cast<uint8_t>(obj.type);
        READWRITE(obj.invite_id, obj.invited_user, type_val,
                  obj.currency_code, obj.created_at, obj.expires_at, obj.is_used,
                  obj.is_expired, obj.block_height);
        if (ser_action.ForRead()) obj.type = static_cast<MeasurementType>(type_val);
    }
    
    bool IsValid(int64_t current_time) const;
    uint256 GetHash() const;
};

/** Validated URL for Bot Crawling */
struct ValidatedURL {
    uint256 url_id;
    std::string url;
    MeasurementType type;            // WATER_PRICE or EXCHANGE_RATE
    std::string currency_code;       // Currency this URL provides data for
    CPubKey submitter;
    std::vector<CPubKey> validators; // Users who validated this URL
    int64_t last_checked;            // Last time bot checked this URL
    bool is_active;                  // Whether URL is still valid
    double reliability_score;        // Reliability score (0.0 to 1.0)
    int validation_count;            // Number of validations
    int block_height;                // Block height when submitted
    
    ValidatedURL()
        : url_id(), url(), type(MeasurementType::WATER_PRICE), currency_code(),
          submitter(), last_checked(0), is_active(true), reliability_score(1.0),
          validation_count(0), block_height(0) {}
    
    SERIALIZE_METHODS(ValidatedURL, obj) {
        uint8_t type_val = static_cast<uint8_t>(obj.type);
        READWRITE(obj.url_id, obj.url, type_val, obj.currency_code,
                  obj.submitter, obj.validators, obj.last_checked, obj.is_active,
                  obj.reliability_score, obj.validation_count, obj.block_height);
        if (ser_action.ForRead()) obj.type = static_cast<MeasurementType>(type_val);
    }
    
    uint256 GetHash() const;
};

/** Daily Average Statistics */
struct DailyAverage {
    std::string currency_code;
    std::string date;                // YYYY-MM-DD format
    double avg_water_price;          // Average water price
    double avg_exchange_rate;        // Average exchange rate (if applicable)
    int measurement_count;           // Number of measurements used
    double std_deviation;            // Standard deviation
    bool is_stable;                  // Currency stability status
    int block_height;                // Block height when calculated
    
    DailyAverage()
        : currency_code(), date(), avg_water_price(0.0), avg_exchange_rate(0.0),
          measurement_count(0), std_deviation(0.0), is_stable(true), block_height(0) {}
    
    SERIALIZE_METHODS(DailyAverage, obj) {
        READWRITE(obj.currency_code, obj.date, obj.avg_water_price, obj.avg_exchange_rate,
                  obj.measurement_count, obj.std_deviation, obj.is_stable, obj.block_height);
    }
};

/** Reward amounts for different measurement types */
namespace Rewards {
    // O Blockchain uses 2 decimal places (100 cents per O)
    // This allows precision for water prices like $1.50, $2.25, etc.
    // Higher rewards to incentivize early adoption and quality data collection
    static constexpr CAmount WATER_PRICE_MEASUREMENT = 5000;     // 50.00 O (~$50-100)
    static constexpr CAmount WATER_PRICE_OFFLINE_VALIDATION = 3000;      // 30.00 O (~$30-60, requires travel)
    static constexpr CAmount EXCHANGE_RATE_MEASUREMENT = 5000;   // 50.00 O (~$50-100)
    static constexpr CAmount EXCHANGE_RATE_OFFLINE_VALIDATION = 3000;    // 30.00 O (~$30-60, requires travel)
    static constexpr CAmount URL_SUBMISSION = 2000;              // 20.00 O (~$20-40, valuable one-time contribution)
    static constexpr CAmount ONLINE_VALIDATION = 1000;           // 10.00 O (~$10-20)
}

/** Configuration parameters */
namespace Config {
    static constexpr int WATER_AVERAGE_TIME_LAPSE = 30;              // Days for water price average
    static constexpr int EXCHANGE_AVERAGE_TIME_LAPSE = 7;            // Days for exchange rate average
    static constexpr int DAILY_WATER_MEASUREMENT_TARGET = 100;       // Target measurements per day
    static constexpr int DAILY_EXCHANGE_MEASUREMENT_TARGET = 50;     // Target per day
    static constexpr int INVITE_EXPIRATION_DAYS = 7;                 // Invites expire after 7 days
    static constexpr int MIN_VALIDATORS_REQUIRED = 3;                // Minimum validators for confidence
    static constexpr double GAUSSIAN_STD_THRESHOLD = 2.0;            // Standard deviations for outliers
}

/** Measurement System Manager */
class MeasurementSystem {
public:
    MeasurementSystem();
    
    // ===== Water Price Measurements =====
    
    /** Submit a water price measurement */
    uint256 SubmitWaterPrice(const WaterPriceMeasurement& measurement);
    
    /** Validate a water price measurement */
    bool ValidateWaterPrice(const uint256& measurement_id, const CPubKey& validator);
    
    /** Get a water price measurement */
    std::optional<WaterPriceMeasurement> GetWaterPriceMeasurement(const uint256& measurement_id) const;
    
    /** Get all water price measurements for a currency in a time range */
    std::vector<WaterPriceMeasurement> GetWaterPricesInRange(
        const std::string& currency, int64_t start_time, int64_t end_time) const;
    
    // ===== Exchange Rate Measurements =====
    
    /** Submit an exchange rate measurement */
    uint256 SubmitExchangeRate(const ExchangeRateMeasurement& measurement);
    
    /** Validate an exchange rate measurement */
    bool ValidateExchangeRate(const uint256& measurement_id, const CPubKey& validator);
    
    /** Get an exchange rate measurement */
    std::optional<ExchangeRateMeasurement> GetExchangeRateMeasurement(const uint256& measurement_id) const;
    
    /** Get all exchange rate measurements in a time range */
    std::vector<ExchangeRateMeasurement> GetExchangeRatesInRange(
        const std::string& from_currency, const std::string& to_currency,
        int64_t start_time, int64_t end_time) const;
    
    // ===== Invitations =====
    
    /** Create measurement invites for users */
    std::vector<MeasurementInvite> CreateInvites(
        int count, MeasurementType type, const std::string& currency_code = "");
    
    /** Create invitations to achieve target measurement count */
    std::vector<MeasurementInvite> CreateInvitesForTargetMeasurements(
        int target_measurements, MeasurementType type, const std::string& currency_code = "");
    
    /** Check if invite is valid */
    bool IsInviteValid(const uint256& invite_id, int64_t current_time) const;
    
    /** Mark invite as used */
    bool MarkInviteUsed(const uint256& invite_id);
    
    /** Get invite by ID */
    std::optional<MeasurementInvite> GetInvite(const uint256& invite_id) const;
    
    /** Expire old invites */
    void ExpireOldInvites(int64_t current_time);
    
    // ===== Validated URLs =====
    
    /** Submit a URL for validation */
    uint256 SubmitURL(const ValidatedURL& url);
    
    /** Validate a URL */
    bool ValidateURL(const uint256& url_id, const CPubKey& validator);
    
    /** Get all active URLs for a type and currency */
    std::vector<ValidatedURL> GetActiveURLs(MeasurementType type, const std::string& currency) const;
    
    /** Update URL reliability score */
    void UpdateURLReliability(const uint256& url_id, bool successful);
    
    // ===== Statistics & Averages =====
    
    /** Calculate average water price for a currency */
    std::optional<double> GetAverageWaterPrice(const std::string& currency, int days) const;
    
    /** Calculate average exchange rate */
    std::optional<double> GetAverageExchangeRate(
        const std::string& from_currency, const std::string& to_currency, int days) const;
    
    /** Calculate Gaussian average (excludes outliers) */
    double CalculateGaussianAverage(const std::vector<double>& values) const;
    
    /** Get conversion rate (successful measurements / invites sent) */
    double GetConversionRate(MeasurementType type) const;
    
    /** Get daily average statistics */
    std::optional<DailyAverage> GetDailyAverage(const std::string& currency, const std::string& date) const;
    
    /** Store daily average */
    void StoreDailyAverage(const DailyAverage& avg);
    
    // ===== Rewards =====
    
    /** Calculate reward for a measurement type */
    CAmount CalculateReward(MeasurementType type, double user_reputation = 1.0) const;
    
    // ===== Utility Functions =====
    
    /** Get total measurements count */
    size_t GetTotalMeasurementCount() const;
    
    /** Get measurement statistics */
    std::map<std::string, int> GetMeasurementStatistics() const;
    
    /** Clear old data (for maintenance) */
    void PruneOldData(int64_t cutoff_time);

private:
    // Storage maps
    std::map<uint256, WaterPriceMeasurement> m_water_prices;
    std::map<uint256, ExchangeRateMeasurement> m_exchange_rates;
    std::map<uint256, MeasurementInvite> m_invites;
    std::map<uint256, ValidatedURL> m_validated_urls;
    std::map<std::string, DailyAverage> m_daily_averages;  // Key: currency_date
    
    // Statistics
    struct Statistics {
        int64_t total_invites_sent;
        int64_t total_measurements_received;
        int64_t total_validations;
        std::map<MeasurementType, int64_t> measurements_by_type;
    } m_stats;
    
    // Conversion rate tracking per currency and measurement type
    struct ConversionRate {
        int64_t invites_sent;
        int64_t measurements_completed;
        double rate;  // measurements_completed / invites_sent
        int64_t last_updated;
        
        ConversionRate() : invites_sent(0), measurements_completed(0), rate(0.5), last_updated(0) {}
    };
    
    // Key format: "currency_code:measurement_type" (e.g., "OUSD:0", "OEUR:1")
    std::map<std::string, ConversionRate> m_conversion_rates;
    
    // Helper functions
    double CalculateStandardDeviation(const std::vector<double>& values, double mean) const;
    std::vector<double> FilterOutliers(const std::vector<double>& values, double std_threshold) const;
    uint256 GenerateInviteId(const CPubKey& user, int64_t timestamp) const;
    std::vector<CPubKey> SelectRandomUsers(int count) const;
    
    // User selection helpers
    bool IsUserQualifiedForInvitation(const CPubKey& user) const;
    std::vector<CPubKey> SelectUsersForCurrency(const std::string& currency_code, int count) const;
    std::vector<CPubKey> GetUsersByCurrencyRegion(const std::string& currency_code) const;
    std::vector<CPubKey> GetRegionalUsers(const std::string& currency_code) const;
    std::vector<CPubKey> SelectWeightedRandom(const std::vector<CPubKey>& users, int count, bool allow_duplicates) const;
    
    // Conversion rate tracking
    double GetConversionRate(const std::string& currency_code, MeasurementType type) const;
    void UpdateConversionRate(const std::string& currency_code, MeasurementType type, bool measurement_completed);
    int CalculateInviteCountForTarget(int target_measurements, const std::string& currency_code, MeasurementType type) const;
};

/** Global measurement system instance */
extern MeasurementSystem g_measurement_system;

} // namespace OMeasurement

#endif // BITCOIN_MEASUREMENT_MEASUREMENT_SYSTEM_H
