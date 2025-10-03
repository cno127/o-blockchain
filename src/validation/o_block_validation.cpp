// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <validation/o_integration.h>
#include <validation.h>
#include <chain.h>
#include <consensus/tx_check.h>
#include <consensus/tx_verify.h>
#include <logging.h>

/**
 * O Blockchain Block Validation Integration
 * 
 * This file integrates user verification into the main blockchain validation process.
 * It extends the standard Bitcoin Core validation to include user consensus checks.
 */

bool ContextualCheckBlockWithUserConsensus(
    const CBlock& block, 
    BlockValidationState& state, 
    const ChainstateManager& chainman, 
    const CBlockIndex* pindexPrev) {
    
    // First, perform standard contextual block validation
    if (!ContextualCheckBlock(block, state, chainman, pindexPrev)) {
        return false;
    }
    
    // If O integration is not available, skip user consensus checks
    if (g_o_integration == nullptr) {
        return true;
    }
    
    // Perform user consensus validation
    const int nHeight = pindexPrev == nullptr ? 0 : pindexPrev->nHeight + 1;
    
    // Get coins view for validation
    CCoinsViewCache coins_view(chainman.ActiveChainstate().CoinsTip());
    
    // Validate block with user consensus
    if (!g_o_integration->ValidateBlockWithUserConsensus(block, state, pindexPrev, coins_view)) {
        return false;
    }
    
    LogPrint(BCLog::USER_CONSENSUS, "Block %s validated with user consensus at height %d\n",
             block.GetHash().GetHex(), nHeight);
    
    return true;
}

bool CheckTransactionWithUserVerification(
    const CTransaction& tx,
    TxValidationState& state,
    const CCoinsViewCache& inputs,
    int nSpendHeight) {
    
    // First, perform standard transaction validation
    if (!CheckTransaction(tx, state)) {
        return false;
    }
    
    // If O integration is not available, skip user verification
    if (g_o_integration == nullptr) {
        return true;
    }
    
    // Convert to multi-currency transaction
    CMultiCurrencyTransaction multi_tx(tx);
    
    // Perform user verification
    if (!g_o_integration->ValidateTransactionWithUserCheck(multi_tx, state, inputs, nSpendHeight)) {
        return false;
    }
    
    return true;
}

bool ConnectBlockWithUserConsensus(
    const CBlock& block,
    BlockValidationState& state,
    CBlockIndex* pindex,
    CCoinsViewCache& view,
    const CChainParams& params,
    bool fJustCheck) {
    
    // First, perform standard block connection
    if (!ConnectBlock(block, state, pindex, view, params, fJustCheck)) {
        return false;
    }
    
    // If O integration is not available, skip user consensus processing
    if (g_o_integration == nullptr) {
        return true;
    }
    
    // Process user consensus events after successful block connection
    if (!g_o_integration->ProcessUserBlockchainEvents(block, pindex->nHeight)) {
        // Log warning but don't fail block connection
        LogWarning("Failed to process user consensus events for block %s at height %d\n",
                  block.GetHash().GetHex(), pindex->nHeight);
    }
    
    // Sync user consensus with blockchain state
    if (!g_o_integration->SyncUserConsensusWithBlockchain(pindex->nHeight)) {
        LogWarning("Failed to sync user consensus with blockchain at height %d\n", pindex->nHeight);
    }
    
    LogPrint(BCLog::USER_CONSENSUS, "Block %s connected with user consensus at height %d\n",
             block.GetHash().GetHex(), pindex->nHeight);
    
    return true;
}

bool AcceptBlockWithUserConsensus(
    const std::shared_ptr<const CBlock>& pblock,
    BlockValidationState& state,
    ChainstateManager& chainman,
    CBlockIndex** ppindex,
    bool fRequested,
    bool* fNewBlock) {
    
    // First, perform standard block acceptance
    if (!AcceptBlock(pblock, state, chainman, ppindex, fRequested, fNewBlock)) {
        return false;
    }
    
    // If O integration is not available, skip user consensus processing
    if (g_o_integration == nullptr) {
        return true;
    }
    
    // Process user consensus state changes
    if (*ppindex != nullptr) {
        g_o_integration->ProcessUserConsensusStateChanges((*ppindex)->nHeight);
    }
    
    return true;
}

/**
 * Enhanced transaction validation for O blockchain
 */
bool ValidateOTransaction(
    const CMultiCurrencyTransaction& tx,
    TxValidationState& state,
    const CCoinsViewCache& inputs,
    int nSpendHeight,
    const Consensus::Params& consensusParams) {
    
    // Perform standard transaction validation
    if (!CheckTransaction(tx.ToLegacyTransaction(), state)) {
        return false;
    }
    
    // Check transaction inputs
    CAmount txfee = 0;
    if (!Consensus::CheckTxInputs(tx.ToLegacyTransaction(), state, inputs, nSpendHeight, txfee)) {
        return false;
    }
    
    // If O integration is available, perform user verification
    if (g_o_integration != nullptr) {
        if (!g_o_integration->ValidateTransactionWithUserCheck(tx, state, inputs, nSpendHeight)) {
            return false;
        }
    }
    
    return true;
}

/**
 * Process user registration transaction
 */
bool ProcessUserRegistrationTransaction(
    const CMultiCurrencyTransaction& tx,
    TxValidationState& state,
    int height) {
    
    if (g_o_integration == nullptr) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "o-integration-not-available",
                           "O blockchain integration not available");
    }
    
    return g_o_integration->ProcessUserRegistration(tx, state, height);
}

/**
 * Process endorsement transaction
 */
bool ProcessEndorsementTransaction(
    const CMultiCurrencyTransaction& tx,
    TxValidationState& state,
    int height) {
    
    if (g_o_integration == nullptr) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "o-integration-not-available",
                           "O blockchain integration not available");
    }
    
    return g_o_integration->ProcessEndorsement(tx, state, height);
}

/**
 * Process measurement transaction
 */
bool ProcessMeasurementTransaction(
    const CMultiCurrencyTransaction& tx,
    TxValidationState& state,
    int height) {
    
    if (g_o_integration == nullptr) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "o-integration-not-available",
                           "O blockchain integration not available");
    }
    
    return g_o_integration->ProcessMeasurement(tx, state, height);
}

/**
 * Get user verification status for a public key
 */
UserStatus GetUserVerificationStatus(const CPubKey& user_public_key) {
    if (g_o_integration == nullptr) {
        return UserStatus::PENDING_VERIFICATION;
    }
    
    return g_o_integration->GetUserVerificationStatus(user_public_key);
}

/**
 * Check if user is verified for a specific transaction type
 */
bool IsUserVerifiedForTransaction(const CPubKey& user_public_key, const std::string& tx_type) {
    if (g_o_integration == nullptr) {
        return false;
    }
    
    return g_o_integration->IsUserVerifiedForTransaction(user_public_key, tx_type);
}

/**
 * Calculate rewards with user verification
 */
CAmount CalculateRewardWithUserVerification(
    const CMultiCurrencyTransaction& tx,
    CAmount base_reward) {
    
    if (g_o_integration == nullptr) {
        return base_reward;
    }
    
    return g_o_integration->CalculateRewardWithUserVerification(tx, base_reward);
}

/**
 * Calculate measurement rewards
 */
CAmount CalculateMeasurementReward(
    const CPubKey& measurer_public_key,
    double measurement_quality,
    CAmount base_reward) {
    
    if (g_o_integration == nullptr) {
        return base_reward;
    }
    
    return g_o_integration->CalculateMeasurementReward(measurer_public_key, measurement_quality, base_reward);
}


