// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_O_EXCHANGE_RATE_INIT_RPC_H
#define BITCOIN_RPC_O_EXCHANGE_RATE_INIT_RPC_H

#include <univalue.h>
#include <rpc/request.h>

class CRPCTable;

/**
 * Register O blockchain exchange rate initialization RPC commands
 * @param t RPC command table
 */
void RegisterOExchangeRateInitRPCCommands(CRPCTable& t);

UniValue initializeexchangerates(const JSONRPCRequest& request);
UniValue getexchangeratestatus(const JSONRPCRequest& request);
UniValue getexchangeratestatistics(const JSONRPCRequest& request);
UniValue getminimummeasurementsthreshold(const JSONRPCRequest& request);

#endif // BITCOIN_RPC_O_EXCHANGE_RATE_INIT_RPC_H
