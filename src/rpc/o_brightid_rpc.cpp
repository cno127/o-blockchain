// Copyright (c) 2024 The Bitcoin Core developers
// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/o_brightid_rpc.h>
#include <consensus/brightid_integration.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <util/strencodings.h>
#include <logging.h>

#include <string>
#include <vector>

// ===== BrightID Integration Management =====

UniValue initializebrightid(const JSONRPCRequest& request)
{
    if (request.params.size() != 2) {
        throw std::runtime_error(
            "initializebrightid \"context_id\" \"api_endpoint\"\n"
            "\nInitialize BrightID integration.\n"
            "\nArguments:\n"
            "1. context_id        (string, required) Context ID for this application\n"
            "2. api_endpoint      (string, required) BrightID API endpoint\n"
            "\nResult:\n"
            "{\n"
            "  \"success\": true,\n"
            "  \"context_id\": \"context_id\",\n"
            "  \"api_endpoint\": \"api_endpoint\",\n"
            "  \"status\": \"Initialized\"\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("initializebrightid", "\"o_blockchain\" \"https://app.brightid.org\"")
            + HelpExampleRpc("initializebrightid", "\"o_blockchain\", \"https://app.brightid.org\"")
        );
    }

    std::string context_id = request.params[0].get_str();
    std::string api_endpoint = request.params[1].get_str();

    if (context_id.empty() || api_endpoint.empty()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Context ID and API endpoint cannot be empty");
    }

    bool success = g_brightid_integration.Initialize(context_id, api_endpoint);

    UniValue result(UniValue::VOBJ);
    result.pushKV("success", success);
    result.pushKV("context_id", context_id);
    result.pushKV("api_endpoint", api_endpoint);
    result.pushKV("status", success ? "Initialized" : "Failed to initialize");

    return result;
}

UniValue getbrightidstatus(const JSONRPCRequest& request)
{
    if (request.params.size() != 0) {
        throw std::runtime_error(
            "getbrightidstatus\n"
            "\nGet BrightID integration status.\n"
            "\nResult:\n"
            "{\n"
            "  \"initialized\": true,\n"
            "  \"api_status\": \"Available\",\n"
            "  \"brightid_available\": true\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getbrightidstatus", "")
            + HelpExampleRpc("getbrightidstatus", "")
        );
    }

    UniValue result(UniValue::VOBJ);
    result.pushKV("initialized", g_brightid_integration.IsBrightIDAvailable());
    result.pushKV("api_status", g_brightid_integration.GetAPIStatus());
    result.pushKV("brightid_available", g_brightid_integration.IsBrightIDAvailable());

    return result;
}

UniValue getbrightidstatistics(const JSONRPCRequest& request)
{
    if (request.params.size() != 0) {
        throw std::runtime_error(
            "getbrightidstatistics\n"
            "\nGet BrightID integration statistics.\n"
            "\nResult:\n"
            "{\n"
            "  \"verification_stats\": {...},\n"
            "  \"user_stats\": {...},\n"
            "  \"social_graph_stats\": {...},\n"
            "  \"privacy_stats\": {...}\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getbrightidstatistics", "")
            + HelpExampleRpc("getbrightidstatistics", "")
        );
    }

    auto verification_stats = g_brightid_integration.GetVerificationStatistics();
    auto user_stats = g_brightid_integration.GetUserStatistics();
    auto social_graph_stats = g_brightid_integration.GetSocialGraphStatistics();
    auto privacy_stats = g_brightid_integration.GetPrivacyStatistics();

    UniValue result(UniValue::VOBJ);
    
    UniValue verification_obj(UniValue::VOBJ);
    for (const auto& [key, value] : verification_stats) {
        verification_obj.pushKV(key, value);
    }
    result.pushKV("verification_stats", verification_obj);
    
    UniValue user_obj(UniValue::VOBJ);
    for (const auto& [key, value] : user_stats) {
        user_obj.pushKV(key, value);
    }
    result.pushKV("user_stats", user_obj);
    
    UniValue social_obj(UniValue::VOBJ);
    for (const auto& [key, value] : social_graph_stats) {
        social_obj.pushKV(key, value);
    }
    result.pushKV("social_graph_stats", social_obj);
    
    UniValue privacy_obj(UniValue::VOBJ);
    for (const auto& [key, value] : privacy_stats) {
        privacy_obj.pushKV(key, value);
    }
    result.pushKV("privacy_stats", privacy_obj);

    return result;
}

// ===== User Verification =====

UniValue verifyuser(const JSONRPCRequest& request)
{
    if (request.params.size() != 3) {
        throw std::runtime_error(
            "verifyuser \"brightid_address\" \"verification_data\" \"signature\"\n"
            "\nVerify a user with BrightID.\n"
            "\nArguments:\n"
            "1. brightid_address   (string, required) BrightID address\n"
            "2. verification_data  (string, required) Verification data from BrightID\n"
            "3. signature          (string, required) Signature from BrightID\n"
            "\nResult:\n"
            "{\n"
            "  \"success\": true,\n"
            "  \"brightid_address\": \"address\",\n"
            "  \"verified\": true,\n"
            "  \"trust_score\": 0.85\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("verifyuser", "\"0x123...\" \"{\\\"method\\\":\\\"social_graph\\\"}\" \"signature\"")
            + HelpExampleRpc("verifyuser", "\"0x123...\", \"{\\\"method\\\":\\\"social_graph\\\"}\", \"signature\"")
        );
    }

    std::string brightid_address = request.params[0].get_str();
    std::string verification_data = request.params[1].get_str();
    std::string signature = request.params[2].get_str();

    if (brightid_address.empty() || verification_data.empty() || signature.empty()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "All parameters must be non-empty");
    }

    if (!g_brightid_integration.ValidateBrightIDAddress(brightid_address)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid BrightID address format");
    }

    BrightIDVerificationRequest verification_request;
    verification_request.brightid_address = brightid_address;
    verification_request.context_id = g_brightid_integration.GenerateContextID();
    verification_request.verification_data = verification_data;
    verification_request.signature = signature;
    verification_request.timestamp = GetTime();
    verification_request.request_id = "req_" + std::to_string(GetTime());

    bool success = g_brightid_integration.VerifyUser(verification_request);
    
    auto user_status = g_brightid_integration.GetUserStatus(brightid_address);
    double trust_score = 0.0;
    if (user_status.has_value()) {
        trust_score = user_status->trust_score;
    }

    UniValue result(UniValue::VOBJ);
    result.pushKV("success", success);
    result.pushKV("brightid_address", brightid_address);
    result.pushKV("verified", success);
    result.pushKV("trust_score", trust_score);

    return result;
}

UniValue getuserstatus(const JSONRPCRequest& request)
{
    if (request.params.size() != 1) {
        throw std::runtime_error(
            "getuserstatus \"brightid_address\"\n"
            "\nGet user verification status.\n"
            "\nArguments:\n"
            "1. brightid_address   (string, required) BrightID address\n"
            "\nResult:\n"
            "{\n"
            "  \"brightid_address\": \"address\",\n"
            "  \"status\": \"VERIFIED\",\n"
            "  \"method\": \"SOCIAL_GRAPH\",\n"
            "  \"trust_score\": 0.85,\n"
            "  \"is_active\": true,\n"
            "  \"verification_timestamp\": 1234567890,\n"
            "  \"expiration_timestamp\": 1234567890\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getuserstatus", "\"0x123...\"")
            + HelpExampleRpc("getuserstatus", "\"0x123...\"")
        );
    }

    std::string brightid_address = request.params[0].get_str();
    if (brightid_address.empty()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "BrightID address cannot be empty");
    }

    auto user_status = g_brightid_integration.GetUserStatus(brightid_address);
    if (!user_status.has_value()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "User not found: " + brightid_address);
    }

    const auto& user = user_status.value();

    UniValue result(UniValue::VOBJ);
    result.pushKV("brightid_address", user.brightid_address);
    result.pushKV("status", static_cast<int>(user.status));
    result.pushKV("method", static_cast<int>(user.method));
    result.pushKV("trust_score", user.trust_score);
    result.pushKV("is_active", user.is_active);
    result.pushKV("verification_timestamp", user.verification_timestamp);
    result.pushKV("expiration_timestamp", user.expiration_timestamp);

    return result;
}

// ===== Privacy-Preserving Features =====

UniValue generateanonymousid(const JSONRPCRequest& request)
{
    if (request.params.size() != 1) {
        throw std::runtime_error(
            "generateanonymousid \"brightid_address\"\n"
            "\nGenerate anonymous ID for privacy-preserving operations.\n"
            "\nArguments:\n"
            "1. brightid_address   (string, required) BrightID address\n"
            "\nResult:\n"
            "{\n"
            "  \"brightid_address\": \"address\",\n"
            "  \"anonymous_id\": \"hash\",\n"
            "  \"privacy_preserved\": true\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("generateanonymousid", "\"0x123...\"")
            + HelpExampleRpc("generateanonymousid", "\"0x123...\"")
        );
    }

    std::string brightid_address = request.params[0].get_str();
    if (brightid_address.empty()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "BrightID address cannot be empty");
    }

    std::string anonymous_id = g_brightid_integration.GenerateAnonymousID(brightid_address);

    UniValue result(UniValue::VOBJ);
    result.pushKV("brightid_address", brightid_address);
    result.pushKV("anonymous_id", anonymous_id);
    result.pushKV("privacy_preserved", true);

    return result;
}

UniValue anonymousverification(const JSONRPCRequest& request)
{
    if (request.params.size() != 1) {
        throw std::runtime_error(
            "anonymousverification \"anonymous_id\"\n"
            "\nVerify user anonymously without revealing identity.\n"
            "\nArguments:\n"
            "1. anonymous_id       (string, required) Anonymous ID\n"
            "\nResult:\n"
            "{\n"
            "  \"anonymous_id\": \"hash\",\n"
            "  \"verified\": true,\n"
            "  \"privacy_preserved\": true\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("anonymousverification", "\"hash123...\"")
            + HelpExampleRpc("anonymousverification", "\"hash123...\"")
        );
    }

    std::string anonymous_id = request.params[0].get_str();
    if (anonymous_id.empty()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Anonymous ID cannot be empty");
    }

    bool verified = g_brightid_integration.AnonymousVerification(anonymous_id);

    UniValue result(UniValue::VOBJ);
    result.pushKV("anonymous_id", anonymous_id);
    result.pushKV("verified", verified);
    result.pushKV("privacy_preserved", true);

    return result;
}

// ===== O Blockchain Integration =====

UniValue registeruser(const JSONRPCRequest& request)
{
    if (request.params.size() != 2) {
        throw std::runtime_error(
            "registeruser \"brightid_address\" \"o_address\"\n"
            "\nRegister user with O Blockchain.\n"
            "\nArguments:\n"
            "1. brightid_address   (string, required) BrightID address\n"
            "2. o_address          (string, required) O Blockchain address\n"
            "\nResult:\n"
            "{\n"
            "  \"success\": true,\n"
            "  \"brightid_address\": \"address\",\n"
            "  \"o_address\": \"address\",\n"
            "  \"registered\": true\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("registeruser", "\"0x123...\" \"bc1q...\"")
            + HelpExampleRpc("registeruser", "\"0x123...\", \"bc1q...\"")
        );
    }

    std::string brightid_address = request.params[0].get_str();
    std::string o_address = request.params[1].get_str();

    if (brightid_address.empty() || o_address.empty()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Addresses cannot be empty");
    }

    bool success = g_brightid_integration.RegisterUser(brightid_address, o_address);

    UniValue result(UniValue::VOBJ);
    result.pushKV("success", success);
    result.pushKV("brightid_address", brightid_address);
    result.pushKV("o_address", o_address);
    result.pushKV("registered", success);

    return result;
}

UniValue getoaddress(const JSONRPCRequest& request)
{
    if (request.params.size() != 1) {
        throw std::runtime_error(
            "getoaddress \"brightid_address\"\n"
            "\nGet O Blockchain address for BrightID address.\n"
            "\nArguments:\n"
            "1. brightid_address   (string, required) BrightID address\n"
            "\nResult:\n"
            "{\n"
            "  \"brightid_address\": \"address\",\n"
            "  \"o_address\": \"address\"\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getoaddress", "\"0x123...\"")
            + HelpExampleRpc("getoaddress", "\"0x123...\"")
        );
    }

    std::string brightid_address = request.params[0].get_str();
    if (brightid_address.empty()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "BrightID address cannot be empty");
    }

    auto o_address = g_brightid_integration.GetOAddress(brightid_address);
    if (!o_address.has_value()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "No O address found for BrightID: " + brightid_address);
    }

    UniValue result(UniValue::VOBJ);
    result.pushKV("brightid_address", brightid_address);
    result.pushKV("o_address", o_address.value());

    return result;
}

UniValue submituserverification(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 6 || request.params.size() > 8) {
        throw std::runtime_error(
            "submituserverification \"user_id\" \"identity_provider\" \"country_code\" \"birth_currency\" \"verification_data\" \"provider_signature\" ( \"expiration\" \"o_pubkey\" )\n"
            "\nSubmit a user verification transaction to the blockchain.\n"
            "Supports multiple identity providers: BrightID, KYC systems, WorldCoin, etc.\n"
            "\nArguments:\n"
            "1. user_id              (string, required) Unique user ID from identity provider\n"
            "2. identity_provider    (string, required) Provider: 'brightid', 'kyc_usa', 'kyc_fra', 'worldcoin', etc.\n"
            "3. country_code         (string, required) ISO 3166-1 alpha-3 country code (USA, FRA, MEX)\n"
            "4. birth_currency       (string, required) Birth currency for UBI (OUSD, OEUR, OMXN) - IMMUTABLE!\n"
            "5. verification_data    (string, required) JSON verification data from provider\n"
            "6. provider_signature   (string, required) Provider's signature (hex)\n"
            "7. expiration           (numeric, optional) Expiration timestamp (0 = never, default = 1 year)\n"
            "8. o_pubkey             (string, optional) O Blockchain pubkey (default = wallet's pubkey)\n"
            "\nResult:\n"
            "{\n"
            "  \"txid\": \"hash\",\n"
            "  \"user_key\": \"provider:userid\",\n"
            "  \"birth_currency\": \"OUSD\",\n"
            "  \"country\": \"USA\"\n"
            "}\n"
            "\nNOTE: birth_currency determines UBI payments for LIFE and CANNOT be changed!\n"
            "\nExamples:\n"
            + HelpExampleCli("submituserverification", "\"0x1a2b3c...\" \"brightid\" \"USA\" \"OUSD\" '{\"score\":95}' \"0xabcd...\"")
            + HelpExampleRpc("submituserverification", "\"0x1a2b3c...\", \"kyc_fra\", \"FRA\", \"OEUR\", '{\"level\":\"full\"}', \"0x1234...\"")
        );
    }

    // TODO: Implement transaction creation and broadcast
    // For now, return placeholder
    
    UniValue result(UniValue::VOBJ);
    result.pushKV("error", "Not yet implemented - implementation in progress");
    result.pushKV("status", "RPC command structure ready, transaction builder coming soon");
    result.pushKV("note", "This will create a USER_VERIFY transaction and broadcast to network");
    
    return result;
}

// ===== RPC Command Registration =====

void RegisterOBrightIDRPCCommands(CRPCTable& t)
{
    // BrightID Integration Management
    static const CRPCCommand commands[] = {
        {"brightid", &initializebrightid},
        {"brightid", &getbrightidstatus},
        {"brightid", &getbrightidstatistics},
        
        // User Verification
        {"brightid", &verifyuser},
        {"brightid", &getuserstatus},
        {"brightid", &submituserverification},  // NEW: Submit to blockchain
        
        // Privacy-Preserving Features
        {"brightid", &generateanonymousid},
        {"brightid", &anonymousverification},
        
        // O Blockchain Integration
        {"brightid", &registeruser},
        {"brightid", &getoaddress},
    };
    
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}
