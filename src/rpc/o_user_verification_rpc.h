// Copyright (c) 2024 The Bitcoin Core developers
// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_O_USER_VERIFICATION_RPC_H
#define BITCOIN_RPC_O_USER_VERIFICATION_RPC_H

#include <rpc/request.h>
#include <univalue.h>

/** RPC commands for O Blockchain User Verification System */
namespace OUserVerificationRPC {

/** Get recommended verification method for a country */
UniValue getrecommendedverificationmethod(const JSONRPCRequest& request);

/** Get all allowed verification methods for a country */
UniValue getallowedverificationmethods(const JSONRPCRequest& request);

/** Check if a country requires KYC */
UniValue doescountryrequirekyc(const JSONRPCRequest& request);

/** Get user verification status */
UniValue getuserverificationstatus(const JSONRPCRequest& request);

/** Register all O User Verification RPC commands */
void RegisterOUserVerificationRPCCommands();

} // namespace OUserVerificationRPC

#endif // BITCOIN_RPC_O_USER_VERIFICATION_RPC_H
