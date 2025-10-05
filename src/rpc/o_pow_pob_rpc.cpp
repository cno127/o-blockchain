// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/o_pow_pob.h>
#include <rpc/server.h>
#include <rpc/server_util.h>
#include <rpc/util.h>
#include <univalue.h>
#include <chainparams.h>
#include <node/context.h>
#include <validation.h>
#include <core_io.h>
#include <util/moneystr.h>
#include <chain.h>

using namespace OConsensus;
using node::NodeContext;

static RPCHelpMan getbusinessratio()
{
    return RPCHelpMan{
        "getbusinessratio",
        "\nGet the current business miner participation ratio.\n",
        {
            {"height", RPCArg::Type::NUM, RPCArg::Default{-1}, "Block height (default: current tip)"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::NUM, "height", "Block height"},
                {RPCResult::Type::NUM, "business_ratio", "Ratio of business miners (0.0 to 1.0)"},
                {RPCResult::Type::NUM, "qualified_business_miners", "Number of qualified business miners"},
                {RPCResult::Type::NUM, "target_block_time", "Target block time in seconds"},
                {RPCResult::Type::NUM, "difficulty_adjustment", "Difficulty adjustment factor"},
            }
        },
        RPCExamples{
            HelpExampleCli("getbusinessratio", "")
            + HelpExampleCli("getbusinessratio", "100000")
            + HelpExampleRpc("getbusinessratio", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            const NodeContext& node = EnsureAnyNodeContext(request.context);
            ChainstateManager& chainman = EnsureChainman(node);
            
            LOCK(cs_main);
            
            const CChain& active_chain = chainman.ActiveChain();
            int height = request.params[0].isNull() ? active_chain.Height() : request.params[0].getInt<int>();
            
            if (height < 0) {
                height = active_chain.Height();
            }
            
            double business_ratio = g_pow_pob_consensus.GetBusinessRatio(height);
            int64_t target_block_time = g_pow_pob_consensus.GetTargetBlockTime(height);
            size_t qualified_count = g_pow_pob_consensus.GetQualifiedBusinessCount();
            double difficulty_adj = 1.0 - (business_ratio * DIFFICULTY_REDUCTION_FACTOR);
            
            UniValue result(UniValue::VOBJ);
            result.pushKV("height", height);
            result.pushKV("business_ratio", business_ratio);
            result.pushKV("qualified_business_miners", (int)qualified_count);
            result.pushKV("target_block_time", target_block_time);
            result.pushKV("difficulty_adjustment", difficulty_adj);
            
            return result;
        },
    };
}

static RPCHelpMan getbusinessminerstats()
{
    return RPCHelpMan{
        "getbusinessminerstats",
        "\nGet statistics for a specific business miner.\n",
        {
            {"pubkey_hash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Public key hash of the miner"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "pubkey_hash", "Miner's public key hash"},
                {RPCResult::Type::BOOL, "is_qualified", "Whether the miner is qualified as a business miner"},
                {RPCResult::Type::NUM, "total_transactions", "Total transactions processed"},
                {RPCResult::Type::NUM, "distinct_recipients", "Number of distinct recipients"},
                {RPCResult::Type::STR_AMOUNT, "transaction_volume", "Total transaction volume"},
                {RPCResult::Type::NUM, "last_qualification_height", "Block height of last qualification update"},
                {RPCResult::Type::NUM, "first_seen_height", "Block height when first seen"},
            }
        },
        RPCExamples{
            HelpExampleCli("getbusinessminerstats", "\"0123456789abcdef...\"")
            + HelpExampleRpc("getbusinessminerstats", "\"0123456789abcdef...\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            uint256 pubkey_hash = ParseHashV(request.params[0], "pubkey_hash");
            
            const BusinessMinerStats* stats = g_pow_pob_consensus.GetBusinessStats(pubkey_hash);
            
            if (stats == nullptr) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Business miner not found");
            }
            
            UniValue result(UniValue::VOBJ);
            result.pushKV("pubkey_hash", pubkey_hash.GetHex());
            result.pushKV("is_qualified", stats->is_qualified);
            result.pushKV("total_transactions", stats->total_transactions);
            result.pushKV("distinct_recipients", stats->distinct_recipients);
            result.pushKV("transaction_volume", UniValue(UniValue::VNUM, FormatMoney(stats->transaction_volume)));
            result.pushKV("last_qualification_height", stats->last_qualification_height);
            result.pushKV("first_seen_height", stats->first_seen_height);
            
            return result;
        },
    };
}

static RPCHelpMan listqualifiedbusinessminers()
{
    return RPCHelpMan{
        "listqualifiedbusinessminers",
        "\nList all qualified business miners.\n",
        {
            {"height", RPCArg::Type::NUM, RPCArg::Default{-1}, "Block height (default: current tip)"},
        },
        RPCResult{
            RPCResult::Type::ARR, "", "Array of qualified business miners",
            {
                {RPCResult::Type::OBJ, "", "",
                {
                    {RPCResult::Type::STR_HEX, "pubkey_hash", "Miner's public key hash"},
                    {RPCResult::Type::NUM, "total_transactions", "Total transactions processed"},
                    {RPCResult::Type::NUM, "distinct_recipients", "Number of distinct recipients"},
                    {RPCResult::Type::STR_AMOUNT, "transaction_volume", "Total transaction volume"},
                }},
            }
        },
        RPCExamples{
            HelpExampleCli("listqualifiedbusinessminers", "")
            + HelpExampleCli("listqualifiedbusinessminers", "100000")
            + HelpExampleRpc("listqualifiedbusinessminers", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            const NodeContext& node = EnsureAnyNodeContext(request.context);
            ChainstateManager& chainman = EnsureChainman(node);
            
            LOCK(cs_main);
            
            const CChain& active_chain = chainman.ActiveChain();
            int height = request.params[0].isNull() ? active_chain.Height() : request.params[0].getInt<int>();
            
            if (height < 0) {
                height = active_chain.Height();
            }
            
            std::vector<uint256> qualified = g_pow_pob_consensus.GetQualifiedBusinessMiners(height);
            
            UniValue result(UniValue::VARR);
            
            for (const auto& pubkey : qualified) {
                const BusinessMinerStats* stats = g_pow_pob_consensus.GetBusinessStats(pubkey);
                if (stats != nullptr) {
                    UniValue miner(UniValue::VOBJ);
                    miner.pushKV("pubkey_hash", pubkey.GetHex());
                    miner.pushKV("total_transactions", stats->total_transactions);
                    miner.pushKV("distinct_recipients", stats->distinct_recipients);
                    miner.pushKV("transaction_volume", UniValue(UniValue::VNUM, FormatMoney(stats->transaction_volume)));
                    result.push_back(miner);
                }
            }
            
            return result;
        },
    };
}

static RPCHelpMan isbusinessminer()
{
    return RPCHelpMan{
        "isbusinessminer",
        "\nCheck if a public key qualifies as a business miner.\n",
        {
            {"pubkey_hash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Public key hash to check"},
            {"height", RPCArg::Type::NUM, RPCArg::Default{-1}, "Block height (default: current tip)"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::BOOL, "is_business_miner", "Whether the public key qualifies as a business miner"},
                {RPCResult::Type::STR, "reason", "Reason for qualification status"},
            }
        },
        RPCExamples{
            HelpExampleCli("isbusinessminer", "\"0123456789abcdef...\"")
            + HelpExampleRpc("isbusinessminer", "\"0123456789abcdef...\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            const NodeContext& node = EnsureAnyNodeContext(request.context);
            ChainstateManager& chainman = EnsureChainman(node);
            
            LOCK(cs_main);
            
            uint256 pubkey_hash = ParseHashV(request.params[0], "pubkey_hash");
            const CChain& active_chain = chainman.ActiveChain();
            int height = request.params[1].isNull() ? active_chain.Height() : request.params[1].getInt<int>();
            
            if (height < 0) {
                height = active_chain.Height();
            }
            
            bool is_business = g_pow_pob_consensus.IsBusinessMiner(pubkey_hash, height);
            
            std::string reason;
            const BusinessMinerStats* stats = g_pow_pob_consensus.GetBusinessStats(pubkey_hash);
            
            if (stats == nullptr) {
                reason = "Miner not found in database";
            } else if (!is_business) {
                if (stats->total_transactions < MIN_BUSINESS_TRANSACTIONS) {
                    reason = strprintf("Insufficient transactions (%d < %d required)", 
                                     stats->total_transactions, MIN_BUSINESS_TRANSACTIONS);
                } else if (stats->distinct_recipients < MIN_BUSINESS_DISTINCT_KEYS) {
                    reason = strprintf("Insufficient distinct recipients (%d < %d required)", 
                                     stats->distinct_recipients, MIN_BUSINESS_DISTINCT_KEYS);
                } else if (stats->transaction_volume < MIN_BUSINESS_VOLUME) {
                    reason = strprintf("Insufficient transaction volume (%s < %s required)", 
                                     FormatMoney(stats->transaction_volume), 
                                     FormatMoney(MIN_BUSINESS_VOLUME));
                } else if (height - stats->last_qualification_height > BUSINESS_QUALIFICATION_PERIOD) {
                    reason = "Qualification period expired";
                } else {
                    reason = "Unknown reason";
                }
            } else {
                reason = "Meets all business miner requirements";
            }
            
            UniValue result(UniValue::VOBJ);
            result.pushKV("is_business_miner", is_business);
            result.pushKV("reason", reason);
            
            return result;
        },
    };
}

static RPCHelpMan getpowpobstats()
{
    return RPCHelpMan{
        "getpowpobstats",
        "\nGet overall PoW/PoB consensus statistics.\n",
        {},
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::NUM, "total_business_miners", "Total number of business miners tracked"},
                {RPCResult::Type::NUM, "qualified_business_miners", "Number of qualified business miners"},
                {RPCResult::Type::NUM, "current_business_ratio", "Current business miner ratio"},
                {RPCResult::Type::NUM, "target_block_time", "Current target block time in seconds"},
                {RPCResult::Type::NUM, "min_transactions_required", "Minimum transactions for business qualification"},
                {RPCResult::Type::NUM, "min_recipients_required", "Minimum distinct recipients required"},
                {RPCResult::Type::STR_AMOUNT, "min_volume_required", "Minimum transaction volume required"},
            }
        },
        RPCExamples{
            HelpExampleCli("getpowpobstats", "")
            + HelpExampleRpc("getpowpobstats", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            const NodeContext& node = EnsureAnyNodeContext(request.context);
            ChainstateManager& chainman = EnsureChainman(node);
            
            LOCK(cs_main);
            
            const CChain& active_chain = chainman.ActiveChain();
            int height = active_chain.Height();
            double business_ratio = g_pow_pob_consensus.GetBusinessRatio(height);
            int64_t target_block_time = g_pow_pob_consensus.GetTargetBlockTime(height);
            size_t qualified_count = g_pow_pob_consensus.GetQualifiedBusinessCount();
            
            UniValue result(UniValue::VOBJ);
            result.pushKV("total_business_miners", (int)qualified_count);  // Simplified for now
            result.pushKV("qualified_business_miners", (int)qualified_count);
            result.pushKV("current_business_ratio", business_ratio);
            result.pushKV("target_block_time", target_block_time);
            result.pushKV("min_transactions_required", MIN_BUSINESS_TRANSACTIONS);
            result.pushKV("min_recipients_required", MIN_BUSINESS_DISTINCT_KEYS);
            result.pushKV("min_volume_required", UniValue(UniValue::VNUM, FormatMoney(MIN_BUSINESS_VOLUME)));
            
            return result;
        },
    };
}

void RegisterOPowPobRPCCommands(CRPCTable& t)
{
    static const CRPCCommand commands[] = {
        {"blockchain", &getbusinessratio},
        {"blockchain", &getbusinessminerstats},
        {"blockchain", &listqualifiedbusinessminers},
        {"blockchain", &isbusinessminer},
        {"blockchain", &getpowpobstats},
    };
    
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}

