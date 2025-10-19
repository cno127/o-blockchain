// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_O_BUSINESS_DB_H
#define BITCOIN_CONSENSUS_O_BUSINESS_DB_H

#include <dbwrapper.h>
#include <consensus/o_pow_pob.h>
#include <sync.h>
#include <uint256.h>

#include <memory>
#include <optional>
#include <vector>

namespace OConsensus {

/** Database key prefixes for business miner data */
static constexpr uint8_t DB_BUSINESS_STATS = 'b';      // Business miner statistics
static constexpr uint8_t DB_BUSINESS_RATIO = 'r';      // Cached business ratios by height
static constexpr uint8_t DB_BUSINESS_QUALIFIED = 'q';  // Qualified business miners index
static constexpr uint8_t DB_BUSINESS_VERSION = 'v';    // Database version

/** Business Miner Database - Persistent storage for PoB consensus data */
class CBusinessMinerDB {
private:
    std::unique_ptr<CDBWrapper> m_db;
    mutable RecursiveMutex m_db_mutex;
    
public:
    explicit CBusinessMinerDB(size_t cache_size, bool memory_only = false, bool wipe_data = false);
    ~CBusinessMinerDB();
    
    // ===== Business Stats Operations =====
    
    /** Write business miner stats to database */
    bool WriteBusinessStats(const uint256& pubkey_hash, const BusinessMinerStats& stats);
    
    /** Read business miner stats from database */
    std::optional<BusinessMinerStats> ReadBusinessStats(const uint256& pubkey_hash) const;
    
    /** Check if business miner exists in database */
    bool HasBusinessMiner(const uint256& pubkey_hash) const;
    
    /** Erase business miner stats from database */
    bool EraseBusinessStats(const uint256& pubkey_hash);
    
    // ===== Ratio Cache Operations =====
    
    /** Write cached business ratio for height */
    bool WriteBusinessRatio(int height, double ratio);
    
    /** Read cached business ratio for height */
    std::optional<double> ReadBusinessRatio(int height) const;
    
    /** Erase cached ratio */
    bool EraseBusinessRatio(int height);
    
    // ===== Batch Operations =====
    
    /** Get all qualified business miners at a height */
    std::vector<uint256> GetQualifiedBusinessMiners(int height) const;
    
    /** Get all business miners (for iteration) */
    std::vector<std::pair<uint256, BusinessMinerStats>> GetAllBusinessMiners() const;
    
    /** Batch write multiple stats */
    bool BatchWriteStats(const std::vector<std::pair<uint256, BusinessMinerStats>>& batch);
    
    /** Prune old business miner data before cutoff height */
    bool PruneOldData(int cutoff_height);
    
    // ===== Statistics =====
    
    /** Get total number of business miners in database */
    size_t GetBusinessMinerCount() const;
    
    /** Get qualified business miner count */
    size_t GetQualifiedBusinessCount(int current_height) const;
    
    // ===== Maintenance =====
    
    /** Compact the database */
    void Compact();
    
    /** Get database size estimate */
    size_t EstimateSize() const;
    
    /** Get database path */
    std::optional<fs::path> StoragePath() const;
};

/** Global business miner database instance */
extern std::unique_ptr<CBusinessMinerDB> g_business_db;

} // namespace OConsensus

#endif // BITCOIN_CONSENSUS_O_BUSINESS_DB_H

