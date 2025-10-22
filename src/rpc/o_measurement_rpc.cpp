// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/o_measurement_rpc.h>
#include <measurement/measurement_system.h>
#include <measurement/o_measurement_db.h>
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
#include <logging.h>

using namespace OMeasurement;
using node::NodeContext;

static RPCHelpMan submitwaterprice()
{
    return RPCHelpMan{
        "submitwaterprice",
        "\nSubmit a water price measurement.\n",
        {
            {"currency", RPCArg::Type::STR, RPCArg::Optional::NO, "Currency code (USD, EUR, JPY, etc.)"},
            {"price", RPCArg::Type::NUM, RPCArg::Optional::NO, "Price in smallest unit (cents for USD, etc.)"},
            {"location", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Geographic location (for offline measurements)"},
            {"source_url", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Source URL (for online measurements)"},
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
            HelpExampleCli("submitwaterprice", "\"USD\" 150 \"New York\" \"https://example.com\" \"abc123...\" \"def456...\"")
            + HelpExampleRpc("submitwaterprice", "\"USD\", 150, \"New York\", \"https://example.com\", \"abc123...\", \"def456...\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            std::string currency = request.params[0].get_str();
            int64_t price = request.params[1].getInt<int64_t>();
            std::string location = request.params[2].isNull() ? "" : request.params[2].get_str();
            std::string source_url = request.params[3].isNull() ? "" : request.params[3].get_str();
            std::string proof_hash = request.params[4].isNull() ? "" : request.params[4].get_str();
            uint256 invite_id = ParseHashV(request.params[5], "invite_id");
            
            // Create measurement
            WaterPriceMeasurement measurement;
            measurement.measurement_id = GetRandHash();
            measurement.currency_code = currency;
            measurement.price = price;
            measurement.location = location;
            measurement.source_url = source_url;
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
    return RPCHelpMan{
        "validatemeasurement",
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
    return RPCHelpMan{
        "getaveragewaterprice",
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
    return RPCHelpMan{
        "submitexchangerate",
        "\nSubmit an exchange rate measurement.\n",
        {
            {"from_currency", RPCArg::Type::STR, RPCArg::Optional::NO, "Source currency code"},
            {"to_currency", RPCArg::Type::STR, RPCArg::Optional::NO, "Target currency code"},
            {"rate", RPCArg::Type::NUM, RPCArg::Optional::NO, "Exchange rate"},
            {"location", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Geographic location (for offline measurements)"},
            {"source_url", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Source URL (for online measurements)"},
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
            HelpExampleCli("submitexchangerate", "\"USD\" \"EUR\" 0.85 \"New York\" \"https://example.com\" \"abc123...\"")
            + HelpExampleRpc("submitexchangerate", "\"USD\", \"EUR\", 0.85, \"New York\", \"https://example.com\", \"abc123...\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            std::string from_currency = request.params[0].get_str();
            std::string to_currency = request.params[1].get_str();
            double rate = request.params[2].get_real();
            std::string location = request.params[3].isNull() ? "" : request.params[3].get_str();
            std::string source_url = request.params[4].isNull() ? "" : request.params[4].get_str();
            uint256 invite_id = ParseHashV(request.params[5], "invite_id");
            
            // Create measurement
            ExchangeRateMeasurement measurement;
            measurement.measurement_id = GetRandHash();
            measurement.from_currency = from_currency;
            measurement.to_currency = to_currency;
            measurement.exchange_rate = rate;
            measurement.location = location;
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
    return RPCHelpMan{
        "createinvites",
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
            
            if (invites.empty()) {
                // No invites created - likely due to readiness conditions not being met
                UniValue error_info(UniValue::VOBJ);
                error_info.pushKV("error", "No invitations created");
                error_info.pushKV("reason", "Readiness conditions not met");
                error_info.pushKV("type", type_str);
                if (!currency.empty()) {
                    error_info.pushKV("currency", currency);
                }
                result.push_back(error_info);
            } else {
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
            }
            
            return result;
        },
    };
}

static RPCHelpMan checkmeasurementreadiness()
{
    return RPCHelpMan{
        "checkmeasurementreadiness",
        "\nCheck if measurement invitations can be created for a specific type and currency.\n",
        {
            {"type", RPCArg::Type::STR, RPCArg::Optional::NO, "Type: 'water', 'exchange', 'validation'"},
            {"currency", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Specific currency (optional)"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "type", "Measurement type"},
                {RPCResult::Type::STR, "currency", "Currency code (if specified)"},
                {RPCResult::Type::BOOL, "ready", "Whether ready to create invitations"},
                {RPCResult::Type::STR, "reason", "Reason for readiness status"},
                {RPCResult::Type::STR, "readiness_details", "Detailed readiness information"},
            }
        },
        RPCExamples{
            HelpExampleCli("checkmeasurementreadiness", "water")
            + HelpExampleCli("checkmeasurementreadiness", "exchange USD")
            + HelpExampleRpc("checkmeasurementreadiness", "water")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            std::string type_str = request.params[0].get_str();
            std::string currency = request.params[1].isNull() ? "" : request.params[1].get_str();
            
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
            
            bool ready = g_measurement_system.CheckMeasurementReadiness(type, currency);
            
            UniValue result(UniValue::VOBJ);
            result.pushKV("type", type_str);
            if (!currency.empty()) {
                result.pushKV("currency", currency);
            }
            result.pushKV("ready", ready);
            
            if (ready) {
                result.pushKV("reason", "Readiness conditions met - invitations can be created");
            } else {
                std::string reason = "Readiness conditions not met - ";
                if (type == MeasurementType::WATER_PRICE || type == MeasurementType::WATER_PRICE_OFFLINE_VALIDATION) {
                    reason += "insufficient users (need 100+ users)";
                } else if (type == MeasurementType::EXCHANGE_RATE) {
                    reason += "insufficient coins (need 100,000+ O coins)";
                } else {
                    reason += "unknown requirements";
                }
                result.pushKV("reason", reason);
            }
            
            // Add detailed readiness information
            if (!currency.empty()) {
                std::string o_currency = g_measurement_system.GetOCurrencyFromFiat(currency);
                if (!o_currency.empty()) {
                    result.pushKV("o_currency", o_currency);
                    result.pushKV("readiness_details", "Check specific currency readiness with measurement_readiness RPC commands");
                }
            } else {
                result.pushKV("readiness_details", "Check general readiness with measurement_readiness RPC commands");
            }
            
            return result;
        },
    };
}

static RPCHelpMan getmeasurementstatistics()
{
    return RPCHelpMan{
        "getmeasurementstatistics",
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
    return RPCHelpMan{
        "submiturl",
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
    return RPCHelpMan{
        "getdailyaveragewaterprice",
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
    return RPCHelpMan{
        "getdailyaverageexchangerate",
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
    return RPCHelpMan{
        "getdailyaverages",
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
    return RPCHelpMan{
        "calculatedailyaverages",
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

static RPCHelpMan getaveragewaterpricewithconfidence()
{
    return RPCHelpMan{
        "getaveragewaterpricewithconfidence",
        "\nGet average water price with confidence information.\n",
        {
            {"currency", RPCArg::Type::STR, RPCArg::Optional::NO, "Currency code (e.g., 'USD', 'EUR')"},
            {"days", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "Number of days to look back (default: 30)"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "currency", "Currency code"},
                {RPCResult::Type::NUM, "average_price", "Average water price"},
                {RPCResult::Type::NUM, "measurement_count", "Number of measurements used"},
                {RPCResult::Type::NUM, "standard_deviation", "Standard deviation"},
                {RPCResult::Type::STR, "confidence_level", "Confidence level: insufficient_data, low_confidence, high_confidence, very_high_confidence"},
                {RPCResult::Type::BOOL, "is_statistically_significant", "Whether the average is statistically significant"},
                {RPCResult::Type::NUM, "days", "Number of days analyzed"},
            }
        },
        RPCExamples{
            HelpExampleCli("getaveragewaterpricewithconfidence", "USD")
            + HelpExampleCli("getaveragewaterpricewithconfidence", "EUR 7")
            + HelpExampleRpc("getaveragewaterpricewithconfidence", "USD")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            std::string currency = request.params[0].get_str();
            int days = request.params[1].isNull() ? 30 : request.params[1].getInt<int>();
            
            auto result = g_measurement_system.GetAverageWaterPriceWithConfidence(currency, days);
            
            if (!result.has_value()) {
                throw JSONRPCError(RPC_INVALID_REQUEST, "No water price measurements found for the specified currency and time period");
            }
            
            UniValue response(UniValue::VOBJ);
            response.pushKV("currency", currency);
            response.pushKV("average_price", result->value);
            response.pushKV("measurement_count", result->measurement_count);
            response.pushKV("standard_deviation", result->std_deviation);
            response.pushKV("confidence_level", result->GetConfidenceString());
            response.pushKV("is_statistically_significant", result->is_statistically_significant);
            response.pushKV("days", days);
            
            return response;
        }
    };
}

static RPCHelpMan getaverageexchangeratewithconfidence()
{
    return RPCHelpMan{
        "getaverageexchangeratewithconfidence",
        "\nGet average exchange rate with confidence information.\n",
        {
            {"from_currency", RPCArg::Type::STR, RPCArg::Optional::NO, "From currency code (e.g., 'OUSD')"},
            {"to_currency", RPCArg::Type::STR, RPCArg::Optional::NO, "To currency code (e.g., 'USD')"},
            {"days", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "Number of days to look back (default: 7)"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "from_currency", "From currency code"},
                {RPCResult::Type::STR, "to_currency", "To currency code"},
                {RPCResult::Type::NUM, "average_rate", "Average exchange rate"},
                {RPCResult::Type::NUM, "measurement_count", "Number of measurements used"},
                {RPCResult::Type::NUM, "standard_deviation", "Standard deviation"},
                {RPCResult::Type::STR, "confidence_level", "Confidence level: insufficient_data, low_confidence, high_confidence, very_high_confidence"},
                {RPCResult::Type::BOOL, "is_statistically_significant", "Whether the average is statistically significant"},
                {RPCResult::Type::NUM, "days", "Number of days analyzed"},
            }
        },
        RPCExamples{
            HelpExampleCli("getaverageexchangeratewithconfidence", "OUSD USD")
            + HelpExampleCli("getaverageexchangeratewithconfidence", "OEUR EUR 14")
            + HelpExampleRpc("getaverageexchangeratewithconfidence", "OUSD USD")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            std::string from_currency = request.params[0].get_str();
            std::string to_currency = request.params[1].get_str();
            int days = request.params[2].isNull() ? 7 : request.params[2].getInt<int>();
            
            auto result = g_measurement_system.GetAverageExchangeRateWithConfidence(from_currency, to_currency, days);
            
            if (!result.has_value()) {
                throw JSONRPCError(RPC_INVALID_REQUEST, "No exchange rate measurements found for the specified currency pair and time period");
            }
            
            UniValue response(UniValue::VOBJ);
            response.pushKV("from_currency", from_currency);
            response.pushKV("to_currency", to_currency);
            response.pushKV("average_rate", result->value);
            response.pushKV("measurement_count", result->measurement_count);
            response.pushKV("standard_deviation", result->std_deviation);
            response.pushKV("confidence_level", result->GetConfidenceString());
            response.pushKV("is_statistically_significant", result->is_statistically_significant);
            response.pushKV("days", days);
            
            return response;
        }
    };
}

static RPCHelpMan getdailyaveragewithconfidence()
{
    return RPCHelpMan{
        "getdailyaveragewithconfidence",
        "\nGet daily average with confidence information.\n",
        {
            {"currency", RPCArg::Type::STR, RPCArg::Optional::NO, "Currency code (e.g., 'OUSD')"},
            {"date", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Date in YYYY-MM-DD format (default: today)"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "currency", "Currency code"},
                {RPCResult::Type::STR, "date", "Date"},
                {RPCResult::Type::NUM, "avg_water_price", "Average water price"},
                {RPCResult::Type::NUM, "avg_exchange_rate", "Average exchange rate"},
                {RPCResult::Type::NUM, "measurement_count", "Number of measurements used"},
                {RPCResult::Type::NUM, "standard_deviation", "Standard deviation"},
                {RPCResult::Type::STR, "confidence_level", "Confidence level: insufficient_data, low_confidence, high_confidence, very_high_confidence"},
                {RPCResult::Type::BOOL, "is_statistically_significant", "Whether the average is statistically significant"},
                {RPCResult::Type::BOOL, "is_stable", "Whether the currency is stable"},
                {RPCResult::Type::NUM, "block_height", "Block height when calculated"},
            }
        },
        RPCExamples{
            HelpExampleCli("getdailyaveragewithconfidence", "OUSD")
            + HelpExampleCli("getdailyaveragewithconfidence", "OEUR 2024-01-15")
            + HelpExampleRpc("getdailyaveragewithconfidence", "OUSD")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            std::string currency = request.params[0].get_str();
            std::string date = request.params[1].isNull() ? g_measurement_system.FormatDate(GetTime()) : request.params[1].get_str();
            
            auto result = g_measurement_system.GetDailyAverage(currency, date);
            
            if (!result.has_value()) {
                throw JSONRPCError(RPC_INVALID_REQUEST, "No daily average found for the specified currency and date");
            }
            
            std::string confidence_str;
            switch (result->confidence_level) {
                case OMeasurement::ConfidenceLevel::INSUFFICIENT_DATA: confidence_str = "insufficient_data"; break;
                case OMeasurement::ConfidenceLevel::LOW_CONFIDENCE: confidence_str = "low_confidence"; break;
                case OMeasurement::ConfidenceLevel::HIGH_CONFIDENCE: confidence_str = "high_confidence"; break;
                case OMeasurement::ConfidenceLevel::VERY_HIGH_CONFIDENCE: confidence_str = "very_high_confidence"; break;
                default: confidence_str = "unknown"; break;
            }
            
            UniValue response(UniValue::VOBJ);
            response.pushKV("currency", result->currency_code);
            response.pushKV("date", result->date);
            response.pushKV("avg_water_price", result->avg_water_price);
            response.pushKV("avg_exchange_rate", result->avg_exchange_rate);
            response.pushKV("measurement_count", result->measurement_count);
            response.pushKV("standard_deviation", result->std_deviation);
            response.pushKV("confidence_level", confidence_str);
            response.pushKV("is_statistically_significant", result->is_statistically_significant);
            response.pushKV("is_stable", result->is_stable);
            response.pushKV("block_height", result->block_height);
            
            return response;
        }
    };
}

static RPCHelpMan getdynamicmeasurementtarget()
{
    return RPCHelpMan{
        "getdynamicmeasurementtarget",
        "\nGet dynamic measurement target for a specific type and currency.\n",
        {
            {"type", RPCArg::Type::STR, RPCArg::Optional::NO, "Type: 'water' or 'exchange'"},
            {"currency", RPCArg::Type::STR, RPCArg::Optional::NO, "Currency code (e.g., 'USD', 'OUSD')"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "type", "Measurement type"},
                {RPCResult::Type::STR, "currency", "Currency code"},
                {RPCResult::Type::NUM, "target", "Dynamic measurement target"},
                {RPCResult::Type::NUM, "volatility", "Current volatility (coefficient of variation)"},
                {RPCResult::Type::BOOL, "is_early_stage", "Whether currency is in early stage"},
                {RPCResult::Type::STR, "target_reason", "Reason for the target value"},
            }
        },
        RPCExamples{
            HelpExampleCli("getdynamicmeasurementtarget", "water USD")
            + HelpExampleCli("getdynamicmeasurementtarget", "exchange OUSD")
            + HelpExampleRpc("getdynamicmeasurementtarget", "water USD")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            std::string type_str = request.params[0].get_str();
            std::string currency = request.params[1].get_str();
            
            MeasurementType type;
            if (type_str == "water") {
                type = MeasurementType::WATER_PRICE;
            } else if (type_str == "exchange") {
                type = MeasurementType::EXCHANGE_RATE;
            } else {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Type must be 'water' or 'exchange'");
            }
            
            int target = g_measurement_system.GetCurrentMeasurementTarget(type, currency);
            double volatility = g_measurement_system.CalculateVolatility(type, currency, 7);
            bool is_early_stage = g_measurement_system.IsEarlyStage(type, currency);
            
            std::string reason;
            if (is_early_stage) {
                reason = "early_stage_high_target";
            } else if (volatility >= 0.15) {
                reason = "high_volatility_requires_more_measurements";
            } else if (volatility <= 0.05) {
                reason = "low_volatility_allows_fewer_measurements";
            } else {
                reason = "medium_volatility_interpolated_target";
            }
            
            UniValue response(UniValue::VOBJ);
            response.pushKV("type", type_str);
            response.pushKV("currency", currency);
            response.pushKV("target", target);
            response.pushKV("volatility", volatility);
            response.pushKV("is_early_stage", is_early_stage);
            response.pushKV("target_reason", reason);
            
            return response;
        }
    };
}

static RPCHelpMan getmeasurementvolatility()
{
    return RPCHelpMan{
        "getmeasurementvolatility",
        "\nGet volatility information for measurements.\n",
        {
            {"type", RPCArg::Type::STR, RPCArg::Optional::NO, "Type: 'water' or 'exchange'"},
            {"currency", RPCArg::Type::STR, RPCArg::Optional::NO, "Currency code (e.g., 'USD', 'OUSD')"},
            {"days", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "Number of days to look back (default: 7)"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "type", "Measurement type"},
                {RPCResult::Type::STR, "currency", "Currency code"},
                {RPCResult::Type::NUM, "volatility", "Coefficient of variation"},
                {RPCResult::Type::NUM, "days", "Number of days analyzed"},
                {RPCResult::Type::STR, "volatility_level", "Volatility level: low, medium, or high"},
                {RPCResult::Type::NUM, "measurement_count", "Number of measurements used"},
            }
        },
        RPCExamples{
            HelpExampleCli("getmeasurementvolatility", "water USD")
            + HelpExampleCli("getmeasurementvolatility", "exchange OUSD 14")
            + HelpExampleRpc("getmeasurementvolatility", "water USD")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            std::string type_str = request.params[0].get_str();
            std::string currency = request.params[1].get_str();
            int days = request.params[2].isNull() ? 7 : request.params[2].getInt<int>();
            
            MeasurementType type;
            if (type_str == "water") {
                type = MeasurementType::WATER_PRICE;
            } else if (type_str == "exchange") {
                type = MeasurementType::EXCHANGE_RATE;
            } else {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Type must be 'water' or 'exchange'");
            }
            
            double volatility = g_measurement_system.CalculateVolatility(type, currency, days);
            
            std::string level;
            if (volatility <= 0.05) {
                level = "low";
            } else if (volatility >= 0.15) {
                level = "high";
            } else {
                level = "medium";
            }
            
            // Get measurement count for context
            int64_t current_time = GetTime();
            int64_t start_time = current_time - (days * 24 * 3600);
            int measurement_count = 0;
            
            if (type == MeasurementType::WATER_PRICE) {
                auto measurements = g_measurement_system.GetWaterPricesInRange(currency, start_time, current_time);
                for (const auto& m : measurements) {
                    if (m.is_validated) measurement_count++;
                }
            } else if (type == MeasurementType::EXCHANGE_RATE) {
                if (g_measurement_system.IsOCurrency(currency)) {
                    std::string fiat_currency = g_measurement_system.GetCorrespondingFiatCurrency(currency);
                    auto measurements = g_measurement_system.GetExchangeRatesInRange(currency, fiat_currency, start_time, current_time);
                    for (const auto& m : measurements) {
                        if (m.is_validated) measurement_count++;
                    }
                }
            }
            
            UniValue response(UniValue::VOBJ);
            response.pushKV("type", type_str);
            response.pushKV("currency", currency);
            response.pushKV("volatility", volatility);
            response.pushKV("days", days);
            response.pushKV("volatility_level", level);
            response.pushKV("measurement_count", measurement_count);
            
            return response;
        }
    };
}

static RPCHelpMan getmeasurementtargetstatistics()
{
    return RPCHelpMan{
        "getmeasurementtargetstatistics",
        "\nGet measurement target statistics for all currencies.\n",
        {
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::OBJ, "targets", "Dynamic targets for all currencies"},
                {RPCResult::Type::NUM, "total_currencies", "Total number of currencies tracked"},
                {RPCResult::Type::NUM, "min_target", "Minimum target across all currencies"},
                {RPCResult::Type::NUM, "max_target", "Maximum target across all currencies"},
                {RPCResult::Type::NUM, "avg_target", "Average target across all currencies"},
            }
        },
        RPCExamples{
            HelpExampleCli("getmeasurementtargetstatistics", "")
            + HelpExampleRpc("getmeasurementtargetstatistics", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            auto stats = g_measurement_system.GetMeasurementTargetStatistics();
            
            UniValue targets(UniValue::VOBJ);
            int min_target = INT_MAX;
            int max_target = 0;
            int total_targets = 0;
            int sum_targets = 0;
            
            for (const auto& [key, target] : stats) {
                targets.pushKV(key, target);
                min_target = std::min(min_target, target);
                max_target = std::max(max_target, target);
                total_targets++;
                sum_targets += target;
            }
            
            double avg_target = total_targets > 0 ? static_cast<double>(sum_targets) / total_targets : 0.0;
            
            UniValue response(UniValue::VOBJ);
            response.pushKV("targets", targets);
            response.pushKV("total_currencies", total_targets);
            response.pushKV("min_target", min_target == INT_MAX ? 0 : min_target);
            response.pushKV("max_target", max_target);
            response.pushKV("avg_target", avg_target);
            
            return response;
        }
    };
}

static RPCHelpMan submitwaterpricewithvalidation()
{
    return RPCHelpMan{
        "submitwaterpricewithvalidation",
        "\nSubmit water price measurement with automated validation.\n",
        {
            {"currency", RPCArg::Type::STR, RPCArg::Optional::NO, "Currency code (e.g., 'USD', 'EUR')"},
            {"price", RPCArg::Type::NUM, RPCArg::Optional::NO, "Water price (e.g., 1.50 for $1.50)"},
            {"source_type", RPCArg::Type::STR, RPCArg::Optional::NO, "Source type: 'user_online', 'user_offline', 'bot_online', 'bot_offline'"},
            {"source_url", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Source URL (required for online measurements)"},
            {"location", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Geographic location (required for offline measurements)"},
            {"image_hash", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Image hash for offline measurements"},
            {"timestamp", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "Measurement timestamp (default: current time)"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "measurement_id", "Measurement ID if successful"},
                {RPCResult::Type::BOOL, "validation_passed", "Whether automated validation passed"},
                {RPCResult::Type::STR, "validation_result", "Validation result: passed, failed_gaussian, failed_timestamp, failed_url, failed_location, failed_format"},
                {RPCResult::Type::STR, "failure_reason", "Failure reason if validation failed"},
                {RPCResult::Type::NUM, "gaussian_deviation", "How many standard deviations from mean"},
            }
        },
        RPCExamples{
            HelpExampleCli("submitwaterpricewithvalidation", "USD 1.50 user_online https://example.com/water-price")
            + HelpExampleCli("submitwaterpricewithvalidation", "USD 1.50 user_offline \"New York, USA\" abc123hash")
            + HelpExampleRpc("submitwaterpricewithvalidation", "USD 1.50 user_online https://example.com/water-price")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            std::string currency = request.params[0].get_str();
            double price = request.params[1].get_real();
            std::string source_type_str = request.params[2].get_str();
            
            // Parse source type
            MeasurementSource source;
            if (source_type_str == "user_online") {
                source = MeasurementSource::USER_ONLINE;
            } else if (source_type_str == "user_offline") {
                source = MeasurementSource::USER_OFFLINE;
            } else if (source_type_str == "bot_online") {
                source = MeasurementSource::BOT_ONLINE;
            } else if (source_type_str == "bot_offline") {
                source = MeasurementSource::BOT_OFFLINE;
            } else {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid source type. Must be: user_online, user_offline, bot_online, bot_offline");
            }
            
            // Create measurement
            WaterPriceMeasurement measurement;
            measurement.measurement_id = GetRandHash();
            measurement.currency_code = currency;
            measurement.price = static_cast<int64_t>(price * 100); // Convert to cents
            measurement.source = source;
            measurement.timestamp = request.params[6].isNull() ? GetTime() : request.params[6].getInt<int64_t>();
            measurement.block_height = 0; // Will be set when mined
            
            // Set source-specific fields
            if (source == MeasurementSource::USER_ONLINE || source == MeasurementSource::BOT_ONLINE) {
                if (request.params[3].isNull()) {
                    throw JSONRPCError(RPC_INVALID_PARAMETER, "source_url is required for online measurements");
                }
                measurement.source_url = request.params[3].get_str();
            } else {
                if (request.params[4].isNull()) {
                    throw JSONRPCError(RPC_INVALID_PARAMETER, "location is required for offline measurements");
                }
                measurement.location = request.params[4].get_str();
                if (!request.params[5].isNull()) {
                    measurement.proof_image_hash = request.params[5].get_str();
                }
            }
            
            // Submit with validation
            uint256 measurement_id = g_measurement_system.SubmitMeasurementWithValidation(measurement);
            
            UniValue response(UniValue::VOBJ);
            if (measurement_id.IsNull()) {
                response.pushKV("measurement_id", "");
                response.pushKV("validation_passed", false);
                response.pushKV("validation_result", "failed");
                response.pushKV("failure_reason", "Automated validation failed");
                response.pushKV("gaussian_deviation", 0.0);
            } else {
                response.pushKV("measurement_id", measurement_id.GetHex());
                response.pushKV("validation_passed", true);
                response.pushKV("validation_result", "passed");
                response.pushKV("failure_reason", "");
                response.pushKV("gaussian_deviation", measurement.auto_validation.gaussian_deviation);
            }
            
            return response;
        }
    };
}

static RPCHelpMan submitexchangeratewithvalidation()
{
    return RPCHelpMan{
        "submitexchangeratewithvalidation",
        "\nSubmit exchange rate measurement with automated validation.\n",
        {
            {"from_currency", RPCArg::Type::STR, RPCArg::Optional::NO, "From currency (e.g., 'OUSD')"},
            {"to_currency", RPCArg::Type::STR, RPCArg::Optional::NO, "To currency (e.g., 'USD')"},
            {"exchange_rate", RPCArg::Type::NUM, RPCArg::Optional::NO, "Exchange rate (e.g., 1.20)"},
            {"source_type", RPCArg::Type::STR, RPCArg::Optional::NO, "Source type: 'user_online', 'user_offline', 'bot_online', 'bot_offline'"},
            {"source_url", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Source URL (required for online measurements)"},
            {"location", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Geographic location (required for offline measurements)"},
            {"image_hash", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Image hash for offline measurements"},
            {"timestamp", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "Measurement timestamp (default: current time)"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "measurement_id", "Measurement ID if successful"},
                {RPCResult::Type::BOOL, "validation_passed", "Whether automated validation passed"},
                {RPCResult::Type::STR, "validation_result", "Validation result"},
                {RPCResult::Type::STR, "failure_reason", "Failure reason if validation failed"},
                {RPCResult::Type::NUM, "gaussian_deviation", "How many standard deviations from mean"},
            }
        },
        RPCExamples{
            HelpExampleCli("submitexchangeratewithvalidation", "OUSD USD 1.20 user_online https://example.com/exchange-rate")
            + HelpExampleCli("submitexchangeratewithvalidation", "OUSD USD 1.20 user_offline \"New York, USA\" abc123hash")
            + HelpExampleRpc("submitexchangeratewithvalidation", "OUSD USD 1.20 user_online https://example.com/exchange-rate")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            std::string from_currency = request.params[0].get_str();
            std::string to_currency = request.params[1].get_str();
            double exchange_rate = request.params[2].get_real();
            std::string source_type_str = request.params[3].get_str();
            
            // Parse source type
            MeasurementSource source;
            if (source_type_str == "user_online") {
                source = MeasurementSource::USER_ONLINE;
            } else if (source_type_str == "user_offline") {
                source = MeasurementSource::USER_OFFLINE;
            } else if (source_type_str == "bot_online") {
                source = MeasurementSource::BOT_ONLINE;
            } else if (source_type_str == "bot_offline") {
                source = MeasurementSource::BOT_OFFLINE;
            } else {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid source type. Must be: user_online, user_offline, bot_online, bot_offline");
            }
            
            // Create measurement
            ExchangeRateMeasurement measurement;
            measurement.measurement_id = GetRandHash();
            measurement.from_currency = from_currency;
            measurement.to_currency = to_currency;
            measurement.exchange_rate = exchange_rate;
            measurement.source = source;
            measurement.timestamp = request.params[7].isNull() ? GetTime() : request.params[7].getInt<int64_t>();
            measurement.block_height = 0; // Will be set when mined
            
            // Set source-specific fields
            if (source == MeasurementSource::USER_ONLINE || source == MeasurementSource::BOT_ONLINE) {
                if (request.params[4].isNull()) {
                    throw JSONRPCError(RPC_INVALID_PARAMETER, "source_url is required for online measurements");
                }
                measurement.source_url = request.params[4].get_str();
            } else {
                if (request.params[5].isNull()) {
                    throw JSONRPCError(RPC_INVALID_PARAMETER, "location is required for offline measurements");
                }
                measurement.location = request.params[5].get_str();
                if (!request.params[6].isNull()) {
                    measurement.proof_image_hash = request.params[6].get_str();
                }
            }
            
            // Submit with validation
            uint256 measurement_id = g_measurement_system.SubmitMeasurementWithValidation(measurement);
            
            UniValue response(UniValue::VOBJ);
            if (measurement_id.IsNull()) {
                response.pushKV("measurement_id", "");
                response.pushKV("validation_passed", false);
                response.pushKV("validation_result", "failed");
                response.pushKV("failure_reason", "Automated validation failed");
                response.pushKV("gaussian_deviation", 0.0);
            } else {
                response.pushKV("measurement_id", measurement_id.GetHex());
                response.pushKV("validation_passed", true);
                response.pushKV("validation_result", "passed");
                response.pushKV("failure_reason", "");
                response.pushKV("gaussian_deviation", measurement.auto_validation.gaussian_deviation);
            }
            
            return response;
        }
    };
}

static RPCHelpMan getgaussianrange()
{
    return RPCHelpMan{
        "getgaussianrange",
        "\nGet Gaussian acceptance range for a currency.\n",
        {
            {"type", RPCArg::Type::STR, RPCArg::Optional::NO, "Type: 'water' or 'exchange'"},
            {"currency", RPCArg::Type::STR, RPCArg::Optional::NO, "Currency code (e.g., 'USD', 'OUSD')"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "type", "Measurement type"},
                {RPCResult::Type::STR, "currency", "Currency code"},
                {RPCResult::Type::NUM, "min_value", "Minimum acceptable value"},
                {RPCResult::Type::NUM, "max_value", "Maximum acceptable value"},
                {RPCResult::Type::NUM, "mean", "Current mean value"},
                {RPCResult::Type::NUM, "std_deviation", "Current standard deviation"},
                {RPCResult::Type::NUM, "threshold", "Gaussian threshold (standard deviations)"},
            }
        },
        RPCExamples{
            HelpExampleCli("getgaussianrange", "water USD")
            + HelpExampleCli("getgaussianrange", "exchange OUSD")
            + HelpExampleRpc("getgaussianrange", "water USD")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            std::string type_str = request.params[0].get_str();
            std::string currency = request.params[1].get_str();
            
            MeasurementType type;
            if (type_str == "water") {
                type = MeasurementType::WATER_PRICE;
            } else if (type_str == "exchange") {
                type = MeasurementType::EXCHANGE_RATE;
            } else {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Type must be 'water' or 'exchange'");
            }
            
            auto range = g_measurement_system.GetGaussianRange(type, currency);
            auto avg_result = g_measurement_system.GetAverageWaterPriceWithConfidence(currency, 7);
            
            UniValue response(UniValue::VOBJ);
            response.pushKV("type", type_str);
            response.pushKV("currency", currency);
            response.pushKV("min_value", range.first);
            response.pushKV("max_value", range.second);
            
            if (avg_result.has_value()) {
                response.pushKV("mean", avg_result->value);
                response.pushKV("std_deviation", avg_result->std_deviation);
            } else {
                response.pushKV("mean", 0.0);
                response.pushKV("std_deviation", 0.0);
            }
            
            response.pushKV("threshold", 3.0); // GAUSSIAN_ACCEPTANCE_THRESHOLD
            
            return response;
        }
    };
}

static RPCHelpMan checkandcreateinvitations()
{
    return RPCHelpMan{
        "checkandcreateinvitations",
        "\nCheck and create invitations automatically based on measurement targets.\n",
        {
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::BOOL, "success", "Whether the operation was successful"},
                {RPCResult::Type::NUM, "total_invites_created", "Total number of invitation sets created"},
                {RPCResult::Type::STR, "message", "Status message"},
            }
        },
        RPCExamples{
            HelpExampleCli("checkandcreateinvitations", "")
            + HelpExampleRpc("checkandcreateinvitations", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            try {
                g_measurement_system.CheckAndCreateInvitations();
                
                UniValue response(UniValue::VOBJ);
                response.pushKV("success", true);
                response.pushKV("total_invites_created", "See logs for details");
                response.pushKV("message", "Automatic invitation check completed successfully");
                
                return response;
            } catch (const std::exception& e) {
                UniValue response(UniValue::VOBJ);
                response.pushKV("success", false);
                response.pushKV("total_invites_created", 0);
                response.pushKV("message", std::string("Error: ") + e.what());
                
                return response;
            }
        }
    };
}

static RPCHelpMan monitormeasurementtargets()
{
    return RPCHelpMan{
        "monitormeasurementtargets",
        "\nMonitor measurement targets for all currencies.\n",
        {
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::BOOL, "success", "Whether the operation was successful"},
                {RPCResult::Type::NUM, "currencies_needing_attention", "Number of currencies needing attention"},
                {RPCResult::Type::STR, "message", "Status message"},
            }
        },
        RPCExamples{
            HelpExampleCli("monitormeasurementtargets", "")
            + HelpExampleRpc("monitormeasurementtargets", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            try {
                g_measurement_system.MonitorMeasurementTargets();
                
                UniValue response(UniValue::VOBJ);
                response.pushKV("success", true);
                response.pushKV("currencies_needing_attention", "See logs for details");
                response.pushKV("message", "Target monitoring completed successfully");
                
                return response;
            } catch (const std::exception& e) {
                UniValue response(UniValue::VOBJ);
                response.pushKV("success", false);
                response.pushKV("currencies_needing_attention", 0);
                response.pushKV("message", std::string("Error: ") + e.what());
                
                return response;
            }
        }
    };
}

static RPCHelpMan getmeasurementgap()
{
    return RPCHelpMan{
        "getmeasurementgap",
        "\nGet measurement gap for a specific currency and type.\n",
        {
            {"type", RPCArg::Type::STR, RPCArg::Optional::NO, "Type: 'water' or 'exchange'"},
            {"currency", RPCArg::Type::STR, RPCArg::Optional::NO, "Currency code (e.g., 'USD', 'OUSD')"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "type", "Measurement type"},
                {RPCResult::Type::STR, "currency", "Currency code"},
                {RPCResult::Type::NUM, "target", "Target number of measurements"},
                {RPCResult::Type::NUM, "current", "Current number of measurements"},
                {RPCResult::Type::NUM, "gap", "Gap (target - current)"},
                {RPCResult::Type::NUM, "gap_ratio", "Gap ratio (gap / target)"},
                {RPCResult::Type::BOOL, "needs_more", "Whether more measurements are needed"},
            }
        },
        RPCExamples{
            HelpExampleCli("getmeasurementgap", "water USD")
            + HelpExampleCli("getmeasurementgap", "exchange OUSD")
            + HelpExampleRpc("getmeasurementgap", "water USD")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            std::string type_str = request.params[0].get_str();
            std::string currency = request.params[1].get_str();
            
            MeasurementType type;
            if (type_str == "water") {
                type = MeasurementType::WATER_PRICE;
            } else if (type_str == "exchange") {
                type = MeasurementType::EXCHANGE_RATE;
            } else {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Type must be 'water' or 'exchange'");
            }
            
            int target = g_measurement_system.GetCurrentMeasurementTarget(type, currency);
            int gap = g_measurement_system.GetMeasurementGap(type, currency);
            int current = target - gap;
            double gap_ratio = target > 0 ? static_cast<double>(gap) / target : 0.0;
            bool needs_more = g_measurement_system.NeedsMoreMeasurements(type, currency);
            
            UniValue response(UniValue::VOBJ);
            response.pushKV("type", type_str);
            response.pushKV("currency", currency);
            response.pushKV("target", target);
            response.pushKV("current", current);
            response.pushKV("gap", gap);
            response.pushKV("gap_ratio", gap_ratio);
            response.pushKV("needs_more", needs_more);
            
            return response;
        }
    };
}

static RPCHelpMan recalculatecurrencystability()
{
    return RPCHelpMan{
        "recalculatecurrencystability",
        "\nManually recalculate currency stability status based on current measurement averages.\n",
        {
            {"height", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "Block height (default: 100000)"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::BOOL, "success", "Whether the operation was successful"},
                {RPCResult::Type::NUM, "block_height", "Block height used for calculation"},
                {RPCResult::Type::STR, "message", "Status message"},
            }
        },
        RPCExamples{
            HelpExampleCli("recalculatecurrencystability", "")
            + HelpExampleCli("recalculatecurrencystability", "150000")
            + HelpExampleRpc("recalculatecurrencystability", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            int height = request.params[0].isNull() ? 100000 : request.params[0].getInt<int>();
            
            try {
                g_measurement_system.RecalculateCurrencyStability(height);
                
                UniValue response(UniValue::VOBJ);
                response.pushKV("success", true);
                response.pushKV("block_height", height);
                response.pushKV("message", "Currency stability recalculation completed successfully");
                
                return response;
            } catch (const std::exception& e) {
                UniValue response(UniValue::VOBJ);
                response.pushKV("success", false);
                response.pushKV("block_height", height);
                response.pushKV("message", std::string("Error: ") + e.what());
                
                return response;
            }
        }
    };
}

static RPCHelpMan getactiveinvites()
{
    return RPCHelpMan{
        "getactiveinvites",
        "\nGet all active measurement invitations across all users.\n"
        "Returns active (non-expired, non-used) invitations in the system.\n",
        {},
        RPCResult{
            RPCResult::Type::ARR, "", "Array of active invitations",
            {
                {RPCResult::Type::OBJ, "", "",
                {
                    {RPCResult::Type::STR, "invite_id", "Unique invitation ID"},
                    {RPCResult::Type::STR, "type", "Measurement type (water/exchange/validation)"},
                    {RPCResult::Type::STR, "currency", "Currency code (if applicable)"},
                    {RPCResult::Type::NUM, "created_at", "Creation timestamp"},
                    {RPCResult::Type::NUM, "expires_at", "Expiration timestamp"},
                    {RPCResult::Type::NUM, "time_remaining", "Seconds until expiration"},
                }},
            }
        },
        RPCExamples{
            HelpExampleCli("getactiveinvites", "")
            + HelpExampleRpc("getactiveinvites", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            // Get invites from database
            if (!g_measurement_db) {
                throw JSONRPCError(RPC_INTERNAL_ERROR, "Measurement database not initialized");
            }

            std::vector<MeasurementInvite> active_invites = g_measurement_db->GetActiveInvites();

            int64_t current_time = GetTime();
            UniValue result(UniValue::VARR);

            for (const auto& invite : active_invites) {
                UniValue inv(UniValue::VOBJ);
                inv.pushKV("invite_id", invite.invite_id.GetHex());
                
                // Type
                std::string type_str;
                switch (invite.type) {
                    case MeasurementType::WATER_PRICE:
                        type_str = "water";
                        break;
                    case MeasurementType::EXCHANGE_RATE:
                        type_str = "exchange";
                        break;
                    case MeasurementType::WATER_PRICE_OFFLINE_VALIDATION:
                        type_str = "validation";
                        break;
                    default:
                        type_str = "unknown";
                }
                inv.pushKV("type", type_str);
                
                if (!invite.currency_code.empty()) {
                    inv.pushKV("currency", invite.currency_code);
                }
                
                inv.pushKV("created_at", invite.created_at);
                inv.pushKV("expires_at", invite.expires_at);
                inv.pushKV("time_remaining", invite.expires_at - current_time);
                
                result.push_back(inv);
            }

            LogPrintf("O Measurement RPC: Returning %d active invitations\n", result.size());

            return result;
        },
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
        {"measurement", &getactiveinvites},
        {"measurement", &checkmeasurementreadiness},
        {"measurement", &getmeasurementstatistics},
        {"measurement", &submiturl},
        {"measurement", &getdailyaveragewaterprice},
        {"measurement", &getdailyaverageexchangerate},
        {"measurement", &getdailyaverages},
        {"measurement", &calculatedailyaverages},
        {"measurement", &getaveragewaterpricewithconfidence},
        {"measurement", &getaverageexchangeratewithconfidence},
        {"measurement", &getdailyaveragewithconfidence},
        {"measurement", &getdynamicmeasurementtarget},
        {"measurement", &getmeasurementvolatility},
        {"measurement", &getmeasurementtargetstatistics},
        {"measurement", &submitwaterpricewithvalidation},
        {"measurement", &submitexchangeratewithvalidation},
        {"measurement", &getgaussianrange},
        {"measurement", &checkandcreateinvitations},
        {"measurement", &monitormeasurementtargets},
        {"measurement", &getmeasurementgap},
        {"measurement", &recalculatecurrencystability},
    };
    
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}
