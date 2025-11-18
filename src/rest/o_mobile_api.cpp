// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rest/o_mobile_api.h>
#include <rest.h>
#include <rpc/server.h>
#include <rpc/server_util.h>
#include <rpc/request.h>
#include <rpc/util.h>
#include <rpc/o_user_rpc.h>
#include <rpc/o_measurement_rpc.h>
#include <rpc/o_blockchain_tx_rpc.h>
#include <rpc/o_currency_rpc.h>
#include <validation/o_integration.h>
#include <consensus/user_consensus.h>
#include <consensus/geographic_access_control.h>
#include <measurement/measurement_system.h>
#include <measurement/o_measurement_db.h>
#include <node/context.h>
#include <util/strencodings.h>
#include <util/time.h>
#include <pubkey.h>
#include <key_io.h>
#include <logging.h>
#include <hash.h>
#include <random.h>

#include <univalue.h>
#include <string>
#include <sstream>
#include <algorithm>

using node::NodeContext;

// Helper function to write JSON response
static bool WriteJSONResponse(HTTPRequest* req, const UniValue& json, enum HTTPStatusCode status = HTTP_OK)
{
    req->WriteHeader("Content-Type", "application/json");
    req->WriteHeader("Access-Control-Allow-Origin", "*"); // CORS for mobile apps
    req->WriteReply(status, json.write() + "\n");
    return true;
}

// Helper function to write error response
static bool WriteErrorResponse(HTTPRequest* req, const std::string& error_code, const std::string& message, enum HTTPStatusCode status = HTTP_BAD_REQUEST)
{
    UniValue error(UniValue::VOBJ);
    error.pushKV("success", false);
    error.pushKV("error", error_code);
    error.pushKV("message", message);
    return WriteJSONResponse(req, error, status);
}

// Helper to parse JSON request body
static bool ParseJSONRequest(HTTPRequest* req, UniValue& json)
{
    std::string body = req->ReadBody();
    if (body.empty()) {
        return false;
    }
    
    try {
        json.read(body);
        return true;
    } catch (const std::exception& e) {
        LogPrintf("REST API: Failed to parse JSON: %s\n", e.what());
        return false;
    }
}

// Extract public key from URL path like /rest/api/v1/users/{publickey}/status
static std::string ExtractPublicKeyFromPath(const std::string& path)
{
    // Path format: /rest/api/v1/users/{publickey}/status
    size_t users_pos = path.find("/users/");
    if (users_pos == std::string::npos) return "";
    
    size_t key_start = users_pos + 7; // Length of "/users/"
    size_t key_end = path.find("/", key_start);
    if (key_end == std::string::npos) {
        key_end = path.length();
    }
    
    return path.substr(key_start, key_end - key_start);
}

// Extract country code from URL path
static std::string ExtractCountryCodeFromPath(const std::string& path)
{
    size_t country_pos = path.find("/country/");
    if (country_pos == std::string::npos) return "";
    
    size_t code_start = country_pos + 9; // Length of "/country/"
    size_t code_end = path.find("/", code_start);
    if (code_end == std::string::npos) {
        code_end = path.length();
    }
    
    return path.substr(code_start, code_end - code_start);
}

// Extract O currency from URL path
static std::string ExtractOCurrencyFromPath(const std::string& path)
{
    size_t rates_pos = path.find("/exchange-rates/");
    if (rates_pos == std::string::npos) return "";
    
    size_t currency_start = rates_pos + 16; // Length of "/exchange-rates/"
    size_t currency_end = path.find("/", currency_start);
    if (currency_end == std::string::npos) {
        currency_end = path.length();
    }
    
    return path.substr(currency_start, currency_end - currency_start);
}

// ===== User Registration Endpoints =====

bool rest_user_register(const std::any& context, HTTPRequest* req, const std::string& strReq)
{
    if (req->GetRequestMethod() != "POST") {
        return WriteErrorResponse(req, "METHOD_NOT_ALLOWED", "Only POST method is allowed", HTTP_BAD_METHOD);
    }
    
    UniValue json;
    if (!ParseJSONRequest(req, json)) {
        return WriteErrorResponse(req, "INVALID_JSON", "Invalid JSON in request body");
    }
    
    // Extract parameters
    if (!json.exists("publickey") || !json.exists("government_id") || 
        !json.exists("birth_currency") || !json.exists("country_code") || 
        !json.exists("identity_proof")) {
        return WriteErrorResponse(req, "MISSING_PARAMETERS", "Missing required parameters");
    }
    
    std::string publickey_str = json["publickey"].get_str();
    std::string government_id = json["government_id"].get_str();
    std::string birth_currency = json["birth_currency"].get_str();
    std::string country_code = json["country_code"].get_str();
    std::string identity_proof = json["identity_proof"].get_str();
    
    // Get country legal restrictions
    auto policy = g_geographic_access_control.GetJurisdictionPolicy(country_code);
    if (!policy.has_value()) {
        return WriteErrorResponse(req, "INVALID_COUNTRY", "Invalid or unsupported country code");
    }
    
    // Create RPC request to register user
    JSONRPCRequest rpc_request;
    rpc_request.params.push_back(publickey_str);
    rpc_request.params.push_back(government_id);
    rpc_request.params.push_back(birth_currency);
    rpc_request.params.push_back(country_code);
    rpc_request.params.push_back(identity_proof);
    
    try {
        // Call the registeruser RPC (would need to expose it properly)
        // For now, we'll create the user directly
        CPubKey publickey;
        if (!publickey.SetHex(publickey_str)) {
            return WriteErrorResponse(req, "INVALID_PUBLICKEY", "Invalid public key format");
        }
        
        OfficialUser new_user;
        new_user.public_key = publickey;
        new_user.government_id_hash = Hash(government_id).GetHex();
        new_user.birth_currency = birth_currency;
        new_user.country_code = country_code;
        new_user.identity_proof_hash = identity_proof;
        new_user.status = UserStatus::PENDING_VERIFICATION;
        
        std::string error_message;
        if (!g_user_consensus.RegisterUser(new_user, error_message)) {
            return WriteErrorResponse(req, "REGISTRATION_FAILED", error_message);
        }
        
        // Get allowed verification methods
        std::vector<VerificationMethod> allowed_methods = 
            g_user_consensus.GetAllowedVerificationMethods(country_code);
        
        std::vector<std::string> method_strings;
        for (const auto& method : allowed_methods) {
            switch (method) {
                case VerificationMethod::GOVERNMENT_ID:
                    method_strings.push_back("government_id");
                    break;
                case VerificationMethod::VIDEO_CALL:
                    method_strings.push_back("video_call");
                    break;
                case VerificationMethod::DOCUMENT_REVIEW:
                    method_strings.push_back("document_review");
                    break;
                case VerificationMethod::BIOMETRIC_VERIFICATION:
                    method_strings.push_back("biometric_verification");
                    break;
                case VerificationMethod::THIRD_PARTY_KYC:
                    method_strings.push_back("third_party_kyc");
                    break;
            }
        }
        
        UniValue response(UniValue::VOBJ);
        response.pushKV("success", true);
        response.pushKV("user_id", publickey_str);
        response.pushKV("status", "pending_verification");
        response.pushKV("message", "User registration submitted successfully. Awaiting endorsements.");
        response.pushKV("registration_height", 0);
        response.pushKV("kyc_required", policy->compliance_level == ComplianceLevel::KYC_REQUIRED);
        
        UniValue methods(UniValue::VARR);
        for (const auto& m : method_strings) {
            methods.push_back(m);
        }
        response.pushKV("allowed_verification_methods", methods);
        
        return WriteJSONResponse(req, response);
        
    } catch (const std::exception& e) {
        return WriteErrorResponse(req, "INTERNAL_ERROR", std::string("Registration failed: ") + e.what(), HTTP_INTERNAL_SERVER_ERROR);
    }
}

bool rest_user_status(const std::any& context, HTTPRequest* req, const std::string& strReq)
{
    if (req->GetRequestMethod() != "GET") {
        return WriteErrorResponse(req, "METHOD_NOT_ALLOWED", "Only GET method is allowed", HTTP_BAD_METHOD);
    }
    
    std::string publickey_str = ExtractPublicKeyFromPath(strReq);
    if (publickey_str.empty()) {
        return WriteErrorResponse(req, "INVALID_PARAMETERS", "Public key not found in URL path");
    }
    
    CPubKey publickey;
    if (!publickey.SetHex(publickey_str)) {
        return WriteErrorResponse(req, "INVALID_PUBLICKEY", "Invalid public key format");
    }
    
    // Get user status from consensus
    UserStatus status = g_user_consensus.IsUserVerified(publickey) ? 
        UserStatus::VERIFIED : UserStatus::PENDING_VERIFICATION;
    double reputation = g_user_consensus.GetReputationScore(publickey);
    
    UniValue response(UniValue::VOBJ);
    response.pushKV("user_id", publickey_str);
    
    std::string status_str;
    switch (status) {
        case UserStatus::VERIFIED:
            status_str = "verified";
            break;
        case UserStatus::PENDING_VERIFICATION:
            status_str = "pending_verification";
            break;
        case UserStatus::VERIFICATION_IN_PROGRESS:
            status_str = "verification_in_progress";
            break;
        case UserStatus::SUSPENDED:
            status_str = "suspended";
            break;
        case UserStatus::BLACKLISTED:
            status_str = "blacklisted";
            break;
        default:
            status_str = "unknown";
    }
    
    response.pushKV("status", status_str);
    response.pushKV("reputation_score", reputation);
    response.pushKV("endorsement_count", 0); // TODO: Get actual count
    response.pushKV("registration_height", 0); // TODO: Get actual height
    response.pushKV("verification_height", 0); // TODO: Get actual height
    
    return WriteJSONResponse(req, response);
}

bool rest_user_legal_restrictions(const std::any& context, HTTPRequest* req, const std::string& strReq)
{
    if (req->GetRequestMethod() != "GET") {
        return WriteErrorResponse(req, "METHOD_NOT_ALLOWED", "Only GET method is allowed", HTTP_BAD_METHOD);
    }
    
    std::string publickey_str = ExtractPublicKeyFromPath(strReq);
    if (publickey_str.empty()) {
        return WriteErrorResponse(req, "INVALID_PARAMETERS", "Public key not found in URL path");
    }
    
    CPubKey publickey;
    if (!publickey.SetHex(publickey_str)) {
        return WriteErrorResponse(req, "INVALID_PUBLICKEY", "Invalid public key format");
    }
    
    // Get user's country (would need to query from user registry)
    // For now, we'll need to get it from the request or user data
    // This is a placeholder - in real implementation, query user's country
    
    // Get country policy
    auto policy = g_geographic_access_control.GetJurisdictionPolicy("US"); // Placeholder
    
    if (!policy.has_value()) {
        return WriteErrorResponse(req, "POLICY_NOT_FOUND", "Country policy not found");
    }
    
    std::vector<VerificationMethod> allowed_methods = 
        g_user_consensus.GetAllowedVerificationMethods(policy->country_code);
    
    std::vector<std::string> method_strings;
    for (const auto& method : allowed_methods) {
        switch (method) {
            case VerificationMethod::GOVERNMENT_ID:
                method_strings.push_back("government_id");
                break;
            case VerificationMethod::VIDEO_CALL:
                method_strings.push_back("video_call");
                break;
            case VerificationMethod::DOCUMENT_REVIEW:
                method_strings.push_back("document_review");
                break;
            case VerificationMethod::BIOMETRIC_VERIFICATION:
                method_strings.push_back("biometric_verification");
                break;
            case VerificationMethod::THIRD_PARTY_KYC:
                method_strings.push_back("third_party_kyc");
                break;
        }
    }
    
    UniValue response(UniValue::VOBJ);
    response.pushKV("country_code", policy->country_code);
    response.pushKV("requires_kyc", policy->compliance_level == ComplianceLevel::KYC_REQUIRED);
    
    std::string access_level_str;
    switch (policy->access_level) {
        case AccessLevel::FULL:
            access_level_str = "full";
            break;
        case AccessLevel::RESTRICTED:
            access_level_str = "restricted";
            break;
        case AccessLevel::BLOCKED:
            access_level_str = "blocked";
            break;
        default:
            access_level_str = "unknown";
    }
    response.pushKV("access_level", access_level_str);
    
    std::string compliance_str;
    switch (policy->compliance_level) {
        case ComplianceLevel::STANDARD:
            compliance_str = "standard";
            break;
        case ComplianceLevel::KYC_REQUIRED:
            compliance_str = "kyc_required";
            break;
        case ComplianceLevel::ENHANCED:
            compliance_str = "enhanced";
            break;
        default:
            compliance_str = "unknown";
    }
    response.pushKV("compliance_level", compliance_str);
    
    UniValue methods(UniValue::VARR);
    for (const auto& m : method_strings) {
        methods.push_back(m);
    }
    response.pushKV("allowed_verification_methods", methods);
    
    UniValue restricted(UniValue::VARR);
    for (const auto& feature : policy->restricted_features) {
        restricted.push_back(feature);
    }
    response.pushKV("restricted_features", restricted);
    response.pushKV("allows_privacy", policy->allows_privacy);
    response.pushKV("allows_anonymous", policy->allows_anonymous);
    
    return WriteJSONResponse(req, response);
}

// ===== Exchange Rate Endpoints =====

bool rest_exchange_rate_current(const std::any& context, HTTPRequest* req, const std::string& strReq)
{
    if (req->GetRequestMethod() != "GET") {
        return WriteErrorResponse(req, "METHOD_NOT_ALLOWED", "Only GET method is allowed", HTTP_BAD_METHOD);
    }
    
    std::string o_currency = ExtractOCurrencyFromPath(strReq);
    if (o_currency.empty()) {
        return WriteErrorResponse(req, "INVALID_PARAMETERS", "O currency not found in URL path");
    }
    
    // Get exchange rate data
    std::string fiat_currency = g_measurement_system.GetCorrespondingFiatCurrency(o_currency);
    if (fiat_currency.empty()) {
        return WriteErrorResponse(req, "INVALID_CURRENCY", "Invalid O currency code");
    }
    
    auto avg_result = g_measurement_system.GetAverageExchangeRateWithConfidence(o_currency, fiat_currency, 7);
    if (!avg_result.has_value()) {
        return WriteErrorResponse(req, "NO_DATA", "No exchange rate data available");
    }
    
    double theoretical_rate = g_measurement_system.GetTheoreticalExchangeRate(o_currency);
    double deviation = g_measurement_system.CalculateStabilityDeviation(o_currency, avg_result->value);
    bool is_stable = deviation <= 0.10; // 10% tolerance
    
    UniValue response(UniValue::VOBJ);
    response.pushKV("o_currency", o_currency);
    response.pushKV("fiat_currency", fiat_currency);
    response.pushKV("current_rate", avg_result->value);
    response.pushKV("theoretical_rate", theoretical_rate);
    response.pushKV("deviation_percent", deviation * 100.0);
    response.pushKV("is_stable", is_stable);
    response.pushKV("last_updated", GetTime());
    response.pushKV("measurement_count", avg_result->measurement_count);
    
    std::string confidence_str;
    switch (avg_result->confidence_level) {
        case OMeasurement::ConfidenceLevel::INSUFFICIENT_DATA:
            confidence_str = "insufficient_data";
            break;
        case OMeasurement::ConfidenceLevel::LOW_CONFIDENCE:
            confidence_str = "low_confidence";
            break;
        case OMeasurement::ConfidenceLevel::HIGH_CONFIDENCE:
            confidence_str = "high_confidence";
            break;
        case OMeasurement::ConfidenceLevel::VERY_HIGH_CONFIDENCE:
            confidence_str = "very_high_confidence";
            break;
        default:
            confidence_str = "unknown";
    }
    response.pushKV("confidence_level", confidence_str);
    
    return WriteJSONResponse(req, response);
}

// ===== Map Data Endpoints =====

bool rest_map_countries(const std::any& context, HTTPRequest* req, const std::string& strReq)
{
    if (req->GetRequestMethod() != "GET") {
        return WriteErrorResponse(req, "METHOD_NOT_ALLOWED", "Only GET method is allowed", HTTP_BAD_METHOD);
    }
    
    // Get all supported fiat currencies
    std::vector<std::string> currencies = OMeasurement::g_measurement_system.GetSupportedFiatCurrencies();
    
    UniValue countries(UniValue::VARR);
    int stable_count = 0;
    int unstable_count = 0;
    
    for (const auto& currency : currencies) {
        std::string o_currency = OMeasurement::g_measurement_system.GetOCurrencyFromFiat(currency);
        if (o_currency.empty()) {
            continue;
        }
        
        auto avg_water = g_measurement_system.GetAverageWaterPrice(currency, 30);
        if (!avg_water.has_value()) continue;
        
        auto avg_exchange = g_measurement_system.GetAverageExchangeRateWithConfidence(o_currency, currency, 7);
        if (!avg_exchange.has_value()) continue;
        
        double deviation = g_measurement_system.CalculateStabilityDeviation(o_currency, avg_exchange->value);
        bool is_stable = deviation <= 0.10;
        
        if (is_stable) stable_count++;
        else unstable_count++;
        
        UniValue country(UniValue::VOBJ);
        country.pushKV("country_code", currency); // Simplified - would map to actual country codes
        country.pushKV("currency", currency);
        country.pushKV("o_currency", o_currency);
        country.pushKV("avg_water_price", avg_water.value());
        country.pushKV("water_price_currency", currency);
        country.pushKV("is_stable", is_stable);
        country.pushKV("stability_color", is_stable ? "green" : "red");
        country.pushKV("measurement_count", avg_exchange->measurement_count);
        country.pushKV("last_updated", GetTime());
        
        // TODO: Add actual country coordinates
        UniValue coords(UniValue::VOBJ);
        coords.pushKV("lat", 0.0);
        coords.pushKV("lng", 0.0);
        country.pushKV("coordinates", coords);
        
        countries.push_back(country);
    }
    
    UniValue response(UniValue::VOBJ);
    response.pushKV("countries", countries);
    response.pushKV("total_countries", static_cast<int>(countries.size()));
    response.pushKV("stable_countries", stable_count);
    response.pushKV("unstable_countries", unstable_count);
    
    return WriteJSONResponse(req, response);
}

// ===== Measurement Notification Endpoints =====

bool rest_notifications_invites(const std::any& context, HTTPRequest* req, const std::string& strReq)
{
    if (req->GetRequestMethod() != "GET") {
        return WriteErrorResponse(req, "METHOD_NOT_ALLOWED", "Only GET method is allowed", HTTP_BAD_METHOD);
    }
    
    std::string publickey_str = ExtractPublicKeyFromPath(strReq);
    if (publickey_str.empty()) {
        return WriteErrorResponse(req, "INVALID_PARAMETERS", "Public key not found in URL path");
    }
    
    CPubKey publickey;
    if (!publickey.SetHex(publickey_str)) {
        return WriteErrorResponse(req, "INVALID_PUBLICKEY", "Invalid public key format");
    }
    
    // Get active invites from database
    if (!g_measurement_db) {
        return WriteErrorResponse(req, "DATABASE_ERROR", "Measurement database not initialized", HTTP_INTERNAL_SERVER_ERROR);
    }
    
    std::vector<OMeasurement::MeasurementInvite> active_invites = g_measurement_db->GetActiveInvites();
    
    // Filter invites for this user
    int64_t current_time = GetTime();
    UniValue invites(UniValue::VARR);
    
    for (const auto& invite : active_invites) {
        if (invite.invited_user != publickey) continue;
        if (!invite.IsValid(current_time)) continue;
        
        UniValue inv(UniValue::VOBJ);
        inv.pushKV("invite_id", invite.invite_id.GetHex());
        
        std::string type_str;
        switch (invite.type) {
            case OMeasurement::MeasurementType::WATER_PRICE:
                type_str = "water";
                break;
            case OMeasurement::MeasurementType::EXCHANGE_RATE:
                type_str = "exchange";
                break;
            case OMeasurement::MeasurementType::WATER_PRICE_OFFLINE_VALIDATION:
                type_str = "validation";
                break;
            default:
                type_str = "unknown";
        }
        inv.pushKV("type", type_str);
        
        if (!invite.currency_code.empty()) {
            inv.pushKV("currency", invite.currency_code);
        }
        
        inv.pushKV("created_at", invite.created_at);
        inv.pushKV("expires_at", invite.expires_at);
        inv.pushKV("time_remaining", invite.expires_at - current_time);
        
        // Calculate reward (placeholder)
        inv.pushKV("reward_amount", "10.00");
        inv.pushKV("reward_currency", "OUSD");
        
        invites.push_back(inv);
    }
    
    UniValue response(UniValue::VOBJ);
    response.pushKV("invites", invites);
    response.pushKV("total", static_cast<int>(invites.size()));
    
    return WriteJSONResponse(req, response);
}

// ===== Exchange Rate Endpoints (Continued) =====

bool rest_exchange_rate_measured(const std::any& context, HTTPRequest* req, const std::string& strReq)
{
    if (req->GetRequestMethod() != "GET") {
        return WriteErrorResponse(req, "METHOD_NOT_ALLOWED", "Only GET method is allowed", HTTP_BAD_METHOD);
    }
    
    std::string o_currency = ExtractOCurrencyFromPath(strReq);
    if (o_currency.empty()) {
        return WriteErrorResponse(req, "INVALID_PARAMETERS", "O currency not found in URL path");
    }
    
    // Parse query parameters
    std::string query = req->GetURI();
    int days = 7; // Default
    size_t days_pos = query.find("days=");
    if (days_pos != std::string::npos) {
        std::string days_str = query.substr(days_pos + 5);
        size_t end_pos = days_str.find("&");
        if (end_pos != std::string::npos) {
            days_str = days_str.substr(0, end_pos);
        }
        try {
            days = std::stoi(days_str);
        } catch (...) {
            days = 7;
        }
    }
    
    std::string fiat_currency = g_measurement_system.GetCorrespondingFiatCurrency(o_currency);
    if (fiat_currency.empty()) {
        return WriteErrorResponse(req, "INVALID_CURRENCY", "Invalid O currency code");
    }
    
    // Get measured rates
    int64_t current_time = GetTime();
    int64_t start_time = current_time - (days * 24 * 3600);
    std::vector<OMeasurement::ExchangeRateMeasurement> measurements = 
        g_measurement_system.GetExchangeRatesInRange(o_currency, fiat_currency, start_time, current_time);
    
    double theoretical_rate = g_measurement_system.GetTheoreticalExchangeRate(o_currency);
    
    UniValue measured_rates(UniValue::VARR);
    double sum_rates = 0.0;
    int valid_count = 0;
    
    for (const auto& m : measurements) {
        if (!m.is_validated) continue;
        
        UniValue rate_obj(UniValue::VOBJ);
        rate_obj.pushKV("rate", m.exchange_rate);
        rate_obj.pushKV("timestamp", m.timestamp);
        
        std::string source_str;
        switch (m.source) {
            case OMeasurement::MeasurementSource::USER_ONLINE:
                source_str = "user_online";
                break;
            case OMeasurement::MeasurementSource::USER_OFFLINE:
                source_str = "user_offline";
                break;
            case OMeasurement::MeasurementSource::BOT_ONLINE:
                source_str = "bot_online";
                break;
            case OMeasurement::MeasurementSource::BOT_OFFLINE:
                source_str = "bot_offline";
                break;
            default:
                source_str = "unknown";
        }
        rate_obj.pushKV("source", source_str);
        rate_obj.pushKV("is_validated", m.is_validated);
        
        measured_rates.push_back(rate_obj);
        sum_rates += m.exchange_rate;
        valid_count++;
    }
    
    double avg_measured = valid_count > 0 ? sum_rates / valid_count : 0.0;
    double volatility = g_measurement_system.CalculateVolatility(OMeasurement::MeasurementType::EXCHANGE_RATE, o_currency, days);
    
    UniValue response(UniValue::VOBJ);
    response.pushKV("o_currency", o_currency);
    response.pushKV("fiat_currency", fiat_currency);
    response.pushKV("measured_rates", measured_rates);
    response.pushKV("theoretical_rate", theoretical_rate);
    response.pushKV("average_measured_rate", avg_measured);
    response.pushKV("volatility", volatility);
    response.pushKV("measurement_count", valid_count);
    
    return WriteJSONResponse(req, response);
}

bool rest_exchange_rate_historical(const std::any& context, HTTPRequest* req, const std::string& strReq)
{
    if (req->GetRequestMethod() != "GET") {
        return WriteErrorResponse(req, "METHOD_NOT_ALLOWED", "Only GET method is allowed", HTTP_BAD_METHOD);
    }
    
    std::string o_currency = ExtractOCurrencyFromPath(strReq);
    if (o_currency.empty()) {
        return WriteErrorResponse(req, "INVALID_PARAMETERS", "O currency not found in URL path");
    }
    
    // Parse query parameters
    std::string query = req->GetURI();
    std::string start_date, end_date, interval = "daily";
    
    size_t start_pos = query.find("start_date=");
    if (start_pos != std::string::npos) {
        start_date = query.substr(start_pos + 11, 10); // YYYY-MM-DD
    }
    
    size_t end_pos = query.find("end_date=");
    if (end_pos != std::string::npos) {
        end_date = query.substr(end_pos + 9, 10);
    }
    
    size_t interval_pos = query.find("interval=");
    if (interval_pos != std::string::npos) {
        std::string interval_str = query.substr(interval_pos + 9);
        size_t end_interval = interval_str.find("&");
        if (end_interval != std::string::npos) {
            interval_str = interval_str.substr(0, end_interval);
        }
        interval = interval_str;
    }
    
    if (start_date.empty() || end_date.empty()) {
        return WriteErrorResponse(req, "MISSING_PARAMETERS", "start_date and end_date are required");
    }
    
    std::string fiat_currency = g_measurement_system.GetCorrespondingFiatCurrency(o_currency);
    if (fiat_currency.empty()) {
        return WriteErrorResponse(req, "INVALID_CURRENCY", "Invalid O currency code");
    }
    
    // Get daily averages in range
    auto daily_averages = g_measurement_system.GetDailyAveragesInRange(o_currency, start_date, end_date);
    
    UniValue data(UniValue::VARR);
    for (const auto& avg : daily_averages) {
        UniValue day(UniValue::VOBJ);
        day.pushKV("date", avg.date);
        day.pushKV("avg_rate", avg.avg_exchange_rate);
        day.pushKV("min_rate", avg.avg_exchange_rate); // TODO: Calculate min/max
        day.pushKV("max_rate", avg.avg_exchange_rate);
        day.pushKV("measurement_count", static_cast<int64_t>(avg.measurement_count));
        day.pushKV("is_stable", avg.is_stable);
        data.push_back(day);
    }
    
    UniValue response(UniValue::VOBJ);
    response.pushKV("o_currency", o_currency);
    response.pushKV("fiat_currency", fiat_currency);
    response.pushKV("data", data);
    
    return WriteJSONResponse(req, response);
}

// ===== Map Data Endpoints (Continued) =====

bool rest_map_country(const std::any& context, HTTPRequest* req, const std::string& strReq)
{
    if (req->GetRequestMethod() != "GET") {
        return WriteErrorResponse(req, "METHOD_NOT_ALLOWED", "Only GET method is allowed", HTTP_BAD_METHOD);
    }
    
    std::string country_code = ExtractCountryCodeFromPath(strReq);
    if (country_code.empty()) {
        return WriteErrorResponse(req, "INVALID_PARAMETERS", "Country code not found in URL path");
    }
    
    // Map country code to currency (currently assumes currency code provided)
    std::string currency = country_code;
    std::string o_currency = OMeasurement::g_measurement_system.GetOCurrencyFromFiat(currency);
    if (o_currency.empty()) {
        // If the input was already an O currency, extract fiat component
        if (!currency.empty() && currency[0] == 'O' && currency.size() > 1) {
            o_currency = currency;
            currency = currency.substr(1);
        } else {
            return WriteErrorResponse(req, "INVALID_CURRENCY", "Unsupported currency code for map data");
        }
    }
    
    // Get water price data
    auto avg_water = g_measurement_system.GetAverageWaterPriceWithConfidence(currency, 30);
    if (!avg_water.has_value()) {
        return WriteErrorResponse(req, "NO_DATA", "No water price data available for this country");
    }
    
    // Get exchange rate data
    auto avg_exchange = g_measurement_system.GetAverageExchangeRateWithConfidence(o_currency, currency, 7);
    if (!avg_exchange.has_value()) {
        return WriteErrorResponse(req, "NO_DATA", "No exchange rate data available for this country");
    }
    
    double deviation = g_measurement_system.CalculateStabilityDeviation(o_currency, avg_exchange->value);
    bool is_stable = deviation <= 0.10;
    double volatility = g_measurement_system.CalculateVolatility(OMeasurement::MeasurementType::EXCHANGE_RATE, o_currency, 7);
    
    UniValue response(UniValue::VOBJ);
    response.pushKV("country_code", country_code);
    response.pushKV("currency", currency);
    response.pushKV("o_currency", o_currency);
    
    // Water price info
    UniValue water_price(UniValue::VOBJ);
    water_price.pushKV("avg_price", avg_water->value);
    water_price.pushKV("currency", currency);
    water_price.pushKV("measurement_count", avg_water->measurement_count);
    water_price.pushKV("confidence_level", avg_water->GetConfidenceString());
    water_price.pushKV("last_updated", GetTime());
    response.pushKV("water_price", water_price);
    
    // Exchange rate info
    UniValue exchange_rate(UniValue::VOBJ);
    exchange_rate.pushKV("current_rate", avg_exchange->value);
    exchange_rate.pushKV("is_stable", is_stable);
    exchange_rate.pushKV("deviation_percent", deviation * 100.0);
    response.pushKV("exchange_rate", exchange_rate);
    
    // Stability info
    UniValue stability(UniValue::VOBJ);
    stability.pushKV("status", is_stable ? "stable" : "unstable");
    stability.pushKV("color", is_stable ? "green" : "red");
    stability.pushKV("volatility", volatility);
    stability.pushKV("trend", "stable"); // TODO: Calculate trend
    response.pushKV("stability", stability);
    
    // Coordinates (placeholder)
    UniValue coords(UniValue::VOBJ);
    coords.pushKV("lat", 0.0);
    coords.pushKV("lng", 0.0);
    response.pushKV("coordinates", coords);
    
    return WriteJSONResponse(req, response);
}

// ===== Wallet Endpoints =====

bool rest_wallet_balance(const std::any& context, HTTPRequest* req, const std::string& strReq)
{
    if (req->GetRequestMethod() != "GET") {
        return WriteErrorResponse(req, "METHOD_NOT_ALLOWED", "Only GET method is allowed", HTTP_BAD_METHOD);
    }
    
    std::string publickey_str = ExtractPublicKeyFromPath(strReq);
    if (publickey_str.empty()) {
        return WriteErrorResponse(req, "INVALID_PARAMETERS", "Public key not found in URL path");
    }
    
    // TODO: Get actual wallet balance from blockchain
    // This would require querying UTXOs for addresses derived from the public key
    
    UniValue balances(UniValue::VOBJ);
    balances.pushKV("OUSD", "0.00");
    balances.pushKV("OEUR", "0.00");
    balances.pushKV("OJPY", "0.00");
    
    UniValue response(UniValue::VOBJ);
    response.pushKV("publickey", publickey_str);
    response.pushKV("balances", balances);
    response.pushKV("total_value_usd", "0.00");
    response.pushKV("last_updated", GetTime());
    
    return WriteJSONResponse(req, response);
}

bool rest_wallet_transactions(const std::any& context, HTTPRequest* req, const std::string& strReq)
{
    if (req->GetRequestMethod() != "GET") {
        return WriteErrorResponse(req, "METHOD_NOT_ALLOWED", "Only GET method is allowed", HTTP_BAD_METHOD);
    }
    
    std::string publickey_str = ExtractPublicKeyFromPath(strReq);
    if (publickey_str.empty()) {
        return WriteErrorResponse(req, "INVALID_PARAMETERS", "Public key not found in URL path");
    }
    
    // Parse query parameters
    int limit = 50;
    int offset = 0;
    std::string currency_filter = "";
    
    // TODO: Parse query string for limit, offset, currency
    
    // TODO: Query blockchain for transactions involving this public key
    
    UniValue transactions(UniValue::VARR);
    // Placeholder - would query actual transactions
    
    UniValue response(UniValue::VOBJ);
    response.pushKV("transactions", transactions);
    response.pushKV("total", 0);
    response.pushKV("limit", limit);
    response.pushKV("offset", offset);
    
    return WriteJSONResponse(req, response);
}

bool rest_wallet_send(const std::any& context, HTTPRequest* req, const std::string& strReq)
{
    if (req->GetRequestMethod() != "POST") {
        return WriteErrorResponse(req, "METHOD_NOT_ALLOWED", "Only POST method is allowed", HTTP_BAD_METHOD);
    }
    
    // TODO: Implement payment sending
    // This would require wallet access and transaction creation
    
    return WriteErrorResponse(req, "NOT_IMPLEMENTED", "Wallet send functionality requires wallet integration");
}

// ===== Measurement Submission Endpoints =====

bool rest_measurements_submit_water(const std::any& context, HTTPRequest* req, const std::string& strReq)
{
    if (req->GetRequestMethod() != "POST") {
        return WriteErrorResponse(req, "METHOD_NOT_ALLOWED", "Only POST method is allowed", HTTP_BAD_METHOD);
    }
    
    UniValue json;
    if (!ParseJSONRequest(req, json)) {
        return WriteErrorResponse(req, "INVALID_JSON", "Invalid JSON in request body");
    }
    
    // Validate required fields
    if (!json.exists("invite_id") || !json.exists("currency") || !json.exists("price") || 
        !json.exists("source_type") || !json.exists("publickey")) {
        return WriteErrorResponse(req, "MISSING_PARAMETERS", "Missing required parameters");
    }
    
    std::string invite_id_str = json["invite_id"].get_str();
    std::string currency = json["currency"].get_str();
    double price = json["price"].get_real();
    std::string source_type = json["source_type"].get_str();
    std::string publickey_str = json["publickey"].get_str();
    
    UniValue invite_id_uni(UniValue::VSTR);
    invite_id_uni.setStr(invite_id_str);
    uint256 invite_id = ParseHashV(invite_id_uni, "invite_id");
    
    // Validate invite
    if (!g_measurement_db) {
        return WriteErrorResponse(req, "DATABASE_ERROR", "Measurement database not initialized", HTTP_INTERNAL_SERVER_ERROR);
    }
    
    auto invite_opt = g_measurement_db->ReadInvite(invite_id);
    if (!invite_opt.has_value() || !invite_opt->IsValid(GetTime())) {
        return WriteErrorResponse(req, "INVALID_INVITE", "Invalid or expired invitation");
    }
    
    // Convert price to micro-units (6 decimal places)
    int64_t price_micro = static_cast<int64_t>(price * 1000000);
    
    // Determine proof type and data
    std::string proof_type = "url";
    std::string proof_data = "";
    
    if (source_type == "user_online" || source_type == "bot_online") {
        if (!json.exists("source_url")) {
            return WriteErrorResponse(req, "MISSING_PARAMETERS", "source_url required for online measurements");
        }
        proof_type = "url";
        proof_data = json["source_url"].get_str();
    } else {
        if (!json.exists("location")) {
            return WriteErrorResponse(req, "MISSING_PARAMETERS", "location required for offline measurements");
        }
        proof_type = "gps_photo";
        proof_data = json["location"].get_str();
        if (json.exists("image_hash")) {
            proof_data += ":" + json["image_hash"].get_str();
        }
    }
    
    // TODO: Create blockchain transaction via RPC call
    // For now, return success with placeholder
    
    UniValue response(UniValue::VOBJ);
    response.pushKV("success", true);
    response.pushKV("measurement_id", GetRandHash().GetHex());
    response.pushKV("validation_passed", true);
    response.pushKV("validation_result", "passed");
    response.pushKV("reward_amount", "10.00");
    response.pushKV("reward_currency", "OUSD");
    response.pushKV("txid", GetRandHash().GetHex());
    response.pushKV("message", "Measurement submitted. Transaction creation requires wallet integration.");
    
    return WriteJSONResponse(req, response);
}

bool rest_measurements_submit_exchange(const std::any& context, HTTPRequest* req, const std::string& strReq)
{
    if (req->GetRequestMethod() != "POST") {
        return WriteErrorResponse(req, "METHOD_NOT_ALLOWED", "Only POST method is allowed", HTTP_BAD_METHOD);
    }
    
    UniValue json;
    if (!ParseJSONRequest(req, json)) {
        return WriteErrorResponse(req, "INVALID_JSON", "Invalid JSON in request body");
    }
    
    // Validate required fields
    if (!json.exists("invite_id") || !json.exists("from_currency") || !json.exists("to_currency") || 
        !json.exists("exchange_rate") || !json.exists("publickey")) {
        return WriteErrorResponse(req, "MISSING_PARAMETERS", "Missing required parameters");
    }
    
    std::string invite_id_str = json["invite_id"].get_str();
    std::string from_currency = json["from_currency"].get_str();
    std::string to_currency = json["to_currency"].get_str();
    double exchange_rate = json["exchange_rate"].get_real();
    std::string publickey_str = json["publickey"].get_str();
    
    UniValue invite_id_uni(UniValue::VSTR);
    invite_id_uni.setStr(invite_id_str);
    uint256 invite_id = ParseHashV(invite_id_uni, "invite_id");
    
    // Validate invite
    if (!g_measurement_db) {
        return WriteErrorResponse(req, "DATABASE_ERROR", "Measurement database not initialized", HTTP_INTERNAL_SERVER_ERROR);
    }
    
    auto invite_opt = g_measurement_db->ReadInvite(invite_id);
    if (!invite_opt.has_value() || !invite_opt->IsValid(GetTime())) {
        return WriteErrorResponse(req, "INVALID_INVITE", "Invalid or expired invitation");
    }
    
    // Convert rate to micro-units
    int64_t rate_micro = static_cast<int64_t>(exchange_rate * 1000000);
    
    std::string proof_data = "";
    if (json.exists("source_url")) {
        proof_data = json["source_url"].get_str();
    }
    
    // TODO: Create blockchain transaction via RPC call
    
    UniValue response(UniValue::VOBJ);
    response.pushKV("success", true);
    response.pushKV("measurement_id", GetRandHash().GetHex());
    response.pushKV("validation_passed", true);
    response.pushKV("reward_amount", "10.00");
    response.pushKV("reward_currency", "OUSD");
    response.pushKV("txid", GetRandHash().GetHex());
    response.pushKV("message", "Measurement submitted. Transaction creation requires wallet integration.");
    
    return WriteJSONResponse(req, response);
}

bool rest_notifications_measurements(const std::any& context, HTTPRequest* req, const std::string& strReq)
{
    if (req->GetRequestMethod() != "GET") {
        return WriteErrorResponse(req, "METHOD_NOT_ALLOWED", "Only GET method is allowed", HTTP_BAD_METHOD);
    }
    
    std::string publickey_str = ExtractPublicKeyFromPath(strReq);
    if (publickey_str.empty()) {
        return WriteErrorResponse(req, "INVALID_PARAMETERS", "Public key not found in URL path");
    }
    
    CPubKey publickey;
    if (!publickey.SetHex(publickey_str)) {
        return WriteErrorResponse(req, "INVALID_PUBLICKEY", "Invalid public key format");
    }
    
    // TODO: Query database for measurements by this user
    // This would require tracking submitter in measurements
    
    UniValue measurements(UniValue::VARR);
    // Placeholder
    
    UniValue response(UniValue::VOBJ);
    response.pushKV("measurements", measurements);
    response.pushKV("total", 0);
    response.pushKV("total_rewards", "0.00");
    
    return WriteJSONResponse(req, response);
}

// ===== General Information Endpoints =====

bool rest_info_currencies(const std::any& context, HTTPRequest* req, const std::string& strReq)
{
    if (req->GetRequestMethod() != "GET") {
        return WriteErrorResponse(req, "METHOD_NOT_ALLOWED", "Only GET method is allowed", HTTP_BAD_METHOD);
    }
    
    std::vector<std::string> fiat_currencies = OMeasurement::g_measurement_system.GetSupportedFiatCurrencies();
    std::vector<std::string> o_currency_list = OMeasurement::g_measurement_system.GetSupportedOCurrencies();
    
    UniValue fiat(UniValue::VARR);
    UniValue o_currencies(UniValue::VARR);
    
    for (const auto& currency : fiat_currencies) {
        fiat.push_back(currency);
    }
    for (const auto& currency : o_currency_list) {
        o_currencies.push_back(currency);
    }
    
    UniValue response(UniValue::VOBJ);
    response.pushKV("fiat_currencies", fiat);
    response.pushKV("o_currencies", o_currencies);
    response.pushKV("total", static_cast<int>(fiat_currencies.size()));
    
    return WriteJSONResponse(req, response);
}

bool rest_info_stability_status(const std::any& context, HTTPRequest* req, const std::string& strReq)
{
    if (req->GetRequestMethod() != "GET") {
        return WriteErrorResponse(req, "METHOD_NOT_ALLOWED", "Only GET method is allowed", HTTP_BAD_METHOD);
    }
    
    std::vector<std::string> currencies = OMeasurement::g_measurement_system.GetSupportedFiatCurrencies();
    
    int total = 0;
    int stable = 0;
    int unstable = 0;
    
    for (const auto& currency : currencies) {
        std::string o_currency = OMeasurement::g_measurement_system.GetOCurrencyFromFiat(currency);
        if (o_currency.empty()) {
            continue;
        }
        auto avg = g_measurement_system.GetAverageExchangeRateWithConfidence(o_currency, currency, 7);
        if (avg.has_value()) {
            total++;
            double deviation = g_measurement_system.CalculateStabilityDeviation(o_currency, avg->value);
            if (deviation <= 0.10) {
                stable++;
            } else {
                unstable++;
            }
        }
    }
    
    double stability_percentage = total > 0 ? (static_cast<double>(stable) / total) * 100.0 : 0.0;
    
    UniValue response(UniValue::VOBJ);
    response.pushKV("total_currencies", total);
    response.pushKV("stable_currencies", stable);
    response.pushKV("unstable_currencies", unstable);
    response.pushKV("stability_percentage", stability_percentage);
    response.pushKV("last_updated", GetTime());
    
    return WriteJSONResponse(req, response);
}

