// Copyright (c) 2024 The Bitcoin Core developers
// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_O_CURRENCY_LIFECYCLE_RPC_H
#define BITCOIN_RPC_O_CURRENCY_LIFECYCLE_RPC_H

#include <univalue.h>
#include <rpc/request.h>

class CRPCTable;

/**
 * Register O blockchain currency lifecycle RPC commands
 * @param t RPC command table
 */
void RegisterOCurrencyLifecycleRPCCommands(CRPCTable& t);

// Currency Lifecycle Management
UniValue getcurrencystatus(const JSONRPCRequest& request);
UniValue listcurrencies(const JSONRPCRequest& request);
UniValue getcurrencystatistics(const JSONRPCRequest& request);

// Currency Replacement Scenarios
UniValue replacecurrencywithexisting(const JSONRPCRequest& request);
UniValue replacecurrencywithnew(const JSONRPCRequest& request);
UniValue converttooonly(const JSONRPCRequest& request);

// O_ONLY Currency Management
UniValue validateoonlycurrency(const JSONRPCRequest& request);
UniValue getoonlystability(const JSONRPCRequest& request);
UniValue updateoonlystability(const JSONRPCRequest& request);

// Fallback Data Management
UniValue addfallbacksource(const JSONRPCRequest& request);
UniValue getfallbackdata(const JSONRPCRequest& request);
UniValue listfallbacksources(const JSONRPCRequest& request);
UniValue removefallbacksource(const JSONRPCRequest& request);

// Emergency Mechanisms
UniValue triggeremergencystabilization(const JSONRPCRequest& request);
UniValue checkemergencystabilization(const JSONRPCRequest& request);
UniValue getemergencystabilizationamount(const JSONRPCRequest& request);

// Data Quality Management
UniValue getdataquality(const JSONRPCRequest& request);
UniValue getdatafreshness(const JSONRPCRequest& request);
UniValue listcurrencieswithpoordata(const JSONRPCRequest& request);

// Regional Currency Management
UniValue getregionalgroups(const JSONRPCRequest& request);
UniValue getcurrencyregion(const JSONRPCRequest& request);
UniValue getregionalproxy(const JSONRPCRequest& request);

// Migration Management
UniValue getcurrenciesrequiringmigration(const JSONRPCRequest& request);
UniValue checkmigrationdeadline(const JSONRPCRequest& request);
UniValue getmigrationdeadline(const JSONRPCRequest& request);

// Statistics and Reporting
UniValue getfallbackstatistics(const JSONRPCRequest& request);
UniValue getdisappearancestatistics(const JSONRPCRequest& request);
UniValue getoonlystatistics(const JSONRPCRequest& request);

#endif // BITCOIN_RPC_O_CURRENCY_LIFECYCLE_RPC_H
