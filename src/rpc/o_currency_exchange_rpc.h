// Copyright (c) 2024 The Bitcoin Core developers
// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_O_CURRENCY_EXCHANGE_RPC_H
#define BITCOIN_RPC_O_CURRENCY_EXCHANGE_RPC_H

#include <univalue.h>
#include <rpc/request.h>

class CRPCTable;

/**
 * Register O blockchain currency exchange RPC commands
 * @param t RPC command table
 */
void RegisterOCurrencyExchangeRPCCommands(CRPCTable& t);

UniValue exchangecurrency(const JSONRPCRequest& request);
UniValue getexchangerate(const JSONRPCRequest& request);
UniValue estimateexchange(const JSONRPCRequest& request);
UniValue validateexchangerate(const JSONRPCRequest& request);
UniValue getexchangehistory(const JSONRPCRequest& request);
UniValue getexchangestatistics(const JSONRPCRequest& request);
UniValue getsupportedpairs(const JSONRPCRequest& request);

#endif // BITCOIN_RPC_O_CURRENCY_EXCHANGE_RPC_H
