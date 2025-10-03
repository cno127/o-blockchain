// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_USER_CONSENSUS_H
#define BITCOIN_CONSENSUS_USER_CONSENSUS_H

#include <pubkey.h>
#include <uint256.h>
#include <serialize.h>
#include <vector>
#include <string>
#include <map>
#include <set>

/**
 * User Registry Consensus System
 * 
 * This implements a Proof of Identity (PoI) + Endorsement consensus mechanism
 * specifically for ensuring user uniqueness in the O blockchain system.
 * 
 * Key Features:
 * - Government ID hash uniqueness enforcement
 * - Multi-layer endorsement system
 * - Geographic diversity requirements
 * - Challenge and resolution mechanisms
 * - Reputation scoring system
 */

enum class UserStatus {
    PENDING_VERIFICATION = 0,    // Initial registration, awaiting endorsements
    VERIFICATION_IN_PROGRESS,    // Being reviewed by endorsers
    VERIFIED,                    // Fully verified user
    SUSPENDED,                   // Temporarily suspended
    BLACKLISTED                  // Permanently banned
};

enum class VerificationMethod {
    GOVERNMENT_ID = 0,
    VIDEO_CALL,
    DOCUMENT_REVIEW,
    BIOMETRIC_VERIFICATION,
    THIRD_PARTY_KYC
};

enum class ConfidenceLevel {
    LOW = 0,
    MEDIUM,
    HIGH
};

/** Official User Structure */
struct OfficialUser {
    CPubKey public_key;
    std::string government_id_hash;     // SHA256 hash of government ID (ensures uniqueness)
    std::string birth_currency;         // User's birth country currency
    std::string identity_proof_hash;    // Hash of identity verification documents
    std::vector<CPubKey> endorsers;     // Users who validated this identity
    UserStatus status = UserStatus::PENDING_VERIFICATION;
    uint32_t registration_height = 0;
    uint32_t verification_height = 0;
    double reputation_score = 0.0;      // Based on validation accuracy
    uint32_t endorsement_threshold = 5; // Minimum endorsements required
    uint32_t required_geo_diversity = 3; // Endorsements from different countries
    
    // Metadata
    std::string country_code;
    std::string registration_ip_hash;   // Hashed IP for anti-sybil
    uint32_t last_activity_height = 0;
    
    SERIALIZE_METHODS(OfficialUser, obj) {
        READWRITE(obj.public_key, obj.government_id_hash, obj.birth_currency, 
                 obj.identity_proof_hash, obj.endorsers, obj.status, 
                 obj.registration_height, obj.verification_height, 
                 obj.reputation_score, obj.endorsement_threshold, 
                 obj.required_geo_diversity, obj.country_code, 
                 obj.registration_ip_hash, obj.last_activity_height);
    }
    
    /** Check if user meets verification requirements */
    bool IsVerificationComplete() const;
    
    /** Get number of unique countries in endorsements */
    uint32_t GetEndorsementGeoDiversity() const;
    
    /** Check if user has sufficient endorsements */
    bool HasSufficientEndorsements() const;
};

/** Endorsement Record */
struct EndorsementRecord {
    uint256 endorsement_id;
    CPubKey endorser;
    CPubKey endorsed_user;
    VerificationMethod verification_method;
    ConfidenceLevel confidence_level;
    std::string verification_notes;     // Optional notes from endorser
    uint32_t timestamp = 0;
    uint32_t block_height = 0;
    bool is_challenged = false;
    std::vector<CPubKey> challengers;   // Users challenging this endorsement
    std::vector<CPubKey> defenders;     // Users defending this endorsement
    double endorser_reputation_at_time = 0.0; // Snapshot of endorser's reputation
    
    SERIALIZE_METHODS(EndorsementRecord, obj) {
        READWRITE(obj.endorsement_id, obj.endorser, obj.endorsed_user, 
                 obj.verification_method, obj.confidence_level, 
                 obj.verification_notes, obj.timestamp, obj.block_height, 
                 obj.is_challenged, obj.challengers, obj.defenders, 
                 obj.endorser_reputation_at_time);
    }
    
    /** Calculate endorsement weight based on reputation and confidence */
    double GetEndorsementWeight() const;
    
    /** Check if endorsement is resolved */
    bool IsResolved() const;
};

/** Challenge Record */
struct ChallengeRecord {
    uint256 challenge_id;
    uint256 endorsement_id;
    CPubKey challenger;
    std::string challenge_reason;
    std::string evidence_hash;          // Hash of evidence provided
    uint32_t timestamp = 0;
    uint32_t block_height = 0;
    bool is_resolved = false;
    bool challenge_upheld = false;      // True if challenge was successful
    std::vector<CPubKey> resolution_voters; // Users who voted on resolution
    
    SERIALIZE_METHODS(ChallengeRecord, obj) {
        READWRITE(obj.challenge_id, obj.endorsement_id, obj.challenger, 
                 obj.challenge_reason, obj.evidence_hash, obj.timestamp, 
                 obj.block_height, obj.is_resolved, obj.challenge_upheld, 
                 obj.resolution_voters);
    }
};

/** User Registry Consensus Engine */
class UserRegistryConsensus {
private:
    // In-memory caches for performance
    std::map<CPubKey, OfficialUser> user_cache;
    std::map<uint256, EndorsementRecord> endorsement_cache;
    std::map<uint256, ChallengeRecord> challenge_cache;
    
    // Government ID hash to user mapping (for uniqueness checking)
    std::map<std::string, CPubKey> government_id_to_user;
    
    // Configuration parameters
    struct ConsensusParams {
        uint32_t min_endorsements = 5;
        uint32_t min_geo_diversity = 3;
        uint32_t challenge_period_blocks = 144; // ~12 hours at 5s blocks
        uint32_t resolution_period_blocks = 288; // ~24 hours
        double min_endorser_reputation = 0.5;
        double challenge_threshold = 0.3; // 30% of endorsers must challenge
        int64_t endorsement_reward = 100; // O tokens
        int64_t challenge_reward = 50;    // O tokens for successful challenges
    } params;
    
public:
    UserRegistryConsensus();
    ~UserRegistryConsensus();
    
    /** Core Consensus Methods */
    bool RegisterUser(const OfficialUser& user, std::string& error_message);
    bool SubmitEndorsement(const EndorsementRecord& endorsement, std::string& error_message);
    bool ChallengeEndorsement(const ChallengeRecord& challenge, std::string& error_message);
    bool ResolveChallenge(uint256 challenge_id, bool uphold_challenge, std::string& error_message);
    
    /** Validation Methods */
    bool IsUserUnique(const std::string& government_id_hash) const;
    bool IsUserVerified(const CPubKey& user_key) const;
    bool ValidateGeoDiversity(const CPubKey& user_key) const;
    bool CheckDuplicateRegistration(const CPubKey& user_key) const;
    
    /** User Management */
    std::vector<CPubKey> GetVerifiedUsers() const;
    std::vector<CPubKey> GetPendingUsers() const;
    std::vector<CPubKey> GetEndorsementCandidates(const CPubKey& user_key) const;
    
    /** Reputation System */
    void UpdateReputationScore(const CPubKey& user_key, double score_change);
    double GetReputationScore(const CPubKey& user_key) const;
    void ProcessEndorsementOutcome(const CPubKey& endorser, bool was_accurate);
    
    /** Challenge Resolution */
    std::vector<ChallengeRecord> GetActiveChallenges() const;
    std::vector<ChallengeRecord> GetChallengesForEndorsement(uint256 endorsement_id) const;
    bool IsEndorsementChallenged(uint256 endorsement_id) const;
    
    /** Statistics and Monitoring */
    struct UserStats {
        uint32_t total_registered = 0;
        uint32_t total_verified = 0;
        uint32_t total_pending = 0;
        uint32_t total_suspended = 0;
        uint32_t total_blacklisted = 0;
        double average_reputation = 0.0;
        uint32_t active_challenges = 0;
        uint32_t resolved_challenges = 0;
    };
    UserStats GetUserStatistics() const;
    
    /** Database Operations */
    bool LoadFromDatabase();
    bool SaveToDatabase();
    bool SyncWithBlockchain(uint32_t current_height);
    
    /** Configuration */
    void SetConsensusParams(const ConsensusParams& new_params);
    ConsensusParams GetConsensusParams() const { return params; }
    
private:
    /** Internal Helper Methods */
    bool ValidateEndorsement(const EndorsementRecord& endorsement) const;
    bool ValidateChallenge(const ChallengeRecord& challenge) const;
    bool CheckEndorsementThresholds(const CPubKey& user_key) const;
    void UpdateUserStatus(const CPubKey& user_key);
    std::vector<CPubKey> SelectRandomEndorsers(uint32_t count, const CPubKey& exclude_user) const;
    double CalculateEndorsementWeight(const EndorsementRecord& endorsement) const;
    
    /** Anti-Sybil Measures */
    bool CheckRegistrationIP(const std::string& ip_hash) const;
    bool CheckEndorsementPatterns(const CPubKey& endorser) const;
    bool DetectCollusionPattern(const std::vector<CPubKey>& endorsers) const;
};

/** Global user registry consensus instance */
extern UserRegistryConsensus g_user_consensus;

#endif // BITCOIN_CONSENSUS_USER_CONSENSUS_H
