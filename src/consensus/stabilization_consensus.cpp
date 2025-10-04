// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/stabilization_consensus.h>
#include <consensus/stabilization_mining.h>
#include <consensus/stabilization_coins.h>
#include <logging.h>
#include <hash.h>
#include <util/strencodings.h>
#include <algorithm>

namespace OConsensus {

StabilizationConsensusValidator g_stabilization_consensus_validator;

StabilizationConsensusValidator::StabilizationConsensusValidator() {
    LogPrintf("O Stabilization Consensus: Initializing consensus validator\n");
}

bool StabilizationConsensusValidator::ValidateStabilizationTransactions(
    const CBlock& block, 
    int height,
    BlockValidationState& state) const {
    
    m_stats.total_blocks_validated++;
    
    // Check if block should have stabilization transactions
    bool should_have_stabilization = ShouldBlockHaveStabilization(block, height);
    
    // Count stabilization transactions in block
    int stabilization_tx_count = 0;
    for (const auto& tx : block.vtx) {
        if (g_stabilization_coins_manager.IsStabilizationTransaction(*tx)) {
            stabilization_tx_count++;
        }
    }
    
    if (should_have_stabilization && stabilization_tx_count == 0) {
        state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-stabilization-missing",
                     "Block should have stabilization transactions but none found");
        m_stats.consensus_violations++;
        return false;
    }
    
    if (!should_have_stabilization && stabilization_tx_count > 0) {
        state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-stabilization-unexpected",
                     "Block has stabilization transactions but should not");
        m_stats.consensus_violations++;
        return false;
    }
    
    if (stabilization_tx_count > 0) {
        m_stats.blocks_with_stabilization++;
    }
    
    // Validate each stabilization transaction
    for (const auto& tx : block.vtx) {
        if (g_stabilization_coins_manager.IsStabilizationTransaction(*tx)) {
            TxValidationState tx_state;
            if (!ValidateStabilizationTransaction(*tx, height, tx_state)) {
                state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-stabilization-tx",
                             strprintf("Invalid stabilization transaction: %s", tx_state.ToString()));
                m_stats.consensus_violations++;
                return false;
            }
            m_stats.stabilization_transactions_validated++;
        }
    }
    
    // Check consensus consistency
    if (!StabilizationConsensus::ValidateConsensusConsistency(block, height)) {
        state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-stabilization-consensus",
                     "Stabilization transactions do not match consensus expectations");
        m_stats.consensus_violations++;
        return false;
    }
    
    return true;
}

bool StabilizationConsensusValidator::ValidateStabilizationTransaction(
    const CTransaction& tx,
    int height,
    TxValidationState& state) const {
    
    // Validate structure
    if (!ValidateStabilizationStructure(tx, state)) {
        return false;
    }
    
    // Validate amounts
    if (!ValidateStabilizationAmounts(tx, state)) {
        return false;
    }
    
    // Validate recipients
    if (!ValidateStabilizationRecipients(tx, state)) {
        return false;
    }
    
    return true;
}

bool StabilizationConsensusValidator::ShouldBlockHaveStabilization(
    const CBlock& block, int height) const {
    
    return IsStabilizationTriggeredByConsensus(block, height);
}

std::vector<CTransaction> StabilizationConsensusValidator::CalculateExpectedStabilizationTransactions(
    const CBlock& block, int height) const {
    
    // Use the same logic as the stabilization mining system
    return g_stabilization_mining.CreateStabilizationTransactions(block, height);
}

bool StabilizationConsensusValidator::VerifyStabilizationTransaction(
    const CTransaction& actual_tx,
    const CTransaction& expected_tx) const {
    
    // Compare transaction hashes (should be identical for consensus)
    if (actual_tx.GetHash() != expected_tx.GetHash()) {
        LogPrintf("O Stabilization Consensus: Transaction hash mismatch\n");
        return false;
    }
    
    // Compare outputs
    if (actual_tx.vout.size() != expected_tx.vout.size()) {
        LogPrintf("O Stabilization Consensus: Output count mismatch\n");
        return false;
    }
    
    for (size_t i = 0; i < actual_tx.vout.size(); ++i) {
        if (actual_tx.vout[i].nValue != expected_tx.vout[i].nValue) {
            LogPrintf("O Stabilization Consensus: Output value mismatch at index %d\n", i);
            return false;
        }
    }
    
    return true;
}

StabilizationConsensusValidator::StabilizationConsensusStats 
StabilizationConsensusValidator::GetConsensusStats() const {
    return m_stats;
}

void StabilizationConsensusValidator::ResetConsensusStats() {
    m_stats = StabilizationConsensusStats();
}

bool StabilizationConsensusValidator::CheckStabilizationConsensusRules(
    const CBlock& block, 
    int height,
    BlockValidationState& state) const {
    
    // Check if stabilization is triggered
    if (!IsStabilizationTriggeredByConsensus(block, height)) {
        return true; // No stabilization needed
    }
    
    // Calculate expected stabilization transactions
    auto expected_txs = CalculateExpectedStabilizationTransactions(block, height);
    
    // Find actual stabilization transactions in block
    std::vector<CTransaction> actual_txs;
    for (const auto& tx : block.vtx) {
        if (g_stabilization_coins_manager.IsStabilizationTransaction(*tx)) {
            actual_txs.push_back(*tx);
        }
    }
    
    // Compare counts
    if (actual_txs.size() != expected_txs.size()) {
        state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-stabilization-count",
                     strprintf("Expected %d stabilization transactions, got %d", 
                              static_cast<int>(expected_txs.size()), 
                              static_cast<int>(actual_txs.size())));
        return false;
    }
    
    // Compare each transaction
    for (size_t i = 0; i < expected_txs.size(); ++i) {
        if (!VerifyStabilizationTransaction(actual_txs[i], expected_txs[i])) {
            state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-stabilization-tx-mismatch",
                         strprintf("Stabilization transaction %d does not match consensus", i));
            return false;
        }
    }
    
    return true;
}

bool StabilizationConsensusValidator::ValidateStabilizationStructure(
    const CTransaction& tx,
    TxValidationState& state) const {
    
    // Stabilization transactions should have no inputs (like coinbase)
    if (!tx.vin.empty()) {
        state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-stabilization-inputs",
                     "Stabilization transaction should have no inputs");
        return false;
    }
    
    // Must have at least one output
    if (tx.vout.empty()) {
        state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-stabilization-outputs",
                     "Stabilization transaction must have outputs");
        return false;
    }
    
    return true;
}

bool StabilizationConsensusValidator::ValidateStabilizationAmounts(
    const CTransaction& tx,
    TxValidationState& state) const {
    
    CAmount total_value = 0;
    for (const auto& output : tx.vout) {
        // Check individual output bounds
        if (output.nValue < StabilizationConfig::MIN_STABILIZATION_REWARD ||
            output.nValue > StabilizationConfig::MAX_STABILIZATION_REWARD) {
            state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-stabilization-amount",
                         strprintf("Stabilization output amount %d out of bounds [%d, %d]",
                                  output.nValue, StabilizationConfig::MIN_STABILIZATION_REWARD,
                                  StabilizationConfig::MAX_STABILIZATION_REWARD));
            return false;
        }
        total_value += output.nValue;
    }
    
    // Check total value is reasonable
    CAmount max_total = StabilizationConfig::MAX_STABILIZATION_REWARD * tx.vout.size();
    if (total_value > max_total) {
        state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-stabilization-total",
                     strprintf("Total stabilization amount %d exceeds maximum %d",
                              total_value, max_total));
        return false;
    }
    
    return true;
}

bool StabilizationConsensusValidator::ValidateStabilizationRecipients(
    const CTransaction& tx,
    TxValidationState& state) const {
    
    // Check recipient count is reasonable
    if (tx.vout.size() < 1 || tx.vout.size() > 50000) {
        state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-stabilization-recipients",
                     strprintf("Invalid recipient count %d", static_cast<int>(tx.vout.size())));
        return false;
    }
    
    // All outputs should have valid scripts
    for (const auto& output : tx.vout) {
        if (output.scriptPubKey.empty()) {
            state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-stabilization-script",
                         "Stabilization output has empty script");
            return false;
        }
    }
    
    return true;
}

bool StabilizationConsensusValidator::IsStabilizationTriggeredByConsensus(
    const CBlock& block, int height) const {
    
    // Use the same logic as the stabilization mining system
    return ShouldTriggerStabilization(block, height);
}

StabilizationConsensusValidator::StabilizationParams 
StabilizationConsensusValidator::CalculateDeterministicParams(
    const CBlock& block, int height) const {
    
    StabilizationParams params;
    
    // Get unstable currencies
    params.unstable_currencies = g_stabilization_mining.GetUnstableCurrencies();
    
    // Calculate coins per currency
    for (const auto& currency : params.unstable_currencies) {
        auto info = g_stabilization_mining.GetStabilityStatus(currency);
        if (info.has_value() && info->IsUnstable()) {
            CAmount coins = g_stabilization_mining.CalculateCoinsForCurrency(currency, block, height);
            params.coins_per_currency[currency] = coins;
            
            // Calculate recipient count
            int recipient_count = g_stabilization_mining.CalculateOptimalRecipientCount(coins);
            params.recipient_counts[currency] = recipient_count;
        }
    }
    
    return params;
}

bool StabilizationConsensusValidator::VerifyStabilizationParams(
    const StabilizationParams& actual,
    const StabilizationParams& expected) const {
    
    // Compare unstable currencies
    if (actual.unstable_currencies != expected.unstable_currencies) {
        return false;
    }
    
    // Compare coins per currency
    for (const auto& [currency, expected_coins] : expected.coins_per_currency) {
        auto it = actual.coins_per_currency.find(currency);
        if (it == actual.coins_per_currency.end() || it->second != expected_coins) {
            return false;
        }
    }
    
    // Compare recipient counts
    for (const auto& [currency, expected_count] : expected.recipient_counts) {
        auto it = actual.recipient_counts.find(currency);
        if (it == actual.recipient_counts.end() || it->second != expected_count) {
            return false;
        }
    }
    
    return true;
}

// Helper functions
namespace StabilizationConsensus {

bool ValidateConsensusConsistency(const CBlock& block, int height) {
    BlockValidationState state;
    return g_stabilization_consensus_validator.CheckStabilizationConsensusRules(block, height, state);
}

uint256 GetStabilizationConsensusHash(const CBlock& block, int height) {
    HashWriter ss{};
    
    // Include block hash and height for determinism
    ss << block.GetHash();
    ss << static_cast<int64_t>(height);
    
    // Include unstable currencies
    auto unstable_currencies = g_stabilization_mining.GetUnstableCurrencies();
    for (const auto& currency : unstable_currencies) {
        ss << currency;
        
        // Include stability info
        auto info = g_stabilization_mining.GetStabilityStatus(currency);
        if (info.has_value()) {
            ss << static_cast<int64_t>(info->stability_ratio * 1000000); // Scale to avoid float issues
            ss << static_cast<int64_t>(info->unstable_since_height);
        }
    }
    
    return ss.GetHash();
}

bool VerifyStabilizationConsensusHash(const CBlock& block, 
                                     int height, 
                                     const uint256& expected_hash) {
    uint256 actual_hash = GetStabilizationConsensusHash(block, height);
    return actual_hash == expected_hash;
}

} // namespace StabilizationConsensus

} // namespace OConsensus
