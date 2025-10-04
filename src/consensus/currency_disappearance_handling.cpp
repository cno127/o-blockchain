// Copyright (c) 2024 The Bitcoin Core developers
// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/currency_disappearance_handling.h>
#include <consensus/currency_lifecycle.h>
#include <logging.h>
#include <util/time.h>
#include <util/strencodings.h>

#include <algorithm>
#include <cmath>

// Global instance
CurrencyDisappearanceHandler g_currency_disappearance_handler;

CurrencyDisappearanceHandler::CurrencyDisappearanceHandler() {
    m_stats.total_fallback_sources = 0;
    m_stats.active_fallback_sources = 0;
    m_stats.o_only_currencies = 0;
    m_stats.emergency_stabilizations = 0;
    m_stats.total_fallback_usage = 0;
    
    InitializeRegionalMappings();
}

CurrencyDisappearanceHandler::~CurrencyDisappearanceHandler() = default;

// ===== Fallback Data Management =====

void CurrencyDisappearanceHandler::AddFallbackSource(const std::string& currency, const FallbackDataSource& source) {
    if (!ValidateFallbackData(source)) {
        LogPrintf("O Currency Disappearance: Invalid fallback data for %s\n", currency.c_str());
        return;
    }
    
    m_fallback_sources[currency].push_back(source);
    UpdateStatistics();
    
    LogPrintf("O Currency Disappearance: Added fallback source for %s: %s (rate: %.6f, confidence: %.2f)\n",
              currency.c_str(), source.source_name.c_str(), source.rate, source.confidence);
}

std::optional<FallbackDataSource> CurrencyDisappearanceHandler::GetFallbackData(const std::string& currency, 
                                                                               FallbackSourceType preferred_type) const {
    auto it = m_fallback_sources.find(currency);
    if (it == m_fallback_sources.end()) {
        return std::nullopt;
    }
    
    // Try to find preferred type first
    for (const auto& source : it->second) {
        if (source.type == preferred_type && source.is_active) {
            return source;
        }
    }
    
    // Fall back to any active source
    for (const auto& source : it->second) {
        if (source.is_active) {
            return source;
        }
    }
    
    return std::nullopt;
}

std::optional<FallbackDataSource> CurrencyDisappearanceHandler::GetBestAvailableData(const std::string& currency) const {
    auto it = m_fallback_sources.find(currency);
    if (it == m_fallback_sources.end()) {
        return std::nullopt;
    }
    
    FallbackDataSource best_source;
    double best_score = 0.0;
    
    for (const auto& source : it->second) {
        if (!source.is_active) continue;
        
        // Calculate score based on type priority and confidence
        double type_priority = 1.0;
        switch (source.type) {
            case FallbackSourceType::PRIMARY: type_priority = 1.0; break;
            case FallbackSourceType::SECONDARY: type_priority = 0.8; break;
            case FallbackSourceType::TERTIARY: type_priority = 0.6; break;
            case FallbackSourceType::EMERGENCY: type_priority = 0.4; break;
        }
        
        double time_decay = CalculateTimeDecay(source.timestamp, GetTime());
        double score = type_priority * source.confidence * time_decay;
        
        if (score > best_score) {
            best_score = score;
            best_source = source;
        }
    }
    
    return best_score > 0.0 ? std::optional<FallbackDataSource>(best_source) : std::nullopt;
}

// ===== Currency Disappearance Scenarios =====

bool CurrencyDisappearanceHandler::HandleCurrencyReplacement(const std::string& old_currency, 
                                                           const std::string& new_currency,
                                                           double conversion_rate,
                                                           const std::string& reason,
                                                           int height) {
    LogPrintf("O Currency Disappearance: Handling currency replacement %s -> %s (rate: %.6f)\n",
              old_currency.c_str(), new_currency.c_str(), conversion_rate);
    
    // Use global currency lifecycle manager
    g_currency_lifecycle_manager.ReplaceCurrencyWithExisting(old_currency, new_currency, 
                                                           conversion_rate, reason, height);
    
    // Update fallback data with conversion rate
    FallbackDataSource fallback_data;
    fallback_data.type = FallbackSourceType::SECONDARY;
    fallback_data.source_name = "currency_replacement";
    fallback_data.currency = old_currency;
    fallback_data.rate = conversion_rate;
    fallback_data.timestamp = GetTime();
    fallback_data.confidence = 0.9; // High confidence for official replacement
    fallback_data.is_active = true;
    
    AddFallbackSource(old_currency, fallback_data);
    
    m_disappearance_count[old_currency]++;
    LogCurrencyDisappearance(old_currency, reason);
    
    return true;
}

bool CurrencyDisappearanceHandler::HandleNewCurrencyReplacement(const std::string& old_currency,
                                                              const std::string& new_currency,
                                                              double conversion_rate,
                                                              const std::string& reason,
                                                              int height) {
    LogPrintf("O Currency Disappearance: Handling new currency replacement %s -> %s (rate: %.6f)\n",
              old_currency.c_str(), new_currency.c_str(), conversion_rate);
    
    // Use global currency lifecycle manager
    g_currency_lifecycle_manager.ReplaceCurrencyWithNew(old_currency, new_currency, 
                                                       conversion_rate, reason, height);
    
    // Update fallback data with conversion rate
    FallbackDataSource fallback_data;
    fallback_data.type = FallbackSourceType::SECONDARY;
    fallback_data.source_name = "new_currency_replacement";
    fallback_data.currency = old_currency;
    fallback_data.rate = conversion_rate;
    fallback_data.timestamp = GetTime();
    fallback_data.confidence = 0.8; // Good confidence for new currency
    fallback_data.is_active = true;
    
    AddFallbackSource(old_currency, fallback_data);
    
    m_disappearance_count[old_currency]++;
    LogCurrencyDisappearance(old_currency, reason);
    
    return true;
}

bool CurrencyDisappearanceHandler::HandleOOnlyConversion(const std::string& currency,
                                                        const std::string& reason,
                                                        int height) {
    LogPrintf("O Currency Disappearance: Handling O_ONLY conversion for %s\n", currency.c_str());
    
    // Use global currency lifecycle manager
    g_currency_lifecycle_manager.ConvertToOOnly(currency, reason, height);
    
    // Initialize O_ONLY stability tracking with measured values (not fixed)
    // Water price will be measured and validated against 1.00 O target
    // Exchange rate is fixed at 1:1 for O_ONLY currencies
    m_o_only_stability[currency] = {1.0, 1.0}; // Initial values, will be updated with measurements
    
    // Add emergency fallback data
    FallbackDataSource fallback_data;
    fallback_data.type = FallbackSourceType::EMERGENCY;
    fallback_data.source_name = "o_only_conversion";
    fallback_data.currency = currency;
    fallback_data.rate = 1.0; // 1:1 with O
    fallback_data.timestamp = GetTime();
    fallback_data.confidence = 1.0; // Perfect confidence for O_ONLY
    fallback_data.is_active = true;
    
    AddFallbackSource(currency, fallback_data);
    
    m_disappearance_count[currency]++;
    LogCurrencyDisappearance(currency, reason);
    
    return true;
}

// ===== O_ONLY Currency Validation =====

bool CurrencyDisappearanceHandler::ValidateOOnlyCurrency(const std::string& currency, 
                                                        double measured_water_price_in_o, 
                                                        double exchange_rate) const {
    if (!g_currency_lifecycle_manager.IsOOnlyCurrency(currency)) {
        return false;
    }
    
    // For O_ONLY currencies:
    // - Water price should equal 1.0 O (measured, not fixed)
    // - Exchange rate with other O currencies should be 1:1 (fixed)
    const double WATER_PRICE_TOLERANCE = 0.10;  // 10% tolerance (same as regular currencies)
    const double EXCHANGE_RATE_TOLERANCE = 0.01; // 1% tolerance (fixed at 1:1)
    
    // Check if measured water price in O coin deviates from target 1.00 O
    bool water_price_stable = std::abs(measured_water_price_in_o - 1.0) <= WATER_PRICE_TOLERANCE;
    
    // Exchange rate should always be 1:1 for O_ONLY currencies
    bool exchange_rate_stable = std::abs(exchange_rate - 1.0) <= EXCHANGE_RATE_TOLERANCE;
    
    LogPrintf("O Currency Disappearance: O_ONLY currency %s validation - "
              "Measured water price: %.3f O (target: 1.000 O, stable: %s), "
              "Exchange rate: %.3f (stable: %s)\n",
              currency.c_str(), measured_water_price_in_o, 
              water_price_stable ? "YES" : "NO",
              exchange_rate, exchange_rate_stable ? "YES" : "NO");
    
    return water_price_stable && exchange_rate_stable;
}

bool CurrencyDisappearanceHandler::IsOOnlyCurrencyStable(const std::string& currency) const {
    auto it = m_o_only_stability.find(currency);
    if (it == m_o_only_stability.end()) {
        return false;
    }
    
    return ValidateOOnlyCurrency(currency, it->second.first, it->second.second);
}

std::pair<double, double> CurrencyDisappearanceHandler::GetOOnlyStabilityMetrics(const std::string& currency) const {
    auto it = m_o_only_stability.find(currency);
    if (it == m_o_only_stability.end()) {
        return {0.0, 0.0};
    }
    
    return it->second;
}

void CurrencyDisappearanceHandler::UpdateOOnlyStability(const std::string& currency,
                                                       double measured_water_price_in_o,
                                                       double exchange_rate) {
    // Store the MEASURED water price (not fixed 1.0)
    m_o_only_stability[currency] = {measured_water_price_in_o, exchange_rate};
    
    // Validate against target 1.00 O
    bool water_price_stable = std::abs(measured_water_price_in_o - 1.0) <= 0.10; // 10% tolerance
    bool exchange_rate_stable = std::abs(exchange_rate - 1.0) <= 0.01; // 1% tolerance
    
    g_currency_lifecycle_manager.UpdateOOnlyStability(currency, water_price_stable, exchange_rate_stable);
    
    // If water price is unstable, trigger stabilization
    if (!water_price_stable) {
        double deviation = std::abs(measured_water_price_in_o - 1.0);
        LogPrintf("O Currency Disappearance: O_ONLY currency %s is UNSTABLE - "
                  "Measured water price: %.3f O (target: 1.000 O, deviation: %.2f%%)\n",
                  currency.c_str(), measured_water_price_in_o, deviation * 100);
        
        // Trigger stabilization mining for O_ONLY currency
        // Note: This would need to be implemented in the stabilization system
        LogPrintf("O Currency Disappearance: Triggering stabilization for O_ONLY currency %s\n", 
                  currency.c_str());
    }
    
    LogPrintf("O Currency Disappearance: Updated O_ONLY stability for %s - "
              "Measured water price: %.3f O, Exchange rate: %.3f\n",
              currency.c_str(), measured_water_price_in_o, exchange_rate);
}

// ===== Fallback Rate Calculation =====

std::optional<double> CurrencyDisappearanceHandler::CalculateFallbackExchangeRate(const std::string& from_currency,
                                                                                 const std::string& to_currency) const {
    // Check if both currencies are O_ONLY
    if (g_currency_lifecycle_manager.IsOOnlyCurrency(from_currency) && 
        g_currency_lifecycle_manager.IsOOnlyCurrency(to_currency)) {
        return 1.0; // 1:1 exchange rate for O_ONLY currencies
    }
    
    // Try to get fallback data for from_currency
    auto fallback_data = GetBestAvailableData(from_currency);
    if (fallback_data.has_value()) {
        return fallback_data->rate;
    }
    
    // Try regional proxy
    auto regional_proxy = GetRegionalProxy(from_currency);
    if (regional_proxy.has_value()) {
        auto proxy_data = GetBestAvailableData(regional_proxy.value());
        if (proxy_data.has_value()) {
            return proxy_data->rate;
        }
    }
    
    // Use global water price average as last resort
    return CalculateGlobalWaterPriceAverage();
}

std::optional<double> CurrencyDisappearanceHandler::CalculateFallbackWaterPrice(const std::string& currency) const {
    // For O_ONLY currencies, water price is always 1.0 O
    if (g_currency_lifecycle_manager.IsOOnlyCurrency(currency)) {
        return 1.0;
    }
    
    // Try to get fallback data
    auto fallback_data = GetBestAvailableData(currency);
    if (fallback_data.has_value()) {
        return fallback_data->rate;
    }
    
    // Use global average as fallback
    return CalculateGlobalWaterPriceAverage();
}

double CurrencyDisappearanceHandler::CalculateGlobalWaterPriceAverage() const {
    // This would typically calculate the global average water price
    // For now, return 1.0 as a reasonable default
    return 1.0;
}

// ===== Emergency Mechanisms =====

bool CurrencyDisappearanceHandler::TriggerEmergencyStabilization(const std::string& currency, int height) const {
    if (!IsEmergencyStabilizationNeeded(currency)) {
        return false;
    }
    
    LogPrintf("O Currency Disappearance: Triggering emergency stabilization for %s at height %d\n",
              currency.c_str(), height);
    
    // This would trigger the stabilization mining system
    // to create coins for stable currency users
    
    return true;
}

bool CurrencyDisappearanceHandler::IsEmergencyStabilizationNeeded(const std::string& currency) const {
    if (!g_currency_lifecycle_manager.IsOOnlyCurrency(currency)) {
        return false;
    }
    
    auto it = m_o_only_stability.find(currency);
    if (it == m_o_only_stability.end()) {
        return false;
    }
    
    double measured_water_price = it->second.first;
    double deviation = std::abs(measured_water_price - 1.0);
    
    // Trigger stabilization if water price deviates more than 10% from 1.00 O
    return deviation > 0.10;
}

CAmount CurrencyDisappearanceHandler::GetEmergencyStabilizationAmount(const std::string& currency) const {
    if (!IsEmergencyStabilizationNeeded(currency)) {
        return 0;
    }
    
    // Calculate stabilization amount based on deviation from 1.00 O
    auto stability_metrics = GetOOnlyStabilityMetrics(currency);
    double measured_water_price = stability_metrics.first;
    double deviation = std::abs(measured_water_price - 1.0);
    
    // More deviation = more stabilization needed
    CAmount base_stabilization = OAmount::O(1000); // 1000.00 O base
    CAmount deviation_multiplier = static_cast<CAmount>(deviation * 10000); // Scale deviation
    
    CAmount emergency_amount = base_stabilization + deviation_multiplier;
    
    // Cap at 100,000 O
    CAmount max_amount = OAmount::O(100000); // 100,000.00 O
    return std::min(emergency_amount, max_amount);
}

// ===== Private Helper Functions =====

void CurrencyDisappearanceHandler::UpdateStatistics() {
    m_stats.total_fallback_sources = 0;
    m_stats.active_fallback_sources = 0;
    m_stats.o_only_currencies = static_cast<int64_t>(m_o_only_stability.size());
    
    for (const auto& [currency, sources] : m_fallback_sources) {
        m_stats.total_fallback_sources += static_cast<int64_t>(sources.size());
        for (const auto& source : sources) {
            if (source.is_active) {
                m_stats.active_fallback_sources++;
            }
        }
    }
}

void CurrencyDisappearanceHandler::LogFallbackUsage(const std::string& currency, FallbackSourceType type) {
    m_fallback_usage_count[currency]++;
    m_stats.total_fallback_usage++;
    
    LogPrintf("O Currency Disappearance: Using fallback data for %s (type: %s, usage count: %d)\n",
              currency.c_str(), GetFallbackSourceDescription(type).c_str(), 
              static_cast<int>(m_fallback_usage_count[currency]));
}

void CurrencyDisappearanceHandler::LogCurrencyDisappearance(const std::string& currency, const std::string& reason) {
    LogPrintf("O Currency Disappearance: Currency %s disappeared - %s\n", 
              currency.c_str(), reason.c_str());
}

bool CurrencyDisappearanceHandler::ValidateFallbackData(const FallbackDataSource& source) const {
    if (source.currency.empty() || source.source_name.empty()) {
        return false;
    }
    
    if (source.rate <= 0.0 || source.rate > 1000000.0) {
        return false;
    }
    
    if (source.confidence < 0.0 || source.confidence > 1.0) {
        return false;
    }
    
    return true;
}

double CurrencyDisappearanceHandler::CalculateTimeDecay(int64_t timestamp, int64_t current_time) const {
    int64_t age_seconds = current_time - timestamp;
    int64_t max_age_seconds = 86400 * 7; // 7 days
    
    if (age_seconds >= max_age_seconds) {
        return 0.0;
    }
    
    return 1.0 - (static_cast<double>(age_seconds) / max_age_seconds);
}

void CurrencyDisappearanceHandler::InitializeRegionalMappings() {
    // Initialize regional currency mappings
    // This would typically be loaded from a configuration file
    
    // North America
    m_currency_regions["OUSD"] = "North America";
    m_currency_regions["OCAD"] = "North America";
    m_currency_regions["OMXN"] = "North America";
    
    // Europe
    m_currency_regions["OEUR"] = "Europe";
    m_currency_regions["OGBP"] = "Europe";
    m_currency_regions["OCHF"] = "Europe";
    
    // Asia
    m_currency_regions["OJPY"] = "Asia";
    m_currency_regions["OCNY"] = "Asia";
    m_currency_regions["OKRW"] = "Asia";
    
    // Create regional groups
    m_regional_groups["North America"] = {"OUSD", "OCAD", "OMXN"};
    m_regional_groups["Europe"] = {"OEUR", "OGBP", "OCHF"};
    m_regional_groups["Asia"] = {"OJPY", "OCNY", "OKRW"};
}

std::optional<std::string> CurrencyDisappearanceHandler::GetRegionalProxy(const std::string& currency) const {
    auto region_it = m_currency_regions.find(currency);
    if (region_it == m_currency_regions.end()) {
        return std::nullopt;
    }
    
    auto group_it = m_regional_groups.find(region_it->second);
    if (group_it == m_regional_groups.end()) {
        return std::nullopt;
    }
    
    // Find first active currency in the same region
    for (const auto& regional_currency : group_it->second) {
        if (regional_currency != currency && g_currency_lifecycle_manager.IsCurrencyActive(regional_currency)) {
            return regional_currency;
        }
    }
    
    return std::nullopt;
}

std::string CurrencyDisappearanceHandler::GetFallbackSourceDescription(FallbackSourceType type) const {
    switch (type) {
        case FallbackSourceType::PRIMARY: return "Primary (Real-time APIs)";
        case FallbackSourceType::SECONDARY: return "Secondary (Historical Data)";
        case FallbackSourceType::TERTIARY: return "Tertiary (Regional Proxies)";
        case FallbackSourceType::EMERGENCY: return "Emergency (Global Averages)";
        default: return "Unknown";
    }
}
