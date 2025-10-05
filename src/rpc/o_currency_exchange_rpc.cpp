// Copyright (c) 2024 The Bitcoin Core developers
// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/o_currency_exchange_rpc.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <univalue.h>
#include <consensus/currency_exchange.h>
#include <consensus/o_amount.h>
#include <pubkey.h>
#include <key_io.h>
#include <random.h>
#include <util/strencodings.h>

#include <string>
#include <vector>

static RPCHelpMan exchangecurrency()
{
    return RPCHelpMan{
        "exchangecurrency",
        "\nExchange one O fiat currency for another using blockchain-defined rates.\n",
        {
            {"from_currency", RPCArg::Type::STR, RPCArg::Optional::NO, "Source currency code (e.g., 'OUSD')"},
            {"to_currency", RPCArg::Type::STR, RPCArg::Optional::NO, "Target currency code (e.g., 'OEUR')"},
            {"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "Amount to exchange (in source currency)"},
            {"to_address", RPCArg::Type::STR, RPCArg::Optional::NO, "Recipient address"},
            {"memo", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Optional memo for the exchange"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "exchange_id", "Exchange transaction ID"},
                {RPCResult::Type::STR, "from_currency", "Source currency"},
                {RPCResult::Type::STR, "to_currency", "Target currency"},
                {RPCResult::Type::STR_AMOUNT, "from_amount", "Amount in source currency"},
                {RPCResult::Type::STR_AMOUNT, "to_amount", "Amount in target currency"},
                {RPCResult::Type::NUM, "exchange_rate", "Exchange rate used"},
                {RPCResult::Type::STR, "status", "Exchange status"},
                {RPCResult::Type::STR, "memo", "Exchange memo"},
            }
        },
        RPCExamples{
            HelpExampleCli("exchangecurrency", "\"OUSD\" \"OEUR\" 100.00 \"bc1q...\" \"Payment for services\"")
            + HelpExampleRpc("exchangecurrency", "\"OUSD\", \"OEUR\", 100.00, \"bc1q...\", \"Payment for services\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            std::string from_currency = request.params[0].get_str();
            std::string to_currency = request.params[1].get_str();
            CAmount amount = request.params[2].getInt<CAmount>();
            std::string to_address = request.params[3].get_str();
            std::string memo = request.params.size() > 4 ? request.params[4].get_str() : "";
            
            // Validate inputs
            if (from_currency.empty() || to_currency.empty()) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Currency codes cannot be empty");
            }
            
            if (from_currency == to_currency) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot exchange same currency");
            }
            
            if (amount <= 0) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Amount must be positive");
            }
            
            // Decode recipient address
            CTxDestination dest = DecodeDestination(to_address);
            if (!IsValidDestination(dest)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid recipient address");
            }
            
            CPubKey to_pubkey;
            // For now, we'll use a placeholder pubkey
            // In a real implementation, you'd derive this from the address
            to_pubkey = CPubKey(); // TODO: Implement proper pubkey derivation
            
            // Get sender pubkey (placeholder for now)
            CPubKey from_pubkey; // TODO: Get from wallet or user context
            
            // Create exchange
            auto exchange = g_currency_exchange_manager.CreateExchange(
                from_pubkey, to_pubkey, from_currency, to_currency, amount, memo);
            
            if (!exchange.has_value()) {
                throw JSONRPCError(RPC_INTERNAL_ERROR, "Failed to create exchange");
            }
            
            // Return exchange details
            UniValue result(UniValue::VOBJ);
            result.pushKV("exchange_id", exchange->exchange_id.ToString());
            result.pushKV("from_currency", exchange->from_currency);
            result.pushKV("to_currency", exchange->to_currency);
            result.pushKV("from_amount", OAmount::FormatO(exchange->from_amount));
            result.pushKV("to_amount", OAmount::FormatO(exchange->to_amount));
            result.pushKV("exchange_rate", exchange->exchange_rate);
            result.pushKV("status", exchange->is_executed ? "executed" : "pending");
            result.pushKV("memo", exchange->memo);
            
            return result;
        },
    };
}

static RPCHelpMan getexchangerate()
{
    return RPCHelpMan{
        "getexchangerate",
        "\nGet current exchange rate between two O fiat currencies.\n",
        {
            {"from_currency", RPCArg::Type::STR, RPCArg::Optional::NO, "Source currency code"},
            {"to_currency", RPCArg::Type::STR, RPCArg::Optional::NO, "Target currency code"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "from_currency", "Source currency"},
                {RPCResult::Type::STR, "to_currency", "Target currency"},
                {RPCResult::Type::NUM, "exchange_rate", "Current exchange rate"},
                {RPCResult::Type::NUM, "last_updated", "Last update timestamp"},
                {RPCResult::Type::BOOL, "is_valid", "Whether rate is valid"},
                {RPCResult::Type::NUM, "deviation_threshold", "Maximum allowed deviation"},
            }
        },
        RPCExamples{
            HelpExampleCli("getexchangerate", "\"OUSD\" \"OEUR\"")
            + HelpExampleRpc("getexchangerate", "\"OUSD\", \"OEUR\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            std::string from_currency = request.params[0].get_str();
            std::string to_currency = request.params[1].get_str();
            
            auto rate = g_currency_exchange_manager.GetCurrentExchangeRate(from_currency, to_currency);
            
            UniValue result(UniValue::VOBJ);
            result.pushKV("from_currency", from_currency);
            result.pushKV("to_currency", to_currency);
            
            if (rate.has_value()) {
                result.pushKV("exchange_rate", rate.value());
                result.pushKV("is_valid", true);
            } else {
                result.pushKV("exchange_rate", 0.0);
                result.pushKV("is_valid", false);
            }
            
            result.pushKV("deviation_threshold", ExchangeConfig::MAX_RATE_DEVIATION);
            
            return result;
        },
    };
}

static RPCHelpMan estimateexchange()
{
    return RPCHelpMan{
        "estimateexchange",
        "\nEstimate the result of a currency exchange without executing it.\n",
        {
            {"from_currency", RPCArg::Type::STR, RPCArg::Optional::NO, "Source currency code"},
            {"to_currency", RPCArg::Type::STR, RPCArg::Optional::NO, "Target currency code"},
            {"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "Amount to exchange"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "from_currency", "Source currency"},
                {RPCResult::Type::STR, "to_currency", "Target currency"},
                {RPCResult::Type::STR_AMOUNT, "from_amount", "Input amount"},
                {RPCResult::Type::STR_AMOUNT, "to_amount", "Estimated output amount"},
                {RPCResult::Type::NUM, "exchange_rate", "Exchange rate used"},
                {RPCResult::Type::BOOL, "is_valid", "Whether exchange is possible"},
                {RPCResult::Type::STR, "status", "Status message"},
            }
        },
        RPCExamples{
            HelpExampleCli("estimateexchange", "\"OUSD\" \"OEUR\" 100.00")
            + HelpExampleRpc("estimateexchange", "\"OUSD\", \"OEUR\", 100.00")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            std::string from_currency = request.params[0].get_str();
            std::string to_currency = request.params[1].get_str();
            CAmount amount = request.params[2].getInt<CAmount>();
            
            auto rate = g_currency_exchange_manager.GetCurrentExchangeRate(from_currency, to_currency);
            
            UniValue result(UniValue::VOBJ);
            result.pushKV("from_currency", from_currency);
            result.pushKV("to_currency", to_currency);
            result.pushKV("from_amount", OAmount::FormatO(amount));
            
            if (rate.has_value()) {
                CAmount to_amount = g_currency_exchange_manager.CalculateExchangeAmount(amount, rate.value());
                result.pushKV("to_amount", OAmount::FormatO(to_amount));
                result.pushKV("exchange_rate", rate.value());
                result.pushKV("is_valid", true);
                result.pushKV("status", "Exchange possible");
            } else {
                result.pushKV("to_amount", "0.00 O");
                result.pushKV("exchange_rate", 0.0);
                result.pushKV("is_valid", false);
                result.pushKV("status", "No exchange rate available");
            }
            
            return result;
        },
    };
}

static RPCHelpMan validateexchangerate()
{
    return RPCHelpMan{
        "validateexchangerate",
        "\nValidate if a proposed exchange rate is within acceptable deviation limits.\n",
        {
            {"from_currency", RPCArg::Type::STR, RPCArg::Optional::NO, "Source currency code"},
            {"to_currency", RPCArg::Type::STR, RPCArg::Optional::NO, "Target currency code"},
            {"proposed_rate", RPCArg::Type::NUM, RPCArg::Optional::NO, "Proposed exchange rate"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "from_currency", "Source currency"},
                {RPCResult::Type::STR, "to_currency", "Target currency"},
                {RPCResult::Type::NUM, "proposed_rate", "Proposed rate"},
                {RPCResult::Type::NUM, "measured_rate", "Measured rate from blockchain"},
                {RPCResult::Type::NUM, "deviation", "Deviation percentage"},
                {RPCResult::Type::NUM, "max_deviation", "Maximum allowed deviation"},
                {RPCResult::Type::BOOL, "is_valid", "Whether rate is valid"},
                {RPCResult::Type::STR, "status", "Validation status"},
            }
        },
        RPCExamples{
            HelpExampleCli("validateexchangerate", "\"OUSD\" \"OEUR\" 0.85")
            + HelpExampleRpc("validateexchangerate", "\"OUSD\", \"OEUR\", 0.85")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            std::string from_currency = request.params[0].get_str();
            std::string to_currency = request.params[1].get_str();
            double proposed_rate = request.params[2].get_real();
            
            bool is_valid = g_currency_exchange_manager.ValidateExchangeRate(
                from_currency, to_currency, proposed_rate);
            
            auto measured_rate = g_currency_exchange_manager.GetCurrentExchangeRate(
                from_currency, to_currency);
            
            double deviation = g_currency_exchange_manager.GetRateDeviation(
                from_currency, to_currency, proposed_rate);
            
            UniValue result(UniValue::VOBJ);
            result.pushKV("from_currency", from_currency);
            result.pushKV("to_currency", to_currency);
            result.pushKV("proposed_rate", proposed_rate);
            result.pushKV("measured_rate", measured_rate.value_or(0.0));
            result.pushKV("deviation", deviation);
            result.pushKV("max_deviation", ExchangeConfig::MAX_RATE_DEVIATION);
            result.pushKV("is_valid", is_valid);
            result.pushKV("status", is_valid ? "Rate is valid" : "Rate deviation too high");
            
            return result;
        },
    };
}

static RPCHelpMan getexchangehistory()
{
    return RPCHelpMan{
        "getexchangehistory",
        "\nGet exchange history for a user or time range.\n",
        {
            {"user_address", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "User address (optional)"},
            {"days", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "Number of days to look back (default: 30)"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::NUM, "total_exchanges", "Total number of exchanges"},
                {RPCResult::Type::ARR, "exchanges", "List of exchanges",
                {
                    {RPCResult::Type::OBJ, "", "Exchange details",
                    {
                        {RPCResult::Type::STR, "exchange_id", "Exchange ID"},
                        {RPCResult::Type::STR, "from_currency", "Source currency"},
                        {RPCResult::Type::STR, "to_currency", "Target currency"},
                        {RPCResult::Type::STR_AMOUNT, "from_amount", "Source amount"},
                        {RPCResult::Type::STR_AMOUNT, "to_amount", "Target amount"},
                        {RPCResult::Type::NUM, "exchange_rate", "Exchange rate"},
                        {RPCResult::Type::NUM, "timestamp", "Exchange timestamp"},
                        {RPCResult::Type::STR, "status", "Exchange status"},
                    }},
                }},
            }
        },
        RPCExamples{
            HelpExampleCli("getexchangehistory", "")
            + HelpExampleCli("getexchangehistory", "\"bc1q...\" 7")
            + HelpExampleRpc("getexchangehistory", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            int days = 30;
            if (request.params.size() > 1) {
                days = request.params[1].getInt<int>();
            }
            
            int64_t current_time = GetTime();
            int64_t start_time = current_time - (days * 24 * 3600);
            
            auto exchanges = g_currency_exchange_manager.GetExchangesInRange(start_time, current_time);
            
            UniValue result(UniValue::VOBJ);
            result.pushKV("total_exchanges", static_cast<int64_t>(exchanges.size()));
            
            UniValue exchanges_array(UniValue::VARR);
            for (const auto& exchange : exchanges) {
                UniValue exchange_obj(UniValue::VOBJ);
                exchange_obj.pushKV("exchange_id", exchange.exchange_id.ToString());
                exchange_obj.pushKV("from_currency", exchange.from_currency);
                exchange_obj.pushKV("to_currency", exchange.to_currency);
                exchange_obj.pushKV("from_amount", OAmount::FormatO(exchange.from_amount));
                exchange_obj.pushKV("to_amount", OAmount::FormatO(exchange.to_amount));
                exchange_obj.pushKV("exchange_rate", exchange.exchange_rate);
                exchange_obj.pushKV("timestamp", exchange.timestamp);
                exchange_obj.pushKV("status", exchange.is_executed ? "executed" : "pending");
                exchanges_array.push_back(exchange_obj);
            }
            
            result.pushKV("exchanges", exchanges_array);
            
            return result;
        },
    };
}

static RPCHelpMan getexchangestatistics()
{
    return RPCHelpMan{
        "getexchangestatistics",
        "\nGet currency exchange statistics.\n",
        {},
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::NUM, "total_exchanges", "Total number of exchanges"},
                {RPCResult::Type::STR_AMOUNT, "total_volume", "Total exchange volume"},
                {RPCResult::Type::OBJ, "volume_by_pair", "Volume by currency pair"},
                {RPCResult::Type::NUM, "supported_pairs", "Number of supported currency pairs"},
            }
        },
        RPCExamples{
            HelpExampleCli("getexchangestatistics", "")
            + HelpExampleRpc("getexchangestatistics", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            auto stats = g_currency_exchange_manager.GetExchangeStatistics();
            auto volume_by_pair = g_currency_exchange_manager.GetExchangeVolumeByPair();
            auto supported_pairs = g_currency_exchange_manager.GetSupportedPairs();
            
            UniValue result(UniValue::VOBJ);
            result.pushKV("total_exchanges", stats["total_exchanges"]);
            result.pushKV("total_volume", OAmount::FormatO(stats["total_volume"]));
            
            UniValue volume_obj(UniValue::VOBJ);
            for (const auto& [pair, volume] : volume_by_pair) {
                volume_obj.pushKV(pair, OAmount::FormatO(volume));
            }
            result.pushKV("volume_by_pair", volume_obj);
            
            result.pushKV("supported_pairs", static_cast<int64_t>(supported_pairs.size()));
            
            return result;
        },
    };
}

static RPCHelpMan getsupportedpairs()
{
    return RPCHelpMan{
        "getsupportedpairs",
        "\nGet list of supported currency exchange pairs.\n",
        {},
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::NUM, "total_pairs", "Total number of supported pairs"},
                {RPCResult::Type::ARR, "pairs", "List of supported currency pairs",
                {
                    {RPCResult::Type::OBJ, "", "Currency pair",
                    {
                        {RPCResult::Type::STR, "from", "Source currency"},
                        {RPCResult::Type::STR, "to", "Target currency"},
                    }},
                }},
            }
        },
        RPCExamples{
            HelpExampleCli("getsupportedpairs", "")
            + HelpExampleRpc("getsupportedpairs", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            auto pairs = g_currency_exchange_manager.GetSupportedPairs();
            
            UniValue result(UniValue::VOBJ);
            result.pushKV("total_pairs", static_cast<int64_t>(pairs.size()));
            
            UniValue pairs_array(UniValue::VARR);
            for (const auto& [from, to] : pairs) {
                UniValue pair_obj(UniValue::VOBJ);
                pair_obj.pushKV("from", from);
                pair_obj.pushKV("to", to);
                pairs_array.push_back(pair_obj);
            }
            
            result.pushKV("pairs", pairs_array);
            
            return result;
        },
    };
}

static RPCHelpMan getcrossocurrencyrate()
{
    return RPCHelpMan{
        "getcrossocurrencyrate",
        "\nCalculate exchange rate between two O currencies via fiat currency bridge.\n",
        {
            {"from_o_currency", RPCArg::Type::STR, RPCArg::Optional::NO, "Source O currency code (e.g., 'OUSD')"},
            {"to_o_currency", RPCArg::Type::STR, RPCArg::Optional::NO, "Target O currency code (e.g., 'OEUR')"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "from_currency", "Source O currency"},
                {RPCResult::Type::STR, "to_currency", "Target O currency"},
                {RPCResult::Type::NUM, "exchange_rate", "Calculated exchange rate"},
                {RPCResult::Type::STR, "calculation_method", "Method used for calculation"},
                {RPCResult::Type::OBJ, "components", "Rate components used in calculation",
                    {
                        {RPCResult::Type::NUM, "from_o_to_fiat_rate", "O currency to fiat rate"},
                        {RPCResult::Type::NUM, "fiat_exchange_rate", "Fiat currency exchange rate"},
                        {RPCResult::Type::NUM, "to_o_to_fiat_rate", "Target O currency to fiat rate"},
                        {RPCResult::Type::STR, "from_fiat", "Source fiat currency"},
                        {RPCResult::Type::STR, "to_fiat", "Target fiat currency"},
                    }
                },
            }
        },
        RPCExamples{
            HelpExampleCli("getcrossocurrencyrate", "\"OUSD\" \"OEUR\"")
            + HelpExampleRpc("getcrossocurrencyrate", "\"OUSD\", \"OEUR\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            std::string from_currency = request.params[0].get_str();
            std::string to_currency = request.params[1].get_str();
            
            auto rate = g_currency_exchange_manager.CalculateOCurrencyExchangeRate(from_currency, to_currency);
            
            UniValue result(UniValue::VOBJ);
            result.pushKV("from_currency", from_currency);
            result.pushKV("to_currency", to_currency);
            
            if (rate.has_value()) {
                result.pushKV("exchange_rate", rate.value());
                result.pushKV("calculation_method", "Cross-O currency via fiat bridge");
                
                // Add calculation components
                UniValue components(UniValue::VOBJ);
                std::string from_fiat = g_currency_exchange_manager.GetCorrespondingFiatCurrency(from_currency);
                std::string to_fiat = g_currency_exchange_manager.GetCorrespondingFiatCurrency(to_currency);
                
                components.pushKV("from_fiat", from_fiat);
                components.pushKV("to_fiat", to_fiat);
                result.pushKV("components", components);
            } else {
                result.pushKV("exchange_rate", UniValue::VNULL);
                result.pushKV("error", "Unable to calculate cross-O currency exchange rate");
            }
            
            return result;
        }
    };
}

void RegisterOCurrencyExchangeRPCCommands(CRPCTable& t)
{
    static const CRPCCommand commands[] = {
        {"exchange", &exchangecurrency},
        {"exchange", &getexchangerate},
        {"exchange", &estimateexchange},
        {"exchange", &validateexchangerate},
        {"exchange", &getexchangehistory},
        {"exchange", &getexchangestatistics},
        {"exchange", &getsupportedpairs},
        {"exchange", &getcrossocurrencyrate},
    };
    
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}
