// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_O_STABILIZATION_INTEGRATION_RPC_H
#define BITCOIN_RPC_O_STABILIZATION_INTEGRATION_RPC_H

#include <univalue.h>
#include <rpc/request.h>

class CRPCTable;

/**
 * Register O blockchain stabilization integration RPC commands
 * @param t RPC command table
 */
void RegisterOStabilizationIntegrationRPCCommands(CRPCTable& t);

UniValue getstabilizationintegrationstatus(const JSONRPCRequest& request);
UniValue getstabilizationconsensusstats(const JSONRPCRequest& request);
UniValue getstabilizationcoinstats(const JSONRPCRequest& request);
UniValue validatestabilizationblock(const JSONRPCRequest& request);
UniValue getstabilizationconsensushash(const JSONRPCRequest& request);

#endif // BITCOIN_RPC_O_STABILIZATION_INTEGRATION_RPC_H
