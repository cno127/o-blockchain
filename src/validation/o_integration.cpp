// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <validation/o_integration.h>
#include <consensus/tx_check.h>
#include <consensus/tx_verify.h>
#include <script/interpreter.h>
#include <logging.h>
#include <util/strencodings.h>

#include <algorithm>

// Global O blockchain integration instance
OBlockchainIntegration* g_o_integration = nullptr;

// Initialize O blockchain integration
bool InitializeOBlockchainIntegration(ChainstateManager* chainman) {
    if (g_o_integration != nullptr) {
        LogPrintf("O blockchain integration already initialized\n");
        return true;
    }
    
    g_o_integration = new OBlockchainIntegration(chainman, &g_user_consensus);
    
    if (g_o_integration == nullptr) {
        LogError("Failed to initialize O blockchain integration\n");
        return false;
    }
    
    LogPrintf("O blockchain integration initialized successfully\n");
    return true;
}

// Shutdown O blockchain integration
void ShutdownOBlockchainIntegration() {
    if (g_o_integration != nullptr) {
        delete g_o_integration;
        g_o_integration = nullptr;
        LogPrintf("O blockchain integration shutdown\n");
    }
}

// OBlockchainIntegration Implementation
OBlockchainIntegration::OBlockchainIntegration(ChainstateManager* chainman, UserRegistryConsensus* user_consensus)
    : m_chainman(chainman), m_user_consensus(user_consensus) {
    
    // Set default integration parameters
    m_params.require_user_verification = true;
    m_params.allow_pending_users = false;
    m_params.validate_measurements = true;
    m_params.validate_endorsements = true;
    m_params.user_activity_timeout = 1000;
    m_params.measurement_reward_multiplier = 1.0;
    m_params.unverified_penalty = 0.5;
    
    LogPrintf("OBlockchainIntegration initialized with chainman and user consensus\n");
}

OBlockchainIntegration::~OBlockchainIntegration() {
    // Cleanup if needed
}

bool OBlockchainIntegration::ValidateTransactionWithUserCheck(
    const CMultiCurrencyTransaction& tx,
    TxValidationState& state,
    const CCoinsViewCache& coins_view,
    int height) {
    
    // First, perform standard transaction validation
    if (!CheckTransaction(tx.ToLegacyTransaction(), state)) {
        return false;
    }
    
    // Extract user public key from transaction
    auto user_public_key = ExtractUserPublicKey(tx);
    if (!user_public_key.has_value()) {
        // For regular transactions without user identification, skip user verification
        return true;
    }
    
    // Check if user verification is required for this transaction type
    if (!m_params.require_user_verification) {
        return true;
    }
    
    // Validate user verification status
    UserStatus user_status = GetUserVerificationStatus(user_public_key.value());
    
    switch (user_status) {
        case UserStatus::VERIFIED:
            // User is fully verified - allow transaction
            UpdateUserActivity(user_public_key.value(), height);
            return true;
            
        case UserStatus::PENDING_VERIFICATION:
        case UserStatus::VERIFICATION_IN_PROGRESS:
            if (m_params.allow_pending_users) {
                // Allow pending users with restrictions
                LogPrint(BCLog::USER_CONSENSUS, "Transaction from pending user %s\n", 
                        user_public_key.value().GetHex());
                return true;
            } else {
                return state.Invalid(TxValidationResult::TX_CONSENSUS, "user-pending-verification",
                                   "User is pending verification");
            }
            
        case UserStatus::SUSPENDED:
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "user-suspended",
                               "User account is suspended");
            
        case UserStatus::BLACKLISTED:
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "user-blacklisted",
                               "User account is blacklisted");
    }
    
    return true;
}

bool OBlockchainIntegration::ValidateBlockWithUserConsensus(
    const CBlock& block,
    BlockValidationState& state,
    const CBlockIndex* pindex,
    const CCoinsViewCache& coins_view) {
    
    // Process each transaction in the block
    for (size_t i = 0; i < block.vtx.size(); ++i) {
        const auto& tx = block.vtx[i];
        
        // Convert to multi-currency transaction if needed
        CMultiCurrencyTransaction multi_tx(*tx);
        
        TxValidationState tx_state;
        if (!ValidateTransactionWithUserCheck(multi_tx, tx_state, coins_view, pindex->nHeight)) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS,
                               tx_state.GetRejectReason(), tx_state.GetDebugMessage());
        }
        
        // Process special transaction types
        if (IsUserRelatedTransaction(multi_tx)) {
            if (!ProcessUserRelatedTransaction(multi_tx, tx_state, pindex->nHeight)) {
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS,
                                   tx_state.GetRejectReason(), tx_state.GetDebugMessage());
            }
        }
    }
    
    // Sync user consensus state with blockchain
    if (!SyncUserConsensusWithBlockchain(pindex->nHeight)) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "user-consensus-sync-failed",
                           "Failed to sync user consensus with blockchain");
    }
    
    return true;
}

bool OBlockchainIntegration::ProcessUserRegistration(
    const CMultiCurrencyTransaction& tx,
    TxValidationState& state,
    int height) {
    
    // Extract user registration data from transaction
    auto user_public_key = ExtractUserPublicKey(tx);
    if (!user_public_key.has_value()) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "user-registration-no-key",
                           "User registration transaction missing public key");
    }
    
    // Validate transaction signature
    if (!ValidateTransactionSignature(tx, user_public_key.value())) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "user-registration-invalid-signature",
                           "Invalid signature in user registration transaction");
    }
    
    // Create OfficialUser structure from transaction data
    OfficialUser new_user;
    new_user.public_key = user_public_key.value();
    new_user.registration_height = height;
    new_user.status = UserStatus::PENDING_VERIFICATION;
    
    // Extract additional user data from transaction outputs
    // This would be implementation-specific based on how you encode user data
    for (const auto& output : tx.vout) {
        // Parse user data from scriptPubKey or other transaction fields
        // This is a placeholder - you'd implement actual data extraction
        if (output.scriptPubKey.size() > 0) {
            // Extract government ID hash, birth currency, etc.
            // new_user.government_id_hash = ExtractGovernmentIdHash(output);
            // new_user.birth_currency = ExtractBirthCurrency(output);
        }
    }
    
    // Validate user data
    if (new_user.government_id_hash.empty() || new_user.birth_currency.empty()) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "user-registration-incomplete-data",
                           "User registration missing required data");
    }
    
    // Submit user registration to consensus
    std::string error_message;
    if (!m_user_consensus->RegisterUser(new_user, error_message)) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "user-registration-failed",
                           strprintf("User registration failed: %s", error_message));
    }
    
    // Trigger user registration event
    OnUserRegistration(new_user, height);
    
    LogPrint(BCLog::USER_CONSENSUS, "User registration processed for %s at height %d\n",
             user_public_key.value().GetHex(), height);
    
    return true;
}

bool OBlockchainIntegration::ProcessEndorsement(
    const CMultiCurrencyTransaction& tx,
    TxValidationState& state,
    int height) {
    
    // Extract endorsement data from transaction
    auto endorser_key = ExtractUserPublicKey(tx);
    if (!endorser_key.has_value()) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "endorsement-no-endorser-key",
                           "Endorsement transaction missing endorser public key");
    }
    
    // Validate endorser is verified
    if (!IsUserVerifiedForTransaction(endorser_key.value(), "endorsement")) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "endorsement-endorser-not-verified",
                           "Endorser must be verified to submit endorsements");
    }
    
    // Create EndorsementRecord from transaction data
    EndorsementRecord endorsement;
    endorsement.endorser = endorser_key.value();
    endorsement.timestamp = GetTime();
    endorsement.block_height = height;
    
    // Extract endorsed user and other data from transaction
    // This would be implementation-specific
    for (const auto& output : tx.vout) {
        // Extract endorsed user public key and verification details
        // endorsement.endorsed_user = ExtractEndorsedUser(output);
        // endorsement.verification_method = ExtractVerificationMethod(output);
        // endorsement.confidence_level = ExtractConfidenceLevel(output);
    }
    
    // Validate endorsement data
    if (endorsement.endorsed_user.IsNull()) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "endorsement-no-endorsed-user",
                           "Endorsement missing endorsed user");
    }
    
    // Submit endorsement to consensus
    std::string error_message;
    if (!m_user_consensus->SubmitEndorsement(endorsement, error_message)) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "endorsement-submission-failed",
                           strprintf("Endorsement submission failed: %s", error_message));
    }
    
    // Trigger endorsement event
    OnEndorsement(endorsement, height);
    
    LogPrint(BCLog::USER_CONSENSUS, "Endorsement processed from %s for %s at height %d\n",
             endorser_key.value().GetHex(), endorsement.endorsed_user.GetHex(), height);
    
    return true;
}

bool OBlockchainIntegration::ProcessMeasurement(
    const CMultiCurrencyTransaction& tx,
    TxValidationState& state,
    int height) {
    
    // Extract measurer public key
    auto measurer_key = ExtractUserPublicKey(tx);
    if (!measurer_key.has_value()) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "measurement-no-measurer-key",
                           "Measurement transaction missing measurer public key");
    }
    
    // Validate measurer is verified for measurements
    if (m_params.validate_measurements && 
        !IsUserVerifiedForTransaction(measurer_key.value(), "measurement")) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "measurement-measurer-not-verified",
                           "Measurer must be verified to submit measurements");
    }
    
    // Extract measurement data from transaction
    // This would be implementation-specific based on your measurement encoding
    for (const auto& output : tx.vout) {
        // Extract measurement type, currency, value, etc.
        // Validate measurement data quality and format
    }
    
    // Update user activity
    UpdateUserActivity(measurer_key.value(), height);
    
    // Trigger measurement event
    OnMeasurement(tx, height);
    
    LogPrint(BCLog::USER_CONSENSUS, "Measurement processed from %s at height %d\n",
             measurer_key.value().GetHex(), height);
    
    return true;
}

bool OBlockchainIntegration::IsUserVerifiedForTransaction(const CPubKey& user_public_key, const std::string& tx_type) const {
    UserStatus status = GetUserVerificationStatus(user_public_key);
    
    switch (status) {
        case UserStatus::VERIFIED:
            return true;
            
        case UserStatus::PENDING_VERIFICATION:
        case UserStatus::VERIFICATION_IN_PROGRESS:
            // Allow pending users for certain transaction types
            if (tx_type == "user_registration" || tx_type == "endorsement") {
                return true;
            }
            return m_params.allow_pending_users;
            
        case UserStatus::SUSPENDED:
        case UserStatus::BLACKLISTED:
            return false;
    }
    
    return false;
}

UserStatus OBlockchainIntegration::GetUserVerificationStatus(const CPubKey& user_public_key) const {
    // Get user from consensus system
    // This would need to be implemented in the user consensus system
    // For now, we'll return a placeholder
    
    if (m_user_consensus->IsUserVerified(user_public_key)) {
        return UserStatus::VERIFIED;
    }
    
    // Check if user exists but is not verified
    // This would require additional methods in UserRegistryConsensus
    
    return UserStatus::PENDING_VERIFICATION;
}

void OBlockchainIntegration::UpdateUserActivity(const CPubKey& user_public_key, int height) {
    // Update user's last activity height
    // This would be implemented in the user consensus system
    LogPrint(BCLog::USER_CONSENSUS, "Updated activity for user %s at height %d\n",
             user_public_key.GetHex(), height);
}

CAmount OBlockchainIntegration::CalculateRewardWithUserVerification(
    const CMultiCurrencyTransaction& tx,
    CAmount base_reward) const {
    
    auto user_public_key = ExtractUserPublicKey(tx);
    if (!user_public_key.has_value()) {
        // No user identification - apply penalty
        return static_cast<CAmount>(base_reward * m_params.unverified_penalty);
    }
    
    UserStatus status = GetUserVerificationStatus(user_public_key.value());
    
    switch (status) {
        case UserStatus::VERIFIED:
            return base_reward;
            
        case UserStatus::PENDING_VERIFICATION:
        case UserStatus::VERIFICATION_IN_PROGRESS:
            return static_cast<CAmount>(base_reward * 0.8); // 80% reward for pending users
            
        case UserStatus::SUSPENDED:
            return 0; // No rewards for suspended users
            
        case UserStatus::BLACKLISTED:
            return 0; // No rewards for blacklisted users
    }
    
    return base_reward;
}

CAmount OBlockchainIntegration::CalculateMeasurementReward(
    const CPubKey& measurer_public_key,
    double measurement_quality,
    CAmount base_reward) const {
    
    UserStatus status = GetUserVerificationStatus(measurer_public_key);
    
    if (status != UserStatus::VERIFIED) {
        return 0; // Only verified users get measurement rewards
    }
    
    // Apply quality multiplier
    CAmount quality_reward = static_cast<CAmount>(base_reward * measurement_quality);
    
    // Apply user verification multiplier
    return static_cast<CAmount>(quality_reward * m_params.measurement_reward_multiplier);
}

bool OBlockchainIntegration::SyncUserConsensusWithBlockchain(int height) {
    // Sync user consensus state with blockchain state
    return m_user_consensus->SyncWithBlockchain(height);
}

bool OBlockchainIntegration::ProcessUserBlockchainEvents(const CBlock& block, int height) {
    // Process user-related events from the block
    for (const auto& tx : block.vtx) {
        CMultiCurrencyTransaction multi_tx(*tx);
        
        if (IsUserRelatedTransaction(multi_tx)) {
            // Process user-related transaction
            // This would trigger appropriate events
        }
    }
    
    return true;
}

UserRegistryConsensus::UserStats OBlockchainIntegration::GetUserBlockchainStats() const {
    return m_user_consensus->GetUserStatistics();
}

void OBlockchainIntegration::SetIntegrationParams(const IntegrationParams& params) {
    m_params = params;
    LogPrintf("O blockchain integration parameters updated\n");
}

void OBlockchainIntegration::OnUserRegistration(const OfficialUser& user_registration, int height) {
    LogPrint(BCLog::USER_CONSENSUS, "User registration event: %s at height %d\n",
             user_registration.public_key.GetHex(), height);
    
    // Additional processing for user registration events
    // Could trigger notifications, update statistics, etc.
}

void OBlockchainIntegration::OnEndorsement(const EndorsementRecord& endorsement, int height) {
    LogPrint(BCLog::USER_CONSENSUS, "Endorsement event: %s endorsing %s at height %d\n",
             endorsement.endorser.GetHex(), endorsement.endorsed_user.GetHex(), height);
    
    // Additional processing for endorsement events
}

void OBlockchainIntegration::OnMeasurement(const CMultiCurrencyTransaction& measurement, int height) {
    auto measurer_key = ExtractUserPublicKey(measurement);
    if (measurer_key.has_value()) {
        LogPrint(BCLog::USER_CONSENSUS, "Measurement event from %s at height %d\n",
                 measurer_key.value().GetHex(), height);
    }
    
    // Additional processing for measurement events
}

// Private helper methods
std::optional<CPubKey> OBlockchainIntegration::ExtractUserPublicKey(const CMultiCurrencyTransaction& tx) const {
    // Extract user public key from transaction
    // This would be implementation-specific based on how you encode user identification
    
    // For now, we'll try to extract from the first input's scriptSig
    if (!tx.vin.empty() && !tx.vin[0].scriptSig.empty()) {
        // Parse scriptSig to extract public key
        // This is a simplified version - you'd implement proper script parsing
        std::vector<std::vector<unsigned char>> solutions;
        if (Solver(tx.vin[0].scriptSig, solutions) == TxoutType::PUBKEY) {
            if (!solutions.empty() && solutions[0].size() == 33) {
                CPubKey pubkey(solutions[0]);
                if (pubkey.IsValid()) {
                    return pubkey;
                }
            }
        }
    }
    
    return std::nullopt;
}

bool OBlockchainIntegration::ValidateTransactionSignature(
    const CMultiCurrencyTransaction& tx,
    const CPubKey& user_public_key) const {
    
    // Validate transaction signature using the user's public key
    // This would implement proper signature validation
    
    // For now, we'll do a basic check
    if (user_public_key.IsNull() || !user_public_key.IsValid()) {
        return false;
    }
    
    // In a real implementation, you'd validate the signature against the transaction hash
    // using the user's public key
    
    return true;
}

bool OBlockchainIntegration::IsUserRelatedTransaction(const CMultiCurrencyTransaction& tx) const {
    // Check if transaction is user-related based on transaction structure
    // This could check for specific output types, script patterns, etc.
    
    // For now, we'll check if we can extract a user public key
    return ExtractUserPublicKey(tx).has_value();
}

void OBlockchainIntegration::ProcessUserConsensusStateChanges(int height) {
    // Process any pending user consensus state changes
    // This could include:
    // - Processing pending verifications
    // - Resolving challenges
    // - Updating reputation scores
    // - Cleaning up expired data
    
    LogPrint(BCLog::USER_CONSENSUS, "Processing user consensus state changes at height %d\n", height);
}

void OBlockchainIntegration::UpdateUserReputationFromTransaction(
    const CPubKey& user_public_key,
    bool tx_success,
    const std::string& tx_type) {
    
    // Update user reputation based on transaction success
    double reputation_change = 0.0;
    
    if (tx_success) {
        // Reward successful transactions
        if (tx_type == "measurement") {
            reputation_change = 0.1;
        } else if (tx_type == "endorsement") {
            reputation_change = 0.05;
        } else {
            reputation_change = 0.01;
        }
    } else {
        // Penalize failed transactions
        reputation_change = -0.1;
    }
    
    m_user_consensus->UpdateReputationScore(user_public_key, reputation_change);
}


