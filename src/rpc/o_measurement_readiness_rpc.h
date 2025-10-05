#ifndef BITCOIN_RPC_O_MEASUREMENT_READINESS_RPC_H
#define BITCOIN_RPC_O_MEASUREMENT_READINESS_RPC_H

class CRPCTable;
class UniValue;
class JSONRPCRequest;

void RegisterOMeasurementReadinessRPCCommands(CRPCTable &tableRPC);

UniValue updateusercount(const JSONRPCRequest& request);
UniValue updatecoinsupply(const JSONRPCRequest& request);
UniValue getreadinessstatus(const JSONRPCRequest& request);
UniValue getreadinessstatistics(const JSONRPCRequest& request);
UniValue getreadyforwaterprice(const JSONRPCRequest& request);
UniValue getreadyforexchangerate(const JSONRPCRequest& request);
UniValue getfullyreadycurrencies(const JSONRPCRequest& request);
UniValue checkmeasurementreadiness(const JSONRPCRequest& request);

#endif // BITCOIN_RPC_O_MEASUREMENT_READINESS_RPC_H
