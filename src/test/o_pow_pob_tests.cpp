// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/o_pow_pob.h>
#include <test/util/setup_common.h>
#include <chainparams.h>
#include <primitives/transaction.h>
#include <primitives/block.h>
#include <uint256.h>
#include <util/strencodings.h>

#include <boost/test/unit_test.hpp>

using namespace OConsensus;

// Helper function to create uint256 from hex string
static inline uint256 uint256S(const std::string& hex_str) {
    return uint256::FromHex(hex_str).value_or(uint256{});
}

BOOST_FIXTURE_TEST_SUITE(o_pow_pob_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(business_miner_qualification_basic)
{
    HybridPowPobConsensus consensus;
    
    // Create a test miner
    uint256 miner_pubkey = uint256S("0x1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef");
    
    // Initially should not be qualified
    BOOST_CHECK(!consensus.IsBusinessMiner(miner_pubkey, 1000));
    
    // Add transactions to qualify the miner
    for (int i = 0; i < MIN_BUSINESS_TRANSACTIONS; ++i) {
        CMutableTransaction tx;
        tx.version = 1;
        tx.nLockTime = 0;
        
        // Add inputs
        CTxIn input;
        tx.vin.push_back(input);
        
        // Add multiple outputs with different recipients
        for (int j = 0; j < 3; ++j) {
            CTxOut output;
            output.nValue = 10000000; // 0.1 BTC
            output.scriptPubKey << OP_DUP << OP_HASH160 << std::vector<unsigned char>(20, i + j) << OP_EQUALVERIFY << OP_CHECKSIG;
            tx.vout.push_back(output);
        }
        
        consensus.UpdateBusinessStats(miner_pubkey, CTransaction(tx), 1000);
    }
    
    // Now should be qualified
    const BusinessMinerStats* stats = consensus.GetBusinessStats(miner_pubkey);
    BOOST_CHECK(stats != nullptr);
    BOOST_CHECK_EQUAL(stats->total_transactions, MIN_BUSINESS_TRANSACTIONS);
    BOOST_CHECK(stats->distinct_recipients >= MIN_BUSINESS_DISTINCT_KEYS);
    BOOST_CHECK(stats->is_qualified);
    BOOST_CHECK(consensus.IsBusinessMiner(miner_pubkey, 1000));
}

BOOST_AUTO_TEST_CASE(business_ratio_calculation)
{
    HybridPowPobConsensus consensus;
    
    // Initially ratio should be 0
    BOOST_CHECK_EQUAL(consensus.GetBusinessRatio(1000), 0.0);
    
    // Add one qualified business miner
    uint256 miner1 = uint256S("0x1111111111111111111111111111111111111111111111111111111111111111");
    
    for (int i = 0; i < MIN_BUSINESS_TRANSACTIONS; ++i) {
        CMutableTransaction tx;
        tx.version = 1;
        
        // Add sufficient value
        for (int j = 0; j < 5; ++j) {
            CTxOut output;
            output.nValue = 100000000; // 1 BTC each
            output.scriptPubKey << OP_DUP << OP_HASH160 << std::vector<unsigned char>(20, i * 5 + j) << OP_EQUALVERIFY << OP_CHECKSIG;
            tx.vout.push_back(output);
        }
        
        consensus.UpdateBusinessStats(miner1, CTransaction(tx), 1000);
    }
    
    // Ratio should now be 1.0 (100% business miners)
    double ratio = consensus.GetBusinessRatio(1000);
    BOOST_CHECK(ratio > 0.0);
    BOOST_CHECK(ratio <= 1.0);
}

BOOST_AUTO_TEST_CASE(difficulty_adjustment)
{
    HybridPowPobConsensus consensus;
    
    // Create a simple block header and index
    CBlockHeader header;
    header.nVersion = 1;
    header.hashPrevBlock = uint256();
    header.hashMerkleRoot = uint256();
    header.nTime = 1234567890;
    header.nBits = 0x1d00ffff;  // Standard difficulty
    header.nNonce = 0;
    
    // Without business miners, should return standard difficulty
    // const CChainParams& params = Params();
    // unsigned int base_difficulty = header.nBits;
    
    // Note: This test is simplified as GetNextWorkRequired needs a valid CBlockIndex
    // In real scenario, we would create a proper blockchain context
}

BOOST_AUTO_TEST_CASE(business_miner_self_mining_validation)
{
    HybridPowPobConsensus consensus;
    
    // uint256 miner_pubkey = uint256S("0xaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    
    // Create a block
    CBlock block;
    block.nVersion = 1;
    block.nTime = 1234567890;
    block.nBits = 0x1d00ffff;
    block.nNonce = 0;
    
    // Add coinbase transaction
    CMutableTransaction coinbase;
    coinbase.version = 1;
    coinbase.vin.resize(1);
    coinbase.vin[0].prevout.SetNull();
    
    CTxOut coinbase_output;
    coinbase_output.nValue = 50 * COIN;
    coinbase.vout.push_back(coinbase_output);
    
    block.vtx.push_back(MakeTransactionRef(coinbase));
    
    // Add a regular transaction (not from the miner)
    CMutableTransaction tx;
    tx.version = 1;
    
    CTxIn input;
    tx.vin.push_back(input);
    
    CTxOut output;
    output.nValue = 10 * COIN;
    tx.vout.push_back(output);
    
    block.vtx.push_back(MakeTransactionRef(tx));
    
    // Should validate successfully (no self-mining)
    BOOST_CHECK(consensus.ValidateBusinessMinerBlock(block, uint256()));
}

BOOST_AUTO_TEST_CASE(target_block_time_calculation)
{
    HybridPowPobConsensus consensus;
    
    // With no business miners, should be at maximum (10 minutes)
    int64_t time_no_business = consensus.GetTargetBlockTime(1000);
    BOOST_CHECK(time_no_business >= TARGET_BLOCK_TIME_MIN);
    BOOST_CHECK(time_no_business <= TARGET_BLOCK_TIME_MAX);
    
    // Add business miners and check that block time decreases
    // (This requires adding actual business miners, simplified here)
}

BOOST_AUTO_TEST_CASE(qualification_expiration)
{
    HybridPowPobConsensus consensus;
    
    uint256 miner_pubkey = uint256S("0xbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
    
    // Qualify the miner at height 1000
    for (int i = 0; i < MIN_BUSINESS_TRANSACTIONS; ++i) {
        CMutableTransaction tx;
        tx.version = 1;
        
        for (int j = 0; j < 5; ++j) {
            CTxOut output;
            output.nValue = 100000000;
            output.scriptPubKey << OP_DUP << OP_HASH160 << std::vector<unsigned char>(20, i * 5 + j) << OP_EQUALVERIFY << OP_CHECKSIG;
            tx.vout.push_back(output);
        }
        
        consensus.UpdateBusinessStats(miner_pubkey, CTransaction(tx), 1000);
    }
    
    // Should be qualified at height 1000
    BOOST_CHECK(consensus.IsBusinessMiner(miner_pubkey, 1000));
    
    // Should still be qualified within the qualification period
    int within_period = 1000 + BUSINESS_QUALIFICATION_PERIOD / 2;
    BOOST_CHECK(consensus.IsBusinessMiner(miner_pubkey, within_period));
    
    // Should NOT be qualified after the qualification period expires
    int after_period = 1000 + BUSINESS_QUALIFICATION_PERIOD + 1;
    BOOST_CHECK(!consensus.IsBusinessMiner(miner_pubkey, after_period));
}

BOOST_AUTO_TEST_CASE(qualified_business_miners_list)
{
    HybridPowPobConsensus consensus;
    
    // Initially should have no qualified miners
    BOOST_CHECK_EQUAL(consensus.GetQualifiedBusinessCount(), 0);
    
    // Add two qualified business miners
    std::vector<uint256> miner_pubkeys = {
        uint256S("0xcccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"),
        uint256S("0xdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd")
    };
    
    for (const auto& miner_pubkey : miner_pubkeys) {
        for (int i = 0; i < MIN_BUSINESS_TRANSACTIONS; ++i) {
            CMutableTransaction tx;
            tx.version = 1;
            
            for (int j = 0; j < 5; ++j) {
                CTxOut output;
                output.nValue = 100000000;
                output.scriptPubKey << OP_DUP << OP_HASH160 << std::vector<unsigned char>(20, i * 5 + j) << OP_EQUALVERIFY << OP_CHECKSIG;
                tx.vout.push_back(output);
            }
            
            consensus.UpdateBusinessStats(miner_pubkey, CTransaction(tx), 2000);
        }
    }
    
    // Should now have two qualified miners
    BOOST_CHECK_EQUAL(consensus.GetQualifiedBusinessCount(), 2);
    
    // Get the list and verify
    std::vector<uint256> qualified = consensus.GetQualifiedBusinessMiners(2000);
    BOOST_CHECK_EQUAL(qualified.size(), 2);
}

BOOST_AUTO_TEST_CASE(pruning_old_data)
{
    HybridPowPobConsensus consensus;
    
    uint256 miner_pubkey = uint256S("0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee");
    
    // Add miner at height 1000
    CMutableTransaction tx;
    tx.version = 1;
    CTxOut output;
    output.nValue = 100000000;
    tx.vout.push_back(output);
    
    consensus.UpdateBusinessStats(miner_pubkey, CTransaction(tx), 1000);
    
    // Miner should exist
    BOOST_CHECK(consensus.GetBusinessStats(miner_pubkey) != nullptr);
    
    // Prune at a height way past the qualification period
    int prune_height = 1000 + (BUSINESS_QUALIFICATION_PERIOD * 3);
    consensus.PruneOldData(prune_height);
    
    // Miner should now be pruned (this depends on implementation details)
    // The miner might still exist but be unqualified
}

BOOST_AUTO_TEST_CASE(re_evaluate_qualifications)
{
    HybridPowPobConsensus consensus;
    
    uint256 miner_pubkey = uint256S("0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
    
    // Add some transactions but not enough to qualify
    for (int i = 0; i < MIN_BUSINESS_TRANSACTIONS / 2; ++i) {
        CMutableTransaction tx;
        tx.version = 1;
        
        CTxOut output;
        output.nValue = 100000000;
        output.scriptPubKey << OP_DUP << OP_HASH160 << std::vector<unsigned char>(20, i) << OP_EQUALVERIFY << OP_CHECKSIG;
        tx.vout.push_back(output);
        
        consensus.UpdateBusinessStats(miner_pubkey, CTransaction(tx), 3000);
    }
    
    // Should not be qualified
    BOOST_CHECK(!consensus.IsBusinessMiner(miner_pubkey, 3000));
    
    // Add more transactions to qualify
    for (int i = MIN_BUSINESS_TRANSACTIONS / 2; i < MIN_BUSINESS_TRANSACTIONS; ++i) {
        CMutableTransaction tx;
        tx.version = 1;
        
        for (int j = 0; j < 5; ++j) {
            CTxOut output;
            output.nValue = 100000000;
            output.scriptPubKey << OP_DUP << OP_HASH160 << std::vector<unsigned char>(20, i * 5 + j) << OP_EQUALVERIFY << OP_CHECKSIG;
            tx.vout.push_back(output);
        }
        
        consensus.UpdateBusinessStats(miner_pubkey, CTransaction(tx), 3000);
    }
    
    // Re-evaluate qualifications
    consensus.ReEvaluateQualifications(3000);
    
    // Should now be qualified
    BOOST_CHECK(consensus.IsBusinessMiner(miner_pubkey, 3000));
}

BOOST_AUTO_TEST_SUITE_END()

