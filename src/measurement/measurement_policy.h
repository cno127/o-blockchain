// Copyright (c) 2024 The Bitcoin Core developers
// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MEASUREMENT_MEASUREMENT_POLICY_H
#define BITCOIN_MEASUREMENT_MEASUREMENT_POLICY_H

#include <string>
#include <vector>
#include <map>
#include <consensus/geographic_access_control.h>
#include <pubkey.h>

/**
 * Measurement Policy for Geographic Regions
 * 
 * This system defines how measurements are collected and rewarded across
 * different geographic regions, including blocked/banned jurisdictions.
 * 
 * Key Principles:
 * 1. Water price data should be monitored GLOBALLY (even in banned regions)
 * 2. Local users in blocked regions cannot receive transaction rewards
 * 3. External users can safely measure data in blocked regions
 * 4. Stabilization mechanism requires global water price data
 */

/** Measurement Collection Strategy for a Region */
enum class MeasurementStrategy {
    LOCAL_ONLY,          // Only local users can submit measurements (normal regions)
    EXTERNAL_ONLY,       // Only external users can submit measurements (blocked regions)
    MIXED,               // Both local and external users can submit (monitored regions)
    BOT_ONLY             // Only automated bots can collect data (extremely restricted)
};

/** Reward Distribution Policy for a Region */
enum class RewardPolicy {
    FULL_REWARDS,        // Full rewards paid via blockchain transactions
    NO_TRANSACTION,      // Rewards tracked but not paid as transactions (blocked regions)
    EXTERNAL_ONLY,       // Only external users receive rewards
    REPUTATION_ONLY      // Only reputation points, no monetary rewards
};

/** Measurement Policy for a Specific Region */
struct RegionMeasurementPolicy {
    std::string country_code;
    std::string country_name;
    AccessLevel access_level;
    MeasurementStrategy collection_strategy;
    RewardPolicy reward_policy;
    
    // Measurement eligibility
    bool allow_local_measurements;      // Can local users submit?
    bool allow_external_measurements;   // Can external users submit?
    bool allow_bot_measurements;        // Can bots collect data?
    
    // Reward eligibility
    bool pay_local_rewards;             // Pay rewards to local users?
    bool pay_external_rewards;          // Pay rewards to external users?
    bool track_unpaid_rewards;          // Track rewards that can't be paid?
    
    // Invitation eligibility
    bool send_local_invites;            // Send invites to local users?
    bool send_external_invites;         // Send invites to external users?
    
    // Legal considerations
    bool legal_risk_local;              // Is there legal risk for local users?
    bool legal_risk_external;           // Is there legal risk for external users?
    std::string legal_notes;            // Legal considerations
    
    RegionMeasurementPolicy()
        : country_code(), country_name(), access_level(AccessLevel::BLOCKED),
          collection_strategy(MeasurementStrategy::EXTERNAL_ONLY),
          reward_policy(RewardPolicy::NO_TRANSACTION),
          allow_local_measurements(false), allow_external_measurements(true),
          allow_bot_measurements(true), pay_local_rewards(false),
          pay_external_rewards(true), track_unpaid_rewards(true),
          send_local_invites(false), send_external_invites(true),
          legal_risk_local(true), legal_risk_external(false), legal_notes() {}
};

/** Measurement Policy Manager */
class MeasurementPolicyManager {
public:
    MeasurementPolicyManager();
    ~MeasurementPolicyManager();
    
    /** Initialize with default policies */
    void Initialize();
    
    /** Get policy for a region */
    RegionMeasurementPolicy GetRegionPolicy(const std::string& country_code) const;
    
    /** Check if user can submit measurement based on their location and target region */
    bool CanUserSubmitMeasurement(const CPubKey& user, const std::string& user_country, 
                                   const std::string& target_region) const;
    
    /** Check if user should receive rewards for measurement */
    bool ShouldReceiveReward(const CPubKey& user, const std::string& user_country, 
                             const std::string& target_region) const;
    
    /** Check if invite should be sent to user for a region */
    bool ShouldSendInvite(const CPubKey& user, const std::string& user_country, 
                         const std::string& target_region) const;
    
    /** Get collection strategy for a region */
    MeasurementStrategy GetCollectionStrategy(const std::string& country_code) const;
    
    /** Get reward policy for a region */
    RewardPolicy GetRewardPolicy(const std::string& country_code) const;
    
    /** Check if region requires external measurers */
    bool RequiresExternalMeasurers(const std::string& country_code) const;
    
    /** Get list of countries that need external measurements */
    std::vector<std::string> GetCountriesNeedingExternalMeasurements() const;
    
    /** Get eligible external users for a blocked region */
    std::vector<CPubKey> GetExternalMeasurers(const std::string& blocked_country) const;
    
    /** Check if user is considered "external" to a region */
    bool IsExternalToRegion(const CPubKey& user, const std::string& user_country, 
                           const std::string& target_region) const;
    
    /** Track unpaid reward for future claiming */
    void TrackUnpaidReward(const CPubKey& user, const std::string& country_code, 
                          int64_t amount, const std::string& reason);
    
    /** Get unpaid rewards for a user */
    std::map<std::string, int64_t> GetUnpaidRewards(const CPubKey& user) const;
    
    /** Update policy for a region */
    void UpdateRegionPolicy(const std::string& country_code, const RegionMeasurementPolicy& policy);
    
    /** Get statistics */
    std::map<std::string, int64_t> GetPolicyStatistics() const;
    
private:
    std::map<std::string, RegionMeasurementPolicy> m_region_policies;
    std::map<CPubKey, std::map<std::string, int64_t>> m_unpaid_rewards; // user -> country -> amount
    
    void LoadDefaultPolicies();
    void InitializeBlockedRegionPolicies();
    void InitializeMonitoredRegionPolicies();
    void InitializeAllowedRegionPolicies();
};

/** Global measurement policy manager instance */
extern MeasurementPolicyManager g_measurement_policy;

#endif // BITCOIN_MEASUREMENT_MEASUREMENT_POLICY_H

