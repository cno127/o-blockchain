// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_VALIDATION_O_INTEGRATION_H
#define BITCOIN_VALIDATION_O_INTEGRATION_H

#include <consensus/user_consensus.h>
#include <consensus/multicurrency.h>
#include <primitives/multicurrency_txout.h>
#include <validation.h>
#include <chain.h>
#include <coins.h>

#include <memory>
#include <vector>

/**
 * O Blockchain Integration Layer
 * 
 * This module integrates the user uniqueness consensus system with the main blockchain
 * validation and transaction processing. It ensures that only verified users can
 * participate in the O blockchain network.
 */

/** Integration between user consensus and main blockchain */
class OBlockchainIntegration {
private:
    // Reference to the main blockchain state
    ChainstateManager* m_chainman;
    
    // User consensus instance
    UserRegistryConsensus* m_user_consensus;
    
    // Integration parameters
    struct IntegrationParams {
        bool require_user_verification = true;        // Require verified users for transactions
        bool allow_pending_users = false;             // Allow pending users to participate
        bool validate_measurements = true;            // Validate user for measurements
        bool validate_endorsements = true;            // Validate user for endorsements
        uint32_t user_activity_timeout = 1000;       // Blocks before user considered inactive
        CAmount measurement_reward_multiplier = 1.0;  // Reward multiplier for verified users
        double unverified_penalty = 0.5;             // Penalty for unverified users
    } m_params;
    
public:
    OBlockchainIntegration(ChainstateManager* chainman, UserRegistryConsensus* user_consensus);
    ~OBlockchainIntegration();
    
    /** Main Blockchain Integration Methods */
    
    /**
     * Validate transaction with user verification
     * @param tx Transaction to validate
     * @param state Validation state
     * @param coins_view Coins view for input validation
     * @param height Current block height
     * @return true if transaction is valid and user is verified
     */
    bool ValidateTransactionWithUserCheck(
        const CMultiCurrencyTransaction& tx,
        TxValidationState& state,
        const CCoinsViewCache& coins_view,
        int height
    );
    
    /**
     * Validate block with user consensus integration
     * @param block Block to validate
     * @param state Block validation state
     * @param pindex Block index
     * @param coins_view Coins view
     * @return true if block is valid
     */
    bool ValidateBlockWithUserConsensus(
        const CBlock& block,
        BlockValidationState& state,
        const CBlockIndex* pindex,
        const CCoinsViewCache& coins_view
    );
    
    /**
     * Process user registration transaction
     * @param tx User registration transaction
     * @param state Validation state
     * @param height Block height
     * @return true if registration is valid
     */
    bool ProcessUserRegistration(
        const CMultiCurrencyTransaction& tx,
        TxValidationState& state,
        int height
    );
    
    /**
     * Process endorsement transaction
     * @param tx Endorsement transaction
     * @param state Validation state
     * @param height Block height
     * @return true if endorsement is valid
     */
    bool ProcessEndorsement(
        const CMultiCurrencyTransaction& tx,
        TxValidationState& state,
        int height
    );
    
    /**
     * Process measurement transaction
     * @param tx Measurement transaction
     * @param state Validation state
     * @param height Block height
     * @return true if measurement is valid
     */
    bool ProcessMeasurement(
        const CMultiCurrencyTransaction& tx,
        TxValidationState& state,
        int height
    );
    
    /** User Verification Methods */
    
    /**
     * Check if user is verified for transaction
     * @param user_public_key User's public key
     * @param tx_type Type of transaction
     * @return true if user is verified for this transaction type
     */
    bool IsUserVerifiedForTransaction(const CPubKey& user_public_key, const std::string& tx_type) const;
    
    /**
     * Get user verification status
     * @param user_public_key User's public key
     * @return User verification status
     */
    UserStatus GetUserVerificationStatus(const CPubKey& user_public_key) const;
    
    /**
     * Update user activity
     * @param user_public_key User's public key
     * @param height Current block height
     */
    void UpdateUserActivity(const CPubKey& user_public_key, int height);
    
    /** Reward and Penalty Methods */
    
    /**
     * Calculate transaction rewards with user verification
     * @param tx Transaction
     * @param base_reward Base reward amount
     * @return Adjusted reward amount
     */
    CAmount CalculateRewardWithUserVerification(
        const CMultiCurrencyTransaction& tx,
        CAmount base_reward
    ) const;
    
    /**
     * Calculate measurement rewards
     * @param measurer_public_key Measurer's public key
     * @param measurement_quality Quality score of measurement
     * @param base_reward Base reward amount
     * @return Adjusted reward amount
     */
    CAmount CalculateMeasurementReward(
        const CPubKey& measurer_public_key,
        double measurement_quality,
        CAmount base_reward
    ) const;
    
    /** Blockchain State Integration */
    
    /**
     * Sync user consensus with blockchain state
     * @param height Current block height
     * @return true if sync successful
     */
    bool SyncUserConsensusWithBlockchain(int height);
    
    /**
     * Process user-related blockchain events
     * @param block Block containing user events
     * @param height Block height
     * @return true if processing successful
     */
    bool ProcessUserBlockchainEvents(const CBlock& block, int height);
    
    /**
     * Get user statistics for blockchain
     * @return User statistics
     */
    UserRegistryConsensus::UserStats GetUserBlockchainStats() const;
    
    /** Configuration and Parameters */
    
    /**
     * Set integration parameters
     * @param params New integration parameters
     */
    void SetIntegrationParams(const IntegrationParams& params);
    
    /**
     * Get current integration parameters
     * @return Current integration parameters
     */
    IntegrationParams GetIntegrationParams() const { return m_params; }
    
    /** Event Handlers */
    
    /**
     * Handle user registration event
     * @param user_registration User registration data
     * @param height Block height
     */
    void OnUserRegistration(const OfficialUser& user_registration, int height);
    
    /**
     * Handle endorsement event
     * @param endorsement Endorsement data
     * @param height Block height
     */
    void OnEndorsement(const EndorsementRecord& endorsement, int height);
    
    /**
     * Handle measurement event
     * @param measurement Measurement data
     * @param height Block height
     */
    void OnMeasurement(const CMultiCurrencyTransaction& measurement, int height);
    
private:
    /** Internal Helper Methods */
    
    /**
     * Extract user public key from transaction
     * @param tx Transaction
     * @return User public key (null if not found)
     */
    std::optional<CPubKey> ExtractUserPublicKey(const CMultiCurrencyTransaction& tx) const;
    
    /**
     * Validate transaction signature with user verification
     * @param tx Transaction
     * @param user_public_key User's public key
     * @return true if signature is valid
     */
    bool ValidateTransactionSignature(
        const CMultiCurrencyTransaction& tx,
        const CPubKey& user_public_key
    ) const;
    
    /**
     * Check if transaction is user-related
     * @param tx Transaction
     * @return true if transaction is user-related
     */
    bool IsUserRelatedTransaction(const CMultiCurrencyTransaction& tx) const;
    
    /**
     * Process user consensus state changes
     * @param height Block height
     */
    void ProcessUserConsensusStateChanges(int height);
    
    /**
     * Update user reputation based on transaction outcomes
     * @param user_public_key User's public key
     * @param tx_success Whether transaction was successful
     * @param tx_type Type of transaction
     */
    void UpdateUserReputationFromTransaction(
        const CPubKey& user_public_key,
        bool tx_success,
        const std::string& tx_type
    );
};

/** Transaction Types for User Verification */
enum class OTransactionType {
    USER_REGISTRATION = 0,
    ENDORSEMENT,
    MEASUREMENT,
    STABILIZATION_MINING,
    REGULAR_TRANSACTION,
    BUSINESS_MINER_REGISTRATION
};

/** Global O blockchain integration instance */
extern OBlockchainIntegration* g_o_integration;

/** Initialize O blockchain integration */
bool InitializeOBlockchainIntegration(ChainstateManager* chainman);

/** Shutdown O blockchain integration */
void ShutdownOBlockchainIntegration();

#endif // BITCOIN_VALIDATION_O_INTEGRATION_H
