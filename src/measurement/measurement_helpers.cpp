// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <measurement/measurement_system.h>
#include <consensus/user_consensus.h>
#include <consensus/currency_lifecycle.h>
#include <consensus/currency_disappearance_handling.h>
#include <hash.h>
#include <logging.h>
#include <random.h>
#include <util/time.h>

#include <algorithm>
#include <cmath>
#include <numeric>

namespace OMeasurement {

// ===== Invitations =====

std::vector<MeasurementInvite> MeasurementSystem::CreateInvites(
    int count, MeasurementType type, const std::string& currency_code)
{
    std::vector<MeasurementInvite> invites;
    int64_t current_time = GetTime();
    int64_t expiration_time = current_time + (Config::INVITE_EXPIRATION_DAYS * 24 * 3600);
    
    // Use currency-specific user selection for better targeting
    std::vector<CPubKey> users;
    if (!currency_code.empty()) {
        users = SelectUsersForCurrency(currency_code, count);
        LogPrintf("O Measurement: Selected %d users for %s currency invitations\n", 
                  static_cast<int>(users.size()), currency_code.c_str());
    } else {
        users = SelectRandomUsers(count);
        LogPrintf("O Measurement: Selected %d users for general invitations\n", 
                  static_cast<int>(users.size()));
    }
    
    for (int i = 0; i < count && i < static_cast<int>(users.size()); ++i) {
        MeasurementInvite invite;
        invite.invite_id = GenerateInviteId(users[i], current_time + i);
        invite.invited_user = users[i];
        invite.type = type;
        invite.currency_code = currency_code;
        invite.created_at = current_time;
        invite.expires_at = expiration_time;
        invite.is_used = false;
        invite.is_expired = false;
        invite.block_height = 0;
        
        m_invites[invite.invite_id] = invite;
        invites.push_back(invite);
        
        m_stats.total_invites_sent++;
    }
    
    LogPrintf("O Measurement: Created %d invitations for %s\n", 
              static_cast<int>(invites.size()), currency_code.empty() ? "general" : currency_code.c_str());
    
    return invites;
}

std::vector<MeasurementInvite> MeasurementSystem::CreateInvitesForTargetMeasurements(
    int target_measurements, MeasurementType type, const std::string& currency_code)
{
    // Calculate how many invitations we need to send to achieve target measurements
    int invite_count = CalculateInviteCountForTarget(target_measurements, currency_code, type);
    
    LogPrintf("O Measurement: Target %d measurements, sending %d invitations (conversion rate: %.2f%%)\n",
              target_measurements, invite_count, GetConversionRate(currency_code, type) * 100.0);
    
    // Create the calculated number of invitations
    return CreateInvites(invite_count, type, currency_code);
}

bool MeasurementSystem::IsInviteValid(const uint256& invite_id, int64_t current_time) const
{
    auto it = m_invites.find(invite_id);
    if (it == m_invites.end()) {
        return false;
    }
    
    return it->second.IsValid(current_time);
}

bool MeasurementSystem::MarkInviteUsed(const uint256& invite_id)
{
    auto it = m_invites.find(invite_id);
    if (it == m_invites.end()) {
        return false;
    }
    
    it->second.is_used = true;
    return true;
}

std::optional<MeasurementInvite> MeasurementSystem::GetInvite(const uint256& invite_id) const
{
    auto it = m_invites.find(invite_id);
    if (it == m_invites.end()) {
        return std::nullopt;
    }
    return it->second;
}

void MeasurementSystem::ExpireOldInvites(int64_t current_time)
{
    int expired_count = 0;
    
    for (auto& [id, invite] : m_invites) {
        if (!invite.is_expired && current_time > invite.expires_at) {
            invite.is_expired = true;
            expired_count++;
        }
    }
    
    if (expired_count > 0) {
        LogPrintf("O Measurement: Expired %d old invites\n", expired_count);
    }
}

// ===== Validated URLs =====

uint256 MeasurementSystem::SubmitURL(const ValidatedURL& url)
{
    if (url.url_id.IsNull()) {
        return uint256();
    }
    
    m_validated_urls[url.url_id] = url;
    
    return url.url_id;
}

bool MeasurementSystem::ValidateURL(const uint256& url_id, const CPubKey& validator)
{
    auto it = m_validated_urls.find(url_id);
    if (it == m_validated_urls.end()) {
        return false;
    }
    
    auto& url = it->second;
    
    for (const auto& v : url.validators) {
        if (v == validator) {
            return false;
        }
    }
    
    url.validators.push_back(validator);
    url.validation_count++;
    
    return true;
}

std::vector<ValidatedURL> MeasurementSystem::GetActiveURLs(
    MeasurementType type, const std::string& currency) const
{
    std::vector<ValidatedURL> results;
    
    for (const auto& [id, url] : m_validated_urls) {
        if (url.type == type && url.is_active && url.currency_code == currency) {
            results.push_back(url);
        }
    }
    
    return results;
}

void MeasurementSystem::UpdateURLReliability(const uint256& url_id, bool successful)
{
    auto it = m_validated_urls.find(url_id);
    if (it == m_validated_urls.end()) {
        return;
    }
    
    auto& url = it->second;
    
    double alpha = 0.1;
    double new_score = successful ? 1.0 : 0.0;
    url.reliability_score = alpha * new_score + (1.0 - alpha) * url.reliability_score;
    
    if (url.reliability_score < 0.3) {
        url.is_active = false;
    }
    
    url.last_checked = GetTime();
}

// ===== Statistics & Averages =====

std::optional<double> MeasurementSystem::GetAverageWaterPrice(
    const std::string& currency, int days) const
{
    int64_t current_time = GetTime();
    int64_t start_time = current_time - (days * 24 * 3600);
    
    std::vector<WaterPriceMeasurement> measurements = 
        GetWaterPricesInRange(currency, start_time, current_time);
    
    if (measurements.empty()) {
        return std::nullopt;
    }
    
    std::vector<double> prices;
    for (const auto& m : measurements) {
        prices.push_back(static_cast<double>(m.price));
    }
    
    double avg = CalculateGaussianAverage(prices);
    
    return avg;
}

std::optional<double> MeasurementSystem::GetAverageExchangeRate(
    const std::string& from_currency, const std::string& to_currency, int days) const
{
    int64_t current_time = GetTime();
    int64_t start_time = current_time - (days * 24 * 3600);
    
    std::vector<ExchangeRateMeasurement> measurements = 
        GetExchangeRatesInRange(from_currency, to_currency, start_time, current_time);
    
    if (measurements.empty()) {
        return std::nullopt;
    }
    
    std::vector<double> rates;
    for (const auto& m : measurements) {
        rates.push_back(m.exchange_rate);
    }
    
    double sum = std::accumulate(rates.begin(), rates.end(), 0.0);
    return sum / rates.size();
}

double MeasurementSystem::CalculateGaussianAverage(const std::vector<double>& values) const
{
    if (values.empty()) return 0.0;
    if (values.size() == 1) return values[0];
    
    double sum = std::accumulate(values.begin(), values.end(), 0.0);
    double mean = sum / values.size();
    
    [[maybe_unused]] double std_dev = CalculateStandardDeviation(values, mean);
    
    std::vector<double> filtered = FilterOutliers(values, Config::GAUSSIAN_STD_THRESHOLD);
    
    if (filtered.empty()) {
        return mean;
    }
    
    sum = std::accumulate(filtered.begin(), filtered.end(), 0.0);
    return sum / filtered.size();
}

double MeasurementSystem::GetConversionRate(MeasurementType type) const
{
    int64_t invites_sent = 0;
    int64_t measurements_received = 0;
    
    for (const auto& [id, invite] : m_invites) {
        if (invite.type == type) {
            invites_sent++;
            if (invite.is_used) {
                measurements_received++;
            }
        }
    }
    
    if (invites_sent == 0) {
        return 0.0;
    }
    
    return static_cast<double>(measurements_received) / invites_sent;
}

std::optional<DailyAverage> MeasurementSystem::GetDailyAverage(
    const std::string& currency, const std::string& date) const
{
    std::string key = currency + "_" + date;
    auto it = m_daily_averages.find(key);
    if (it == m_daily_averages.end()) {
        return std::nullopt;
    }
    return it->second;
}

void MeasurementSystem::StoreDailyAverage(const DailyAverage& avg)
{
    std::string key = avg.currency_code + "_" + avg.date;
    m_daily_averages[key] = avg;
    
    LogPrintf("O Measurement: Stored daily average for %s on %s - "
              "Water price: %.4f, Exchange rate: %.4f, Measurements: %d, Stable: %s\n",
              avg.currency_code.c_str(), avg.date.c_str(), avg.avg_water_price,
              avg.avg_exchange_rate, avg.measurement_count, avg.is_stable ? "YES" : "NO");
}

void MeasurementSystem::CalculateDailyAverages(int height) {
    int64_t current_time = GetTime();
    std::string today = FormatDate(current_time);
    
    LogPrintf("O Measurement: Calculating daily averages for %s at height %d\n", 
              today.c_str(), height);
    
    // Get all supported currencies
    std::vector<std::string> currencies = {
        "OUSD", "OEUR", "OJPY", "OGBP", "OCNY", "OCAD", "OAUD", "OCHF", "ONZD",
        "OSEK", "ONOK", "ODKK", "OPLN", "OCZK", "OHUF", "OKRW", "OSGD", "OHKD",
        "OTWD", "OTHB", "OMYR", "OIDR", "OPHP", "OVND", "OINR", "OBRL", "ORUB",
        "OZAR", "OTRY", "OEGP", "OSAR", "OAED", "OILS", "OQAR", "OKWD", "OBHD",
        "OOMR", "OJOD", "OLBP", "OMAD", "OTND", "ODZD", "OMRO", "OLYD", "OXOF",
        "OXAF", "OXPF", "OALL", "OAMD", "OAZN", "OBYN", "OBGN", "OBIF", "OKHR",
        "OKGS", "OKZT", "OLAK", "OLSL", "OLTL", "OMDL", "OMKD", "OMNT", "ORON",
        "ORSD", "OTJS", "OTMT", "OUAH", "OUZS", "OXDR", "OZWL"
    };
    
    for (const auto& currency : currencies) {
        CalculateDailyAverageForCurrency(currency, today, height);
    }
}

void MeasurementSystem::CalculateDailyAverageForCurrency(const std::string& currency, 
                                                        const std::string& date, 
                                                        int height) {
    // Calculate daily average water price
    auto water_price_avg = GetDailyAverageWaterPriceInternal(currency, date);
    
    // Calculate daily average exchange rate (O currency to corresponding fiat)
    double exchange_rate_avg = 0.0;
    if (IsOCurrency(currency)) {
        std::string fiat_currency = GetCorrespondingFiatCurrency(currency);
        auto exchange_avg = GetDailyAverageExchangeRateInternal(currency, fiat_currency, date);
        if (exchange_avg.has_value()) {
            exchange_rate_avg = exchange_avg.value();
        }
    }
    
    // Determine if currency is stable
    bool is_stable = true;
    if (IsOCurrency(currency)) {
        is_stable = IsOCurrencyStable(currency, exchange_rate_avg);
    }
    
    // Create daily average record
    DailyAverage daily_avg;
    daily_avg.currency_code = currency;
    daily_avg.date = date;
    daily_avg.avg_water_price = water_price_avg.value_or(0.0);
    daily_avg.avg_exchange_rate = exchange_rate_avg;
    daily_avg.measurement_count = GetDailyMeasurementCount(currency, date);
    daily_avg.std_deviation = CalculateDailyStandardDeviation(currency, date);
    daily_avg.is_stable = is_stable;
    daily_avg.block_height = height;
    
    // Store the daily average
    StoreDailyAverage(daily_avg);
}

std::optional<double> MeasurementSystem::GetDailyAverageWaterPrice(const std::string& currency, const std::string& date) const {
    auto daily_avg = GetDailyAverage(currency, date);
    if (daily_avg.has_value()) {
        return daily_avg->avg_water_price;
    }
    return std::nullopt;
}

std::optional<double> MeasurementSystem::GetDailyAverageExchangeRate(const std::string& o_currency, const std::string& date) const {
    if (!IsOCurrency(o_currency)) {
        return std::nullopt;
    }
    
    auto daily_avg = GetDailyAverage(o_currency, date);
    if (daily_avg.has_value()) {
        return daily_avg->avg_exchange_rate;
    }
    return std::nullopt;
}

std::vector<DailyAverage> MeasurementSystem::GetDailyAveragesInRange(const std::string& currency, 
                                                                     const std::string& start_date, 
                                                                     const std::string& end_date) const {
    std::vector<DailyAverage> results;
    
    // Simple implementation - in practice, you'd parse dates and iterate through date range
    for (const auto& [key, daily_avg] : m_daily_averages) {
        if (daily_avg.currency_code == currency && 
            daily_avg.date >= start_date && 
            daily_avg.date <= end_date) {
            results.push_back(daily_avg);
        }
    }
    
    // Sort by date
    std::sort(results.begin(), results.end(), 
              [](const DailyAverage& a, const DailyAverage& b) {
                  return a.date < b.date;
              });
    
    return results;
}

// Helper functions for daily average calculations

std::optional<double> MeasurementSystem::GetDailyAverageWaterPriceInternal(const std::string& currency, const std::string& date) const {
    // Parse date to get start and end timestamps for the day
    int64_t start_time = ParseDateToTimestamp(date);
    int64_t end_time = start_time + 24 * 3600 - 1; // End of day
    
    std::vector<WaterPriceMeasurement> measurements = GetWaterPricesInRange(currency, start_time, end_time);
    
    if (measurements.empty()) {
        return std::nullopt;
    }
    
    std::vector<double> prices;
    for (const auto& m : measurements) {
        if (m.is_validated) {
            prices.push_back(static_cast<double>(m.price) / 100.0); // Convert from cents
        }
    }
    
    if (prices.empty()) {
        return std::nullopt;
    }
    
    return CalculateGaussianAverage(prices);
}

std::optional<double> MeasurementSystem::GetDailyAverageExchangeRateInternal(const std::string& from_currency, const std::string& to_currency, const std::string& date) const {
    // Parse date to get start and end timestamps for the day
    int64_t start_time = ParseDateToTimestamp(date);
    int64_t end_time = start_time + 24 * 3600 - 1; // End of day
    
    std::vector<ExchangeRateMeasurement> measurements = GetExchangeRatesInRange(from_currency, to_currency, start_time, end_time);
    
    if (measurements.empty()) {
        return std::nullopt;
    }
    
    std::vector<double> rates;
    for (const auto& m : measurements) {
        if (m.is_validated) {
            rates.push_back(m.exchange_rate);
        }
    }
    
    if (rates.empty()) {
        return std::nullopt;
    }
    
    return CalculateGaussianAverage(rates);
}

int MeasurementSystem::GetDailyMeasurementCount(const std::string& currency, const std::string& date) const {
    int64_t start_time = ParseDateToTimestamp(date);
    int64_t end_time = start_time + 24 * 3600 - 1;
    
    std::vector<WaterPriceMeasurement> water_measurements = GetWaterPricesInRange(currency, start_time, end_time);
    
    int count = 0;
    for (const auto& m : water_measurements) {
        if (m.is_validated) count++;
    }
    
    // Add exchange rate measurements if it's an O currency
    if (IsOCurrency(currency)) {
        std::string fiat_currency = GetCorrespondingFiatCurrency(currency);
        std::vector<ExchangeRateMeasurement> exchange_measurements = GetExchangeRatesInRange(currency, fiat_currency, start_time, end_time);
        
        for (const auto& m : exchange_measurements) {
            if (m.is_validated) count++;
        }
    }
    
    return count;
}

double MeasurementSystem::CalculateDailyStandardDeviation(const std::string& currency, const std::string& date) const {
    int64_t start_time = ParseDateToTimestamp(date);
    int64_t end_time = start_time + 24 * 3600 - 1;
    
    std::vector<WaterPriceMeasurement> measurements = GetWaterPricesInRange(currency, start_time, end_time);
    
    if (measurements.empty()) {
        return 0.0;
    }
    
    std::vector<double> prices;
    for (const auto& m : measurements) {
        if (m.is_validated) {
            prices.push_back(static_cast<double>(m.price) / 100.0);
        }
    }
    
    if (prices.size() < 2) {
        return 0.0;
    }
    
    double mean = std::accumulate(prices.begin(), prices.end(), 0.0) / prices.size();
    return CalculateStandardDeviation(prices, mean);
}

std::string MeasurementSystem::FormatDate(int64_t timestamp) const {
    // Simple date formatting - in practice, you'd use a proper date library
    time_t time = static_cast<time_t>(timestamp);
    struct tm* tm_info = gmtime(&time);
    
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d", tm_info);
    return std::string(buffer);
}

int64_t MeasurementSystem::ParseDateToTimestamp(const std::string& date) const {
    // Simple date parsing - in practice, you'd use a proper date library
    // Format: YYYY-MM-DD
    if (date.length() != 10 || date[4] != '-' || date[7] != '-') {
        return 0;
    }
    
    int year = std::stoi(date.substr(0, 4));
    int month = std::stoi(date.substr(5, 2));
    int day = std::stoi(date.substr(8, 2));
    
    struct tm tm = {};
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = 0;
    tm.tm_min = 0;
    tm.tm_sec = 0;
    
    return static_cast<int64_t>(timegm(&tm));
}

CAmount MeasurementSystem::CalculateReward(MeasurementType type, double user_reputation) const
{
    CAmount base_reward = 0;
    
    switch (type) {
        case MeasurementType::WATER_PRICE:
            base_reward = Rewards::WATER_PRICE_MEASUREMENT;
            break;
        case MeasurementType::WATER_PRICE_OFFLINE_VALIDATION:
            base_reward = Rewards::WATER_PRICE_OFFLINE_VALIDATION;
            break;
        case MeasurementType::EXCHANGE_RATE:
            base_reward = Rewards::EXCHANGE_RATE_MEASUREMENT;
            break;
        case MeasurementType::EXCHANGE_RATE_OFFLINE_VALIDATION:
            base_reward = Rewards::EXCHANGE_RATE_OFFLINE_VALIDATION;
            break;
        case MeasurementType::URL_SUBMISSION:
            base_reward = Rewards::URL_SUBMISSION;
            break;
        case MeasurementType::ONLINE_WATER_PRICE_VALIDATION:
        case MeasurementType::ONLINE_EXCHANGE_VALIDATION:
            base_reward = Rewards::ONLINE_VALIDATION;
            break;
    }
    
    double reputation_factor = 0.5 + (user_reputation * 0.5);
    reputation_factor = std::max(0.5, std::min(1.5, reputation_factor));
    
    return static_cast<CAmount>(base_reward * reputation_factor);
}

size_t MeasurementSystem::GetTotalMeasurementCount() const
{
    return m_water_prices.size() + m_exchange_rates.size();
}

std::map<std::string, int> MeasurementSystem::GetMeasurementStatistics() const
{
    std::map<std::string, int> stats;
    stats["total_water_prices"] = m_water_prices.size();
    stats["total_exchange_rates"] = m_exchange_rates.size();
    stats["total_invites"] = m_invites.size();
    stats["total_urls"] = m_validated_urls.size();
    stats["invites_sent"] = m_stats.total_invites_sent;
    stats["measurements_received"] = m_stats.total_measurements_received;
    stats["validations"] = m_stats.total_validations;
    
    return stats;
}

void MeasurementSystem::PruneOldData(int64_t cutoff_time)
{
    int pruned = 0;
    
    auto water_it = m_water_prices.begin();
    while (water_it != m_water_prices.end()) {
        if (water_it->second.timestamp < cutoff_time) {
            water_it = m_water_prices.erase(water_it);
            pruned++;
        } else {
            ++water_it;
        }
    }
    
    auto exchange_it = m_exchange_rates.begin();
    while (exchange_it != m_exchange_rates.end()) {
        if (exchange_it->second.timestamp < cutoff_time) {
            exchange_it = m_exchange_rates.erase(exchange_it);
            pruned++;
        } else {
            ++exchange_it;
        }
    }
    
    if (pruned > 0) {
        LogPrintf("O Measurement: Pruned %d old measurements\n", pruned);
    }
}

// ===== Helper Functions =====

double MeasurementSystem::CalculateStandardDeviation(
    const std::vector<double>& values, double mean) const
{
    if (values.size() <= 1) return 0.0;
    
    double sum_squared_diff = 0.0;
    for (double val : values) {
        double diff = val - mean;
        sum_squared_diff += diff * diff;
    }
    
    return std::sqrt(sum_squared_diff / (values.size() - 1));
}

std::vector<double> MeasurementSystem::FilterOutliers(
    const std::vector<double>& values, double std_threshold) const
{
    if (values.empty()) return {};
    
    double sum = std::accumulate(values.begin(), values.end(), 0.0);
    double mean = sum / values.size();
    
    double std_dev = CalculateStandardDeviation(values, mean);
    
    std::vector<double> filtered;
    for (double val : values) {
        if (std::abs(val - mean) <= std_threshold * std_dev) {
            filtered.push_back(val);
        }
    }
    
    return filtered;
}

uint256 MeasurementSystem::GenerateInviteId(const CPubKey& user, int64_t timestamp) const
{
    HashWriter ss{};
    ss << user << timestamp << GetRandHash();
    return ss.GetHash();
}

std::vector<CPubKey> MeasurementSystem::SelectRandomUsers(int count) const
{
    // Get all verified users from user consensus system
    // TODO: Replace with actual user registry when available
    std::vector<CPubKey> verified_users;
    // std::vector<CPubKey> verified_users = g_user_registry.GetVerifiedUsers();
    
    if (verified_users.empty()) {
        LogPrintf("O Measurement: No verified users available for invitations\n");
        return {};
    }
    
    // Filter by reputation and recent activity
    std::vector<CPubKey> qualified_users;
    for (const auto& user : verified_users) {
        if (IsUserQualifiedForInvitation(user)) {
            qualified_users.push_back(user);
        }
    }
    
    if (qualified_users.empty()) {
        LogPrintf("O Measurement: No qualified users available for invitations\n");
        return {};
    }
    
    // Random selection from qualified users
    std::vector<CPubKey> selected_users;
    FastRandomContext rng;
    
    for (int i = 0; i < count && i < static_cast<int>(qualified_users.size()); ++i) {
        int random_index = rng.randrange(qualified_users.size());
        selected_users.push_back(qualified_users[random_index]);
        // Remove selected user to avoid duplicates in same batch
        qualified_users.erase(qualified_users.begin() + random_index);
    }
    
    LogPrintf("O Measurement: Selected %d users from %d qualified users\n", 
              static_cast<int>(selected_users.size()), static_cast<int>(verified_users.size()));
    
    return selected_users;
}

// ===== User Selection Helpers =====

bool MeasurementSystem::IsUserQualifiedForInvitation(const CPubKey& user) const
{
    // TODO: Implement user qualification check when user registry is available
    // For now, accept all users as qualified
    // Check if user is verified and active
    // auto user_info = g_user_registry.GetUserInfo(user);
    // if (!user_info || user_info->status != UserStatus::VERIFIED) {
    //     return false;
    // }
    
    // Check if user has been active recently (within last 30 days)
    // int64_t current_time = GetTime();
    // int64_t thirty_days_ago = current_time - (30 * 24 * 3600);
    
    // if (user_info->last_activity_time < thirty_days_ago) {
    //     return false;
    // }
    
    // Check if user has good reputation (above minimum threshold)
    // if (user_info->reputation_score < 0.5) {  // Minimum 50% reputation
    //     return false;
    // }
    
    // Check if user is not suspended or blacklisted
    // if (user_info->status == UserStatus::SUSPENDED || 
    //     user_info->status == UserStatus::BLACKLISTED) {
    //     return false;
    // }
    
    return true;
}

std::vector<CPubKey> MeasurementSystem::SelectUsersForCurrency(const std::string& currency_code, int count) const
{
    // Get users from the currency's native region
    std::vector<CPubKey> native_users = GetUsersByCurrencyRegion(currency_code);
    
    // Filter native users by qualification
    std::vector<CPubKey> qualified_native;
    for (const auto& user : native_users) {
        if (IsUserQualifiedForInvitation(user)) {
            qualified_native.push_back(user);
        }
    }
    
    // If we have enough native users, use them (allow multiple invitations)
    if (static_cast<int>(qualified_native.size()) >= count) {
        return SelectWeightedRandom(qualified_native, count, true);  // Allow duplicates
    }
    
    // Fill remaining slots with regional users
    std::vector<CPubKey> regional_users = GetRegionalUsers(currency_code);
    std::vector<CPubKey> qualified_regional;
    for (const auto& user : regional_users) {
        if (IsUserQualifiedForInvitation(user)) {
            qualified_regional.push_back(user);
        }
    }
    
    // Combine native and regional users
    std::vector<CPubKey> all_qualified = qualified_native;
    all_qualified.insert(all_qualified.end(), qualified_regional.begin(), qualified_regional.end());
    
    return SelectWeightedRandom(all_qualified, count, false);  // No duplicates
}

std::vector<CPubKey> MeasurementSystem::GetUsersByCurrencyRegion(const std::string& currency_code) const
{
    // Map currency codes to their native regions
    std::map<std::string, std::vector<std::string>> currency_regions = {
        // North America
        {"OUSD", {"US", "USA"}},
        {"OCAD", {"CA", "CAN"}},
        {"OMXN", {"MX", "MEX"}},
        
        // Europe
        {"OEUR", {"DE", "FR", "IT", "ES", "NL", "BE", "AT", "PT", "FI", "IE", "GR", "LU", "MT", "CY", "SK", "SI", "EE", "LV", "LT"}},
        {"OGBP", {"GB", "UK"}},
        {"OCHF", {"CH", "CHE"}},
        {"OSEK", {"SE", "SWE"}},
        {"ONOK", {"NO", "NOR"}},
        {"ODKK", {"DK", "DNK"}},
        {"OPLN", {"PL", "POL"}},
        {"OCZK", {"CZ", "CZE"}},
        {"OHUF", {"HU", "HUN"}},
        
        // Asia
        {"OJPY", {"JP", "JPN"}},
        {"OCNY", {"CN", "CHN"}},
        {"OKRW", {"KR", "KOR"}},
        {"OSGD", {"SG", "SGP"}},
        {"OHKD", {"HK", "HKG"}},
        {"OTWD", {"TW", "TWN"}},
        {"OTHB", {"TH", "THA"}},
        {"OMYR", {"MY", "MYS"}},
        {"OIDR", {"ID", "IDN"}},
        {"OPHP", {"PH", "PHL"}},
        {"OVND", {"VN", "VNM"}},
        {"OINR", {"IN", "IND"}},
        
        // Middle East & Africa
        {"OAED", {"AE", "ARE"}},
        {"OSAR", {"SA", "SAU"}},
        {"OQAR", {"QA", "QAT"}},
        {"OKWD", {"KW", "KWT"}},
        {"OBHD", {"BH", "BHR"}},
        {"OOMR", {"OM", "OMN"}},
        {"OJOD", {"JO", "JOR"}},
        {"OILS", {"IL", "ISR"}},
        {"OTRY", {"TR", "TUR"}},
        {"OEGP", {"EG", "EGY"}},
        {"OZAR", {"ZA", "ZAF"}},
        {"ONGN", {"NG", "NGA"}},
        {"OKES", {"KE", "KEN"}},
        {"OETB", {"ET", "ETH"}},
        
        // Americas
        {"OBRL", {"BR", "BRA"}},
        {"OARS", {"AR", "ARG"}},
        {"OCLP", {"CL", "CHL"}},
        {"OCOP", {"CO", "COL"}},
        {"OPEN", {"PE", "PER"}},
        {"OUYU", {"UY", "URY"}},
        {"OVES", {"VE", "VEN"}},
        
        // Others
        {"OAUD", {"AU", "AUS"}},
        {"ONZD", {"NZ", "NZL"}},
        {"OISK", {"IS", "ISL"}},
        {"OLKR", {"LK", "LKA"}},
        {"OBDT", {"BD", "BGD"}},
        {"OPKR", {"PK", "PAK"}},
        {"OAFN", {"AF", "AFG"}},
        {"OIQD", {"IQ", "IRQ"}},
        {"OIRR", {"IR", "IRN"}},
        {"OLBP", {"LB", "LBN"}},
        {"OSYP", {"SY", "SYR"}},
        {"OYER", {"YE", "YEM"}}
    };
    
    auto it = currency_regions.find(currency_code);
    if (it == currency_regions.end()) {
        LogPrintf("O Measurement: Unknown currency code %s\n", currency_code.c_str());
        return {};
    }
    
    // Get all verified users
    // TODO: Replace with actual user registry when available
    std::vector<CPubKey> all_users;
    // std::vector<CPubKey> all_users = g_user_registry.GetVerifiedUsers();
    std::vector<CPubKey> regional_users;
    
    // Filter users by region
    // TODO: Implement region filtering when user registry is available
    // for (const auto& user : all_users) {
    //     auto user_info = g_user_registry.GetUserInfo(user);
    //     if (user_info && user_info->status == UserStatus::VERIFIED) {
    //         // Check if user's region matches currency region
    //         for (const auto& region : it->second) {
    //             if (user_info->country_code == region) {
    //                 regional_users.push_back(user);
    //                 break;
    //             }
    //         }
    //     }
    // }
    
    LogPrintf("O Measurement: Found %d users in %s region\n", 
              static_cast<int>(regional_users.size()), currency_code.c_str());
    
    return regional_users;
}

std::vector<CPubKey> MeasurementSystem::GetRegionalUsers(const std::string& currency_code) const
{
    // Get users from neighboring regions (not native but close)
    std::map<std::string, std::vector<std::string>> regional_mapping = {
        // North America - some overlap between US/Canada/Mexico
        {"OUSD", {"CA", "MX"}},
        {"OCAD", {"US", "MX"}},
        {"OMXN", {"US", "CA"}},
        
        // Europe - neighboring countries
        {"OEUR", {"GB", "CH", "NO", "SE", "DK"}},
        {"OGBP", {"IE", "FR", "NL", "BE"}},
        {"OCHF", {"DE", "FR", "IT", "AT"}},
        
        // Asia - neighboring countries
        {"OJPY", {"KR", "CN", "TW"}},
        {"OCNY", {"HK", "TW", "JP", "KR"}},
        {"OKRW", {"JP", "CN", "TW"}},
        
        // Add more regional mappings as needed
    };
    
    auto it = regional_mapping.find(currency_code);
    if (it == regional_mapping.end()) {
        return {};  // No regional users for this currency
    }
    
    // TODO: Replace with actual user registry when available
    std::vector<CPubKey> all_users;
    // std::vector<CPubKey> all_users = g_user_registry.GetVerifiedUsers();
    std::vector<CPubKey> regional_users;
    
    // TODO: Implement region filtering when user registry is available
    // for (const auto& user : all_users) {
    //     auto user_info = g_user_registry.GetUserInfo(user);
    //     if (user_info && user_info->status == UserStatus::VERIFIED) {
    //         for (const auto& region : it->second) {
    //             if (user_info->country_code == region) {
    //                 regional_users.push_back(user);
    //                 break;
    //             }
    //         }
    //     }
    // }
    
    return regional_users;
}

std::vector<CPubKey> MeasurementSystem::SelectWeightedRandom(const std::vector<CPubKey>& users, int count, bool allow_duplicates) const
{
    if (users.empty()) {
        return {};
    }
    
    std::vector<CPubKey> selected;
    std::vector<CPubKey> available_users = users;
    FastRandomContext rng;
    
    for (int i = 0; i < count; ++i) {
        if (available_users.empty()) {
            if (allow_duplicates) {
                // Reset available users if duplicates are allowed
                available_users = users;
            } else {
                break;  // No more unique users available
            }
        }
        
        int random_index = rng.randrange(available_users.size());
        selected.push_back(available_users[random_index]);
        
        if (!allow_duplicates) {
            // Remove selected user to avoid duplicates
            available_users.erase(available_users.begin() + random_index);
        }
    }
    
    return selected;
}

// ===== Conversion Rate Tracking =====

double MeasurementSystem::GetConversionRate(const std::string& currency_code, MeasurementType type) const
{
    std::string key = currency_code + ":" + std::to_string(static_cast<int>(type));
    auto it = m_conversion_rates.find(key);
    
    if (it == m_conversion_rates.end()) {
        // Default conversion rate for new currency/type combinations
        return 0.5;  // 50% default (conservative estimate)
    }
    
    return it->second.rate;
}

void MeasurementSystem::UpdateConversionRate(const std::string& currency_code, MeasurementType type, bool measurement_completed)
{
    std::string key = currency_code + ":" + std::to_string(static_cast<int>(type));
    auto& conversion_rate = m_conversion_rates[key];
    
    // Update counters
    conversion_rate.invites_sent++;
    if (measurement_completed) {
        conversion_rate.measurements_completed++;
    }
    
    // Recalculate conversion rate
    if (conversion_rate.invites_sent > 0) {
        conversion_rate.rate = static_cast<double>(conversion_rate.measurements_completed) / conversion_rate.invites_sent;
    }
    
    conversion_rate.last_updated = GetTime();
    
    LogPrintf("O Measurement: Updated conversion rate for %s type %d: %.2f%% (%d/%d)\n",
              currency_code.c_str(), static_cast<int>(type), 
              conversion_rate.rate * 100.0, 
              conversion_rate.measurements_completed, 
              conversion_rate.invites_sent);
}

int MeasurementSystem::CalculateInviteCountForTarget(int target_measurements, const std::string& currency_code, MeasurementType type) const
{
    double conversion_rate = GetConversionRate(currency_code, type);
    
    // Ensure minimum conversion rate to avoid infinite invitations
    if (conversion_rate < 0.1) {
        conversion_rate = 0.1;  // Minimum 10% conversion rate
    }
    
    // Calculate invitations needed: target / conversion_rate
    int invite_count = static_cast<int>(std::ceil(target_measurements / conversion_rate));
    
    // Add safety margin (20% extra invitations)
    invite_count = static_cast<int>(std::ceil(invite_count * 1.2));
    
    // Ensure reasonable bounds
    int min_invites = target_measurements;  // At least as many as target
    int max_invites = target_measurements * 10;  // At most 10x target (safety limit)
    
    invite_count = std::max(min_invites, std::min(invite_count, max_invites));
    
    LogPrintf("O Measurement: Target %d measurements, conversion rate %.2f%%, calculated %d invitations\n",
              target_measurements, conversion_rate * 100.0, invite_count);
    
    return invite_count;
}

// ===== O_ONLY Currency Measurement Processing =====

void MeasurementSystem::ProcessOOnlyCurrencyMeasurement(const std::string& currency,
                                                       double water_price_in_o_coin,
                                                       int height) {
    // Check if currency is O_ONLY
    if (!g_currency_lifecycle_manager.IsOOnlyCurrency(currency)) {
        LogPrintf("O Measurement: Currency %s is not O_ONLY, skipping O_ONLY measurement processing\n", 
                  currency.c_str());
        return;
    }
    
    LogPrintf("O Measurement: Processing O_ONLY currency measurement for %s - Water price: %.3f O\n",
              currency.c_str(), water_price_in_o_coin);
    
    // Create measurement record for O_ONLY currency
    WaterPriceMeasurement measurement;
    measurement.measurement_id = GetRandHash();
    measurement.currency_code = currency;
    measurement.price = static_cast<int64_t>(water_price_in_o_coin * 100); // Convert to cents (2 decimals)
    measurement.location = "O_ONLY_MEASUREMENT";
    measurement.timestamp = GetTime();
    measurement.block_height = height;
    measurement.is_validated = true; // O_ONLY measurements are auto-validated
    measurement.confidence_score = 1.0; // Full confidence for O_ONLY measurements
    
    // Add to measurement system
    m_water_prices[measurement.measurement_id] = measurement;
    
    // Update O_ONLY stability tracking
    g_currency_disappearance_handler.UpdateOOnlyStability(currency, 
                                                         water_price_in_o_coin, 
                                                         1.0); // Exchange rate fixed at 1:1
    
    LogPrintf("O Measurement: O_ONLY currency %s measurement processed - "
              "Water price: %.3f O, Exchange rate: 1.000 (fixed)\n",
              currency.c_str(), water_price_in_o_coin);
}

// ===== O Currency ↔ Fiat Currency Validation =====

bool MeasurementSystem::IsValidOCurrencyToFiatPair(const std::string& o_currency, const std::string& fiat_currency) const {
    if (!IsOCurrency(o_currency)) {
        return false; // First currency must be an O currency
    }
    
    std::string expected_fiat = GetCorrespondingFiatCurrency(o_currency);
    return expected_fiat == fiat_currency;
}

std::string MeasurementSystem::GetCorrespondingFiatCurrency(const std::string& o_currency) const {
    if (!IsOCurrency(o_currency)) {
        return ""; // Not an O currency
    }
    return RemoveOPrefix(o_currency);
}

std::string MeasurementSystem::GetCorrespondingOCurrency(const std::string& fiat_currency) const {
    return AddOPrefix(fiat_currency);
}

bool MeasurementSystem::IsOCurrency(const std::string& currency) const {
    return currency.length() > 1 && currency[0] == 'O' && std::isupper(currency[1]);
}

std::string MeasurementSystem::RemoveOPrefix(const std::string& o_currency) const {
    if (IsOCurrency(o_currency)) {
        return o_currency.substr(1); // Remove 'O' prefix
    }
    return o_currency;
}

std::string MeasurementSystem::AddOPrefix(const std::string& fiat_currency) const {
    return "O" + fiat_currency;
}

// ===== Water Price Stability Validation =====

bool MeasurementSystem::IsOCurrencyStable(const std::string& o_currency, double measured_exchange_rate) const {
    double deviation = CalculateStabilityDeviation(o_currency, measured_exchange_rate);
    const double STABILITY_THRESHOLD = 0.10; // 10% tolerance
    
    bool is_stable = deviation <= STABILITY_THRESHOLD;
    
    LogPrintf("O Stability: Currency %s, Measured rate: %.4f, Theoretical rate: %.4f, "
              "Deviation: %.2f%%, Stable: %s\n",
              o_currency.c_str(), measured_exchange_rate, GetTheoreticalExchangeRate(o_currency),
              deviation * 100.0, is_stable ? "YES" : "NO");
    
    return is_stable;
}

double MeasurementSystem::GetTheoreticalExchangeRate(const std::string& o_currency) const {
    // For O currencies, the theoretical exchange rate is based on water price in the fiat currency
    // If 1 liter of water costs $1.20 USD, then 1 OUSD should equal $1.20 USD
    // If 1 liter of water costs €0.95 EUR, then 1 OEUR should equal €0.95 EUR
    
    if (!IsOCurrency(o_currency)) {
        return 1.0; // Not an O currency
    }
    
    // Get the corresponding fiat currency
    std::string fiat_currency = GetCorrespondingFiatCurrency(o_currency);
    
    // Get the average water price in the fiat currency
    auto water_price_avg = GetAverageWaterPrice(fiat_currency, 7); // Last 7 days
    
    if (water_price_avg.has_value()) {
        double theoretical_rate = water_price_avg.value();
        
        LogPrintf("O Stability: Theoretical exchange rate for %s/%s = %.4f "
                  "(based on water price: %.4f %s per liter)\n",
                  o_currency.c_str(), fiat_currency.c_str(), theoretical_rate,
                  water_price_avg.value(), fiat_currency.c_str());
        
        return theoretical_rate;
    }
    
    // Fallback: if no water price data available, assume 1:1
    LogPrintf("O Stability: No water price data for %s, using fallback 1:1 rate\n", 
              fiat_currency.c_str());
    return 1.0;
}

double MeasurementSystem::CalculateStabilityDeviation(const std::string& o_currency, double measured_rate) const {
    double theoretical_rate = GetTheoreticalExchangeRate(o_currency);
    
    if (theoretical_rate == 0) {
        return 1.0; // 100% deviation if theoretical rate is 0
    }
    
    double deviation = std::abs(measured_rate - theoretical_rate) / theoretical_rate;
    return deviation;
}

} // namespace OMeasurement

