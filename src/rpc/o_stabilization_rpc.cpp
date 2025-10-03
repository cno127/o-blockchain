// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/o_stabilization_rpc.h>
#include <consensus/stabilization_mining.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <univalue.h>
#include <util/moneystr.h>

using namespace OConsensus;

static RPCHelpMan getstabilitystatus()
{
    return RPCHelpMan{"getstabilitystatus",
        "\nGet stability status for a currency.\n",
        {
            {"currency", RPCArg::Type::STR, RPCArg::Optional::NO, "Currency code (USD, EUR, etc.)"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "currency", "Currency code"},
                {RPCResult::Type::NUM, "expected_water_price", "Expected water price (1.0 for O standard)"},
                {RPCResult::Type::NUM, "observed_water_price", "Observed water price from measurements"},
                {RPCResult::Type::NUM, "observed_exchange_rate", "Exchange rate with O coin"},
                {RPCResult::Type::NUM, "stability_ratio", "Deviation ratio (0.0 = perfect, >0.1 = unstable)"},
                {RPCResult::Type::STR, "status", "Status: STABLE, UNSTABLE, INACTIVE, or UNKNOWN"},
                {RPCResult::Type::NUM, "unstable_since_height", "Block height when became unstable (0 if stable)"},
                {RPCResult::Type::NUM, "last_check_height", "Last block height checked"},
                {RPCResult::Type::NUM, "measurement_count", "Number of measurements used"},
            }
        },
        RPCExamples{
            HelpExampleCli("getstabilitystatus", "\"USD\"")
            + HelpExampleRpc("getstabilitystatus", "\"USD\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            std::string currency = request.params[0].get_str();
            
            auto status = g_stabilization_mining.GetStabilityStatus(currency);
            
            if (!status.has_value()) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Currency not found or not yet evaluated");
            }
            
            std::string status_str;
            switch (status->status) {
                case StabilityStatus::STABLE: status_str = "STABLE"; break;
                case StabilityStatus::UNSTABLE: status_str = "UNSTABLE"; break;
                case StabilityStatus::INACTIVE: status_str = "INACTIVE"; break;
                default: status_str = "UNKNOWN";
            }
            
            UniValue result(UniValue::VOBJ);
            result.pushKV("currency", status->currency_code);
            result.pushKV("expected_water_price", status->expected_water_price);
            result.pushKV("observed_water_price", status->observed_water_price);
            result.pushKV("observed_exchange_rate", status->observed_exchange_rate);
            result.pushKV("stability_ratio", status->stability_ratio);
            result.pushKV("status", status_str);
            result.pushKV("unstable_since_height", status->unstable_since_height);
            result.pushKV("last_check_height", status->last_check_height);
            result.pushKV("measurement_count", status->measurement_count);
            
            return result;
        },
    };
}

static RPCHelpMan getunstablecurrencies()
{
    return RPCHelpMan{"getunstablecurrencies",
        "\nGet list of all unstable currencies.\n",
        {},
        RPCResult{
            RPCResult::Type::ARR, "", "Array of unstable currencies",
            {
                {RPCResult::Type::OBJ, "", "",
                {
                    {RPCResult::Type::STR, "currency", "Currency code"},
                    {RPCResult::Type::NUM, "stability_ratio", "Deviation ratio"},
                    {RPCResult::Type::NUM, "unstable_since_height", "Block height when became unstable"},
                    {RPCResult::Type::NUM, "blocks_unstable", "Number of blocks unstable"},
                }},
            }
        },
        RPCExamples{
            HelpExampleCli("getunstablecurrencies", "")
            + HelpExampleRpc("getunstablecurrencies", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            std::vector<std::string> unstable = g_stabilization_mining.GetUnstableCurrencies();
            
            UniValue result(UniValue::VARR);
            
            for (const auto& currency : unstable) {
                auto status = g_stabilization_mining.GetStabilityStatus(currency);
                if (status.has_value()) {
                    UniValue curr(UniValue::VOBJ);
                    curr.pushKV("currency", currency);
                    curr.pushKV("stability_ratio", status->stability_ratio);
                    curr.pushKV("unstable_since_height", status->unstable_since_height);
                    // Note: Would need current height from context for blocks_unstable
                    curr.pushKV("blocks_unstable", 0); // TODO: Calculate from current height
                    result.push_back(curr);
                }
            }
            
            return result;
        },
    };
}

static RPCHelpMan getstabilizationstats()
{
    return RPCHelpMan{"getstabilizationstats",
        "\nGet overall stabilization mining statistics.\n",
        {},
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::NUM, "total_unstable_currencies", "Number of unstable currencies"},
                {RPCResult::Type::NUM, "total_stable_currencies", "Number of stable currencies"},
                {RPCResult::Type::NUM, "total_inactive_currencies", "Number of inactive currencies"},
                {RPCResult::Type::STR_AMOUNT, "total_coins_created", "Total stabilization coins created"},
                {RPCResult::Type::NUM, "total_transactions", "Total stabilization transactions"},
                {RPCResult::Type::NUM, "total_recipients", "Total recipients of stabilization rewards"},
            }
        },
        RPCExamples{
            HelpExampleCli("getstabilizationstats", "")
            + HelpExampleRpc("getstabilizationstats", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            auto stats = g_stabilization_mining.GetStatistics();
            
            UniValue result(UniValue::VOBJ);
            result.pushKV("total_unstable_currencies", stats.total_unstable_currencies);
            result.pushKV("total_stable_currencies", stats.total_stable_currencies);
            result.pushKV("total_inactive_currencies", stats.total_inactive_currencies);
            result.pushKV("total_coins_created", UniValue(UniValue::VNUM, FormatMoney(stats.total_coins_created)));
            result.pushKV("total_transactions", stats.total_transactions);
            result.pushKV("total_recipients", stats.total_recipients);
            
            return result;
        },
    };
}

static RPCHelpMan getstabilizationhistory()
{
    return RPCHelpMan{"getstabilizationhistory",
        "\nGet stabilization transaction history for a currency.\n",
        {
            {"currency", RPCArg::Type::STR, RPCArg::Optional::NO, "Currency code"},
            {"start_height", RPCArg::Type::NUM, RPCArg::Default{0}, "Start block height"},
            {"end_height", RPCArg::Type::NUM, RPCArg::Default{999999999}, "End block height"},
        },
        RPCResult{
            RPCResult::Type::ARR, "", "Array of stabilization transactions",
            {
                {RPCResult::Type::OBJ, "", "",
                {
                    {RPCResult::Type::STR_HEX, "tx_id", "Transaction ID"},
                    {RPCResult::Type::STR, "unstable_currency", "Currency that caused stabilization"},
                    {RPCResult::Type::STR_AMOUNT, "coins_created", "Amount of coins created"},
                    {RPCResult::Type::NUM, "recipient_count", "Number of recipients"},
                    {RPCResult::Type::NUM, "block_height", "Block height"},
                    {RPCResult::Type::NUM, "timestamp", "Unix timestamp"},
                    {RPCResult::Type::NUM, "deviation_ratio", "Deviation ratio that triggered"},
                }},
            }
        },
        RPCExamples{
            HelpExampleCli("getstabilizationhistory", "\"USD\"")
            + HelpExampleCli("getstabilizationhistory", "\"EUR\" 100000 200000")
            + HelpExampleRpc("getstabilizationhistory", "\"USD\", 0, 999999999")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            std::string currency = request.params[0].get_str();
            int start_height = request.params[1].isNull() ? 0 : request.params[1].getInt<int>();
            int end_height = request.params[2].isNull() ? 999999999 : request.params[2].getInt<int>();
            
            std::vector<StabilizationTransaction> history = 
                g_stabilization_mining.GetStabilizationHistory(currency, start_height, end_height);
            
            UniValue result(UniValue::VARR);
            
            for (const auto& tx : history) {
                UniValue obj(UniValue::VOBJ);
                obj.pushKV("tx_id", tx.tx_id.GetHex());
                obj.pushKV("unstable_currency", tx.unstable_currency);
                obj.pushKV("coins_created", UniValue(UniValue::VNUM, FormatMoney(tx.coins_created)));
                obj.pushKV("recipient_count", (int)tx.recipients.size());
                obj.pushKV("block_height", tx.block_height);
                obj.pushKV("timestamp", tx.timestamp);
                obj.pushKV("deviation_ratio", tx.deviation_ratio);
                result.push_back(obj);
            }
            
            return result;
        },
    };
}

static RPCHelpMan gettotalcoinscreated()
{
    return RPCHelpMan{"gettotalcoinscreated",
        "\nGet total stabilization coins created for a currency.\n",
        {
            {"currency", RPCArg::Type::STR, RPCArg::Optional::NO, "Currency code"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "currency", "Currency code"},
                {RPCResult::Type::STR_AMOUNT, "total_coins_created", "Total coins created"},
            }
        },
        RPCExamples{
            HelpExampleCli("gettotalcoinscreated", "\"USD\"")
            + HelpExampleRpc("gettotalcoinscreated", "\"USD\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            std::string currency = request.params[0].get_str();
            
            CAmount total = g_stabilization_mining.GetTotalCoinsCreated(currency);
            
            UniValue result(UniValue::VOBJ);
            result.pushKV("currency", currency);
            result.pushKV("total_coins_created", UniValue(UniValue::VNUM, FormatMoney(total)));
            
            return result;
        },
    };
}

void RegisterOStabilizationRPCCommands(CRPCTable& t)
{
    static const CRPCCommand commands[] = {
        {"stabilization", &getstabilitystatus},
        {"stabilization", &getunstablecurrencies},
        {"stabilization", &getstabilizationstats},
        {"stabilization", &getstabilizationhistory},
        {"stabilization", &gettotalcoinscreated},
    };
    
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}

