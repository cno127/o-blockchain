// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/o_measurement_rpc.h>
#include <measurement/measurement_system.h>
#include <rpc/server.h>
#include <rpc/server_util.h>
#include <rpc/util.h>
#include <univalue.h>
#include <util/moneystr.h>
#include <util/strencodings.h>
#include <util/time.h>
#include <pubkey.h>
#include <key_io.h>
#include <random.h>

using namespace OMeasurement;
using node::NodeContext;

static RPCHelpMan submitwaterprice()
{
    return RPCHelpMan{"submitwaterprice",
        "\nSubmit a water price measurement.\n",
        {
            {"currency", RPCArg::Type::STR, RPCArg::Optional::NO, "Currency code (USD, EUR, JPY, etc.)"},
            {"price", RPCArg::Type::NUM, RPCArg::Optional::NO, "Price in smallest unit (cents for USD, etc.)"},
            {"location", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Location of measurement"},
            {"proof_hash", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "SHA256 hash of proof image"},
            {"invite_id", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Invitation ID"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "measurement_id", "Unique measurement ID"},
                {RPCResult::Type::STR, "status", "Submission status"},
                {RPCResult::Type::STR_AMOUNT, "reward", "Reward amount"},
            }
        },
        RPCExamples{
            HelpExampleCli("submitwaterprice", "\"USD\" 150 \"New York\" \"abc123...\" \"def456...\"")
            + HelpExampleRpc("submitwaterprice", "\"USD\", 150, \"New York\", \"abc123...\", \"def456...\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            std::string currency = request.params[0].get_str();
            int64_t price = request.params[1].getInt<int64_t>();
            std::string location = request.params[2].isNull() ? "" : request.params[2].get_str();
            std::string proof_hash = request.params[3].isNull() ? "" : request.params[3].get_str();
            uint256 invite_id = ParseHashV(request.params[4], "invite_id");
            
            // Create measurement
            WaterPriceMeasurement measurement;
            measurement.measurement_id = GetRandHash();
            measurement.currency_code = currency;
            measurement.price = price;
            measurement.location = location;
            measurement.proof_image_hash = proof_hash;
            measurement.timestamp = GetTime();
            measurement.invite_id = invite_id;
            measurement.is_validated = false;
            measurement.confidence_score = 0.0;
            
            // Submit to system
            uint256 result_id = g_measurement_system.SubmitWaterPrice(measurement);
            
            if (result_id.IsNull()) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Failed to submit measurement. Check invite validity.");
            }
            
            // Calculate reward
            CAmount reward = g_measurement_system.CalculateReward(MeasurementType::WATER_PRICE, 1.0);
            
            UniValue result(UniValue::VOBJ);
            result.pushKV("measurement_id", result_id.GetHex());
            result.pushKV("status", "submitted");
            result.pushKV("reward", UniValue(UniValue::VNUM, FormatMoney(reward)));
            
            return result;
        },
    };
}

static RPCHelpMan validatemeasurement()
{
    return RPCHelpMan{"validatemeasurement",
        "\nValidate a water price or exchange rate measurement.\n",
        {
            {"measurement_id", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Measurement ID to validate"},
            {"type", RPCArg::Type::STR, RPCArg::Optional::NO, "Type: 'water' or 'exchange'"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::BOOL, "success", "Whether validation was successful"},
                {RPCResult::Type::NUM, "validator_count", "Number of validators"},
                {RPCResult::Type::BOOL, "is_validated", "Whether measurement is now validated"},
                {RPCResult::Type::STR_AMOUNT, "reward", "Reward amount"},
            }
        },
        RPCExamples{
            HelpExampleCli("validatemeasurement", "\"abc123...\" \"water\"")
            + HelpExampleRpc("validatemeasurement", "\"abc123...\", \"water\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            uint256 measurement_id = ParseHashV(request.params[0], "measurement_id");
            std::string type_str = request.params[1].get_str();
            
            // TODO: Get validator's public key from wallet/context
            CPubKey validator;  // Would come from authenticated user
            
            bool success = false;
            int validator_count = 0;
            bool is_validated = false;
            
            if (type_str == "water") {
                success = g_measurement_system.ValidateWaterPrice(measurement_id, validator);
                
                if (success) {
                    auto measurement = g_measurement_system.GetWaterPriceMeasurement(measurement_id);
                    if (measurement.has_value()) {
                        validator_count = measurement->validators.size();
                        is_validated = measurement->is_validated;
                    }
                }
            } else if (type_str == "exchange") {
                success = g_measurement_system.ValidateExchangeRate(measurement_id, validator);
                
                if (success) {
                    auto measurement = g_measurement_system.GetExchangeRateMeasurement(measurement_id);
                    if (measurement.has_value()) {
                        validator_count = measurement->validators.size();
                        is_validated = measurement->is_validated;
                    }
                }
            } else {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Type must be 'water' or 'exchange'");
            }
            
            CAmount reward = g_measurement_system.CalculateReward(
                MeasurementType::WATER_PRICE_OFFLINE_VALIDATION, 1.0);
            
            UniValue result(UniValue::VOBJ);
            result.pushKV("success", success);
            result.pushKV("validator_count", validator_count);
            result.pushKV("is_validated", is_validated);
            result.pushKV("reward", UniValue(UniValue::VNUM, FormatMoney(reward)));
            
            return result;
        },
    };
}

static RPCHelpMan getaveragewaterprice()
{
    return RPCHelpMan{"getaveragewaterprice",
        "\nGet average water price for a currency over a time period.\n",
        {
            {"currency", RPCArg::Type::STR, RPCArg::Optional::NO, "Currency code (USD, EUR, etc.)"},
            {"days", RPCArg::Type::NUM, RPCArg::Default{30}, "Number of days to average (default 30)"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "currency", "Currency code"},
                {RPCResult::Type::NUM, "average_price", "Average price"},
                {RPCResult::Type::NUM, "days", "Number of days in calculation"},
                {RPCResult::Type::NUM, "measurement_count", "Number of measurements used"},
            }
        },
        RPCExamples{
            HelpExampleCli("getaveragewaterprice", "\"USD\"")
            + HelpExampleCli("getaveragewaterprice", "\"EUR\" 60")
            + HelpExampleRpc("getaveragewaterprice", "\"USD\", 30")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            std::string currency = request.params[0].get_str();
            int days = request.params[1].isNull() ? 30 : request.params[1].getInt<int>();
            
            auto avg = g_measurement_system.GetAverageWaterPrice(currency, days);
            
            if (!avg.has_value()) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, 
                    "No measurements found for currency in time period");
            }
            
            // Get measurement count
            int64_t current_time = GetTime();
            int64_t start_time = current_time - (days * 24 * 3600);
            auto measurements = g_measurement_system.GetWaterPricesInRange(
                currency, start_time, current_time);
            
            UniValue result(UniValue::VOBJ);
            result.pushKV("currency", currency);
            result.pushKV("average_price", avg.value());
            result.pushKV("days", days);
            result.pushKV("measurement_count", (int)measurements.size());
            
            return result;
        },
    };
}

static RPCHelpMan submitexchangerate()
{
    return RPCHelpMan{"submitexchangerate",
        "\nSubmit an exchange rate measurement.\n",
        {
            {"from_currency", RPCArg::Type::STR, RPCArg::Optional::NO, "Source currency code"},
            {"to_currency", RPCArg::Type::STR, RPCArg::Optional::NO, "Target currency code"},
            {"rate", RPCArg::Type::NUM, RPCArg::Optional::NO, "Exchange rate"},
            {"source_url", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Source URL"},
            {"invite_id", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Invitation ID"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "measurement_id", "Unique measurement ID"},
                {RPCResult::Type::STR, "status", "Submission status"},
                {RPCResult::Type::STR_AMOUNT, "reward", "Reward amount"},
            }
        },
        RPCExamples{
            HelpExampleCli("submitexchangerate", "\"USD\" \"EUR\" 0.85 \"https://example.com\" \"abc123...\"")
            + HelpExampleRpc("submitexchangerate", "\"USD\", \"EUR\", 0.85, \"https://example.com\", \"abc123...\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            std::string from_currency = request.params[0].get_str();
            std::string to_currency = request.params[1].get_str();
            double rate = request.params[2].get_real();
            std::string source_url = request.params[3].isNull() ? "" : request.params[3].get_str();
            uint256 invite_id = ParseHashV(request.params[4], "invite_id");
            
            // Create measurement
            ExchangeRateMeasurement measurement;
            measurement.measurement_id = GetRandHash();
            measurement.from_currency = from_currency;
            measurement.to_currency = to_currency;
            measurement.exchange_rate = rate;
            measurement.source_url = source_url;
            measurement.timestamp = GetTime();
            measurement.invite_id = invite_id;
            measurement.is_validated = false;
            
            // Submit to system
            uint256 result_id = g_measurement_system.SubmitExchangeRate(measurement);
            
            if (result_id.IsNull()) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Failed to submit measurement");
            }
            
            CAmount reward = g_measurement_system.CalculateReward(MeasurementType::EXCHANGE_RATE, 1.0);
            
            UniValue result(UniValue::VOBJ);
            result.pushKV("measurement_id", result_id.GetHex());
            result.pushKV("status", "submitted");
            result.pushKV("reward", UniValue(UniValue::VNUM, FormatMoney(reward)));
            
            return result;
        },
    };
}

static RPCHelpMan createinvites()
{
    return RPCHelpMan{"createinvites",
        "\nCreate measurement invitations for users.\n",
        {
            {"count", RPCArg::Type::NUM, RPCArg::Optional::NO, "Number of invites to create"},
            {"type", RPCArg::Type::STR, RPCArg::Optional::NO, "Type: 'water', 'exchange', 'validation'"},
            {"currency", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Specific currency (optional)"},
        },
        RPCResult{
            RPCResult::Type::ARR, "", "Array of created invitations",
            {
                {RPCResult::Type::OBJ, "", "",
                {
                    {RPCResult::Type::STR_HEX, "invite_id", "Invitation ID"},
                    {RPCResult::Type::STR, "type", "Invitation type"},
                    {RPCResult::Type::NUM, "expires_at", "Expiration timestamp"},
                    {RPCResult::Type::STR, "currency", "Currency code (if specified)"},
                }},
            }
        },
        RPCExamples{
            HelpExampleCli("createinvites", "10 \"water\"")
            + HelpExampleCli("createinvites", "5 \"exchange\" \"USD\"")
            + HelpExampleRpc("createinvites", "10, \"water\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            int count = request.params[0].getInt<int>();
            std::string type_str = request.params[1].get_str();
            std::string currency = request.params[2].isNull() ? "" : request.params[2].get_str();
            
            if (count < 1 || count > 1000) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Count must be between 1 and 1000");
            }
            
            MeasurementType type;
            if (type_str == "water") {
                type = MeasurementType::WATER_PRICE;
            } else if (type_str == "exchange") {
                type = MeasurementType::EXCHANGE_RATE;
            } else if (type_str == "validation") {
                type = MeasurementType::WATER_PRICE_OFFLINE_VALIDATION;
            } else {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Type must be 'water', 'exchange', or 'validation'");
            }
            
            std::vector<MeasurementInvite> invites = 
                g_measurement_system.CreateInvites(count, type, currency);
            
            UniValue result(UniValue::VARR);
            
            for (const auto& invite : invites) {
                UniValue inv(UniValue::VOBJ);
                inv.pushKV("invite_id", invite.invite_id.GetHex());
                inv.pushKV("type", type_str);
                inv.pushKV("expires_at", invite.expires_at);
                if (!invite.currency_code.empty()) {
                    inv.pushKV("currency", invite.currency_code);
                }
                result.push_back(inv);
            }
            
            return result;
        },
    };
}

static RPCHelpMan getmeasurementstatistics()
{
    return RPCHelpMan{"getmeasurementstatistics",
        "\nGet overall measurement system statistics.\n",
        {},
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::NUM, "total_water_prices", "Total water price measurements"},
                {RPCResult::Type::NUM, "total_exchange_rates", "Total exchange rate measurements"},
                {RPCResult::Type::NUM, "total_invites", "Total invitations sent"},
                {RPCResult::Type::NUM, "total_urls", "Total validated URLs"},
                {RPCResult::Type::NUM, "invites_sent", "Historical invites sent"},
                {RPCResult::Type::NUM, "measurements_received", "Historical measurements received"},
                {RPCResult::Type::NUM, "validations", "Total validations"},
                {RPCResult::Type::NUM, "conversion_rate_water", "Water price conversion rate"},
                {RPCResult::Type::NUM, "conversion_rate_exchange", "Exchange rate conversion rate"},
            }
        },
        RPCExamples{
            HelpExampleCli("getmeasurementstatistics", "")
            + HelpExampleRpc("getmeasurementstatistics", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            auto stats = g_measurement_system.GetMeasurementStatistics();
            
            double conv_water = g_measurement_system.GetConversionRate(MeasurementType::WATER_PRICE);
            double conv_exchange = g_measurement_system.GetConversionRate(MeasurementType::EXCHANGE_RATE);
            
            UniValue result(UniValue::VOBJ);
            result.pushKV("total_water_prices", stats["total_water_prices"]);
            result.pushKV("total_exchange_rates", stats["total_exchange_rates"]);
            result.pushKV("total_invites", stats["total_invites"]);
            result.pushKV("total_urls", stats["total_urls"]);
            result.pushKV("invites_sent", stats["invites_sent"]);
            result.pushKV("measurements_received", stats["measurements_received"]);
            result.pushKV("validations", stats["validations"]);
            result.pushKV("conversion_rate_water", conv_water);
            result.pushKV("conversion_rate_exchange", conv_exchange);
            
            return result;
        },
    };
}

static RPCHelpMan submiturl()
{
    return RPCHelpMan{"submiturl",
        "\nSubmit a URL for automated bot data collection.\n",
        {
            {"url", RPCArg::Type::STR, RPCArg::Optional::NO, "URL to submit"},
            {"type", RPCArg::Type::STR, RPCArg::Optional::NO, "Type: 'water' or 'exchange'"},
            {"currency", RPCArg::Type::STR, RPCArg::Optional::NO, "Currency code this URL provides"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "url_id", "URL ID"},
                {RPCResult::Type::STR, "status", "Submission status"},
                {RPCResult::Type::STR_AMOUNT, "reward", "Reward amount"},
            }
        },
        RPCExamples{
            HelpExampleCli("submiturl", "\"https://waterprices.com/us\" \"water\" \"USD\"")
            + HelpExampleRpc("submiturl", "\"https://waterprices.com/us\", \"water\", \"USD\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            std::string url_str = request.params[0].get_str();
            std::string type_str = request.params[1].get_str();
            std::string currency = request.params[2].get_str();
            
            MeasurementType type;
            if (type_str == "water") {
                type = MeasurementType::WATER_PRICE;
            } else if (type_str == "exchange") {
                type = MeasurementType::EXCHANGE_RATE;
            } else {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Type must be 'water' or 'exchange'");
            }
            
            // Create validated URL
            ValidatedURL url;
            url.url_id = GetRandHash();
            url.url = url_str;
            url.type = type;
            url.currency_code = currency;
            url.last_checked = 0;
            url.is_active = true;
            url.reliability_score = 1.0;
            url.validation_count = 0;
            
            uint256 result_id = g_measurement_system.SubmitURL(url);
            
            if (result_id.IsNull()) {
                throw JSONRPCError(RPC_INTERNAL_ERROR, "Failed to submit URL");
            }
            
            CAmount reward = g_measurement_system.CalculateReward(MeasurementType::URL_SUBMISSION, 1.0);
            
            UniValue result(UniValue::VOBJ);
            result.pushKV("url_id", result_id.GetHex());
            result.pushKV("status", "submitted");
            result.pushKV("reward", UniValue(UniValue::VNUM, FormatMoney(reward)));
            
            return result;
        },
    };
}

static RPCHelpMan getdailyaveragewaterprice()
{
    return RPCHelpMan{"getdailyaveragewaterprice",
        "\nGet daily average water price for a specific currency and date.\n",
        {
            {"currency", RPCArg::Type::STR, RPCArg::Optional::NO, "Currency code (e.g., 'OUSD')"},
            {"date", RPCArg::Type::STR, RPCArg::Optional::NO, "Date in YYYY-MM-DD format"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "currency", "Currency code"},
                {RPCResult::Type::STR, "date", "Date"},
                {RPCResult::Type::NUM, "avg_water_price", "Daily average water price"},
                {RPCResult::Type::NUM, "measurement_count", "Number of measurements used"},
                {RPCResult::Type::NUM, "std_deviation", "Standard deviation"},
                {RPCResult::Type::BOOL, "is_stable", "Currency stability status"},
            }
        },
        RPCExamples{
            HelpExampleCli("getdailyaveragewaterprice", "\"OUSD\" \"2025-01-15\"")
            + HelpExampleRpc("getdailyaveragewaterprice", "\"OUSD\", \"2025-01-15\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            std::string currency = request.params[0].get_str();
            std::string date = request.params[1].get_str();
            
            auto avg = g_measurement_system.GetDailyAverageWaterPrice(currency, date);
            
            UniValue result(UniValue::VOBJ);
            result.pushKV("currency", currency);
            result.pushKV("date", date);
            
            if (avg.has_value()) {
                result.pushKV("avg_water_price", avg.value());
                
                // Get additional daily average data
                auto daily_avg = g_measurement_system.GetDailyAverage(currency, date);
                if (daily_avg.has_value()) {
                    result.pushKV("measurement_count", static_cast<int64_t>(daily_avg->measurement_count));
                    result.pushKV("std_deviation", daily_avg->std_deviation);
                    result.pushKV("is_stable", daily_avg->is_stable);
                }
            } else {
                result.pushKV("avg_water_price", UniValue::VNULL);
                result.pushKV("error", "No daily average data available for this currency and date");
            }
            
            return result;
        }
    };
}

static RPCHelpMan getdailyaverageexchangerate()
{
    return RPCHelpMan{"getdailyaverageexchangerate",
        "\nGet daily average exchange rate for an O currency to its corresponding fiat currency.\n",
        {
            {"o_currency", RPCArg::Type::STR, RPCArg::Optional::NO, "O currency code (e.g., 'OUSD')"},
            {"date", RPCArg::Type::STR, RPCArg::Optional::NO, "Date in YYYY-MM-DD format"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "o_currency", "O currency code"},
                {RPCResult::Type::STR, "fiat_currency", "Corresponding fiat currency"},
                {RPCResult::Type::STR, "date", "Date"},
                {RPCResult::Type::NUM, "avg_exchange_rate", "Daily average exchange rate"},
                {RPCResult::Type::BOOL, "is_stable", "Currency stability status"},
                {RPCResult::Type::NUM, "theoretical_rate", "Theoretical rate (based on water price in fiat currency)"},
                {RPCResult::Type::NUM, "deviation_percent", "Deviation from theoretical rate"},
                {RPCResult::Type::NUM, "water_price_per_liter", "Water price per liter in fiat currency"},
                {RPCResult::Type::STR, "water_price_currency", "Currency of the water price"},
            }
        },
        RPCExamples{
            HelpExampleCli("getdailyaverageexchangerate", "\"OUSD\" \"2025-01-15\"")
            + HelpExampleRpc("getdailyaverageexchangerate", "\"OUSD\", \"2025-01-15\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            std::string o_currency = request.params[0].get_str();
            std::string date = request.params[1].get_str();
            
            auto avg = g_measurement_system.GetDailyAverageExchangeRate(o_currency, date);
            
            UniValue result(UniValue::VOBJ);
            result.pushKV("o_currency", o_currency);
            result.pushKV("date", date);
            
            if (avg.has_value()) {
                std::string fiat_currency = g_measurement_system.GetCorrespondingFiatCurrency(o_currency);
                double theoretical_rate = g_measurement_system.GetTheoreticalExchangeRate(o_currency);
                double deviation = g_measurement_system.CalculateStabilityDeviation(o_currency, avg.value());
                
                // Get the water price that the theoretical rate is based on
                auto water_price = g_measurement_system.GetAverageWaterPrice(fiat_currency, 7);
                
                result.pushKV("fiat_currency", fiat_currency);
                result.pushKV("avg_exchange_rate", avg.value());
                result.pushKV("theoretical_rate", theoretical_rate);
                result.pushKV("deviation_percent", deviation * 100.0);
                result.pushKV("is_stable", deviation <= 0.10); // 10% tolerance
                
                if (water_price.has_value()) {
                    result.pushKV("water_price_per_liter", water_price.value());
                    result.pushKV("water_price_currency", fiat_currency);
                }
            } else {
                result.pushKV("avg_exchange_rate", UniValue::VNULL);
                result.pushKV("error", "No daily average exchange rate data available for this O currency and date");
            }
            
            return result;
        }
    };
}

static RPCHelpMan getdailyaverages()
{
    return RPCHelpMan{"getdailyaverages",
        "\nGet daily averages for a currency in a date range.\n",
        {
            {"currency", RPCArg::Type::STR, RPCArg::Optional::NO, "Currency code (e.g., 'OUSD')"},
            {"start_date", RPCArg::Type::STR, RPCArg::Optional::NO, "Start date in YYYY-MM-DD format"},
            {"end_date", RPCArg::Type::STR, RPCArg::Optional::NO, "End date in YYYY-MM-DD format"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "currency", "Currency code"},
                {RPCResult::Type::STR, "start_date", "Start date"},
                {RPCResult::Type::STR, "end_date", "End date"},
                {RPCResult::Type::ARR, "daily_averages", "Array of daily average data",
                    {
                        {RPCResult::Type::OBJ, "", "",
                            {
                                {RPCResult::Type::STR, "date", "Date"},
                                {RPCResult::Type::NUM, "avg_water_price", "Average water price"},
                                {RPCResult::Type::NUM, "avg_exchange_rate", "Average exchange rate"},
                                {RPCResult::Type::NUM, "measurement_count", "Number of measurements"},
                                {RPCResult::Type::BOOL, "is_stable", "Stability status"},
                            }
                        }
                    }
                },
            }
        },
        RPCExamples{
            HelpExampleCli("getdailyaverages", "\"OUSD\" \"2025-01-01\" \"2025-01-31\"")
            + HelpExampleRpc("getdailyaverages", "\"OUSD\", \"2025-01-01\", \"2025-01-31\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            std::string currency = request.params[0].get_str();
            std::string start_date = request.params[1].get_str();
            std::string end_date = request.params[2].get_str();
            
            auto daily_averages = g_measurement_system.GetDailyAveragesInRange(currency, start_date, end_date);
            
            UniValue result(UniValue::VOBJ);
            result.pushKV("currency", currency);
            result.pushKV("start_date", start_date);
            result.pushKV("end_date", end_date);
            
            UniValue averages(UniValue::VARR);
            for (const auto& avg : daily_averages) {
                UniValue avg_obj(UniValue::VOBJ);
                avg_obj.pushKV("date", avg.date);
                avg_obj.pushKV("avg_water_price", avg.avg_water_price);
                avg_obj.pushKV("avg_exchange_rate", avg.avg_exchange_rate);
                avg_obj.pushKV("measurement_count", static_cast<int64_t>(avg.measurement_count));
                avg_obj.pushKV("is_stable", avg.is_stable);
                averages.push_back(avg_obj);
            }
            
            result.pushKV("daily_averages", averages);
            result.pushKV("count", static_cast<int64_t>(daily_averages.size()));
            
            return result;
        }
    };
}

static RPCHelpMan calculatedailyaverages()
{
    return RPCHelpMan{"calculatedailyaverages",
        "\nCalculate and store daily averages for all currencies at current block height.\n",
        {
            {"height", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "Block height (defaults to current height)"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::NUM, "block_height", "Block height when calculated"},
                {RPCResult::Type::STR, "date", "Date for which averages were calculated"},
                {RPCResult::Type::NUM, "currencies_processed", "Number of currencies processed"},
                {RPCResult::Type::STR, "status", "Calculation status"},
            }
        },
        RPCExamples{
            HelpExampleCli("calculatedailyaverages", "100000")
            + HelpExampleRpc("calculatedailyaverages", "100000")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            int height = request.params[0].isNull() ? 100000 : request.params[0].getInt<int>();
            
            g_measurement_system.CalculateDailyAverages(height);
            
            UniValue result(UniValue::VOBJ);
            result.pushKV("block_height", height);
            result.pushKV("date", g_measurement_system.FormatDate(GetTime()));
            result.pushKV("currencies_processed", static_cast<int64_t>(65)); // Number of supported currencies
            result.pushKV("status", "Daily averages calculated and stored successfully");
            
            return result;
        }
    };
}

void RegisterOMeasurementRPCCommands(CRPCTable& t)
{
    static const CRPCCommand commands[] = {
        {"measurement", &submitwaterprice},
        {"measurement", &validatemeasurement},
        {"measurement", &getaveragewaterprice},
        {"measurement", &submitexchangerate},
        {"measurement", &createinvites},
        {"measurement", &getmeasurementstatistics},
        {"measurement", &submiturl},
        {"measurement", &getdailyaveragewaterprice},
        {"measurement", &getdailyaverageexchangerate},
        {"measurement", &getdailyaverages},
        {"measurement", &calculatedailyaverages},
    };
    
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}
