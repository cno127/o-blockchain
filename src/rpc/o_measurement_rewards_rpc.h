// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_O_MEASUREMENT_REWARDS_RPC_H
#define BITCOIN_RPC_O_MEASUREMENT_REWARDS_RPC_H

#include <univalue.h>
#include <rpc/request.h>

class CRPCTable;

/**
 * Register O blockchain measurement rewards RPC commands
 * @param t RPC command table
 */
void RegisterOMeasurementRewardsRPCCommands(CRPCTable& t);

UniValue getmeasurementrewardstats(const JSONRPCRequest& request);
UniValue getpendingmeasurementrewards(const JSONRPCRequest& request);
UniValue calculatemeasurementrewards(const JSONRPCRequest& request);

#endif // BITCOIN_RPC_O_MEASUREMENT_REWARDS_RPC_H
