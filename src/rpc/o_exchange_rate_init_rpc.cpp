// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/o_exchange_rate_init_rpc.h>
#include <consensus/exchange_rate_initialization.h>
#include <rpc/util.h>
#include <util/moneystr.h>
#include <rpc/server.h>

using namespace OConsensus;

static RPCHelpMan initializeexchangerates()
{
    return RPCHelpMan{
        "initializeexchangerates",
        "\nInitialize all O currency exchange rates with theoretical water prices.\n",
        {},
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::NUM, "initialized_count", "Number of exchange rates initialized"},
                {RPCResult::Type::STR, "status", "Initialization status"},
                {RPCResult::Type::ARR, "exchange_rates", "List of initialized exchange rates",
                {
                    {RPCResult::Type::STR, "o_currency", "O currency code"},
                    {RPCResult::Type::STR, "fiat_currency", "Corresponding fiat currency"},
                    {RPCResult::Type::NUM, "rate", "Exchange rate"},
                    {RPCResult::Type::STR, "status", "Rate status (theoretical, theoretical_fallback)"},
                }},
            }
        },
        RPCExamples{
            HelpExampleCli("initializeexchangerates", "")
            + HelpExampleRpc("initializeexchangerates", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            // Initialize exchange rates
            g_exchange_rate_init_manager.InitializeExchangeRates();
            
            // Get all initialized rates
            auto all_rates = g_exchange_rate_init_manager.GetAllExchangeRates();
            auto stats = g_exchange_rate_init_manager.GetExchangeRateStatistics();
            
            UniValue result(UniValue::VOBJ);
            result.pushKV("initialized_count", static_cast<int>(all_rates.size()));
            result.pushKV("status", "success");
            
            UniValue rates_array(UniValue::VARR);
            for (const auto& [rate_key, rate_value] : all_rates) {
                // Parse rate_key (format: "OUSD/USD")
                size_t slash_pos = rate_key.find('/');
                if (slash_pos != std::string::npos) {
                    std::string o_currency = rate_key.substr(0, slash_pos);
                    std::string fiat_currency = rate_key.substr(slash_pos + 1);
                    
                    UniValue rate_obj(UniValue::VOBJ);
                    rate_obj.pushKV("o_currency", o_currency);
                    rate_obj.pushKV("fiat_currency", fiat_currency);
                    rate_obj.pushKV("rate", rate_value);
                    rate_obj.pushKV("status", g_exchange_rate_init_manager.GetExchangeRateStatus(o_currency, fiat_currency));
                    
                    rates_array.push_back(rate_obj);
                }
            }
            
            result.pushKV("exchange_rates", rates_array);
            
            return result;
        },
    };
}

static RPCHelpMan getexchangeratestatus()
{
    return RPCHelpMan{
        "getexchangeratestatus",
        "\nGet exchange rate status for a specific O currency and fiat currency pair.\n",
        {
            {"o_currency", RPCArg::Type::STR, RPCArg::Optional::NO, "O currency code (e.g., 'OUSD')"},
            {"fiat_currency", RPCArg::Type::STR, RPCArg::Optional::NO, "Fiat currency code (e.g., 'USD')"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "o_currency", "O currency code"},
                {RPCResult::Type::STR, "fiat_currency", "Fiat currency code"},
                {RPCResult::Type::NUM, "exchange_rate", "Current exchange rate"},
                {RPCResult::Type::STR, "status", "Rate status (theoretical, measured, insufficient_data, not_initialized)"},
                {RPCResult::Type::NUM, "measurement_count", "Number of measurements received"},
                {RPCResult::Type::NUM, "minimum_required", "Minimum measurements required for measured rate"},
                {RPCResult::Type::BOOL, "has_minimum_measurements", "Whether minimum measurements threshold is met"},
            }
        },
        RPCExamples{
            HelpExampleCli("getexchangeratestatus", "OUSD USD")
            + HelpExampleRpc("getexchangeratestatus", "OUSD USD")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            std::string o_currency = request.params[0].get_str();
            std::string fiat_currency = request.params[1].get_str();
            
            auto rate = g_exchange_rate_init_manager.GetExchangeRate(o_currency, fiat_currency);
            std::string status = g_exchange_rate_init_manager.GetExchangeRateStatus(o_currency, fiat_currency);
            bool has_minimum = g_exchange_rate_init_manager.HasMinimumMeasurements(o_currency, fiat_currency);
            int minimum_required = g_exchange_rate_init_manager.GetMinimumMeasurementsThreshold();
            
            UniValue result(UniValue::VOBJ);
            result.pushKV("o_currency", o_currency);
            result.pushKV("fiat_currency", fiat_currency);
            
            if (rate.has_value()) {
                result.pushKV("exchange_rate", rate.value());
            } else {
                result.pushKV("exchange_rate", "not_available");
            }
            
            result.pushKV("status", status);
            result.pushKV("measurement_count", g_exchange_rate_init_manager.GetMeasurementCount(o_currency, fiat_currency));
            result.pushKV("minimum_required", minimum_required);
            result.pushKV("has_minimum_measurements", has_minimum);
            result.pushKV("measurement_trend", g_exchange_rate_init_manager.GetMeasurementTrend(o_currency, fiat_currency));
            result.pushKV("is_disappearing", g_exchange_rate_init_manager.DetectCurrencyDisappearance(o_currency, fiat_currency));
            
            return result;
        },
    };
}

static RPCHelpMan getexchangeratestatistics()
{
    return RPCHelpMan{
        "getexchangeratestatistics",
        "\nGet exchange rate initialization statistics.\n",
        {},
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::NUM, "total_rates", "Total number of exchange rates"},
                {RPCResult::Type::NUM, "theoretical_rates", "Number of rates using theoretical values"},
                {RPCResult::Type::NUM, "measured_rates", "Number of rates using measured values"},
                {RPCResult::Type::NUM, "insufficient_data_rates", "Number of rates with insufficient data"},
                {RPCResult::Type::NUM, "minimum_measurements_threshold", "Minimum measurements required for measured rates"},
            }
        },
        RPCExamples{
            HelpExampleCli("getexchangeratestatistics", "")
            + HelpExampleRpc("getexchangeratestatistics", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            auto stats = g_exchange_rate_init_manager.GetExchangeRateStatistics();
            
            UniValue result(UniValue::VOBJ);
            result.pushKV("total_rates", stats["total"]);
            result.pushKV("theoretical_rates", stats["theoretical"]);
            result.pushKV("measured_rates", stats["measured"]);
            result.pushKV("insufficient_data_rates", stats["insufficient_data"]);
            result.pushKV("minimum_measurements_threshold", g_exchange_rate_init_manager.GetMinimumMeasurementsThreshold());
            
            return result;
        },
    };
}

static RPCHelpMan getminimummeasurementsthreshold()
{
    return RPCHelpMan{
        "getminimummeasurementsthreshold",
        "\nGet the minimum number of measurements required before switching from theoretical to measured exchange rates.\n",
        {},
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::NUM, "minimum_measurements", "Minimum measurements required"},
                {RPCResult::Type::STR, "description", "Description of the threshold"},
            }
        },
        RPCExamples{
            HelpExampleCli("getminimummeasurementsthreshold", "")
            + HelpExampleRpc("getminimummeasurementsthreshold", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            int threshold = g_exchange_rate_init_manager.GetMinimumMeasurementsThreshold();
            
            UniValue result(UniValue::VOBJ);
            result.pushKV("minimum_measurements", threshold);
            result.pushKV("description", "Minimum number of exchange rate measurements required before switching from theoretical (water price based) to measured exchange rates");
            
            return result;
        },
    };
}

static RPCHelpMan detectcurrencydisappearance()
{
    return RPCHelpMan{
        "detectcurrencydisappearance",
        "\nDetect if a currency is disappearing based on progressive decrease in measurements.\n",
        {
            {"o_currency", RPCArg::Type::STR, RPCArg::Optional::NO, "O currency code (e.g., 'OUSD')"},
            {"fiat_currency", RPCArg::Type::STR, RPCArg::Optional::NO, "Fiat currency code (e.g., 'USD')"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "o_currency", "O currency code"},
                {RPCResult::Type::STR, "fiat_currency", "Fiat currency code"},
                {RPCResult::Type::BOOL, "is_disappearing", "Whether currency is detected as disappearing"},
                {RPCResult::Type::STR, "reason", "Reason for disappearance detection"},
                {RPCResult::Type::NUM, "days_since_last_measurement", "Days since last measurement"},
                {RPCResult::Type::NUM, "total_measurements", "Total number of measurements"},
            }
        },
        RPCExamples{
            HelpExampleCli("detectcurrencydisappearance", "OUSD USD")
            + HelpExampleRpc("detectcurrencydisappearance", "OUSD USD")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            std::string o_currency = request.params[0].get_str();
            std::string fiat_currency = request.params[1].get_str();
            
            bool is_disappearing = g_exchange_rate_init_manager.DetectCurrencyDisappearance(o_currency, fiat_currency);
            std::string trend = g_exchange_rate_init_manager.GetMeasurementTrend(o_currency, fiat_currency);
            int measurement_count = g_exchange_rate_init_manager.GetMeasurementCount(o_currency, fiat_currency);
            
            UniValue result(UniValue::VOBJ);
            result.pushKV("o_currency", o_currency);
            result.pushKV("fiat_currency", fiat_currency);
            result.pushKV("is_disappearing", is_disappearing);
            result.pushKV("reason", is_disappearing ? "Progressive decrease in measurements detected" : "Currency is active");
            result.pushKV("measurement_trend", trend);
            result.pushKV("total_measurements", measurement_count);
            
            return result;
        },
    };
}

static RPCHelpMan getmeasurementtrend()
{
    return RPCHelpMan{
        "getmeasurementtrend",
        "\nGet measurement trend for a currency pair.\n",
        {
            {"o_currency", RPCArg::Type::STR, RPCArg::Optional::NO, "O currency code (e.g., 'OUSD')"},
            {"fiat_currency", RPCArg::Type::STR, RPCArg::Optional::NO, "Fiat currency code (e.g., 'USD')"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "o_currency", "O currency code"},
                {RPCResult::Type::STR, "fiat_currency", "Fiat currency code"},
                {RPCResult::Type::STR, "trend", "Measurement trend (active, stable, decreasing, disappearing, no_measurements, insufficient_data)"},
                {RPCResult::Type::NUM, "total_measurements", "Total number of measurements"},
                {RPCResult::Type::BOOL, "is_disappearing", "Whether currency is detected as disappearing"},
            }
        },
        RPCExamples{
            HelpExampleCli("getmeasurementtrend", "OUSD USD")
            + HelpExampleRpc("getmeasurementtrend", "OUSD USD")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            std::string o_currency = request.params[0].get_str();
            std::string fiat_currency = request.params[1].get_str();
            
            std::string trend = g_exchange_rate_init_manager.GetMeasurementTrend(o_currency, fiat_currency);
            bool is_disappearing = g_exchange_rate_init_manager.DetectCurrencyDisappearance(o_currency, fiat_currency);
            int measurement_count = g_exchange_rate_init_manager.GetMeasurementCount(o_currency, fiat_currency);
            
            UniValue result(UniValue::VOBJ);
            result.pushKV("o_currency", o_currency);
            result.pushKV("fiat_currency", fiat_currency);
            result.pushKV("trend", trend);
            result.pushKV("total_measurements", measurement_count);
            result.pushKV("is_disappearing", is_disappearing);
            
            return result;
        },
    };
}

void RegisterOExchangeRateInitRPCCommands(CRPCTable& t)
{
    static const CRPCCommand commands[] = {
        {"exchange_rate_init", &initializeexchangerates},
        {"exchange_rate_init", &getexchangeratestatus},
        {"exchange_rate_init", &getexchangeratestatistics},
        {"exchange_rate_init", &getminimummeasurementsthreshold},
        {"exchange_rate_init", &detectcurrencydisappearance},
        {"exchange_rate_init", &getmeasurementtrend},
    };
    
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}
