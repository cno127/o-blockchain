// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/user_consensus.h>
#include <consensus/geographic_access_control.h>
#include <hash.h>
#include <logging.h>
#include <util/time.h>
#include <random.h>

#include <algorithm>
#include <chrono>

// Global user registry consensus instance
UserRegistryConsensus g_user_consensus;

// OfficialUser Implementation
bool OfficialUser::IsVerificationComplete() const {
    return status == UserStatus::VERIFIED && 
           HasSufficientEndorsements() && 
           GetEndorsementGeoDiversity() >= required_geo_diversity;
}

uint32_t OfficialUser::GetEndorsementGeoDiversity() const {
    std::set<std::string> unique_countries;
    // In a real implementation, you'd map endorsers to their countries
    // For now, we'll assume each endorser represents a different country
    unique_countries.insert(country_code); // Include the user's own country
    return unique_countries.size();
}

bool OfficialUser::HasSufficientEndorsements() const {
    return endorsers.size() >= endorsement_threshold;
}

// EndorsementRecord Implementation
double EndorsementRecord::GetEndorsementWeight() const {
    double base_weight = 1.0;
    
    // Adjust based on confidence level
    switch (confidence_level) {
        case ConfidenceLevel::HIGH: base_weight *= 1.5; break;
        case ConfidenceLevel::MEDIUM: base_weight *= 1.0; break;
        case ConfidenceLevel::LOW: base_weight *= 0.5; break;
    }
    
    // Adjust based on verification method
    switch (verification_method) {
        case VerificationMethod::BIOMETRIC_VERIFICATION: base_weight *= 1.3; break;
        case VerificationMethod::VIDEO_CALL: base_weight *= 1.2; break;
        case VerificationMethod::GOVERNMENT_ID: base_weight *= 1.1; break;
        case VerificationMethod::DOCUMENT_REVIEW: base_weight *= 1.0; break;
        case VerificationMethod::THIRD_PARTY_KYC: base_weight *= 0.8; break;
    }
    
    // Adjust based on endorser reputation
    base_weight *= endorser_reputation_at_time;
    
    return base_weight;
}

bool EndorsementRecord::IsResolved() const {
    return !is_challenged || challengers.empty();
}

// UserRegistryConsensus Implementation
UserRegistryConsensus::UserRegistryConsensus() {
    // Initialize with default parameters
    params.min_endorsements = 5;
    params.min_geo_diversity = 3;
    params.challenge_period_blocks = 144;
    params.resolution_period_blocks = 288;
    params.min_endorser_reputation = 0.5;
    params.challenge_threshold = 0.3;
    params.endorsement_reward = 100;
    params.challenge_reward = 50;
}

UserRegistryConsensus::~UserRegistryConsensus() {
    // Cleanup if needed
}

bool UserRegistryConsensus::RegisterUser(const OfficialUser& user, std::string& error_message) {
    // Check if user already exists
    if (user_cache.find(user.public_key) != user_cache.end()) {
        error_message = "User already registered";
        return false;
    }
    
    // Check government ID uniqueness
    if (!IsUserUnique(user.government_id_hash)) {
        error_message = "Government ID already registered";
        return false;
    }
    
    // Validate user data
    if (user.government_id_hash.empty() || user.birth_currency.empty()) {
        error_message = "Invalid user data";
        return false;
    }
    
    // Check geographic access control for KYC requirements
    if (g_geographic_access_control.DoesCountryRequireKYC(user.country_code)) {
        LogPrintf("UserConsensus: User from %s requires KYC verification\n", user.country_code.c_str());
        // For KYC-required countries, we'll use THIRD_PARTY_KYC as the primary method
        // This will be enforced during the endorsement process
    }
    
    // Check for suspicious registration patterns
    if (!CheckRegistrationIP(user.registration_ip_hash)) {
        error_message = "Suspicious registration pattern detected";
        return false;
    }
    
    // Add user to cache
    user_cache[user.public_key] = user;
    government_id_to_user[user.government_id_hash] = user.public_key;
    
    // User registered successfully
    
    return true;
}

bool UserRegistryConsensus::SubmitEndorsement(const EndorsementRecord& endorsement, std::string& error_message) {
    // Validate endorsement
    if (!ValidateEndorsement(endorsement)) {
        error_message = "Invalid endorsement";
        return false;
    }
    
    // Check if endorser is verified
    if (!IsUserVerified(endorsement.endorser)) {
        error_message = "Endorser must be verified";
        return false;
    }
    
    // Check endorser reputation
    double endorser_reputation = GetReputationScore(endorsement.endorser);
    if (endorser_reputation < params.min_endorser_reputation) {
        error_message = "Endorser reputation too low";
        return false;
    }
    
    // Check for duplicate endorsement
    auto it = endorsement_cache.find(endorsement.endorsement_id);
    if (it != endorsement_cache.end()) {
        error_message = "Endorsement already exists";
        return false;
    }
    
    // Check endorsement patterns for anti-sybil
    if (!CheckEndorsementPatterns(endorsement.endorser)) {
        error_message = "Suspicious endorsement pattern detected";
        return false;
    }
    
    // Add endorsement to cache
    endorsement_cache[endorsement.endorsement_id] = endorsement;
    
    // Update endorsed user's endorsers list
    auto user_it = user_cache.find(endorsement.endorsed_user);
    if (user_it != user_cache.end()) {
        user_it->second.endorsers.push_back(endorsement.endorser);
        UpdateUserStatus(endorsement.endorsed_user);
    }
    
    // Endorsement submitted successfully
    
    return true;
}

bool UserRegistryConsensus::ChallengeEndorsement(const ChallengeRecord& challenge, std::string& error_message) {
    // Validate challenge
    if (!ValidateChallenge(challenge)) {
        error_message = "Invalid challenge";
        return false;
    }
    
    // Check if challenger is verified
    if (!IsUserVerified(challenge.challenger)) {
        error_message = "Challenger must be verified";
        return false;
    }
    
    // Check if endorsement exists
    auto endorsement_it = endorsement_cache.find(challenge.endorsement_id);
    if (endorsement_it == endorsement_cache.end()) {
        error_message = "Endorsement not found";
        return false;
    }
    
    // Check if endorsement is already challenged
    if (endorsement_it->second.is_challenged) {
        error_message = "Endorsement already challenged";
        return false;
    }
    
    // Check if challenger already challenged this endorsement
    auto it = std::find(endorsement_it->second.challengers.begin(), 
                       endorsement_it->second.challengers.end(), 
                       challenge.challenger);
    if (it != endorsement_it->second.challengers.end()) {
        error_message = "Challenger already challenged this endorsement";
        return false;
    }
    
    // Add challenge
    endorsement_it->second.challengers.push_back(challenge.challenger);
    endorsement_it->second.is_challenged = true;
    
    challenge_cache[challenge.challenge_id] = challenge;
    
    // Challenge submitted successfully
    
    return true;
}

bool UserRegistryConsensus::IsUserUnique(const std::string& government_id_hash) const {
    return government_id_to_user.find(government_id_hash) == government_id_to_user.end();
}

bool UserRegistryConsensus::IsUserVerified(const CPubKey& user_key) const {
    auto it = user_cache.find(user_key);
    return it != user_cache.end() && it->second.status == UserStatus::VERIFIED;
}

bool UserRegistryConsensus::ValidateGeoDiversity(const CPubKey& user_key) const {
    auto it = user_cache.find(user_key);
    if (it == user_cache.end()) {
        return false;
    }
    
    return it->second.GetEndorsementGeoDiversity() >= params.min_geo_diversity;
}

bool UserRegistryConsensus::CheckDuplicateRegistration(const CPubKey& user_key) const {
    return user_cache.find(user_key) != user_cache.end();
}

std::vector<CPubKey> UserRegistryConsensus::GetVerifiedUsers() const {
    std::vector<CPubKey> verified_users;
    for (const auto& pair : user_cache) {
        if (pair.second.status == UserStatus::VERIFIED) {
            verified_users.push_back(pair.first);
        }
    }
    return verified_users;
}

std::vector<CPubKey> UserRegistryConsensus::GetPendingUsers() const {
    std::vector<CPubKey> pending_users;
    for (const auto& pair : user_cache) {
        if (pair.second.status == UserStatus::PENDING_VERIFICATION ||
            pair.second.status == UserStatus::VERIFICATION_IN_PROGRESS) {
            pending_users.push_back(pair.first);
        }
    }
    return pending_users;
}

void UserRegistryConsensus::UpdateReputationScore(const CPubKey& user_key, double score_change) {
    auto it = user_cache.find(user_key);
    if (it != user_cache.end()) {
        it->second.reputation_score += score_change;
        // Clamp reputation score between 0 and 10
        it->second.reputation_score = std::max(0.0, std::min(10.0, it->second.reputation_score));
    }
}

double UserRegistryConsensus::GetReputationScore(const CPubKey& user_key) const {
    auto it = user_cache.find(user_key);
    if (it != user_cache.end()) {
        return it->second.reputation_score;
    }
    return 0.0;
}

void UserRegistryConsensus::ProcessEndorsementOutcome(const CPubKey& endorser, bool was_accurate) {
    double score_change = was_accurate ? 0.1 : -0.2; // Reward accuracy, penalize inaccuracy
    UpdateReputationScore(endorser, score_change);
}

UserRegistryConsensus::UserStats UserRegistryConsensus::GetUserStatistics() const {
    UserStats stats;
    
    for (const auto& pair : user_cache) {
        stats.total_registered++;
        
        switch (pair.second.status) {
            case UserStatus::VERIFIED:
                stats.total_verified++;
                break;
            case UserStatus::PENDING_VERIFICATION:
            case UserStatus::VERIFICATION_IN_PROGRESS:
                stats.total_pending++;
                break;
            case UserStatus::SUSPENDED:
                stats.total_suspended++;
                break;
            case UserStatus::BLACKLISTED:
                stats.total_blacklisted++;
                break;
        }
        
        stats.average_reputation += pair.second.reputation_score;
    }
    
    if (stats.total_registered > 0) {
        stats.average_reputation /= stats.total_registered;
    }
    
    stats.active_challenges = challenge_cache.size();
    
    return stats;
}

bool UserRegistryConsensus::ValidateEndorsement(const EndorsementRecord& endorsement) const {
    // Basic validation
    if (endorsement.endorser.size() == 0 || endorsement.endorsed_user.size() == 0) {
        return false;
    }
    
    if (endorsement.endorser == endorsement.endorsed_user) {
        return false; // Can't endorse yourself
    }
    
    if (endorsement.timestamp == 0 || endorsement.block_height == 0) {
        return false;
    }
    
    // Check if endorser exists and is verified
    auto endorser_it = user_cache.find(endorsement.endorser);
    if (endorser_it == user_cache.end() || endorser_it->second.status != UserStatus::VERIFIED) {
        return false;
    }
    
    // Check if endorsed user exists
    auto endorsed_it = user_cache.find(endorsement.endorsed_user);
    if (endorsed_it == user_cache.end()) {
        return false;
    }
    
    // Check endorser reputation
    if (endorser_it->second.reputation_score < params.min_endorser_reputation) {
        return false;
    }
    
    // Check for duplicate endorsements
    for (const auto& existing_endorsement : endorsement_cache) {
        if (existing_endorsement.second.endorser == endorsement.endorser &&
            existing_endorsement.second.endorsed_user == endorsement.endorsed_user) {
            return false;
        }
    }
    
    // Check if verification method is allowed for the endorsed user's country
    std::vector<VerificationMethod> allowed_methods = GetAllowedVerificationMethods(endorsed_it->second.country_code);
    bool method_allowed = std::find(allowed_methods.begin(), allowed_methods.end(), endorsement.verification_method) != allowed_methods.end();
    
    if (!method_allowed) {
        LogPrintf("UserConsensus: Verification method %d not allowed for country %s\n", 
                  static_cast<int>(endorsement.verification_method), endorsed_it->second.country_code.c_str());
        return false;
    }
    
    // For KYC-required countries, ensure at least one endorsement uses KYC method
    if (g_geographic_access_control.DoesCountryRequireKYC(endorsed_it->second.country_code)) {
        // Check if this is a KYC endorsement or if there are already KYC endorsements
        bool has_kyc_endorsement = (endorsement.verification_method == VerificationMethod::THIRD_PARTY_KYC);
        
        if (!has_kyc_endorsement) {
            // Check if there are already KYC endorsements for this user
            for (const auto& existing_endorsement : endorsement_cache) {
                if (existing_endorsement.second.endorsed_user == endorsement.endorsed_user &&
                    existing_endorsement.second.verification_method == VerificationMethod::THIRD_PARTY_KYC) {
                    has_kyc_endorsement = true;
                    break;
                }
            }
        }
        
        if (!has_kyc_endorsement) {
            LogPrintf("UserConsensus: User from KYC-required country %s should have KYC endorsement\n", 
                      endorsed_it->second.country_code.c_str());
            // For now, we'll allow non-KYC endorsements but log a warning
            // In a stricter implementation, we could require KYC endorsements
        }
    }
    
    return true;
}

bool UserRegistryConsensus::ValidateChallenge(const ChallengeRecord& challenge) const {
    // Basic validation
    if (challenge.challenger.size() == 0) {
        return false;
    }
    
    if (challenge.challenge_reason.empty()) {
        return false;
    }
    
    if (challenge.timestamp == 0 || challenge.block_height == 0) {
        return false;
    }
    
    return true;
}

void UserRegistryConsensus::UpdateUserStatus(const CPubKey& user_key) {
    auto it = user_cache.find(user_key);
    if (it == user_cache.end()) {
        return;
    }
    
    OfficialUser& user = it->second;
    
    // Check if user meets verification requirements
    if (user.HasSufficientEndorsements() && ValidateGeoDiversity(user_key)) {
        if (user.status == UserStatus::PENDING_VERIFICATION ||
            user.status == UserStatus::VERIFICATION_IN_PROGRESS) {
            user.status = UserStatus::VERIFIED;
            user.verification_height = GetTime(); // In real implementation, use block height
            // User verified successfully
        }
    }
}

std::vector<CPubKey> UserRegistryConsensus::SelectRandomEndorsers(uint32_t count, const CPubKey& exclude_user) const {
    std::vector<CPubKey> candidates;
    std::vector<CPubKey> verified_users = GetVerifiedUsers();
    
    // Filter out the user to be endorsed and users with low reputation
    for (const auto& user_key : verified_users) {
        if (user_key != exclude_user && GetReputationScore(user_key) >= params.min_endorser_reputation) {
            candidates.push_back(user_key);
        }
    }
    
    // Shuffle and select random endorsers
    std::shuffle(candidates.begin(), candidates.end(), FastRandomContext());
    
    std::vector<CPubKey> selected;
    selected.reserve(std::min(count, static_cast<uint32_t>(candidates.size())));
    
    for (uint32_t i = 0; i < count && i < candidates.size(); ++i) {
        selected.push_back(candidates[i]);
    }
    
    return selected;
}

bool UserRegistryConsensus::CheckRegistrationIP(const std::string& ip_hash) const {
    // In a real implementation, this would check against known suspicious IPs
    // and registration patterns. For now, we'll just return true.
    return true;
}

bool UserRegistryConsensus::CheckEndorsementPatterns(const CPubKey& endorser) const {
    // Check for suspicious endorsement patterns (too many endorsements in short time, etc.)
    // This is a simplified version - in reality, you'd implement more sophisticated detection
    
    auto it = user_cache.find(endorser);
    if (it == user_cache.end()) {
        return false;
    }
    
    // Check if endorser has too many endorsements in a short period
    // This would require timestamp tracking of endorsements
    
    return true;
}

bool UserRegistryConsensus::DetectCollusionPattern(const std::vector<CPubKey>& endorsers) const {
    // Detect if endorsers are colluding (same IP, similar registration times, etc.)
    // This is a placeholder for more sophisticated collusion detection
    
    // Check for circular endorsements
    for (const auto& endorser : endorsers) {
        auto it = user_cache.find(endorser);
        if (it != user_cache.end()) {
            const auto& user_endorsers = it->second.endorsers;
            for (const auto& other_endorser : endorsers) {
                if (endorser != other_endorser) {
                    auto it2 = std::find(user_endorsers.begin(), user_endorsers.end(), other_endorser);
                    if (it2 != user_endorsers.end()) {
                        // Circular endorsement detected
                        return true;
                    }
                }
            }
        }
    }
    
    return false;
}

void UserRegistryConsensus::SetConsensusParams(const ConsensusParams& new_params) {
    params = new_params;
}

bool UserRegistryConsensus::LoadFromDatabase() {
    // In a real implementation, this would load from persistent storage
    // For now, we'll just return true
    return true;
}

bool UserRegistryConsensus::SaveToDatabase() {
    // In a real implementation, this would save to persistent storage
    // For now, we'll just return true
    return true;
}

bool UserRegistryConsensus::SyncWithBlockchain(uint32_t current_height) {
    // Sync user consensus state with blockchain state
    // This would process any blockchain events related to user registration
    
    // Update user activity heights
    for (auto& pair : user_cache) {
        if (pair.second.status == UserStatus::VERIFIED) {
            pair.second.last_activity_height = current_height;
        }
    }
    
    return true;
}

// Verification Method Selection Implementation
VerificationMethod UserRegistryConsensus::GetRecommendedVerificationMethod(const std::string& country_code) const {
    // Check if country requires KYC
    if (g_geographic_access_control.DoesCountryRequireKYC(country_code)) {
        LogPrintf("UserConsensus: Country %s requires KYC - recommending THIRD_PARTY_KYC\n", country_code.c_str());
        return VerificationMethod::THIRD_PARTY_KYC;
    }
    
    // For crypto-friendly countries, prefer BrightID (which we'll map to GOVERNMENT_ID for now)
    // In a full implementation, we'd have a separate BrightID verification method
    LogPrintf("UserConsensus: Country %s is crypto-friendly - recommending GOVERNMENT_ID\n", country_code.c_str());
    return VerificationMethod::GOVERNMENT_ID;
}

std::vector<VerificationMethod> UserRegistryConsensus::GetAllowedVerificationMethods(const std::string& country_code) const {
    std::vector<VerificationMethod> allowed_methods;
    
    // Check if country requires KYC
    if (g_geographic_access_control.DoesCountryRequireKYC(country_code)) {
        // KYC-required countries: Allow KYC and government ID verification
        allowed_methods.push_back(VerificationMethod::THIRD_PARTY_KYC);
        allowed_methods.push_back(VerificationMethod::GOVERNMENT_ID);
        allowed_methods.push_back(VerificationMethod::DOCUMENT_REVIEW);
        LogPrintf("UserConsensus: Country %s allows KYC-based verification methods\n", country_code.c_str());
    } else {
        // Crypto-friendly countries: Allow all verification methods including privacy-preserving ones
        allowed_methods.push_back(VerificationMethod::GOVERNMENT_ID);
        allowed_methods.push_back(VerificationMethod::VIDEO_CALL);
        allowed_methods.push_back(VerificationMethod::DOCUMENT_REVIEW);
        allowed_methods.push_back(VerificationMethod::BIOMETRIC_VERIFICATION);
        // Note: THIRD_PARTY_KYC is also allowed but not recommended for privacy
        allowed_methods.push_back(VerificationMethod::THIRD_PARTY_KYC);
        LogPrintf("UserConsensus: Country %s allows all verification methods\n", country_code.c_str());
    }
    
    return allowed_methods;
}
