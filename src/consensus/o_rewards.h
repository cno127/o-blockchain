// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_O_REWARDS_H
#define BITCOIN_CONSENSUS_O_REWARDS_H

#include <consensus/amount.h>

namespace ORewards {

/**
 * O Blockchain Mining Rewards
 * 
 * All rewards use 2 decimal places (100 cents per O)
 * 1 O = 100 units (like USD: 1 dollar = 100 cents)
 */

/** Block Mining Rewards - CONSTANT (No Halving) */
static constexpr CAmount BLOCK_REWARD = 5000;              // 50.00 O per block (PoW) - CONSTANT
static constexpr CAmount POB_BLOCK_REWARD = 4000;          // 40.00 O per block (PoB - 80% of PoW) - CONSTANT

/** Transaction Fees */
static constexpr CAmount MIN_TRANSACTION_FEE = 10;         // 0.10 O per transaction
static constexpr CAmount RECOMMENDED_FEE = 10;             // 0.10 O recommended

/** Supply Limits */
// O Blockchain has UNLIMITED supply (unlike Bitcoin)
// Rationale: As a stable coin pegged to water prices, supply must grow with:
// 1. Global water consumption/economy growth
// 2. Measurement participation
// 3. Stabilization needs
// A capped supply would cause deflation and hoarding (opposite of transactional goal)
static constexpr int64_t MAX_O_SUPPLY = 0;  // 0 = unlimited supply

/** Constant Block Reward (No Halving) */
inline CAmount GetBlockReward(int height) {
    // Block rewards remain constant for all heights
    // This maintains stability for the stablecoin
    return BLOCK_REWARD;
}

/** Get PoB block reward (constant 80% of PoW reward) */
inline CAmount GetPoBBlockReward(int height) {
    // PoB rewards also remain constant
    return POB_BLOCK_REWARD;
}

/** Calculate total supply at given height (unlimited) */
inline CAmount GetTotalSupply(int height) {
    // For unlimited supply, we don't track total supply
    // Each block creates constant rewards forever
    return 0;  // 0 = unlimited supply
}

/** Check if rewards are active (always true for unlimited supply) */
inline bool RewardsActive(int height) {
    return true;  // Rewards never stop for unlimited supply
}

} // namespace ORewards

#endif // BITCOIN_CONSENSUS_O_REWARDS_H

