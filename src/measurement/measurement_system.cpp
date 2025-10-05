// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <measurement/measurement_system.h>
#include <measurement/volume_conversion.h>
#include <hash.h>
#include <logging.h>
#include <random.h>
#include <util/time.h>

#include <algorithm>
#include <cmath>
#include <numeric>

namespace OMeasurement {

MeasurementSystem g_measurement_system;

// ===== Hash Functions =====

uint256 WaterPriceMeasurement::GetHash() const {
    HashWriter ss{};
    ss << measurement_id << submitter << currency_code << price << timestamp;
    return ss.GetHash();
}

uint256 ExchangeRateMeasurement::GetHash() const {
    HashWriter ss{};
    ss << measurement_id << submitter << from_currency << to_currency << timestamp;
    return ss.GetHash();
}

uint256 MeasurementInvite::GetHash() const {
    HashWriter ss{};
    ss << invite_id << invited_user << created_at;
    return ss.GetHash();
}

uint256 ValidatedURL::GetHash() const {
    HashWriter ss{};
    ss << url_id << url << submitter;
    return ss.GetHash();
}

bool MeasurementInvite::IsValid(int64_t current_time) const {
    if (is_used) return false;
    if (is_expired) return false;
    if (current_time > expires_at) return false;
    return true;
}

// ===== MeasurementSystem Implementation =====

MeasurementSystem::MeasurementSystem()
{
    m_stats.total_invites_sent = 0;
    m_stats.total_measurements_received = 0;
    m_stats.total_validations = 0;
    
    LogPrintf("O Measurement System: Initialized\n");
}

// ===== Water Price Measurements =====

uint256 MeasurementSystem::SubmitWaterPrice(const WaterPriceMeasurement& measurement)
{
    if (measurement.measurement_id.IsNull()) {
        LogPrintf("O Measurement: Invalid measurement ID\n");
        return uint256();
    }
    
    if (!IsInviteValidForUser(measurement.invite_id, measurement.submitter, measurement.timestamp)) {
        LogPrintf("O Measurement: Invalid invite or security violation - submitter does not match invited user\n");
        return uint256();
    }
    
    // CRITICAL: Validate volume and unit before accepting measurement
    VolumeValidationResult volume_validation = ValidateWaterPriceVolume(
        measurement.volume,
        measurement.volume_unit,
        measurement.price,
        measurement.currency_code
    );
    
    if (!volume_validation.is_valid) {
        LogPrintf("O Measurement: Volume validation failed - %s (Volume: %.4f %s, Price: %lld)\n",
                  volume_validation.error_message.c_str(),
                  measurement.volume,
                  measurement.volume_unit.c_str(),
                  measurement.price);
        return uint256();
    }
    
    // Store measurement with validated price_per_liter
    WaterPriceMeasurement validated_measurement = measurement;
    validated_measurement.price_per_liter = static_cast<int64_t>(volume_validation.price_per_liter);
    
    m_water_prices[measurement.measurement_id] = validated_measurement;
    MarkInviteUsed(measurement.invite_id);
    
    m_stats.total_measurements_received++;
    m_stats.measurements_by_type[MeasurementType::WATER_PRICE]++;
    
    LogPrintf("O Measurement: Water price submitted - Currency: %s, Container: %lld for %.4f %s, Price/L: %lld\n",
              measurement.currency_code.c_str(),
              measurement.price,
              measurement.volume,
              measurement.volume_unit.c_str(),
              validated_measurement.price_per_liter);
    
    // Trigger stability recalculation if we have enough measurements
    if (m_stats.total_measurements_received % 10 == 0) { // Every 10 measurements
        RecalculateCurrencyStability(0); // Use 0 for height when not in block context
    }
    
    return measurement.measurement_id;
}

bool MeasurementSystem::ValidateWaterPrice(const uint256& measurement_id, const CPubKey& validator)
{
    auto it = m_water_prices.find(measurement_id);
    if (it == m_water_prices.end()) {
        return false;
    }
    
    auto& measurement = it->second;
    
    for (const auto& v : measurement.validators) {
        if (v == validator) {
            return false;
        }
    }
    
    measurement.validators.push_back(validator);
    
    if (measurement.validators.size() >= static_cast<size_t>(Config::MIN_VALIDATORS_REQUIRED)) {
        measurement.is_validated = true;
        measurement.confidence_score = std::min(1.0, measurement.validators.size() / 10.0);
    }
    
    m_stats.total_validations++;
    
    return true;
}

std::optional<WaterPriceMeasurement> MeasurementSystem::GetWaterPriceMeasurement(
    const uint256& measurement_id) const
{
    auto it = m_water_prices.find(measurement_id);
    if (it == m_water_prices.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::vector<WaterPriceMeasurement> MeasurementSystem::GetWaterPricesInRange(
    const std::string& currency, int64_t start_time, int64_t end_time) const
{
    std::vector<WaterPriceMeasurement> results;
    
    for (const auto& [id, measurement] : m_water_prices) {
        if (measurement.currency_code == currency &&
            measurement.timestamp >= start_time &&
            measurement.timestamp <= end_time &&
            measurement.is_validated) {
            results.push_back(measurement);
        }
    }
    
    return results;
}

// ===== Exchange Rate Measurements =====

uint256 MeasurementSystem::SubmitExchangeRate(const ExchangeRateMeasurement& measurement)
{
    if (measurement.measurement_id.IsNull()) {
        LogPrintf("O Measurement: Invalid measurement ID for exchange rate submission\n");
        return uint256();
    }
    
    if (!IsInviteValidForUser(measurement.invite_id, measurement.submitter, measurement.timestamp)) {
        LogPrintf("O Measurement: Invalid invite or security violation for exchange rate - submitter does not match invited user\n");
        return uint256();
    }
    
    // Validate that this is an O currency to corresponding fiat currency pair
    if (!IsValidOCurrencyToFiatPair(measurement.from_currency, measurement.to_currency)) {
        LogPrintf("O Measurement: Invalid currency pair %s/%s - must be O currency to corresponding fiat currency\n",
                  measurement.from_currency.c_str(), measurement.to_currency.c_str());
        return uint256();
    }
    
    // Check stability of the O currency
    bool is_stable = IsOCurrencyStable(measurement.from_currency, measurement.exchange_rate);
    
    LogPrintf("O Measurement: Exchange rate submission for %s/%s - Rate: %.4f, Stable: %s\n",
              measurement.from_currency.c_str(), measurement.to_currency.c_str(),
              measurement.exchange_rate, is_stable ? "YES" : "NO");
    
    m_exchange_rates[measurement.measurement_id] = measurement;
    MarkInviteUsed(measurement.invite_id);
    
    m_stats.total_measurements_received++;
    m_stats.measurements_by_type[MeasurementType::EXCHANGE_RATE]++;
    
    // Trigger stability recalculation if we have enough measurements
    if (m_stats.total_measurements_received % 10 == 0) { // Every 10 measurements
        RecalculateCurrencyStability(0); // Use 0 for height when not in block context
    }
    
    return measurement.measurement_id;
}

bool MeasurementSystem::ValidateExchangeRate(const uint256& measurement_id, const CPubKey& validator)
{
    auto it = m_exchange_rates.find(measurement_id);
    if (it == m_exchange_rates.end()) {
        return false;
    }
    
    auto& measurement = it->second;
    
    for (const auto& v : measurement.validators) {
        if (v == validator) {
            return false;
        }
    }
    
    measurement.validators.push_back(validator);
    
    if (measurement.validators.size() >= static_cast<size_t>(Config::MIN_VALIDATORS_REQUIRED)) {
        measurement.is_validated = true;
    }
    
    m_stats.total_validations++;
    
    return true;
}

std::optional<ExchangeRateMeasurement> MeasurementSystem::GetExchangeRateMeasurement(
    const uint256& measurement_id) const
{
    auto it = m_exchange_rates.find(measurement_id);
    if (it == m_exchange_rates.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::vector<ExchangeRateMeasurement> MeasurementSystem::GetExchangeRatesInRange(
    const std::string& from_currency, const std::string& to_currency,
    int64_t start_time, int64_t end_time) const
{
    std::vector<ExchangeRateMeasurement> results;
    
    for (const auto& [id, measurement] : m_exchange_rates) {
        if (measurement.from_currency == from_currency &&
            measurement.to_currency == to_currency &&
            measurement.timestamp >= start_time &&
            measurement.timestamp <= end_time &&
            measurement.is_validated) {
            results.push_back(measurement);
        }
    }
    
    return results;
}

// Continued in next section...

} // namespace OMeasurement
