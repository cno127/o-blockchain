#ifndef BITCOIN_RPC_O_CURRENCY_RPC_H
#define BITCOIN_RPC_O_CURRENCY_RPC_H

#include <univalue.h>

class CRPCTable;

/**
 * Register O blockchain currency RPC commands
 * @param t RPC command table
 */
void RegisterOCurrencyRPCCommands(CRPCTable& t);

UniValue listcurrencies(const JSONRPCRequest& request);
UniValue getcurrency(const JSONRPCRequest& request);
UniValue registercurrency(const JSONRPCRequest& request);

#endif // BITCOIN_RPC_O_CURRENCY_RPC_H

