// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_O_MEASUREMENT_RPC_H
#define BITCOIN_RPC_O_MEASUREMENT_RPC_H

#include <univalue.h>
#include <rpc/request.h>

class CRPCTable;

/** Register O Measurement RPC commands */
void RegisterOMeasurementRPCCommands(CRPCTable& t);

// Daily average RPC functions
UniValue getdailyaveragewaterprice(const JSONRPCRequest& request);
UniValue getdailyaverageexchangerate(const JSONRPCRequest& request);
UniValue checkmeasurementreadiness(const JSONRPCRequest& request);
UniValue getdailyaverages(const JSONRPCRequest& request);
UniValue calculatedailyaverages(const JSONRPCRequest& request);

#endif // BITCOIN_RPC_O_MEASUREMENT_RPC_H
