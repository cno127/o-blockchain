// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/o_rpc_tests.h>
#include <rpc/o_user_rpc.h>
#include <validation/o_integration.h>
#include <consensus/user_consensus.h>
#include <rpc/server.h>
#include <test/util/setup_common.h>
#include <key.h>
#include <pubkey.h>
#include <hash.h>

#include <boost/test/unit_test.hpp>
#include <univalue.h>

BOOST_FIXTURE_TEST_SUITE(o_rpc_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(test_registeruser_rpc) {
    // Test registeruser RPC method
    UserRegistryConsensus user_consensus;
    OBlockchainIntegration integration(nullptr, &user_consensus);
    g_o_integration = &integration;
    
    // Create test public key
    CKey key;
    key.MakeNewKey(true);
    CPubKey pubkey = key.GetPubKey();
    std::string pubkey_hex = HexStr(pubkey);
    
    // Test parameters
    UniValue params(UniValue::VARR);
    params.push_back(pubkey_hex);
    params.push_back("123456789");
    params.push_back("USD");
    params.push_back("US");
    params.push_back("abcd1234efgh5678");
    
    // Create mock RPC request
    JSONRPCRequest request;
    request.params = params;
    
    // Test successful registration
    try {
        UniValue result = ::registeruser(request);
        BOOST_CHECK(result.isObject());
        BOOST_CHECK(result["user_id"].get_str() == pubkey_hex);
        BOOST_CHECK(result["status"].get_str() == "pending_verification");
        BOOST_CHECK(result["message"].get_str().find("submitted successfully") != std::string::npos);
    } catch (const std::exception& e) {
        BOOST_FAIL("registeruser RPC failed: " + std::string(e.what()));
    }
    
    // Test duplicate registration
    try {
        UniValue result = ::registeruser(request);
        BOOST_FAIL("Duplicate registration should have failed");
    } catch (const std::exception& e) {
        BOOST_CHECK(std::string(e.what()).find("User already registered") != std::string::npos);
    }
    
    g_o_integration = nullptr;
}

BOOST_AUTO_TEST_CASE(test_submitendorsement_rpc) {
    // Test submitendorsement RPC method
    UserRegistryConsensus user_consensus;
    OBlockchainIntegration integration(nullptr, &user_consensus);
    g_o_integration = &integration;
    
    // Create endorser (verified user)
    CKey endorser_key;
    endorser_key.MakeNewKey(true);
    CPubKey endorser_pubkey = endorser_key.GetPubKey();
    std::string endorser_hex = endorser_pubkey.GetHex();
    
    OfficialUser endorser;
    endorser.public_key = endorser_pubkey;
    endorser.government_id_hash = "endorser_government_id";
    endorser.birth_currency = "USD";
    endorser.country_code = "US";
    endorser.status = UserStatus::VERIFIED;
    endorser.reputation_score = 5.0;
    
    std::string error_message;
    BOOST_CHECK(user_consensus.RegisterUser(endorser, error_message));
    
    // Create user to be endorsed
    CKey user_key;
    user_key.MakeNewKey(true);
    CPubKey user_pubkey = user_key.GetPubKey();
    std::string user_hex = user_pubkey.GetHex();
    
    OfficialUser user;
    user.public_key = user_pubkey;
    user.government_id_hash = "user_government_id";
    user.birth_currency = "USD";
    user.country_code = "US";
    user.status = UserStatus::PENDING_VERIFICATION;
    
    BOOST_CHECK(user_consensus.RegisterUser(user, error_message));
    
    // Test parameters
    UniValue params(UniValue::VARR);
    params.push_back(endorser_hex);
    params.push_back(user_hex);
    params.push_back("government_id");
    params.push_back("high");
    params.push_back("Verified government ID document");
    
    // Create mock RPC request
    JSONRPCRequest request;
    request.params = params;
    
    // Test successful endorsement
    try {
        UniValue result = ::submitendorsement(request);
        BOOST_CHECK(result.isObject());
        BOOST_CHECK(result["status"].get_str() == "submitted");
        BOOST_CHECK(result["message"].get_str().find("submitted successfully") != std::string::npos);
        BOOST_CHECK(result["endorsement_id"].isStr());
    } catch (const std::exception& e) {
        BOOST_FAIL("submitendorsement RPC failed: " + std::string(e.what()));
    }
    
    g_o_integration = nullptr;
}

BOOST_AUTO_TEST_CASE(test_getuserstatus_rpc) {
    // Test getuserstatus RPC method
    UserRegistryConsensus user_consensus;
    OBlockchainIntegration integration(nullptr, &user_consensus);
    g_o_integration = &integration;
    
    // Create verified user
    CKey key;
    key.MakeNewKey(true);
    CPubKey pubkey = key.GetPubKey();
    std::string pubkey_hex = HexStr(pubkey);
    
    OfficialUser user;
    user.public_key = pubkey;
    user.government_id_hash = "status_test_user_id";
    user.birth_currency = "USD";
    user.country_code = "US";
    user.status = UserStatus::VERIFIED;
    user.reputation_score = 7.5;
    
    std::string error_message;
    BOOST_CHECK(user_consensus.RegisterUser(user, error_message));
    
    // Test parameters
    UniValue params(UniValue::VARR);
    params.push_back(pubkey_hex);
    
    // Create mock RPC request
    JSONRPCRequest request;
    request.params = params;
    
    // Test successful status query
    try {
        UniValue result = ::getuserstatus(request);
        BOOST_CHECK(result.isObject());
        BOOST_CHECK(result["user_id"].get_str() == pubkey_hex);
        BOOST_CHECK(result["status"].get_str() == "verified");
        BOOST_CHECK(result["reputation_score"].isNum());
    } catch (const std::exception& e) {
        BOOST_FAIL("getuserstatus RPC failed: " + std::string(e.what()));
    }
    
    g_o_integration = nullptr;
}

BOOST_AUTO_TEST_CASE(test_listverifiedusers_rpc) {
    // Test listverifiedusers RPC method
    UserRegistryConsensus user_consensus;
    OBlockchainIntegration integration(nullptr, &user_consensus);
    g_o_integration = &integration;
    
    // Create multiple verified users
    std::vector<CPubKey> test_users;
    for (int i = 0; i < 5; ++i) {
        CKey key;
        key.MakeNewKey(true);
        CPubKey pubkey = key.GetPubKey();
        test_users.push_back(pubkey);
        
        OfficialUser user;
        user.public_key = pubkey;
        user.government_id_hash = "list_test_user_" + std::to_string(i);
        user.birth_currency = "USD";
        user.country_code = "US";
        user.status = UserStatus::VERIFIED;
        user.reputation_score = static_cast<double>(i + 1);
        
        std::string error_message;
        BOOST_CHECK(user_consensus.RegisterUser(user, error_message));
    }
    
    // Test parameters
    UniValue params(UniValue::VARR);
    params.push_back(3); // limit
    params.push_back(0); // offset
    
    // Create mock RPC request
    JSONRPCRequest request;
    request.params = params;
    
    // Test successful user listing
    try {
        UniValue result = ::listverifiedusers(request);
        BOOST_CHECK(result.isObject());
        BOOST_CHECK(result["total_count"].getInt<int>() == 5);
        BOOST_CHECK(result["returned_count"].getInt<int>() == 3);
        
        UniValue users = result["users"];
        BOOST_CHECK(users.isArray());
        BOOST_CHECK(users.size() == 3);
        
        for (size_t i = 0; i < users.size(); ++i) {
            UniValue user_obj = users[i];
            BOOST_CHECK(user_obj.isObject());
            BOOST_CHECK(user_obj["public_key"].isStr());
            BOOST_CHECK(user_obj["reputation_score"].isNum());
        }
    } catch (const std::exception& e) {
        BOOST_FAIL("listverifiedusers RPC failed: " + std::string(e.what()));
    }
    
    g_o_integration = nullptr;
}

BOOST_AUTO_TEST_CASE(test_getuserstats_rpc) {
    // Test getuserstats RPC method
    UserRegistryConsensus user_consensus;
    OBlockchainIntegration integration(nullptr, &user_consensus);
    g_o_integration = &integration;
    
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
        user.government_id_hash = "stats_test_user_" + std::to_string(i);
        user.birth_currency = "USD";
        user.country_code = "US";
        user.status = statuses[i];
        user.reputation_score = static_cast<double>(i + 1);
        
        std::string error_message;
        BOOST_CHECK(user_consensus.RegisterUser(user, error_message));
    }
    
    // Test parameters (empty)
    UniValue params(UniValue::VARR);
    
    // Create mock RPC request
    JSONRPCRequest request;
    request.params = params;
    
    // Test successful stats query
    try {
        UniValue result = ::getuserstats(request);
        BOOST_CHECK(result.isObject());
        BOOST_CHECK(result["total_registered"].getInt<int>() == 6);
        BOOST_CHECK(result["total_verified"].getInt<int>() == 2);
        BOOST_CHECK(result["total_pending"].getInt<int>() == 2);
        BOOST_CHECK(result["total_suspended"].getInt<int>() == 1);
        BOOST_CHECK(result["total_blacklisted"].getInt<int>() == 1);
        BOOST_CHECK(result["average_reputation"].isNum());
        BOOST_CHECK(result["active_challenges"].isNum());
    } catch (const std::exception& e) {
        BOOST_FAIL("getuserstats RPC failed: " + std::string(e.what()));
    }
    
    g_o_integration = nullptr;
}

BOOST_AUTO_TEST_CASE(test_invalid_parameters) {
    // Test RPC methods with invalid parameters
    UserRegistryConsensus user_consensus;
    OBlockchainIntegration integration(nullptr, &user_consensus);
    g_o_integration = &integration;
    
    // Test registeruser with invalid public key
    UniValue params(UniValue::VARR);
    params.push_back("invalid_public_key");
    params.push_back("123456789");
    params.push_back("USD");
    params.push_back("US");
    params.push_back("abcd1234efgh5678");
    
    JSONRPCRequest request;
    request.params = params;
    
    try {
        UniValue result = ::registeruser(request);
        BOOST_FAIL("Invalid public key should have caused error");
    } catch (const std::exception& e) {
        BOOST_CHECK(std::string(e.what()).find("Invalid public key") != std::string::npos);
    }
    
    // Test submitendorsement with invalid verification method
    CKey key;
    key.MakeNewKey(true);
    CPubKey pubkey = key.GetPubKey();
    std::string pubkey_hex = HexStr(pubkey);
    
    UniValue endorsement_params(UniValue::VARR);
    endorsement_params.push_back(pubkey_hex);
    endorsement_params.push_back(pubkey_hex);
    endorsement_params.push_back("invalid_method");
    endorsement_params.push_back("high");
    
    JSONRPCRequest endorsement_request;
    endorsement_request.params = endorsement_params;
    
    try {
        UniValue result = submitendorsement().call(endorsement_request);
        BOOST_FAIL("Invalid verification method should have caused error");
    } catch (const std::exception& e) {
        BOOST_CHECK(std::string(e.what()).find("Invalid verification method") != std::string::npos);
    }
    
    g_o_integration = nullptr;
}

BOOST_AUTO_TEST_CASE(test_integration_not_available) {
    // Test RPC methods when integration is not available
    g_o_integration = nullptr;
    
    // Test registeruser without integration
    UniValue params(UniValue::VARR);
    params.push_back("02f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f9");
    params.push_back("123456789");
    params.push_back("USD");
    params.push_back("US");
    params.push_back("abcd1234efgh5678");
    
    JSONRPCRequest request;
    request.params = params;
    
    try {
        UniValue result = ::registeruser(request);
        BOOST_FAIL("Should have failed when integration not available");
    } catch (const std::exception& e) {
        BOOST_CHECK(std::string(e.what()).find("O blockchain integration not available") != std::string::npos);
    }
}

BOOST_AUTO_TEST_SUITE_END()
