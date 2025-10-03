// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_O_POW_POB_H
#define BITCOIN_CONSENSUS_O_POW_POB_H

#include <primitives/block.h>
#include <primitives/transaction.h>
#include <uint256.h>
#include <pubkey.h>
#include <map>
#include <set>
#include <vector>

class CBlockIndex;
class CChainParams;

namespace OConsensus {

/** Business Miner Qualification Thresholds */
static constexpr int64_t MIN_BUSINESS_TRANSACTIONS = 100;      // Per qualification period
static constexpr int64_t MIN_BUSINESS_DISTINCT_KEYS = 20;      // Unique recipients required
static constexpr int64_t BUSINESS_QUALIFICATION_PERIOD = 144 * 7;  // 1 week in blocks (~10 min blocks)
static constexpr int64_t MIN_BUSINESS_VOLUME = 1000000;       // Minimum transaction volume (10,000.00 O in satoshis)

/** Mining Difficulty Adjustments */
static constexpr double DIFFICULTY_REDUCTION_FACTOR = 0.5;     // Max 50% reduction at full business participation
static constexpr double MAX_BUSINESS_RATIO = 0.8;              // Cap at 80% business miners
static constexpr double BUSINESS_DIFFICULTY_BONUS = 0.1;       // 10% easier for qualified businesses

/** Block Rate Scaling - Fast Transactional Currency */
static constexpr int64_t TARGET_BLOCK_TIME = 12;               // 12 seconds (Ethereum-like for transactions)
static constexpr int BLOCKS_PER_HOUR = 300;                    // 300 blocks per hour (12-second blocks)
static constexpr int BLOCKS_PER_DAY = 7200;                    // 7,200 blocks per day
static constexpr int64_t TARGET_BLOCK_TIME_MIN = 12;           // 12 seconds minimum (fixed)
static constexpr int64_t TARGET_BLOCK_TIME_MAX = 12;           // 12 seconds maximum (fixed)

/** Business Miner Statistics */
struct BusinessMinerStats {
    uint256 miner_pubkey_hash;
    int64_t total_transactions;
    int64_t distinct_recipients;
    int64_t last_qualification_height;
    int64_t first_seen_height;
    bool is_qualified;
    CAmount transaction_volume;  // Total value processed
    std::set<uint256> recipient_set;  // Track unique recipients
    
    BusinessMinerStats() 
        : miner_pubkey_hash(), total_transactions(0), distinct_recipients(0),
          last_qualification_height(0), first_seen_height(0), is_qualified(false),
          transaction_volume(0) {}
    
    SERIALIZE_METHODS(BusinessMinerStats, obj) {
        READWRITE(obj.miner_pubkey_hash, obj.total_transactions, obj.distinct_recipients,
                  obj.last_qualification_height, obj.first_seen_height, obj.is_qualified,
                  obj.transaction_volume);
    }
};

/** Hybrid PoW/PoB Consensus Manager */
class HybridPowPobConsensus {
public:
    HybridPowPobConsensus();
    
    /** Check if a public key qualifies as a business miner */
    bool IsBusinessMiner(const uint256& pubkey_hash, int height) const;
    
    /** Calculate current business miner ratio */
    double GetBusinessRatio(int height) const;
    
    /** Get adjusted mining difficulty based on business participation */
    unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader* pblock, const CChainParams& params);
    
    /** Validate that block meets PoW requirements (considers PoB status) */
    bool CheckProofOfWork(uint256 hash, unsigned int nBits, bool is_business_miner, const CChainParams& params) const;
    
    /** Update business miner statistics from a transaction */
    void UpdateBusinessStats(const uint256& pubkey_hash, const CTransaction& tx, int height);
    
    /** Ensure business miners don't mine their own transactions */
    bool ValidateBusinessMinerBlock(const CBlock& block, const uint256& miner_pubkey) const;
    
    /** Get target block time based on business participation */
    int64_t GetTargetBlockTime(int height) const;
    
    /** Get business miner statistics */
    const BusinessMinerStats* GetBusinessStats(const uint256& pubkey_hash) const;
    
    /** Get all qualified business miners at a given height */
    std::vector<uint256> GetQualifiedBusinessMiners(int height) const;
    
    /** Re-evaluate business miner qualifications (call periodically) */
    void ReEvaluateQualifications(int current_height);
    
    /** Clear stale business miner data */
    void PruneOldData(int current_height);
    
    /** Get total number of qualified business miners */
    size_t GetQualifiedBusinessCount() const;
    
    /** Extract miner public key from coinbase transaction */
    static uint256 ExtractMinerPubKey(const CBlock& block);

private:
    std::map<uint256, BusinessMinerStats> m_business_miners;
    mutable std::map<int, double> m_cached_business_ratios;  // Cache ratios by height
    
    /** Calculate transaction statistics for a miner in the qualification period */
    BusinessMinerStats CalculateMinerStats(const uint256& pubkey_hash, int height) const;
    
    /** Check if miner meets diversity requirements */
    bool CheckMinerDiversity(const BusinessMinerStats& stats) const;
    
    /** Get the base difficulty (Bitcoin's standard algorithm) */
    unsigned int GetNextWorkRequired_Bitcoin(const CBlockIndex* pindexLast, const CBlockHeader* pblock, const CChainParams& params);
    
    /** Calculate difficulty adjustment factor based on business ratio */
    double CalculateDifficultyAdjustment(double business_ratio) const;
};

/** Global instance (initialized in validation.cpp) */
extern HybridPowPobConsensus g_pow_pob_consensus;

} // namespace OConsensus

#endif // BITCOIN_CONSENSUS_O_POW_POB_H

