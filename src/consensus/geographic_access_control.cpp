// Copyright (c) 2024 The Bitcoin Core developers
// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/geographic_access_control.h>
#include <util/strencodings.h>
#include <logging.h>
#include <random.h>
#include <hash.h>
#include <cstring>
#include <fstream>
#include <sstream>

// Global instance
GeographicAccessControl g_geographic_access_control;

GeographicAccessControl::GeographicAccessControl()
    : m_default_access_level(AccessLevel::BLOCKED),
      m_default_compliance_level(ComplianceLevel::FULL),
      m_ip_geolocation_service(),
      m_compliance_reporting_enabled(false),
      m_compliance_reporting_threshold(0)
{
    // Initialize statistics
    m_access_stats = AccessStatistics{};
    m_jurisdiction_stats = JurisdictionStatistics{};
}

GeographicAccessControl::~GeographicAccessControl()
{
    // Cleanup if needed
}

bool GeographicAccessControl::Initialize()
{
    LogPrintf("GeographicAccessControl: Initializing with default jurisdiction policies\n");
    
    // Load default jurisdiction policies
    LoadDefaultJurisdictionPolicies();
    
    // Update statistics
    UpdateStatistics();
    
    LogPrintf("GeographicAccessControl: Initialized with %d jurisdiction policies\n", 
              static_cast<int>(m_jurisdiction_policies.size()));
    
    return true;
}

void GeographicAccessControl::LoadDefaultJurisdictionPolicies()
{
    // Phase 1: Crypto-Friendly Jurisdictions (Full Access)
    
    // Switzerland - Full access with privacy
    JurisdictionPolicy switzerland;
    switzerland.country_code = "CH";
    switzerland.country_name = "Switzerland";
    switzerland.access_level = AccessLevel::ALLOWED;
    switzerland.compliance_level = ComplianceLevel::BASIC;
    switzerland.allowed_features = {"privacy", "anonymous", "brightid", "measurement", "stabilization", "exchange"};
    switzerland.restricted_features = {};
    switzerland.requires_kyc = false;
    switzerland.allows_privacy = true;
    switzerland.allows_anonymous = true;
    switzerland.daily_transaction_limit = OAmount::O(100000); // 1,000,000.00 O
    switzerland.monthly_transaction_limit = OAmount::O(1000000); // 10,000,000.00 O
    switzerland.lifetime_transaction_limit = 0; // Unlimited
    switzerland.max_balance_limit = 0; // Unlimited
    switzerland.requires_reporting = false;
    switzerland.regulatory_authority = "FINMA";
    switzerland.compliance_requirements = "Basic AML compliance";
    switzerland.last_updated = GetTime();
    m_jurisdiction_policies["CH"] = switzerland;
    
    // Singapore - Full access with privacy
    JurisdictionPolicy singapore;
    singapore.country_code = "SG";
    singapore.country_name = "Singapore";
    singapore.access_level = AccessLevel::ALLOWED;
    singapore.compliance_level = ComplianceLevel::BASIC;
    singapore.allowed_features = {"privacy", "anonymous", "brightid", "measurement", "stabilization", "exchange"};
    singapore.restricted_features = {};
    singapore.requires_kyc = false;
    singapore.allows_privacy = true;
    singapore.allows_anonymous = true;
    singapore.daily_transaction_limit = OAmount::O(100000); // 1,000,000.00 O
    singapore.monthly_transaction_limit = OAmount::O(1000000); // 10,000,000.00 O
    singapore.lifetime_transaction_limit = 0; // Unlimited
    singapore.max_balance_limit = 0; // Unlimited
    singapore.requires_reporting = false;
    singapore.regulatory_authority = "MAS";
    singapore.compliance_requirements = "Basic AML compliance";
    singapore.last_updated = GetTime();
    m_jurisdiction_policies["SG"] = singapore;
    
    // UAE - Full access with privacy
    JurisdictionPolicy uae;
    uae.country_code = "AE";
    uae.country_name = "United Arab Emirates";
    uae.access_level = AccessLevel::ALLOWED;
    uae.compliance_level = ComplianceLevel::BASIC;
    uae.allowed_features = {"privacy", "anonymous", "brightid", "measurement", "stabilization", "exchange"};
    uae.restricted_features = {};
    uae.requires_kyc = false;
    uae.allows_privacy = true;
    uae.allows_anonymous = true;
    uae.daily_transaction_limit = OAmount::O(100000); // 1,000,000.00 O
    uae.monthly_transaction_limit = OAmount::O(1000000); // 10,000,000.00 O
    uae.lifetime_transaction_limit = 0; // Unlimited
    uae.max_balance_limit = 0; // Unlimited
    uae.requires_reporting = false;
    uae.regulatory_authority = "VARA";
    uae.compliance_requirements = "Basic AML compliance";
    uae.last_updated = GetTime();
    m_jurisdiction_policies["AE"] = uae;
    
    // Portugal - Full access with privacy
    JurisdictionPolicy portugal;
    portugal.country_code = "PT";
    portugal.country_name = "Portugal";
    portugal.access_level = AccessLevel::ALLOWED;
    portugal.compliance_level = ComplianceLevel::BASIC;
    portugal.allowed_features = {"privacy", "anonymous", "brightid", "measurement", "stabilization", "exchange"};
    portugal.restricted_features = {};
    portugal.requires_kyc = false;
    portugal.allows_privacy = true;
    portugal.allows_anonymous = true;
    portugal.daily_transaction_limit = OAmount::O(100000); // 1,000,000.00 O
    portugal.monthly_transaction_limit = OAmount::O(1000000); // 10,000,000.00 O
    portugal.lifetime_transaction_limit = 0; // Unlimited
    portugal.max_balance_limit = 0; // Unlimited
    portugal.requires_reporting = false;
    portugal.regulatory_authority = "Banco de Portugal";
    portugal.compliance_requirements = "Basic EU compliance";
    portugal.last_updated = GetTime();
    m_jurisdiction_policies["PT"] = portugal;
    
    // Germany - Full access with privacy
    JurisdictionPolicy germany;
    germany.country_code = "DE";
    germany.country_name = "Germany";
    germany.access_level = AccessLevel::ALLOWED;
    germany.compliance_level = ComplianceLevel::BASIC;
    germany.allowed_features = {"privacy", "anonymous", "brightid", "measurement", "stabilization", "exchange"};
    germany.restricted_features = {};
    germany.requires_kyc = false;
    germany.allows_privacy = true;
    germany.allows_anonymous = true;
    germany.daily_transaction_limit = OAmount::O(100000); // 1,000,000.00 O
    germany.monthly_transaction_limit = OAmount::O(1000000); // 10,000,000.00 O
    germany.lifetime_transaction_limit = 0; // Unlimited
    germany.max_balance_limit = 0; // Unlimited
    germany.requires_reporting = false;
    germany.regulatory_authority = "BaFin";
    germany.compliance_requirements = "Basic EU compliance";
    germany.last_updated = GetTime();
    m_jurisdiction_policies["DE"] = germany;
    
    // Japan - Full access with privacy
    JurisdictionPolicy japan;
    japan.country_code = "JP";
    japan.country_name = "Japan";
    japan.access_level = AccessLevel::ALLOWED;
    japan.compliance_level = ComplianceLevel::BASIC;
    japan.allowed_features = {"privacy", "anonymous", "brightid", "measurement", "stabilization", "exchange"};
    japan.restricted_features = {};
    japan.requires_kyc = false;
    japan.allows_privacy = true;
    japan.allows_anonymous = true;
    japan.daily_transaction_limit = OAmount::O(100000); // 1,000,000.00 O
    japan.monthly_transaction_limit = OAmount::O(1000000); // 10,000,000.00 O
    japan.lifetime_transaction_limit = 0; // Unlimited
    japan.max_balance_limit = 0; // Unlimited
    japan.requires_reporting = false;
    japan.regulatory_authority = "FSA";
    japan.compliance_requirements = "Basic AML compliance";
    japan.last_updated = GetTime();
    m_jurisdiction_policies["JP"] = japan;
    
    // South Korea - Full access with privacy
    JurisdictionPolicy south_korea;
    south_korea.country_code = "KR";
    south_korea.country_name = "South Korea";
    south_korea.access_level = AccessLevel::ALLOWED;
    south_korea.compliance_level = ComplianceLevel::BASIC;
    south_korea.allowed_features = {"privacy", "anonymous", "brightid", "measurement", "stabilization", "exchange"};
    south_korea.restricted_features = {};
    south_korea.requires_kyc = false;
    south_korea.allows_privacy = true;
    south_korea.allows_anonymous = true;
    south_korea.daily_transaction_limit = OAmount::O(100000); // 1,000,000.00 O
    south_korea.monthly_transaction_limit = OAmount::O(1000000); // 10,000,000.00 O
    south_korea.lifetime_transaction_limit = 0; // Unlimited
    south_korea.max_balance_limit = 0; // Unlimited
    south_korea.requires_reporting = false;
    south_korea.regulatory_authority = "FSC";
    south_korea.compliance_requirements = "Basic AML compliance";
    south_korea.last_updated = GetTime();
    m_jurisdiction_policies["KR"] = south_korea;
    
    // Australia - Full access with privacy
    JurisdictionPolicy australia;
    australia.country_code = "AU";
    australia.country_name = "Australia";
    australia.access_level = AccessLevel::ALLOWED;
    australia.compliance_level = ComplianceLevel::BASIC;
    australia.allowed_features = {"privacy", "anonymous", "brightid", "measurement", "stabilization", "exchange"};
    australia.restricted_features = {};
    australia.requires_kyc = false;
    australia.allows_privacy = true;
    australia.allows_anonymous = true;
    australia.daily_transaction_limit = OAmount::O(100000); // 1,000,000.00 O
    australia.monthly_transaction_limit = OAmount::O(1000000); // 10,000,000.00 O
    australia.lifetime_transaction_limit = 0; // Unlimited
    australia.max_balance_limit = 0; // Unlimited
    australia.requires_reporting = false;
    australia.regulatory_authority = "AUSTRAC";
    australia.compliance_requirements = "Basic AML compliance";
    australia.last_updated = GetTime();
    m_jurisdiction_policies["AU"] = australia;
    
    // Canada - Full access with privacy
    JurisdictionPolicy canada;
    canada.country_code = "CA";
    canada.country_name = "Canada";
    canada.access_level = AccessLevel::ALLOWED;
    canada.compliance_level = ComplianceLevel::BASIC;
    canada.allowed_features = {"privacy", "anonymous", "brightid", "measurement", "stabilization", "exchange"};
    canada.restricted_features = {};
    canada.requires_kyc = false;
    canada.allows_privacy = true;
    canada.allows_anonymous = true;
    canada.daily_transaction_limit = OAmount::O(100000); // 1,000,000.00 O
    canada.monthly_transaction_limit = OAmount::O(1000000); // 10,000,000.00 O
    canada.lifetime_transaction_limit = 0; // Unlimited
    canada.max_balance_limit = 0; // Unlimited
    canada.requires_reporting = false;
    canada.regulatory_authority = "FINTRAC";
    canada.compliance_requirements = "Basic AML compliance";
    canada.last_updated = GetTime();
    m_jurisdiction_policies["CA"] = canada;
    
    // Phase 1: Blocked Jurisdictions (No Access)
    
    // China - Blocked
    JurisdictionPolicy china;
    china.country_code = "CN";
    china.country_name = "China";
    china.access_level = AccessLevel::BLOCKED;
    china.compliance_level = ComplianceLevel::FULL;
    china.allowed_features = {};
    china.restricted_features = {"all"};
    china.requires_kyc = true;
    china.allows_privacy = false;
    china.allows_anonymous = false;
    china.daily_transaction_limit = 0;
    china.monthly_transaction_limit = 0;
    china.lifetime_transaction_limit = 0;
    china.max_balance_limit = 0;
    china.requires_reporting = true;
    china.regulatory_authority = "PBOC";
    china.compliance_requirements = "Complete crypto ban";
    china.last_updated = GetTime();
    m_jurisdiction_policies["CN"] = china;
    
    // India - Blocked
    JurisdictionPolicy india;
    india.country_code = "IN";
    india.country_name = "India";
    india.access_level = AccessLevel::BLOCKED;
    india.compliance_level = ComplianceLevel::FULL;
    india.allowed_features = {};
    india.restricted_features = {"all"};
    india.requires_kyc = true;
    india.allows_privacy = false;
    india.allows_anonymous = false;
    india.daily_transaction_limit = 0;
    india.monthly_transaction_limit = 0;
    india.lifetime_transaction_limit = 0;
    india.max_balance_limit = 0;
    india.requires_reporting = true;
    india.regulatory_authority = "RBI";
    india.compliance_requirements = "High taxes, regulatory uncertainty";
    india.last_updated = GetTime();
    m_jurisdiction_policies["IN"] = india;
    
    // Bangladesh - Blocked
    JurisdictionPolicy bangladesh;
    bangladesh.country_code = "BD";
    bangladesh.country_name = "Bangladesh";
    bangladesh.access_level = AccessLevel::BLOCKED;
    bangladesh.compliance_level = ComplianceLevel::FULL;
    bangladesh.allowed_features = {};
    bangladesh.restricted_features = {"all"};
    bangladesh.requires_kyc = true;
    bangladesh.allows_privacy = false;
    bangladesh.allows_anonymous = false;
    bangladesh.daily_transaction_limit = 0;
    bangladesh.monthly_transaction_limit = 0;
    bangladesh.lifetime_transaction_limit = 0;
    bangladesh.max_balance_limit = 0;
    bangladesh.requires_reporting = true;
    bangladesh.regulatory_authority = "Bangladesh Bank";
    bangladesh.compliance_requirements = "Crypto ban";
    bangladesh.last_updated = GetTime();
    m_jurisdiction_policies["BD"] = bangladesh;
    
    // Nepal - Blocked
    JurisdictionPolicy nepal;
    nepal.country_code = "NP";
    nepal.country_name = "Nepal";
    nepal.access_level = AccessLevel::BLOCKED;
    nepal.compliance_level = ComplianceLevel::FULL;
    nepal.allowed_features = {};
    nepal.restricted_features = {"all"};
    nepal.requires_kyc = true;
    nepal.allows_privacy = false;
    nepal.allows_anonymous = false;
    nepal.daily_transaction_limit = 0;
    nepal.monthly_transaction_limit = 0;
    nepal.lifetime_transaction_limit = 0;
    nepal.max_balance_limit = 0;
    nepal.requires_reporting = true;
    nepal.regulatory_authority = "Nepal Rastra Bank";
    nepal.compliance_requirements = "Crypto ban";
    nepal.last_updated = GetTime();
    m_jurisdiction_policies["NP"] = nepal;
    
    // Bolivia - Blocked
    JurisdictionPolicy bolivia;
    bolivia.country_code = "BO";
    bolivia.country_name = "Bolivia";
    bolivia.access_level = AccessLevel::BLOCKED;
    bolivia.compliance_level = ComplianceLevel::FULL;
    bolivia.allowed_features = {};
    bolivia.restricted_features = {"all"};
    bolivia.requires_kyc = true;
    bolivia.allows_privacy = false;
    bolivia.allows_anonymous = false;
    bolivia.daily_transaction_limit = 0;
    bolivia.monthly_transaction_limit = 0;
    bolivia.lifetime_transaction_limit = 0;
    bolivia.max_balance_limit = 0;
    bolivia.requires_reporting = true;
    bolivia.regulatory_authority = "Banco Central de Bolivia";
    bolivia.compliance_requirements = "Crypto ban";
    bolivia.last_updated = GetTime();
    m_jurisdiction_policies["BO"] = bolivia;
    
    // Ecuador - Blocked
    JurisdictionPolicy ecuador;
    ecuador.country_code = "EC";
    ecuador.country_name = "Ecuador";
    ecuador.access_level = AccessLevel::BLOCKED;
    ecuador.compliance_level = ComplianceLevel::FULL;
    ecuador.allowed_features = {};
    ecuador.restricted_features = {"all"};
    ecuador.requires_kyc = true;
    ecuador.allows_privacy = false;
    ecuador.allows_anonymous = false;
    ecuador.daily_transaction_limit = 0;
    ecuador.monthly_transaction_limit = 0;
    ecuador.lifetime_transaction_limit = 0;
    ecuador.max_balance_limit = 0;
    ecuador.requires_reporting = true;
    ecuador.regulatory_authority = "Banco Central del Ecuador";
    ecuador.compliance_requirements = "Crypto ban";
    ecuador.last_updated = GetTime();
    m_jurisdiction_policies["EC"] = ecuador;
    
    // Phase 2: High-Risk Jurisdictions (Monitor)
    
    // United States - Monitored
    JurisdictionPolicy usa;
    usa.country_code = "US";
    usa.country_name = "United States";
    usa.access_level = AccessLevel::MONITORED;
    usa.compliance_level = ComplianceLevel::FULL;
    usa.allowed_features = {"measurement", "stabilization", "exchange"};
    usa.restricted_features = {"privacy", "anonymous", "brightid"};
    usa.requires_kyc = true;
    usa.allows_privacy = false;
    usa.allows_anonymous = false;
    usa.daily_transaction_limit = OAmount::O(10000); // 100,000.00 O
    usa.monthly_transaction_limit = OAmount::O(100000); // 1,000,000.00 O
    usa.lifetime_transaction_limit = OAmount::O(1000000); // 10,000,000.00 O
    usa.max_balance_limit = OAmount::O(100000); // 1,000,000.00 O
    usa.requires_reporting = true;
    usa.regulatory_authority = "FinCEN";
    usa.compliance_requirements = "Full KYC, SAR reporting, state licensing";
    usa.last_updated = GetTime();
    m_jurisdiction_policies["US"] = usa;
    
    // United Kingdom - Monitored
    JurisdictionPolicy uk;
    uk.country_code = "GB";
    uk.country_name = "United Kingdom";
    uk.access_level = AccessLevel::MONITORED;
    uk.compliance_level = ComplianceLevel::FULL;
    uk.allowed_features = {"measurement", "stabilization", "exchange"};
    uk.restricted_features = {"privacy", "anonymous", "brightid"};
    uk.requires_kyc = true;
    uk.allows_privacy = false;
    uk.allows_anonymous = false;
    uk.daily_transaction_limit = OAmount::O(10000); // 100,000.00 O
    uk.monthly_transaction_limit = OAmount::O(100000); // 1,000,000.00 O
    uk.lifetime_transaction_limit = OAmount::O(1000000); // 10,000,000.00 O
    uk.max_balance_limit = OAmount::O(100000); // 1,000,000.00 O
    uk.requires_reporting = true;
    uk.regulatory_authority = "FCA";
    uk.compliance_requirements = "Full KYC, AML compliance";
    uk.last_updated = GetTime();
    m_jurisdiction_policies["GB"] = uk;
    
    // France - Monitored
    JurisdictionPolicy france;
    france.country_code = "FR";
    france.country_name = "France";
    france.access_level = AccessLevel::MONITORED;
    france.compliance_level = ComplianceLevel::FULL;
    france.allowed_features = {"measurement", "stabilization", "exchange"};
    france.restricted_features = {"privacy", "anonymous", "brightid"};
    france.requires_kyc = true;
    france.allows_privacy = false;
    france.allows_anonymous = false;
    france.daily_transaction_limit = OAmount::O(10000); // 100,000.00 O
    france.monthly_transaction_limit = OAmount::O(100000); // 1,000,000.00 O
    france.lifetime_transaction_limit = OAmount::O(1000000); // 10,000,000.00 O
    france.max_balance_limit = OAmount::O(100000); // 1,000,000.00 O
    france.requires_reporting = true;
    france.regulatory_authority = "ACPR";
    france.compliance_requirements = "Full KYC, AML compliance";
    france.last_updated = GetTime();
    m_jurisdiction_policies["FR"] = france;
    
    // Italy - Monitored
    JurisdictionPolicy italy;
    italy.country_code = "IT";
    italy.country_name = "Italy";
    italy.access_level = AccessLevel::MONITORED;
    italy.compliance_level = ComplianceLevel::FULL;
    italy.allowed_features = {"measurement", "stabilization", "exchange"};
    italy.restricted_features = {"privacy", "anonymous", "brightid"};
    italy.requires_kyc = true;
    italy.allows_privacy = false;
    italy.allows_anonymous = false;
    italy.daily_transaction_limit = OAmount::O(10000); // 100,000.00 O
    italy.monthly_transaction_limit = OAmount::O(100000); // 1,000,000.00 O
    italy.lifetime_transaction_limit = OAmount::O(1000000); // 10,000,000.00 O
    italy.max_balance_limit = OAmount::O(100000); // 1,000,000.00 O
    italy.requires_reporting = true;
    italy.regulatory_authority = "Bank of Italy";
    italy.compliance_requirements = "Full KYC, AML compliance";
    italy.last_updated = GetTime();
    m_jurisdiction_policies["IT"] = italy;
    
    // Spain - Monitored
    JurisdictionPolicy spain;
    spain.country_code = "ES";
    spain.country_name = "Spain";
    spain.access_level = AccessLevel::MONITORED;
    spain.compliance_level = ComplianceLevel::FULL;
    spain.allowed_features = {"measurement", "stabilization", "exchange"};
    spain.restricted_features = {"privacy", "anonymous", "brightid"};
    spain.requires_kyc = true;
    spain.allows_privacy = false;
    spain.allows_anonymous = false;
    spain.daily_transaction_limit = OAmount::O(10000); // 100,000.00 O
    spain.monthly_transaction_limit = OAmount::O(100000); // 1,000,000.00 O
    spain.lifetime_transaction_limit = OAmount::O(1000000); // 10,000,000.00 O
    spain.max_balance_limit = OAmount::O(100000); // 1,000,000.00 O
    spain.requires_reporting = true;
    spain.regulatory_authority = "Bank of Spain";
    spain.compliance_requirements = "Full KYC, AML compliance";
    spain.last_updated = GetTime();
    m_jurisdiction_policies["ES"] = spain;
    
    LogPrintf("GeographicAccessControl: Loaded %d default jurisdiction policies\n", 
              static_cast<int>(m_jurisdiction_policies.size()));
}

bool GeographicAccessControl::SetJurisdictionPolicy(const JurisdictionPolicy& policy)
{
    if (!ValidateJurisdictionPolicy(policy)) {
        LogPrintf("GeographicAccessControl: Invalid jurisdiction policy for %s\n", 
                  policy.country_code.c_str());
        return false;
    }
    
    m_jurisdiction_policies[policy.country_code] = policy;
    UpdateStatistics();
    
    LogPrintf("GeographicAccessControl: Updated jurisdiction policy for %s\n", 
              policy.country_code.c_str());
    
    return true;
}

std::optional<JurisdictionPolicy> GeographicAccessControl::GetJurisdictionPolicy(const std::string& country_code) const
{
    auto it = m_jurisdiction_policies.find(country_code);
    if (it != m_jurisdiction_policies.end()) {
        return it->second;
    }
    
    // Return default policy for unknown countries
    JurisdictionPolicy default_policy;
    default_policy.country_code = country_code;
    default_policy.country_name = "Unknown";
    default_policy.access_level = m_default_access_level;
    default_policy.compliance_level = m_default_compliance_level;
    default_policy.allowed_features = {};
    default_policy.restricted_features = {"all"};
    default_policy.requires_kyc = true;
    default_policy.allows_privacy = false;
    default_policy.allows_anonymous = false;
    default_policy.daily_transaction_limit = 0;
    default_policy.monthly_transaction_limit = 0;
    default_policy.lifetime_transaction_limit = 0;
    default_policy.max_balance_limit = 0;
    default_policy.requires_reporting = true;
    default_policy.regulatory_authority = "Unknown";
    default_policy.compliance_requirements = "Unknown";
    default_policy.last_updated = GetTime();
    
    return default_policy;
}

bool GeographicAccessControl::RegisterUser(const std::string& user_id, const std::string& country_code, 
                                          const std::string& ip_address_hash)
{
    auto policy = GetJurisdictionPolicy(country_code);
    if (!policy.has_value()) {
        LogPrintf("GeographicAccessControl: Cannot register user %s - unknown country %s\n", 
                  user_id.c_str(), country_code.c_str());
        return false;
    }
    
    UserAccessRecord record;
    record.user_id = user_id;
    record.country_code = country_code;
    record.ip_address_hash = ip_address_hash;
    record.access_level = policy->access_level;
    record.compliance_level = policy->compliance_level;
    record.registration_timestamp = GetTime();
    record.last_access_timestamp = GetTime();
    record.daily_transaction_total = 0;
    record.monthly_transaction_total = 0;
    record.lifetime_transaction_total = 0;
    record.current_balance = 0;
    record.is_kyc_verified = false;
    record.is_privacy_enabled = policy->allows_privacy;
    record.is_anonymous = policy->allows_anonymous;
    record.used_features = {};
    record.restricted_features = policy->restricted_features;
    record.last_policy_update = GetTime();
    
    m_user_access_records[user_id] = record;
    UpdateStatistics();
    
    LogPrintf("GeographicAccessControl: Registered user %s from %s with access level %d\n", 
              user_id.c_str(), country_code.c_str(), static_cast<int>(record.access_level));
    
    return true;
}

AccessLevel GeographicAccessControl::CheckUserAccess(const std::string& user_id) const
{
    auto it = m_user_access_records.find(user_id);
    if (it != m_user_access_records.end()) {
        return it->second.access_level;
    }
    
    return m_default_access_level;
}

AccessLevel GeographicAccessControl::CheckAccessByLocation(const std::string& country_code, 
                                                          const std::string& ip_address_hash) const
{
    auto policy = GetJurisdictionPolicy(country_code);
    if (policy.has_value()) {
        return policy->access_level;
    }
    
    return m_default_access_level;
}

bool GeographicAccessControl::CanUserAccessFeature(const std::string& user_id, const std::string& feature) const
{
    auto it = m_user_access_records.find(user_id);
    if (it != m_user_access_records.end()) {
        return it->second.CanAccessFeature(feature);
    }
    
    return false;
}

bool GeographicAccessControl::CanCountryAccessFeature(const std::string& country_code, const std::string& feature) const
{
    auto policy = GetJurisdictionPolicy(country_code);
    if (policy.has_value()) {
        return policy->IsFeatureAllowed(feature);
    }
    
    return false;
}

bool GeographicAccessControl::CanUserMakeTransaction(const std::string& user_id, int64_t amount) const
{
    auto it = m_user_access_records.find(user_id);
    if (it != m_user_access_records.end()) {
        return it->second.CanMakeTransaction(amount);
    }
    
    return false;
}

bool GeographicAccessControl::CanCountryMakeTransaction(const std::string& country_code, int64_t amount) const
{
    auto policy = GetJurisdictionPolicy(country_code);
    if (policy.has_value()) {
        return policy->IsTransactionWithinLimits(amount, 0, 0);
    }
    
    return false;
}

bool GeographicAccessControl::CanUserUsePrivacyFeatures(const std::string& user_id) const
{
    auto it = m_user_access_records.find(user_id);
    if (it != m_user_access_records.end()) {
        return it->second.is_privacy_enabled;
    }
    
    return false;
}

bool GeographicAccessControl::CanCountryUsePrivacyFeatures(const std::string& country_code) const
{
    auto policy = GetJurisdictionPolicy(country_code);
    if (policy.has_value()) {
        return policy->allows_privacy;
    }
    
    return false;
}

bool GeographicAccessControl::CanUserParticipateAnonymously(const std::string& user_id) const
{
    auto it = m_user_access_records.find(user_id);
    if (it != m_user_access_records.end()) {
        return it->second.is_anonymous;
    }
    
    return false;
}

bool GeographicAccessControl::CanCountryParticipateAnonymously(const std::string& country_code) const
{
    auto policy = GetJurisdictionPolicy(country_code);
    if (policy.has_value()) {
        return policy->allows_anonymous;
    }
    
    return false;
}

bool GeographicAccessControl::DoesUserRequireKYC(const std::string& user_id) const
{
    auto it = m_user_access_records.find(user_id);
    if (it != m_user_access_records.end()) {
        return it->second.compliance_level == ComplianceLevel::STANDARD || 
               it->second.compliance_level == ComplianceLevel::FULL;
    }
    
    return true; // Default to requiring KYC for unknown users
}

bool GeographicAccessControl::DoesCountryRequireKYC(const std::string& country_code) const
{
    auto policy = GetJurisdictionPolicy(country_code);
    if (policy.has_value()) {
        return policy->requires_kyc;
    }
    
    return true; // Default to requiring KYC for unknown countries
}

void GeographicAccessControl::UpdateStatistics()
{
    // Reset statistics
    m_access_stats = AccessStatistics{};
    m_jurisdiction_stats = JurisdictionStatistics{};
    
    // Count users by access level
    for (const auto& pair : m_user_access_records) {
        const auto& record = pair.second;
        m_access_stats.total_users++;
        
        switch (record.access_level) {
            case AccessLevel::ALLOWED:
                m_access_stats.allowed_users++;
                break;
            case AccessLevel::RESTRICTED:
                m_access_stats.restricted_users++;
                break;
            case AccessLevel::BLOCKED:
                m_access_stats.blocked_users++;
                break;
            case AccessLevel::MONITORED:
                m_access_stats.monitored_users++;
                break;
        }
        
        m_access_stats.users_by_country[record.country_code]++;
        m_access_stats.users_by_access_level[std::to_string(static_cast<int>(record.access_level))]++;
        m_access_stats.users_by_compliance_level[std::to_string(static_cast<int>(record.compliance_level))]++;
    }
    
    // Count jurisdictions by access level
    for (const auto& pair : m_jurisdiction_policies) {
        const auto& policy = pair.second;
        m_jurisdiction_stats.total_jurisdictions++;
        
        switch (policy.access_level) {
            case AccessLevel::ALLOWED:
                m_jurisdiction_stats.allowed_jurisdictions++;
                break;
            case AccessLevel::RESTRICTED:
                m_jurisdiction_stats.restricted_jurisdictions++;
                break;
            case AccessLevel::BLOCKED:
                m_jurisdiction_stats.blocked_jurisdictions++;
                break;
            case AccessLevel::MONITORED:
                m_jurisdiction_stats.monitored_jurisdictions++;
                break;
        }
        
        m_jurisdiction_stats.jurisdictions_by_access_level[std::to_string(static_cast<int>(policy.access_level))]++;
        m_jurisdiction_stats.jurisdictions_by_compliance_level[std::to_string(static_cast<int>(policy.compliance_level))]++;
    }
}

bool GeographicAccessControl::ValidateJurisdictionPolicy(const JurisdictionPolicy& policy) const
{
    if (policy.country_code.empty() || policy.country_code.length() != 2) {
        return false;
    }
    
    if (policy.country_name.empty()) {
        return false;
    }
    
    if (policy.daily_transaction_limit < 0 || 
        policy.monthly_transaction_limit < 0 || 
        policy.lifetime_transaction_limit < 0 || 
        policy.max_balance_limit < 0) {
        return false;
    }
    
    return true;
}

std::string GeographicAccessControl::HashIPAddress(const std::string& ip_address) const
{
    // Simple hash for IP address (in production, use proper hashing)
    return std::to_string(std::hash<std::string>{}(ip_address));
}

void GeographicAccessControl::LogAccessAttempt(const std::string& user_id, const std::string& country_code, 
                                              AccessLevel access_level, const std::string& reason)
{
    LogPrintf("GeographicAccessControl: Access attempt - User: %s, Country: %s, Level: %d, Reason: %s\n", 
              user_id.c_str(), country_code.c_str(), static_cast<int>(access_level), reason.c_str());
}

void GeographicAccessControl::NotifyComplianceViolation(const std::string& user_id, const std::string& violation_type)
{
    LogPrintf("GeographicAccessControl: Compliance violation - User: %s, Type: %s\n", 
              user_id.c_str(), violation_type.c_str());
}

// JurisdictionPolicy methods
bool JurisdictionPolicy::IsFeatureAllowed(const std::string& feature) const
{
    for (const auto& allowed_feature : allowed_features) {
        if (allowed_feature == feature) {
            return true;
        }
    }
    return false;
}

bool JurisdictionPolicy::IsFeatureRestricted(const std::string& feature) const
{
    for (const auto& restricted_feature : restricted_features) {
        if (restricted_feature == feature || restricted_feature == "all") {
            return true;
        }
    }
    return false;
}

bool JurisdictionPolicy::IsTransactionWithinLimits(int64_t amount, int64_t daily_total, int64_t monthly_total) const
{
    if (amount <= 0) {
        return false;
    }
    
    if (daily_transaction_limit > 0 && (daily_total + amount) > daily_transaction_limit) {
        return false;
    }
    
    if (monthly_transaction_limit > 0 && (monthly_total + amount) > monthly_transaction_limit) {
        return false;
    }
    
    return true;
}

bool JurisdictionPolicy::IsBalanceWithinLimits(int64_t balance) const
{
    if (max_balance_limit > 0 && balance > max_balance_limit) {
        return false;
    }
    
    return true;
}

// UserAccessRecord methods
bool UserAccessRecord::CanAccessFeature(const std::string& feature) const
{
    // Check if feature is restricted
    for (const auto& restricted_feature : restricted_features) {
        if (restricted_feature == feature || restricted_feature == "all") {
            return false;
        }
    }
    
    // Check if feature is allowed
    for (const auto& used_feature : used_features) {
        if (used_feature == feature) {
            return true;
        }
    }
    
    // Default to allowing if not explicitly restricted
    return true;
}

bool UserAccessRecord::CanMakeTransaction(int64_t amount) const
{
    if (amount <= 0) {
        return false;
    }
    
    // Check daily limit
    if ((daily_transaction_total + amount) > OAmount::O(100000)) { // 1,000,000.00 O default
        return false;
    }
    
    // Check monthly limit
    if ((monthly_transaction_total + amount) > OAmount::O(1000000)) { // 10,000,000.00 O default
        return false;
    }
    
    return true;
}

void UserAccessRecord::UpdateTransactionTotals(int64_t amount)
{
    daily_transaction_total += amount;
    monthly_transaction_total += amount;
    lifetime_transaction_total += amount;
}

void UserAccessRecord::ResetDailyTotals()
{
    daily_transaction_total = 0;
}

void UserAccessRecord::ResetMonthlyTotals()
{
    monthly_transaction_total = 0;
}
