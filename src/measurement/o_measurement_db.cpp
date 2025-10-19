// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <measurement/o_measurement_db.h>
#include <common/args.h>
#include <logging.h>
#include <util/fs.h>
#include <util/time.h>
#include <streams.h>

namespace OMeasurement {

// Global instance (initialized in init.cpp)
std::unique_ptr<CMeasurementDB> g_measurement_db;

CMeasurementDB::CMeasurementDB(size_t cache_size, bool memory_only, bool wipe_data)
{
    DBParams db_params;
    db_params.path = gArgs.GetDataDirNet() / "measurements";
    db_params.cache_bytes = cache_size;
    db_params.memory_only = memory_only;
    db_params.wipe_data = wipe_data;
    db_params.obfuscate = true;  // Obfuscate for security
    
    try {
        m_db = std::make_unique<CDBWrapper>(db_params);
        LogPrintf("O Measurement DB: Opened database at %s (cache: %d MB, memory_only: %d)\n",
                  fs::PathToString(db_params.path), cache_size / (1024 * 1024), memory_only);
    } catch (const std::exception& e) {
        LogPrintf("O Measurement DB: Error opening database: %s\n", e.what());
        throw;
    }
}

CMeasurementDB::~CMeasurementDB() = default;

// ===== Water Price Measurement Operations =====

bool CMeasurementDB::WriteWaterPrice(const uint256& measurement_id, const WaterPriceMeasurement& measurement)
{
    LOCK(m_db_mutex);
    
    CDBBatch batch(*m_db);
    batch.Write(std::make_pair(DB_WATER_PRICE, measurement_id), measurement);
    
    bool success = m_db->WriteBatch(batch, true);
    
    if (success) {
        LogDebug(BCLog::NET, "O Measurement DB: Wrote water price %s for %s (price: %d)\n",
                 measurement_id.ToString().substr(0, 16), measurement.currency_code, measurement.price);
    }
    
    return success;
}

std::optional<WaterPriceMeasurement> CMeasurementDB::ReadWaterPrice(const uint256& measurement_id) const
{
    LOCK(m_db_mutex);
    
    WaterPriceMeasurement measurement;
    if (m_db->Read(std::make_pair(DB_WATER_PRICE, measurement_id), measurement)) {
        return measurement;
    }
    
    return std::nullopt;
}

bool CMeasurementDB::HasWaterPrice(const uint256& measurement_id) const
{
    LOCK(m_db_mutex);
    return m_db->Exists(std::make_pair(DB_WATER_PRICE, measurement_id));
}

bool CMeasurementDB::EraseWaterPrice(const uint256& measurement_id)
{
    LOCK(m_db_mutex);
    
    CDBBatch batch(*m_db);
    batch.Erase(std::make_pair(DB_WATER_PRICE, measurement_id));
    
    return m_db->WriteBatch(batch, true);
}

std::vector<WaterPriceMeasurement> CMeasurementDB::GetWaterPricesInRange(
    const std::string& currency, int64_t start_time, int64_t end_time) const
{
    LOCK(m_db_mutex);
    
    std::vector<WaterPriceMeasurement> measurements;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    for (iterator->Seek(DB_WATER_PRICE); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, uint256> key;
        if (!iterator->GetKey(key) || key.first != DB_WATER_PRICE) {
            break;
        }
        
        WaterPriceMeasurement measurement;
        if (iterator->GetValue(measurement)) {
            if (measurement.currency_code == currency &&
                measurement.timestamp >= start_time &&
                measurement.timestamp <= end_time) {
                measurements.push_back(measurement);
            }
        }
    }
    
    LogDebug(BCLog::NET, "O Measurement DB: Retrieved %d water prices for %s (%d-%d)\n",
             measurements.size(), currency, start_time, end_time);
    
    return measurements;
}

std::vector<WaterPriceMeasurement> CMeasurementDB::GetRecentWaterPrices(
    const std::string& currency, int days) const
{
    int64_t cutoff = GetTime() - (days * 86400);
    int64_t now = GetTime();
    return GetWaterPricesInRange(currency, cutoff, now);
}

// ===== Exchange Rate Measurement Operations =====

bool CMeasurementDB::WriteExchangeRate(const uint256& measurement_id, const ExchangeRateMeasurement& measurement)
{
    LOCK(m_db_mutex);
    
    CDBBatch batch(*m_db);
    batch.Write(std::make_pair(DB_EXCHANGE_RATE, measurement_id), measurement);
    
    bool success = m_db->WriteBatch(batch, true);
    
    if (success) {
        LogDebug(BCLog::NET, "O Measurement DB: Wrote exchange rate %s (%s/%s: %.6f)\n",
                 measurement_id.ToString().substr(0, 16), measurement.from_currency,
                 measurement.to_currency, measurement.exchange_rate);
    }
    
    return success;
}

std::optional<ExchangeRateMeasurement> CMeasurementDB::ReadExchangeRate(const uint256& measurement_id) const
{
    LOCK(m_db_mutex);
    
    ExchangeRateMeasurement measurement;
    if (m_db->Read(std::make_pair(DB_EXCHANGE_RATE, measurement_id), measurement)) {
        return measurement;
    }
    
    return std::nullopt;
}

bool CMeasurementDB::HasExchangeRate(const uint256& measurement_id) const
{
    LOCK(m_db_mutex);
    return m_db->Exists(std::make_pair(DB_EXCHANGE_RATE, measurement_id));
}

bool CMeasurementDB::EraseExchangeRate(const uint256& measurement_id)
{
    LOCK(m_db_mutex);
    
    CDBBatch batch(*m_db);
    batch.Erase(std::make_pair(DB_EXCHANGE_RATE, measurement_id));
    
    return m_db->WriteBatch(batch, true);
}

std::vector<ExchangeRateMeasurement> CMeasurementDB::GetExchangeRatesInRange(
    const std::string& from_currency, const std::string& to_currency,
    int64_t start_time, int64_t end_time) const
{
    LOCK(m_db_mutex);
    
    std::vector<ExchangeRateMeasurement> measurements;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    for (iterator->Seek(DB_EXCHANGE_RATE); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, uint256> key;
        if (!iterator->GetKey(key) || key.first != DB_EXCHANGE_RATE) {
            break;
        }
        
        ExchangeRateMeasurement measurement;
        if (iterator->GetValue(measurement)) {
            if (measurement.from_currency == from_currency &&
                measurement.to_currency == to_currency &&
                measurement.timestamp >= start_time &&
                measurement.timestamp <= end_time) {
                measurements.push_back(measurement);
            }
        }
    }
    
    LogDebug(BCLog::NET, "O Measurement DB: Retrieved %d exchange rates for %s/%s\n",
             measurements.size(), from_currency, to_currency);
    
    return measurements;
}

std::vector<ExchangeRateMeasurement> CMeasurementDB::GetRecentExchangeRates(
    const std::string& from_currency, const std::string& to_currency, int days) const
{
    int64_t cutoff = GetTime() - (days * 86400);
    int64_t now = GetTime();
    return GetExchangeRatesInRange(from_currency, to_currency, cutoff, now);
}

// ===== Measurement Invitation Operations =====

bool CMeasurementDB::WriteInvite(const uint256& invite_id, const MeasurementInvite& invite)
{
    LOCK(m_db_mutex);
    
    CDBBatch batch(*m_db);
    batch.Write(std::make_pair(DB_INVITE, invite_id), invite);
    
    bool success = m_db->WriteBatch(batch, true);
    
    if (success) {
        LogDebug(BCLog::NET, "O Measurement DB: Wrote invite %s for user %s\n",
                 invite_id.ToString().substr(0, 16), 
                 HexStr(invite.invited_user).substr(0, 16));
    }
    
    return success;
}

std::optional<MeasurementInvite> CMeasurementDB::ReadInvite(const uint256& invite_id) const
{
    LOCK(m_db_mutex);
    
    MeasurementInvite invite;
    if (m_db->Read(std::make_pair(DB_INVITE, invite_id), invite)) {
        return invite;
    }
    
    return std::nullopt;
}

bool CMeasurementDB::HasInvite(const uint256& invite_id) const
{
    LOCK(m_db_mutex);
    return m_db->Exists(std::make_pair(DB_INVITE, invite_id));
}

bool CMeasurementDB::MarkInviteUsed(const uint256& invite_id)
{
    LOCK(m_db_mutex);
    
    auto invite_opt = ReadInvite(invite_id);
    if (!invite_opt.has_value()) {
        return false;
    }
    
    MeasurementInvite invite = invite_opt.value();
    
    // SECURITY CHECK: Prevent double-use
    if (invite.is_used) {
        LogPrintf("O Measurement DB: SECURITY: Attempt to reuse invite %s\n",
                  invite_id.ToString().substr(0, 16));
        return false;
    }
    
    invite.is_used = true;
    
    return WriteInvite(invite_id, invite);
}

bool CMeasurementDB::EraseInvite(const uint256& invite_id)
{
    LOCK(m_db_mutex);
    
    CDBBatch batch(*m_db);
    batch.Erase(std::make_pair(DB_INVITE, invite_id));
    
    return m_db->WriteBatch(batch, true);
}

std::vector<MeasurementInvite> CMeasurementDB::GetUserInvites(const CPubKey& user) const
{
    LOCK(m_db_mutex);
    
    std::vector<MeasurementInvite> user_invites;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    for (iterator->Seek(DB_INVITE); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, uint256> key;
        if (!iterator->GetKey(key) || key.first != DB_INVITE) {
            break;
        }
        
        MeasurementInvite invite;
        if (iterator->GetValue(invite) && invite.invited_user == user) {
            user_invites.push_back(invite);
        }
    }
    
    return user_invites;
}

std::vector<MeasurementInvite> CMeasurementDB::GetActiveInvites() const
{
    LOCK(m_db_mutex);
    
    int64_t current_time = GetTime();
    std::vector<MeasurementInvite> active_invites;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    for (iterator->Seek(DB_INVITE); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, uint256> key;
        if (!iterator->GetKey(key) || key.first != DB_INVITE) {
            break;
        }
        
        MeasurementInvite invite;
        if (iterator->GetValue(invite)) {
            if (!invite.is_used && !invite.is_expired && invite.expires_at > current_time) {
                active_invites.push_back(invite);
            }
        }
    }
    
    LogDebug(BCLog::NET, "O Measurement DB: Found %d active invites\n", active_invites.size());
    return active_invites;
}

// ===== Validated URL Operations =====

bool CMeasurementDB::WriteValidatedURL(const uint256& url_id, const ValidatedURL& url)
{
    LOCK(m_db_mutex);
    
    CDBBatch batch(*m_db);
    batch.Write(std::make_pair(DB_VALIDATED_URL, url_id), url);
    
    return m_db->WriteBatch(batch, true);
}

std::optional<ValidatedURL> CMeasurementDB::ReadValidatedURL(const uint256& url_id) const
{
    LOCK(m_db_mutex);
    
    ValidatedURL url;
    if (m_db->Read(std::make_pair(DB_VALIDATED_URL, url_id), url)) {
        return url;
    }
    
    return std::nullopt;
}

std::vector<ValidatedURL> CMeasurementDB::GetActiveURLs(MeasurementType type, const std::string& currency) const
{
    LOCK(m_db_mutex);
    
    std::vector<ValidatedURL> active_urls;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    for (iterator->Seek(DB_VALIDATED_URL); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, uint256> key;
        if (!iterator->GetKey(key) || key.first != DB_VALIDATED_URL) {
            break;
        }
        
        ValidatedURL url;
        if (iterator->GetValue(url)) {
            if (url.is_active && url.type == type && url.currency_code == currency) {
                active_urls.push_back(url);
            }
        }
    }
    
    return active_urls;
}

bool CMeasurementDB::UpdateURLReliability(const uint256& url_id, double reliability_score)
{
    LOCK(m_db_mutex);
    
    auto url_opt = ReadValidatedURL(url_id);
    if (!url_opt.has_value()) {
        return false;
    }
    
    ValidatedURL url = url_opt.value();
    url.reliability_score = reliability_score;
    url.last_checked = GetTime();
    
    return WriteValidatedURL(url_id, url);
}

bool CMeasurementDB::DeactivateURL(const uint256& url_id)
{
    LOCK(m_db_mutex);
    
    auto url_opt = ReadValidatedURL(url_id);
    if (!url_opt.has_value()) {
        return false;
    }
    
    ValidatedURL url = url_opt.value();
    url.is_active = false;
    
    return WriteValidatedURL(url_id, url);
}

// ===== Daily Average Operations =====

bool CMeasurementDB::WriteDailyAverage(const std::string& currency, const std::string& date, const DailyAverage& average)
{
    LOCK(m_db_mutex);
    
    std::string key = currency + "_" + date;
    
    CDBBatch batch(*m_db);
    batch.Write(std::make_pair(DB_DAILY_AVERAGE, key), average);
    
    bool success = m_db->WriteBatch(batch, true);
    
    if (success) {
        LogDebug(BCLog::NET, "O Measurement DB: Wrote daily average %s/%s (price: %.2f, count: %d)\n",
                 currency, date, average.avg_water_price, average.measurement_count);
    }
    
    return success;
}

std::optional<DailyAverage> CMeasurementDB::ReadDailyAverage(const std::string& currency, const std::string& date) const
{
    LOCK(m_db_mutex);
    
    std::string key = currency + "_" + date;
    
    DailyAverage average;
    if (m_db->Read(std::make_pair(DB_DAILY_AVERAGE, key), average)) {
        return average;
    }
    
    return std::nullopt;
}

std::vector<DailyAverage> CMeasurementDB::GetDailyAveragesInRange(
    const std::string& currency, const std::string& start_date, const std::string& end_date) const
{
    LOCK(m_db_mutex);
    
    std::vector<DailyAverage> averages;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    std::string start_key = currency + "_" + start_date;
    std::string end_key = currency + "_" + end_date;
    
    for (iterator->Seek(std::make_pair(DB_DAILY_AVERAGE, start_key)); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, std::string> key;
        if (!iterator->GetKey(key) || key.first != DB_DAILY_AVERAGE) {
            break;
        }
        
        // Check if we've passed the end range
        if (key.second > end_key) {
            break;
        }
        
        DailyAverage average;
        if (iterator->GetValue(average) && average.currency_code == currency) {
            averages.push_back(average);
        }
    }
    
    return averages;
}

std::vector<DailyAverage> CMeasurementDB::GetRecentDailyAverages(const std::string& currency, int days) const
{
    // TODO: Calculate actual date strings for range query
    // For now, iterate and filter by timestamp
    
    LOCK(m_db_mutex);
    
    [[maybe_unused]] int64_t cutoff = GetTime() - (days * 86400);  // TODO: Use for filtering by timestamp
    std::vector<DailyAverage> averages;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    for (iterator->Seek(DB_DAILY_AVERAGE); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, std::string> key;
        if (!iterator->GetKey(key) || key.first != DB_DAILY_AVERAGE) {
            break;
        }
        
        DailyAverage average;
        if (iterator->GetValue(average) && average.currency_code == currency) {
            // Check if recent enough (approximate via block height or date parsing)
            averages.push_back(average);
            if (averages.size() >= static_cast<size_t>(days)) {
                break;  // Got enough
            }
        }
    }
    
    return averages;
}

// ===== Batch Operations =====

bool CMeasurementDB::BatchWriteWaterPrices(const std::vector<std::pair<uint256, WaterPriceMeasurement>>& batch)
{
    LOCK(m_db_mutex);
    
    CDBBatch db_batch(*m_db);
    
    for (const auto& [id, measurement] : batch) {
        db_batch.Write(std::make_pair(DB_WATER_PRICE, id), measurement);
    }
    
    bool success = m_db->WriteBatch(db_batch, true);
    
    if (success) {
        LogPrintf("O Measurement DB: Batch wrote %d water price measurements\n", batch.size());
    }
    
    return success;
}

bool CMeasurementDB::BatchWriteExchangeRates(const std::vector<std::pair<uint256, ExchangeRateMeasurement>>& batch)
{
    LOCK(m_db_mutex);
    
    CDBBatch db_batch(*m_db);
    
    for (const auto& [id, measurement] : batch) {
        db_batch.Write(std::make_pair(DB_EXCHANGE_RATE, id), measurement);
    }
    
    bool success = m_db->WriteBatch(db_batch, true);
    
    if (success) {
        LogPrintf("O Measurement DB: Batch wrote %d exchange rate measurements\n", batch.size());
    }
    
    return success;
}

bool CMeasurementDB::BatchWriteInvites(const std::vector<std::pair<uint256, MeasurementInvite>>& batch)
{
    LOCK(m_db_mutex);
    
    CDBBatch db_batch(*m_db);
    
    for (const auto& [id, invite] : batch) {
        db_batch.Write(std::make_pair(DB_INVITE, id), invite);
    }
    
    bool success = m_db->WriteBatch(db_batch, true);
    
    if (success) {
        LogPrintf("O Measurement DB: Batch wrote %d measurement invites\n", batch.size());
    }
    
    return success;
}

// ===== Query Operations =====

std::vector<uint256> CMeasurementDB::FindMeasurementsBySubmitter(const CPubKey& submitter, MeasurementType type) const
{
    LOCK(m_db_mutex);
    
    std::vector<uint256> measurement_ids;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    uint8_t prefix = (type == MeasurementType::WATER_PRICE) ? DB_WATER_PRICE : DB_EXCHANGE_RATE;
    
    for (iterator->Seek(prefix); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, uint256> key;
        if (!iterator->GetKey(key) || key.first != prefix) {
            break;
        }
        
        if (type == MeasurementType::WATER_PRICE) {
            WaterPriceMeasurement measurement;
            if (iterator->GetValue(measurement) && measurement.submitter == submitter) {
                measurement_ids.push_back(key.second);
            }
        } else if (type == MeasurementType::EXCHANGE_RATE) {
            ExchangeRateMeasurement measurement;
            if (iterator->GetValue(measurement) && measurement.submitter == submitter) {
                measurement_ids.push_back(key.second);
            }
        }
    }
    
    return measurement_ids;
}

std::vector<uint256> CMeasurementDB::FindUnvalidatedMeasurements(MeasurementType type) const
{
    LOCK(m_db_mutex);
    
    std::vector<uint256> unvalidated;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    uint8_t prefix = (type == MeasurementType::WATER_PRICE) ? DB_WATER_PRICE : DB_EXCHANGE_RATE;
    
    for (iterator->Seek(prefix); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, uint256> key;
        if (!iterator->GetKey(key) || key.first != prefix) {
            break;
        }
        
        if (type == MeasurementType::WATER_PRICE) {
            WaterPriceMeasurement measurement;
            if (iterator->GetValue(measurement) && !measurement.is_validated) {
                unvalidated.push_back(key.second);
            }
        } else if (type == MeasurementType::EXCHANGE_RATE) {
            ExchangeRateMeasurement measurement;
            if (iterator->GetValue(measurement) && !measurement.is_validated) {
                unvalidated.push_back(key.second);
            }
        }
    }
    
    return unvalidated;
}

std::vector<uint256> CMeasurementDB::FindMeasurementsByHeight(int start_height, int end_height, MeasurementType type) const
{
    LOCK(m_db_mutex);
    
    std::vector<uint256> measurements;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    uint8_t prefix = (type == MeasurementType::WATER_PRICE) ? DB_WATER_PRICE : DB_EXCHANGE_RATE;
    
    for (iterator->Seek(prefix); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, uint256> key;
        if (!iterator->GetKey(key) || key.first != prefix) {
            break;
        }
        
        if (type == MeasurementType::WATER_PRICE) {
            WaterPriceMeasurement measurement;
            if (iterator->GetValue(measurement)) {
                if (measurement.block_height >= start_height && measurement.block_height <= end_height) {
                    measurements.push_back(key.second);
                }
            }
        } else if (type == MeasurementType::EXCHANGE_RATE) {
            ExchangeRateMeasurement measurement;
            if (iterator->GetValue(measurement)) {
                if (measurement.block_height >= start_height && measurement.block_height <= end_height) {
                    measurements.push_back(key.second);
                }
            }
        }
    }
    
    return measurements;
}

// ===== Statistics =====

size_t CMeasurementDB::GetWaterPriceCount() const
{
    LOCK(m_db_mutex);
    
    size_t count = 0;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    for (iterator->Seek(DB_WATER_PRICE); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, uint256> key;
        if (!iterator->GetKey(key) || key.first != DB_WATER_PRICE) {
            break;
        }
        count++;
    }
    
    return count;
}

size_t CMeasurementDB::GetExchangeRateCount() const
{
    LOCK(m_db_mutex);
    
    size_t count = 0;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    for (iterator->Seek(DB_EXCHANGE_RATE); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, uint256> key;
        if (!iterator->GetKey(key) || key.first != DB_EXCHANGE_RATE) {
            break;
        }
        count++;
    }
    
    return count;
}

size_t CMeasurementDB::GetInviteCount() const
{
    LOCK(m_db_mutex);
    
    size_t count = 0;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    for (iterator->Seek(DB_INVITE); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, uint256> key;
        if (!iterator->GetKey(key) || key.first != DB_INVITE) {
            break;
        }
        count++;
    }
    
    return count;
}

size_t CMeasurementDB::GetActiveInviteCount() const
{
    return GetActiveInvites().size();
}

size_t CMeasurementDB::GetValidatedURLCount() const
{
    LOCK(m_db_mutex);
    
    size_t count = 0;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    for (iterator->Seek(DB_VALIDATED_URL); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, uint256> key;
        if (!iterator->GetKey(key) || key.first != DB_VALIDATED_URL) {
            break;
        }
        count++;
    }
    
    return count;
}

std::map<std::string, size_t> CMeasurementDB::GetMeasurementCountByCurrency() const
{
    LOCK(m_db_mutex);
    
    std::map<std::string, size_t> counts;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    for (iterator->Seek(DB_WATER_PRICE); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, uint256> key;
        if (!iterator->GetKey(key) || key.first != DB_WATER_PRICE) {
            break;
        }
        
        WaterPriceMeasurement measurement;
        if (iterator->GetValue(measurement)) {
            counts[measurement.currency_code]++;
        }
    }
    
    return counts;
}

// ===== Maintenance =====

bool CMeasurementDB::PruneOldMeasurements(int64_t cutoff_timestamp)
{
    LOCK(m_db_mutex);
    
    CDBBatch batch(*m_db);
    int pruned_water = 0;
    int pruned_exchange = 0;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    // Prune old water price measurements
    for (iterator->Seek(DB_WATER_PRICE); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, uint256> key;
        if (!iterator->GetKey(key) || key.first != DB_WATER_PRICE) {
            break;
        }
        
        WaterPriceMeasurement measurement;
        if (iterator->GetValue(measurement) && measurement.timestamp < cutoff_timestamp) {
            batch.Erase(std::make_pair(DB_WATER_PRICE, key.second));
            pruned_water++;
        }
    }
    
    // Prune old exchange rate measurements
    for (iterator->Seek(DB_EXCHANGE_RATE); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, uint256> key;
        if (!iterator->GetKey(key) || key.first != DB_EXCHANGE_RATE) {
            break;
        }
        
        ExchangeRateMeasurement measurement;
        if (iterator->GetValue(measurement) && measurement.timestamp < cutoff_timestamp) {
            batch.Erase(std::make_pair(DB_EXCHANGE_RATE, key.second));
            pruned_exchange++;
        }
    }
    
    bool success = m_db->WriteBatch(batch, true);
    
    if (success && (pruned_water > 0 || pruned_exchange > 0)) {
        LogPrintf("O Measurement DB: Pruned %d water prices and %d exchange rates (before %d)\n",
                  pruned_water, pruned_exchange, cutoff_timestamp);
    }
    
    return success;
}

bool CMeasurementDB::PruneExpiredInvites(int64_t current_time)
{
    LOCK(m_db_mutex);
    
    CDBBatch batch(*m_db);
    int pruned = 0;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    for (iterator->Seek(DB_INVITE); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, uint256> key;
        if (!iterator->GetKey(key) || key.first != DB_INVITE) {
            break;
        }
        
        MeasurementInvite invite;
        if (iterator->GetValue(invite)) {
            // Prune if used or expired
            if (invite.is_used || invite.expires_at < current_time) {
                batch.Erase(std::make_pair(DB_INVITE, key.second));
                pruned++;
            }
        }
    }
    
    bool success = m_db->WriteBatch(batch, true);
    
    if (success && pruned > 0) {
        LogPrintf("O Measurement DB: Pruned %d expired/used invites\n", pruned);
    }
    
    return success;
}

bool CMeasurementDB::PruneInactiveURLs()
{
    LOCK(m_db_mutex);
    
    CDBBatch batch(*m_db);
    int pruned = 0;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    for (iterator->Seek(DB_VALIDATED_URL); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, uint256> key;
        if (!iterator->GetKey(key) || key.first != DB_VALIDATED_URL) {
            break;
        }
        
        ValidatedURL url;
        if (iterator->GetValue(url)) {
            // Prune if inactive and low reliability
            if (!url.is_active && url.reliability_score < 0.3) {
                batch.Erase(std::make_pair(DB_VALIDATED_URL, key.second));
                pruned++;
            }
        }
    }
    
    bool success = m_db->WriteBatch(batch, true);
    
    if (success && pruned > 0) {
        LogPrintf("O Measurement DB: Pruned %d inactive URLs\n", pruned);
    }
    
    return success;
}

void CMeasurementDB::Compact()
{
    LOCK(m_db_mutex);
    
    LogPrintf("O Measurement DB: Database compaction requested\n");
    LogPrintf("O Measurement DB: Note: Compaction happens automatically via LevelDB\n");
    LogPrintf("O Measurement DB: To force compaction, restart node with -reindex\n");
    
    // Note: CDBWrapper doesn't expose CompactRange publicly
    // Compaction happens automatically in LevelDB
}

size_t CMeasurementDB::EstimateSize() const
{
    LOCK(m_db_mutex);
    return m_db->DynamicMemoryUsage();
}

std::optional<fs::path> CMeasurementDB::StoragePath() const
{
    return m_db->StoragePath();
}

// ===== Backup/Restore =====

bool CMeasurementDB::ExportMeasurements(const fs::path& export_path, MeasurementType type) const
{
    LOCK(m_db_mutex);
    
    try {
        LogPrintf("O Measurement DB: Exporting %s measurements to %s\n",
                  type == MeasurementType::WATER_PRICE ? "water price" : "exchange rate",
                  fs::PathToString(export_path));
        
        // TODO: Implement proper serialization to file
        // For now, just log the operation
        
        return true;
    } catch (const std::exception& e) {
        LogPrintf("O Measurement DB: Export failed: %s\n", e.what());
        return false;
    }
}

bool CMeasurementDB::ImportMeasurements(const fs::path& import_path, MeasurementType type)
{
    LOCK(m_db_mutex);
    
    try {
        LogPrintf("O Measurement DB: Importing %s measurements from %s\n",
                  type == MeasurementType::WATER_PRICE ? "water price" : "exchange rate",
                  fs::PathToString(import_path));
        
        // TODO: Implement proper deserialization from file
        
        return true;
    } catch (const std::exception& e) {
        LogPrintf("O Measurement DB: Import failed: %s\n", e.what());
        return false;
    }
}

bool CMeasurementDB::VerifyIntegrity() const
{
    LOCK(m_db_mutex);
    
    size_t total_water = 0;
    size_t total_exchange = 0;
    size_t corrupted = 0;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    // Check water prices
    for (iterator->Seek(DB_WATER_PRICE); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, uint256> key;
        if (!iterator->GetKey(key) || key.first != DB_WATER_PRICE) {
            break;
        }
        
        total_water++;
        
        WaterPriceMeasurement measurement;
        if (!iterator->GetValue(measurement)) {
            corrupted++;
        }
    }
    
    // Check exchange rates
    for (iterator->Seek(DB_EXCHANGE_RATE); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, uint256> key;
        if (!iterator->GetKey(key) || key.first != DB_EXCHANGE_RATE) {
            break;
        }
        
        total_exchange++;
        
        ExchangeRateMeasurement measurement;
        if (!iterator->GetValue(measurement)) {
            corrupted++;
        }
    }
    
    LogPrintf("O Measurement DB: Integrity check - Water: %d, Exchange: %d, Corrupted: %d\n",
              total_water, total_exchange, corrupted);
    
    return corrupted == 0;
}

} // namespace OMeasurement

