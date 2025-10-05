// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_MEASUREMENT_REWARDS_H
#define BITCOIN_CONSENSUS_MEASUREMENT_REWARDS_H

#include <consensus/amount.h>
#include <primitives/transaction.h>
#include <primitives/block.h>
#include <pubkey.h>
#include <measurement/measurement_system.h>

using namespace OMeasurement;

namespace OConsensus {

/** Measurement Reward Transaction */
struct MeasurementRewardTransaction {
    uint256 measurement_id;
    CPubKey recipient;
    CAmount reward_amount;
    MeasurementType measurement_type;
    int64_t timestamp;
    int block_height;
    
    MeasurementRewardTransaction()
        : measurement_id(), recipient(), reward_amount(0), 
          measurement_type(MeasurementType::WATER_PRICE), timestamp(0), block_height(0) {}
    
    SERIALIZE_METHODS(MeasurementRewardTransaction, obj) {
        uint8_t type_val = static_cast<uint8_t>(obj.measurement_type);
        READWRITE(obj.measurement_id, obj.recipient, obj.reward_amount, type_val,
                  obj.timestamp, obj.block_height);
        if (ser_action.ForRead()) obj.measurement_type = static_cast<MeasurementType>(type_val);
    }
    
    uint256 GetHash() const;
    bool IsValid() const;
};

/** Measurement Rewards Manager */
class MeasurementRewardsManager {
public:
    MeasurementRewardsManager();
    
    /** Create measurement reward transactions for a block */
    std::vector<CMutableTransaction> CreateMeasurementRewardTransactions(const CBlock& block, int height);
    
    /** Calculate total measurement rewards for a block */
    CAmount CalculateTotalMeasurementRewards(const CBlock& block, int height) const;
    
    /** Get pending measurement rewards */
    std::vector<MeasurementRewardTransaction> GetPendingMeasurementRewards() const;
    
    /** Process measurement reward transactions */
    void ProcessMeasurementRewards(const std::vector<MeasurementRewardTransaction>& rewards, int height);
    
    /** Clear processed rewards */
    void ClearProcessedRewards();
    
    /** Get measurement reward statistics */
    std::map<std::string, int> GetMeasurementRewardStats() const;

private:
    std::vector<MeasurementRewardTransaction> m_pending_rewards;
    std::map<std::string, int> m_reward_stats;
    
    /** Create reward transaction for a single measurement */
    CMutableTransaction CreateRewardTransaction(const MeasurementRewardTransaction& reward) const;
    
    /** Get measurements that need rewards from the measurement system */
    std::vector<MeasurementRewardTransaction> GetUnrewardedMeasurements(int height) const;
};

/** Global measurement rewards manager */
extern MeasurementRewardsManager g_measurement_rewards_manager;

} // namespace OConsensus

#endif // BITCOIN_CONSENSUS_MEASUREMENT_REWARDS_H
