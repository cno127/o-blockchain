// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/o_business_db.h>
#include <common/args.h>
#include <logging.h>
#include <util/fs.h>
#include <streams.h>

namespace OConsensus {

// Global instance (initialized in init.cpp)
std::unique_ptr<CBusinessMinerDB> g_business_db;

CBusinessMinerDB::CBusinessMinerDB(size_t cache_size, bool memory_only, bool wipe_data)
{
    DBParams db_params;
    db_params.path = gArgs.GetDataDirNet() / "business_miners";
    db_params.cache_bytes = cache_size;
    db_params.memory_only = memory_only;
    db_params.wipe_data = wipe_data;
    db_params.obfuscate = true;  // Obfuscate data for security
    
    try {
        m_db = std::make_unique<CDBWrapper>(db_params);
        LogPrintf("O Business DB: Opened database at %s (cache: %d MB, memory_only: %d)\n",
                  fs::PathToString(db_params.path), cache_size / (1024 * 1024), memory_only);
    } catch (const std::exception& e) {
        LogPrintf("O Business DB: Error opening database: %s\n", e.what());
        throw;
    }
}

CBusinessMinerDB::~CBusinessMinerDB() = default;

// ===== Business Stats Operations =====

bool CBusinessMinerDB::WriteBusinessStats(const uint256& pubkey_hash, const BusinessMinerStats& stats)
{
    LOCK(m_db_mutex);
    
    CDBBatch batch(*m_db);
    batch.Write(std::make_pair(DB_BUSINESS_STATS, pubkey_hash), stats);
    
    bool success = m_db->WriteBatch(batch, true);
    
    if (success) {
        LogDebug(BCLog::NET, "O Business DB: Wrote stats for miner %s (tx=%d, recipients=%d)\n",
                 pubkey_hash.GetHex().substr(0, 16), stats.total_transactions, stats.distinct_recipients);
    } else {
        LogPrintf("O Business DB: Failed to write stats for miner %s\n", 
                  pubkey_hash.GetHex().substr(0, 16));
    }
    
    return success;
}

std::optional<BusinessMinerStats> CBusinessMinerDB::ReadBusinessStats(const uint256& pubkey_hash) const
{
    LOCK(m_db_mutex);
    
    BusinessMinerStats stats;
    if (m_db->Read(std::make_pair(DB_BUSINESS_STATS, pubkey_hash), stats)) {
        return stats;
    }
    
    return std::nullopt;
}

bool CBusinessMinerDB::HasBusinessMiner(const uint256& pubkey_hash) const
{
    LOCK(m_db_mutex);
    return m_db->Exists(std::make_pair(DB_BUSINESS_STATS, pubkey_hash));
}

bool CBusinessMinerDB::EraseBusinessStats(const uint256& pubkey_hash)
{
    LOCK(m_db_mutex);
    
    CDBBatch batch(*m_db);
    batch.Erase(std::make_pair(DB_BUSINESS_STATS, pubkey_hash));
    
    bool success = m_db->WriteBatch(batch, true);
    
    if (success) {
        LogDebug(BCLog::NET, "O Business DB: Erased stats for miner %s\n",
                 pubkey_hash.GetHex().substr(0, 16));
    }
    
    return success;
}

// ===== Ratio Cache Operations =====

bool CBusinessMinerDB::WriteBusinessRatio(int height, double ratio)
{
    LOCK(m_db_mutex);
    
    // Convert double to int64_t for serialization (multiply by 1000000 for 6 decimal precision)
    int64_t ratio_int = static_cast<int64_t>(ratio * 1000000);
    
    CDBBatch batch(*m_db);
    batch.Write(std::make_pair(DB_BUSINESS_RATIO, height), ratio_int);
    
    return m_db->WriteBatch(batch, true);
}

std::optional<double> CBusinessMinerDB::ReadBusinessRatio(int height) const
{
    LOCK(m_db_mutex);
    
    int64_t ratio_int;
    if (m_db->Read(std::make_pair(DB_BUSINESS_RATIO, height), ratio_int)) {
        // Convert back to double
        return static_cast<double>(ratio_int) / 1000000.0;
    }
    
    return std::nullopt;
}

bool CBusinessMinerDB::EraseBusinessRatio(int height)
{
    LOCK(m_db_mutex);
    
    CDBBatch batch(*m_db);
    batch.Erase(std::make_pair(DB_BUSINESS_RATIO, height));
    
    return m_db->WriteBatch(batch, true);
}

// ===== Batch Operations =====

std::vector<uint256> CBusinessMinerDB::GetQualifiedBusinessMiners(int height) const
{
    LOCK(m_db_mutex);
    
    std::vector<uint256> qualified;
    
    // Iterate through all business miners
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    for (iterator->Seek(DB_BUSINESS_STATS); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, uint256> key;
        if (!iterator->GetKey(key) || key.first != DB_BUSINESS_STATS) {
            break;
        }
        
        BusinessMinerStats stats;
        if (!iterator->GetValue(stats)) {
            continue;
        }
        
        // Check if qualified and recent
        if (stats.is_qualified && 
            (height - stats.last_qualification_height <= BUSINESS_QUALIFICATION_PERIOD)) {
            qualified.push_back(key.second);
        }
    }
    
    return qualified;
}

std::vector<std::pair<uint256, BusinessMinerStats>> CBusinessMinerDB::GetAllBusinessMiners() const
{
    LOCK(m_db_mutex);
    
    std::vector<std::pair<uint256, BusinessMinerStats>> all_miners;
    
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    for (iterator->Seek(DB_BUSINESS_STATS); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, uint256> key;
        if (!iterator->GetKey(key) || key.first != DB_BUSINESS_STATS) {
            break;
        }
        
        BusinessMinerStats stats;
        if (iterator->GetValue(stats)) {
            all_miners.emplace_back(key.second, stats);
        }
    }
    
    LogPrintf("O Business DB: Retrieved %d business miners from database\n", all_miners.size());
    
    return all_miners;
}

bool CBusinessMinerDB::BatchWriteStats(const std::vector<std::pair<uint256, BusinessMinerStats>>& batch)
{
    LOCK(m_db_mutex);
    
    CDBBatch db_batch(*m_db);
    
    for (const auto& [pubkey_hash, stats] : batch) {
        db_batch.Write(std::make_pair(DB_BUSINESS_STATS, pubkey_hash), stats);
    }
    
    bool success = m_db->WriteBatch(db_batch, true);
    
    if (success) {
        LogPrintf("O Business DB: Batch wrote %d business miner stats\n", batch.size());
    }
    
    return success;
}

bool CBusinessMinerDB::PruneOldData(int cutoff_height)
{
    LOCK(m_db_mutex);
    
    CDBBatch batch(*m_db);
    int pruned_stats = 0;
    int pruned_ratios = 0;
    
    // Prune old business miner stats
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    for (iterator->Seek(DB_BUSINESS_STATS); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, uint256> key;
        if (!iterator->GetKey(key) || key.first != DB_BUSINESS_STATS) {
            break;
        }
        
        BusinessMinerStats stats;
        if (iterator->GetValue(stats)) {
            // Remove if inactive for 2x qualification period
            if (cutoff_height - stats.last_qualification_height > (BUSINESS_QUALIFICATION_PERIOD * 2)) {
                batch.Erase(std::make_pair(DB_BUSINESS_STATS, key.second));
                pruned_stats++;
            }
        }
    }
    
    // Prune old cached ratios (keep only recent month)
    int ratio_cutoff = cutoff_height - BLOCKS_PER_DAY_MAX * 30;  // ~30 days
    
    for (iterator->Seek(DB_BUSINESS_RATIO); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, int> key;
        if (!iterator->GetKey(key) || key.first != DB_BUSINESS_RATIO) {
            break;
        }
        
        if (key.second < ratio_cutoff) {
            batch.Erase(std::make_pair(DB_BUSINESS_RATIO, key.second));
            pruned_ratios++;
        }
    }
    
    bool success = m_db->WriteBatch(batch, true);
    
    if (success && (pruned_stats > 0 || pruned_ratios > 0)) {
        LogPrintf("O Business DB: Pruned %d inactive miners and %d old ratios at height %d\n",
                  pruned_stats, pruned_ratios, cutoff_height);
    }
    
    return success;
}

// ===== Statistics =====

size_t CBusinessMinerDB::GetBusinessMinerCount() const
{
    LOCK(m_db_mutex);
    
    size_t count = 0;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    for (iterator->Seek(DB_BUSINESS_STATS); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, uint256> key;
        if (!iterator->GetKey(key) || key.first != DB_BUSINESS_STATS) {
            break;
        }
        count++;
    }
    
    return count;
}

size_t CBusinessMinerDB::GetQualifiedBusinessCount(int current_height) const
{
    LOCK(m_db_mutex);
    
    size_t count = 0;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    for (iterator->Seek(DB_BUSINESS_STATS); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, uint256> key;
        if (!iterator->GetKey(key) || key.first != DB_BUSINESS_STATS) {
            break;
        }
        
        BusinessMinerStats stats;
        if (iterator->GetValue(stats)) {
            // Check if qualified and recent
            if (stats.is_qualified && 
                (current_height - stats.last_qualification_height <= BUSINESS_QUALIFICATION_PERIOD)) {
                count++;
            }
        }
    }
    
    return count;
}

// ===== Maintenance =====

void CBusinessMinerDB::Compact()
{
    LOCK(m_db_mutex);
    
    LogPrintf("O Business DB: Database compaction requested\n");
    LogPrintf("O Business DB: Note: Compaction happens automatically via LevelDB\n");
    LogPrintf("O Business DB: To force compaction, restart node with -reindex\n");
    
    // Note: CDBWrapper doesn't expose CompactRange publicly
    // Compaction happens automatically in LevelDB
    // Manual compaction can be triggered via -reindex flag
}

size_t CBusinessMinerDB::EstimateSize() const
{
    LOCK(m_db_mutex);
    return m_db->DynamicMemoryUsage();
}

std::optional<fs::path> CBusinessMinerDB::StoragePath() const
{
    return m_db->StoragePath();
}

} // namespace OConsensus

