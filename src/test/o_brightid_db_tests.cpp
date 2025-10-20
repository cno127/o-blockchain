// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/o_brightid_db.h>
#include <consensus/brightid_integration.h>
#include <test/util/setup_common.h>
#include <boost/test/unit_test.hpp>
#include <util/strencodings.h>

using namespace OConsensus;

BOOST_FIXTURE_TEST_SUITE(o_brightid_db_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(brightid_db_basic_write_read)
{
    // Create temporary database
    auto db = std::make_unique<CBrightIDUserDB>(1 << 20, true, false); // 1MB cache, memory-only
    
    // Create test user
    BrightIDUser user;
    user.brightid_address = "test_brightid_123";
    user.context_id = "o_blockchain";
    user.status = BrightIDStatus::VERIFIED;
    user.method = BrightIDVerificationMethod::MEETUP;
    user.verification_timestamp = 1234567890;
    user.expiration_timestamp = 1234567890 + 86400 * 30;
    user.trust_score = 0.85;
    user.is_active = true;
    user.connections = {"conn1", "conn2", "conn3"};
    
    // Write user
    BOOST_CHECK(db->WriteUser("test_brightid_123", user));
    
    // Read user back
    auto read_user = db->ReadUser("test_brightid_123");
    BOOST_CHECK(read_user.has_value());
    BOOST_CHECK_EQUAL(read_user->brightid_address, "test_brightid_123");
    BOOST_CHECK_EQUAL(read_user->context_id, "o_blockchain");
    BOOST_CHECK(read_user->status == BrightIDStatus::VERIFIED);
    BOOST_CHECK(read_user->method == BrightIDVerificationMethod::MEETUP);
    BOOST_CHECK_EQUAL(read_user->verification_timestamp, 1234567890);
    BOOST_CHECK_EQUAL(read_user->trust_score, 0.85);
    BOOST_CHECK_EQUAL(read_user->is_active, true);
    BOOST_CHECK_EQUAL(read_user->connections.size(), 3);
}

BOOST_AUTO_TEST_CASE(brightid_db_double_serialization)
{
    // Test double precision serialization/deserialization
    auto db = std::make_unique<CBrightIDUserDB>(1 << 20, true, false);
    
    // Test various double values
    std::vector<double> test_values = {
        0.0,
        0.000001,  // Minimum precision
        0.5,
        0.123456,  // Exactly 6 decimals
        0.999999,  // Maximum precision
        1.0,
        123.456789,
        999999.999999
    };
    
    for (size_t i = 0; i < test_values.size(); i++) {
        BrightIDUser user;
        user.brightid_address = "test_" + std::to_string(i);
        user.trust_score = test_values[i];
        
        BOOST_CHECK(db->WriteUser(user.brightid_address, user));
        
        auto read_user = db->ReadUser(user.brightid_address);
        BOOST_CHECK(read_user.has_value());
        
        // Check precision (6 decimal places)
        BOOST_CHECK_CLOSE(read_user->trust_score, test_values[i], 0.0001); // 0.0001% tolerance
    }
}

BOOST_AUTO_TEST_CASE(brightid_db_address_linking)
{
    auto db = std::make_unique<CBrightIDUserDB>(1 << 20, true, false);
    
    // First create a verified user
    BrightIDUser user;
    user.brightid_address = "brightid_abc";
    user.status = BrightIDStatus::VERIFIED;
    BOOST_CHECK(db->WriteUser("brightid_abc", user));
    
    // Test linking addresses
    BOOST_CHECK(db->LinkAddresses("brightid_abc", "o_address_xyz"));
    
    // Verify forward mapping
    auto o_addr = db->GetOAddress("brightid_abc");
    BOOST_CHECK(o_addr.has_value());
    BOOST_CHECK_EQUAL(*o_addr, "o_address_xyz");
    
    // Verify reverse mapping
    auto brightid_addr = db->GetBrightIDAddress("o_address_xyz");
    BOOST_CHECK(brightid_addr.has_value());
    BOOST_CHECK_EQUAL(*brightid_addr, "brightid_abc");
    
    // Test unlinking
    BOOST_CHECK(db->UnlinkAddresses("brightid_abc"));
    
    // Verify mappings removed
    BOOST_CHECK(!db->GetOAddress("brightid_abc").has_value());
    BOOST_CHECK(!db->GetBrightIDAddress("o_address_xyz").has_value());
}

BOOST_AUTO_TEST_CASE(brightid_db_anonymous_reputation)
{
    auto db = std::make_unique<CBrightIDUserDB>(1 << 20, true, false);
    
    // Write anonymous reputation
    BOOST_CHECK(db->WriteAnonymousReputation("anon_id_123", 0.75));
    
    // Read it back
    auto reputation = db->GetAnonymousReputation("anon_id_123");
    BOOST_CHECK(reputation.has_value());
    BOOST_CHECK_CLOSE(*reputation, 0.75, 0.0001);
    
    // Update reputation
    BOOST_CHECK(db->WriteAnonymousReputation("anon_id_123", 0.90));
    auto updated = db->GetAnonymousReputation("anon_id_123");
    BOOST_CHECK(updated.has_value());
    BOOST_CHECK_CLOSE(*updated, 0.90, 0.0001);
    
    // Test edge cases
    BOOST_CHECK(db->WriteAnonymousReputation("anon_min", 0.000001));
    BOOST_CHECK(db->WriteAnonymousReputation("anon_max", 0.999999));
    
    auto min_rep = db->GetAnonymousReputation("anon_min");
    auto max_rep = db->GetAnonymousReputation("anon_max");
    BOOST_CHECK_CLOSE(*min_rep, 0.000001, 0.0001);
    BOOST_CHECK_CLOSE(*max_rep, 0.999999, 0.0001);
}

BOOST_AUTO_TEST_CASE(brightid_db_user_status_update)
{
    auto db = std::make_unique<CBrightIDUserDB>(1 << 20, true, false);
    
    // Create user
    BrightIDUser user;
    user.brightid_address = "test_user";
    user.status = BrightIDStatus::UNVERIFIED;
    BOOST_CHECK(db->WriteUser("test_user", user));
    
    // Update status
    BOOST_CHECK(db->UpdateUserStatus("test_user", BrightIDStatus::VERIFIED));
    
    // Verify update
    auto updated_user = db->ReadUser("test_user");
    BOOST_CHECK(updated_user.has_value());
    BOOST_CHECK(updated_user->status == BrightIDStatus::VERIFIED);
}

BOOST_AUTO_TEST_CASE(brightid_db_trust_score_update)
{
    auto db = std::make_unique<CBrightIDUserDB>(1 << 20, true, false);
    
    BrightIDUser user;
    user.brightid_address = "test_user";
    user.trust_score = 0.5;
    BOOST_CHECK(db->WriteUser("test_user", user));
    
    // Update trust score
    BOOST_CHECK(db->UpdateTrustScore("test_user", 0.95));
    
    // Verify update
    auto updated = db->ReadUser("test_user");
    BOOST_CHECK(updated.has_value());
    BOOST_CHECK_CLOSE(updated->trust_score, 0.95, 0.0001);
}

BOOST_AUTO_TEST_CASE(brightid_db_batch_operations)
{
    auto db = std::make_unique<CBrightIDUserDB>(1 << 20, true, false);
    
    // Create batch of users
    std::vector<std::pair<std::string, BrightIDUser>> batch;
    for (int i = 0; i < 100; i++) {
        BrightIDUser user;
        user.brightid_address = "batch_user_" + std::to_string(i);
        user.status = BrightIDStatus::VERIFIED;
        user.trust_score = 0.5 + (i * 0.005); // 0.5 to 0.995
        batch.emplace_back(user.brightid_address, user);
    }
    
    // Batch write
    BOOST_CHECK(db->BatchWriteUsers(batch));
    
    // Verify all users were written
    for (int i = 0; i < 100; i++) {
        std::string addr = "batch_user_" + std::to_string(i);
        auto user = db->ReadUser(addr);
        BOOST_CHECK(user.has_value());
        BOOST_CHECK_CLOSE(user->trust_score, 0.5 + (i * 0.005), 0.0001);
    }
}

BOOST_AUTO_TEST_CASE(brightid_db_has_user)
{
    auto db = std::make_unique<CBrightIDUserDB>(1 << 20, true, false);
    
    // Non-existent user
    BOOST_CHECK(!db->HasUser("nonexistent"));
    
    // Create user
    BrightIDUser user;
    user.brightid_address = "test_user";
    BOOST_CHECK(db->WriteUser("test_user", user));
    
    // Check exists
    BOOST_CHECK(db->HasUser("test_user"));
}

BOOST_AUTO_TEST_CASE(brightid_db_erase_user)
{
    auto db = std::make_unique<CBrightIDUserDB>(1 << 20, true, false);
    
    // Create user
    BrightIDUser user;
    user.brightid_address = "test_user";
    BOOST_CHECK(db->WriteUser("test_user", user));
    BOOST_CHECK(db->HasUser("test_user"));
    
    // Erase user
    BOOST_CHECK(db->EraseUser("test_user"));
    
    // Verify removed
    BOOST_CHECK(!db->HasUser("test_user"));
    auto read = db->ReadUser("test_user");
    BOOST_CHECK(!read.has_value());
}

BOOST_AUTO_TEST_CASE(brightid_db_get_verified_users)
{
    auto db = std::make_unique<CBrightIDUserDB>(1 << 20, true, false);
    
    // Create mix of users
    for (int i = 0; i < 10; i++) {
        BrightIDUser user;
        user.brightid_address = "user_" + std::to_string(i);
        user.status = (i % 2 == 0) ? BrightIDStatus::VERIFIED : BrightIDStatus::UNVERIFIED;
        BOOST_CHECK(db->WriteUser(user.brightid_address, user));
    }
    
    // Get verified users
    auto verified = db->GetVerifiedUsers();
    BOOST_CHECK_EQUAL(verified.size(), 5); // 5 verified users
    
    // Verify all are verified
    for (const auto& user : verified) {
        BOOST_CHECK(user.status == BrightIDStatus::VERIFIED);
    }
}

BOOST_AUTO_TEST_CASE(brightid_db_edge_cases)
{
    auto db = std::make_unique<CBrightIDUserDB>(1 << 20, true, false);
    
    // Test empty strings
    BrightIDUser user;
    user.brightid_address = "";
    BOOST_CHECK(db->WriteUser("", user));
    auto read = db->ReadUser("");
    BOOST_CHECK(read.has_value());
    
    // Test very long addresses
    std::string long_addr(1000, 'x');
    user.brightid_address = long_addr;
    BOOST_CHECK(db->WriteUser(long_addr, user));
    read = db->ReadUser(long_addr);
    BOOST_CHECK(read.has_value());
    
    // Test special characters
    user.brightid_address = "!@#$%^&*()_+-=[]{}|;':\",./<>?";
    BOOST_CHECK(db->WriteUser(user.brightid_address, user));
    read = db->ReadUser(user.brightid_address);
    BOOST_CHECK(read.has_value());
}

BOOST_AUTO_TEST_CASE(brightid_db_concurrent_write_read)
{
    auto db = std::make_unique<CBrightIDUserDB>(1 << 20, true, false);
    
    // Write many users rapidly (simulates concurrent access)
    for (int i = 0; i < 1000; i++) {
        BrightIDUser user;
        user.brightid_address = "concurrent_user_" + std::to_string(i);
        user.status = BrightIDStatus::VERIFIED;
        user.trust_score = static_cast<double>(i) / 1000.0;
        BOOST_CHECK(db->WriteUser(user.brightid_address, user));
    }
    
    // Read all back
    for (int i = 0; i < 1000; i++) {
        std::string addr = "concurrent_user_" + std::to_string(i);
        auto user = db->ReadUser(addr);
        BOOST_CHECK(user.has_value());
        BOOST_CHECK_CLOSE(user->trust_score, static_cast<double>(i) / 1000.0, 0.0001);
    }
}

BOOST_AUTO_TEST_SUITE_END()

