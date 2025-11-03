// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PRIMITIVES_O_TRANSACTIONS_H
#define BITCOIN_PRIMITIVES_O_TRANSACTIONS_H

#include <primitives/transaction.h>
#include <pubkey.h>
#include <script/script.h>
#include <serialize.h>
#include <uint256.h>
#include <util/strencodings.h>

#include <string>
#include <vector>

namespace OTransactions {

/** O Blockchain Transaction Types */
enum class OTxType : uint8_t {
    USER_VERIFY = 0x01,          // Generic user verification (Proof of Personhood)
    WATER_PRICE = 0x02,          // Water price measurement
    EXCHANGE_RATE = 0x03,        // Exchange rate measurement
    BUSINESS_REGISTER = 0x04,    // Business miner registration (future)
    MEASUREMENT_VALIDATION = 0x05, // Human validation of a measurement
    MEASUREMENT_INVITE = 0x06    // Measurement invitation (consensus on who measures what)
};

/** O Blockchain Transaction Version */
static constexpr uint8_t O_TX_VERSION = 0x01;

/** OP_RETURN prefix for O transactions: "OBLK" in hex */
static const std::vector<unsigned char> O_TX_PREFIX = {0x4F, 0x42, 0x4C, 0x4B};

/**
 * Generic User Verification Transaction Data
 * 
 * Stores a user's Proof of Personhood verification in the blockchain.
 * Supports multiple identity providers: BrightID, KYC, WorldCoin, etc.
 * Once included in a block, all nodes recognize this user as verified.
 */
class CUserVerificationData {
public:
    std::string user_id;                     // Unique user identifier (from identity provider)
    std::string identity_provider;           // Provider: "brightid", "kyc_usa", "kyc_fra", "worldcoin", etc.
    std::string country_code;                // User's country (ISO 3166-1 alpha-3): "USA", "FRA", "MEX"
    std::string birth_currency;              // Birth currency for UBI (OUSD, OEUR, OMXN, etc.) - IMMUTABLE!
    std::string verification_data;           // JSON from identity provider (contains proof)
    std::vector<unsigned char> provider_sig; // Identity provider's signature
    int64_t timestamp;                       // When verification occurred
    int64_t expiration;                      // When verification expires (0 = never)
    CPubKey o_pubkey;                        // O Blockchain public key to link
    std::vector<unsigned char> user_sig;     // User's signature (proves ownership of o_pubkey)
    
    CUserVerificationData()
        : user_id(), identity_provider(), country_code(), birth_currency(), verification_data(),
          provider_sig(), timestamp(0), expiration(0), o_pubkey(), user_sig() {}
    
    SERIALIZE_METHODS(CUserVerificationData, obj) {
        READWRITE(obj.user_id, obj.identity_provider, obj.country_code, obj.birth_currency,
                  obj.verification_data, obj.provider_sig, obj.timestamp, obj.expiration, 
                  obj.o_pubkey, obj.user_sig);
    }
    
    /** Validate the verification data */
    bool IsValid() const;
    
    /** Get hash for signing/verification */
    uint256 GetHash() const;
    
    /** Create OP_RETURN script for this verification */
    CScript ToScript() const;
    
    /** Parse from OP_RETURN script */
    static bool FromScript(const CScript& script, CUserVerificationData& data);
    
    /** Check if this is a BrightID verification */
    bool IsBrightID() const { return identity_provider == "brightid"; }
    
    /** Check if this is a KYC verification */
    bool IsKYC() const { return identity_provider.substr(0, 4) == "kyc_"; }
    
    /** Get identity provider type */
    std::string GetProviderType() const { return identity_provider; }
};

/**
 * Water Price Measurement Transaction Data
 * 
 * Stores a water price measurement in the blockchain.
 * Used to calculate O currency exchange rates.
 */
class CWaterPriceMeasurementData {
public:
    std::string currency_code;               // USD, EUR, JPY, etc.
    int64_t price;                           // Price * 1,000,000 (6 decimal places)
    CPubKey measurer;                        // Who submitted the measurement
    int64_t timestamp;                       // When measured
    uint256 invite_id;                       // Measurement invitation ID
    std::string proof_type;                  // "url" or "gps_photo"
    std::string proof_data;                  // URL or GPS coords + photo hash
    std::vector<unsigned char> signature;    // Measurer's signature
    
    CWaterPriceMeasurementData()
        : currency_code(), price(0), measurer(), timestamp(0),
          invite_id(), proof_type(), proof_data(), signature() {}
    
    SERIALIZE_METHODS(CWaterPriceMeasurementData, obj) {
        READWRITE(obj.currency_code, obj.price, obj.measurer, obj.timestamp,
                  obj.invite_id, obj.proof_type, obj.proof_data, obj.signature);
    }
    
    /** Validate the measurement data */
    bool IsValid() const;
    
    /** Get price as double */
    double GetPriceAsDouble() const { return static_cast<double>(price) / 1000000.0; }
    
    /** Get hash for signing/verification */
    uint256 GetHash() const;
    
    /** Create OP_RETURN script for this measurement */
    CScript ToScript() const;
    
    /** Parse from OP_RETURN script */
    static bool FromScript(const CScript& script, CWaterPriceMeasurementData& data);
};

/**
 * Exchange Rate Measurement Transaction Data
 * 
 * Stores an observed exchange rate in the blockchain.
 * Used for stability monitoring and triggering stabilization.
 */
class CExchangeRateMeasurementData {
public:
    std::string from_currency;               // e.g., "OUSD"
    std::string to_currency;                 // e.g., "USD"
    int64_t exchange_rate;                   // Rate * 1,000,000 (6 decimal places)
    CPubKey measurer;                        // Who submitted the measurement
    int64_t timestamp;                       // When measured
    uint256 invite_id;                       // Measurement invitation ID
    std::string proof_data;                  // Exchange platform URL or proof
    std::vector<unsigned char> signature;    // Measurer's signature
    
    CExchangeRateMeasurementData()
        : from_currency(), to_currency(), exchange_rate(0), measurer(),
          timestamp(0), invite_id(), proof_data(), signature() {}
    
    SERIALIZE_METHODS(CExchangeRateMeasurementData, obj) {
        READWRITE(obj.from_currency, obj.to_currency, obj.exchange_rate,
                  obj.measurer, obj.timestamp, obj.invite_id, obj.proof_data, obj.signature);
    }
    
    /** Validate the measurement data */
    bool IsValid() const;
    
    /** Get rate as double */
    double GetRateAsDouble() const { return static_cast<double>(exchange_rate) / 1000000.0; }
    
    /** Get hash for signing/verification */
    uint256 GetHash() const;
    
    /** Create OP_RETURN script for this measurement */
    CScript ToScript() const;
    
    /** Parse from OP_RETURN script */
    static bool FromScript(const CScript& script, CExchangeRateMeasurementData& data);
};

/**
 * Measurement Validation Transaction Data
 * 
 * Stores a human validation of a water price or exchange rate measurement.
 * Multiple validators can submit validations for the same measurement.
 */
class CMeasurementValidationData {
public:
    uint256 measurement_id;                  // ID of measurement being validated
    OTxType measurement_type;                // WATER_PRICE or EXCHANGE_RATE
    CPubKey validator;                       // Who is validating
    bool validation_result;                  // true = valid, false = invalid
    int64_t timestamp;                       // When validated
    std::string validation_notes;            // Optional notes (e.g., why invalid)
    std::vector<unsigned char> signature;    // Validator's signature
    
    CMeasurementValidationData()
        : measurement_id(), measurement_type(OTxType::WATER_PRICE), validator(),
          validation_result(true), timestamp(0), validation_notes(), signature() {}
    
    SERIALIZE_METHODS(CMeasurementValidationData, obj) {
        uint8_t type_val = static_cast<uint8_t>(obj.measurement_type);
        READWRITE(obj.measurement_id, type_val, obj.validator, obj.validation_result,
                  obj.timestamp, obj.validation_notes, obj.signature);
        SER_READ(obj, obj.measurement_type = static_cast<OTxType>(type_val));
    }
    
    /** Validate the validation data (meta!) */
    bool IsValid() const;
    
    /** Get hash for signing/verification */
    uint256 GetHash() const;
    
    /** Create OP_RETURN script for this validation */
    CScript ToScript() const;
    
    /** Parse from OP_RETURN script */
    static bool FromScript(const CScript& script, CMeasurementValidationData& data);
};

/**
 * Measurement Invitation Transaction Data
 * 
 * Stores a measurement invitation in the blockchain.
 * Ensures all nodes agree on which users are invited to measure what.
 */
class CMeasurementInviteData {
public:
    uint256 invite_id;                       // Unique invitation ID
    CPubKey invited_user;                    // Who is invited to measure
    uint8_t measurement_type;                // 0x02 (WATER_PRICE) or 0x03 (EXCHANGE_RATE)
    std::string currency_code;               // Currency to measure (optional for specific currency)
    int64_t created_at;                      // When invitation was created
    int64_t expires_at;                      // When invitation expires
    int block_height;                        // Block height when created
    std::vector<unsigned char> signature;    // System/miner signature
    
    CMeasurementInviteData()
        : invite_id(), invited_user(), measurement_type(0x02), currency_code(),
          created_at(0), expires_at(0), block_height(0), signature() {}
    
    SERIALIZE_METHODS(CMeasurementInviteData, obj) {
        READWRITE(obj.invite_id, obj.invited_user, obj.measurement_type, obj.currency_code,
                  obj.created_at, obj.expires_at, obj.block_height, obj.signature);
    }
    
    /** Validate the invitation data */
    bool IsValid() const;
    
    /** Get hash for signing/verification */
    uint256 GetHash() const;
    
    /** Create OP_RETURN script for this invitation */
    CScript ToScript() const;
    
    /** Parse from OP_RETURN script */
    static bool FromScript(const CScript& script, CMeasurementInviteData& data);
};

/**
 * Helper functions for O transactions
 */

/** Check if a transaction contains O-specific data */
bool IsOTransaction(const CTransaction& tx);

/** Get O transaction type from transaction */
std::optional<OTxType> GetOTxType(const CTransaction& tx);

/** Extract user verification data from transaction */
std::optional<CUserVerificationData> ExtractUserVerification(const CTransaction& tx);

/** Extract water price measurement data from transaction */
std::optional<CWaterPriceMeasurementData> ExtractWaterPriceMeasurement(const CTransaction& tx);

/** Extract exchange rate measurement data from transaction */
std::optional<CExchangeRateMeasurementData> ExtractExchangeRateMeasurement(const CTransaction& tx);

/** Extract measurement validation data from transaction */
std::optional<CMeasurementValidationData> ExtractMeasurementValidation(const CTransaction& tx);

/** Extract measurement invitation data from transaction */
std::optional<CMeasurementInviteData> ExtractMeasurementInvite(const CTransaction& tx);

} // namespace OTransactions

#endif // BITCOIN_PRIMITIVES_O_TRANSACTIONS_H

