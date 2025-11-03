// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <measurement/measurement_system.h>
#include <measurement/volume_conversion.h>
#include <measurement/o_measurement_db.h>
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

// NOTE: SubmitWaterPrice() and ValidateWaterPrice() have been REMOVED.
// Measurements are now submitted via blockchain transactions (submitwaterpricetx RPC).
// All nodes automatically extract and validate measurements from the blockchain.
// See src/consensus/o_tx_validation.cpp for processing logic.

std::optional<WaterPriceMeasurement> MeasurementSystem::GetWaterPriceMeasurement(
    const uint256& measurement_id) const
{
    // Read from persistent database instead of RAM
    if (g_measurement_db) {
        return g_measurement_db->ReadWaterPrice(measurement_id);
    }
    return std::nullopt;
}

std::vector<WaterPriceMeasurement> MeasurementSystem::GetWaterPricesInRange(
    const std::string& currency, int64_t start_time, int64_t end_time) const
{
    // Query persistent database for water prices in range
    if (!g_measurement_db) {
        LogPrintf("O Measurement: Database not initialized\n");
        return std::vector<WaterPriceMeasurement>();
    }
    
    return g_measurement_db->GetWaterPricesInRange(currency, start_time, end_time);
}

// ===== Exchange Rate Measurements =====

// NOTE: SubmitExchangeRate() and ValidateExchangeRate() have been REMOVED.
// Exchange rate measurements are now submitted via blockchain transactions (submitexchangeratetx RPC).
// All nodes automatically extract and validate measurements from the blockchain.
// See src/consensus/o_tx_validation.cpp for processing logic.

std::optional<ExchangeRateMeasurement> MeasurementSystem::GetExchangeRateMeasurement(
    const uint256& measurement_id) const
{
    // Read from persistent database instead of RAM
    if (g_measurement_db) {
        return g_measurement_db->ReadExchangeRate(measurement_id);
    }
    return std::nullopt;
}

std::vector<ExchangeRateMeasurement> MeasurementSystem::GetExchangeRatesInRange(
    const std::string& from_currency, const std::string& to_currency,
    int64_t start_time, int64_t end_time) const
{
    // Query persistent database for exchange rates in range
    if (!g_measurement_db) {
        LogPrintf("O Measurement: Database not initialized\n");
        return std::vector<ExchangeRateMeasurement>();
    }
    
    return g_measurement_db->GetExchangeRatesInRange(from_currency, to_currency, start_time, end_time);
}

// Continued in next section...

} // namespace OMeasurement
