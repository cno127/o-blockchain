// Copyright (c) 2024 The Bitcoin Core developers
// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <measurement/measurement_policy.h>
#include <consensus/geographic_access_control.h>
#include <logging.h>

// Global instance
MeasurementPolicyManager g_measurement_policy;

MeasurementPolicyManager::MeasurementPolicyManager() {
    // Constructor
}

MeasurementPolicyManager::~MeasurementPolicyManager() {
    // Destructor
}

void MeasurementPolicyManager::Initialize() {
    LogPrintf("O Measurement Policy: Initializing global measurement policies\n");
    LoadDefaultPolicies();
    LogPrintf("O Measurement Policy: Initialized %d region policies\n", static_cast<int>(m_region_policies.size()));
}

void MeasurementPolicyManager::LoadDefaultPolicies() {
    InitializeBlockedRegionPolicies();
    InitializeMonitoredRegionPolicies();
    InitializeAllowedRegionPolicies();
}

void MeasurementPolicyManager::InitializeBlockedRegionPolicies() {
    // Blocked countries: China, India, Bangladesh, Nepal, Bolivia, Ecuador
    // Strategy: Use external users + bots, no local participation, no transaction rewards to locals
    
    std::vector<std::pair<std::string, std::string>> blocked_countries = {
        {"CN", "China"},
        {"IN", "India"},
        {"BD", "Bangladesh"},
        {"NP", "Nepal"},
        {"BO", "Bolivia"},
        {"EC", "Ecuador"}
    };
    
    for (const auto& [code, name] : blocked_countries) {
        RegionMeasurementPolicy policy;
        policy.country_code = code;
        policy.country_name = name;
        policy.access_level = AccessLevel::BLOCKED;
        policy.collection_strategy = MeasurementStrategy::EXTERNAL_ONLY;
        policy.reward_policy = RewardPolicy::EXTERNAL_ONLY;
        
        // Measurement eligibility
        policy.allow_local_measurements = false;      // Protect local users from legal risk
        policy.allow_external_measurements = true;    // External users can safely measure
        policy.allow_bot_measurements = true;         // Bots can collect data
        
        // Reward eligibility
        policy.pay_local_rewards = false;             // Cannot send transactions to blocked regions
        policy.pay_external_rewards = true;           // External users receive full rewards
        policy.track_unpaid_rewards = true;           // Track what locals would have earned
        
        // Invitation eligibility  
        policy.send_local_invites = false;            // Don't invite locals (legal risk)
        policy.send_external_invites = true;          // Invite external users
        
        // Legal considerations
        policy.legal_risk_local = true;               // High legal risk for locals
        policy.legal_risk_external = false;           // Low risk for external users
        policy.legal_notes = "Crypto banned - local participation prohibited, external monitoring allowed";
        
        m_region_policies[code] = policy;
        
        LogPrintf("O Measurement Policy: Set BLOCKED policy for %s (%s) - External-only measurements\n", 
                  code.c_str(), name.c_str());
    }
}

void MeasurementPolicyManager::InitializeMonitoredRegionPolicies() {
    // Monitored countries: USA, UK, etc.
    // Strategy: Mixed approach, full rewards but with compliance monitoring
    
    std::vector<std::pair<std::string, std::string>> monitored_countries = {
        {"US", "United States"},
        {"GB", "United Kingdom"}
    };
    
    for (const auto& [code, name] : monitored_countries) {
        RegionMeasurementPolicy policy;
        policy.country_code = code;
        policy.country_name = name;
        policy.access_level = AccessLevel::MONITORED;
        policy.collection_strategy = MeasurementStrategy::MIXED;
        policy.reward_policy = RewardPolicy::FULL_REWARDS;
        
        // Measurement eligibility
        policy.allow_local_measurements = true;       // Locals can participate
        policy.allow_external_measurements = true;    // External users also welcome
        policy.allow_bot_measurements = true;         // Bots can collect data
        
        // Reward eligibility
        policy.pay_local_rewards = true;              // Full rewards with compliance
        policy.pay_external_rewards = true;           // External users also rewarded
        policy.track_unpaid_rewards = false;          // All rewards paid
        
        // Invitation eligibility
        policy.send_local_invites = true;             // Invite locals
        policy.send_external_invites = true;          // Also invite external users
        
        // Legal considerations
        policy.legal_risk_local = false;              // Low risk with KYC compliance
        policy.legal_risk_external = false;           // Low risk for external users
        policy.legal_notes = "KYC required for locals, full access with compliance";
        
        m_region_policies[code] = policy;
        
        LogPrintf("O Measurement Policy: Set MONITORED policy for %s (%s) - Mixed measurements with compliance\n", 
                  code.c_str(), name.c_str());
    }
}

void MeasurementPolicyManager::InitializeAllowedRegionPolicies() {
    // Allowed countries: Switzerland, Singapore, UAE, Portugal, Germany, etc.
    // Strategy: Full local participation, full rewards
    
    std::vector<std::pair<std::string, std::string>> allowed_countries = {
        {"CH", "Switzerland"},
        {"SG", "Singapore"},
        {"AE", "United Arab Emirates"},
        {"PT", "Portugal"},
        {"DE", "Germany"},
        {"AU", "Australia"},
        {"CA", "Canada"}
    };
    
    for (const auto& [code, name] : allowed_countries) {
        RegionMeasurementPolicy policy;
        policy.country_code = code;
        policy.country_name = name;
        policy.access_level = AccessLevel::ALLOWED;
        policy.collection_strategy = MeasurementStrategy::LOCAL_ONLY;
        policy.reward_policy = RewardPolicy::FULL_REWARDS;
        
        // Measurement eligibility
        policy.allow_local_measurements = true;       // Locals preferred
        policy.allow_external_measurements = true;    // External users also welcome
        policy.allow_bot_measurements = true;         // Bots can collect data
        
        // Reward eligibility
        policy.pay_local_rewards = true;              // Full rewards
        policy.pay_external_rewards = true;           // External users also rewarded
        policy.track_unpaid_rewards = false;          // All rewards paid
        
        // Invitation eligibility
        policy.send_local_invites = true;             // Invite locals
        policy.send_external_invites = false;         // Locals preferred, no need for external
        
        // Legal considerations
        policy.legal_risk_local = false;              // No legal risk
        policy.legal_risk_external = false;           // No legal risk
        policy.legal_notes = "Full crypto-friendly access";
        
        m_region_policies[code] = policy;
        
        LogPrintf("O Measurement Policy: Set ALLOWED policy for %s (%s) - Full local participation\n", 
                  code.c_str(), name.c_str());
    }
}

RegionMeasurementPolicy MeasurementPolicyManager::GetRegionPolicy(const std::string& country_code) const {
    auto it = m_region_policies.find(country_code);
    if (it != m_region_policies.end()) {
        return it->second;
    }
    
    // Default policy for unknown regions: cautious approach
    RegionMeasurementPolicy default_policy;
    default_policy.country_code = country_code;
    default_policy.country_name = "Unknown";
    default_policy.access_level = AccessLevel::RESTRICTED;
    default_policy.collection_strategy = MeasurementStrategy::MIXED;
    default_policy.reward_policy = RewardPolicy::FULL_REWARDS;
    default_policy.allow_local_measurements = true;
    default_policy.allow_external_measurements = true;
    default_policy.allow_bot_measurements = true;
    default_policy.pay_local_rewards = true;
    default_policy.pay_external_rewards = true;
    default_policy.send_local_invites = true;
    default_policy.send_external_invites = false;
    
    return default_policy;
}

bool MeasurementPolicyManager::CanUserSubmitMeasurement(const CPubKey& user, const std::string& user_country, 
                                                        const std::string& target_region) const {
    RegionMeasurementPolicy policy = GetRegionPolicy(target_region);
    
    // Is user local or external to target region?
    bool is_local = (user_country == target_region);
    bool is_external = !is_local;
    
    if (is_local && policy.allow_local_measurements) {
        LogPrintf("O Measurement Policy: Local user from %s can submit measurement\n", user_country.c_str());
        return true;
    }
    
    if (is_external && policy.allow_external_measurements) {
        LogPrintf("O Measurement Policy: External user (from %s) can submit measurement for %s\n", 
                  user_country.c_str(), target_region.c_str());
        return true;
    }
    
    LogPrintf("O Measurement Policy: User from %s CANNOT submit measurement for %s (policy restriction)\n", 
              user_country.c_str(), target_region.c_str());
    return false;
}

bool MeasurementPolicyManager::ShouldReceiveReward(const CPubKey& user, const std::string& user_country, 
                                                    const std::string& target_region) const {
    RegionMeasurementPolicy policy = GetRegionPolicy(target_region);
    
    // Is user local or external to target region?
    bool is_local = (user_country == target_region);
    bool is_external = !is_local;
    
    if (is_local && policy.pay_local_rewards) {
        LogPrintf("O Measurement Policy: Local user from %s will receive transaction reward\n", user_country.c_str());
        return true;
    }
    
    if (is_external && policy.pay_external_rewards) {
        LogPrintf("O Measurement Policy: External user (from %s) will receive transaction reward for %s measurement\n", 
                  user_country.c_str(), target_region.c_str());
        return true;
    }
    
    // Track unpaid reward if policy requires
    if (is_local && policy.track_unpaid_rewards && !policy.pay_local_rewards) {
        LogPrintf("O Measurement Policy: Tracking unpaid reward for user from blocked region %s\n", user_country.c_str());
        // Reward is earned but cannot be paid as transaction (tracked for future)
    }
    
    return false;
}

bool MeasurementPolicyManager::ShouldSendInvite(const CPubKey& user, const std::string& user_country, 
                                                const std::string& target_region) const {
    RegionMeasurementPolicy policy = GetRegionPolicy(target_region);
    
    // Is user local or external to target region?
    bool is_local = (user_country == target_region);
    bool is_external = !is_local;
    
    if (is_local && policy.send_local_invites) {
        return true;
    }
    
    if (is_external && policy.send_external_invites) {
        return true;
    }
    
    return false;
}

MeasurementStrategy MeasurementPolicyManager::GetCollectionStrategy(const std::string& country_code) const {
    RegionMeasurementPolicy policy = GetRegionPolicy(country_code);
    return policy.collection_strategy;
}

RewardPolicy MeasurementPolicyManager::GetRewardPolicy(const std::string& country_code) const {
    RegionMeasurementPolicy policy = GetRegionPolicy(country_code);
    return policy.reward_policy;
}

bool MeasurementPolicyManager::RequiresExternalMeasurers(const std::string& country_code) const {
    RegionMeasurementPolicy policy = GetRegionPolicy(country_code);
    return (policy.collection_strategy == MeasurementStrategy::EXTERNAL_ONLY);
}

std::vector<std::string> MeasurementPolicyManager::GetCountriesNeedingExternalMeasurements() const {
    std::vector<std::string> countries;
    
    for (const auto& [code, policy] : m_region_policies) {
        if (policy.collection_strategy == MeasurementStrategy::EXTERNAL_ONLY) {
            countries.push_back(code);
        }
    }
    
    return countries;
}

std::vector<CPubKey> MeasurementPolicyManager::GetExternalMeasurers(const std::string& blocked_country) const {
    // TODO: Implement when user registry is available
    // Get verified users from allowed/monitored regions who can safely measure blocked regions
    std::vector<CPubKey> external_users;
    
    LogPrintf("O Measurement Policy: Finding external measurers for blocked region %s\n", blocked_country.c_str());
    
    // Prefer users from nearby regions or those with travel history
    // For now, return empty vector until user registry is integrated
    
    return external_users;
}

bool MeasurementPolicyManager::IsExternalToRegion(const CPubKey& user, const std::string& user_country, 
                                                  const std::string& target_region) const {
    return (user_country != target_region);
}

void MeasurementPolicyManager::TrackUnpaidReward(const CPubKey& user, const std::string& country_code, 
                                                 int64_t amount, const std::string& reason) {
    m_unpaid_rewards[user][country_code] += amount;
    
    LogPrintf("O Measurement Policy: Tracked unpaid reward: %lld for user in %s (reason: %s)\n", 
              amount, country_code.c_str(), reason.c_str());
}

std::map<std::string, int64_t> MeasurementPolicyManager::GetUnpaidRewards(const CPubKey& user) const {
    auto it = m_unpaid_rewards.find(user);
    if (it != m_unpaid_rewards.end()) {
        return it->second;
    }
    return {};
}

void MeasurementPolicyManager::UpdateRegionPolicy(const std::string& country_code, const RegionMeasurementPolicy& policy) {
    m_region_policies[country_code] = policy;
    LogPrintf("O Measurement Policy: Updated policy for %s\n", country_code.c_str());
}

std::map<std::string, int64_t> MeasurementPolicyManager::GetPolicyStatistics() const {
    std::map<std::string, int64_t> stats;
    
    stats["total_regions"] = m_region_policies.size();
    stats["blocked_regions"] = 0;
    stats["monitored_regions"] = 0;
    stats["allowed_regions"] = 0;
    stats["external_only_regions"] = 0;
    stats["users_with_unpaid_rewards"] = m_unpaid_rewards.size();
    
    for (const auto& [code, policy] : m_region_policies) {
        if (policy.access_level == AccessLevel::BLOCKED) stats["blocked_regions"]++;
        if (policy.access_level == AccessLevel::MONITORED) stats["monitored_regions"]++;
        if (policy.access_level == AccessLevel::ALLOWED) stats["allowed_regions"]++;
        if (policy.collection_strategy == MeasurementStrategy::EXTERNAL_ONLY) stats["external_only_regions"]++;
    }
    
    return stats;
}

