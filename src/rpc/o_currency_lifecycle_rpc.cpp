// Copyright (c) 2024 The Bitcoin Core developers
// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/o_currency_lifecycle_rpc.h>
#include <consensus/currency_lifecycle.h>
#include <consensus/currency_disappearance_handling.h>
#include <rpc/server.h>
#include <rpc/server_util.h>
#include <rpc/util.h>
#include <util/strencodings.h>
#include <validation.h>
#include <node/context.h>
#include <chain.h>
#include <rpc/request.h>

#include <string>
#include <vector>

// ===== Currency Lifecycle Management =====

UniValue getcurrencystatus(const JSONRPCRequest& request)
{
    if (request.params.size() != 1) {
        throw std::runtime_error(
            "getcurrencystatus \"currency\"\n"
            "\nGet the lifecycle status of a currency.\n"
            "\nArguments:\n"
            "1. currency          (string, required) Currency code (e.g., \"OUSD\")\n"
            "\nResult:\n"
            "{\n"
            "  \"currency\": \"OUSD\",\n"
            "  \"status\": \"ACTIVE\",\n"
            "  \"status_description\": \"Active - Normal operation\",\n"
            "  \"status_reason\": \"Currency is operating normally\",\n"
            "  \"status_change_height\": 12345,\n"
            "  \"last_data_height\": 12340,\n"
            "  \"data_dry_period\": 5,\n"
            "  \"is_water_price_stable\": true,\n"
            "  \"is_exchange_rate_stable\": true,\n"
            "  \"replacement_info\": null\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getcurrencystatus", "\"OUSD\"")
            + HelpExampleRpc("getcurrencystatus", "\"OUSD\"")
        );
    }

    std::string currency = request.params[0].get_str();
    if (currency.empty()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Currency code cannot be empty");
    }

    auto info = g_currency_lifecycle_manager.GetCurrencyInfo(currency);
    if (!info.has_value()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Currency not found: " + currency);
    }

    UniValue result(UniValue::VOBJ);
    result.pushKV("currency", currency);
    result.pushKV("status", static_cast<int>(info->status));
    result.pushKV("status_description", g_currency_lifecycle_manager.GetStatusDescription(info->status));
    result.pushKV("status_reason", info->status_reason);
    result.pushKV("status_change_height", info->status_change_height);
    result.pushKV("last_data_height", info->last_data_height);
    result.pushKV("data_dry_period", info->data_dry_period);
    result.pushKV("is_water_price_stable", info->is_water_price_stable);
    result.pushKV("is_exchange_rate_stable", info->is_exchange_rate_stable);
    
    if (info->replacement_info.has_value()) {
        UniValue replacement(UniValue::VOBJ);
        replacement.pushKV("old_currency", info->replacement_info->old_currency);
        replacement.pushKV("new_currency", info->replacement_info->new_currency);
        replacement.pushKV("conversion_rate", info->replacement_info->conversion_rate);
        replacement.pushKV("replacement_height", info->replacement_info->replacement_height);
        replacement.pushKV("migration_deadline", info->replacement_info->migration_deadline);
        replacement.pushKV("reason", info->replacement_info->reason);
        replacement.pushKV("is_mandatory", info->replacement_info->is_mandatory);
        result.pushKV("replacement_info", replacement);
    } else {
        result.pushKV("replacement_info", UniValue::VNULL);
    }

    return result;
}

UniValue listcurrencies(const JSONRPCRequest& request)
{
    if (request.params.size() > 1) {
        throw std::runtime_error(
            "listcurrencies [status]\n"
            "\nList all currencies with their lifecycle status.\n"
            "\nArguments:\n"
            "1. status            (string, optional) Filter by status (ACTIVE, REPLACED, NEW_CURRENCY, O_ONLY, DEPRECATED)\n"
            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"currency\": \"OUSD\",\n"
            "    \"status\": \"ACTIVE\",\n"
            "    \"status_description\": \"Active - Normal operation\"\n"
            "  },\n"
            "  ...\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("listcurrencies", "")
            + HelpExampleCli("listcurrencies", "\"ACTIVE\"")
            + HelpExampleRpc("listcurrencies", "")
        );
    }

    std::string status_filter;
    if (request.params.size() > 0) {
        status_filter = request.params[0].get_str();
    }

    UniValue result(UniValue::VARR);
    
    // Get all currencies (this would typically come from the currency registry)
    std::vector<std::string> all_currencies = {
        "OUSD", "OEUR", "OJPY", "OGBP", "OCAD", "OAUD", "OCHF", "OCNY", "OKRW", "OMXN"
    };
    
    for (const auto& currency : all_currencies) {
        auto info = g_currency_lifecycle_manager.GetCurrencyInfo(currency);
        if (!info.has_value()) continue;
        
        if (!status_filter.empty()) {
            std::string status_str = g_currency_lifecycle_manager.GetStatusDescription(info->status);
            if (status_str.find(status_filter) == std::string::npos) {
                continue;
            }
        }
        
        UniValue currency_info(UniValue::VOBJ);
        currency_info.pushKV("currency", currency);
        currency_info.pushKV("status", static_cast<int>(info->status));
        currency_info.pushKV("status_description", g_currency_lifecycle_manager.GetStatusDescription(info->status));
        result.push_back(currency_info);
    }

    return result;
}

UniValue getcurrencystatistics(const JSONRPCRequest& request)
{
    if (request.params.size() != 0) {
        throw std::runtime_error(
            "getcurrencystatistics\n"
            "\nGet currency lifecycle statistics.\n"
            "\nResult:\n"
            "{\n"
            "  \"total_currencies\": 10,\n"
            "  \"active_currencies\": 8,\n"
            "  \"replaced_currencies\": 1,\n"
            "  \"o_only_currencies\": 1,\n"
            "  \"deprecated_currencies\": 0,\n"
            "  \"total_replacements\": 1\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getcurrencystatistics", "")
            + HelpExampleRpc("getcurrencystatistics", "")
        );
    }

    auto stats = g_currency_lifecycle_manager.GetLifecycleStatistics();
    
    UniValue result(UniValue::VOBJ);
    for (const auto& [key, value] : stats) {
        result.pushKV(key, value);
    }

    return result;
}

// ===== Currency Replacement Scenarios =====

UniValue replacecurrencywithexisting(const JSONRPCRequest& request)
{
    if (request.params.size() != 4) {
        throw std::runtime_error(
            "replacecurrencywithexisting \"old_currency\" \"new_currency\" conversion_rate \"reason\"\n"
            "\nReplace a currency with an existing currency (Scenario 1).\n"
            "\nArguments:\n"
            "1. old_currency      (string, required) Currency to be replaced\n"
            "2. new_currency      (string, required) Existing currency to replace with\n"
            "3. conversion_rate   (numeric, required) Conversion rate (old to new)\n"
            "4. reason            (string, required) Reason for replacement\n"
            "\nResult:\n"
            "{\n"
            "  \"success\": true,\n"
            "  \"old_currency\": \"OLD\",\n"
            "  \"new_currency\": \"NEW\",\n"
            "  \"conversion_rate\": 100.0,\n"
            "  \"replacement_height\": 12345,\n"
            "  \"migration_deadline\": 12945\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("replacecurrencywithexisting", "\"OLD\" \"NEW\" 100.0 \"Currency redenomination\"")
            + HelpExampleRpc("replacecurrencywithexisting", "\"OLD\", \"NEW\", 100.0, \"Currency redenomination\"")
        );
    }

    std::string old_currency = request.params[0].get_str();
    std::string new_currency = request.params[1].get_str();
    double conversion_rate = request.params[2].get_real();
    std::string reason = request.params[3].get_str();

    if (old_currency.empty() || new_currency.empty()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Currency codes cannot be empty");
    }

    if (conversion_rate <= 0.0) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Conversion rate must be positive");
    }

    if (reason.empty()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Reason cannot be empty");
    }

    // Get current block height
    const node::NodeContext& node = EnsureAnyNodeContext(request.context);
    int height = node.chainman->ActiveChain().Height();

    // Handle the replacement
    bool success = g_currency_disappearance_handler.HandleCurrencyReplacement(
        old_currency, new_currency, conversion_rate, reason, height);

    if (!success) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Failed to replace currency");
    }

    UniValue result(UniValue::VOBJ);
    result.pushKV("success", true);
    result.pushKV("old_currency", old_currency);
    result.pushKV("new_currency", new_currency);
    result.pushKV("conversion_rate", conversion_rate);
    result.pushKV("replacement_height", height);
    result.pushKV("migration_deadline", height + (144 * 30)); // 30 days

    return result;
}

UniValue replacecurrencywithnew(const JSONRPCRequest& request)
{
    if (request.params.size() != 4) {
        throw std::runtime_error(
            "replacecurrencywithnew \"old_currency\" \"new_currency\" conversion_rate \"reason\"\n"
            "\nReplace a currency with a new currency (Scenario 2).\n"
            "\nArguments:\n"
            "1. old_currency      (string, required) Currency to be replaced\n"
            "2. new_currency      (string, required) New currency to replace with\n"
            "3. conversion_rate   (numeric, required) Conversion rate (old to new)\n"
            "4. reason            (string, required) Reason for replacement\n"
            "\nResult:\n"
            "{\n"
            "  \"success\": true,\n"
            "  \"old_currency\": \"OLD\",\n"
            "  \"new_currency\": \"NEW\",\n"
            "  \"conversion_rate\": 100.0,\n"
            "  \"replacement_height\": 12345,\n"
            "  \"migration_deadline\": 12945,\n"
            "  \"system_update_required\": true\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("replacecurrencywithnew", "\"OLD\" \"NEW\" 100.0 \"New currency introduction\"")
            + HelpExampleRpc("replacecurrencywithnew", "\"OLD\", \"NEW\", 100.0, \"New currency introduction\"")
        );
    }

    std::string old_currency = request.params[0].get_str();
    std::string new_currency = request.params[1].get_str();
    double conversion_rate = request.params[2].get_real();
    std::string reason = request.params[3].get_str();

    if (old_currency.empty() || new_currency.empty()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Currency codes cannot be empty");
    }

    if (conversion_rate <= 0.0) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Conversion rate must be positive");
    }

    if (reason.empty()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Reason cannot be empty");
    }

    // Get current block height
    const node::NodeContext& node = EnsureAnyNodeContext(request.context);
    int height = node.chainman->ActiveChain().Height();

    // Handle the replacement
    bool success = g_currency_disappearance_handler.HandleNewCurrencyReplacement(
        old_currency, new_currency, conversion_rate, reason, height);

    if (!success) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Failed to replace currency with new currency");
    }

    UniValue result(UniValue::VOBJ);
    result.pushKV("success", true);
    result.pushKV("old_currency", old_currency);
    result.pushKV("new_currency", new_currency);
    result.pushKV("conversion_rate", conversion_rate);
    result.pushKV("replacement_height", height);
    result.pushKV("migration_deadline", height + (144 * 30)); // 30 days
    result.pushKV("system_update_required", true);

    return result;
}

UniValue converttooonly(const JSONRPCRequest& request)
{
    if (request.params.size() != 2) {
        throw std::runtime_error(
            "converttooonly \"currency\" \"reason\"\n"
            "\nConvert a currency to O_ONLY (Scenario 3).\n"
            "\nArguments:\n"
            "1. currency          (string, required) Currency to convert to O_ONLY\n"
            "2. reason            (string, required) Reason for conversion\n"
            "\nResult:\n"
            "{\n"
            "  \"success\": true,\n"
            "  \"currency\": \"CURRENCY\",\n"
            "  \"conversion_height\": 12345,\n"
            "  \"water_price\": 1.0,\n"
            "  \"exchange_rate\": 1.0,\n"
            "  \"is_stable\": true\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("converttooonly", "\"CURRENCY\" \"Economic collapse\"")
            + HelpExampleRpc("converttooonly", "\"CURRENCY\", \"Economic collapse\"")
        );
    }

    std::string currency = request.params[0].get_str();
    std::string reason = request.params[1].get_str();

    if (currency.empty()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Currency code cannot be empty");
    }

    if (reason.empty()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Reason cannot be empty");
    }

    // Get current block height
    const node::NodeContext& node = EnsureAnyNodeContext(request.context);
    int height = node.chainman->ActiveChain().Height();

    // Handle the conversion
    bool success = g_currency_disappearance_handler.HandleOOnlyConversion(currency, reason, height);

    if (!success) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Failed to convert currency to O_ONLY");
    }

    UniValue result(UniValue::VOBJ);
    result.pushKV("success", true);
    result.pushKV("currency", currency);
    result.pushKV("conversion_height", height);
    result.pushKV("water_price", 1.0);
    result.pushKV("exchange_rate", 1.0);
    result.pushKV("is_stable", true);

    return result;
}

// ===== O_ONLY Currency Management =====

UniValue validateoonlycurrency(const JSONRPCRequest& request)
{
    if (request.params.size() != 3) {
        throw std::runtime_error(
            "validateoonlycurrency \"currency\" water_price exchange_rate\n"
            "\nValidate O_ONLY currency stability.\n"
            "\nArguments:\n"
            "1. currency          (string, required) O_ONLY currency code\n"
            "2. water_price       (numeric, required) Measured water price\n"
            "3. exchange_rate     (numeric, required) Measured exchange rate\n"
            "\nResult:\n"
            "{\n"
            "  \"currency\": \"CURRENCY\",\n"
            "  \"is_valid\": true,\n"
            "  \"water_price_stable\": true,\n"
            "  \"exchange_rate_stable\": true,\n"
            "  \"water_price_deviation\": 0.0,\n"
            "  \"exchange_rate_deviation\": 0.0,\n"
            "  \"tolerance\": 0.01\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("validateoonlycurrency", "\"CURRENCY\" 1.0 1.0")
            + HelpExampleRpc("validateoonlycurrency", "\"CURRENCY\", 1.0, 1.0")
        );
    }

    std::string currency = request.params[0].get_str();
    double water_price = request.params[1].get_real();
    double exchange_rate = request.params[2].get_real();

    if (currency.empty()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Currency code cannot be empty");
    }

    if (water_price <= 0.0 || exchange_rate <= 0.0) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Water price and exchange rate must be positive");
    }

    bool is_valid = g_currency_disappearance_handler.ValidateOOnlyCurrency(currency, water_price, exchange_rate);
    
    double water_price_deviation = std::abs(water_price - 1.0);
    double exchange_rate_deviation = std::abs(exchange_rate - 1.0);
    bool water_price_stable = water_price_deviation <= 0.01;
    bool exchange_rate_stable = exchange_rate_deviation <= 0.01;

    UniValue result(UniValue::VOBJ);
    result.pushKV("currency", currency);
    result.pushKV("is_valid", is_valid);
    result.pushKV("water_price_stable", water_price_stable);
    result.pushKV("exchange_rate_stable", exchange_rate_stable);
    result.pushKV("water_price_deviation", water_price_deviation);
    result.pushKV("exchange_rate_deviation", exchange_rate_deviation);
    result.pushKV("tolerance", 0.01);

    return result;
}

UniValue getoonlystability(const JSONRPCRequest& request)
{
    if (request.params.size() != 1) {
        throw std::runtime_error(
            "getoonlystability \"currency\"\n"
            "\nGet O_ONLY currency stability metrics.\n"
            "\nArguments:\n"
            "1. currency          (string, required) O_ONLY currency code\n"
            "\nResult:\n"
            "{\n"
            "  \"currency\": \"CURRENCY\",\n"
            "  \"water_price\": 1.0,\n"
            "  \"exchange_rate\": 1.0,\n"
            "  \"is_stable\": true,\n"
            "  \"water_price_deviation\": 0.0,\n"
            "  \"exchange_rate_deviation\": 0.0\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getoonlystability", "\"CURRENCY\"")
            + HelpExampleRpc("getoonlystability", "\"CURRENCY\"")
        );
    }

    std::string currency = request.params[0].get_str();
    if (currency.empty()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Currency code cannot be empty");
    }

    auto stability_metrics = g_currency_disappearance_handler.GetOOnlyStabilityMetrics(currency);
    bool is_stable = g_currency_disappearance_handler.IsOOnlyCurrencyStable(currency);
    
    double water_price_deviation = std::abs(stability_metrics.first - 1.0);
    double exchange_rate_deviation = std::abs(stability_metrics.second - 1.0);

    UniValue result(UniValue::VOBJ);
    result.pushKV("currency", currency);
    result.pushKV("water_price", stability_metrics.first);
    result.pushKV("exchange_rate", stability_metrics.second);
    result.pushKV("is_stable", is_stable);
    result.pushKV("water_price_deviation", water_price_deviation);
    result.pushKV("exchange_rate_deviation", exchange_rate_deviation);

    return result;
}

UniValue updateoonlystability(const JSONRPCRequest& request)
{
    if (request.params.size() != 3) {
        throw std::runtime_error(
            "updateoonlystability \"currency\" water_price exchange_rate\n"
            "\nUpdate O_ONLY currency stability metrics.\n"
            "\nArguments:\n"
            "1. currency          (string, required) O_ONLY currency code\n"
            "2. water_price       (numeric, required) Measured water price\n"
            "3. exchange_rate     (numeric, required) Measured exchange rate\n"
            "\nResult:\n"
            "{\n"
            "  \"success\": true,\n"
            "  \"currency\": \"CURRENCY\",\n"
            "  \"water_price\": 1.0,\n"
            "  \"exchange_rate\": 1.0,\n"
            "  \"is_stable\": true\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("updateoonlystability", "\"CURRENCY\" 1.0 1.0")
            + HelpExampleRpc("updateoonlystability", "\"CURRENCY\", 1.0, 1.0")
        );
    }

    std::string currency = request.params[0].get_str();
    double water_price = request.params[1].get_real();
    double exchange_rate = request.params[2].get_real();

    if (currency.empty()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Currency code cannot be empty");
    }

    if (water_price <= 0.0 || exchange_rate <= 0.0) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Water price and exchange rate must be positive");
    }

    g_currency_disappearance_handler.UpdateOOnlyStability(currency, water_price, exchange_rate);
    bool is_stable = g_currency_disappearance_handler.IsOOnlyCurrencyStable(currency);

    UniValue result(UniValue::VOBJ);
    result.pushKV("success", true);
    result.pushKV("currency", currency);
    result.pushKV("water_price", water_price);
    result.pushKV("exchange_rate", exchange_rate);
    result.pushKV("is_stable", is_stable);

    return result;
}

// ===== Emergency Mechanisms =====

UniValue triggeremergencystabilization(const JSONRPCRequest& request)
{
    if (request.params.size() != 1) {
        throw std::runtime_error(
            "triggeremergencystabilization \"currency\"\n"
            "\nTrigger emergency stabilization for an unstable O_ONLY currency.\n"
            "\nArguments:\n"
            "1. currency          (string, required) O_ONLY currency code\n"
            "\nResult:\n"
            "{\n"
            "  \"success\": true,\n"
            "  \"currency\": \"CURRENCY\",\n"
            "  \"emergency_triggered\": true,\n"
            "  \"stabilization_amount\": 1000.00,\n"
            "  \"reason\": \"O_ONLY currency is unstable\"\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("triggeremergencystabilization", "\"CURRENCY\"")
            + HelpExampleRpc("triggeremergencystabilization", "\"CURRENCY\"")
        );
    }

    std::string currency = request.params[0].get_str();
    if (currency.empty()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Currency code cannot be empty");
    }

    // Get current block height
    const node::NodeContext& node = EnsureAnyNodeContext(request.context);
    int height = node.chainman->ActiveChain().Height();

    bool emergency_triggered = g_currency_disappearance_handler.TriggerEmergencyStabilization(currency, height);
    CAmount stabilization_amount = g_currency_disappearance_handler.GetEmergencyStabilizationAmount(currency);

    UniValue result(UniValue::VOBJ);
    result.pushKV("success", true);
    result.pushKV("currency", currency);
    result.pushKV("emergency_triggered", emergency_triggered);
    result.pushKV("stabilization_amount", stabilization_amount);
    result.pushKV("reason", emergency_triggered ? "O_ONLY currency is unstable" : "O_ONLY currency is stable");

    return result;
}

UniValue checkemergencystabilization(const JSONRPCRequest& request)
{
    if (request.params.size() != 1) {
        throw std::runtime_error(
            "checkemergencystabilization \"currency\"\n"
            "\nCheck if emergency stabilization is needed for an O_ONLY currency.\n"
            "\nArguments:\n"
            "1. currency          (string, required) O_ONLY currency code\n"
            "\nResult:\n"
            "{\n"
            "  \"currency\": \"CURRENCY\",\n"
            "  \"emergency_needed\": false,\n"
            "  \"is_stable\": true,\n"
            "  \"water_price_deviation\": 0.0,\n"
            "  \"exchange_rate_deviation\": 0.0\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("checkemergencystabilization", "\"CURRENCY\"")
            + HelpExampleRpc("checkemergencystabilization", "\"CURRENCY\"")
        );
    }

    std::string currency = request.params[0].get_str();
    if (currency.empty()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Currency code cannot be empty");
    }

    bool emergency_needed = g_currency_disappearance_handler.IsEmergencyStabilizationNeeded(currency);
    bool is_stable = g_currency_disappearance_handler.IsOOnlyCurrencyStable(currency);
    auto stability_metrics = g_currency_disappearance_handler.GetOOnlyStabilityMetrics(currency);
    
    double water_price_deviation = std::abs(stability_metrics.first - 1.0);
    double exchange_rate_deviation = std::abs(stability_metrics.second - 1.0);

    UniValue result(UniValue::VOBJ);
    result.pushKV("currency", currency);
    result.pushKV("emergency_needed", emergency_needed);
    result.pushKV("is_stable", is_stable);
    result.pushKV("water_price_deviation", water_price_deviation);
    result.pushKV("exchange_rate_deviation", exchange_rate_deviation);

    return result;
}

UniValue getemergencystabilizationamount(const JSONRPCRequest& request)
{
    if (request.params.size() != 1) {
        throw std::runtime_error(
            "getemergencystabilizationamount \"currency\"\n"
            "\nGet the emergency stabilization amount for an O_ONLY currency.\n"
            "\nArguments:\n"
            "1. currency          (string, required) O_ONLY currency code\n"
            "\nResult:\n"
            "{\n"
            "  \"currency\": \"CURRENCY\",\n"
            "  \"stabilization_amount\": 1000.00,\n"
            "  \"emergency_needed\": false,\n"
            "  \"calculation\": \"Based on water price and exchange rate deviations\"\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getemergencystabilizationamount", "\"CURRENCY\"")
            + HelpExampleRpc("getemergencystabilizationamount", "\"CURRENCY\"")
        );
    }

    std::string currency = request.params[0].get_str();
    if (currency.empty()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Currency code cannot be empty");
    }

    CAmount stabilization_amount = g_currency_disappearance_handler.GetEmergencyStabilizationAmount(currency);
    bool emergency_needed = g_currency_disappearance_handler.IsEmergencyStabilizationNeeded(currency);

    UniValue result(UniValue::VOBJ);
    result.pushKV("currency", currency);
    result.pushKV("stabilization_amount", stabilization_amount);
    result.pushKV("emergency_needed", emergency_needed);
    result.pushKV("calculation", "Based on water price and exchange rate deviations");

    return result;
}

// ===== RPC Command Registration =====

void RegisterOCurrencyLifecycleRPCCommands(CRPCTable& t)
{
    // Currency Lifecycle Management
    static const CRPCCommand commands[] = {
        {"currency_lifecycle", &getcurrencystatus},
        {"currency_lifecycle", &listcurrencies},
        {"currency_lifecycle", &getcurrencystatistics},
        
        // Currency Replacement Scenarios
        {"currency_lifecycle", &replacecurrencywithexisting},
        {"currency_lifecycle", &replacecurrencywithnew},
        {"currency_lifecycle", &converttooonly},
        
        // O_ONLY Currency Management
        {"currency_lifecycle", &validateoonlycurrency},
        {"currency_lifecycle", &getoonlystability},
        {"currency_lifecycle", &updateoonlystability},
        
        // Emergency Mechanisms
        {"currency_lifecycle", &triggeremergencystabilization},
        {"currency_lifecycle", &checkemergencystabilization},
        {"currency_lifecycle", &getemergencystabilizationamount},
    };
    
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}
