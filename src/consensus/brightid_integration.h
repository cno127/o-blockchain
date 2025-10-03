// Copyright (c) 2024 The Bitcoin Core developers
// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_BRIGHTID_INTEGRATION_H
#define BITCOIN_CONSENSUS_BRIGHTID_INTEGRATION_H

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <serialize.h>
#include <consensus/amount.h>
#include <consensus/o_amount.h>

/** BrightID Verification Status */
enum class BrightIDStatus {
    UNVERIFIED,         // Not verified by BrightID
    VERIFIED,          // Verified by BrightID
    SPONSORED,         // Sponsored by another user
    MEETUP_VERIFIED,   // Verified through meetup
    FAILED,            // Verification failed
    EXPIRED            // Verification expired
};

/** BrightID Verification Method */
enum class BrightIDVerificationMethod {
    SOCIAL_GRAPH,      // Social graph verification
    SPONSORSHIP,       // Sponsored by verified user
    MEETUP,            // In-person meetup verification
    COMMUNITY,         // Community verification
    UNKNOWN            // Unknown method
};

/** BrightID User Information */
struct BrightIDUser {
    std::string brightid_address;        // BrightID address (hash)
    std::string context_id;              // Context ID for this application
    BrightIDStatus status;               // Verification status
    BrightIDVerificationMethod method;   // Verification method used
    int64_t verification_timestamp;      // When verification occurred
    int64_t expiration_timestamp;        // When verification expires
    std::string sponsor_address;         // Address of sponsor (if sponsored)
    std::vector<std::string> connections; // Connected BrightID addresses
    double trust_score;                  // Trust score from social graph
    bool is_active;                      // Whether user is currently active
    
    BrightIDUser()
        : brightid_address(), context_id(), status(BrightIDStatus::UNVERIFIED),
          method(BrightIDVerificationMethod::UNKNOWN), verification_timestamp(0),
          expiration_timestamp(0), sponsor_address(), connections(),
          trust_score(0.0), is_active(false) {}
    
    SERIALIZE_METHODS(BrightIDUser, obj) {
        uint8_t status_val = static_cast<uint8_t>(obj.status);
        uint8_t method_val = static_cast<uint8_t>(obj.method);
        READWRITE(obj.brightid_address, obj.context_id, status_val, method_val,
                  obj.verification_timestamp, obj.expiration_timestamp,
                  obj.sponsor_address, obj.connections, obj.trust_score, obj.is_active);
        if (ser_action.ForRead()) {
            obj.status = static_cast<BrightIDStatus>(status_val);
            obj.method = static_cast<BrightIDVerificationMethod>(method_val);
        }
    }
    
    bool IsVerified() const { return status == BrightIDStatus::VERIFIED || 
                                    status == BrightIDStatus::SPONSORED || 
                                    status == BrightIDStatus::MEETUP_VERIFIED; }
    bool IsExpired() const { return status == BrightIDStatus::EXPIRED; }
    bool IsActive() const { return is_active && IsVerified() && !IsExpired(); }
};

/** BrightID Verification Request */
struct BrightIDVerificationRequest {
    std::string brightid_address;
    std::string context_id;
    std::string verification_data;       // JSON data from BrightID
    std::string signature;               // Signature from BrightID
    int64_t timestamp;
    std::string request_id;              // Unique request identifier
    
    BrightIDVerificationRequest()
        : brightid_address(), context_id(), verification_data(),
          signature(), timestamp(0), request_id() {}
    
    SERIALIZE_METHODS(BrightIDVerificationRequest, obj) {
        READWRITE(obj.brightid_address, obj.context_id, obj.verification_data,
                  obj.signature, obj.timestamp, obj.request_id);
    }
    
    bool IsValid() const {
        return !brightid_address.empty() && !context_id.empty() && 
               !verification_data.empty() && !signature.empty() && timestamp > 0;
    }
};

/** BrightID Integration Manager */
class BrightIDIntegration {
public:
    BrightIDIntegration();
    ~BrightIDIntegration();
    
    // ===== BrightID API Integration =====
    
    /** Initialize BrightID integration */
    bool Initialize(const std::string& context_id, const std::string& api_endpoint);
    
    /** Verify user with BrightID */
    bool VerifyUser(const BrightIDVerificationRequest& request);
    
    /** Check user verification status */
    std::optional<BrightIDUser> GetUserStatus(const std::string& brightid_address) const;
    
    /** Update user verification status */
    void UpdateUserStatus(const std::string& brightid_address, const BrightIDUser& user);
    
    /** Remove expired verifications */
    void CleanupExpiredVerifications();
    
    // ===== Social Graph Analysis =====
    
    /** Analyze social graph for user */
    double AnalyzeSocialGraph(const std::string& brightid_address) const;
    
    /** Get user connections */
    std::vector<std::string> GetUserConnections(const std::string& brightid_address) const;
    
    /** Calculate trust score */
    double CalculateTrustScore(const std::string& brightid_address) const;
    
    /** Detect potential Sybil attacks */
    bool DetectSybilAttack(const std::string& brightid_address) const;
    
    // ===== Verification Methods =====
    
    /** Verify through social graph */
    bool VerifySocialGraph(const std::string& brightid_address);
    
    /** Verify through sponsorship */
    bool VerifySponsorship(const std::string& brightid_address, const std::string& sponsor_address);
    
    /** Verify through meetup */
    bool VerifyMeetup(const std::string& brightid_address, const std::string& meetup_data);
    
    /** Verify through community */
    bool VerifyCommunity(const std::string& brightid_address, const std::string& community_proof);
    
    // ===== Privacy-Preserving Features =====
    
    /** Generate anonymous user ID */
    std::string GenerateAnonymousID(const std::string& brightid_address) const;
    
    /** Verify without revealing identity */
    bool AnonymousVerification(const std::string& anonymous_id) const;
    
    /** Get privacy-preserving reputation */
    double GetPrivacyPreservingReputation(const std::string& anonymous_id) const;
    
    /** Update reputation without identity correlation */
    void UpdateAnonymousReputation(const std::string& anonymous_id, double reputation_delta);
    
    // ===== Integration with O Blockchain =====
    
    /** Register user with O Blockchain */
    bool RegisterUser(const std::string& brightid_address, const std::string& o_address);
    
    /** Link BrightID to O Blockchain address */
    bool LinkAddress(const std::string& brightid_address, const std::string& o_address);
    
    /** Unlink BrightID from O Blockchain address */
    bool UnlinkAddress(const std::string& brightid_address, const std::string& o_address);
    
    /** Get O Blockchain address for BrightID */
    std::optional<std::string> GetOAddress(const std::string& brightid_address) const;
    
    /** Get BrightID address for O Blockchain address */
    std::optional<std::string> GetBrightIDAddress(const std::string& o_address) const;
    
    // ===== Statistics and Reporting =====
    
    /** Get verification statistics */
    std::map<std::string, int64_t> GetVerificationStatistics() const;
    
    /** Get user statistics */
    std::map<std::string, int64_t> GetUserStatistics() const;
    
    /** Get social graph statistics */
    std::map<std::string, double> GetSocialGraphStatistics() const;
    
    /** Get privacy statistics */
    std::map<std::string, int64_t> GetPrivacyStatistics() const;
    
    // ===== Configuration =====
    
    /** Set verification requirements */
    void SetVerificationRequirements(double min_trust_score, int min_connections, int64_t max_age);
    
    /** Get verification requirements */
    std::tuple<double, int, int64_t> GetVerificationRequirements() const;
    
    /** Set privacy settings */
    void SetPrivacySettings(bool anonymous_mode, bool data_retention, int64_t retention_period);
    
    /** Get privacy settings */
    std::tuple<bool, bool, int64_t> GetPrivacySettings() const;
    
    // ===== Utility Functions =====
    
    /** Check if BrightID is available */
    bool IsBrightIDAvailable() const;
    
    /** Get BrightID API status */
    std::string GetAPIStatus() const;
    
    /** Validate BrightID address format */
    bool ValidateBrightIDAddress(const std::string& address) const;
    
    /** Generate context ID for application */
    std::string GenerateContextID() const;
    
    // ===== Data Management =====
    
    /** Backup BrightID data */
    void BackupData(const std::string& backup_path) const;
    
    /** Restore BrightID data */
    bool RestoreData(const std::string& backup_path);
    
    /** Clear all data (for testing) */
    void ClearAllData();
    
    /** Prune old data */
    void PruneOldData(int64_t cutoff_timestamp);

private:
    // Configuration
    std::string m_context_id;
    std::string m_api_endpoint;
    bool m_initialized;
    
    // Verification requirements
    double m_min_trust_score;
    int m_min_connections;
    int64_t m_max_verification_age;
    
    // Privacy settings
    bool m_anonymous_mode;
    bool m_data_retention;
    int64_t m_retention_period;
    
    // Storage
    std::map<std::string, BrightIDUser> m_users;
    std::map<std::string, std::string> m_brightid_to_o_address;
    std::map<std::string, std::string> m_o_to_brightid_address;
    std::map<std::string, std::string> m_anonymous_ids;
    std::map<std::string, double> m_anonymous_reputations;
    
    // Statistics
    struct BrightIDStats {
        int64_t total_users;
        int64_t verified_users;
        int64_t active_users;
        int64_t expired_users;
        int64_t failed_verifications;
        int64_t social_graph_verifications;
        int64_t sponsorship_verifications;
        int64_t meetup_verifications;
        int64_t community_verifications;
        double average_trust_score;
        int64_t total_connections;
    } m_stats;
    
    // Helper functions
    void UpdateStatistics();
    void LogVerification(const std::string& brightid_address, BrightIDStatus status, 
                        BrightIDVerificationMethod method);
    bool ValidateVerificationData(const std::string& verification_data) const;
    std::string HashBrightIDAddress(const std::string& address) const;
    bool CheckAPIEndpoint(const std::string& endpoint) const;
    void NotifyUserVerification(const std::string& brightid_address, BrightIDStatus status);
};

/** Global BrightID integration instance */
extern BrightIDIntegration g_brightid_integration;

#endif // BITCOIN_CONSENSUS_BRIGHTID_INTEGRATION_H
