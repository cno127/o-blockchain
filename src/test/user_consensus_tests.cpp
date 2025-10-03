// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/user_consensus_tests.h>
#include <consensus/user_consensus.h>
#include <key.h>
#include <pubkey.h>
#include <util/strencodings.h>
#include <test/util/setup_common.h>
#include <hash.h>
#include <util/time.h>

#include <boost/test/unit_test.hpp>
#include <string>
#include <vector>

BOOST_FIXTURE_TEST_SUITE(user_consensus_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(test_user_registration) {
    // Test user registration functionality
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
    
    // Test duplicate government ID
    OfficialUser duplicate_user = test_user;
    CKey key2;
    key2.MakeNewKey(true);
    duplicate_user.public_key = key2.GetPubKey();
    
    BOOST_CHECK(!consensus.RegisterUser(duplicate_user, error_message));
    BOOST_CHECK(error_message.find("Government ID") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_endorsement_system) {
    // Test endorsement system
    UserRegistryConsensus consensus;
    
    // Create endorser (verified user)
    CKey endorser_key;
    endorser_key.MakeNewKey(true);
    CPubKey endorser_pubkey = endorser_key.GetPubKey();
    
    OfficialUser endorser;
    endorser.public_key = endorser_pubkey;
    endorser.government_id_hash = "endorser_government_id_hash";
    endorser.birth_currency = "EUR";
    endorser.country_code = "DE";
    endorser.status = UserStatus::VERIFIED;
    endorser.reputation_score = 5.0;
    
    std::string error_message;
    BOOST_CHECK(consensus.RegisterUser(endorser, error_message));
    
    // Create user to be endorsed
    CKey user_key;
    user_key.MakeNewKey(true);
    CPubKey user_pubkey = user_key.GetPubKey();
    
    OfficialUser user_to_endorse;
    user_to_endorse.public_key = user_pubkey;
    user_to_endorse.government_id_hash = "user_government_id_hash";
    user_to_endorse.birth_currency = "USD";
    user_to_endorse.country_code = "US";
    user_to_endorse.status = UserStatus::PENDING_VERIFICATION;
    
    BOOST_CHECK(consensus.RegisterUser(user_to_endorse, error_message));
    
    // Create endorsement
    EndorsementRecord endorsement;
    std::string data_str(endorser_pubkey.data(), endorser_pubkey.data() + endorser_pubkey.size());
    data_str.append(user_pubkey.data(), user_pubkey.data() + user_pubkey.size());
    endorsement.endorsement_id = Hash(data_str);
    endorsement.endorser = endorser_pubkey;
    endorsement.endorsed_user = user_pubkey;
    endorsement.verification_method = VerificationMethod::GOVERNMENT_ID;
    endorsement.confidence_level = ConfidenceLevel::HIGH;
    endorsement.timestamp = GetTime();
    endorsement.block_height = 100;
    
    // Test successful endorsement
    BOOST_CHECK(consensus.SubmitEndorsement(endorsement, error_message));
    BOOST_CHECK(error_message.empty());
    
    // Test duplicate endorsement
    BOOST_CHECK(!consensus.SubmitEndorsement(endorsement, error_message));
    BOOST_CHECK(!error_message.empty());
}

BOOST_AUTO_TEST_CASE(test_user_verification_status) {
    // Test user verification status checking
    UserRegistryConsensus consensus;
    
    // Create test users with different statuses
    std::vector<UserStatus> statuses = {
        UserStatus::PENDING_VERIFICATION,
        UserStatus::VERIFICATION_IN_PROGRESS,
        UserStatus::VERIFIED,
        UserStatus::SUSPENDED,
        UserStatus::BLACKLISTED
    };
    
    std::vector<CPubKey> test_users;
    
    for (size_t i = 0; i < statuses.size(); ++i) {
        CKey key;
        key.MakeNewKey(true);
        CPubKey pubkey = key.GetPubKey();
        test_users.push_back(pubkey);
        
        OfficialUser user;
        user.public_key = pubkey;
        user.government_id_hash = "test_government_id_" + std::to_string(i);
        user.birth_currency = "USD";
        user.country_code = "US";
        user.status = statuses[i];
        
        std::string error_message;
        BOOST_CHECK(consensus.RegisterUser(user, error_message));
    }
    
    // Test verification status checking
    BOOST_CHECK(!consensus.IsUserVerified(test_users[0])); // PENDING_VERIFICATION
    BOOST_CHECK(!consensus.IsUserVerified(test_users[1])); // VERIFICATION_IN_PROGRESS
    BOOST_CHECK(consensus.IsUserVerified(test_users[2]));  // VERIFIED
    BOOST_CHECK(!consensus.IsUserVerified(test_users[3])); // SUSPENDED
    BOOST_CHECK(!consensus.IsUserVerified(test_users[4])); // BLACKLISTED
}

BOOST_AUTO_TEST_CASE(test_reputation_system) {
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
    
    // Test reputation bounds (should be clamped between 0 and 10)
    consensus.UpdateReputationScore(pubkey, -10.0);
    BOOST_CHECK_EQUAL(consensus.GetReputationScore(pubkey), 0.0);
    
    consensus.UpdateReputationScore(pubkey, 15.0);
    BOOST_CHECK_EQUAL(consensus.GetReputationScore(pubkey), 10.0);
}

BOOST_AUTO_TEST_CASE(test_challenge_system) {
    // Test endorsement challenge system
    UserRegistryConsensus consensus;
    
    // Create endorser and user
    CKey endorser_key, user_key;
    endorser_key.MakeNewKey(true);
    user_key.MakeNewKey(true);
    CPubKey endorser_pubkey = endorser_key.GetPubKey();
    CPubKey user_pubkey = user_key.GetPubKey();
    
    // Register users
    OfficialUser endorser;
    endorser.public_key = endorser_pubkey;
    endorser.government_id_hash = "challenge_endorser_id";
    endorser.birth_currency = "USD";
    endorser.status = UserStatus::VERIFIED;
    endorser.reputation_score = 5.0;
    
    OfficialUser user;
    user.public_key = user_pubkey;
    user.government_id_hash = "challenge_user_id";
    user.birth_currency = "USD";
    user.status = UserStatus::PENDING_VERIFICATION;
    
    std::string error_message;
    BOOST_CHECK(consensus.RegisterUser(endorser, error_message));
    BOOST_CHECK(consensus.RegisterUser(user, error_message));
    
    // Create endorsement
    EndorsementRecord endorsement;
    std::string data_str(endorser_pubkey.data(), endorser_pubkey.data() + endorser_pubkey.size());
    data_str.append(user_pubkey.data(), user_pubkey.data() + user_pubkey.size());
    endorsement.endorsement_id = Hash(data_str);
    endorsement.endorser = endorser_pubkey;
    endorsement.endorsed_user = user_pubkey;
    endorsement.verification_method = VerificationMethod::GOVERNMENT_ID;
    endorsement.confidence_level = ConfidenceLevel::HIGH;
    endorsement.timestamp = GetTime();
    
    BOOST_CHECK(consensus.SubmitEndorsement(endorsement, error_message));
    
    // Create challenger
    CKey challenger_key;
    challenger_key.MakeNewKey(true);
    CPubKey challenger_pubkey = challenger_key.GetPubKey();
    
    OfficialUser challenger;
    challenger.public_key = challenger_pubkey;
    challenger.government_id_hash = "challenge_challenger_id";
    challenger.birth_currency = "USD";
    challenger.status = UserStatus::VERIFIED;
    challenger.reputation_score = 7.0;
    
    BOOST_CHECK(consensus.RegisterUser(challenger, error_message));
    
    // Create challenge
    ChallengeRecord challenge;
    std::string challenge_data(challenger_pubkey.data(), challenger_pubkey.data() + challenger_pubkey.size());
    challenge_data += std::to_string(GetTime());
    challenge.challenge_id = Hash(challenge_data);
    challenge.endorsement_id = endorsement.endorsement_id;
    challenge.challenger = challenger_pubkey;
    challenge.challenge_reason = "Suspicious verification method";
    challenge.timestamp = GetTime();
    
    // Test successful challenge
    BOOST_CHECK(consensus.ChallengeEndorsement(challenge, error_message));
    BOOST_CHECK(error_message.empty());
    
    // Test duplicate challenge
    BOOST_CHECK(!consensus.ChallengeEndorsement(challenge, error_message));
    BOOST_CHECK(!error_message.empty());
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
    
    // Test average reputation calculation
    double expected_avg = (1.0 + 2.0 + 3.0 + 4.0 + 5.0 + 6.0) / 6.0;
    BOOST_CHECK_CLOSE(stats.average_reputation, expected_avg, 0.001);
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

BOOST_AUTO_TEST_CASE(test_geographic_diversity) {
    // Test geographic diversity requirements
    UserRegistryConsensus consensus;
    
    // Create user from US
    CKey user_key;
    user_key.MakeNewKey(true);
    CPubKey user_pubkey = user_key.GetPubKey();
    
    OfficialUser user;
    user.public_key = user_pubkey;
    user.government_id_hash = "geo_diversity_user_id";
    user.birth_currency = "USD";
    user.country_code = "US";
    user.status = UserStatus::PENDING_VERIFICATION;
    
    std::string error_message;
    BOOST_CHECK(consensus.RegisterUser(user, error_message));
    
    // Create endorsers from different countries
    std::vector<std::string> countries = {"US", "DE", "JP", "FR", "GB"};
    std::vector<CPubKey> endorsers;
    
    for (size_t i = 0; i < countries.size(); ++i) {
        CKey key;
        key.MakeNewKey(true);
        CPubKey pubkey = key.GetPubKey();
        endorsers.push_back(pubkey);
        
        OfficialUser endorser;
        endorser.public_key = pubkey;
        endorser.government_id_hash = "geo_endorser_" + std::to_string(i);
        endorser.birth_currency = countries[i] == "US" ? "USD" : "EUR";
        endorser.country_code = countries[i];
        endorser.status = UserStatus::VERIFIED;
        endorser.reputation_score = 5.0;
        
        BOOST_CHECK(consensus.RegisterUser(endorser, error_message));
        
        // Create endorsement
        EndorsementRecord endorsement;
        std::string endorsement_data(pubkey.data(), pubkey.data() + pubkey.size());
        endorsement_data.append(user_pubkey.data(), user_pubkey.data() + user_pubkey.size());
        endorsement_data += std::to_string(i);
        endorsement.endorsement_id = Hash(endorsement_data);
        endorsement.endorser = pubkey;
        endorsement.endorsed_user = user_pubkey;
        endorsement.verification_method = VerificationMethod::GOVERNMENT_ID;
        endorsement.confidence_level = ConfidenceLevel::HIGH;
        endorsement.timestamp = GetTime();
        
        BOOST_CHECK(consensus.SubmitEndorsement(endorsement, error_message));
    }
    
    // Test geographic diversity validation
    BOOST_CHECK(consensus.ValidateGeoDiversity(user_pubkey));
    
    // Test with insufficient diversity
    // Remove some endorsements to test minimum threshold
    // This would require additional methods in the consensus system
}

BOOST_AUTO_TEST_SUITE_END()
