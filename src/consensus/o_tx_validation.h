// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_O_TX_VALIDATION_H
#define BITCOIN_CONSENSUS_O_TX_VALIDATION_H

#include <consensus/validation.h>
#include <primitives/o_transactions.h>
#include <primitives/transaction.h>

class CBlockIndex;

namespace OConsensus {

/**
 * Validate and process O-specific transactions
 * 
 * These functions are called during block validation to process
 * BrightID verifications, water price measurements, and exchange rate measurements.
 */

/**
 * Process all O-specific transactions in a block
 * 
 * Called during ConnectBlock to extract and store O data in databases.
 * 
 * @param block The block containing transactions
 * @param pindex Block index with height and timestamp
 * @return true if all O transactions are valid, false otherwise
 */
bool ProcessOTransactions(const CBlock& block, const CBlockIndex* pindex);

/**
 * Validate and process a user verification transaction
 * 
 * Supports multiple identity providers (BrightID, KYC, WorldCoin, etc.)
 * 
 * Validates:
 * - Identity provider signature is valid
 * - User signature proves ownership of o_pubkey
 * - No duplicate verification for same user_id + provider
 * - Country code is valid
 * 
 * @param data The verification data
 * @param tx The transaction containing this data
 * @param height Block height
 * @return true if valid and processed, false otherwise
 */
bool ProcessUserVerification(
    const OTransactions::CUserVerificationData& data,
    const CTransaction& tx,
    int height
);

/**
 * Validate and process a water price measurement transaction
 * 
 * Validates:
 * - Measurer is verified in BrightID database
 * - Has valid measurement invitation
 * - Price is reasonable (anti-spam)
 * - Proof data is valid (URL accessible or GPS valid)
 * - Signature is valid
 * 
 * @param data The measurement data
 * @param tx The transaction containing this data
 * @param height Block height
 * @return true if valid and processed, false otherwise
 */
bool ProcessWaterPriceMeasurement(
    const OTransactions::CWaterPriceMeasurementData& data,
    const CTransaction& tx,
    int height
);

/**
 * Validate and process an exchange rate measurement transaction
 * 
 * Validates:
 * - Measurer is verified
 * - Has valid measurement invitation
 * - Exchange rate is reasonable
 * - Currencies exist
 * - Signature is valid
 * 
 * @param data The measurement data
 * @param tx The transaction containing this data
 * @param height Block height
 * @return true if valid and processed, false otherwise
 */
bool ProcessExchangeRateMeasurement(
    const OTransactions::CExchangeRateMeasurementData& data,
    const CTransaction& tx,
    int height
);

/**
 * Validate identity provider signature
 * 
 * Verifies that the identity provider's signature is valid for the given data.
 * Supports multiple providers: BrightID, KYC systems, WorldCoin, etc.
 * 
 * @param data The verification data
 * @return true if signature is valid
 */
bool ValidateProviderSignature(const OTransactions::CUserVerificationData& data);

/**
 * Validate user's signature on verification
 * 
 * Verifies that the user actually owns the o_pubkey they're linking to.
 * 
 * @param data The verification data
 * @return true if user signature is valid
 */
bool ValidateUserSignature(const OTransactions::CUserVerificationData& data);

/**
 * Validate and process a measurement validation transaction
 * 
 * Validates:
 * - Validator is verified
 * - Measurement exists in database
 * - Validator hasn't already validated this measurement
 * - Signature is valid
 * 
 * Updates validator list in the measurement database.
 * 
 * @param data The validation data
 * @param tx The transaction containing this data
 * @param height Block height
 * @return true if valid and processed, false otherwise
 */
bool ProcessMeasurementValidation(
    const OTransactions::CMeasurementValidationData& data,
    const CTransaction& tx,
    int height
);

/**
 * Validate and process a measurement invitation transaction
 * 
 * Validates:
 * - Invitation ID is unique
 * - Invited user exists and is verified
 * - Expiration time is reasonable
 * - System signature is valid (TODO)
 * 
 * Stores invitation in measurement database.
 * 
 * @param data The invitation data
 * @param tx The transaction containing this data
 * @param height Block height
 * @return true if valid and processed, false otherwise
 */
bool ProcessMeasurementInvite(
    const OTransactions::CMeasurementInviteData& data,
    const CTransaction& tx,
    int height
);

/**
 * Validate measurement invitation
 * 
 * Checks that the measurer has a valid invitation to submit this measurement.
 * 
 * @param invite_id The invitation ID
 * @param measurer The public key of the measurer
 * @return true if invitation is valid and not expired
 */
bool ValidateMeasurementInvitation(const uint256& invite_id, const CPubKey& measurer);

/**
 * Validate water price proof
 * 
 * Validates the proof data (URL or GPS+photo) for a water price measurement.
 * 
 * @param proof_type "url" or "gps_photo"
 * @param proof_data The proof string (URL or GPS coords + hash)
 * @param currency_code The currency being measured
 * @return true if proof is valid
 */
bool ValidateWaterPriceProof(
    const std::string& proof_type,
    const std::string& proof_data,
    const std::string& currency_code
);

/**
 * Check if measurer is verified for measurements
 * 
 * @param measurer The public key of the measurer
 * @return true if measurer is verified in BrightID database
 */
bool IsMeasurerVerified(const CPubKey& measurer);

} // namespace OConsensus

#endif // BITCOIN_CONSENSUS_O_TX_VALIDATION_H

