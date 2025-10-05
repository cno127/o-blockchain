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
    ONLINE_EXCHANGE_VALIDATION = 6,
    // New automated validation types
    ONLINE_WATER_PRICE_MEASUREMENT = 7,    // User provides URL + value
    OFFLINE_WATER_PRICE_MEASUREMENT = 8,   // User provides image + value + location
    ONLINE_EXCHANGE_RATE_MEASUREMENT = 9,  // User provides URL + value
    OFFLINE_EXCHANGE_RATE_MEASUREMENT = 10 // User provides image + value + location
};

/** Measurement Source Types */
enum class MeasurementSource : uint8_t {
    USER_ONLINE = 0,      // User provides online data
    USER_OFFLINE = 1,     // User provides offline data
    BOT_ONLINE = 2,       // Bot scrapes online data
    BOT_OFFLINE = 3       // Bot processes offline data
};

/** Automated Validation Result */
enum class AutomatedValidationResult : uint8_t {
    PASSED = 0,           // Passed all automated checks
    FAILED_GAUSSIAN = 1,  // Failed Gaussian range validation
    FAILED_TIMESTAMP = 2, // Failed timestamp validation (offline)
    FAILED_URL = 3,       // Failed URL validation (online)
    FAILED_LOCATION = 4,  // Failed location validation (offline)
    FAILED_FORMAT = 5     // Failed format validation
};

/** Automated Validation Information */
struct AutomatedValidationInfo {
    AutomatedValidationResult result;
    std::string failure_reason;
    double gaussian_deviation;       // How many standard deviations from mean
    bool timestamp_valid;            // For offline measurements
    bool url_accessible;             // For online measurements
    bool location_valid;             // For offline measurements
    int64_t validation_timestamp;    // When automated validation was performed
    
    AutomatedValidationInfo() 
        : result(AutomatedValidationResult::PASSED), failure_reason(), 
          gaussian_deviation(0.0), timestamp_valid(true), url_accessible(true),
          location_valid(true), validation_timestamp(0) {}
    
    SERIALIZE_METHODS(AutomatedValidationInfo, obj) {
        uint8_t result_val = static_cast<uint8_t>(obj.result);
        READWRITE(result_val, obj.failure_reason, obj.gaussian_deviation,
                  obj.timestamp_valid, obj.url_accessible, obj.location_valid,
                  obj.validation_timestamp);
        if (ser_action.ForRead()) obj.result = static_cast<AutomatedValidationResult>(result_val);
    }
};

/** Water Price Measurement Structure */
struct WaterPriceMeasurement {
    uint256 measurement_id;
    CPubKey submitter;
    std::string currency_code;       // USD, EUR, JPY, etc.
    int64_t price;                   // Price in smallest unit (cents, etc.) FOR THE CONTAINER
    double volume;                   // Volume of container (e.g., 1.0, 0.95, 33.8)
    std::string volume_unit;         // Unit: "L", "ml", "fl oz", etc.
    int64_t price_per_liter;         // Calculated price per liter (pro-rated)
    std::string location;            // Geographic location (for offline measurements)
    std::string source_url;          // Source URL (for online measurements)
    std::string proof_image_hash;    // IPFS or SHA256 hash of proof image
    int64_t timestamp;               // Unix timestamp
    int block_height;                // Block height when submitted
    bool is_validated;               // Whether validated by other users
    std::vector<CPubKey> validators; // Users who validated this measurement
    double confidence_score;         // Confidence score (0.0 to 1.0)
    uint256 invite_id;               // Invitation that prompted this measurement
    MeasurementSource source;        // Source type (user/bot, online/offline)
    AutomatedValidationInfo auto_validation; // Automated validation results
    
    WaterPriceMeasurement() 
        : measurement_id(), submitter(), currency_code(), price(0), volume(1.0), 
          volume_unit("L"), price_per_liter(0), location(),
          source_url(), proof_image_hash(), timestamp(0), block_height(0), is_validated(false),
          confidence_score(0.0), invite_id(), source(MeasurementSource::USER_ONLINE) {}
    
    SERIALIZE_METHODS(WaterPriceMeasurement, obj) {
        uint8_t source_val = static_cast<uint8_t>(obj.source);
        READWRITE(obj.measurement_id, obj.submitter, obj.currency_code, obj.price,
                  obj.volume, obj.volume_unit, obj.price_per_liter,
                  obj.location, obj.source_url, obj.proof_image_hash, obj.timestamp, obj.block_height,
                  obj.is_validated, obj.validators, obj.confidence_score, obj.invite_id,
                  source_val, obj.auto_validation);
        if (ser_action.ForRead()) obj.source = static_cast<MeasurementSource>(source_val);
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
    std::string location;            // Geographic location (for offline measurements)
    std::string source_url;          // Source URL (for online measurements)
    std::string proof_image_hash;    // IPFS or SHA256 hash of proof
    int64_t timestamp;               // Unix timestamp
    int block_height;                // Block height when submitted
    bool is_validated;               // Whether validated
    std::vector<CPubKey> validators; // Validators
    uint256 invite_id;               // Invitation ID
    MeasurementSource source;        // Source type (user/bot, online/offline)
    AutomatedValidationInfo auto_validation; // Automated validation results
    
    ExchangeRateMeasurement()
        : measurement_id(), submitter(), from_currency(), to_currency(),
          exchange_rate(0.0), location(), source_url(), proof_image_hash(), timestamp(0),
          block_height(0), is_validated(false), invite_id(), source(MeasurementSource::USER_ONLINE) {}
    
    SERIALIZE_METHODS(ExchangeRateMeasurement, obj) {
        uint8_t source_val = static_cast<uint8_t>(obj.source);
        READWRITE(obj.measurement_id, obj.submitter, obj.from_currency, obj.to_currency,
                  obj.exchange_rate, obj.location, obj.source_url, obj.proof_image_hash, obj.timestamp,
                  obj.block_height, obj.is_validated, obj.validators, obj.invite_id,
                  source_val, obj.auto_validation);
        if (ser_action.ForRead()) obj.source = static_cast<MeasurementSource>(source_val);
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

/** Statistical Confidence Levels */
enum class ConfidenceLevel {
    INSUFFICIENT_DATA = 0,    // Less than MIN_MEASUREMENTS_FOR_SIGNIFICANT_AVERAGE
    LOW_CONFIDENCE = 1,       // MIN_MEASUREMENTS_FOR_SIGNIFICANT_AVERAGE to MIN_MEASUREMENTS_FOR_HIGH_CONFIDENCE-1
    HIGH_CONFIDENCE = 2,      // MIN_MEASUREMENTS_FOR_HIGH_CONFIDENCE or more
    VERY_HIGH_CONFIDENCE = 3  // 20+ measurements
};

/** Average with Confidence Information */
struct AverageWithConfidence {
    double value;
    int measurement_count;
    double std_deviation;
    ConfidenceLevel confidence_level;
    bool is_statistically_significant;
    
    AverageWithConfidence() 
        : value(0.0), measurement_count(0), std_deviation(0.0), 
          confidence_level(ConfidenceLevel::INSUFFICIENT_DATA), is_statistically_significant(false) {}
    
    AverageWithConfidence(double val, int count, double std_dev)
        : value(val), measurement_count(count), std_deviation(std_dev) {
        UpdateConfidenceLevel();
    }
    
    void UpdateConfidenceLevel() {
        if (measurement_count < 5) { // MIN_MEASUREMENTS_FOR_SIGNIFICANT_AVERAGE
            confidence_level = ConfidenceLevel::INSUFFICIENT_DATA;
            is_statistically_significant = false;
        } else if (measurement_count < 10) { // MIN_MEASUREMENTS_FOR_HIGH_CONFIDENCE
            confidence_level = ConfidenceLevel::LOW_CONFIDENCE;
            is_statistically_significant = true;
        } else if (measurement_count < 20) {
            confidence_level = ConfidenceLevel::HIGH_CONFIDENCE;
            is_statistically_significant = true;
        } else {
            confidence_level = ConfidenceLevel::VERY_HIGH_CONFIDENCE;
            is_statistically_significant = true;
        }
    }
    
    std::string GetConfidenceString() const {
        switch (confidence_level) {
            case ConfidenceLevel::INSUFFICIENT_DATA: return "insufficient_data";
            case ConfidenceLevel::LOW_CONFIDENCE: return "low_confidence";
            case ConfidenceLevel::HIGH_CONFIDENCE: return "high_confidence";
            case ConfidenceLevel::VERY_HIGH_CONFIDENCE: return "very_high_confidence";
            default: return "unknown";
        }
    }
    
    SERIALIZE_METHODS(AverageWithConfidence, obj) {
        READWRITE(obj.value, obj.measurement_count, obj.std_deviation, 
                  obj.confidence_level, obj.is_statistically_significant);
    }
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
    ConfidenceLevel confidence_level; // Statistical confidence level
    bool is_statistically_significant; // Whether the average is statistically significant
    
    DailyAverage()
        : currency_code(), date(), avg_water_price(0.0), avg_exchange_rate(0.0),
          measurement_count(0), std_deviation(0.0), is_stable(true), block_height(0),
          confidence_level(ConfidenceLevel::INSUFFICIENT_DATA), is_statistically_significant(false) {}
    
    SERIALIZE_METHODS(DailyAverage, obj) {
        READWRITE(obj.currency_code, obj.date, obj.avg_water_price, obj.avg_exchange_rate,
                  obj.measurement_count, obj.std_deviation, obj.is_stable, obj.block_height,
                  obj.confidence_level, obj.is_statistically_significant);
    }
};

/** Reward amounts for different measurement types */
namespace Rewards {
    // O Blockchain uses 2 decimal places (100 cents per O)
    // This allows precision for water prices like $1.50, $2.25, etc.
    // Reward structure based on effort and value provided:
    // - Initial measurements: Base reward for data collection
    // - Online validation: Low effort, remote verification
    // - Offline validation: High effort, requires physical travel and verification
    
    static constexpr CAmount WATER_PRICE_MEASUREMENT = 5000;     // 50.00 O (~$50-100, initial measurement)
    static constexpr CAmount EXCHANGE_RATE_MEASUREMENT = 5000;   // 50.00 O (~$50-100, initial measurement)
    static constexpr CAmount URL_SUBMISSION = 2000;              // 20.00 O (~$20-40, valuable one-time contribution)
    
    // Validation rewards - significantly higher for offline due to travel requirements
    static constexpr CAmount ONLINE_VALIDATION = 1000;           // 10.00 O (~$10-20, remote verification)
    static constexpr CAmount WATER_PRICE_OFFLINE_VALIDATION = 8000;      // 80.00 O (~$80-160, requires travel + verification)
    static constexpr CAmount EXCHANGE_RATE_OFFLINE_VALIDATION = 8000;    // 80.00 O (~$80-160, requires travel + verification)
}

/** Configuration parameters */
namespace Config {
    static constexpr int WATER_AVERAGE_TIME_LAPSE = 30;              // Days for water price average
    static constexpr int EXCHANGE_AVERAGE_TIME_LAPSE = 7;            // Days for exchange rate average
    static constexpr int INVITE_EXPIRATION_DAYS = 7;                 // Invites expire after 7 days
    static constexpr int MIN_VALIDATORS_REQUIRED = 3;                // Minimum validators for confidence
    static constexpr double GAUSSIAN_STD_THRESHOLD = 2.0;            // Standard deviations for outliers
    
    // Statistical significance requirements
    static constexpr int MIN_MEASUREMENTS_FOR_SIGNIFICANT_AVERAGE = 5;    // Minimum measurements for statistical significance
    static constexpr int MIN_MEASUREMENTS_FOR_HIGH_CONFIDENCE = 10;       // Minimum measurements for high confidence
    static constexpr int MIN_MEASUREMENTS_FOR_DAILY_AVERAGE = 3;          // Minimum measurements for daily average
    
    // Dynamic measurement targets
    static constexpr int MIN_DAILY_MEASUREMENTS = 50;                // Minimum daily measurements (floor)
    static constexpr int MAX_DAILY_MEASUREMENTS = 300;               // Maximum daily measurements (ceiling)
    static constexpr int EARLY_STAGE_TARGET = 200;                   // Target when data is scarce (first 30 days)
    static constexpr int STABLE_TARGET = 75;                         // Target when measurements are stable
    static constexpr int VOLATILE_TARGET = 150;                      // Target when measurements are volatile
    static constexpr int VOLATILITY_LOOKBACK_DAYS = 7;               // Days to look back for volatility calculation
    static constexpr double HIGH_VOLATILITY_THRESHOLD = 0.15;        // 15% coefficient of variation = high volatility
    static constexpr double LOW_VOLATILITY_THRESHOLD = 0.05;         // 5% coefficient of variation = low volatility
    static constexpr int EARLY_STAGE_DAYS = 30;                      // Days considered "early stage" with scarce data
    
    // Automated validation parameters
    static constexpr double GAUSSIAN_ACCEPTANCE_THRESHOLD = 3.0;     // Accept measurements within 3 standard deviations
    static constexpr int OFFLINE_TIMESTAMP_TOLERANCE = 3600;         // 60 minutes in seconds
    static constexpr int URL_VALIDATION_TIMEOUT = 10;                // 10 seconds timeout for URL validation
    static constexpr int MIN_LOCATION_LENGTH = 3;                    // Minimum location string length
    static constexpr int MAX_LOCATION_LENGTH = 200;                  // Maximum location string length
    
    // Automatic invitation triggers
    static constexpr int AUTO_INVITE_CHECK_INTERVAL = 1800;          // Check every 30 minutes (1800 seconds)
    static constexpr int AUTO_INVITE_BLOCK_INTERVAL = 10;            // Check every 10 blocks
    static constexpr double MEASUREMENT_GAP_THRESHOLD = 0.8;         // Create invites when gap > 80% of target
    static constexpr int MAX_AUTO_INVITES_PER_CURRENCY = 50;         // Maximum auto invites per currency per check
    static constexpr int AUTO_INVITE_COOLDOWN = 3600;                // 1 hour cooldown between auto invites for same currency
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
    
    /** Check if measurement type is ready for invitations */
    bool CheckMeasurementReadiness(MeasurementType type, const std::string& currency_code) const;
    
    /** Get O currency code from fiat currency code */
    std::string GetOCurrencyFromFiat(const std::string& fiat_currency) const;
    
    /** Get string representation of measurement type */
    std::string GetMeasurementTypeString(MeasurementType type) const;
    
    // ===== Dynamic Measurement Targets =====
    
    /** Calculate dynamic measurement target based on volatility and data availability */
    int CalculateDynamicMeasurementTarget(MeasurementType type, const std::string& currency) const;
    
    /** Calculate volatility (coefficient of variation) for measurements */
    double CalculateVolatility(MeasurementType type, const std::string& currency, int days) const;
    
    /** Check if currency is in early stage (scarce data) */
    bool IsEarlyStage(MeasurementType type, const std::string& currency) const;
    
    /** Get current measurement target for a currency */
    int GetCurrentMeasurementTarget(MeasurementType type, const std::string& currency) const;
    
    /** Get measurement target statistics */
    std::map<std::string, int> GetMeasurementTargetStatistics() const;
    
    // ===== Automated Validation System =====
    
    /** Perform automated validation on a measurement */
    AutomatedValidationInfo PerformAutomatedValidation(const WaterPriceMeasurement& measurement) const;
    AutomatedValidationInfo PerformAutomatedValidation(const ExchangeRateMeasurement& measurement) const;
    
    /** Validate measurement value against Gaussian range */
    bool ValidateGaussianRange(MeasurementType type, const std::string& currency, double value, double& deviation) const;
    
    /** Validate timestamp for offline measurements (within 60 minutes) */
    bool ValidateTimestamp(int64_t measurement_timestamp, int64_t current_timestamp) const;
    
    /** Validate URL accessibility for online measurements */
    bool ValidateURL(const std::string& url) const;
    
    /** Validate location format for offline measurements */
    bool ValidateLocation(const std::string& location) const;
    
    /** Get Gaussian range for a currency */
    std::pair<double, double> GetGaussianRange(MeasurementType type, const std::string& currency) const;
    
    /** Submit measurement with automated validation */
    uint256 SubmitMeasurementWithValidation(const WaterPriceMeasurement& measurement);
    uint256 SubmitMeasurementWithValidation(const ExchangeRateMeasurement& measurement);
    
    // ===== Automatic Invitation Triggers =====
    
    /** Check and create invitations automatically based on targets */
    void CheckAndCreateInvitations();
    
    /** Monitor measurement targets for all currencies */
    void MonitorMeasurementTargets();
    
    /** Check if a currency needs more measurements */
    bool NeedsMoreMeasurements(MeasurementType type, const std::string& currency) const;
    
    /** Get measurement gap for a currency (target - current) */
    int GetMeasurementGap(MeasurementType type, const std::string& currency) const;
    
    /** Create automatic invitations for a currency if needed */
    void CreateAutomaticInvitations(MeasurementType type, const std::string& currency);
    
    /** Check if invite is valid */
    bool IsInviteValid(const uint256& invite_id, int64_t current_time) const;
    
    /** Check if invite is valid for specific user (SECURITY CHECK) */
    bool IsInviteValidForUser(const uint256& invite_id, const CPubKey& submitter, int64_t current_time) const;
    
    /** Mark invite as used */
    bool MarkInviteUsed(const uint256& invite_id);
    
    /** Get invite by ID */
    std::optional<MeasurementInvite> GetInvite(const uint256& invite_id) const;
    
    /** Get invite details for security auditing */
    std::optional<std::string> GetInviteDetails(const uint256& invite_id) const;
    
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
    
    /** Calculate average water price for a currency with confidence information */
    std::optional<AverageWithConfidence> GetAverageWaterPriceWithConfidence(const std::string& currency, int days) const;
    
    /** Calculate average exchange rate with confidence information */
    std::optional<AverageWithConfidence> GetAverageExchangeRateWithConfidence(
        const std::string& from_currency, const std::string& to_currency, int days) const;
    
    /** Calculate average water price for a currency (legacy - returns nullopt if not statistically significant) */
    std::optional<double> GetAverageWaterPrice(const std::string& currency, int days) const;
    
    /** Calculate average exchange rate (legacy - returns nullopt if not statistically significant) */
    std::optional<double> GetAverageExchangeRate(
        const std::string& from_currency, const std::string& to_currency, int days) const;
    
    /** Process O_ONLY currency measurement (water price in O coin) */
    void ProcessOOnlyCurrencyMeasurement(const std::string& currency,
                                       double water_price_in_o_coin,
                                       int height);
    
    // ===== O Currency ↔ Fiat Currency Validation =====
    
    /** Validate if currency pair is correct (O currency to its corresponding fiat) */
    bool IsValidOCurrencyToFiatPair(const std::string& o_currency, const std::string& fiat_currency) const;
    
    /** Get corresponding fiat currency for an O currency */
    std::string GetCorrespondingFiatCurrency(const std::string& o_currency) const;
    
    /** Get corresponding O currency for a fiat currency */
    std::string GetCorrespondingOCurrency(const std::string& fiat_currency) const;
    
    /** Check if currency is an O currency (prefixed with 'O') */
    bool IsOCurrency(const std::string& currency) const;
    
    /** Remove 'O' prefix from O currency to get fiat currency */
    std::string RemoveOPrefix(const std::string& o_currency) const;
    
    /** Add 'O' prefix to fiat currency to get O currency */
    std::string AddOPrefix(const std::string& fiat_currency) const;
    
    // ===== Water Price Stability Validation =====
    
    /** Check if O currency is stable based on water price comparison */
    bool IsOCurrencyStable(const std::string& o_currency, double measured_exchange_rate) const;
    
    /** Get theoretical exchange rate based on water price (should be 1:1) */
    double GetTheoreticalExchangeRate(const std::string& o_currency) const;
    
    /** Calculate stability deviation percentage */
    double CalculateStabilityDeviation(const std::string& o_currency, double measured_rate) const;
    
    /** Calculate Gaussian average (excludes outliers) */
    double CalculateGaussianAverage(const std::vector<double>& values) const;
    
    /** Calculate standard deviation */
    double CalculateStandardDeviation(const std::vector<double>& values) const;
    
    /** Get conversion rate (successful measurements / invites sent) */
    double GetConversionRate(MeasurementType type) const;
    
    /** Get daily average statistics */
    std::optional<DailyAverage> GetDailyAverage(const std::string& currency, const std::string& date) const;
    
    /** Store daily average */
    void StoreDailyAverage(const DailyAverage& avg);
    
    /** Calculate and store daily averages for all currencies */
    void CalculateDailyAverages(int height);
    
    /** Recalculate currency stability status based on current averages */
    void RecalculateCurrencyStability(int height);
    
    /** Get daily average water price for a specific date */
    std::optional<double> GetDailyAverageWaterPrice(const std::string& currency, const std::string& date) const;
    
    /** Get daily average exchange rate for O currency to fiat for a specific date */
    std::optional<double> GetDailyAverageExchangeRate(const std::string& o_currency, const std::string& date) const;
    
    /** Get all daily averages for a currency in date range */
    std::vector<DailyAverage> GetDailyAveragesInRange(const std::string& currency, 
                                                     const std::string& start_date, 
                                                     const std::string& end_date) const;

private:
    // Helper functions for daily average calculations
    void CalculateDailyAverageForCurrency(const std::string& currency, const std::string& date, int height);
    std::optional<double> GetDailyAverageWaterPriceInternal(const std::string& currency, const std::string& date) const;
    std::optional<double> GetDailyAverageExchangeRateInternal(const std::string& from_currency, const std::string& to_currency, const std::string& date) const;
    int GetDailyMeasurementCount(const std::string& currency, const std::string& date) const;
    double CalculateDailyStandardDeviation(const std::string& currency, const std::string& date) const;
    int64_t ParseDateToTimestamp(const std::string& date) const;
    
public:
    /** Format timestamp to YYYY-MM-DD string */
    std::string FormatDate(int64_t timestamp) const;
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
    
    // Automatic invitation cooldown tracking
    struct AutoInviteCooldown {
        int64_t last_invite_time;
        int invites_sent;
        
        AutoInviteCooldown() : last_invite_time(0), invites_sent(0) {}
    };
    
    // Key format: "currency_code:measurement_type" (e.g., "USD:0", "OUSD:1")
    std::map<std::string, AutoInviteCooldown> m_auto_invite_cooldowns;
    
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
