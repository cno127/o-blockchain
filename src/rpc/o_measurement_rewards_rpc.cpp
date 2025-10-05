// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/o_measurement_rewards_rpc.h>
#include <consensus/measurement_rewards.h>
#include <rpc/util.h>
#include <util/moneystr.h>
#include <key_io.h>
#include <addresstype.h>
#include <rpc/server.h>

using namespace OConsensus;

static RPCHelpMan getmeasurementrewardstats()
{
    return RPCHelpMan{
        "getmeasurementrewardstats",
        "\nGet measurement reward statistics.\n",
        {},
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::OBJ, "reward_stats", "Statistics by measurement type"},
                {RPCResult::Type::NUM, "total_rewards_paid", "Total number of rewards paid"},
                {RPCResult::Type::STR_AMOUNT, "total_amount_paid", "Total amount of O coins paid as rewards"},
            }
        },
        RPCExamples{
            HelpExampleCli("getmeasurementrewardstats", "")
            + HelpExampleRpc("getmeasurementrewardstats", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            auto stats = g_measurement_rewards_manager.GetMeasurementRewardStats();
            
            UniValue result(UniValue::VOBJ);
            UniValue reward_stats(UniValue::VOBJ);
            
            int total_rewards = 0;
            for (const auto& [type, count] : stats) {
                reward_stats.pushKV(type, count);
                total_rewards += count;
            }
            
            result.pushKV("reward_stats", reward_stats);
            result.pushKV("total_rewards_paid", total_rewards);
            result.pushKV("total_amount_paid", "0.00 O"); // TODO: Calculate actual total amount
            
            return result;
        },
    };
}

static RPCHelpMan getpendingmeasurementrewards()
{
    return RPCHelpMan{
        "getpendingmeasurementrewards",
        "\nGet pending measurement rewards.\n",
        {},
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::NUM, "pending_count", "Number of pending rewards"},
                {RPCResult::Type::ARR, "pending_rewards", "List of pending rewards",
                {
                    {RPCResult::Type::STR_HEX, "measurement_id", "Measurement ID"},
                    {RPCResult::Type::STR, "recipient", "Recipient address"},
                    {RPCResult::Type::STR_AMOUNT, "amount", "Reward amount"},
                    {RPCResult::Type::STR, "type", "Measurement type"},
                }},
            }
        },
        RPCExamples{
            HelpExampleCli("getpendingmeasurementrewards", "")
            + HelpExampleRpc("getpendingmeasurementrewards", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            auto pending_rewards = g_measurement_rewards_manager.GetPendingMeasurementRewards();
            
            UniValue result(UniValue::VOBJ);
            result.pushKV("pending_count", static_cast<int>(pending_rewards.size()));
            
            UniValue rewards_array(UniValue::VARR);
            for (const auto& reward : pending_rewards) {
                UniValue reward_obj(UniValue::VOBJ);
                reward_obj.pushKV("measurement_id", reward.measurement_id.GetHex());
                reward_obj.pushKV("recipient", EncodeDestination(PKHash(reward.recipient.GetID())));
                reward_obj.pushKV("amount", FormatMoney(reward.reward_amount));
                
                std::string type_name = "unknown";
                switch (reward.measurement_type) {
                    case MeasurementType::WATER_PRICE:
                        type_name = "water_price";
                        break;
                    case MeasurementType::WATER_PRICE_OFFLINE_VALIDATION:
                        type_name = "water_price_offline_validation";
                        break;
                    case MeasurementType::EXCHANGE_RATE:
                        type_name = "exchange_rate";
                        break;
                    case MeasurementType::EXCHANGE_RATE_OFFLINE_VALIDATION:
                        type_name = "exchange_rate_offline_validation";
                        break;
                    case MeasurementType::URL_SUBMISSION:
                        type_name = "url_submission";
                        break;
                    case MeasurementType::ONLINE_WATER_PRICE_VALIDATION:
                    case MeasurementType::ONLINE_EXCHANGE_VALIDATION:
                        type_name = "online_validation";
                        break;
                    case MeasurementType::ONLINE_WATER_PRICE_MEASUREMENT:
                        type_name = "online_water_price_measurement";
                        break;
                    case MeasurementType::OFFLINE_WATER_PRICE_MEASUREMENT:
                        type_name = "offline_water_price_measurement";
                        break;
                    case MeasurementType::ONLINE_EXCHANGE_RATE_MEASUREMENT:
                        type_name = "online_exchange_rate_measurement";
                        break;
                    case MeasurementType::OFFLINE_EXCHANGE_RATE_MEASUREMENT:
                        type_name = "offline_exchange_rate_measurement";
                        break;
                }
                reward_obj.pushKV("type", type_name);
                
                rewards_array.push_back(reward_obj);
            }
            
            result.pushKV("pending_rewards", rewards_array);
            
            return result;
        },
    };
}

static RPCHelpMan calculatemeasurementrewards()
{
    return RPCHelpMan{
        "calculatemeasurementrewards",
        "\nCalculate measurement rewards for current block.\n",
        {
            {"height", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "Block height (defaults to current height)"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::NUM, "block_height", "Block height"},
                {RPCResult::Type::NUM, "reward_count", "Number of rewards to be created"},
                {RPCResult::Type::STR_AMOUNT, "total_reward_amount", "Total amount of rewards"},
            }
        },
        RPCExamples{
            HelpExampleCli("calculatemeasurementrewards", "100000")
            + HelpExampleRpc("calculatemeasurementrewards", "100000")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            int height = request.params[0].isNull() ? 100000 : request.params[0].getInt<int>();
            
            // Create a dummy block for calculation
            CBlock dummy_block;
            dummy_block.nTime = GetTime();
            
            auto reward_txs = g_measurement_rewards_manager.CreateMeasurementRewardTransactions(dummy_block, height);
            CAmount total_amount = g_measurement_rewards_manager.CalculateTotalMeasurementRewards(dummy_block, height);
            
            UniValue result(UniValue::VOBJ);
            result.pushKV("block_height", height);
            result.pushKV("reward_count", static_cast<int>(reward_txs.size()));
            result.pushKV("total_reward_amount", FormatMoney(total_amount));
            
            return result;
        },
    };
}

void RegisterOMeasurementRewardsRPCCommands(CRPCTable& t)
{
    static const CRPCCommand commands[] = {
        {"measurement_rewards", &getmeasurementrewardstats},
        {"measurement_rewards", &getpendingmeasurementrewards},
        {"measurement_rewards", &calculatemeasurementrewards},
    };
    
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}
