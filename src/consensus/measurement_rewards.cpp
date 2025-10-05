// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/measurement_rewards.h>
#include <measurement/measurement_system.h>
#include <logging.h>
#include <util/time.h>
#include <hash.h>
#include <key_io.h>
#include <addresstype.h>

namespace OConsensus {

MeasurementRewardsManager g_measurement_rewards_manager;

MeasurementRewardsManager::MeasurementRewardsManager() {
    LogPrintf("O Measurement Rewards: Initialized measurement rewards manager\n");
}

uint256 MeasurementRewardTransaction::GetHash() const {
    HashWriter ss{};
    ss << measurement_id << recipient << reward_amount << static_cast<uint8_t>(measurement_type) << timestamp;
    return ss.GetHash();
}

bool MeasurementRewardTransaction::IsValid() const {
    return !measurement_id.IsNull() && 
           recipient.IsValid() && 
           reward_amount > 0 && 
           timestamp > 0 && 
           block_height > 0;
}

std::vector<CMutableTransaction> MeasurementRewardsManager::CreateMeasurementRewardTransactions(
    const CBlock& block, int height) {
    
    std::vector<CMutableTransaction> reward_transactions;
    
    // Get unrewarded measurements from the measurement system
    auto unrewarded_measurements = GetUnrewardedMeasurements(height);
    
    LogPrintf("O Measurement Rewards: Found %d unrewarded measurements for block %d\n", 
              static_cast<int>(unrewarded_measurements.size()), height);
    
    for (const auto& reward : unrewarded_measurements) {
        if (reward.IsValid()) {
            CMutableTransaction reward_tx = CreateRewardTransaction(reward);
            if (!reward_tx.vin.empty() || !reward_tx.vout.empty()) {
                reward_transactions.push_back(reward_tx);
                
                // Track reward statistics
                std::string type_name = "unknown";
                switch (reward.measurement_type) {
                    case MeasurementType::WATER_PRICE:
                        type_name = "water_price";
                        break;
                    case MeasurementType::WATER_PRICE_OFFLINE_VALIDATION:
                        type_name = "water_price_offline_validation";
                        break;
                    case MeasurementType::EXCHANGE_RATE:
                        type_name = "exchange_rate";
                        break;
                    case MeasurementType::EXCHANGE_RATE_OFFLINE_VALIDATION:
                        type_name = "exchange_rate_offline_validation";
                        break;
                    case MeasurementType::URL_SUBMISSION:
                        type_name = "url_submission";
                        break;
                    case MeasurementType::ONLINE_WATER_PRICE_VALIDATION:
                    case MeasurementType::ONLINE_EXCHANGE_VALIDATION:
                        type_name = "online_validation";
                        break;
                    case MeasurementType::ONLINE_WATER_PRICE_MEASUREMENT:
                        type_name = "online_water_price_measurement";
                        break;
                    case MeasurementType::OFFLINE_WATER_PRICE_MEASUREMENT:
                        type_name = "offline_water_price_measurement";
                        break;
                    case MeasurementType::ONLINE_EXCHANGE_RATE_MEASUREMENT:
                        type_name = "online_exchange_rate_measurement";
                        break;
                    case MeasurementType::OFFLINE_EXCHANGE_RATE_MEASUREMENT:
                        type_name = "offline_exchange_rate_measurement";
                        break;
                }
                
                m_reward_stats[type_name]++;
                
                LogPrintf("O Measurement Rewards: Created reward transaction for measurement %s - "
                         "Type: %s, Amount: %d, Recipient: %s\n",
                         reward.measurement_id.GetHex().substr(0, 8).c_str(),
                         type_name.c_str(),
                         reward.reward_amount,
                         EncodeDestination(PKHash(reward.recipient.GetID())).c_str());
            }
        }
    }
    
    return reward_transactions;
}

CAmount MeasurementRewardsManager::CalculateTotalMeasurementRewards(const CBlock& block, int height) const {
    auto unrewarded_measurements = GetUnrewardedMeasurements(height);
    
    CAmount total_rewards = 0;
    for (const auto& reward : unrewarded_measurements) {
        if (reward.IsValid()) {
            total_rewards += reward.reward_amount;
        }
    }
    
    return total_rewards;
}

std::vector<MeasurementRewardTransaction> MeasurementRewardsManager::GetPendingMeasurementRewards() const {
    return m_pending_rewards;
}

void MeasurementRewardsManager::ProcessMeasurementRewards(
    const std::vector<MeasurementRewardTransaction>& rewards, int height) {
    
    for (const auto& reward : rewards) {
        if (reward.IsValid()) {
            m_pending_rewards.push_back(reward);
            
            LogPrintf("O Measurement Rewards: Processed reward for measurement %s at height %d\n",
                     reward.measurement_id.GetHex().substr(0, 8).c_str(), height);
        }
    }
}

void MeasurementRewardsManager::ClearProcessedRewards() {
    m_pending_rewards.clear();
    LogPrintf("O Measurement Rewards: Cleared processed rewards\n");
}

std::map<std::string, int> MeasurementRewardsManager::GetMeasurementRewardStats() const {
    return m_reward_stats;
}

CMutableTransaction MeasurementRewardsManager::CreateRewardTransaction(
    const MeasurementRewardTransaction& reward) const {
    
    CMutableTransaction tx;
    
    // Create a coinbase-like transaction for measurement rewards
    // No inputs needed - these are newly created coins
    
    // Create output to the measurement contributor
    CScript reward_script = GetScriptForDestination(PKHash(reward.recipient.GetID()));
    tx.vout.push_back(CTxOut(reward.reward_amount, reward_script));
    
    // Add metadata to the transaction
    // We can use the nLockTime field to store the measurement ID hash
    tx.nLockTime = reward.measurement_id.GetUint64(0);
    
    return tx;
}

std::vector<MeasurementRewardTransaction> MeasurementRewardsManager::GetUnrewardedMeasurements(int height) const {
    std::vector<MeasurementRewardTransaction> unrewarded_rewards;
    
    // Get all water price measurements that haven't been rewarded yet
    // This would need to be implemented in the measurement system
    // For now, we'll create a placeholder implementation
    
    // TODO: Implement actual integration with measurement system to get unrewarded measurements
    // This would involve:
    // 1. Getting all measurements from the measurement system
    // 2. Checking which ones haven't been rewarded yet
    // 3. Creating reward transactions for them
    
    LogPrintf("O Measurement Rewards: Getting unrewarded measurements for height %d\n", height);
    
    return unrewarded_rewards;
}

} // namespace OConsensus
