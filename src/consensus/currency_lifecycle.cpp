// Copyright (c) 2024 The Bitcoin Core developers
// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/currency_lifecycle.h>
#include <logging.h>
#include <util/time.h>
#include <util/strencodings.h>

#include <algorithm>
#include <cmath>

// Global instance
CurrencyLifecycleManager g_currency_lifecycle_manager;

// ===== CurrencyLifecycleManager Implementation =====

CurrencyLifecycleManager::CurrencyLifecycleManager() {
    m_stats.total_currencies = 0;
    m_stats.active_currencies = 0;
    m_stats.replaced_currencies = 0;
    m_stats.o_only_currencies = 0;
    m_stats.deprecated_currencies = 0;
    m_stats.total_replacements = 0;
}

CurrencyLifecycleManager::~CurrencyLifecycleManager() = default;

std::optional<CurrencyLifecycleInfo> CurrencyLifecycleManager::GetCurrencyInfo(const std::string& currency) const {
    auto it = m_currency_lifecycle.find(currency);
    if (it == m_currency_lifecycle.end()) {
        return std::nullopt;
    }
    return it->second;
}

void CurrencyLifecycleManager::UpdateCurrencyStatus(const std::string& currency, CurrencyStatus status, 
                                                   const std::string& reason, int height) {
    auto it = m_currency_lifecycle.find(currency);
    CurrencyStatus old_status = CurrencyStatus::ACTIVE;
    
    if (it != m_currency_lifecycle.end()) {
        old_status = it->second.status;
        it->second.status = status;
        it->second.status_change_height = height;
        it->second.status_reason = reason;
    } else {
        // Create new entry
        CurrencyLifecycleInfo info;
        info.currency_code = currency;
        info.status = status;
        info.status_change_height = height;
        info.status_reason = reason;
        info.last_data_height = height;
        info.data_dry_period = 0;
        m_currency_lifecycle[currency] = info;
    }
    
    LogStatusChange(currency, old_status, status, reason);
    UpdateStatistics();
}

bool CurrencyLifecycleManager::IsCurrencyActive(const std::string& currency) const {
    auto info = GetCurrencyInfo(currency);
    return info.has_value() && info->IsActive();
}

std::vector<std::string> CurrencyLifecycleManager::GetActiveCurrencies() const {
    std::vector<std::string> active;
    for (const auto& [currency, info] : m_currency_lifecycle) {
        if (info.IsActive()) {
            active.push_back(currency);
        }
    }
    return active;
}

std::vector<std::string> CurrencyLifecycleManager::GetOOnlyCurrencies() const {
    std::vector<std::string> o_only;
    for (const auto& [currency, info] : m_currency_lifecycle) {
        if (info.IsOOnly()) {
            o_only.push_back(currency);
        }
    }
    return o_only;
}

// ===== Currency Replacement Scenarios =====

void CurrencyLifecycleManager::ReplaceCurrencyWithExisting(const std::string& old_currency,
                                                          const std::string& new_currency,
                                                          double conversion_rate,
                                                          const std::string& reason,
                                                          int height) {
    // Validate replacement
    if (!ValidateReplacement(old_currency, new_currency)) {
        LogPrintf("O Currency Lifecycle: Invalid replacement %s -> %s\n", 
                  old_currency.c_str(), new_currency.c_str());
        return;
    }
    
    // Create replacement info
    CurrencyReplacement replacement;
    replacement.old_currency = old_currency;
    replacement.new_currency = new_currency;
    replacement.conversion_rate = conversion_rate;
    replacement.replacement_height = height;
    replacement.migration_deadline = height + (144 * 30); // 30 days for migration
    replacement.reason = reason;
    replacement.is_mandatory = true;
    
    // Store replacement info
    m_replacements[old_currency] = replacement;
    
    // Update currency status
    std::string status_reason = strprintf("Replaced by existing currency %s (rate: %.6f)", 
                                         new_currency.c_str(), conversion_rate);
    UpdateCurrencyStatus(old_currency, CurrencyStatus::REPLACED, status_reason, height);
    
    // Notify users
    NotifyUsersOfReplacement(old_currency, new_currency);
    
    LogPrintf("O Currency Lifecycle: Currency %s replaced by existing %s (rate: %.6f)\n",
              old_currency.c_str(), new_currency.c_str(), conversion_rate);
}

void CurrencyLifecycleManager::ReplaceCurrencyWithNew(const std::string& old_currency,
                                                     const std::string& new_currency,
                                                     double conversion_rate,
                                                     const std::string& reason,
                                                     int height) {
    // Validate replacement
    if (!ValidateReplacement(old_currency, new_currency)) {
        LogPrintf("O Currency Lifecycle: Invalid replacement %s -> %s\n", 
                  old_currency.c_str(), new_currency.c_str());
        return;
    }
    
    // Create replacement info
    CurrencyReplacement replacement;
    replacement.old_currency = old_currency;
    replacement.new_currency = new_currency;
    replacement.conversion_rate = conversion_rate;
    replacement.replacement_height = height;
    replacement.migration_deadline = height + (144 * 30); // 30 days for migration
    replacement.reason = reason;
    replacement.is_mandatory = true;
    
    // Store replacement info
    m_replacements[old_currency] = replacement;
    
    // Update currency status
    std::string status_reason = strprintf("Replaced by new currency %s (rate: %.6f) - SYSTEM UPDATE REQUIRED", 
                                         new_currency.c_str(), conversion_rate);
    UpdateCurrencyStatus(old_currency, CurrencyStatus::NEW_CURRENCY, status_reason, height);
    
    // Notify users
    NotifyUsersOfReplacement(old_currency, new_currency);
    
    LogPrintf("O Currency Lifecycle: Currency %s replaced by new %s (rate: %.6f) - SYSTEM UPDATE REQUIRED\n",
              old_currency.c_str(), new_currency.c_str(), conversion_rate);
}

void CurrencyLifecycleManager::ConvertToOOnly(const std::string& currency,
                                             const std::string& reason,
                                             int height) {
    // Update currency status
    std::string status_reason = strprintf("Converted to O_ONLY: %s", reason.c_str());
    UpdateCurrencyStatus(currency, CurrencyStatus::O_ONLY, status_reason, height);
    
    // Initialize O_ONLY stability status
    UpdateOOnlyStability(currency, true, true); // Assume stable initially
    
    LogPrintf("O Currency Lifecycle: Currency %s converted to O_ONLY: %s\n",
              currency.c_str(), reason.c_str());
}

std::optional<CurrencyReplacement> CurrencyLifecycleManager::GetReplacementInfo(const std::string& currency) const {
    auto it = m_replacements.find(currency);
    if (it == m_replacements.end()) {
        return std::nullopt;
    }
    return it->second;
}

// ===== O_ONLY Currency Handling =====

bool CurrencyLifecycleManager::IsOOnlyCurrency(const std::string& currency) const {
    auto info = GetCurrencyInfo(currency);
    return info.has_value() && info->IsOOnly();
}

bool CurrencyLifecycleManager::ValidateOOnlyStability(const std::string& currency, 
                                                     double water_price, 
                                                     double exchange_rate) const {
    if (!IsOOnlyCurrency(currency)) {
        return false;
    }
    
    // For O_ONLY currencies:
    // - Water price should equal 1.0 O
    // - Exchange rate with other O currencies should be 1:1
    const double WATER_PRICE_TOLERANCE = 0.01;  // 1% tolerance
    const double EXCHANGE_RATE_TOLERANCE = 0.01; // 1% tolerance
    
    bool water_price_stable = std::abs(water_price - 1.0) <= WATER_PRICE_TOLERANCE;
    bool exchange_rate_stable = std::abs(exchange_rate - 1.0) <= EXCHANGE_RATE_TOLERANCE;
    
    LogPrintf("O Currency Lifecycle: O_ONLY currency %s validation - Water price: %.3f (stable: %s), Exchange rate: %.3f (stable: %s)\n",
              currency.c_str(), water_price, water_price_stable ? "YES" : "NO",
              exchange_rate, exchange_rate_stable ? "YES" : "NO");
    
    return water_price_stable && exchange_rate_stable;
}

std::pair<bool, bool> CurrencyLifecycleManager::GetOOnlyStabilityStatus(const std::string& currency) const {
    auto info = GetCurrencyInfo(currency);
    if (!info.has_value() || !info->IsOOnly()) {
        return {false, false};
    }
    
    return {info->is_water_price_stable, info->is_exchange_rate_stable};
}

void CurrencyLifecycleManager::UpdateOOnlyStability(const std::string& currency,
                                                   bool water_price_stable,
                                                   bool exchange_rate_stable) {
    auto it = m_currency_lifecycle.find(currency);
    if (it != m_currency_lifecycle.end() && it->second.IsOOnly()) {
        it->second.is_water_price_stable = water_price_stable;
        it->second.is_exchange_rate_stable = exchange_rate_stable;
        
        LogPrintf("O Currency Lifecycle: O_ONLY currency %s stability updated - Water price: %s, Exchange rate: %s\n",
                  currency.c_str(), water_price_stable ? "STABLE" : "UNSTABLE",
                  exchange_rate_stable ? "STABLE" : "UNSTABLE");
    }
}

// ===== Data Availability Management =====

void CurrencyLifecycleManager::UpdateDataAvailability(const std::string& currency, int height) {
    auto it = m_currency_lifecycle.find(currency);
    if (it != m_currency_lifecycle.end()) {
        it->second.last_data_height = height;
        it->second.data_dry_period = 0;
    }
}

bool CurrencyLifecycleManager::HasRecentData(const std::string& currency, int height, int max_blocks_old) const {
    auto info = GetCurrencyInfo(currency);
    if (!info.has_value()) {
        return false;
    }
    
    return (height - info->last_data_height) <= max_blocks_old;
}

std::vector<std::string> CurrencyLifecycleManager::GetCurrenciesWithStaleData(int height, int max_blocks_old) const {
    std::vector<std::string> stale_currencies;
    
    for (const auto& [currency, info] : m_currency_lifecycle) {
        if (info.IsActive() && !HasRecentData(currency, height, max_blocks_old)) {
            stale_currencies.push_back(currency);
        }
    }
    
    return stale_currencies;
}

// ===== Migration Management =====

std::vector<std::string> CurrencyLifecycleManager::GetCurrenciesRequiringMigration(int height) const {
    std::vector<std::string> requiring_migration;
    
    for (const auto& [currency, replacement] : m_replacements) {
        if (height >= replacement.replacement_height && 
            height <= replacement.migration_deadline) {
            requiring_migration.push_back(currency);
        }
    }
    
    return requiring_migration;
}

bool CurrencyLifecycleManager::IsMigrationDeadlinePassed(const std::string& currency, int height) const {
    auto replacement = GetReplacementInfo(currency);
    if (!replacement.has_value()) {
        return false;
    }
    
    return height > replacement->migration_deadline;
}

std::optional<int64_t> CurrencyLifecycleManager::GetMigrationDeadline(const std::string& currency) const {
    auto replacement = GetReplacementInfo(currency);
    if (!replacement.has_value()) {
        return std::nullopt;
    }
    
    return replacement->migration_deadline;
}

// ===== Statistics and Reporting =====

std::map<std::string, int64_t> CurrencyLifecycleManager::GetLifecycleStatistics() const {
    std::map<std::string, int64_t> stats;
    stats["total_currencies"] = m_stats.total_currencies;
    stats["active_currencies"] = m_stats.active_currencies;
    stats["replaced_currencies"] = m_stats.replaced_currencies;
    stats["o_only_currencies"] = m_stats.o_only_currencies;
    stats["deprecated_currencies"] = m_stats.deprecated_currencies;
    stats["total_replacements"] = m_stats.total_replacements;
    return stats;
}

std::map<CurrencyStatus, int> CurrencyLifecycleManager::GetStatusDistribution() const {
    std::map<CurrencyStatus, int> distribution;
    
    for (const auto& [currency, info] : m_currency_lifecycle) {
        distribution[info.status]++;
    }
    
    return distribution;
}

std::map<std::string, int64_t> CurrencyLifecycleManager::GetReplacementStatistics() const {
    std::map<std::string, int64_t> stats;
    stats["total_replacements"] = static_cast<int64_t>(m_replacements.size());
    
    for (const auto& [currency, replacement] : m_replacements) {
        stats["replacement_" + replacement.new_currency]++;
    }
    
    return stats;
}

// ===== Utility Functions =====

std::string CurrencyLifecycleManager::GetStatusDescription(CurrencyStatus status) const {
    switch (status) {
        case CurrencyStatus::ACTIVE: return "Active - Normal operation";
        case CurrencyStatus::REPLACED: return "Replaced - User migration required";
        case CurrencyStatus::NEW_CURRENCY: return "New Currency - System update required";
        case CurrencyStatus::O_ONLY: return "O Only - Water price = 1 O, Exchange rate = 1:1";
        case CurrencyStatus::DEPRECATED: return "Deprecated - Currency discontinued";
        default: return "Unknown Status";
    }
}

std::string CurrencyLifecycleManager::GetStatusReason(const std::string& currency) const {
    auto info = GetCurrencyInfo(currency);
    if (!info.has_value()) {
        return "Currency not found";
    }
    return info->status_reason;
}

bool CurrencyLifecycleManager::NeedsSystemUpdate(const std::string& currency) const {
    auto info = GetCurrencyInfo(currency);
    return info.has_value() && info->IsNewCurrency();
}

std::vector<std::string> CurrencyLifecycleManager::GetCurrenciesNeedingUpdates() const {
    std::vector<std::string> needing_updates;
    
    for (const auto& [currency, info] : m_currency_lifecycle) {
        if (info.IsNewCurrency()) {
            needing_updates.push_back(currency);
        }
    }
    
    return needing_updates;
}

// ===== Data Management =====

void CurrencyLifecycleManager::PruneOldData(int cutoff_height) {
    int pruned = 0;
    
    for (auto it = m_currency_lifecycle.begin(); it != m_currency_lifecycle.end();) {
        if (it->second.status_change_height < cutoff_height && 
            it->second.IsDeprecated()) {
            it = m_currency_lifecycle.erase(it);
            pruned++;
        } else {
            ++it;
        }
    }
    
    if (pruned > 0) {
        LogPrintf("O Currency Lifecycle: Pruned %d old currency entries\n", pruned);
        UpdateStatistics();
    }
}

void CurrencyLifecycleManager::ClearAllData() {
    m_currency_lifecycle.clear();
    m_replacements.clear();
    m_stats = LifecycleStats{};
}

// ===== Private Helper Functions =====

void CurrencyLifecycleManager::UpdateStatistics() {
    m_stats.total_currencies = static_cast<int64_t>(m_currency_lifecycle.size());
    m_stats.active_currencies = 0;
    m_stats.replaced_currencies = 0;
    m_stats.o_only_currencies = 0;
    m_stats.deprecated_currencies = 0;
    m_stats.total_replacements = static_cast<int64_t>(m_replacements.size());
    
    for (const auto& [currency, info] : m_currency_lifecycle) {
        switch (info.status) {
            case CurrencyStatus::ACTIVE:
                m_stats.active_currencies++;
                break;
            case CurrencyStatus::REPLACED:
            case CurrencyStatus::NEW_CURRENCY:
                m_stats.replaced_currencies++;
                break;
            case CurrencyStatus::O_ONLY:
                m_stats.o_only_currencies++;
                break;
            case CurrencyStatus::DEPRECATED:
                m_stats.deprecated_currencies++;
                break;
        }
    }
}

void CurrencyLifecycleManager::LogStatusChange(const std::string& currency, CurrencyStatus old_status, 
                                              CurrencyStatus new_status, const std::string& reason) {
    LogPrintf("O Currency Lifecycle: Currency %s status changed from %s to %s - %s\n",
              currency.c_str(), GetStatusDescription(old_status).c_str(),
              GetStatusDescription(new_status).c_str(), reason.c_str());
}

bool CurrencyLifecycleManager::ValidateReplacement(const std::string& old_currency, const std::string& new_currency) const {
    if (old_currency.empty() || new_currency.empty()) {
        return false;
    }
    
    if (old_currency == new_currency) {
        return false;
    }
    
    // Check if old currency exists and is active
    auto old_info = GetCurrencyInfo(old_currency);
    if (!old_info.has_value() || !old_info->IsActive()) {
        return false;
    }
    
    // Check if new currency exists (for existing currency replacement)
    auto new_info = GetCurrencyInfo(new_currency);
    if (new_info.has_value() && new_info->IsActive()) {
        // Replacing with existing active currency - this is valid
        return true;
    }
    
    // Replacing with new currency - this is also valid
    return true;
}

void CurrencyLifecycleManager::NotifyUsersOfReplacement(const std::string& old_currency, const std::string& new_currency) {
    // TODO: Implement user notification system
    // This would notify users who have balances in the old currency
    // about the replacement and migration process
    
    LogPrintf("O Currency Lifecycle: Notifying users of currency replacement %s -> %s\n",
              old_currency.c_str(), new_currency.c_str());
}

std::vector<CurrencyLifecycleInfo> CurrencyLifecycleManager::GetAllCurrencyInfo() const {
    std::vector<CurrencyLifecycleInfo> result;
    result.reserve(m_currency_lifecycle.size());
    
    for (const auto& pair : m_currency_lifecycle) {
        result.push_back(pair.second);
    }
    
    return result;
}
