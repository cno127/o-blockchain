// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/o_brightid_db.h>
#include <common/args.h>
#include <logging.h>
#include <util/fs.h>
#include <util/time.h>
#include <util/strencodings.h>
#include <streams.h>

namespace OConsensus {

// Global instance (initialized in init.cpp)
// Legacy name kept for backward compatibility during transition
std::unique_ptr<CIdentityUserDB> g_brightid_db;
std::unique_ptr<CIdentityUserDB> g_identity_db;  // New name

CIdentityUserDB::CIdentityUserDB(size_t cache_size, bool memory_only, bool wipe_data)
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

CIdentityUserDB::~CIdentityUserDB() = default;

// ===== User Operations =====

bool CIdentityUserDB::WriteUser(const std::string& provider_address, const VerifiedUser& user)
{
    LOCK(m_db_mutex);
    
    CDBBatch batch(*m_db);
    batch.Write(std::make_pair(DB_BRIGHTID_USER, provider_address), user);
    
    bool success = m_db->WriteBatch(batch, true);
    
    if (success) {
        LogDebug(BCLog::NET, "O Identity DB: Wrote user %s (status=%d, trust=%.2f)\n",
                 provider_address.substr(0, 16), static_cast<int>(user.status), user.trust_score);
    } else {
        LogPrintf("O Identity DB: Failed to write user %s\n", provider_address.substr(0, 16));
    }
    
    return success;
}

std::optional<VerifiedUser> CIdentityUserDB::ReadUser(const std::string& provider_address) const
{
    LOCK(m_db_mutex);
    
    VerifiedUser user;
    if (m_db->Read(std::make_pair(DB_BRIGHTID_USER, provider_address), user)) {
        return user;
    }
    
    return std::nullopt;
}

bool CIdentityUserDB::HasUser(const std::string& provider_address) const
{
    LOCK(m_db_mutex);
    return m_db->Exists(std::make_pair(DB_BRIGHTID_USER, provider_address));
}

bool CIdentityUserDB::EraseUser(const std::string& provider_address)
{
    LOCK(m_db_mutex);
    
    CDBBatch batch(*m_db);
    
    // Erase user data
    batch.Erase(std::make_pair(DB_BRIGHTID_USER, provider_address));
    
    // Also erase address mappings
    auto o_addr = GetOAddress(provider_address);
    if (o_addr.has_value()) {
        batch.Erase(std::make_pair(DB_BRIGHTID_TO_O, provider_address));
        batch.Erase(std::make_pair(DB_O_TO_BRIGHTID, o_addr.value()));
    }
    
    // Erase anonymous data
    auto anon_id = GetAnonymousID(provider_address);
    if (anon_id.has_value()) {
        batch.Erase(std::make_pair(DB_ANONYMOUS_ID, provider_address));
        batch.Erase(std::make_pair(DB_ANONYMOUS_REP, anon_id.value()));
    }
    
    bool success = m_db->WriteBatch(batch, true);
    
    if (success) {
        LogDebug(BCLog::NET, "O Identity DB: Erased user %s\n", provider_address.substr(0, 16));
    }
    
    return success;
}

bool CIdentityUserDB::UpdateUserStatus(const std::string& provider_address, BrightIDStatus status)
{
    LOCK(m_db_mutex);
    
    auto user_opt = ReadUser(provider_address);
    if (!user_opt.has_value()) {
        return false;
    }
    
    VerifiedUser user = user_opt.value();
    user.status = status;
    
    return WriteUser(provider_address, user);
}

bool CIdentityUserDB::UpdateTrustScore(const std::string& provider_address, double trust_score)
{
    LOCK(m_db_mutex);
    
    auto user_opt = ReadUser(provider_address);
    if (!user_opt.has_value()) {
        return false;
    }
    
    VerifiedUser user = user_opt.value();
    user.trust_score = trust_score;
    
    return WriteUser(provider_address, user);
}

// ===== Address Mapping Operations =====

bool CIdentityUserDB::LinkAddresses(const std::string& provider_address, const std::string& o_address)
{
    LOCK(m_db_mutex);
    
    CDBBatch batch(*m_db);
    
    // Store both directions for fast lookup
    batch.Write(std::make_pair(DB_BRIGHTID_TO_O, provider_address), o_address);
    batch.Write(std::make_pair(DB_O_TO_BRIGHTID, o_address), provider_address);
    
    bool success = m_db->WriteBatch(batch, true);
    
    if (success) {
        LogDebug(BCLog::NET, "O BrightID DB: Linked %s <-> %s\n",
                 provider_address.substr(0, 16), o_address.substr(0, 16));
    }
    
    return success;
}

bool CIdentityUserDB::UnlinkAddresses(const std::string& provider_address)
{
    LOCK(m_db_mutex);
    
    auto o_addr = GetOAddress(provider_address);
    if (!o_addr.has_value()) {
        return false;
    }
    
    CDBBatch batch(*m_db);
    batch.Erase(std::make_pair(DB_BRIGHTID_TO_O, provider_address));
    batch.Erase(std::make_pair(DB_O_TO_BRIGHTID, o_addr.value()));
    
    bool success = m_db->WriteBatch(batch, true);
    
    if (success) {
        LogDebug(BCLog::NET, "O BrightID DB: Unlinked %s <-> %s\n",
                 provider_address.substr(0, 16), o_addr.value().substr(0, 16));
    }
    
    return success;
}

std::optional<std::string> CIdentityUserDB::GetOAddress(const std::string& provider_address) const
{
    LOCK(m_db_mutex);
    
    std::string o_address;
    if (m_db->Read(std::make_pair(DB_BRIGHTID_TO_O, provider_address), o_address)) {
        return o_address;
    }
    
    return std::nullopt;
}

std::optional<std::string> CIdentityUserDB::GetBrightIDAddress(const std::string& o_address) const
{
    LOCK(m_db_mutex);
    
    std::string provider_address;
    if (m_db->Read(std::make_pair(DB_O_TO_BRIGHTID, o_address), provider_address)) {
        return provider_address;
    }
    
    return std::nullopt;
}

// ===== Anonymous ID Operations =====

bool CIdentityUserDB::WriteAnonymousID(const std::string& provider_address, const std::string& anonymous_id)
{
    LOCK(m_db_mutex);
    
    CDBBatch batch(*m_db);
    batch.Write(std::make_pair(DB_ANONYMOUS_ID, provider_address), anonymous_id);
    
    return m_db->WriteBatch(batch, true);
}

std::optional<std::string> CIdentityUserDB::GetAnonymousID(const std::string& provider_address) const
{
    LOCK(m_db_mutex);
    
    std::string anonymous_id;
    if (m_db->Read(std::make_pair(DB_ANONYMOUS_ID, provider_address), anonymous_id)) {
        return anonymous_id;
    }
    
    return std::nullopt;
}

bool CIdentityUserDB::WriteAnonymousReputation(const std::string& anonymous_id, double reputation)
{
    LOCK(m_db_mutex);
    
    // Convert double to int64_t for serialization (multiply by 1000000 for 6 decimal precision)
    int64_t reputation_int = static_cast<int64_t>(reputation * 1000000);
    
    CDBBatch batch(*m_db);
    batch.Write(std::make_pair(DB_ANONYMOUS_REP, anonymous_id), reputation_int);
    
    return m_db->WriteBatch(batch, true);
}

std::optional<double> CIdentityUserDB::GetAnonymousReputation(const std::string& anonymous_id) const
{
    LOCK(m_db_mutex);
    
    int64_t reputation_int;
    if (m_db->Read(std::make_pair(DB_ANONYMOUS_REP, anonymous_id), reputation_int)) {
        // Convert back to double
        return static_cast<double>(reputation_int) / 1000000.0;
    }
    
    return std::nullopt;
}

bool CIdentityUserDB::EraseAnonymousData(const std::string& provider_address)
{
    LOCK(m_db_mutex);
    
    auto anon_id = GetAnonymousID(provider_address);
    if (!anon_id.has_value()) {
        return false;
    }
    
    CDBBatch batch(*m_db);
    batch.Erase(std::make_pair(DB_ANONYMOUS_ID, provider_address));
    batch.Erase(std::make_pair(DB_ANONYMOUS_REP, anon_id.value()));
    
    return m_db->WriteBatch(batch, true);
}

// ===== Batch Operations =====

std::vector<VerifiedUser> CIdentityUserDB::GetVerifiedUsers() const
{
    LOCK(m_db_mutex);
    
    std::vector<VerifiedUser> verified_users;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    for (iterator->Seek(DB_BRIGHTID_USER); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, std::string> key;
        if (!iterator->GetKey(key) || key.first != DB_BRIGHTID_USER) {
            break;
        }
        
        VerifiedUser user;
        if (iterator->GetValue(user) && user.IsVerified()) {
            verified_users.push_back(user);
        }
    }
    
    LogDebug(BCLog::NET, "O BrightID DB: Retrieved %d verified users\n", verified_users.size());
    return verified_users;
}

std::vector<VerifiedUser> CIdentityUserDB::GetActiveUsers() const
{
    LOCK(m_db_mutex);
    
    std::vector<VerifiedUser> active_users;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    for (iterator->Seek(DB_BRIGHTID_USER); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, std::string> key;
        if (!iterator->GetKey(key) || key.first != DB_BRIGHTID_USER) {
            break;
        }
        
        VerifiedUser user;
        if (iterator->GetValue(user) && user.IsActive()) {
            active_users.push_back(user);
        }
    }
    
    LogDebug(BCLog::NET, "O BrightID DB: Retrieved %d active users\n", active_users.size());
    return active_users;
}

std::vector<VerifiedUser> CIdentityUserDB::GetUsersByStatus(BrightIDStatus status) const
{
    LOCK(m_db_mutex);
    
    std::vector<VerifiedUser> users_by_status;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    for (iterator->Seek(DB_BRIGHTID_USER); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, std::string> key;
        if (!iterator->GetKey(key) || key.first != DB_BRIGHTID_USER) {
            break;
        }
        
        VerifiedUser user;
        if (iterator->GetValue(user) && user.status == status) {
            users_by_status.push_back(user);
        }
    }
    
    return users_by_status;
}

std::vector<std::pair<std::string, VerifiedUser>> CIdentityUserDB::GetAllUsers() const
{
    LOCK(m_db_mutex);
    
    std::vector<std::pair<std::string, VerifiedUser>> all_users;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    for (iterator->Seek(DB_BRIGHTID_USER); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, std::string> key;
        if (!iterator->GetKey(key) || key.first != DB_BRIGHTID_USER) {
            break;
        }
        
        VerifiedUser user;
        if (iterator->GetValue(user)) {
            all_users.emplace_back(key.second, user);
        }
    }
    
    LogPrintf("O BrightID DB: Retrieved %d total users from database\n", all_users.size());
    return all_users;
}

bool CIdentityUserDB::BatchWriteUsers(const std::vector<std::pair<std::string, VerifiedUser>>& batch)
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

bool CIdentityUserDB::BatchEraseUsers(const std::vector<std::string>& provider_addresses)
{
    LOCK(m_db_mutex);
    
    CDBBatch batch(*m_db);
    
    for (const auto& addr : provider_addresses) {
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
        LogPrintf("O BrightID DB: Batch erased %d users\n", provider_addresses.size());
    }
    
    return success;
}

// ===== Query Operations =====

std::vector<std::string> CIdentityUserDB::FindUsersByMethod(BrightIDVerificationMethod method) const
{
    LOCK(m_db_mutex);
    
    std::vector<std::string> matching_users;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    for (iterator->Seek(DB_BRIGHTID_USER); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, std::string> key;
        if (!iterator->GetKey(key) || key.first != DB_BRIGHTID_USER) {
            break;
        }
        
        VerifiedUser user;
        if (iterator->GetValue(user) && user.method == method) {
            matching_users.push_back(key.second);
        }
    }
    
    return matching_users;
}

std::vector<std::string> CIdentityUserDB::FindUsersByTrustScore(double min_score) const
{
    LOCK(m_db_mutex);
    
    std::vector<std::string> matching_users;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    for (iterator->Seek(DB_BRIGHTID_USER); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, std::string> key;
        if (!iterator->GetKey(key) || key.first != DB_BRIGHTID_USER) {
            break;
        }
        
        VerifiedUser user;
        if (iterator->GetValue(user) && user.trust_score >= min_score) {
            matching_users.push_back(key.second);
        }
    }
    
    return matching_users;
}

std::vector<std::string> CIdentityUserDB::FindUsersAfterTimestamp(int64_t timestamp) const
{
    LOCK(m_db_mutex);
    
    std::vector<std::string> matching_users;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    for (iterator->Seek(DB_BRIGHTID_USER); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, std::string> key;
        if (!iterator->GetKey(key) || key.first != DB_BRIGHTID_USER) {
            break;
        }
        
        VerifiedUser user;
        if (iterator->GetValue(user) && user.verification_timestamp >= timestamp) {
            matching_users.push_back(key.second);
        }
    }
    
    return matching_users;
}

std::vector<std::string> CIdentityUserDB::FindExpiringUsers(int64_t days_until_expiry) const
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
        
        VerifiedUser user;
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

std::vector<CPubKey> CIdentityUserDB::FindUsersByBirthCurrency(const std::string& birth_currency) const
{
    LOCK(m_db_mutex);
    
    std::vector<CPubKey> matching_users;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    for (iterator->Seek(DB_BRIGHTID_USER); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, std::string> key;
        if (!iterator->GetKey(key) || key.first != DB_BRIGHTID_USER) {
            break;
        }
        
        VerifiedUser user;
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

std::optional<std::string> CIdentityUserDB::GetBirthCurrencyByPubKey(const CPubKey& pubkey) const
{
    LOCK(m_db_mutex);
    
    // Convert public key to hex string (O address)
    // Use the same format as stored in the database (HexStr of pubkey bytes)
    std::vector<unsigned char> pubkey_bytes(pubkey.begin(), pubkey.end());
    std::string o_address = HexStr(pubkey_bytes);
    
    // Get BrightID address from O address
    std::string provider_address;
    if (!m_db->Read(std::make_pair(DB_O_TO_BRIGHTID, o_address), provider_address)) {
        LogDebug(BCLog::NET, "O BrightID DB: No BrightID address found for O address %s\n", 
                 o_address.substr(0, 16).c_str());
        return std::nullopt;
    }
    
    // Read user data
    auto user_opt = ReadUser(provider_address);
    if (!user_opt.has_value()) {
        LogDebug(BCLog::NET, "O BrightID DB: No user found for BrightID address %s\n",
                 provider_address.substr(0, 16).c_str());
        return std::nullopt;
    }
    
    const VerifiedUser& user = user_opt.value();
    
    // Extract birth currency from context_id (format: "COUNTRY:CURRENCY")
    size_t colon_pos = user.context_id.find(':');
    if (colon_pos == std::string::npos || colon_pos >= user.context_id.length() - 1) {
        LogDebug(BCLog::NET, "O BrightID DB: Invalid context_id format for user %s: %s\n",
                 provider_address.substr(0, 16).c_str(), user.context_id.c_str());
        return std::nullopt;
    }
    
    std::string birth_currency = user.context_id.substr(colon_pos + 1);
    LogDebug(BCLog::NET, "O BrightID DB: Found birth currency %s for user %s\n",
             birth_currency.c_str(), provider_address.substr(0, 16).c_str());
    
    return birth_currency;
}

// ===== Statistics =====

size_t CIdentityUserDB::GetUserCount() const
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

size_t CIdentityUserDB::GetVerifiedUserCount() const
{
    LOCK(m_db_mutex);
    
    size_t count = 0;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    for (iterator->Seek(DB_BRIGHTID_USER); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, std::string> key;
        if (!iterator->GetKey(key) || key.first != DB_BRIGHTID_USER) {
            break;
        }
        
        VerifiedUser user;
        if (iterator->GetValue(user) && user.IsVerified()) {
            count++;
        }
    }
    
    return count;
}

size_t CIdentityUserDB::GetActiveUserCount() const
{
    LOCK(m_db_mutex);
    
    size_t count = 0;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    for (iterator->Seek(DB_BRIGHTID_USER); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, std::string> key;
        if (!iterator->GetKey(key) || key.first != DB_BRIGHTID_USER) {
            break;
        }
        
        VerifiedUser user;
        if (iterator->GetValue(user) && user.IsActive()) {
            count++;
        }
    }
    
    return count;
}

std::map<BrightIDStatus, size_t> CIdentityUserDB::GetUserCountByStatus() const
{
    LOCK(m_db_mutex);
    
    std::map<BrightIDStatus, size_t> status_counts;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    for (iterator->Seek(DB_BRIGHTID_USER); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, std::string> key;
        if (!iterator->GetKey(key) || key.first != DB_BRIGHTID_USER) {
            break;
        }
        
        VerifiedUser user;
        if (iterator->GetValue(user)) {
            status_counts[user.status]++;
        }
    }
    
    return status_counts;
}

double CIdentityUserDB::GetAverageTrustScore() const
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
        
        VerifiedUser user;
        if (iterator->GetValue(user) && user.IsVerified()) {
            total_trust += user.trust_score;
            count++;
        }
    }
    
    return count > 0 ? total_trust / count : 0.0;
}

// ===== Maintenance =====

bool CIdentityUserDB::PruneExpiredUsers(int64_t cutoff_timestamp)
{
    LOCK(m_db_mutex);
    
    std::vector<std::string> to_erase;
    std::unique_ptr<CDBIterator> iterator(m_db->NewIterator());
    
    for (iterator->Seek(DB_BRIGHTID_USER); iterator->Valid(); iterator->Next()) {
        std::pair<uint8_t, std::string> key;
        if (!iterator->GetKey(key) || key.first != DB_BRIGHTID_USER) {
            break;
        }
        
        VerifiedUser user;
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

bool CIdentityUserDB::PruneInactiveUsers(int64_t inactive_days)
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
        
        VerifiedUser user;
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

void CIdentityUserDB::Compact()
{
    LOCK(m_db_mutex);
    
    LogPrintf("O BrightID DB: Database compaction requested\n");
    LogPrintf("O BrightID DB: Note: Compaction happens automatically via LevelDB\n");
    LogPrintf("O BrightID DB: To force compaction, restart node with -reindex\n");
    
    // Note: CDBWrapper doesn't expose CompactRange publicly
    // Compaction happens automatically in LevelDB
    // Manual compaction can be triggered via -reindex flag
}

size_t CIdentityUserDB::EstimateSize() const
{
    LOCK(m_db_mutex);
    return m_db->DynamicMemoryUsage();
}

std::optional<fs::path> CIdentityUserDB::StoragePath() const
{
    return m_db->StoragePath();
}

// ===== Backup/Restore =====

bool CIdentityUserDB::ExportUsers(const fs::path& export_path) const
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

bool CIdentityUserDB::ImportUsers(const fs::path& import_path)
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

bool CIdentityUserDB::VerifyIntegrity() const
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
        
        VerifiedUser user;
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

