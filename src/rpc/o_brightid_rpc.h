// Copyright (c) 2024 The Bitcoin Core developers
// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_O_BRIGHTID_RPC_H
#define BITCOIN_RPC_O_BRIGHTID_RPC_H

#include <univalue.h>
#include <rpc/request.h>

class CRPCTable;

/**
 * Register O blockchain BrightID integration RPC commands
 * @param t RPC command table
 */
void RegisterOBrightIDRPCCommands(CRPCTable& t);

// BrightID Integration Management
UniValue initializebrightid(const JSONRPCRequest& request);
UniValue getbrightidstatus(const JSONRPCRequest& request);
UniValue getbrightidstatistics(const JSONRPCRequest& request);

// User Verification
UniValue verifyuser(const JSONRPCRequest& request);
UniValue getuserstatus(const JSONRPCRequest& request);
UniValue verifybysocialgraph(const JSONRPCRequest& request);
UniValue verifybysponsorship(const JSONRPCRequest& request);
UniValue verifybymeetup(const JSONRPCRequest& request);
UniValue verifybycommunity(const JSONRPCRequest& request);

// Privacy-Preserving Features
UniValue generateanonymousid(const JSONRPCRequest& request);
UniValue anonymousverification(const JSONRPCRequest& request);
UniValue getprivacyreputation(const JSONRPCRequest& request);
UniValue updateprivacyreputation(const JSONRPCRequest& request);

// O Blockchain Integration
UniValue registeruser(const JSONRPCRequest& request);
UniValue linkaddress(const JSONRPCRequest& request);
UniValue unlinkaddress(const JSONRPCRequest& request);
UniValue getoaddress(const JSONRPCRequest& request);
UniValue getbrightidaddress(const JSONRPCRequest& request);

// Social Graph Analysis
UniValue analyzesocialgraph(const JSONRPCRequest& request);
UniValue getuserconnections(const JSONRPCRequest& request);
UniValue calculatetrustscore(const JSONRPCRequest& request);
UniValue detectsybilattack(const JSONRPCRequest& request);

// Configuration
UniValue setverificationrequirements(const JSONRPCRequest& request);
UniValue getverificationrequirements(const JSONRPCRequest& request);
UniValue setprivacysettings(const JSONRPCRequest& request);
UniValue getprivacysettings(const JSONRPCRequest& request);

// Utility Functions
UniValue validatebrightidaddress(const JSONRPCRequest& request);
UniValue generatecontextid(const JSONRPCRequest& request);
UniValue cleanupverifications(const JSONRPCRequest& request);

// Data Management
UniValue backupbrightiddata(const JSONRPCRequest& request);
UniValue restorebrightiddata(const JSONRPCRequest& request);
UniValue clearbrightiddata(const JSONRPCRequest& request);

#endif // BITCOIN_RPC_O_BRIGHTID_RPC_H
