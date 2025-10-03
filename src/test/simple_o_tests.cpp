// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/simple_o_tests.h>
#include <consensus/user_consensus.h>
#include <key.h>
#include <pubkey.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>
#include <string>

BOOST_FIXTURE_TEST_SUITE(simple_o_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(test_user_consensus_basic) {
    // Test basic user consensus functionality
    UserRegistryConsensus consensus;
    
    // Create test user
    CKey key;
    key.MakeNewKey(true);
    CPubKey pubkey = key.GetPubKey();
    
    OfficialUser test_user;
    test_user.public_key = pubkey;
    test_user.government_id_hash = "test_government_id_hash_123";
    test_user.birth_currency = "USD";
    test_user.country_code = "US";
    test_user.identity_proof_hash = "test_identity_proof_hash";
    test_user.status = UserStatus::PENDING_VERIFICATION;
    
    // Test successful registration
    std::string error_message;
    BOOST_CHECK(consensus.RegisterUser(test_user, error_message));
    BOOST_CHECK(error_message.empty());
    
    // Test duplicate registration
    BOOST_CHECK(!consensus.RegisterUser(test_user, error_message));
    BOOST_CHECK(!error_message.empty());
}

BOOST_AUTO_TEST_CASE(test_user_reputation_system) {
    // Test reputation system
    UserRegistryConsensus consensus;
    
    CKey key;
    key.MakeNewKey(true);
    CPubKey pubkey = key.GetPubKey();
    
    OfficialUser user;
    user.public_key = pubkey;
    user.government_id_hash = "reputation_test_government_id";
    user.birth_currency = "USD";
    user.country_code = "US";
    user.reputation_score = 5.0;
    
    std::string error_message;
    BOOST_CHECK(consensus.RegisterUser(user, error_message));
    
    // Test initial reputation
    BOOST_CHECK_EQUAL(consensus.GetReputationScore(pubkey), 5.0);
    
    // Test reputation updates
    consensus.UpdateReputationScore(pubkey, 1.0);
    BOOST_CHECK_EQUAL(consensus.GetReputationScore(pubkey), 6.0);
    
    consensus.UpdateReputationScore(pubkey, -2.0);
    BOOST_CHECK_EQUAL(consensus.GetReputationScore(pubkey), 4.0);
}

BOOST_AUTO_TEST_CASE(test_user_statistics) {
    // Test user statistics
    UserRegistryConsensus consensus;
    
    // Create users with different statuses
    std::vector<UserStatus> statuses = {
        UserStatus::PENDING_VERIFICATION,
        UserStatus::VERIFICATION_IN_PROGRESS,
        UserStatus::VERIFIED,
        UserStatus::VERIFIED,
        UserStatus::SUSPENDED,
        UserStatus::BLACKLISTED
    };
    
    for (size_t i = 0; i < statuses.size(); ++i) {
        CKey key;
        key.MakeNewKey(true);
        CPubKey pubkey = key.GetPubKey();
        
        OfficialUser user;
        user.public_key = pubkey;
        user.government_id_hash = "stats_test_id_" + std::to_string(i);
        user.birth_currency = "USD";
        user.country_code = "US";
        user.status = statuses[i];
        user.reputation_score = static_cast<double>(i + 1);
        
        std::string error_message;
        BOOST_CHECK(consensus.RegisterUser(user, error_message));
    }
    
    // Test statistics
    UserRegistryConsensus::UserStats stats = consensus.GetUserStatistics();
    
    BOOST_CHECK_EQUAL(stats.total_registered, 6);
    BOOST_CHECK_EQUAL(stats.total_verified, 2);
    BOOST_CHECK_EQUAL(stats.total_pending, 2);
    BOOST_CHECK_EQUAL(stats.total_suspended, 1);
    BOOST_CHECK_EQUAL(stats.total_blacklisted, 1);
}

BOOST_AUTO_TEST_CASE(test_government_id_uniqueness) {
    // Test government ID uniqueness enforcement
    UserRegistryConsensus consensus;
    
    std::string government_id = "unique_government_id_123";
    std::string government_id_hash = Hash(government_id).GetHex();
    
    // Create first user
    CKey key1;
    key1.MakeNewKey(true);
    CPubKey pubkey1 = key1.GetPubKey();
    
    OfficialUser user1;
    user1.public_key = pubkey1;
    user1.government_id_hash = government_id_hash;
    user1.birth_currency = "USD";
    user1.country_code = "US";
    
    std::string error_message;
    BOOST_CHECK(consensus.RegisterUser(user1, error_message));
    
    // Test uniqueness check
    BOOST_CHECK(!consensus.IsUserUnique(government_id_hash));
    
    // Try to create second user with same government ID
    CKey key2;
    key2.MakeNewKey(true);
    CPubKey pubkey2 = key2.GetPubKey();
    
    OfficialUser user2;
    user2.public_key = pubkey2;
    user2.government_id_hash = government_id_hash; // Same government ID
    user2.birth_currency = "USD";
    user2.country_code = "US";
    
    BOOST_CHECK(!consensus.RegisterUser(user2, error_message));
    BOOST_CHECK(error_message.find("Government ID") != std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()


