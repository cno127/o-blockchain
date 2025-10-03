#include <rpc/o_currency_rpc.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <univalue.h>
#include <consensus/multicurrency.h>

#include <string>
#include <vector>

static RPCHelpMan listcurrencies()
{
    return RPCHelpMan{"listcurrencies",
        "\nList all registered currencies in the O blockchain system.\n",
        {
            {"category", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Filter by category: 'all', 'fiat', 'crypto', 'water'"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::NUM, "total", "Total number of registered currencies"},
                {RPCResult::Type::ARR, "currencies", "List of currencies",
                {
                    {RPCResult::Type::OBJ, "", "Currency metadata",
                    {
                        {RPCResult::Type::NUM, "id", "Currency ID"},
                        {RPCResult::Type::STR, "symbol", "Currency symbol (3-letter code)"},
                        {RPCResult::Type::STR, "name", "Full currency name"},
                        {RPCResult::Type::NUM, "decimals", "Number of decimal places"},
                        {RPCResult::Type::BOOL, "is_fiat", "Whether this is a fiat currency"},
                        {RPCResult::Type::STR, "issuer_address", "Issuer address (if applicable)"},
                    }},
                }},
            }
        },
        RPCExamples{
            HelpExampleCli("listcurrencies", "")
            + HelpExampleCli("listcurrencies", "fiat")
            + HelpExampleRpc("listcurrencies", "")
            + HelpExampleRpc("listcurrencies", "\"water\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            std::string category = "all";
            if (request.params.size() > 0) {
                category = request.params[0].get_str();
            }

            UniValue result(UniValue::VOBJ);
            auto all_currencies = g_currency_registry.GetAllCurrencies();
            
            std::vector<UniValue> filtered_currencies;
            
            for (const auto& currency : all_currencies) {
                bool include = false;
                
                if (category == "all") {
                    include = true;
                } else if (category == "fiat" && currency.is_fiat) {
                    include = true;
                } else if (category == "crypto" && !currency.is_fiat) {
                    include = true;
                } else if (category == "water" && currency.is_fiat) {
                    include = true;
                }
                
                if (include) {
                    UniValue currency_obj(UniValue::VOBJ);
                    currency_obj.pushKV("id", static_cast<int64_t>(currency.id));
                    currency_obj.pushKV("symbol", currency.symbol);
                    currency_obj.pushKV("name", currency.name);
                    currency_obj.pushKV("decimals", static_cast<int64_t>(currency.decimals));
                    currency_obj.pushKV("is_fiat", currency.is_fiat);
                    currency_obj.pushKV("issuer_address", currency.issuer_address);
                    filtered_currencies.push_back(currency_obj);
                }
            }
            
            result.pushKV("total", static_cast<int64_t>(filtered_currencies.size()));
            result.pushKV("currencies", filtered_currencies);
            
            return result;
        }
    };
}

static RPCHelpMan getcurrency()
{
    return RPCHelpMan{"getcurrency",
        "\nGet detailed information about a specific currency.\n",
        {
            {"symbol_or_id", RPCArg::Type::STR, RPCArg::Optional::NO, "Currency symbol (e.g., 'USD') or ID (e.g., 1)"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "Currency metadata",
            {
                {RPCResult::Type::NUM, "id", "Currency ID"},
                {RPCResult::Type::STR, "symbol", "Currency symbol (3-letter code)"},
                {RPCResult::Type::STR, "name", "Full currency name"},
                {RPCResult::Type::NUM, "decimals", "Number of decimal places"},
                {RPCResult::Type::BOOL, "is_fiat", "Whether this is a fiat currency"},
                {RPCResult::Type::STR, "issuer_address", "Issuer address (if applicable)"},
            }
        },
        RPCExamples{
            HelpExampleCli("getcurrency", "USD")
            + HelpExampleCli("getcurrency", "1")
            + HelpExampleRpc("getcurrency", "\"EUR\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            std::string symbol_or_id = request.params[0].get_str();
            
            std::optional<CurrencyMetadata> metadata;
            
            // Try as symbol first
            auto id = g_currency_registry.GetCurrencyId(symbol_or_id);
            if (id) {
                metadata = g_currency_registry.GetCurrency(*id);
            } else {
                // Try as ID
                try {
                    CurrencyId currency_id = static_cast<CurrencyId>(std::stoul(symbol_or_id));
                    metadata = g_currency_registry.GetCurrency(currency_id);
                } catch (const std::exception& e) {
                    throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid currency symbol or ID");
                }
            }
            
            if (!metadata) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Currency not found");
            }
            
            UniValue result(UniValue::VOBJ);
            result.pushKV("id", static_cast<int64_t>(metadata->id));
            result.pushKV("symbol", metadata->symbol);
            result.pushKV("name", metadata->name);
            result.pushKV("decimals", static_cast<int64_t>(metadata->decimals));
            result.pushKV("is_fiat", metadata->is_fiat);
            result.pushKV("issuer_address", metadata->issuer_address);
            
            return result;
        }
    };
}

static RPCHelpMan registercurrency()
{
    return RPCHelpMan{"registercurrency",
        "\nRegister a new currency in the O blockchain system (requires special permissions).\n",
        {
            {"id", RPCArg::Type::NUM, RPCArg::Optional::NO, "Currency ID (must be unique)"},
            {"symbol", RPCArg::Type::STR, RPCArg::Optional::NO, "Currency symbol (3-letter code)"},
            {"name", RPCArg::Type::STR, RPCArg::Optional::NO, "Full currency name"},
            {"decimals", RPCArg::Type::NUM, RPCArg::Optional::NO, "Number of decimal places"},
            {"is_fiat", RPCArg::Type::BOOL, RPCArg::Optional::NO, "Whether this is a fiat currency"},
            {"issuer_address", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Issuer address (optional)"},
        },
        RPCResult{
            RPCResult::Type::BOOL, "", "true if currency was registered successfully"
        },
        RPCExamples{
            HelpExampleCli("registercurrency", "100 \"TEST\" \"Test Currency\" 2 true")
            + HelpExampleRpc("registercurrency", "100, \"TEST\", \"Test Currency\", 2, true")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            // Note: In a real implementation, this would require special permissions
            CurrencyId id = static_cast<CurrencyId>(request.params[0].get_int64());
            std::string symbol = request.params[1].get_str();
            std::string name = request.params[2].get_str();
            uint8_t decimals = static_cast<uint8_t>(request.params[3].get_int64());
            bool is_fiat = request.params[4].get_bool();
            
            std::string issuer_address = "";
            if (request.params.size() > 5) {
                issuer_address = request.params[5].get_str();
            }
            
            CurrencyMetadata metadata(id, symbol, name, decimals, is_fiat, issuer_address);
            
            bool success = g_currency_registry.RegisterCurrency(metadata);
            
            if (!success) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Failed to register currency (ID or symbol may already exist)");
            }
            
            return success;
        }
    };
}

void RegisterOCurrencyRPCCommands(CRPCTable& t)
{
    static const CRPCCommand commands[] =
    {
        {"o", &listcurrencies},
        {"o", &getcurrency},
        {"o", &registercurrency},
    };
    for (const auto& c : commands)
    {
        t.appendCommand(c.name, &c);
    }
}
