// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/o_integration_tests.h>
#include <validation/o_integration.h>
#include <validation/o_block_validation.h>
#include <consensus/user_consensus.h>
#include <primitives/multicurrency_txout.h>
#include <primitives/transaction.h>
#include <key.h>
#include <pubkey.h>
#include <script/script.h>
#include <test/util/setup_common.h>
#include <hash.h>

#include <boost/test/unit_test.hpp>
#include <memory>

BOOST_FIXTURE_TEST_SUITE(o_integration_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(test_transaction_validation_with_user_check) {
    // Test transaction validation with user verification
    UserRegistryConsensus user_consensus;
    OBlockchainIntegration integration(nullptr, &user_consensus);
    
    // Create test transaction
    CMultiCurrencyTransaction tx;
    tx.nVersion = 1;
    tx.nLockTime = 0;
    
    // Create test user
    CKey user_key;
    user_key.MakeNewKey(true);
    CPubKey user_pubkey = user_key.GetPubKey();
    
    // Create transaction input with user's signature
    CMultiCurrencyTxIn txin;
    txin.prevout = COutPoint(Txid(uint256::ZERO), 0);
    txin.scriptSig = CScript() << ToByteVector(user_pubkey) << std::vector<unsigned char>(72, 0); // Mock signature
    txin.nSequence = 0xffffffff;
    
    // Create transaction output
    CMultiCurrencyTxOut txout;
    txout.scriptPubKey = CScript() << OP_DUP << OP_HASH160 << ToByteVector(uint160()) << OP_EQUALVERIFY << OP_CHECKSIG;
    std::vector<MultiCurrencyAmount> amounts;
    amounts.emplace_back(CURRENCY_BTC, 1000000); // 0.01 BTC
    txout.amounts = amounts;
    
    tx.vin.push_back(txin);
    tx.vout.push_back(txout);
    
    // Test with unverified user
    TxValidationState state;
    CCoinsViewCache coins_view;
    
    BOOST_CHECK(!integration.ValidateTransactionWithUserCheck(tx, state, coins_view, 100));
    BOOST_CHECK(state.IsInvalid());
    
    // Register user as verified
    OfficialUser user;
    user.public_key = user_pubkey;
    user.government_id_hash = "test_government_id_hash";
    user.birth_currency = "USD";
    user.country_code = "US";
    user.status = UserStatus::VERIFIED;
    
    std::string error_message;
    BOOST_CHECK(user_consensus.RegisterUser(user, error_message));
    
    // Test with verified user
    TxValidationState state2;
    BOOST_CHECK(integration.ValidateTransactionWithUserCheck(tx, state2, coins_view, 100));
    BOOST_CHECK(state2.IsValid());
}

BOOST_AUTO_TEST_CASE(test_user_registration_transaction_processing) {
    // Test user registration transaction processing
    UserRegistryConsensus user_consensus;
    OBlockchainIntegration integration(nullptr, &user_consensus);
    
    // Create user registration transaction
    CMultiCurrencyTransaction tx;
    tx.nVersion = 1;
    tx.nLockTime = 0;
    
    CKey user_key;
    user_key.MakeNewKey(true);
    CPubKey user_pubkey = user_key.GetPubKey();
    
    // Create registration transaction input
    CMultiCurrencyTxIn txin;
    txin.prevout = COutPoint(Txid(uint256::ZERO), 0);
    txin.scriptSig = CScript() << ToByteVector(user_pubkey) << std::vector<unsigned char>(72, 0);
    txin.nSequence = 0xffffffff;
    
    // Create registration transaction output (would contain user data)
    CMultiCurrencyTxOut txout;
    txout.scriptPubKey = CScript() << OP_RETURN << ToByteVector(user_pubkey);
    std::vector<MultiCurrencyAmount> amounts;
    amounts.emplace_back(CURRENCY_BTC, 0); // No monetary value for registration
    txout.amounts = amounts;
    
    tx.vin.push_back(txin);
    tx.vout.push_back(txout);
    
    // Test user registration processing
    TxValidationState state;
    BOOST_CHECK(integration.ProcessUserRegistration(tx, state, 100));
    BOOST_CHECK(state.IsValid());
}

BOOST_AUTO_TEST_CASE(test_endorsement_transaction_processing) {
    // Test endorsement transaction processing
    UserRegistryConsensus user_consensus;
    OBlockchainIntegration integration(nullptr, &user_consensus);
    
    // Create endorser (verified user)
    CKey endorser_key;
    endorser_key.MakeNewKey(true);
    CPubKey endorser_pubkey = endorser_key.GetPubKey();
    
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
    
    OfficialUser user;
    user.public_key = user_pubkey;
    user.government_id_hash = "user_government_id";
    user.birth_currency = "USD";
    user.country_code = "US";
    user.status = UserStatus::PENDING_VERIFICATION;
    
    BOOST_CHECK(user_consensus.RegisterUser(user, error_message));
    
    // Create endorsement transaction
    CMultiCurrencyTransaction tx;
    tx.nVersion = 1;
    tx.nLockTime = 0;
    
    // Create endorsement transaction input
    CMultiCurrencyTxIn txin;
    txin.prevout = COutPoint(Txid(uint256::ZERO), 0);
    txin.scriptSig = CScript() << ToByteVector(endorser_pubkey) << std::vector<unsigned char>(72, 0);
    txin.nSequence = 0xffffffff;
    
    // Create endorsement transaction output (would contain endorsement data)
    CMultiCurrencyTxOut txout;
    txout.scriptPubKey = CScript() << OP_RETURN << ToByteVector(endorser_pubkey) << ToByteVector(user_pubkey);
    std::vector<MultiCurrencyAmount> amounts;
    amounts.emplace_back(CURRENCY_BTC, 0);
    txout.amounts = amounts;
    
    tx.vin.push_back(txin);
    tx.vout.push_back(txout);
    
    // Test endorsement processing
    TxValidationState state;
    BOOST_CHECK(integration.ProcessEndorsement(tx, state, 100));
    BOOST_CHECK(state.IsValid());
}

BOOST_AUTO_TEST_CASE(test_measurement_transaction_processing) {
    // Test measurement transaction processing
    UserRegistryConsensus user_consensus;
    OBlockchainIntegration integration(nullptr, &user_consensus);
    
    // Create verified user for measurement
    CKey user_key;
    user_key.MakeNewKey(true);
    CPubKey user_pubkey = user_key.GetPubKey();
    
    OfficialUser user;
    user.public_key = user_pubkey;
    user.government_id_hash = "measurement_user_id";
    user.birth_currency = "USD";
    user.country_code = "US";
    user.status = UserStatus::VERIFIED;
    
    std::string error_message;
    BOOST_CHECK(user_consensus.RegisterUser(user, error_message));
    
    // Create measurement transaction
    CMultiCurrencyTransaction tx;
    tx.nVersion = 1;
    tx.nLockTime = 0;
    
    // Create measurement transaction input
    CMultiCurrencyTxIn txin;
    txin.prevout = COutPoint(Txid(uint256::ZERO), 0);
    txin.scriptSig = CScript() << ToByteVector(user_pubkey) << std::vector<unsigned char>(72, 0);
    txin.nSequence = 0xffffffff;
    
    // Create measurement transaction output (would contain measurement data)
    CMultiCurrencyTxOut txout;
    txout.scriptPubKey = CScript() << OP_RETURN << ToByteVector(user_pubkey);
    std::vector<MultiCurrencyAmount> amounts;
    amounts.emplace_back(CURRENCY_BTC, 0);
    txout.amounts = amounts;
    
    tx.vin.push_back(txin);
    tx.vout.push_back(txout);
    
    // Test measurement processing
    TxValidationState state;
    BOOST_CHECK(integration.ProcessMeasurement(tx, state, 100));
    BOOST_CHECK(state.IsValid());
}

BOOST_AUTO_TEST_CASE(test_reward_calculation_with_user_verification) {
    // Test reward calculation with user verification
    UserRegistryConsensus user_consensus;
    OBlockchainIntegration integration(nullptr, &user_consensus);
    
    CAmount base_reward = 1000;
    
    // Test with unverified user
    CKey unverified_key;
    unverified_key.MakeNewKey(true);
    CPubKey unverified_pubkey = unverified_key.GetPubKey();
    
    CMultiCurrencyTransaction unverified_tx;
    unverified_tx.nVersion = 1;
    unverified_tx.nLockTime = 0;
    
    CMultiCurrencyTxIn txin;
    txin.prevout = COutPoint(Txid(uint256::ZERO), 0);
    txin.scriptSig = CScript() << ToByteVector(unverified_pubkey) << std::vector<unsigned char>(72, 0);
    txin.nSequence = 0xffffffff;
    
    CMultiCurrencyTxOut txout;
    txout.scriptPubKey = CScript() << OP_DUP << OP_HASH160 << ToByteVector(uint160()) << OP_EQUALVERIFY << OP_CHECKSIG;
    std::vector<MultiCurrencyAmount> amounts;
    amounts.emplace_back(CURRENCY_BTC, 1000000);
    txout.amounts = amounts;
    
    unverified_tx.vin.push_back(txin);
    unverified_tx.vout.push_back(txout);
    
    CAmount unverified_reward = integration.CalculateRewardWithUserVerification(unverified_tx, base_reward);
    BOOST_CHECK_EQUAL(unverified_reward, static_cast<CAmount>(base_reward * 0.5)); // 50% penalty
    
    // Test with verified user
    CKey verified_key;
    verified_key.MakeNewKey(true);
    CPubKey verified_pubkey = verified_key.GetPubKey();
    
    OfficialUser verified_user;
    verified_user.public_key = verified_pubkey;
    verified_user.government_id_hash = "verified_user_id";
    verified_user.birth_currency = "USD";
    verified_user.country_code = "US";
    verified_user.status = UserStatus::VERIFIED;
    
    std::string error_message;
    BOOST_CHECK(user_consensus.RegisterUser(verified_user, error_message));
    
    CMultiCurrencyTransaction verified_tx;
    verified_tx.nVersion = 1;
    verified_tx.nLockTime = 0;
    
    CMultiCurrencyTxIn verified_txin;
    verified_txin.prevout = COutPoint(uint256::ZERO, 0);
    verified_txin.scriptSig = CScript() << ToByteVector(verified_pubkey) << std::vector<unsigned char>(72, 0);
    verified_txin.nSequence = 0xffffffff;
    
    CMultiCurrencyTxOut verified_txout;
    verified_txout.scriptPubKey = CScript() << OP_DUP << OP_HASH160 << ToByteVector(uint160()) << OP_EQUALVERIFY << OP_CHECKSIG;
    std::vector<MultiCurrencyAmount> verified_amounts;
    verified_amounts.emplace_back(CURRENCY_BTC, 1000000);
    verified_txout.amounts = verified_amounts;
    
    verified_tx.vin.push_back(verified_txin);
    verified_tx.vout.push_back(verified_txout);
    
    CAmount verified_reward = integration.CalculateRewardWithUserVerification(verified_tx, base_reward);
    BOOST_CHECK_EQUAL(verified_reward, base_reward); // Full reward for verified user
}

BOOST_AUTO_TEST_CASE(test_measurement_reward_calculation) {
    // Test measurement reward calculation
    UserRegistryConsensus user_consensus;
    OBlockchainIntegration integration(nullptr, &user_consensus);
    
    CKey user_key;
    user_key.MakeNewKey(true);
    CPubKey user_pubkey = user_key.GetPubKey();
    
    // Test with unverified user
    CAmount unverified_reward = integration.CalculateMeasurementReward(user_pubkey, 0.9, 1000);
    BOOST_CHECK_EQUAL(unverified_reward, 0); // No reward for unverified user
    
    // Register user as verified
    OfficialUser user;
    user.public_key = user_pubkey;
    user.government_id_hash = "measurement_reward_user_id";
    user.birth_currency = "USD";
    user.country_code = "US";
    user.status = UserStatus::VERIFIED;
    
    std::string error_message;
    BOOST_CHECK(user_consensus.RegisterUser(user, error_message));
    
    // Test with verified user
    CAmount base_reward = 1000;
    double measurement_quality = 0.9;
    CAmount expected_reward = static_cast<CAmount>(base_reward * measurement_quality * 1.0); // 1.0 multiplier
    
    CAmount verified_reward = integration.CalculateMeasurementReward(user_pubkey, measurement_quality, base_reward);
    BOOST_CHECK_EQUAL(verified_reward, expected_reward);
}

BOOST_AUTO_TEST_CASE(test_user_verification_for_transaction_type) {
    // Test user verification for different transaction types
    UserRegistryConsensus user_consensus;
    OBlockchainIntegration integration(nullptr, &user_consensus);
    
    // Create pending user
    CKey pending_key;
    pending_key.MakeNewKey(true);
    CPubKey pending_pubkey = pending_key.GetPubKey();
    
    OfficialUser pending_user;
    pending_user.public_key = pending_pubkey;
    pending_user.government_id_hash = "pending_user_id";
    pending_user.birth_currency = "USD";
    pending_user.country_code = "US";
    pending_user.status = UserStatus::PENDING_VERIFICATION;
    
    std::string error_message;
    BOOST_CHECK(user_consensus.RegisterUser(pending_user, error_message));
    
    // Test transaction type verification
    BOOST_CHECK(!integration.IsUserVerifiedForTransaction(pending_pubkey, "measurement"));
    BOOST_CHECK(!integration.IsUserVerifiedForTransaction(pending_pubkey, "endorsement"));
    BOOST_CHECK(integration.IsUserVerifiedForTransaction(pending_pubkey, "user_registration"));
    
    // Create verified user
    CKey verified_key;
    verified_key.MakeNewKey(true);
    CPubKey verified_pubkey = verified_key.GetPubKey();
    
    OfficialUser verified_user;
    verified_user.public_key = verified_pubkey;
    verified_user.government_id_hash = "verified_user_id";
    verified_user.birth_currency = "USD";
    verified_user.country_code = "US";
    verified_user.status = UserStatus::VERIFIED;
    
    BOOST_CHECK(user_consensus.RegisterUser(verified_user, error_message));
    
    // Test verified user for all transaction types
    BOOST_CHECK(integration.IsUserVerifiedForTransaction(verified_pubkey, "measurement"));
    BOOST_CHECK(integration.IsUserVerifiedForTransaction(verified_pubkey, "endorsement"));
    BOOST_CHECK(integration.IsUserVerifiedForTransaction(verified_pubkey, "user_registration"));
    BOOST_CHECK(integration.IsUserVerifiedForTransaction(verified_pubkey, "regular_transaction"));
}

BOOST_AUTO_TEST_CASE(test_blockchain_state_sync) {
    // Test blockchain state synchronization
    UserRegistryConsensus user_consensus;
    OBlockchainIntegration integration(nullptr, &user_consensus);
    
    // Test sync with blockchain
    BOOST_CHECK(integration.SyncUserConsensusWithBlockchain(100));
    
    // Test user statistics
    UserRegistryConsensus::UserStats stats = integration.GetUserBlockchainStats();
    BOOST_CHECK_EQUAL(stats.total_registered, 0);
    BOOST_CHECK_EQUAL(stats.total_verified, 0);
    BOOST_CHECK_EQUAL(stats.total_pending, 0);
}

BOOST_AUTO_TEST_CASE(test_integration_parameters) {
    // Test integration parameters
    UserRegistryConsensus user_consensus;
    OBlockchainIntegration integration(nullptr, &user_consensus);
    
    // Test default parameters
    OBlockchainIntegration::IntegrationParams params = integration.GetIntegrationParams();
    BOOST_CHECK(params.require_user_verification == true);
    BOOST_CHECK(params.allow_pending_users == false);
    BOOST_CHECK(params.validate_measurements == true);
    BOOST_CHECK(params.validate_endorsements == true);
    BOOST_CHECK_EQUAL(params.user_activity_timeout, 1000);
    BOOST_CHECK_CLOSE(params.measurement_reward_multiplier, 1.0, 0.001);
    BOOST_CHECK_CLOSE(params.unverified_penalty, 0.5, 0.001);
    
    // Test parameter modification
    params.require_user_verification = false;
    params.allow_pending_users = true;
    params.measurement_reward_multiplier = 1.5;
    
    integration.SetIntegrationParams(params);
    
    OBlockchainIntegration::IntegrationParams updated_params = integration.GetIntegrationParams();
    BOOST_CHECK(updated_params.require_user_verification == false);
    BOOST_CHECK(updated_params.allow_pending_users == true);
    BOOST_CHECK_CLOSE(updated_params.measurement_reward_multiplier, 1.5, 0.001);
}

BOOST_AUTO_TEST_SUITE_END()
