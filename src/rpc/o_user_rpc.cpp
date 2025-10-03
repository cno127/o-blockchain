// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/o_user_rpc.h>
#include <validation/o_integration.h>
#include <consensus/user_consensus.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <rpc/request.h>
#include <util/strencodings.h>
#include <key_io.h>
#include <logging.h>

#include <univalue.h>

/**
 * O Blockchain User RPC Methods
 * 
 * This module provides RPC methods for user registration, verification,
 * and management in the O blockchain system.
 */

static RPCHelpMan registeruser() {
    return RPCHelpMan{
        "registeruser",
        "\nRegister a new user in the O blockchain system.\n",
        {
            {"publickey", RPCArg::Type::STR, RPCArg::Optional::NO, "User's public key in hex format"},
            {"government_id", RPCArg::Type::STR, RPCArg::Optional::NO, "Government ID number (will be hashed)"},
            {"birth_currency", RPCArg::Type::STR, RPCArg::Optional::NO, "Birth country currency code (e.g., USD, EUR)"},
            {"country_code", RPCArg::Type::STR, RPCArg::Optional::NO, "Country code (e.g., US, EU, JP)"},
            {"identity_proof", RPCArg::Type::STR, RPCArg::Optional::NO, "Hash of identity verification documents"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "user_id", "User's public key"},
                {RPCResult::Type::STR, "status", "Registration status"},
                {RPCResult::Type::STR, "message", "Status message"},
                {RPCResult::Type::NUM, "registration_height", "Block height when registered"},
            }
        },
        RPCExamples{
            HelpExampleCli("registeruser", "\"02f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f9\" \"123456789\" \"USD\" \"US\" \"abcd1234...\"")
            + HelpExampleRpc("registeruser", "\"02f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f9\", \"123456789\", \"USD\", \"US\", \"abcd1234...\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue {
            if (g_o_integration == nullptr) {
                throw JSONRPCError(RPC_MISC_ERROR, "O blockchain integration not available");
            }

            // Parse parameters
            std::string publickey_str = request.params[0].get_str();
            std::string government_id = request.params[1].get_str();
            std::string birth_currency = request.params[2].get_str();
            std::string country_code = request.params[3].get_str();
            std::string identity_proof = request.params[4].get_str();

            // Decode public key
            CPubKey publickey;
            if (!publickey.SetHex(publickey_str)) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid public key format");
            }

            if (!publickey.IsValid()) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid public key");
            }

            // Create user registration
            OfficialUser new_user;
            new_user.public_key = publickey;
            new_user.government_id_hash = Hash(government_id).GetHex();
            new_user.birth_currency = birth_currency;
            new_user.country_code = country_code;
            new_user.identity_proof_hash = identity_proof;
            new_user.status = UserStatus::PENDING_VERIFICATION;
            new_user.registration_height = 0; // Will be set when processed

            // Submit registration
            std::string error_message;
            if (!g_user_consensus.RegisterUser(new_user, error_message)) {
                throw JSONRPCError(RPC_MISC_ERROR, strprintf("User registration failed: %s", error_message));
            }

            UniValue result(UniValue::VOBJ);
            result.pushKV("user_id", publickey.GetHex());
            result.pushKV("status", "pending_verification");
            result.pushKV("message", "User registration submitted successfully. Awaiting endorsements.");
            result.pushKV("registration_height", 0);

            return result;
        }
    };
}

static RPCHelpMan submitendorsement() {
    return RPCHelpMan{
        "submitendorsement",
        "\nSubmit an endorsement for a user verification.\n",
        {
            {"endorser_key", RPCArg::Type::STR, RPCArg::Optional::NO, "Endorser's public key in hex format"},
            {"endorsed_user", RPCArg::Type::STR, RPCArg::Optional::NO, "User being endorsed (public key)"},
            {"verification_method", RPCArg::Type::STR, RPCArg::Optional::NO, "Verification method used"},
            {"confidence_level", RPCArg::Type::STR, RPCArg::Optional::NO, "Confidence level (high, medium, low)"},
            {"verification_notes", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Optional verification notes"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "endorsement_id", "Unique endorsement ID"},
                {RPCResult::Type::STR, "status", "Endorsement status"},
                {RPCResult::Type::STR, "message", "Status message"},
            }
        },
        RPCExamples{
            HelpExampleCli("submitendorsement", "\"02f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f9\" \"02a1b2c3...\" \"government_id\" \"high\" \"Verified government ID\"")
            + HelpExampleRpc("submitendorsement", "\"02f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f9\", \"02a1b2c3...\", \"government_id\", \"high\", \"Verified government ID\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue {
            if (g_o_integration == nullptr) {
                throw JSONRPCError(RPC_MISC_ERROR, "O blockchain integration not available");
            }

            // Parse parameters
            std::string endorser_key_str = request.params[0].get_str();
            std::string endorsed_user_str = request.params[1].get_str();
            std::string verification_method = request.params[2].get_str();
            std::string confidence_level = request.params[3].get_str();
            std::string verification_notes = request.params.size() > 4 ? request.params[4].get_str() : "";

            // Decode public keys
            CPubKey endorser_key, endorsed_user;
            if (!endorser_key.SetHex(endorser_key_str) || !endorsed_user.SetHex(endorsed_user_str)) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid public key format");
            }

            // Parse verification method
            VerificationMethod method;
            if (verification_method == "government_id") {
                method = VerificationMethod::GOVERNMENT_ID;
            } else if (verification_method == "video_call") {
                method = VerificationMethod::VIDEO_CALL;
            } else if (verification_method == "document_review") {
                method = VerificationMethod::DOCUMENT_REVIEW;
            } else if (verification_method == "biometric_verification") {
                method = VerificationMethod::BIOMETRIC_VERIFICATION;
            } else if (verification_method == "third_party_kyc") {
                method = VerificationMethod::THIRD_PARTY_KYC;
            } else {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid verification method");
            }

            // Parse confidence level
            ConfidenceLevel confidence;
            if (confidence_level == "high") {
                confidence = ConfidenceLevel::HIGH;
            } else if (confidence_level == "medium") {
                confidence = ConfidenceLevel::MEDIUM;
            } else if (confidence_level == "low") {
                confidence = ConfidenceLevel::LOW;
            } else {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid confidence level");
            }

            // Create endorsement record
            EndorsementRecord endorsement;
            endorsement.endorsement_id = Hash(endorser_key_str + endorsed_user_str + std::to_string(GetTime())).GetUint64(0);
            endorsement.endorser = endorser_key;
            endorsement.endorsed_user = endorsed_user;
            endorsement.verification_method = method;
            endorsement.confidence_level = confidence;
            endorsement.verification_notes = verification_notes;
            endorsement.timestamp = GetTime();
            endorsement.block_height = 0; // Will be set when processed

            // Submit endorsement
            std::string error_message;
            if (!g_user_consensus.SubmitEndorsement(endorsement, error_message)) {
                throw JSONRPCError(RPC_MISC_ERROR, strprintf("Endorsement submission failed: %s", error_message));
            }

            UniValue result(UniValue::VOBJ);
            result.pushKV("endorsement_id", strprintf("%016x", endorsement.endorsement_id.GetUint64(0)));
            result.pushKV("status", "submitted");
            result.pushKV("message", "Endorsement submitted successfully");

            return result;
        }
    };
}

static RPCHelpMan getuserstatus() {
    return RPCHelpMan{
        "getuserstatus",
        "\nGet the verification status of a user.\n",
        {
            {"publickey", RPCArg::Type::STR, RPCArg::Optional::NO, "User's public key in hex format"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "user_id", "User's public key"},
                {RPCResult::Type::STR, "status", "Verification status"},
                {RPCResult::Type::NUM, "reputation_score", "User's reputation score"},
                {RPCResult::Type::NUM, "endorsement_count", "Number of endorsements received"},
                {RPCResult::Type::NUM, "registration_height", "Block height when registered"},
            }
        },
        RPCExamples{
            HelpExampleCli("getuserstatus", "\"02f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f9\"")
            + HelpExampleRpc("getuserstatus", "\"02f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f9\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue {
            if (g_o_integration == nullptr) {
                throw JSONRPCError(RPC_MISC_ERROR, "O blockchain integration not available");
            }

            std::string publickey_str = request.params[0].get_str();
            CPubKey publickey;
            if (!publickey.SetHex(publickey_str)) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid public key format");
            }

            UserStatus status = g_o_integration->GetUserVerificationStatus(publickey);
            double reputation = g_user_consensus.GetReputationScore(publickey);

            UniValue result(UniValue::VOBJ);
            result.pushKV("user_id", publickey.GetHex());
            
            switch (status) {
                case UserStatus::VERIFIED:
                    result.pushKV("status", "verified");
                    break;
                case UserStatus::PENDING_VERIFICATION:
                    result.pushKV("status", "pending_verification");
                    break;
                case UserStatus::VERIFICATION_IN_PROGRESS:
                    result.pushKV("status", "verification_in_progress");
                    break;
                case UserStatus::SUSPENDED:
                    result.pushKV("status", "suspended");
                    break;
                case UserStatus::BLACKLISTED:
                    result.pushKV("status", "blacklisted");
                    break;
                default:
                    result.pushKV("status", "unknown");
                    break;
            }
            
            result.pushKV("reputation_score", reputation);
            result.pushKV("endorsement_count", 0); // Would need to implement in user consensus
            result.pushKV("registration_height", 0); // Would need to implement in user consensus

            return result;
        }
    };
}

static RPCHelpMan listverifiedusers() {
    return RPCHelpMan{
        "listverifiedusers",
        "\nList all verified users in the system.\n",
        {
            {"limit", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "Maximum number of users to return (default: 100)"},
            {"offset", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "Number of users to skip (default: 0)"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::ARR, "users", "Array of verified users",
                {
                    {RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::STR, "public_key", "User's public key"},
                        {RPCResult::Type::NUM, "reputation_score", "User's reputation score"},
                        {RPCResult::Type::STR, "country_code", "User's country code"},
                        {RPCResult::Type::STR, "birth_currency", "User's birth currency"},
                    }}},
                }},
                {RPCResult::Type::NUM, "total_count", "Total number of verified users"},
                {RPCResult::Type::NUM, "returned_count", "Number of users returned"},
            }
        },
        RPCExamples{
            HelpExampleCli("listverifiedusers", "50 0")
            + HelpExampleRpc("listverifiedusers", "50, 0")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue {
            if (g_o_integration == nullptr) {
                throw JSONRPCError(RPC_MISC_ERROR, "O blockchain integration not available");
            }

            int limit = 100;
            int offset = 0;
            
            if (request.params.size() > 0) {
                limit = request.params[0].getInt<int>();
            }
            if (request.params.size() > 1) {
                offset = request.params[1].getInt<int>();
            }

            std::vector<CPubKey> verified_users = g_user_consensus.GetVerifiedUsers();
            
            UniValue users(UniValue::VARR);
            int start = std::min(offset, static_cast<int>(verified_users.size()));
            int end = std::min(start + limit, static_cast<int>(verified_users.size()));
            
            for (int i = start; i < end; ++i) {
                const CPubKey& user_key = verified_users[i];
                double reputation = g_user_consensus.GetReputationScore(user_key);
                
                UniValue user(UniValue::VOBJ);
                user.pushKV("public_key", user_key.GetHex());
                user.pushKV("reputation_score", reputation);
                user.pushKV("country_code", ""); // Would need to implement in user consensus
                user.pushKV("birth_currency", ""); // Would need to implement in user consensus
                
                users.push_back(user);
            }
            
            UniValue result(UniValue::VOBJ);
            result.pushKV("users", users);
            result.pushKV("total_count", static_cast<int>(verified_users.size()));
            result.pushKV("returned_count", end - start);

            return result;
        }
    };
}

static RPCHelpMan getuserstats() {
    return RPCHelpMan{
        "getuserstats",
        "\nGet statistics about users in the O blockchain system.\n",
        {},
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::NUM, "total_registered", "Total number of registered users"},
                {RPCResult::Type::NUM, "total_verified", "Total number of verified users"},
                {RPCResult::Type::NUM, "total_pending", "Total number of pending users"},
                {RPCResult::Type::NUM, "total_suspended", "Total number of suspended users"},
                {RPCResult::Type::NUM, "total_blacklisted", "Total number of blacklisted users"},
                {RPCResult::Type::NUM, "average_reputation", "Average reputation score"},
                {RPCResult::Type::NUM, "active_challenges", "Number of active challenges"},
            }
        },
        RPCExamples{
            HelpExampleCli("getuserstats", "")
            + HelpExampleRpc("getuserstats", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue {
            if (g_o_integration == nullptr) {
                throw JSONRPCError(RPC_MISC_ERROR, "O blockchain integration not available");
            }

            UserRegistryConsensus::UserStats stats = g_o_integration->GetUserBlockchainStats();
            
            UniValue result(UniValue::VOBJ);
            result.pushKV("total_registered", static_cast<int>(stats.total_registered));
            result.pushKV("total_verified", static_cast<int>(stats.total_verified));
            result.pushKV("total_pending", static_cast<int>(stats.total_pending));
            result.pushKV("total_suspended", static_cast<int>(stats.total_suspended));
            result.pushKV("total_blacklisted", static_cast<int>(stats.total_blacklisted));
            result.pushKV("average_reputation", stats.average_reputation);
            result.pushKV("active_challenges", static_cast<int>(stats.active_challenges));

            return result;
        }
    };
}

// Register RPC methods
void RegisterOBlockchainRPCCommands(CRPCTable& t) {
    static const CRPCCommand commands[] = {
        {"o", &registeruser},
        {"o", &submitendorsement},
        {"o", &getuserstatus},
        {"o", &listverifiedusers},
        {"o", &getuserstats},
    };
    
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}
