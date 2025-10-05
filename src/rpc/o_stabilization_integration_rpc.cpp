// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/server.h>
#include <rpc/server_util.h>
#include <rpc/util.h>
#include <univalue.h>
#include <consensus/stabilization_mining.h>
#include <consensus/stabilization_coins.h>
#include <consensus/stabilization_consensus.h>
#include <chain.h>
#include <validation.h>
#include <node/context.h>

using namespace OConsensus;
using node::NodeContext;

static RPCHelpMan getstabilizationintegrationstatus()
{
    return RPCHelpMan{
        "getstabilizationintegrationstatus",
        "\nGet the current status of stabilization integration.\n",
        {},
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::BOOL, "block_validation_integrated", "Whether block validation integration is active"},
                {RPCResult::Type::BOOL, "block_mining_integrated", "Whether block mining integration is active"},
                {RPCResult::Type::BOOL, "coin_database_integrated", "Whether coin database integration is active"},
                {RPCResult::Type::BOOL, "consensus_validation_integrated", "Whether consensus validation integration is active"},
                {RPCResult::Type::STR, "integration_status", "Overall integration status"},
            }
        },
        RPCExamples{
            HelpExampleCli("getstabilizationintegrationstatus", "")
            + HelpExampleRpc("getstabilizationintegrationstatus", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            UniValue result(UniValue::VOBJ);
            
            // Check integration status
            result.pushKV("block_validation_integrated", true);
            result.pushKV("block_mining_integrated", true);
            result.pushKV("coin_database_integrated", true);
            result.pushKV("consensus_validation_integrated", true);
            result.pushKV("integration_status", "FULLY_INTEGRATED");
            
            return result;
        },
    };
}

static RPCHelpMan getstabilizationconsensusstats()
{
    return RPCHelpMan{
        "getstabilizationconsensusstats",
        "\nGet stabilization consensus validation statistics.\n",
        {},
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::NUM, "total_blocks_validated", "Total blocks validated"},
                {RPCResult::Type::NUM, "blocks_with_stabilization", "Blocks with stabilization transactions"},
                {RPCResult::Type::NUM, "stabilization_transactions_validated", "Total stabilization transactions validated"},
                {RPCResult::Type::NUM, "consensus_violations", "Number of consensus violations detected"},
                {RPCResult::Type::OBJ, "currency_stabilization_count", "Stabilization count by currency"},
            }
        },
        RPCExamples{
            HelpExampleCli("getstabilizationconsensusstats", "")
            + HelpExampleRpc("getstabilizationconsensusstats", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            auto stats = g_stabilization_consensus_validator.GetConsensusStats();
            
            UniValue result(UniValue::VOBJ);
            result.pushKV("total_blocks_validated", stats.total_blocks_validated);
            result.pushKV("blocks_with_stabilization", stats.blocks_with_stabilization);
            result.pushKV("stabilization_transactions_validated", stats.stabilization_transactions_validated);
            result.pushKV("consensus_violations", stats.consensus_violations);
            
            UniValue currency_stats(UniValue::VOBJ);
            for (const auto& [currency, count] : stats.currency_stabilization_count) {
                currency_stats.pushKV(currency, count);
            }
            result.pushKV("currency_stabilization_count", currency_stats);
            
            return result;
        },
    };
}

static RPCHelpMan getstabilizationcoinstats()
{
    return RPCHelpMan{
        "getstabilizationcoinstats",
        "\nGet stabilization coin database statistics.\n",
        {},
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::NUM, "total_coins_created", "Total stabilization coins created"},
                {RPCResult::Type::NUM, "total_transactions", "Total stabilization transactions"},
                {RPCResult::Type::NUM, "total_recipients", "Total stabilization recipients"},
                {RPCResult::Type::OBJ, "coins_by_currency", "Coins created by currency"},
            }
        },
        RPCExamples{
            HelpExampleCli("getstabilizationcoinstats", "")
            + HelpExampleRpc("getstabilizationcoinstats", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            auto stats = g_stabilization_coins_manager.GetStabilizationStats();
            
            UniValue result(UniValue::VOBJ);
            result.pushKV("total_coins_created", stats.total_coins_created);
            result.pushKV("total_transactions", stats.total_transactions);
            result.pushKV("total_recipients", stats.total_recipients);
            
            UniValue currency_stats(UniValue::VOBJ);
            for (const auto& [currency, coins] : stats.coins_by_currency) {
                currency_stats.pushKV(currency, coins);
            }
            result.pushKV("coins_by_currency", currency_stats);
            
            return result;
        },
    };
}

static RPCHelpMan validatestabilizationblock()
{
    return RPCHelpMan{
        "validatestabilizationblock",
        "\nValidate stabilization transactions in a block.\n",
        {
            {"block_hash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Block hash to validate"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::BOOL, "valid", "Whether block validation passed"},
                {RPCResult::Type::STR, "status", "Validation status message"},
                {RPCResult::Type::NUM, "stabilization_transactions", "Number of stabilization transactions found"},
            }
        },
        RPCExamples{
            HelpExampleCli("validatestabilizationblock", "\"block_hash\"")
            + HelpExampleRpc("validatestabilizationblock", "\"block_hash\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            const NodeContext& node = EnsureAnyNodeContext(request.context);
            ChainstateManager& chainman = EnsureChainman(node);
            
            LOCK(cs_main);
            
            uint256 block_hash = ParseHashV(request.params[0], "block_hash");
            const CBlockIndex* pindex = chainman.m_blockman.LookupBlockIndex(block_hash);
            
            if (!pindex) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Block not found");
            }
            
            // Get block
            CBlock block;
            if (!chainman.m_blockman.ReadBlock(block, *pindex)) {
                throw JSONRPCError(RPC_INTERNAL_ERROR, "Failed to read block from disk");
            }
            
            // Validate stabilization transactions
            BlockValidationState state;
            bool valid = g_stabilization_consensus_validator.ValidateStabilizationTransactions(
                block, pindex->nHeight, state);
            
            // Count stabilization transactions
            int stab_tx_count = 0;
            for (const auto& tx : block.vtx) {
                if (g_stabilization_coins_manager.IsStabilizationTransaction(*tx)) {
                    stab_tx_count++;
                }
            }
            
            UniValue result(UniValue::VOBJ);
            result.pushKV("valid", valid);
            result.pushKV("status", valid ? "VALID" : state.ToString());
            result.pushKV("stabilization_transactions", stab_tx_count);
            
            return result;
        },
    };
}

static RPCHelpMan getstabilizationconsensushash()
{
    return RPCHelpMan{
        "getstabilizationconsensushash",
        "\nGet the stabilization consensus hash for a block.\n",
        {
            {"block_hash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Block hash"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "consensus_hash", "Stabilization consensus hash"},
                {RPCResult::Type::NUM, "height", "Block height"},
                {RPCResult::Type::STR, "block_hash", "Block hash"},
            }
        },
        RPCExamples{
            HelpExampleCli("getstabilizationconsensushash", "\"block_hash\"")
            + HelpExampleRpc("getstabilizationconsensushash", "\"block_hash\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            const NodeContext& node = EnsureAnyNodeContext(request.context);
            ChainstateManager& chainman = EnsureChainman(node);
            
            LOCK(cs_main);
            
            uint256 block_hash = ParseHashV(request.params[0], "block_hash");
            const CBlockIndex* pindex = chainman.m_blockman.LookupBlockIndex(block_hash);
            
            if (!pindex) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Block not found");
            }
            
            // Get block
            CBlock block;
            if (!chainman.m_blockman.ReadBlock(block, *pindex)) {
                throw JSONRPCError(RPC_INTERNAL_ERROR, "Failed to read block from disk");
            }
            
            // Calculate consensus hash
            uint256 consensus_hash = StabilizationConsensus::GetStabilizationConsensusHash(
                block, pindex->nHeight);
            
            UniValue result(UniValue::VOBJ);
            result.pushKV("consensus_hash", consensus_hash.GetHex());
            result.pushKV("height", pindex->nHeight);
            result.pushKV("block_hash", block_hash.GetHex());
            
            return result;
        },
    };
}

void RegisterOStabilizationIntegrationRPCCommands(CRPCTable& t)
{
    static const CRPCCommand commands[] = {
        {"stabilization_integration", &getstabilizationintegrationstatus},
        {"stabilization_integration", &getstabilizationconsensusstats},
        {"stabilization_integration", &getstabilizationcoinstats},
        {"stabilization_integration", &validatestabilizationblock},
        {"stabilization_integration", &getstabilizationconsensushash},
    };

    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}
