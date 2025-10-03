// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_VALIDATION_O_BLOCK_VALIDATION_H
#define BITCOIN_VALIDATION_O_BLOCK_VALIDATION_H

#include <consensus/user_consensus.h>
#include <primitives/multicurrency_txout.h>
#include <validation.h>
#include <chain.h>
#include <consensus/params.h>

/**
 * O Blockchain Block Validation Integration
 * 
 * This header provides integration functions for incorporating user verification
 * into the main blockchain validation process.
 */

/**
 * Enhanced contextual block validation with user consensus
 * @param block Block to validate
 * @param state Block validation state
 * @param chainman Chain state manager
 * @param pindexPrev Previous block index
 * @return true if block is valid
 */
bool ContextualCheckBlockWithUserConsensus(
    const CBlock& block,
    BlockValidationState& state,
    const ChainstateManager& chainman,
    const CBlockIndex* pindexPrev);

/**
 * Enhanced transaction validation with user verification
 * @param tx Transaction to validate
 * @param state Transaction validation state
 * @param inputs Coins view for input validation
 * @param nSpendHeight Spend height
 * @return true if transaction is valid
 */
bool CheckTransactionWithUserVerification(
    const CTransaction& tx,
    TxValidationState& state,
    const CCoinsViewCache& inputs,
    int nSpendHeight);

/**
 * Enhanced block connection with user consensus
 * @param block Block to connect
 * @param state Block validation state
 * @param pindex Block index
 * @param view Coins view cache
 * @param params Chain parameters
 * @param fJustCheck Whether to just check without connecting
 * @return true if block connection successful
 */
bool ConnectBlockWithUserConsensus(
    const CBlock& block,
    BlockValidationState& state,
    CBlockIndex* pindex,
    CCoinsViewCache& view,
    const CChainParams& params,
    bool fJustCheck);

/**
 * Enhanced block acceptance with user consensus
 * @param pblock Block to accept
 * @param state Block validation state
 * @param chainman Chain state manager
 * @param ppindex Block index pointer
 * @param fRequested Whether block was requested
 * @param fNewBlock Whether this is a new block
 * @return true if block acceptance successful
 */
bool AcceptBlockWithUserConsensus(
    const std::shared_ptr<const CBlock>& pblock,
    BlockValidationState& state,
    ChainstateManager& chainman,
    CBlockIndex** ppindex,
    bool fRequested,
    bool* fNewBlock);

/**
 * Validate O blockchain transaction
 * @param tx Multi-currency transaction to validate
 * @param state Transaction validation state
 * @param inputs Coins view for input validation
 * @param nSpendHeight Spend height
 * @param consensusParams Consensus parameters
 * @return true if transaction is valid
 */
bool ValidateOTransaction(
    const CMultiCurrencyTransaction& tx,
    TxValidationState& state,
    const CCoinsViewCache& inputs,
    int nSpendHeight,
    const Consensus::Params& consensusParams);

/**
 * Process user registration transaction
 * @param tx User registration transaction
 * @param state Transaction validation state
 * @param height Block height
 * @return true if registration processing successful
 */
bool ProcessUserRegistrationTransaction(
    const CMultiCurrencyTransaction& tx,
    TxValidationState& state,
    int height);

/**
 * Process endorsement transaction
 * @param tx Endorsement transaction
 * @param state Transaction validation state
 * @param height Block height
 * @return true if endorsement processing successful
 */
bool ProcessEndorsementTransaction(
    const CMultiCurrencyTransaction& tx,
    TxValidationState& state,
    int height);

/**
 * Process measurement transaction
 * @param tx Measurement transaction
 * @param state Transaction validation state
 * @param height Block height
 * @return true if measurement processing successful
 */
bool ProcessMeasurementTransaction(
    const CMultiCurrencyTransaction& tx,
    TxValidationState& state,
    int height);

/**
 * Get user verification status
 * @param user_public_key User's public key
 * @return User verification status
 */
UserStatus GetUserVerificationStatus(const CPubKey& user_public_key);

/**
 * Check if user is verified for transaction type
 * @param user_public_key User's public key
 * @param tx_type Transaction type
 * @return true if user is verified for this transaction type
 */
bool IsUserVerifiedForTransaction(const CPubKey& user_public_key, const std::string& tx_type);

/**
 * Calculate rewards with user verification
 * @param tx Transaction
 * @param base_reward Base reward amount
 * @return Adjusted reward amount
 */
CAmount CalculateRewardWithUserVerification(
    const CMultiCurrencyTransaction& tx,
    CAmount base_reward);

/**
 * Calculate measurement rewards
 * @param measurer_public_key Measurer's public key
 * @param measurement_quality Measurement quality score
 * @param base_reward Base reward amount
 * @return Adjusted reward amount
 */
CAmount CalculateMeasurementReward(
    const CPubKey& measurer_public_key,
    double measurement_quality,
    CAmount base_reward);

#endif // BITCOIN_VALIDATION_O_BLOCK_VALIDATION_H


