// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/o_pow_pob.h>
#include <arith_uint256.h>
#include <chain.h>
#include <chainparams.h>
#include <consensus/params.h>
#include <logging.h>
#include <pow.h>
#include <util/time.h>
#include <hash.h>
#include <cmath>

namespace OConsensus {

HybridPowPobConsensus g_pow_pob_consensus;

HybridPowPobConsensus::HybridPowPobConsensus() 
{
    LogPrintf("O Blockchain: Initializing Hybrid PoW/PoB Consensus\n");
}

bool HybridPowPobConsensus::IsBusinessMiner(const uint256& pubkey_hash, int height) const 
{
    auto it = m_business_miners.find(pubkey_hash);
    if (it == m_business_miners.end()) {
        return false;
    }
    
    const auto& stats = it->second;
    
    // Check if data is stale
    if (height - stats.last_qualification_height > BUSINESS_QUALIFICATION_PERIOD) {
        return false;
    }
    
    // Check qualification criteria
    if (stats.total_transactions < MIN_BUSINESS_TRANSACTIONS) {
        return false;
    }
    
    if (stats.distinct_recipients < MIN_BUSINESS_DISTINCT_KEYS) {
        return false;
    }
    
    if (stats.transaction_volume < MIN_BUSINESS_VOLUME) {
        return false;
    }
    
    return stats.is_qualified;
}

double HybridPowPobConsensus::GetBusinessRatio(int height) const 
{
    // Check cache first
    auto cached = m_cached_business_ratios.find(height);
    if (cached != m_cached_business_ratios.end()) {
        return cached->second;
    }
    
    int total_active_miners = 0;
    int qualified_business_miners = 0;
    
    // Count active miners in the qualification period
    for (const auto& [pubkey, stats] : m_business_miners) {
        if (height - stats.last_qualification_height <= BUSINESS_QUALIFICATION_PERIOD) {
            total_active_miners++;
            if (IsBusinessMiner(pubkey, height)) {
                qualified_business_miners++;
            }
        }
    }
    
    if (total_active_miners == 0) {
        m_cached_business_ratios[height] = 0.0;
        return 0.0;
    }
    
    double ratio = static_cast<double>(qualified_business_miners) / total_active_miners;
    ratio = std::min(ratio, MAX_BUSINESS_RATIO);
    
    m_cached_business_ratios[height] = ratio;
    return ratio;
}

double HybridPowPobConsensus::CalculateDifficultyAdjustment(double business_ratio) const 
{
    // As business participation increases, difficulty decreases
    // Formula: adjustment = 1.0 - (business_ratio * reduction_factor)
    // This means at 100% business participation, difficulty is reduced by 50%
    double adjustment = 1.0 - (business_ratio * DIFFICULTY_REDUCTION_FACTOR);
    
    // Ensure adjustment stays within reasonable bounds
    adjustment = std::max(0.5, std::min(1.0, adjustment));
    
    return adjustment;
}

unsigned int HybridPowPobConsensus::GetNextWorkRequired(const CBlockIndex* pindexLast, 
                                                         const CBlockHeader* pblock, 
                                                         const CChainParams& params) 
{
    // Get base difficulty using Bitcoin's algorithm
    unsigned int base_bits = GetNextWorkRequired_Bitcoin(pindexLast, pblock, params);
    
    // If we don't have enough history, use base difficulty
    if (pindexLast == nullptr) {
        return base_bits;
    }
    
    int next_height = pindexLast->nHeight + 1;
    
    // Get business miner ratio
    double business_ratio = GetBusinessRatio(next_height);
    
    // If no business miners, use standard difficulty
    if (business_ratio < 0.01) {  // Less than 1% business participation
        return base_bits;
    }
    
    // Calculate difficulty adjustment
    double adjustment = CalculateDifficultyAdjustment(business_ratio);
    
    // Convert bits to target
    arith_uint256 base_target;
    base_target.SetCompact(base_bits);
    
    // Apply adjustment (higher target = easier difficulty)
    // adjustment < 1.0 means we make it easier by increasing the target
    arith_uint256 adjusted_target = base_target;
    adjusted_target *= static_cast<int>(adjustment * 1000);
    adjusted_target /= 1000;
    
    // Ensure we don't exceed the proof of work limit
    const arith_uint256 pow_limit = UintToArith256(params.GetConsensus().powLimit);
    if (adjusted_target > pow_limit) {
        adjusted_target = pow_limit;
    }
    
    unsigned int adjusted_bits = adjusted_target.GetCompact();
    
    LogDebug(BCLog::NET, "O PoW/PoB: height=%d, business_ratio=%.2f, adjustment=%.3f, base_bits=%08x, adjusted_bits=%08x\n",
             next_height, business_ratio, adjustment, base_bits, adjusted_bits);
    
    return adjusted_bits;
}

unsigned int HybridPowPobConsensus::GetNextWorkRequired_Bitcoin(const CBlockIndex* pindexLast, 
                                                                 const CBlockHeader* pblock, 
                                                                 const CChainParams& params) 
{
    // Use Bitcoin Core's standard difficulty adjustment algorithm
    const Consensus::Params& consensus = params.GetConsensus();
    
    if (pindexLast == nullptr) {
        return UintToArith256(consensus.powLimit).GetCompact();
    }
    
    // Only change once per difficulty adjustment interval
    if ((pindexLast->nHeight + 1) % consensus.DifficultyAdjustmentInterval() != 0) {
        return pindexLast->nBits;
    }
    
    // Go back by what we want to be 14 days worth of blocks
    int nHeightFirst = pindexLast->nHeight - (consensus.DifficultyAdjustmentInterval() - 1);
    assert(nHeightFirst >= 0);
    const CBlockIndex* pindexFirst = pindexLast->GetAncestor(nHeightFirst);
    assert(pindexFirst);
    
    return CalculateNextWorkRequired(pindexLast, pindexFirst->GetBlockTime(), consensus);
}

bool HybridPowPobConsensus::CheckProofOfWork(uint256 hash, unsigned int nBits, 
                                              bool is_business_miner, 
                                              const CChainParams& params) const 
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;
    
    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);
    
    const arith_uint256 pow_limit = UintToArith256(params.GetConsensus().powLimit);
    
    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > pow_limit) {
        return false;
    }
    
    // Business miners get a small bonus (10% easier threshold)
    if (is_business_miner) {
        bnTarget *= 110;
        bnTarget /= 100;
        
        // Ensure still within limits
        if (bnTarget > pow_limit) {
            bnTarget = pow_limit;
        }
    }
    
    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget) {
        return false;
    }
    
    return true;
}

bool HybridPowPobConsensus::ValidateBusinessMinerBlock(const CBlock& block, 
                                                        const uint256& miner_pubkey) const 
{
    if (miner_pubkey.IsNull()) {
        return true;  // Not a business miner block
    }
    
    // Business miners cannot mine blocks containing their own transactions
    for (const auto& tx : block.vtx) {
        // Skip coinbase transaction
        if (tx->IsCoinBase()) {
            continue;
        }
        
        // Check each input to see if it belongs to the miner
        for ([[maybe_unused]] const auto& input : tx->vin) {
            // Extract pubkey hash from input script
            // This is simplified; actual implementation would need proper script parsing
            uint256 input_pubkey_hash;
            // TODO: Implement proper pubkey extraction from scriptSig
            
            if (!input_pubkey_hash.IsNull() && input_pubkey_hash == miner_pubkey) {
                LogPrintf("O PoB: Business miner %s attempted to mine own transaction\n", 
                         miner_pubkey.GetHex());
                return false;
            }
        }
    }
    
    return true;
}

void HybridPowPobConsensus::UpdateBusinessStats(const uint256& pubkey_hash, 
                                                 const CTransaction& tx, 
                                                 int height) 
{
    if (pubkey_hash.IsNull()) {
        return;
    }
    
    auto& stats = m_business_miners[pubkey_hash];
    
    // Initialize if first time seeing this miner
    if (stats.miner_pubkey_hash.IsNull()) {
        stats.miner_pubkey_hash = pubkey_hash;
        stats.first_seen_height = height;
    }
    
    // Update statistics
    stats.total_transactions++;
    stats.last_qualification_height = height;
    
    // Track unique recipients
    for (const auto& output : tx.vout) {
        // Extract recipient pubkey hash from output script
        uint256 recipient_hash;
        // TODO: Implement proper pubkey extraction from scriptPubKey
        // For now, use a hash of the script
        HashWriter ss{};
        ss << output.scriptPubKey;
        recipient_hash = ss.GetHash();
        
        stats.recipient_set.insert(recipient_hash);
    }
    stats.distinct_recipients = stats.recipient_set.size();
    
    // Update transaction volume
    CAmount tx_value = 0;
    for (const auto& output : tx.vout) {
        tx_value += output.nValue;
    }
    stats.transaction_volume += tx_value;
    
    // Update qualification status
    stats.is_qualified = (stats.total_transactions >= MIN_BUSINESS_TRANSACTIONS) &&
                        (stats.distinct_recipients >= MIN_BUSINESS_DISTINCT_KEYS) &&
                        (stats.transaction_volume >= MIN_BUSINESS_VOLUME);
    
    if (stats.is_qualified) {
        LogDebug(BCLog::NET, "O PoB: Miner %s qualified as business miner (tx=%d, recipients=%d, volume=%d)\n",
                 pubkey_hash.GetHex().substr(0, 16), stats.total_transactions, 
                 stats.distinct_recipients, stats.transaction_volume);
    }
}

int64_t HybridPowPobConsensus::GetTargetBlockTime(int height) const 
{
    // Fixed 12-second block time for transactional currency
    // Business participation affects difficulty, not block time
    return TARGET_BLOCK_TIME;
}

const BusinessMinerStats* HybridPowPobConsensus::GetBusinessStats(const uint256& pubkey_hash) const 
{
    auto it = m_business_miners.find(pubkey_hash);
    if (it == m_business_miners.end()) {
        return nullptr;
    }
    return &it->second;
}

std::vector<uint256> HybridPowPobConsensus::GetQualifiedBusinessMiners(int height) const 
{
    std::vector<uint256> qualified;
    
    for (const auto& [pubkey, stats] : m_business_miners) {
        if (IsBusinessMiner(pubkey, height)) {
            qualified.push_back(pubkey);
        }
    }
    
    return qualified;
}

void HybridPowPobConsensus::ReEvaluateQualifications(int current_height) 
{
    int requalified = 0;
    int disqualified = 0;
    
    for (auto& [pubkey, stats] : m_business_miners) {
        bool was_qualified = stats.is_qualified;
        
        // Check if still within qualification period
        if (current_height - stats.last_qualification_height > BUSINESS_QUALIFICATION_PERIOD) {
            if (was_qualified) {
                stats.is_qualified = false;
                disqualified++;
            }
            continue;
        }
        
        // Re-evaluate qualification
        bool should_be_qualified = (stats.total_transactions >= MIN_BUSINESS_TRANSACTIONS) &&
                                   (stats.distinct_recipients >= MIN_BUSINESS_DISTINCT_KEYS) &&
                                   (stats.transaction_volume >= MIN_BUSINESS_VOLUME);
        
        if (should_be_qualified && !was_qualified) {
            stats.is_qualified = true;
            requalified++;
        } else if (!should_be_qualified && was_qualified) {
            stats.is_qualified = false;
            disqualified++;
        }
    }
    
    if (requalified > 0 || disqualified > 0) {
        LogPrintf("O PoB: Re-evaluated qualifications at height %d: +%d qualified, -%d disqualified\n",
                 current_height, requalified, disqualified);
    }
    
    // Clear cached ratios
    m_cached_business_ratios.clear();
}

void HybridPowPobConsensus::PruneOldData(int current_height) 
{
    auto it = m_business_miners.begin();
    int pruned = 0;
    
    while (it != m_business_miners.end()) {
        // Remove miners that haven't been active for 2x the qualification period
        if (current_height - it->second.last_qualification_height > (BUSINESS_QUALIFICATION_PERIOD * 2)) {
            it = m_business_miners.erase(it);
            pruned++;
        } else {
            ++it;
        }
    }
    
    if (pruned > 0) {
        LogPrintf("O PoB: Pruned %d inactive business miners at height %d\n", pruned, current_height);
    }
}

size_t HybridPowPobConsensus::GetQualifiedBusinessCount() const 
{
    size_t count = 0;
    for (const auto& [pubkey, stats] : m_business_miners) {
        if (stats.is_qualified) {
            count++;
        }
    }
    return count;
}

uint256 HybridPowPobConsensus::ExtractMinerPubKey(const CBlock& block) 
{
    if (block.vtx.empty() || !block.vtx[0]->IsCoinBase()) {
        return uint256();
    }
    
    const CTransaction& coinbase = *block.vtx[0];
    
    // Try to extract pubkey from coinbase output script
    if (coinbase.vout.empty()) {
        return uint256();
    }
    
    // Hash the scriptPubKey as a simple identifier
    HashWriter ss{};
    ss << coinbase.vout[0].scriptPubKey;
    return ss.GetHash();
}

} // namespace OConsensus

