// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_STABILIZATION_CONSENSUS_H
#define BITCOIN_CONSENSUS_STABILIZATION_CONSENSUS_H

#include <consensus/validation.h>
#include <consensus/stabilization_mining.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <uint256.h>

#include <vector>
#include <map>
#include <optional>

namespace OConsensus {

/** Stabilization Consensus Validation Manager */
class StabilizationConsensusValidator {
public:
    StabilizationConsensusValidator();
    
    /** Validate stabilization transactions in a block */
    bool ValidateStabilizationTransactions(const CBlock& block, 
                                          int height,
                                          BlockValidationState& state) const;
    
    /** Validate individual stabilization transaction */
    bool ValidateStabilizationTransaction(const CTransaction& tx,
                                         int height,
                                         TxValidationState& state) const;
    
    /** Check if block should have stabilization transactions */
    bool ShouldBlockHaveStabilization(const CBlock& block, int height) const;
    
    /** Calculate expected stabilization transactions for consensus */
    std::vector<CTransaction> CalculateExpectedStabilizationTransactions(
        const CBlock& block, int height) const;
    
    /** Verify stabilization transaction matches expected */
    bool VerifyStabilizationTransaction(const CTransaction& actual_tx,
                                       const CTransaction& expected_tx) const;
    
    /** Get stabilization consensus statistics */
    struct StabilizationConsensusStats {
        int total_blocks_validated;
        int blocks_with_stabilization;
        int stabilization_transactions_validated;
        int consensus_violations;
        std::map<std::string, int> currency_stabilization_count;
        
        StabilizationConsensusStats() 
            : total_blocks_validated(0), blocks_with_stabilization(0),
              stabilization_transactions_validated(0), consensus_violations(0) {}
    };
    
    StabilizationConsensusStats GetConsensusStats() const;
    
    /** Reset consensus statistics */
    void ResetConsensusStats();
    
    /** Check consensus rules for stabilization */
    bool CheckStabilizationConsensusRules(const CBlock& block, 
                                         int height,
                                         BlockValidationState& state) const;

private:
    mutable StabilizationConsensusStats m_stats;
    
    /** Validate stabilization transaction structure */
    bool ValidateStabilizationStructure(const CTransaction& tx,
                                       TxValidationState& state) const;
    
    /** Validate stabilization transaction amounts */
    bool ValidateStabilizationAmounts(const CTransaction& tx,
                                     TxValidationState& state) const;
    
    /** Validate stabilization transaction recipients */
    bool ValidateStabilizationRecipients(const CTransaction& tx,
                                        TxValidationState& state) const;
    
    /** Check if stabilization is triggered by consensus rules */
    bool IsStabilizationTriggeredByConsensus(const CBlock& block, int height) const;
    
    /** Calculate deterministic stabilization parameters */
    struct StabilizationParams {
        std::vector<std::string> unstable_currencies;
        std::map<std::string, CAmount> coins_per_currency;
        std::map<std::string, int> recipient_counts;
        
        StabilizationParams() {}
    };
    
    StabilizationParams CalculateDeterministicParams(const CBlock& block, int height) const;
    
    /** Verify stabilization parameters match consensus */
    bool VerifyStabilizationParams(const StabilizationParams& actual,
                                  const StabilizationParams& expected) const;
};

/** Global stabilization consensus validator instance */
extern StabilizationConsensusValidator g_stabilization_consensus_validator;

/** Consensus validation helper functions */
namespace StabilizationConsensus {
    /** Check if all nodes would create the same stabilization transactions */
    bool ValidateConsensusConsistency(const CBlock& block, int height);
    
    /** Get consensus-critical stabilization data hash */
    uint256 GetStabilizationConsensusHash(const CBlock& block, int height);
    
    /** Verify stabilization consensus hash */
    bool VerifyStabilizationConsensusHash(const CBlock& block, 
                                         int height, 
                                         const uint256& expected_hash);
}

} // namespace OConsensus

#endif // BITCOIN_CONSENSUS_STABILIZATION_CONSENSUS_H
