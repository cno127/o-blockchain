// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/o_tx_validation.h>

#include <chain.h>
#include <consensus/o_brightid_db.h>
#include <hash.h>
#include <logging.h>
#include <measurement/o_measurement_db.h>
#include <primitives/block.h>
#include <pubkey.h>
#include <util/time.h>
#include <util/strencodings.h>

#include <optional>

namespace OConsensus {

bool ProcessOTransactions(const CBlock& block, const CBlockIndex* pindex) {
    if (!pindex) {
        LogPrintf("O Validation: Invalid block index\n");
        return false;
    }
    
    int height = pindex->nHeight;
    int processed_count = 0;
    
    // Process all transactions in the block
    for (const auto& tx : block.vtx) {
        if (!OTransactions::IsOTransaction(*tx)) {
            continue;
        }
        
        auto tx_type = OTransactions::GetOTxType(*tx);
        if (!tx_type.has_value()) {
            LogPrintf("O Validation: Could not determine O transaction type\n");
            continue;  // Skip malformed O transactions
        }
        
        bool processed = false;
        
        switch (tx_type.value()) {
            case OTransactions::OTxType::USER_VERIFY: {
                auto data = OTransactions::ExtractUserVerification(*tx);
                if (data.has_value()) {
                    processed = ProcessUserVerification(data.value(), *tx, height);
                }
                break;
            }
            
            case OTransactions::OTxType::WATER_PRICE: {
                auto data = OTransactions::ExtractWaterPriceMeasurement(*tx);
                if (data.has_value()) {
                    processed = ProcessWaterPriceMeasurement(data.value(), *tx, height);
                }
                break;
            }
            
            case OTransactions::OTxType::EXCHANGE_RATE: {
                auto data = OTransactions::ExtractExchangeRateMeasurement(*tx);
                if (data.has_value()) {
                    processed = ProcessExchangeRateMeasurement(data.value(), *tx, height);
                }
                break;
            }
            
            case OTransactions::OTxType::MEASUREMENT_VALIDATION: {
                auto data = OTransactions::ExtractMeasurementValidation(*tx);
                if (data.has_value()) {
                    processed = ProcessMeasurementValidation(data.value(), *tx, height);
                }
                break;
            }
            
            case OTransactions::OTxType::MEASUREMENT_INVITE: {
                auto data = OTransactions::ExtractMeasurementInvite(*tx);
                if (data.has_value()) {
                    processed = ProcessMeasurementInvite(data.value(), *tx, height);
                }
                break;
            }
            
            default:
                LogPrintf("O Validation: Unknown O transaction type: %d\n", 
                         static_cast<int>(tx_type.value()));
                break;
        }
        
        if (processed) {
            processed_count++;
        }
    }
    
    if (processed_count > 0) {
        LogPrintf("O Validation: Processed %d O-specific transactions at height %d\n",
                 processed_count, height);
    }
    
    return true;  // Non-critical errors don't invalidate the block
}

bool ProcessUserVerification(
    const OTransactions::CUserVerificationData& data,
    const CTransaction& tx,
    int height
) {
    LogPrintf("O Validation: Processing user verification [%s] for %s from %s at height %d\n",
             data.identity_provider.c_str(), data.user_id.c_str(), data.country_code.c_str(), height);
    
    // Validate data structure
    if (!data.IsValid()) {
        LogPrintf("O Validation: Invalid user verification data\n");
        return false;
    }
    
    // Validate identity provider signature
    if (!ValidateProviderSignature(data)) {
        LogPrintf("O Validation: Invalid provider signature for %s/%s\n", 
                 data.identity_provider.c_str(), data.user_id.c_str());
        return false;
    }
    
    // Validate user signature (proves ownership of o_pubkey)
    if (!ValidateUserSignature(data)) {
        LogPrintf("O Validation: Invalid user signature for %s\n", data.user_id.c_str());
        return false;
    }
    
    // Create unique user key: provider + user_id
    std::string user_key = data.identity_provider + ":" + data.user_id;
    
    // Check if already verified (prevent duplicates)
    if (g_brightid_db) {
        auto existing = g_brightid_db->ReadUser(user_key);
        if (existing.has_value() && existing->IsVerified()) {
            LogPrintf("O Validation: User %s already verified, skipping\n",
                     user_key.c_str());
            return true;  // Not an error, just redundant
        }
    }
    
    // Create BrightIDUser object (note: reusing this structure for all identity providers)
    // TODO: Consider renaming to VerifiedUser in future refactor
    BrightIDUser user;
    user.brightid_address = user_key;  // Store as "provider:userid"
    user.context_id = data.country_code + ":" + data.birth_currency;  // Store country:currency
    user.verification_timestamp = data.timestamp;
    user.expiration_timestamp = data.expiration > 0 ? data.expiration : (data.timestamp + (365 * 24 * 60 * 60));
    user.status = BrightIDStatus::VERIFIED;
    user.method = BrightIDVerificationMethod::SOCIAL_GRAPH;  // Generic, provider-specific in verification_data
    user.trust_score = 1.0;  // Can be parsed from verification_data if needed
    user.is_active = true;
    
    // Store in database
    if (g_brightid_db) {
        if (!g_brightid_db->WriteUser(user_key, user)) {
            LogPrintf("O Validation: Failed to write user to database\n");
            return false;
        }
        
        // Link addresses (user_key <-> o_pubkey)
        std::string o_address = HexStr(data.o_pubkey);
        if (!g_brightid_db->LinkAddresses(user_key, o_address)) {
            LogPrintf("O Validation: Failed to link addresses\n");
            return false;
        }
        
        LogPrintf("O Validation: User verification stored: %s [%s] from %s → Birth Currency: %s\n",
                 data.user_id.c_str(), data.identity_provider.c_str(), 
                 data.country_code.c_str(), data.birth_currency.c_str());
    }
    
    return true;
}

bool ProcessWaterPriceMeasurement(
    const OTransactions::CWaterPriceMeasurementData& data,
    const CTransaction& tx,
    int height
) {
    LogPrintf("O Validation: Processing water price measurement for %s at height %d\n",
             data.currency_code.c_str(), height);
    
    // Validate data structure
    if (!data.IsValid()) {
        LogPrintf("O Validation: Invalid water price measurement data\n");
        return false;
    }
    
    // Validate measurer is verified
    if (!IsMeasurerVerified(data.measurer)) {
        LogPrintf("O Validation: Measurer not verified for water price measurement\n");
        return false;
    }
    
    // Validate measurement invitation
    if (!ValidateMeasurementInvitation(data.invite_id, data.measurer)) {
        LogPrintf("O Validation: Invalid or expired measurement invitation\n");
        return false;
    }
    
    // Validate proof
    if (!ValidateWaterPriceProof(data.proof_type, data.proof_data, data.currency_code)) {
        LogPrintf("O Validation: Invalid water price proof\n");
        return false;
    }
    
    // TODO: Validate signature
    
    // Store in measurement database
    if (OMeasurement::g_measurement_db) {
        OMeasurement::WaterPriceMeasurement measurement;
        measurement.measurement_id = data.GetHash();
        measurement.submitter = data.measurer;
        measurement.currency_code = data.currency_code;
        measurement.price = data.price;  // Already in smallest unit
        measurement.volume = 1.0;  // Normalized to 1 liter
        measurement.volume_unit = "L";
        measurement.price_per_liter = data.price;
        measurement.location = (data.proof_type == "gps_photo") ? data.proof_data : "";
        measurement.source_url = (data.proof_type == "url") ? data.proof_data : "";
        measurement.proof_image_hash = "";  // Could extract from GPS proof data
        measurement.timestamp = data.timestamp;
        measurement.block_height = height;
        measurement.is_validated = true;  // Validated by blockchain consensus
        measurement.validators = {};  // No additional validators needed (blockchain validated)
        measurement.confidence_score = 1.0;  // Maximum confidence (in blockchain)
        measurement.invite_id = data.invite_id;
        measurement.source = (data.proof_type == "url") ? 
            OMeasurement::MeasurementSource::USER_ONLINE : 
            OMeasurement::MeasurementSource::USER_OFFLINE;
        // auto_validation will be default-initialized
        
        if (!OMeasurement::g_measurement_db->WriteWaterPrice(measurement.measurement_id, measurement)) {
            LogPrintf("O Validation: Failed to write water price to database\n");
            return false;
        }
        
        LogPrintf("O Validation: Water price stored: %s = %.6f at height %d\n",
                 data.currency_code.c_str(), data.GetPriceAsDouble(), height);
    }
    
    return true;
}

bool ProcessExchangeRateMeasurement(
    const OTransactions::CExchangeRateMeasurementData& data,
    const CTransaction& tx,
    int height
) {
    LogPrintf("O Validation: Processing exchange rate measurement %s/%s at height %d\n",
             data.from_currency.c_str(), data.to_currency.c_str(), height);
    
    // Validate data structure
    if (!data.IsValid()) {
        LogPrintf("O Validation: Invalid exchange rate measurement data\n");
        return false;
    }
    
    // Validate measurer is verified
    if (!IsMeasurerVerified(data.measurer)) {
        LogPrintf("O Validation: Measurer not verified for exchange rate measurement\n");
        return false;
    }
    
    // Validate measurement invitation
    if (!ValidateMeasurementInvitation(data.invite_id, data.measurer)) {
        LogPrintf("O Validation: Invalid or expired measurement invitation\n");
        return false;
    }
    
    // TODO: Validate proof data
    // TODO: Validate signature
    
    // Store in measurement database
    if (OMeasurement::g_measurement_db) {
        OMeasurement::ExchangeRateMeasurement measurement;
        measurement.measurement_id = data.GetHash();
        measurement.submitter = data.measurer;
        measurement.from_currency = data.from_currency;
        measurement.to_currency = data.to_currency;
        measurement.exchange_rate = data.GetRateAsDouble();
        measurement.location = "";  // No location for exchange rates
        measurement.source_url = data.proof_data;  // URL proof
        measurement.proof_image_hash = "";
        measurement.timestamp = data.timestamp;
        measurement.block_height = height;
        measurement.is_validated = true;  // Validated by blockchain consensus
        measurement.validators = {};  // No additional validators needed
        measurement.invite_id = data.invite_id;
        measurement.source = OMeasurement::MeasurementSource::USER_ONLINE;
        // auto_validation will be default-initialized
        
        if (!OMeasurement::g_measurement_db->WriteExchangeRate(measurement.measurement_id, measurement)) {
            LogPrintf("O Validation: Failed to write exchange rate to database\n");
            return false;
        }
        
        LogPrintf("O Validation: Exchange rate stored: %s/%s = %.6f at height %d\n",
                 data.from_currency.c_str(), data.to_currency.c_str(), 
                 data.GetRateAsDouble(), height);
    }
    
    return true;
}

bool ValidateProviderSignature(const OTransactions::CUserVerificationData& data) {
    if (data.provider_sig.empty()) {
        return false;
    }
    
    // Validate based on identity provider type
    if (data.IsBrightID()) {
        // TODO: Validate against BrightID's public key
        // For now, basic validation
        return data.provider_sig.size() >= 64;
    }
    else if (data.IsKYC()) {
        // TODO: Validate against KYC provider's public key
        // Each country might have different KYC providers
        // Example: kyc_usa, kyc_fra, kyc_mex, etc.
        return data.provider_sig.size() >= 64;
    }
    else if (data.identity_provider == "worldcoin") {
        // TODO: Validate against WorldCoin's public key
        return data.provider_sig.size() >= 64;
    }
    else if (data.identity_provider == "idena") {
        // TODO: Validate against Idena's verification
        return data.provider_sig.size() >= 64;
    }
    
    // Unknown provider - for now, accept if signature exists
    // In production, should maintain a whitelist of approved providers
    return data.provider_sig.size() >= 64;
}

bool ValidateUserSignature(const OTransactions::CUserVerificationData& data) {
    if (data.user_sig.empty() || !data.o_pubkey.IsValid()) {
        return false;
    }
    
    // Verify that user signed the hash of the verification data
    // uint256 hash = data.GetHash();
    
    // TODO: Implement proper signature verification
    // Verify that user_sig is a valid signature of data.GetHash() by o_pubkey
    // For now, basic validation
    return data.user_sig.size() >= 64;  // Minimum signature size
}

bool ValidateMeasurementInvitation(const uint256& invite_id, const CPubKey& measurer) {
    if (!OMeasurement::g_measurement_db) {
        return false;
    }
    
    auto invite = OMeasurement::g_measurement_db->ReadInvite(invite_id);
    if (!invite.has_value()) {
        return false;
    }
    
    // Check if invitation is for this measurer
    if (invite->invited_user != measurer) {
        return false;
    }
    
    // Check if not expired
    int64_t current_time = GetTime();
    if (invite->expires_at < current_time) {
        return false;
    }
    
    // Check if not already used
    if (invite->is_used) {
        return false;
    }
    
    return true;
}

bool ValidateWaterPriceProof(
    const std::string& proof_type,
    const std::string& proof_data,
    const std::string& currency_code
) {
    if (proof_type == "url") {
        // Validate URL format (basic check)
        if (proof_data.find("http://") != 0 && proof_data.find("https://") != 0) {
            return false;
        }
        
        // TODO: In production, validate that URL is accessible and contains water price
        // For now, accept valid URL format
        return proof_data.length() >= 10 && proof_data.length() <= 500;
    }
    else if (proof_type == "gps_photo") {
        // Validate GPS coordinates + photo hash format
        // Expected format: "lat,lon,photohash"
        auto parts = util::SplitString(proof_data, ',');
        if (parts.size() != 3) {
            return false;
        }
        
        // TODO: Validate GPS coordinates are reasonable
        // TODO: Validate photo hash format
        
        return true;
    }
    
    return false;
}

bool IsMeasurerVerified(const CPubKey& measurer) {
    if (!g_brightid_db) {
        return false;
    }
    
    // Get BrightID address for this O pubkey
    std::string pubkey_str = HexStr(measurer);
    auto brightid_addr = g_brightid_db->GetBrightIDAddress(pubkey_str);
    
    if (!brightid_addr.has_value()) {
        return false;
    }
    
    // Check if user is verified
    auto user = g_brightid_db->ReadUser(brightid_addr.value());
    if (!user.has_value()) {
        return false;
    }
    
    return user->IsVerified() && user->IsActive();
}

bool ProcessMeasurementValidation(
    const OTransactions::CMeasurementValidationData& data,
    const CTransaction& tx,
    int height)
{
    // Basic validation
    if (!data.IsValid()) {
        LogPrintf("O Validation: Invalid measurement validation data\n");
        return false;
    }
    
    // Check validator is verified
    if (!IsMeasurerVerified(data.validator)) {
        LogPrintf("O Validation: Validator not verified in BrightID database\n");
        return false;
    }
    
    // TODO: Validate signature
    // if (!ValidateUserSignature(data)) {
    //     LogPrintf("O Validation: Invalid validator signature\n");
    //     return false;
    // }
    
    // Store validation in measurement database
    if (!OMeasurement::g_measurement_db) {
        LogPrintf("O Validation: Measurement database not initialized\n");
        return false;
    }
    
    bool validation_stored = false;
    
    if (data.measurement_type == OTransactions::OTxType::WATER_PRICE) {
        // Read existing water price measurement
        auto measurement = OMeasurement::g_measurement_db->ReadWaterPrice(data.measurement_id);
        if (!measurement.has_value()) {
            LogPrintf("O Validation: Water price measurement not found: %s\n",
                     data.measurement_id.GetHex().c_str());
            return false;
        }
        
        // Check if validator already validated this measurement
        for (const auto& validator : measurement->validators) {
            if (validator == data.validator) {
                LogPrintf("O Validation: Validator already validated this measurement\n");
                return true;  // Not an error, just redundant
            }
        }
        
        // Add validator to the list
        measurement->validators.push_back(data.validator);
        
        // Update validation status if we have enough validators
        if (measurement->validators.size() >= 3) {  // MIN_VALIDATORS_REQUIRED
            measurement->is_validated = true;
            measurement->confidence_score = std::min(1.0, measurement->validators.size() / 10.0);
        }
        
        // Store updated measurement
        validation_stored = OMeasurement::g_measurement_db->WriteWaterPrice(data.measurement_id, measurement.value());
        
        LogPrintf("O Validation: Water price validation stored: %s by %s (total validators: %d)\n",
                 data.measurement_id.GetHex().c_str(),
                 HexStr(data.validator).c_str(),
                 static_cast<int>(measurement->validators.size()));
    }
    else if (data.measurement_type == OTransactions::OTxType::EXCHANGE_RATE) {
        // Read existing exchange rate measurement
        auto measurement = OMeasurement::g_measurement_db->ReadExchangeRate(data.measurement_id);
        if (!measurement.has_value()) {
            LogPrintf("O Validation: Exchange rate measurement not found: %s\n",
                     data.measurement_id.GetHex().c_str());
            return false;
        }
        
        // Check if validator already validated this measurement
        for (const auto& validator : measurement->validators) {
            if (validator == data.validator) {
                LogPrintf("O Validation: Validator already validated this measurement\n");
                return true;  // Not an error, just redundant
            }
        }
        
        // Add validator to the list
        measurement->validators.push_back(data.validator);
        
        // Update validation status if we have enough validators
        if (measurement->validators.size() >= 3) {  // MIN_VALIDATORS_REQUIRED
            measurement->is_validated = true;
        }
        
        // Store updated measurement
        validation_stored = OMeasurement::g_measurement_db->WriteExchangeRate(data.measurement_id, measurement.value());
        
        LogPrintf("O Validation: Exchange rate validation stored: %s by %s (total validators: %d)\n",
                 data.measurement_id.GetHex().c_str(),
                 HexStr(data.validator).c_str(),
                 static_cast<int>(measurement->validators.size()));
    }
    else {
        LogPrintf("O Validation: Invalid measurement type for validation: %d\n",
                 static_cast<int>(data.measurement_type));
        return false;
    }
    
    return validation_stored;
}

bool ProcessMeasurementInvite(
    const OTransactions::CMeasurementInviteData& data,
    const CTransaction& tx,
    int height)
{
    // Basic validation
    if (!data.IsValid()) {
        LogPrintf("O Validation: Invalid measurement invitation data\n");
        return false;
    }
    
    // Check if invited user is verified
    if (!IsMeasurerVerified(data.invited_user)) {
        LogPrintf("O Validation: Invited user not verified in BrightID database\n");
        return false;
    }
    
    // TODO: Validate system signature
    // if (!ValidateSystemSignature(data)) {
    //     LogPrintf("O Validation: Invalid system signature on invitation\n");
    //     return false;
    // }
    
    // Store invitation in measurement database
    if (!OMeasurement::g_measurement_db) {
        LogPrintf("O Validation: Measurement database not initialized\n");
        return false;
    }
    
    // Check if invitation already exists (prevent duplicates)
    auto existing = OMeasurement::g_measurement_db->ReadInvite(data.invite_id);
    if (existing.has_value()) {
        LogPrintf("O Validation: Invitation %s already exists, skipping\n",
                 data.invite_id.GetHex().c_str());
        return true;  // Not an error, just redundant
    }
    
    // Convert to OMeasurement::MeasurementInvite structure
    OMeasurement::MeasurementInvite invite;
    invite.invite_id = data.invite_id;
    invite.invited_user = data.invited_user;
    
    // Convert measurement type
    if (data.measurement_type == 0x02) {
        invite.type = OMeasurement::MeasurementType::WATER_PRICE;
    } else if (data.measurement_type == 0x03) {
        invite.type = OMeasurement::MeasurementType::EXCHANGE_RATE;
    } else {
        LogPrintf("O Validation: Invalid measurement type in invitation: %d\n",
                 static_cast<int>(data.measurement_type));
        return false;
    }
    
    invite.currency_code = data.currency_code;
    invite.created_at = data.created_at;
    invite.expires_at = data.expires_at;
    invite.is_used = false;
    invite.is_expired = (data.expires_at < GetTime());
    invite.block_height = data.block_height;
    
    // Store invitation
    if (!OMeasurement::g_measurement_db->WriteInvite(data.invite_id, invite)) {
        LogPrintf("O Validation: Failed to write invitation to database\n");
        return false;
    }
    
    LogPrintf("O Validation: Measurement invitation stored: %s for user %s (%s) at height %d\n",
             data.invite_id.GetHex().c_str(),
             HexStr(data.invited_user).c_str(),
             data.currency_code.c_str(),
             height);
    
    return true;
}

} // namespace OConsensus

