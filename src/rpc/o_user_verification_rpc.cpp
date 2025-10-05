// Copyright (c) 2024 The Bitcoin Core developers
// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/o_user_verification_rpc.h>
#include <rpc/server.h>
#include <consensus/user_consensus.h>
#include <consensus/geographic_access_control.h>
#include <logging.h>

namespace OUserVerificationRPC {

static RPCHelpMan getrecommendedverificationmethod()
{
    return RPCHelpMan{
        "getrecommendedverificationmethod",
        "\nGet the recommended verification method for a specific country.\n",
        {
            {"country_code", RPCArg::Type::STR, RPCArg::Optional::NO, "ISO 3166-1 alpha-2 country code (e.g., 'US', 'CH', 'SG')"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "country_code", "The country code provided"},
                {RPCResult::Type::STR, "country_name", "Full country name"},
                {RPCResult::Type::STR, "recommended_method", "Recommended verification method"},
                {RPCResult::Type::STR, "method_description", "Description of the verification method"},
                {RPCResult::Type::BOOL, "requires_kyc", "Whether this country requires KYC"},
                {RPCResult::Type::STR, "reasoning", "Explanation for the recommendation"},
            }
        },
        RPCExamples{
            HelpExampleCli("getrecommendedverificationmethod", "US")
            + HelpExampleCli("getrecommendedverificationmethod", "CH")
            + HelpExampleRpc("getrecommendedverificationmethod", "SG")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            if (request.params.size() != 1) {
                throw std::runtime_error(self.ToString());
            }

            std::string country_code = request.params[0].get_str();
            
            // Validate country code format
            if (country_code.length() != 2) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Country code must be 2 characters (ISO 3166-1 alpha-2)");
            }
            
            // Convert to uppercase
            std::transform(country_code.begin(), country_code.end(), country_code.begin(), ::toupper);
            
            // Get recommended verification method
            VerificationMethod recommended_method = g_user_consensus.GetRecommendedVerificationMethod(country_code);
            
            // Get country policy
            auto policy = g_geographic_access_control.GetJurisdictionPolicy(country_code);
            std::string country_name = policy.has_value() ? policy->country_name : "Unknown";
            bool requires_kyc = g_geographic_access_control.DoesCountryRequireKYC(country_code);
            
            // Convert verification method to string
            std::string method_name;
            std::string method_description;
            std::string reasoning;
            
            switch (recommended_method) {
                case VerificationMethod::THIRD_PARTY_KYC:
                    method_name = "THIRD_PARTY_KYC";
                    method_description = "Third-party KYC verification service";
                    reasoning = "This country requires KYC compliance for financial services";
                    break;
                case VerificationMethod::GOVERNMENT_ID:
                    method_name = "GOVERNMENT_ID";
                    method_description = "Government-issued ID verification";
                    reasoning = "This country allows privacy-preserving verification methods";
                    break;
                case VerificationMethod::VIDEO_CALL:
                    method_name = "VIDEO_CALL";
                    method_description = "Video call verification";
                    reasoning = "This country allows privacy-preserving verification methods";
                    break;
                case VerificationMethod::DOCUMENT_REVIEW:
                    method_name = "DOCUMENT_REVIEW";
                    method_description = "Document review verification";
                    reasoning = "This country allows privacy-preserving verification methods";
                    break;
                case VerificationMethod::BIOMETRIC_VERIFICATION:
                    method_name = "BIOMETRIC_VERIFICATION";
                    method_description = "Biometric verification";
                    reasoning = "This country allows privacy-preserving verification methods";
                    break;
                default:
                    method_name = "UNKNOWN";
                    method_description = "Unknown verification method";
                    reasoning = "Unknown verification method";
                    break;
            }
            
            UniValue result(UniValue::VOBJ);
            result.pushKV("country_code", country_code);
            result.pushKV("country_name", country_name);
            result.pushKV("recommended_method", method_name);
            result.pushKV("method_description", method_description);
            result.pushKV("requires_kyc", requires_kyc);
            result.pushKV("reasoning", reasoning);
            
            return result;
        }
    };
}

static RPCHelpMan getallowedverificationmethods()
{
    return RPCHelpMan{
        "getallowedverificationmethods",
        "\nGet all allowed verification methods for a specific country.\n",
        {
            {"country_code", RPCArg::Type::STR, RPCArg::Optional::NO, "ISO 3166-1 alpha-2 country code (e.g., 'US', 'CH', 'SG')"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "country_code", "The country code provided"},
                {RPCResult::Type::STR, "country_name", "Full country name"},
                {RPCResult::Type::ARR, "allowed_methods", "List of allowed verification methods",
                {
                    {RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::STR, "method", "Verification method name"},
                        {RPCResult::Type::STR, "description", "Description of the method"},
                        {RPCResult::Type::BOOL, "is_kyc", "Whether this is a KYC method"},
                        {RPCResult::Type::BOOL, "is_privacy_preserving", "Whether this method preserves privacy"},
                    }},
                }},
                {RPCResult::Type::BOOL, "requires_kyc", "Whether this country requires KYC"},
                {RPCResult::Type::STR, "access_level", "Access level for this country"},
            }
        },
        RPCExamples{
            HelpExampleCli("getallowedverificationmethods", "US")
            + HelpExampleCli("getallowedverificationmethods", "CH")
            + HelpExampleRpc("getallowedverificationmethods", "SG")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            if (request.params.size() != 1) {
                throw std::runtime_error(self.ToString());
            }

            std::string country_code = request.params[0].get_str();
            
            // Validate country code format
            if (country_code.length() != 2) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Country code must be 2 characters (ISO 3166-1 alpha-2)");
            }
            
            // Convert to uppercase
            std::transform(country_code.begin(), country_code.end(), country_code.begin(), ::toupper);
            
            // Get allowed verification methods
            std::vector<VerificationMethod> allowed_methods = g_user_consensus.GetAllowedVerificationMethods(country_code);
            
            // Get country policy
            auto policy = g_geographic_access_control.GetJurisdictionPolicy(country_code);
            std::string country_name = policy.has_value() ? policy->country_name : "Unknown";
            bool requires_kyc = g_geographic_access_control.DoesCountryRequireKYC(country_code);
            
            // Convert access level to string
            std::string access_level = "UNKNOWN";
            if (policy.has_value()) {
                switch (policy->access_level) {
                    case AccessLevel::ALLOWED: access_level = "ALLOWED"; break;
                    case AccessLevel::RESTRICTED: access_level = "RESTRICTED"; break;
                    case AccessLevel::BLOCKED: access_level = "BLOCKED"; break;
                    case AccessLevel::MONITORED: access_level = "MONITORED"; break;
                }
            }
            
            UniValue methods_array(UniValue::VARR);
            for (const auto& method : allowed_methods) {
                UniValue method_obj(UniValue::VOBJ);
                
                std::string method_name;
                std::string method_description;
                bool is_kyc = false;
                bool is_privacy_preserving = false;
                
                switch (method) {
                    case VerificationMethod::THIRD_PARTY_KYC:
                        method_name = "THIRD_PARTY_KYC";
                        method_description = "Third-party KYC verification service";
                        is_kyc = true;
                        is_privacy_preserving = false;
                        break;
                    case VerificationMethod::GOVERNMENT_ID:
                        method_name = "GOVERNMENT_ID";
                        method_description = "Government-issued ID verification";
                        is_kyc = false;
                        is_privacy_preserving = true;
                        break;
                    case VerificationMethod::VIDEO_CALL:
                        method_name = "VIDEO_CALL";
                        method_description = "Video call verification";
                        is_kyc = false;
                        is_privacy_preserving = true;
                        break;
                    case VerificationMethod::DOCUMENT_REVIEW:
                        method_name = "DOCUMENT_REVIEW";
                        method_description = "Document review verification";
                        is_kyc = false;
                        is_privacy_preserving = true;
                        break;
                    case VerificationMethod::BIOMETRIC_VERIFICATION:
                        method_name = "BIOMETRIC_VERIFICATION";
                        method_description = "Biometric verification";
                        is_kyc = false;
                        is_privacy_preserving = true;
                        break;
                }
                
                method_obj.pushKV("method", method_name);
                method_obj.pushKV("description", method_description);
                method_obj.pushKV("is_kyc", is_kyc);
                method_obj.pushKV("is_privacy_preserving", is_privacy_preserving);
                
                methods_array.push_back(method_obj);
            }
            
            UniValue result(UniValue::VOBJ);
            result.pushKV("country_code", country_code);
            result.pushKV("country_name", country_name);
            result.pushKV("allowed_methods", methods_array);
            result.pushKV("requires_kyc", requires_kyc);
            result.pushKV("access_level", access_level);
            
            return result;
        }
    };
}

static RPCHelpMan doescountryrequirekyc()
{
    return RPCHelpMan{
        "doescountryrequirekyc",
        "\nCheck if a specific country requires KYC verification.\n",
        {
            {"country_code", RPCArg::Type::STR, RPCArg::Optional::NO, "ISO 3166-1 alpha-2 country code (e.g., 'US', 'CH', 'SG')"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "country_code", "The country code provided"},
                {RPCResult::Type::STR, "country_name", "Full country name"},
                {RPCResult::Type::BOOL, "requires_kyc", "Whether this country requires KYC"},
                {RPCResult::Type::STR, "compliance_level", "Compliance level required"},
                {RPCResult::Type::STR, "regulatory_authority", "Regulatory authority for this country"},
                {RPCResult::Type::STR, "compliance_requirements", "Specific compliance requirements"},
            }
        },
        RPCExamples{
            HelpExampleCli("doescountryrequirekyc", "US")
            + HelpExampleCli("doescountryrequirekyc", "CH")
            + HelpExampleRpc("doescountryrequirekyc", "SG")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            if (request.params.size() != 1) {
                throw std::runtime_error(self.ToString());
            }

            std::string country_code = request.params[0].get_str();
            
            // Validate country code format
            if (country_code.length() != 2) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Country code must be 2 characters (ISO 3166-1 alpha-2)");
            }
            
            // Convert to uppercase
            std::transform(country_code.begin(), country_code.end(), country_code.begin(), ::toupper);
            
            // Get country policy
            auto policy = g_geographic_access_control.GetJurisdictionPolicy(country_code);
            
            if (!policy.has_value()) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Country code not found: " + country_code);
            }
            
            // Convert compliance level to string
            std::string compliance_level;
            switch (policy->compliance_level) {
                case ComplianceLevel::NONE: compliance_level = "NONE"; break;
                case ComplianceLevel::BASIC: compliance_level = "BASIC"; break;
                case ComplianceLevel::STANDARD: compliance_level = "STANDARD"; break;
                case ComplianceLevel::FULL: compliance_level = "FULL"; break;
            }
            
            UniValue result(UniValue::VOBJ);
            result.pushKV("country_code", country_code);
            result.pushKV("country_name", policy->country_name);
            result.pushKV("requires_kyc", policy->requires_kyc);
            result.pushKV("compliance_level", compliance_level);
            result.pushKV("regulatory_authority", policy->regulatory_authority);
            result.pushKV("compliance_requirements", policy->compliance_requirements);
            
            return result;
        }
    };
}

} // namespace OUserVerificationRPC

void RegisterOUserVerificationRPCCommands(CRPCTable& t)
{
    static const CRPCCommand commands[] = {
        {"o_user_verification", &OUserVerificationRPC::getrecommendedverificationmethod},
        {"o_user_verification", &OUserVerificationRPC::getallowedverificationmethods},
        {"o_user_verification", &OUserVerificationRPC::doescountryrequirekyc},
    };
    
    for (const auto& c : commands) {
        t.appendCommand(c.category, &c);
    }
}
