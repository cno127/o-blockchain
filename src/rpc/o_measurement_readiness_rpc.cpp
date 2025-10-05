// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/o_measurement_readiness_rpc.h>
#include <consensus/measurement_readiness.h>
#include <rpc/util.h>
#include <util/moneystr.h>
#include <rpc/server.h>

using namespace OConsensus;

static RPCHelpMan updateusercount()
{
    return RPCHelpMan{
        "updateusercount",
        "\nUpdate user count for an O currency.\n",
        {
            {"o_currency", RPCArg::Type::STR, RPCArg::Optional::NO, "O currency code (e.g., 'OUSD')"},
            {"user_count", RPCArg::Type::NUM, RPCArg::Optional::NO, "Current user count"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "o_currency", "O currency code"},
                {RPCResult::Type::NUM, "user_count", "Updated user count"},
                {RPCResult::Type::BOOL, "water_price_ready", "Whether ready for water price measurements"},
                {RPCResult::Type::NUM, "progress_percent", "Progress percentage towards minimum users"},
            }
        },
        RPCExamples{
            HelpExampleCli("updateusercount", "OUSD 150")
            + HelpExampleRpc("updateusercount", "OUSD 150")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            std::string o_currency = request.params[0].get_str();
            int user_count = request.params[1].getInt<int>();
            
            g_measurement_readiness_manager.UpdateUserCount(o_currency, user_count);
            
            UniValue result(UniValue::VOBJ);
            result.pushKV("o_currency", o_currency);
            result.pushKV("user_count", user_count);
            result.pushKV("water_price_ready", g_measurement_readiness_manager.IsWaterPriceMeasurementReady(o_currency));
            
            double progress = (double)user_count / g_measurement_readiness_manager.GetMinimumUsersForWaterPrice() * 100.0;
            result.pushKV("progress_percent", progress);
            
            return result;
        },
    };
}

static RPCHelpMan updatecoinsupply()
{
    return RPCHelpMan{
        "updatecoinsupply",
        "\nUpdate coin supply for an O currency.\n",
        {
            {"o_currency", RPCArg::Type::STR, RPCArg::Optional::NO, "O currency code (e.g., 'OUSD')"},
            {"total_supply", RPCArg::Type::STR, RPCArg::Optional::NO, "Total supply in O coins"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "o_currency", "O currency code"},
                {RPCResult::Type::STR, "total_supply", "Updated total supply"},
                {RPCResult::Type::BOOL, "exchange_rate_ready", "Whether ready for exchange rate measurements"},
                {RPCResult::Type::NUM, "progress_percent", "Progress percentage towards minimum coins"},
            }
        },
        RPCExamples{
            HelpExampleCli("updatecoinsupply", "OUSD 150000.00")
            + HelpExampleRpc("updatecoinsupply", "OUSD 150000.00")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            std::string o_currency = request.params[0].get_str();
            CAmount total_supply = AmountFromValue(request.params[1]);
            
            g_measurement_readiness_manager.UpdateCoinSupply(o_currency, total_supply);
            
            UniValue result(UniValue::VOBJ);
            result.pushKV("o_currency", o_currency);
            result.pushKV("total_supply", FormatMoney(total_supply));
            result.pushKV("exchange_rate_ready", g_measurement_readiness_manager.IsExchangeRateMeasurementReady(o_currency));
            
            double progress = (double)total_supply / g_measurement_readiness_manager.GetMinimumCoinsForExchangeRate() * 100.0;
            result.pushKV("progress_percent", progress);
            
            return result;
        },
    };
}

static RPCHelpMan getreadinessstatus()
{
    return RPCHelpMan{
        "getreadinessstatus",
        "\nGet detailed readiness status for an O currency.\n",
        {
            {"o_currency", RPCArg::Type::STR, RPCArg::Optional::NO, "O currency code (e.g., 'OUSD')"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "o_currency", "O currency code"},
                {RPCResult::Type::NUM, "user_count", "Current user count"},
                {RPCResult::Type::STR, "coin_supply", "Current coin supply"},
                {RPCResult::Type::BOOL, "water_price_ready", "Whether ready for water price measurements"},
                {RPCResult::Type::BOOL, "exchange_rate_ready", "Whether ready for exchange rate measurements"},
                {RPCResult::Type::BOOL, "fully_ready", "Whether fully ready for all measurements"},
                {RPCResult::Type::STR, "readiness_status", "Overall readiness status"},
                {RPCResult::Type::NUM, "user_progress_percent", "Progress towards minimum users"},
                {RPCResult::Type::NUM, "coin_progress_percent", "Progress towards minimum coins"},
                {RPCResult::Type::STR, "last_updated", "Last update timestamp"},
            }
        },
        RPCExamples{
            HelpExampleCli("getreadinessstatus", "OUSD")
            + HelpExampleRpc("getreadinessstatus", "OUSD")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            std::string o_currency = request.params[0].get_str();
            auto status_map = g_measurement_readiness_manager.GetDetailedReadinessStatus(o_currency);
            
            UniValue result(UniValue::VOBJ);
            for (const auto& pair : status_map) {
                result.pushKV(pair.first, pair.second);
            }
            return result;
        },
    };
}

static RPCHelpMan getreadinessstatistics()
{
    return RPCHelpMan{
        "getreadinessstatistics",
        "\nGet overall readiness statistics for all O currencies.\n",
        {},
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::NUM, "total_currencies_tracked", "Total number of currencies being tracked"},
                {RPCResult::Type::NUM, "water_price_ready_count", "Number of currencies ready for water price measurements"},
                {RPCResult::Type::NUM, "exchange_rate_ready_count", "Number of currencies ready for exchange rate measurements"},
                {RPCResult::Type::NUM, "fully_ready_count", "Number of currencies fully ready for all measurements"},
                {RPCResult::Type::NUM, "minimum_users_for_water_price", "Minimum users required for water price measurements"},
                {RPCResult::Type::STR, "minimum_coins_for_exchange_rate", "Minimum coins required for exchange rate measurements"},
            }
        },
        RPCExamples{
            HelpExampleCli("getreadinessstatistics", "")
            + HelpExampleRpc("getreadinessstatistics", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            auto stats_map = g_measurement_readiness_manager.GetReadinessStatistics();
            
            UniValue result(UniValue::VOBJ);
            for (const auto& pair : stats_map) {
                result.pushKV(pair.first, pair.second);
            }
            return result;
        },
    };
}

static RPCHelpMan getreadyforwaterprice()
{
    return RPCHelpMan{
        "getreadyforwaterprice",
        "\nGet list of O currencies ready for water price measurements.\n",
        {},
        RPCResult{
            RPCResult::Type::ARR, "", "",
            {
                {RPCResult::Type::STR, "", "O currency code ready for water price measurements"},
            }
        },
        RPCExamples{
            HelpExampleCli("getreadyforwaterprice", "")
            + HelpExampleRpc("getreadyforwaterprice", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            auto ready_currencies = g_measurement_readiness_manager.GetReadyForWaterPriceMeasurements();
            
            UniValue result(UniValue::VARR);
            for (const auto& currency : ready_currencies) {
                result.push_back(currency);
            }
            
            return result;
        },
    };
}

static RPCHelpMan getreadyforexchangerate()
{
    return RPCHelpMan{
        "getreadyforexchangerate",
        "\nGet list of O currencies ready for exchange rate measurements.\n",
        {},
        RPCResult{
            RPCResult::Type::ARR, "", "",
            {
                {RPCResult::Type::STR, "", "O currency code ready for exchange rate measurements"},
            }
        },
        RPCExamples{
            HelpExampleCli("getreadyforexchangerate", "")
            + HelpExampleRpc("getreadyforexchangerate", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            auto ready_currencies = g_measurement_readiness_manager.GetReadyForExchangeRateMeasurements();
            
            UniValue result(UniValue::VARR);
            for (const auto& currency : ready_currencies) {
                result.push_back(currency);
            }
            
            return result;
        },
    };
}

static RPCHelpMan getfullyreadycurrencies()
{
    return RPCHelpMan{
        "getfullyreadycurrencies",
        "\nGet list of O currencies fully ready for all measurements.\n",
        {},
        RPCResult{
            RPCResult::Type::ARR, "", "",
            {
                {RPCResult::Type::STR, "", "O currency code fully ready for all measurements"},
            }
        },
        RPCExamples{
            HelpExampleCli("getfullyreadycurrencies", "")
            + HelpExampleRpc("getfullyreadycurrencies", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            auto fully_ready_currencies = g_measurement_readiness_manager.GetFullyReadyCurrencies();
            
            UniValue result(UniValue::VARR);
            for (const auto& currency : fully_ready_currencies) {
                result.push_back(currency);
            }
            
            return result;
        },
    };
}

static RPCHelpMan checkmeasurementreadiness()
{
    return RPCHelpMan{
        "checkmeasurementreadiness",
        "\nCheck if measurements can be started for an O currency.\n",
        {
            {"o_currency", RPCArg::Type::STR, RPCArg::Optional::NO, "O currency code (e.g., 'OUSD')"},
            {"measurement_type", RPCArg::Type::STR, RPCArg::Optional::NO, "Measurement type ('water_price' or 'exchange_rate')"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "o_currency", "O currency code"},
                {RPCResult::Type::STR, "measurement_type", "Measurement type checked"},
                {RPCResult::Type::BOOL, "ready", "Whether ready to start measurements"},
                {RPCResult::Type::STR, "reason", "Reason for readiness status"},
                {RPCResult::Type::STR, "current_value", "Current value (users or coins)"},
                {RPCResult::Type::STR, "required_value", "Required value (users or coins)"},
                {RPCResult::Type::NUM, "progress_percent", "Progress percentage"},
            }
        },
        RPCExamples{
            HelpExampleCli("checkmeasurementreadiness", "OUSD water_price")
            + HelpExampleRpc("checkmeasurementreadiness", "OUSD water_price")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            std::string o_currency = request.params[0].get_str();
            std::string measurement_type = request.params[1].get_str();
            
            UniValue result(UniValue::VOBJ);
            result.pushKV("o_currency", o_currency);
            result.pushKV("measurement_type", measurement_type);
            
            if (measurement_type == "water_price") {
                bool ready = g_measurement_readiness_manager.IsWaterPriceMeasurementReady(o_currency);
                int user_count = g_measurement_readiness_manager.GetUserCount(o_currency);
                int required = g_measurement_readiness_manager.GetMinimumUsersForWaterPrice();
                
                result.pushKV("ready", ready);
                result.pushKV("reason", ready ? "Sufficient users for water price measurements" : "Insufficient users for water price measurements");
                result.pushKV("current_value", user_count);
                result.pushKV("required_value", required);
                result.pushKV("progress_percent", (double)user_count / required * 100.0);
                
            } else if (measurement_type == "exchange_rate") {
                bool ready = g_measurement_readiness_manager.IsExchangeRateMeasurementReady(o_currency);
                CAmount coin_supply = g_measurement_readiness_manager.GetCoinSupply(o_currency);
                CAmount required = g_measurement_readiness_manager.GetMinimumCoinsForExchangeRate();
                
                result.pushKV("ready", ready);
                result.pushKV("reason", ready ? "Sufficient coins for exchange rate measurements" : "Insufficient coins for exchange rate measurements");
                result.pushKV("current_value", FormatMoney(coin_supply));
                result.pushKV("required_value", FormatMoney(required));
                result.pushKV("progress_percent", (double)coin_supply / required * 100.0);
                
            } else {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid measurement type. Must be 'water_price' or 'exchange_rate'");
            }
            
            return result;
        },
    };
}

void RegisterOMeasurementReadinessRPCCommands(CRPCTable &tableRPC)
{
    static const CRPCCommand commands[] = {
        {"measurement_readiness", &updateusercount},
        {"measurement_readiness", &updatecoinsupply},
        {"measurement_readiness", &getreadinessstatus},
        {"measurement_readiness", &getreadinessstatistics},
        {"measurement_readiness", &getreadyforwaterprice},
        {"measurement_readiness", &getreadyforexchangerate},
        {"measurement_readiness", &getfullyreadycurrencies},
        {"measurement_readiness", &checkmeasurementreadiness},
    };
    
    for (const auto& c : commands) {
        tableRPC.appendCommand(c.name, &c);
    }
}
