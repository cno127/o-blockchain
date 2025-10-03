// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_O_USER_RPC_H
#define BITCOIN_RPC_O_USER_RPC_H

#include <rpc/server.h>

/**
 * Register O blockchain user RPC commands
 * @param t RPC command table
 */
void RegisterOBlockchainRPCCommands(CRPCTable& t);

#endif // BITCOIN_RPC_O_USER_RPC_H
