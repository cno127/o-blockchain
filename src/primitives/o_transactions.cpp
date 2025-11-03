// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/o_transactions.h>

#include <hash.h>
#include <script/script.h>
#include <streams.h>
#include <util/strencodings.h>

namespace OTransactions {

// ===== CUserVerificationData =====

bool CUserVerificationData::IsValid() const {
    // Basic validation
    if (user_id.empty() || identity_provider.empty() || verification_data.empty()) {
        return false;
    }
    
    if (country_code.empty() || country_code.length() != 3) {
        return false;
    }
    
    // Birth currency must be specified and valid format (e.g., "OUSD", "OEUR", "OMXN")
    if (birth_currency.empty() || birth_currency.length() < 4 || birth_currency.length() > 5) {
        return false;
    }
    
    // Birth currency must start with "O"
    if (birth_currency[0] != 'O') {
        return false;
    }
    
    if (provider_sig.empty() || user_sig.empty()) {
        return false;
    }
    
    if (!o_pubkey.IsValid()) {
        return false;
    }
    
    if (timestamp <= 0) {
        return false;
    }
    
    // Validate identity provider format
    // Allowed: "brightid", "kyc_usa", "kyc_fra", "worldcoin", "idena", etc.
    if (identity_provider.length() > 50) {
        return false;
    }
    
    return true;
}

uint256 CUserVerificationData::GetHash() const {
    HashWriter ss{};
    ss << user_id << identity_provider << country_code << birth_currency << verification_data 
       << timestamp << expiration << o_pubkey;
    return ss.GetHash();
}

CScript CUserVerificationData::ToScript() const {
    DataStream ds;
    ds << *this;
    
    std::vector<unsigned char> data(UCharCast(ds.data()), UCharCast(ds.data() + ds.size()));
    
    CScript script;
    script << OP_RETURN;
    script << O_TX_PREFIX;
    script << O_TX_VERSION;
    script << static_cast<uint8_t>(OTxType::USER_VERIFY);
    script << data;
    
    return script;
}

bool CUserVerificationData::FromScript(const CScript& script, CUserVerificationData& data) {
    if (!script.IsUnspendable()) {
        return false;
    }
    
    CScript::const_iterator pc = script.begin();
    opcodetype opcode;
    std::vector<unsigned char> vch;
    
    // Read OP_RETURN
    if (!script.GetOp(pc, opcode) || opcode != OP_RETURN) {
        return false;
    }
    
    // Read prefix
    if (!script.GetOp(pc, opcode, vch) || vch != O_TX_PREFIX) {
        return false;
    }
    
    // Read version
    if (!script.GetOp(pc, opcode, vch) || vch.size() != 1 || vch[0] != O_TX_VERSION) {
        return false;
    }
    
    // Read type
    if (!script.GetOp(pc, opcode, vch) || vch.size() != 1 || 
        vch[0] != static_cast<uint8_t>(OTxType::USER_VERIFY)) {
        return false;
    }
    
    // Read data
    if (!script.GetOp(pc, opcode, vch)) {
        return false;
    }
    
    try {
        DataStream ds{vch};
        ds >> data;
        return data.IsValid();
    } catch (...) {
        return false;
    }
}

// ===== CWaterPriceMeasurementData =====

bool CWaterPriceMeasurementData::IsValid() const {
    if (currency_code.empty() || currency_code.length() != 3) {
        return false;
    }
    
    if (price <= 0 || price > 1000000000) {  // Max $1000 per liter (anti-spam)
        return false;
    }
    
    if (!measurer.IsValid()) {
        return false;
    }
    
    if (timestamp <= 0) {
        return false;
    }
    
    if (invite_id.IsNull()) {
        return false;
    }
    
    if (proof_type != "url" && proof_type != "gps_photo") {
        return false;
    }
    
    if (proof_data.empty()) {
        return false;
    }
    
    if (signature.empty()) {
        return false;
    }
    
    return true;
}

uint256 CWaterPriceMeasurementData::GetHash() const {
    HashWriter ss{};
    ss << currency_code << price << measurer << timestamp << invite_id << proof_type << proof_data;
    return ss.GetHash();
}

CScript CWaterPriceMeasurementData::ToScript() const {
    DataStream ds;
    ds << *this;
    
    std::vector<unsigned char> data(UCharCast(ds.data()), UCharCast(ds.data() + ds.size()));
    
    CScript script;
    script << OP_RETURN;
    script << O_TX_PREFIX;
    script << O_TX_VERSION;
    script << static_cast<uint8_t>(OTxType::WATER_PRICE);
    script << data;
    
    return script;
}

bool CWaterPriceMeasurementData::FromScript(const CScript& script, CWaterPriceMeasurementData& data) {
    if (!script.IsUnspendable()) {
        return false;
    }
    
    CScript::const_iterator pc = script.begin();
    opcodetype opcode;
    std::vector<unsigned char> vch;
    
    // Read OP_RETURN
    if (!script.GetOp(pc, opcode) || opcode != OP_RETURN) {
        return false;
    }
    
    // Read prefix
    if (!script.GetOp(pc, opcode, vch) || vch != O_TX_PREFIX) {
        return false;
    }
    
    // Read version
    if (!script.GetOp(pc, opcode, vch) || vch.size() != 1 || vch[0] != O_TX_VERSION) {
        return false;
    }
    
    // Read type
    if (!script.GetOp(pc, opcode, vch) || vch.size() != 1 || 
        vch[0] != static_cast<uint8_t>(OTxType::WATER_PRICE)) {
        return false;
    }
    
    // Read data
    if (!script.GetOp(pc, opcode, vch)) {
        return false;
    }
    
    try {
        DataStream ds{vch};
        ds >> data;
        return data.IsValid();
    } catch (...) {
        return false;
    }
}

// ===== CExchangeRateMeasurementData =====

bool CExchangeRateMeasurementData::IsValid() const {
    if (from_currency.empty() || to_currency.empty()) {
        return false;
    }
    
    if (exchange_rate <= 0) {
        return false;
    }
    
    if (!measurer.IsValid()) {
        return false;
    }
    
    if (timestamp <= 0) {
        return false;
    }
    
    if (invite_id.IsNull()) {
        return false;
    }
    
    if (proof_data.empty()) {
        return false;
    }
    
    if (signature.empty()) {
        return false;
    }
    
    return true;
}

uint256 CExchangeRateMeasurementData::GetHash() const {
    HashWriter ss{};
    ss << from_currency << to_currency << exchange_rate << measurer << timestamp << invite_id << proof_data;
    return ss.GetHash();
}

CScript CExchangeRateMeasurementData::ToScript() const {
    DataStream ds;
    ds << *this;
    
    std::vector<unsigned char> data(UCharCast(ds.data()), UCharCast(ds.data() + ds.size()));
    
    CScript script;
    script << OP_RETURN;
    script << O_TX_PREFIX;
    script << O_TX_VERSION;
    script << static_cast<uint8_t>(OTxType::EXCHANGE_RATE);
    script << data;
    
    return script;
}

bool CExchangeRateMeasurementData::FromScript(const CScript& script, CExchangeRateMeasurementData& data) {
    if (!script.IsUnspendable()) {
        return false;
    }
    
    CScript::const_iterator pc = script.begin();
    opcodetype opcode;
    std::vector<unsigned char> vch;
    
    // Read OP_RETURN
    if (!script.GetOp(pc, opcode) || opcode != OP_RETURN) {
        return false;
    }
    
    // Read prefix
    if (!script.GetOp(pc, opcode, vch) || vch != O_TX_PREFIX) {
        return false;
    }
    
    // Read version
    if (!script.GetOp(pc, opcode, vch) || vch.size() != 1 || vch[0] != O_TX_VERSION) {
        return false;
    }
    
    // Read type
    if (!script.GetOp(pc, opcode, vch) || vch.size() != 1 || 
        vch[0] != static_cast<uint8_t>(OTxType::EXCHANGE_RATE)) {
        return false;
    }
    
    // Read data
    if (!script.GetOp(pc, opcode, vch)) {
        return false;
    }
    
    try {
        DataStream ds{vch};
        ds >> data;
        return data.IsValid();
    } catch (...) {
        return false;
    }
}

// ===== Helper Functions =====

bool IsOTransaction(const CTransaction& tx) {
    // Check all outputs for OP_RETURN with O prefix
    for (const auto& output : tx.vout) {
        const CScript& script = output.scriptPubKey;
        
        if (!script.IsUnspendable()) {
            continue;
        }
        
        CScript::const_iterator pc = script.begin();
        opcodetype opcode;
        std::vector<unsigned char> vch;
        
        // Check for OP_RETURN
        if (!script.GetOp(pc, opcode) || opcode != OP_RETURN) {
            continue;
        }
        
        // Check for O prefix
        if (script.GetOp(pc, opcode, vch) && vch == O_TX_PREFIX) {
            return true;
        }
    }
    
    return false;
}

std::optional<OTxType> GetOTxType(const CTransaction& tx) {
    // Find OP_RETURN output with O prefix
    for (const auto& output : tx.vout) {
        const CScript& script = output.scriptPubKey;
        
        if (!script.IsUnspendable()) {
            continue;
        }
        
        CScript::const_iterator pc = script.begin();
        opcodetype opcode;
        std::vector<unsigned char> vch;
        
        // Read OP_RETURN
        if (!script.GetOp(pc, opcode) || opcode != OP_RETURN) {
            continue;
        }
        
        // Read prefix
        if (!script.GetOp(pc, opcode, vch) || vch != O_TX_PREFIX) {
            continue;
        }
        
        // Read version
        if (!script.GetOp(pc, opcode, vch) || vch.size() != 1 || vch[0] != O_TX_VERSION) {
            continue;
        }
        
        // Read type
        if (!script.GetOp(pc, opcode, vch) || vch.size() != 1) {
            continue;
        }
        
        return static_cast<OTxType>(vch[0]);
    }
    
    return std::nullopt;
}

std::optional<CUserVerificationData> ExtractUserVerification(const CTransaction& tx) {
    for (const auto& output : tx.vout) {
        CUserVerificationData data;
        if (CUserVerificationData::FromScript(output.scriptPubKey, data)) {
            return data;
        }
    }
    return std::nullopt;
}

std::optional<CWaterPriceMeasurementData> ExtractWaterPriceMeasurement(const CTransaction& tx) {
    for (const auto& output : tx.vout) {
        CWaterPriceMeasurementData data;
        if (CWaterPriceMeasurementData::FromScript(output.scriptPubKey, data)) {
            return data;
        }
    }
    return std::nullopt;
}

std::optional<CExchangeRateMeasurementData> ExtractExchangeRateMeasurement(const CTransaction& tx) {
    for (const auto& output : tx.vout) {
        CExchangeRateMeasurementData data;
        if (CExchangeRateMeasurementData::FromScript(output.scriptPubKey, data)) {
            return data;
        }
    }
    return std::nullopt;
}

// ===== CMeasurementValidationData =====

bool CMeasurementValidationData::IsValid() const {
    // Basic validation
    if (measurement_id.IsNull()) {
        return false;
    }
    
    if (measurement_type != OTxType::WATER_PRICE && measurement_type != OTxType::EXCHANGE_RATE) {
        return false;
    }
    
    if (!validator.IsValid()) {
        return false;
    }
    
    if (timestamp <= 0) {
        return false;
    }
    
    if (signature.empty()) {
        return false;
    }
    
    return true;
}

uint256 CMeasurementValidationData::GetHash() const {
    HashWriter ss{};
    ss << measurement_id << static_cast<uint8_t>(measurement_type) << validator 
       << validation_result << timestamp << validation_notes;
    return ss.GetHash();
}

CScript CMeasurementValidationData::ToScript() const {
    DataStream ds;
    ds << *this;
    
    std::vector<unsigned char> data(UCharCast(ds.data()), UCharCast(ds.data() + ds.size()));
    
    // Build OP_RETURN script: OP_RETURN <O_TX_PREFIX> <MEASUREMENT_VALIDATION> <serialized data>
    CScript script;
    script << OP_RETURN;
    script << std::vector<unsigned char>(O_TX_PREFIX.begin(), O_TX_PREFIX.end());
    script << static_cast<uint8_t>(OTxType::MEASUREMENT_VALIDATION);
    script << data;
    
    return script;
}

bool CMeasurementValidationData::FromScript(const CScript& script, CMeasurementValidationData& data) {
    auto pc = script.begin();
    std::vector<unsigned char> vch;
    opcodetype opcode;
    
    // Check for OP_RETURN
    if (!script.GetOp(pc, opcode) || opcode != OP_RETURN) {
        return false;
    }
    
    // Check for O_TX_PREFIX
    if (!script.GetOp(pc, opcode, vch)) {
        return false;
    }
    if (vch.size() != O_TX_PREFIX.size() || 
        !std::equal(vch.begin(), vch.end(), O_TX_PREFIX.begin())) {
        return false;
    }
    
    // Check for MEASUREMENT_VALIDATION type
    if (!script.GetOp(pc, opcode, vch)) {
        return false;
    }
    if (vch.size() != 1 || vch[0] != static_cast<uint8_t>(OTxType::MEASUREMENT_VALIDATION)) {
        return false;
    }
    
    // Parse serialized data
    if (!script.GetOp(pc, opcode, vch)) {
        return false;
    }
    
    try {
        DataStream ds{vch};
        ds >> data;
        return true;
    } catch (...) {
        return false;
    }
}

std::optional<CMeasurementValidationData> ExtractMeasurementValidation(const CTransaction& tx) {
    for (const auto& output : tx.vout) {
        CMeasurementValidationData data;
        if (CMeasurementValidationData::FromScript(output.scriptPubKey, data)) {
            return data;
        }
    }
    return std::nullopt;
}

// ===== CMeasurementInviteData =====

bool CMeasurementInviteData::IsValid() const {
    // Basic validation
    if (invite_id.IsNull()) {
        return false;
    }
    
    if (!invited_user.IsValid()) {
        return false;
    }
    
    if (measurement_type != 0x02 && measurement_type != 0x03) {
        return false;
    }
    
    if (created_at <= 0 || expires_at <= created_at) {
        return false;
    }
    
    if (block_height < 0) {
        return false;
    }
    
    // Signature can be empty for now (TODO: add system signature validation)
    
    return true;
}

uint256 CMeasurementInviteData::GetHash() const {
    HashWriter ss{};
    ss << invite_id << invited_user << measurement_type << currency_code 
       << created_at << expires_at << block_height;
    return ss.GetHash();
}

CScript CMeasurementInviteData::ToScript() const {
    DataStream ds;
    ds << *this;
    
    std::vector<unsigned char> data(UCharCast(ds.data()), UCharCast(ds.data() + ds.size()));
    
    // Build OP_RETURN script: OP_RETURN <O_TX_PREFIX> <MEASUREMENT_INVITE> <serialized data>
    CScript script;
    script << OP_RETURN;
    script << std::vector<unsigned char>(O_TX_PREFIX.begin(), O_TX_PREFIX.end());
    script << static_cast<uint8_t>(OTxType::MEASUREMENT_INVITE);
    script << data;
    
    return script;
}

bool CMeasurementInviteData::FromScript(const CScript& script, CMeasurementInviteData& data) {
    auto pc = script.begin();
    std::vector<unsigned char> vch;
    opcodetype opcode;
    
    // Check for OP_RETURN
    if (!script.GetOp(pc, opcode) || opcode != OP_RETURN) {
        return false;
    }
    
    // Check for O_TX_PREFIX
    if (!script.GetOp(pc, opcode, vch)) {
        return false;
    }
    if (vch.size() != O_TX_PREFIX.size() || 
        !std::equal(vch.begin(), vch.end(), O_TX_PREFIX.begin())) {
        return false;
    }
    
    // Check for MEASUREMENT_INVITE type
    if (!script.GetOp(pc, opcode, vch)) {
        return false;
    }
    if (vch.size() != 1 || vch[0] != static_cast<uint8_t>(OTxType::MEASUREMENT_INVITE)) {
        return false;
    }
    
    // Parse serialized data
    if (!script.GetOp(pc, opcode, vch)) {
        return false;
    }
    
    try {
        DataStream ds{vch};
        ds >> data;
        return true;
    } catch (...) {
        return false;
    }
}

std::optional<CMeasurementInviteData> ExtractMeasurementInvite(const CTransaction& tx) {
    for (const auto& output : tx.vout) {
        CMeasurementInviteData data;
        if (CMeasurementInviteData::FromScript(output.scriptPubKey, data)) {
            return data;
        }
    }
    return std::nullopt;
}

} // namespace OTransactions

