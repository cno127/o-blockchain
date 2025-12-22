// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/o_rewards.h>
#include <consensus/amount.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(o_reward_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(test_constant_block_reward)
{
    // Test that block rewards are constant (no halving)
    CAmount reward_height_1 = ORewards::GetBlockReward(1);
    CAmount reward_height_100 = ORewards::GetBlockReward(100);
    CAmount reward_height_1000 = ORewards::GetBlockReward(1000);
    CAmount reward_height_100000 = ORewards::GetBlockReward(100000);
    
    // All rewards should be the same (constant)
    BOOST_CHECK_EQUAL(reward_height_1, reward_height_100);
    BOOST_CHECK_EQUAL(reward_height_100, reward_height_1000);
    BOOST_CHECK_EQUAL(reward_height_1000, reward_height_100000);
    
    // Should equal the BLOCK_REWARD constant
    BOOST_CHECK_EQUAL(reward_height_1, ORewards::BLOCK_REWARD);
}

BOOST_AUTO_TEST_CASE(test_block_reward_value)
{
    // Test that block reward is 2,800.00 OUSD (280,000 units)
    // O Blockchain uses 2 decimals: 1 OUSD = 100 units
    CAmount expected_reward = 280000; // 2,800.00 OUSD
    
    CAmount actual_reward = ORewards::GetBlockReward(1);
    
    BOOST_CHECK_EQUAL(actual_reward, expected_reward);
    BOOST_CHECK_EQUAL(actual_reward, ORewards::BLOCK_REWARD);
}

BOOST_AUTO_TEST_CASE(test_pob_block_reward)
{
    // Test that PoB block reward is 80% of PoW reward
    CAmount pow_reward = ORewards::GetBlockReward(1);
    CAmount pob_reward = ORewards::GetPoBBlockReward(1);
    
    // PoB should be 80% of PoW
    CAmount expected_pob = (pow_reward * 80) / 100;
    BOOST_CHECK_EQUAL(pob_reward, expected_pob);
    
    // Should equal the POB_BLOCK_REWARD constant
    BOOST_CHECK_EQUAL(pob_reward, ORewards::POB_BLOCK_REWARD);
    
    // PoB should be 2,240.00 OUSD (224,000 units)
    CAmount expected_pob_reward = 224000; // 2,240.00 OUSD
    BOOST_CHECK_EQUAL(pob_reward, expected_pob_reward);
}

BOOST_AUTO_TEST_CASE(test_pob_reward_value)
{
    // Test that PoB block reward is exactly 2,240.00 OUSD (224,000 units)
    CAmount expected_reward = 224000; // 2,240.00 OUSD
    
    CAmount actual_reward = ORewards::GetPoBBlockReward(1);
    
    BOOST_CHECK_EQUAL(actual_reward, expected_reward);
    BOOST_CHECK_EQUAL(actual_reward, ORewards::POB_BLOCK_REWARD);
}

BOOST_AUTO_TEST_CASE(test_rewards_never_zero)
{
    // Test that rewards are never zero (unlimited supply)
    for (int height = 1; height <= 1000000; height *= 10) {
        CAmount reward = ORewards::GetBlockReward(height);
        BOOST_CHECK_GT(reward, 0);
        BOOST_CHECK(ORewards::RewardsActive(height));
    }
}

BOOST_AUTO_TEST_CASE(test_reward_constants)
{
    // Verify the actual constant values
    BOOST_CHECK_EQUAL(ORewards::BLOCK_REWARD, 280000);      // 2,800.00 OUSD
    BOOST_CHECK_EQUAL(ORewards::POB_BLOCK_REWARD, 224000);  // 2,240.00 OUSD
    
    // Verify PoB is 80% of PoW
    BOOST_CHECK_EQUAL(ORewards::POB_BLOCK_REWARD, (ORewards::BLOCK_REWARD * 80) / 100);
}

BOOST_AUTO_TEST_SUITE_END()

