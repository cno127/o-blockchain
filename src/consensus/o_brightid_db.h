// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_O_BRIGHTID_DB_H
#define BITCOIN_CONSENSUS_O_BRIGHTID_DB_H

#include <dbwrapper.h>
#include <consensus/brightid_integration.h>
#include <pubkey.h>
#include <sync.h>
#include <uint256.h>

#include <memory>
#include <optional>
#include <vector>
#include <string>

namespace OConsensus {

/** Database key prefixes for BrightID user data */
static constexpr uint8_t DB_BRIGHTID_USER = 'u';           // User data by BrightID address
static constexpr uint8_t DB_BRIGHTID_TO_O = 'b';           // BrightID address -> O address mapping
static constexpr uint8_t DB_O_TO_BRIGHTID = 'o';           // O address -> BrightID address mapping
static constexpr uint8_t DB_ANONYMOUS_ID = 'a';            // BrightID address -> Anonymous ID
static constexpr uint8_t DB_ANONYMOUS_REP = 'r';           // Anonymous ID -> Reputation score
static constexpr uint8_t DB_BRIGHTID_STATS = 's';          // Statistics
static constexpr uint8_t DB_BRIGHTID_VERSION = 'v';        // Database version

/** BrightID User Database - Persistent storage for Proof of Personhood data */
class CBrightIDUserDB {
private:
    std::unique_ptr<CDBWrapper> m_db;
    mutable RecursiveMutex m_db_mutex;
    
public:
    explicit CBrightIDUserDB(size_t cache_size, bool memory_only = false, bool wipe_data = false);
    ~CBrightIDUserDB();
    
    // ===== User Operations =====
    
    /** Write user data to database */
    bool WriteUser(const std::string& brightid_address, const BrightIDUser& user);
    
    /** Read user data from database */
    std::optional<BrightIDUser> ReadUser(const std::string& brightid_address) const;
    
    /** Check if user exists in database */
    bool HasUser(const std::string& brightid_address) const;
    
    /** Erase user from database */
    bool EraseUser(const std::string& brightid_address);
    
    /** Update user status */
    bool UpdateUserStatus(const std::string& brightid_address, BrightIDStatus status);
    
    /** Update user trust score */
    bool UpdateTrustScore(const std::string& brightid_address, double trust_score);
    
    // ===== Address Mapping Operations =====
    
    /** Link BrightID address to O address */
    bool LinkAddresses(const std::string& brightid_address, const std::string& o_address);
    
    /** Unlink BrightID address from O address */
    bool UnlinkAddresses(const std::string& brightid_address);
    
    /** Get O address for BrightID */
    std::optional<std::string> GetOAddress(const std::string& brightid_address) const;
    
    /** Get BrightID address for O address */
    std::optional<std::string> GetBrightIDAddress(const std::string& o_address) const;
    
    // ===== Anonymous ID Operations =====
    
    /** Store anonymous ID mapping */
    bool WriteAnonymousID(const std::string& brightid_address, const std::string& anonymous_id);
    
    /** Get anonymous ID for BrightID address */
    std::optional<std::string> GetAnonymousID(const std::string& brightid_address) const;
    
    /** Write anonymous reputation */
    bool WriteAnonymousReputation(const std::string& anonymous_id, double reputation);
    
    /** Get anonymous reputation */
    std::optional<double> GetAnonymousReputation(const std::string& anonymous_id) const;
    
    /** Erase anonymous data */
    bool EraseAnonymousData(const std::string& brightid_address);
    
    // ===== Batch Operations =====
    
    /** Get all verified users */
    std::vector<BrightIDUser> GetVerifiedUsers() const;
    
    /** Get all active users */
    std::vector<BrightIDUser> GetActiveUsers() const;
    
    /** Get users by status */
    std::vector<BrightIDUser> GetUsersByStatus(BrightIDStatus status) const;
    
    /** Get all users (for iteration/migration) */
    std::vector<std::pair<std::string, BrightIDUser>> GetAllUsers() const;
    
    /** Batch write multiple users */
    bool BatchWriteUsers(const std::vector<std::pair<std::string, BrightIDUser>>& batch);
    
    /** Batch erase multiple users */
    bool BatchEraseUsers(const std::vector<std::string>& brightid_addresses);
    
    // ===== Query Operations =====
    
    /** Find users by verification method */
    std::vector<std::string> FindUsersByMethod(BrightIDVerificationMethod method) const;
    
    /** Find users with trust score above threshold */
    std::vector<std::string> FindUsersByTrustScore(double min_score) const;
    
    /** Find users verified after timestamp */
    std::vector<std::string> FindUsersAfterTimestamp(int64_t timestamp) const;
    
    /** Find users expiring soon */
    std::vector<std::string> FindExpiringUsers(int64_t days_until_expiry) const;
    
    /** Find users by birth currency (for stabilization rewards) */
    std::vector<CPubKey> FindUsersByBirthCurrency(const std::string& birth_currency) const;
    
    // ===== Statistics =====
    
    /** Get total number of users in database */
    size_t GetUserCount() const;
    
    /** Get verified user count */
    size_t GetVerifiedUserCount() const;
    
    /** Get active user count */
    size_t GetActiveUserCount() const;
    
    /** Get users by status count */
    std::map<BrightIDStatus, size_t> GetUserCountByStatus() const;
    
    /** Get average trust score */
    double GetAverageTrustScore() const;
    
    // ===== Maintenance =====
    
    /** Prune expired users before cutoff timestamp */
    bool PruneExpiredUsers(int64_t cutoff_timestamp);
    
    /** Prune inactive users (not active for specified days) */
    bool PruneInactiveUsers(int64_t inactive_days);
    
    /** Compact the database */
    void Compact();
    
    /** Get database size estimate */
    size_t EstimateSize() const;
    
    /** Get database path */
    std::optional<fs::path> StoragePath() const;
    
    // ===== Backup/Restore =====
    
    /** Export all users to file (for backup) */
    bool ExportUsers(const fs::path& export_path) const;
    
    /** Import users from file (for restore) */
    bool ImportUsers(const fs::path& import_path);
    
    /** Verify database integrity */
    bool VerifyIntegrity() const;
};

/** Global BrightID user database instance */
extern std::unique_ptr<CBrightIDUserDB> g_brightid_db;

} // namespace OConsensus

#endif // BITCOIN_CONSENSUS_O_BRIGHTID_DB_H

