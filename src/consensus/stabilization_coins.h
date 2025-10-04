// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_STABILIZATION_COINS_H
#define BITCOIN_CONSENSUS_STABILIZATION_COINS_H

#include <consensus/amount.h>
#include <primitives/transaction.h>
#include <coins.h>
#include <uint256.h>
#include <pubkey.h>

#include <map>
#include <vector>
#include <optional>

namespace OConsensus {

/** Stabilization Coin Database Manager */
class StabilizationCoinsManager {
public:
    StabilizationCoinsManager();
    
    /** Update coin database with stabilization transactions */
    void UpdateCoinsWithStabilization(const std::vector<CTransaction>& stab_txs, 
                                     CCoinsViewCache& view, 
                                     int height);
    
    /** Validate stabilization transaction outputs */
    bool ValidateStabilizationOutputs(const CTransaction& stab_tx, 
                                     const CCoinsViewCache& view) const;
    
    /** Get stabilization coin statistics */
    struct StabilizationCoinStats {
        CAmount total_coins_created;
        int total_transactions;
        int total_recipients;
        std::map<std::string, CAmount> coins_by_currency;
        
        StabilizationCoinStats() 
            : total_coins_created(0), total_transactions(0), total_recipients(0) {}
    };
    
    StabilizationCoinStats GetStabilizationStats() const;
    
    /** Check if a transaction is a stabilization transaction */
    bool IsStabilizationTransaction(const CTransaction& tx) const;
    
    /** Get stabilization transaction info */
    struct StabilizationTxInfo {
        std::string unstable_currency;
        CAmount total_coins_created;
        int recipient_count;
        double deviation_ratio;
        
        StabilizationTxInfo() 
            : unstable_currency(), total_coins_created(0), recipient_count(0), deviation_ratio(0.0) {}
    };
    
    std::optional<StabilizationTxInfo> GetStabilizationTxInfo(const uint256& tx_hash) const;
    
    /** Record stabilization transaction for tracking */
    void RecordStabilizationTransaction(const uint256& tx_hash, 
                                       const std::string& currency,
                                       CAmount coins_created,
                                       int recipient_count,
                                       double deviation_ratio);

private:
    std::map<uint256, StabilizationTxInfo> m_stabilization_txs;
    StabilizationCoinStats m_stats;
    
    /** Generate unique output script for stabilization coins */
    CScript GenerateStabilizationScript(const CPubKey& recipient, 
                                       const std::string& currency) const;
    
    /** Validate stabilization coin creation limits */
    bool ValidateStabilizationLimits(CAmount total_coins, int recipient_count) const;
};

/** Global stabilization coins manager instance */
extern StabilizationCoinsManager g_stabilization_coins_manager;

} // namespace OConsensus

#endif // BITCOIN_CONSENSUS_STABILIZATION_COINS_H
