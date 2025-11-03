// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/o_brightid_db.h>
#include <common/args.h>
#include <logging.h>
#include <util/fs.h>
#include <util/time.h>
#include <streams.h>

namespace OConsensus {

// Global instance (initialized in init.cpp)
std::unique_ptr<CBrightIDUserDB> g_brightid_db;

CBrightIDUserDB::CBrightIDUserDB(size_t cache_size, bool memory_only, bool wipe_data)
{
    DBParams db_params;
    db_params.path = gArgs.GetDataDirNet() / "brightid_users";
    db_params.cache_bytes = cache_size;
    db_params.memory_only = memory_only;
    db_params.wipe_data = wipe_data;
    db_params.obfuscate = true;  // Obfuscate for privacy
    
    try {
        m_db = std::make_unique<CDBWrapper>(db_params);
        LogPrintf("O BrightID DB: Opened database at %s (cache: %d MB, memory_only: %d)\n",
                  fs::PathToString(db_params.path), cache_size / (1024 * 1024), memory_only);
    } catch (const std::exception& e) {
        LogPrintf("O BrightID DB: Error opening database: %s\n", e.what());
        throw;
    }
}

CBrightIDUserDB::~CBrightIDUserDB() = default;

// ===== User Operations =====

bool CBrightIDUserDB::WriteUser(const std::string& brightid_address, const BrightIDUser& user)
{
    LOCK(m_db_mutex);
    
    CDBBatch batch(*m_db);
    batch.Write(std::make_pair(DB_BRIGHTID_USER, brightid_address), user);
    
    bool success = m_db->WriteBatch(batch, true);
    
    if (success) {
        LogDebug(BCLog::NET, "O BrightID DB: Wrote user %s (status=%d, trust=%.2f)\n",
                 brightid_address.substr(0, 16), static_cast<int>(user.status), user.trust_score);
    } else {
        LogPrintf("O BrightID DB: Failed to write user %s\n", brightid_address.substr(0, 16));
    }
    
    return success;
}

std::optional<BrightIDUser> CBrightIDUserDB::ReadUser(const std::string& brightid_address) const
{
    LOCK(m_db_mutex);
    
    BrightIDUser user;
    if (m_db->Read(std::make_pair(DB_BRIGHTID_USER, brightid_address), user)) {
        return user;
    }
    
    return std::nullopt;
}

bool CBrightIDUserDB::HasUser(const std::string& brightid_address) const
{
    LOCK(m_db_mutex);
    return m_db->Exists(std::make_pair(DB_BRIGHTID_USER, brightid_address));
}

bool CBrightIDUserDB::EraseUser(const std::string& brightid_address)
{
    LOCK(m_db_mutex);
    
    CDBBatch batch(*m_db);
    
    // Erase user data
    batch.Erase(std::make_pair(DB_BRIGHTID_USER, brightid_address));
    
    // Also erase address mappings
    auto o_addr = GetOAddress(brightid_address);
    if (o_addr.has_value()) {
        batch.Erase(std::make_pair(DB_BRIGHTID_TO_O, brightid_address));
        batch.Erase(std::make_pair(DB_O_TO_BRIGHTID, o_addr.value()));
    }
    
    // Erase anonymous data
    auto anon_id = GetAnonymousID(brightid_address);
    if (anon_id.has_value()) {
        batch.Erase(std::make_pair(DB_ANONYMOUS_ID, brightid_address));
        batch.Erase(std::make_pair(DB_ANONYMOUS_REP, anon_id.value()));
    }
    
    bool success = m_db->WriteBatch(batch, true);
    
    if (success) {
        LogDebug(BCLog::NET, "O BrightID DB: Erased user %s\n", brightid_address.substr(0, 16));
    }
    
    return success;
}

bool CBrightIDUserDB::UpdateUserStatus(const std::string& brightid_address, BrightIDStatus status)
{
    LOCK(m_db_mutex);
    
    auto user_opt = ReadUser(brightid_address);
    if (!user_opt.has_value()) {
        return false;
    }
    
    BrightIDUser user = user_opt.value();
    user.status = status;
    
    return WriteUser(brightid_address, user);
}

bool CBrightIDUserDB::UpdateTrustScore(const std::string& brightid_address, double trust_score)
{
    LOCK(m_db_mutex);
    
    auto user_opt = ReadUser(brightid_address);
    if (!user_opt.has_value()) {
        return false;
    }
    
    BrightIDUser user = user_opt.value();
    user.trust_score = trust_score;
    
    return WriteUser(brightid_address, user);
}

// ===== Address Mapping Operations =====

bool CBrightIDUserDB::LinkAddresses(const std::string& brightid_address, const std::string& o_address)
{
    LOCK(m_db_mutex);
    
    CDBBatch batch(*m_db);
    
    // Store both directions for fast lookup
    batch.Write(std::make_pair(DB_BRIGHTID_TO_O, brightid_address), o_address);
    batch.Write(std::make_pair(DB_O_TO_BRIGHTID, o_address), brightid_address);
    
    bool success = m_db->WriteBatch(batch, true);
    
    if (success) {
        LogDebug(BCLog::NET, "O BrightID DB: Linked %s <-> %s\n",
                 brightid_address.substr(0, 16), o_address.substr(0, 16));
    }
    
    return success;
}

bool CBrightIDUserDB::UnlinkAddresses(const std::string& brightid_address)
{
    LOCK(m_db_mutex);
    
    auto o_addr = GetOAddress(brightid_address);
    if (!o_addr.has_value()) {
        return false;
    }
    
    CDBBatch batch(*m_db);
    batch.Erase(std::make_pair(DB_BRIGHTID_TO_O, brightid_address));
    batch.Erase(std::make_pair(DB_O_TO_BRIGHTID, o_addr.value()));
    
    bool success = m_db->WriteBatch(batch, true);
    
    if (success) {
        LogDebug(BCLog::NET, "O BrightID DB: Unlinked %s <-> %s\n",
                 brightid_address.substr(0, 16), o_addr.value().substr(0, 16));
    }
    
    return success;
}

std::optional<std::string> CBrightIDUserDB::GetOAddress(const std::string& brightid_address) const
{
    LOCK(m_db_mutex);
    
    std::string o_address;
    if (m_db->Read(std::make_pair(DB_BRIGHTID_TO_O, brightid_address), o_address)) {
        return o_address;
    }
    
    return std::nullopt;
}

std::optional<std::string> CBrightIDUserDB::GetBrightIDAddress(const std::string& o_address) const
{
    LOCK(m_db_mutex);
    
    std::string brightid_address;
    if (m_db->Read(std::make_pair(DB_O_TO_BRIGHTID, o_address), brightid_address)) {
        return brightid_address;
    }
    
    return std::nullopt;
}

// ===== Anonymous ID Operations =====

bool CBrightIDUserDB::WriteAnonymousID(const std::string& brightid_address, const std::string& anonymous_id)
{
    LOCK(m_db_mutex);
    
    CDBBatch batch(*m_db);
    batch.Write(std::make_pair(DB_ANONYMOUS_ID, brightid_address), anonymous_id);
    
    return m_db->WriteBatch(batch, true);
}

std::optional<std::string> CBrightIDUserDB::GetAnonymousID(const std::string& brightid_address) const
{
    LOCK(m_db_mutex);
    
    std::string anonymous_id;
    if (m_db->Read(std::make_pair(DB_ANONYMOUS_ID, brightid_address), anonymous_id)) {
        return anonymous_id;
    }
    
    return std::nullopt;
}

bool CBrightIDUserDB::WriteAnonymousReputation(const std::string& anonymous_id, double reputation)
{
    LOCK(m_db_mutex);
    
    // Convert double to int64_t for serialization (multiply by 1000000 for 6 decimal precision)
    int64_t reputation_int = static_cast<int64_t>(reputation * 1000000);
    
    CDBBatch batch(*m_db);
    batch.Write(std::make_pair(DB_ANONYMOUS_REP, anonymous_id), reputation_int);
    
    return m_db->WriteBatch(batch, true);
}

std::optional<double> CBrightIDUserDB::GetAnonymousReputation(const std::string& anonymous_id) const
{
    LOCK(m_db_mutex);
    
    int64_t reputation_int;
    if (m_db->Read(std::make_pair(DB_ANONYMOUS_REP, anonymous_id), reputation_int)) {
        // Convert back to double
        return static_cast<double>(reputation_int) / 1000000.0;
    }
    
    return std::nullopt;
}

bool CBrightIDUserDB::EraseAnonymousData(const std::string& brightid_address)
{
    LOCK(m_db_mutex);
    
    auto anon_id = GetAnonymousID(brightid_address);
    if (!anon_id.has_value()) {
        return false;
    }
    
    CDBBatch batch(*m_db);
    batch.Erase(std::make_pair(DB_ANONYMOUS_ID, brightid_address));
    batch.Erase(std::make_pair(DB_ANONYMOUS_REP, anon_id.value()));
    
    return m_db->WriteBatch(batch, true);
}

// ===== Batch Operations =====

std::vector<BrightIDUser> CBrightIDUserDB::GetVerifiedUsers() const
{
    LOCK(m_db_mutex);
    
    std::vector<BrightIDUser> verified_users;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    for (iterator->Seek(DB_BRIGHTID_USER); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, std::string> key;
        if (!iterator->GetKey(key) || key.first != DB_BRIGHTID_USER) {
            break;
        }
        
        BrightIDUser user;
        if (iterator->GetValue(user) && user.IsVerified()) {
            verified_users.push_back(user);
        }
    }
    
    LogDebug(BCLog::NET, "O BrightID DB: Retrieved %d verified users\n", verified_users.size());
    return verified_users;
}

std::vector<BrightIDUser> CBrightIDUserDB::GetActiveUsers() const
{
    LOCK(m_db_mutex);
    
    std::vector<BrightIDUser> active_users;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    for (iterator->Seek(DB_BRIGHTID_USER); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, std::string> key;
        if (!iterator->GetKey(key) || key.first != DB_BRIGHTID_USER) {
            break;
        }
        
        BrightIDUser user;
        if (iterator->GetValue(user) && user.IsActive()) {
            active_users.push_back(user);
        }
    }
    
    LogDebug(BCLog::NET, "O BrightID DB: Retrieved %d active users\n", active_users.size());
    return active_users;
}

std::vector<BrightIDUser> CBrightIDUserDB::GetUsersByStatus(BrightIDStatus status) const
{
    LOCK(m_db_mutex);
    
    std::vector<BrightIDUser> users_by_status;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    for (iterator->Seek(DB_BRIGHTID_USER); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, std::string> key;
        if (!iterator->GetKey(key) || key.first != DB_BRIGHTID_USER) {
            break;
        }
        
        BrightIDUser user;
        if (iterator->GetValue(user) && user.status == status) {
            users_by_status.push_back(user);
        }
    }
    
    return users_by_status;
}

std::vector<std::pair<std::string, BrightIDUser>> CBrightIDUserDB::GetAllUsers() const
{
    LOCK(m_db_mutex);
    
    std::vector<std::pair<std::string, BrightIDUser>> all_users;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    for (iterator->Seek(DB_BRIGHTID_USER); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, std::string> key;
        if (!iterator->GetKey(key) || key.first != DB_BRIGHTID_USER) {
            break;
        }
        
        BrightIDUser user;
        if (iterator->GetValue(user)) {
            all_users.emplace_back(key.second, user);
        }
    }
    
    LogPrintf("O BrightID DB: Retrieved %d total users from database\n", all_users.size());
    return all_users;
}

bool CBrightIDUserDB::BatchWriteUsers(const std::vector<std::pair<std::string, BrightIDUser>>& batch)
{
    LOCK(m_db_mutex);
    
    CDBBatch db_batch(*m_db);
    
    for (const auto& [brightid_addr, user] : batch) {
        db_batch.Write(std::make_pair(DB_BRIGHTID_USER, brightid_addr), user);
    }
    
    bool success = m_db->WriteBatch(db_batch, true);
    
    if (success) {
        LogPrintf("O BrightID DB: Batch wrote %d users\n", batch.size());
    }
    
    return success;
}

bool CBrightIDUserDB::BatchEraseUsers(const std::vector<std::string>& brightid_addresses)
{
    LOCK(m_db_mutex);
    
    CDBBatch batch(*m_db);
    
    for (const auto& addr : brightid_addresses) {
        batch.Erase(std::make_pair(DB_BRIGHTID_USER, addr));
        
        // Also erase related data
        auto o_addr = GetOAddress(addr);
        if (o_addr.has_value()) {
            batch.Erase(std::make_pair(DB_BRIGHTID_TO_O, addr));
            batch.Erase(std::make_pair(DB_O_TO_BRIGHTID, o_addr.value()));
        }
        
        auto anon_id = GetAnonymousID(addr);
        if (anon_id.has_value()) {
            batch.Erase(std::make_pair(DB_ANONYMOUS_ID, addr));
            batch.Erase(std::make_pair(DB_ANONYMOUS_REP, anon_id.value()));
        }
    }
    
    bool success = m_db->WriteBatch(batch, true);
    
    if (success) {
        LogPrintf("O BrightID DB: Batch erased %d users\n", brightid_addresses.size());
    }
    
    return success;
}

// ===== Query Operations =====

std::vector<std::string> CBrightIDUserDB::FindUsersByMethod(BrightIDVerificationMethod method) const
{
    LOCK(m_db_mutex);
    
    std::vector<std::string> matching_users;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    for (iterator->Seek(DB_BRIGHTID_USER); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, std::string> key;
        if (!iterator->GetKey(key) || key.first != DB_BRIGHTID_USER) {
            break;
        }
        
        BrightIDUser user;
        if (iterator->GetValue(user) && user.method == method) {
            matching_users.push_back(key.second);
        }
    }
    
    return matching_users;
}

std::vector<std::string> CBrightIDUserDB::FindUsersByTrustScore(double min_score) const
{
    LOCK(m_db_mutex);
    
    std::vector<std::string> matching_users;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    for (iterator->Seek(DB_BRIGHTID_USER); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, std::string> key;
        if (!iterator->GetKey(key) || key.first != DB_BRIGHTID_USER) {
            break;
        }
        
        BrightIDUser user;
        if (iterator->GetValue(user) && user.trust_score >= min_score) {
            matching_users.push_back(key.second);
        }
    }
    
    return matching_users;
}

std::vector<std::string> CBrightIDUserDB::FindUsersAfterTimestamp(int64_t timestamp) const
{
    LOCK(m_db_mutex);
    
    std::vector<std::string> matching_users;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    for (iterator->Seek(DB_BRIGHTID_USER); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, std::string> key;
        if (!iterator->GetKey(key) || key.first != DB_BRIGHTID_USER) {
            break;
        }
        
        BrightIDUser user;
        if (iterator->GetValue(user) && user.verification_timestamp >= timestamp) {
            matching_users.push_back(key.second);
        }
    }
    
    return matching_users;
}

std::vector<std::string> CBrightIDUserDB::FindExpiringUsers(int64_t days_until_expiry) const
{
    LOCK(m_db_mutex);
    
    int64_t expiry_threshold = GetTime() + (days_until_expiry * 86400);
    std::vector<std::string> expiring_users;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    for (iterator->Seek(DB_BRIGHTID_USER); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, std::string> key;
        if (!iterator->GetKey(key) || key.first != DB_BRIGHTID_USER) {
            break;
        }
        
        BrightIDUser user;
        if (iterator->GetValue(user) && 
            user.expiration_timestamp > 0 && 
            user.expiration_timestamp <= expiry_threshold) {
            expiring_users.push_back(key.second);
        }
    }
    
    LogPrintf("O BrightID DB: Found %d users expiring within %d days\n", 
              expiring_users.size(), days_until_expiry);
    return expiring_users;
}

std::vector<CPubKey> CBrightIDUserDB::FindUsersByBirthCurrency(const std::string& birth_currency) const
{
    LOCK(m_db_mutex);
    
    std::vector<CPubKey> matching_users;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    for (iterator->Seek(DB_BRIGHTID_USER); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, std::string> key;
        if (!iterator->GetKey(key) || key.first != DB_BRIGHTID_USER) {
            break;
        }
        
        BrightIDUser user;
        if (iterator->GetValue(user)) {
            // Birth currency is stored in context_id as "COUNTRY:CURRENCY"
            // Example: "USA:OUSD", "MEX:OMXN", "FRA:OEUR"
            size_t colon_pos = user.context_id.find(':');
            if (colon_pos != std::string::npos) {
                std::string stored_currency = user.context_id.substr(colon_pos + 1);
                
                // Match birth currency and ensure user is verified and active
                if (stored_currency == birth_currency && 
                    user.IsVerified() && 
                    user.is_active) {
                    
                    // Get user's O address (public key)
                    auto o_address = GetOAddress(key.second);
                    if (o_address.has_value()) {
                        // Convert hex string to CPubKey
                        std::vector<unsigned char> pubkey_bytes = ParseHex(o_address.value());
                        if (pubkey_bytes.size() == 33 || pubkey_bytes.size() == 65) {
                            CPubKey pubkey(pubkey_bytes.begin(), pubkey_bytes.end());
                            if (pubkey.IsValid()) {
                                matching_users.push_back(pubkey);
                            }
                        }
                    }
                }
            }
        }
    }
    
    LogDebug(BCLog::NET, "O BrightID DB: Found %d users with birth currency %s\n", 
             matching_users.size(), birth_currency.c_str());
    return matching_users;
}

// ===== Statistics =====

size_t CBrightIDUserDB::GetUserCount() const
{
    LOCK(m_db_mutex);
    
    size_t count = 0;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    for (iterator->Seek(DB_BRIGHTID_USER); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, std::string> key;
        if (!iterator->GetKey(key) || key.first != DB_BRIGHTID_USER) {
            break;
        }
        count++;
    }
    
    return count;
}

size_t CBrightIDUserDB::GetVerifiedUserCount() const
{
    LOCK(m_db_mutex);
    
    size_t count = 0;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    for (iterator->Seek(DB_BRIGHTID_USER); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, std::string> key;
        if (!iterator->GetKey(key) || key.first != DB_BRIGHTID_USER) {
            break;
        }
        
        BrightIDUser user;
        if (iterator->GetValue(user) && user.IsVerified()) {
            count++;
        }
    }
    
    return count;
}

size_t CBrightIDUserDB::GetActiveUserCount() const
{
    LOCK(m_db_mutex);
    
    size_t count = 0;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    for (iterator->Seek(DB_BRIGHTID_USER); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, std::string> key;
        if (!iterator->GetKey(key) || key.first != DB_BRIGHTID_USER) {
            break;
        }
        
        BrightIDUser user;
        if (iterator->GetValue(user) && user.IsActive()) {
            count++;
        }
    }
    
    return count;
}

std::map<BrightIDStatus, size_t> CBrightIDUserDB::GetUserCountByStatus() const
{
    LOCK(m_db_mutex);
    
    std::map<BrightIDStatus, size_t> status_counts;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    for (iterator->Seek(DB_BRIGHTID_USER); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, std::string> key;
        if (!iterator->GetKey(key) || key.first != DB_BRIGHTID_USER) {
            break;
        }
        
        BrightIDUser user;
        if (iterator->GetValue(user)) {
            status_counts[user.status]++;
        }
    }
    
    return status_counts;
}

double CBrightIDUserDB::GetAverageTrustScore() const
{
    LOCK(m_db_mutex);
    
    double total_trust = 0.0;
    size_t count = 0;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    for (iterator->Seek(DB_BRIGHTID_USER); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, std::string> key;
        if (!iterator->GetKey(key) || key.first != DB_BRIGHTID_USER) {
            break;
        }
        
        BrightIDUser user;
        if (iterator->GetValue(user) && user.IsVerified()) {
            total_trust += user.trust_score;
            count++;
        }
    }
    
    return count > 0 ? total_trust / count : 0.0;
}

// ===== Maintenance =====

bool CBrightIDUserDB::PruneExpiredUsers(int64_t cutoff_timestamp)
{
    LOCK(m_db_mutex);
    
    std::vector<std::string> to_erase;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    for (iterator->Seek(DB_BRIGHTID_USER); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, std::string> key;
        if (!iterator->GetKey(key) || key.first != DB_BRIGHTID_USER) {
            break;
        }
        
        BrightIDUser user;
        if (iterator->GetValue(user)) {
            // Prune if expired before cutoff
            if (user.expiration_timestamp > 0 && user.expiration_timestamp < cutoff_timestamp) {
                to_erase.push_back(key.second);
            }
        }
    }
    
    if (!to_erase.empty()) {
        bool success = BatchEraseUsers(to_erase);
        if (success) {
            LogPrintf("O BrightID DB: Pruned %d expired users (before %d)\n", 
                      to_erase.size(), cutoff_timestamp);
        }
        return success;
    }
    
    return true;
}

bool CBrightIDUserDB::PruneInactiveUsers(int64_t inactive_days)
{
    LOCK(m_db_mutex);
    
    int64_t cutoff_timestamp = GetTime() - (inactive_days * 86400);
    std::vector<std::string> to_erase;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    for (iterator->Seek(DB_BRIGHTID_USER); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, std::string> key;
        if (!iterator->GetKey(key) || key.first != DB_BRIGHTID_USER) {
            break;
        }
        
        BrightIDUser user;
        if (iterator->GetValue(user)) {
            // Prune if not active and last verification was before cutoff
            if (!user.is_active && user.verification_timestamp < cutoff_timestamp) {
                to_erase.push_back(key.second);
            }
        }
    }
    
    if (!to_erase.empty()) {
        bool success = BatchEraseUsers(to_erase);
        if (success) {
            LogPrintf("O BrightID DB: Pruned %d inactive users (>%d days)\n", 
                      to_erase.size(), inactive_days);
        }
        return success;
    }
    
    return true;
}

void CBrightIDUserDB::Compact()
{
    LOCK(m_db_mutex);
    
    LogPrintf("O BrightID DB: Database compaction requested\n");
    LogPrintf("O BrightID DB: Note: Compaction happens automatically via LevelDB\n");
    LogPrintf("O BrightID DB: To force compaction, restart node with -reindex\n");
    
    // Note: CDBWrapper doesn't expose CompactRange publicly
    // Compaction happens automatically in LevelDB
    // Manual compaction can be triggered via -reindex flag
}

size_t CBrightIDUserDB::EstimateSize() const
{
    LOCK(m_db_mutex);
    return m_db->DynamicMemoryUsage();
}

std::optional<fs::path> CBrightIDUserDB::StoragePath() const
{
    return m_db->StoragePath();
}

// ===== Backup/Restore =====

bool CBrightIDUserDB::ExportUsers(const fs::path& export_path) const
{
    LOCK(m_db_mutex);
    
    try {
        auto all_users = GetAllUsers();
        
        // TODO: Implement proper serialization to file
        // For now, just log the count
        LogPrintf("O BrightID DB: Exporting %d users to %s\n", 
                  all_users.size(), fs::PathToString(export_path).c_str());
        
        return true;
    } catch (const std::exception& e) {
        LogPrintf("O BrightID DB: Export failed: %s\n", e.what());
        return false;
    }
}

bool CBrightIDUserDB::ImportUsers(const fs::path& import_path)
{
    LOCK(m_db_mutex);
    
    try {
        // TODO: Implement proper deserialization from file
        LogPrintf("O BrightID DB: Importing users from %s\n", fs::PathToString(import_path).c_str());
        
        return true;
    } catch (const std::exception& e) {
        LogPrintf("O BrightID DB: Import failed: %s\n", e.what());
        return false;
    }
}

bool CBrightIDUserDB::VerifyIntegrity() const
{
    LOCK(m_db_mutex);
    
    size_t total_users = 0;
    size_t corrupted_users = 0;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    for (iterator->Seek(DB_BRIGHTID_USER); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, std::string> key;
        if (!iterator->GetKey(key) || key.first != DB_BRIGHTID_USER) {
            break;
        }
        
        total_users++;
        
        BrightIDUser user;
        if (!iterator->GetValue(user)) {
            corrupted_users++;
            LogPrintf("O BrightID DB: Corrupted user entry: %s\n", key.second.substr(0, 16));
        }
    }
    
    LogPrintf("O BrightID DB: Integrity check complete. Total: %d, Corrupted: %d\n",
              total_users, corrupted_users);
    
    return corrupted_users == 0;
}

} // namespace OConsensus

