// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_O_BLOCKCHAIN_TX_RPC_H
#define BITCOIN_RPC_O_BLOCKCHAIN_TX_RPC_H

class CRPCTable;

/**
 * Register O Blockchain transaction RPC commands
 * 
 * These commands create and broadcast blockchain transactions
 * containing O-specific data (USER_VERIFY, WATER_PRICE, EXCHANGE_RATE, etc.)
 */
void RegisterOBlockchainTxRPCCommands(CRPCTable& t);

#endif // BITCOIN_RPC_O_BLOCKCHAIN_TX_RPC_H

