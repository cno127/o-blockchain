// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_REGISTER_H
#define BITCOIN_RPC_REGISTER_H

#include <bitcoin-build-config.h> // IWYU pragma: keep

/** These are in one header file to avoid creating tons of single-function
 * headers for everything under src/rpc/ */
class CRPCTable;

void RegisterBlockchainRPCCommands(CRPCTable &tableRPC);
void RegisterFeeRPCCommands(CRPCTable&);
void RegisterMempoolRPCCommands(CRPCTable&);
void RegisterMiningRPCCommands(CRPCTable &tableRPC);
void RegisterNodeRPCCommands(CRPCTable&);
void RegisterNetRPCCommands(CRPCTable&);
void RegisterOutputScriptRPCCommands(CRPCTable&);
void RegisterRawTransactionRPCCommands(CRPCTable &tableRPC);
void RegisterSignMessageRPCCommands(CRPCTable&);
void RegisterSignerRPCCommands(CRPCTable &tableRPC);
void RegisterTxoutProofRPCCommands(CRPCTable&);
// O Blockchain RPC commands
void RegisterOPowPobRPCCommands(CRPCTable &tableRPC);
void RegisterOMeasurementRPCCommands(CRPCTable &tableRPC);
void RegisterOStabilizationRPCCommands(CRPCTable &tableRPC);
void RegisterOCurrencyExchangeRPCCommands(CRPCTable &tableRPC);
void RegisterOStabilizationIntegrationRPCCommands(CRPCTable &tableRPC);
void RegisterOMeasurementRewardsRPCCommands(CRPCTable &tableRPC);
// void RegisterOCurrencyLifecycleRPCCommands(CRPCTable &tableRPC);  // Temporarily disabled
// void RegisterOBrightIDRPCCommands(CRPCTable &tableRPC);  // Temporarily disabled
// void RegisterOBlockchainRPCCommands(CRPCTable &tableRPC);

static inline void RegisterAllCoreRPCCommands(CRPCTable &t)
{
    RegisterBlockchainRPCCommands(t);
    RegisterFeeRPCCommands(t);
    RegisterMempoolRPCCommands(t);
    RegisterMiningRPCCommands(t);
    RegisterNodeRPCCommands(t);
    RegisterNetRPCCommands(t);
    RegisterOutputScriptRPCCommands(t);
    RegisterRawTransactionRPCCommands(t);
    RegisterSignMessageRPCCommands(t);
#ifdef ENABLE_EXTERNAL_SIGNER
    RegisterSignerRPCCommands(t);
#endif // ENABLE_EXTERNAL_SIGNER
    RegisterTxoutProofRPCCommands(t);
           // O Blockchain RPC commands
           RegisterOPowPobRPCCommands(t);
           RegisterOMeasurementRPCCommands(t);
           RegisterOStabilizationRPCCommands(t);
           RegisterOCurrencyExchangeRPCCommands(t);
           RegisterOStabilizationIntegrationRPCCommands(t);
           RegisterOMeasurementRewardsRPCCommands(t);
           // RegisterOCurrencyLifecycleRPCCommands(t);  // Temporarily disabled
           // RegisterOBrightIDRPCCommands(t);  // Temporarily disabled
           // RegisterOBlockchainRPCCommands(t);
}

#endif // BITCOIN_RPC_REGISTER_H
