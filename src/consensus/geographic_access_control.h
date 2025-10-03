// Copyright (c) 2024 The Bitcoin Core developers
// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_GEOGRAPHIC_ACCESS_CONTROL_H
#define BITCOIN_CONSENSUS_GEOGRAPHIC_ACCESS_CONTROL_H

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <serialize.h>
#include <consensus/amount.h>
#include <consensus/o_amount.h>

/** Geographic Access Control for O Blockchain
 * 
 * This system implements targeted deployment by controlling access
 * based on user jurisdiction while maintaining privacy-preserving
 * features for crypto-friendly regions.
 */

/** Access Level for Different Jurisdictions */
enum class AccessLevel {
    ALLOWED,        // Full access with privacy features
    RESTRICTED,     // Limited access with some restrictions
    BLOCKED,        // No access allowed
    MONITORED       // Access with enhanced monitoring
};

/** Compliance Level for Different Jurisdictions */
enum class ComplianceLevel {
    NONE,           // No compliance required (crypto-friendly)
    BASIC,          // Basic AML compliance
    STANDARD,       // Standard KYC compliance
    FULL            // Full regulatory compliance
};

/** Jurisdiction Policy Configuration */
struct JurisdictionPolicy {
    std::string country_code;                    // ISO 3166-1 alpha-2 country code
    std::string country_name;                    // Full country name
    AccessLevel access_level;                    // Access level for this jurisdiction
    ComplianceLevel compliance_level;            // Compliance level required
    std::vector<std::string> allowed_features;   // Features allowed in this jurisdiction
    std::vector<std::string> restricted_features; // Features restricted in this jurisdiction
    bool requires_kyc;                          // Whether KYC is required
    bool allows_privacy;                        // Whether privacy features are allowed
    bool allows_anonymous;                      // Whether anonymous participation is allowed
    int64_t daily_transaction_limit;            // Daily transaction limit in satoshis
    int64_t monthly_transaction_limit;          // Monthly transaction limit in satoshis
    int64_t lifetime_transaction_limit;         // Lifetime transaction limit in satoshis
    int64_t max_balance_limit;                  // Maximum balance limit in satoshis
    bool requires_reporting;                    // Whether transaction reporting is required
    std::string regulatory_authority;           // Regulatory authority for this jurisdiction
    std::string compliance_requirements;        // Specific compliance requirements
    int64_t last_updated;                       // Last policy update timestamp
    
    JurisdictionPolicy()
        : country_code(), country_name(), access_level(AccessLevel::BLOCKED),
          compliance_level(ComplianceLevel::FULL), allowed_features(),
          restricted_features(), requires_kyc(true), allows_privacy(false),
          allows_anonymous(false), daily_transaction_limit(0),
          monthly_transaction_limit(0), lifetime_transaction_limit(0),
          max_balance_limit(0), requires_reporting(true),
          regulatory_authority(), compliance_requirements(), last_updated(0) {}
    
    SERIALIZE_METHODS(JurisdictionPolicy, obj) {
        uint8_t access_level_val = static_cast<uint8_t>(obj.access_level);
        uint8_t compliance_level_val = static_cast<uint8_t>(obj.compliance_level);
        READWRITE(obj.country_code, obj.country_name, access_level_val, compliance_level_val,
                  obj.allowed_features, obj.restricted_features, obj.requires_kyc,
                  obj.allows_privacy, obj.allows_anonymous, obj.daily_transaction_limit,
                  obj.monthly_transaction_limit, obj.lifetime_transaction_limit,
                  obj.max_balance_limit, obj.requires_reporting, obj.regulatory_authority,
                  obj.compliance_requirements, obj.last_updated);
        if (ser_action.ForRead()) {
            obj.access_level = static_cast<AccessLevel>(access_level_val);
            obj.compliance_level = static_cast<ComplianceLevel>(compliance_level_val);
        }
    }
    
    /** Check if a feature is allowed in this jurisdiction */
    bool IsFeatureAllowed(const std::string& feature) const;
    
    /** Check if a feature is restricted in this jurisdiction */
    bool IsFeatureRestricted(const std::string& feature) const;
    
    /** Check if transaction is within limits */
    bool IsTransactionWithinLimits(int64_t amount, int64_t daily_total, int64_t monthly_total) const;
    
    /** Check if balance is within limits */
    bool IsBalanceWithinLimits(int64_t balance) const;
};

/** User Access Record */
struct UserAccessRecord {
    std::string user_id;                        // User identifier
    std::string country_code;                   // User's country
    std::string ip_address_hash;                // Hashed IP address
    AccessLevel access_level;                   // User's access level
    ComplianceLevel compliance_level;           // User's compliance level
    int64_t registration_timestamp;             // When user registered
    int64_t last_access_timestamp;              // Last access time
    int64_t daily_transaction_total;            // Daily transaction total
    int64_t monthly_transaction_total;          // Monthly transaction total
    int64_t lifetime_transaction_total;         // Lifetime transaction total
    int64_t current_balance;                    // Current balance
    bool is_kyc_verified;                       // Whether user is KYC verified
    bool is_privacy_enabled;                    // Whether privacy features are enabled
    bool is_anonymous;                          // Whether user is anonymous
    std::vector<std::string> used_features;     // Features used by user
    std::vector<std::string> restricted_features; // Features restricted for user
    int64_t last_policy_update;                 // Last policy update for user
    
    UserAccessRecord()
        : user_id(), country_code(), ip_address_hash(),
          access_level(AccessLevel::BLOCKED), compliance_level(ComplianceLevel::FULL),
          registration_timestamp(0), last_access_timestamp(0),
          daily_transaction_total(0), monthly_transaction_total(0),
          lifetime_transaction_total(0), current_balance(0),
          is_kyc_verified(false), is_privacy_enabled(false),
          is_anonymous(false), used_features(), restricted_features(),
          last_policy_update(0) {}
    
    SERIALIZE_METHODS(UserAccessRecord, obj) {
        uint8_t access_level_val = static_cast<uint8_t>(obj.access_level);
        uint8_t compliance_level_val = static_cast<uint8_t>(obj.compliance_level);
        READWRITE(obj.user_id, obj.country_code, obj.ip_address_hash,
                  access_level_val, compliance_level_val, obj.registration_timestamp,
                  obj.last_access_timestamp, obj.daily_transaction_total,
                  obj.monthly_transaction_total, obj.lifetime_transaction_total,
                  obj.current_balance, obj.is_kyc_verified, obj.is_privacy_enabled,
                  obj.is_anonymous, obj.used_features, obj.restricted_features,
                  obj.last_policy_update);
        if (ser_action.ForRead()) {
            obj.access_level = static_cast<AccessLevel>(access_level_val);
            obj.compliance_level = static_cast<ComplianceLevel>(compliance_level_val);
        }
    }
    
    /** Check if user can access a feature */
    bool CanAccessFeature(const std::string& feature) const;
    
    /** Check if user can make a transaction */
    bool CanMakeTransaction(int64_t amount) const;
    
    /** Update transaction totals */
    void UpdateTransactionTotals(int64_t amount);
    
    /** Reset daily totals */
    void ResetDailyTotals();
    
    /** Reset monthly totals */
    void ResetMonthlyTotals();
};

/** Geographic Access Control Manager */
class GeographicAccessControl {
public:
    GeographicAccessControl();
    ~GeographicAccessControl();
    
    // ===== Jurisdiction Management =====
    
    /** Initialize with default jurisdiction policies */
    bool Initialize();
    
    /** Add or update jurisdiction policy */
    bool SetJurisdictionPolicy(const JurisdictionPolicy& policy);
    
    /** Get jurisdiction policy */
    std::optional<JurisdictionPolicy> GetJurisdictionPolicy(const std::string& country_code) const;
    
    /** Remove jurisdiction policy */
    bool RemoveJurisdictionPolicy(const std::string& country_code);
    
    /** Get all jurisdiction policies */
    std::vector<JurisdictionPolicy> GetAllJurisdictionPolicies() const;
    
    // ===== User Access Management =====
    
    /** Register user with geographic access control */
    bool RegisterUser(const std::string& user_id, const std::string& country_code, 
                     const std::string& ip_address_hash);
    
    /** Check user access level */
    AccessLevel CheckUserAccess(const std::string& user_id) const;
    
    /** Check user access by country and IP */
    AccessLevel CheckAccessByLocation(const std::string& country_code, 
                                     const std::string& ip_address_hash) const;
    
    /** Update user access record */
    bool UpdateUserAccess(const std::string& user_id, const UserAccessRecord& record);
    
    /** Get user access record */
    std::optional<UserAccessRecord> GetUserAccessRecord(const std::string& user_id) const;
    
    /** Remove user access record */
    bool RemoveUserAccessRecord(const std::string& user_id);
    
    // ===== Feature Access Control =====
    
    /** Check if user can access a feature */
    bool CanUserAccessFeature(const std::string& user_id, const std::string& feature) const;
    
    /** Check if country can access a feature */
    bool CanCountryAccessFeature(const std::string& country_code, const std::string& feature) const;
    
    /** Get allowed features for user */
    std::vector<std::string> GetAllowedFeatures(const std::string& user_id) const;
    
    /** Get restricted features for user */
    std::vector<std::string> GetRestrictedFeatures(const std::string& user_id) const;
    
    // ===== Transaction Access Control =====
    
    /** Check if user can make a transaction */
    bool CanUserMakeTransaction(const std::string& user_id, int64_t amount) const;
    
    /** Check if country allows transaction */
    bool CanCountryMakeTransaction(const std::string& country_code, int64_t amount) const;
    
    /** Validate transaction against limits */
    bool ValidateTransactionLimits(const std::string& user_id, int64_t amount) const;
    
    /** Update user transaction totals */
    bool UpdateUserTransactionTotals(const std::string& user_id, int64_t amount);
    
    // ===== Privacy and Compliance Control =====
    
    /** Check if user can use privacy features */
    bool CanUserUsePrivacyFeatures(const std::string& user_id) const;
    
    /** Check if country allows privacy features */
    bool CanCountryUsePrivacyFeatures(const std::string& country_code) const;
    
    /** Check if user can participate anonymously */
    bool CanUserParticipateAnonymously(const std::string& user_id) const;
    
    /** Check if country allows anonymous participation */
    bool CanCountryParticipateAnonymously(const std::string& country_code) const;
    
    /** Check if user requires KYC */
    bool DoesUserRequireKYC(const std::string& user_id) const;
    
    /** Check if country requires KYC */
    bool DoesCountryRequireKYC(const std::string& country_code) const;
    
    // ===== Compliance and Reporting =====
    
    /** Check if user requires transaction reporting */
    bool DoesUserRequireReporting(const std::string& user_id) const;
    
    /** Check if country requires transaction reporting */
    bool DoesCountryRequireReporting(const std::string& country_code) const;
    
    /** Generate compliance report for jurisdiction */
    std::string GenerateComplianceReport(const std::string& country_code) const;
    
    /** Generate user compliance report */
    std::string GenerateUserComplianceReport(const std::string& user_id) const;
    
    // ===== Statistics and Monitoring =====
    
    /** Get access statistics */
    struct AccessStatistics {
        int64_t total_users;
        int64_t allowed_users;
        int64_t restricted_users;
        int64_t blocked_users;
        int64_t monitored_users;
        std::map<std::string, int64_t> users_by_country;
        std::map<std::string, int64_t> users_by_access_level;
        std::map<std::string, int64_t> users_by_compliance_level;
    };
    
    AccessStatistics GetAccessStatistics() const;
    
    /** Get jurisdiction statistics */
    struct JurisdictionStatistics {
        int64_t total_jurisdictions;
        int64_t allowed_jurisdictions;
        int64_t restricted_jurisdictions;
        int64_t blocked_jurisdictions;
        int64_t monitored_jurisdictions;
        std::map<std::string, int64_t> jurisdictions_by_access_level;
        std::map<std::string, int64_t> jurisdictions_by_compliance_level;
    };
    
    JurisdictionStatistics GetJurisdictionStatistics() const;
    
    // ===== Utility Functions =====
    
    /** Detect user country from IP address */
    std::string DetectCountryFromIP(const std::string& ip_address) const;
    
    /** Validate country code */
    bool ValidateCountryCode(const std::string& country_code) const;
    
    /** Get country name from code */
    std::string GetCountryName(const std::string& country_code) const;
    
    /** Check if IP address is allowed */
    bool IsIPAddressAllowed(const std::string& ip_address) const;
    
    /** Check if IP address is blocked */
    bool IsIPAddressBlocked(const std::string& ip_address) const;
    
    // ===== Configuration =====
    
    /** Set default access level */
    void SetDefaultAccessLevel(AccessLevel level);
    
    /** Set default compliance level */
    void SetDefaultComplianceLevel(ComplianceLevel level);
    
    /** Set IP geolocation service */
    void SetIPGeolocationService(const std::string& service_url);
    
    /** Set compliance reporting requirements */
    void SetComplianceReportingRequirements(bool enabled, int64_t threshold);
    
    // ===== Data Management =====
    
    /** Backup access control data */
    void BackupData(const std::string& backup_path) const;
    
    /** Restore access control data */
    bool RestoreData(const std::string& backup_path);
    
    /** Clear all data (for testing) */
    void ClearAllData();
    
    /** Prune old data */
    void PruneOldData(int64_t cutoff_timestamp);

private:
    // Configuration
    AccessLevel m_default_access_level;
    ComplianceLevel m_default_compliance_level;
    std::string m_ip_geolocation_service;
    bool m_compliance_reporting_enabled;
    int64_t m_compliance_reporting_threshold;
    
    // Storage
    std::map<std::string, JurisdictionPolicy> m_jurisdiction_policies;
    std::map<std::string, UserAccessRecord> m_user_access_records;
    std::map<std::string, std::string> m_ip_to_country_cache;
    
    // Statistics
    AccessStatistics m_access_stats;
    JurisdictionStatistics m_jurisdiction_stats;
    
    // Helper functions
    void UpdateStatistics();
    void LoadDefaultJurisdictionPolicies();
    bool ValidateJurisdictionPolicy(const JurisdictionPolicy& policy) const;
    std::string HashIPAddress(const std::string& ip_address) const;
    bool CheckIPGeolocationService(const std::string& service_url) const;
    void LogAccessAttempt(const std::string& user_id, const std::string& country_code, 
                         AccessLevel access_level, const std::string& reason);
    void NotifyComplianceViolation(const std::string& user_id, const std::string& violation_type);
};

/** Global geographic access control instance */
extern GeographicAccessControl g_geographic_access_control;

#endif // BITCOIN_CONSENSUS_GEOGRAPHIC_ACCESS_CONTROL_H
