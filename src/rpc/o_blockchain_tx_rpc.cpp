// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * RPC commands for creating and broadcasting O Blockchain transactions
 * 
 * These commands create actual blockchain transactions (with OP_RETURN)
 * containing O-specific data that will be validated and stored by all nodes.
 */

#include <rpc/server.h>
#include <rpc/server_util.h>
#include <rpc/util.h>
#include <consensus/validation.h>
#include <core_io.h>
#include <key_io.h>
#include <node/context.h>
#include <policy/policy.h>
#include <primitives/o_transactions.h>
#include <primitives/transaction.h>
#include <rpc/blockchain.h>
#include <rpc/request.h>
#include <univalue.h>
#include <util/moneystr.h>
#include <util/strencodings.h>
#include <validation.h>
#include <wallet/spend.h>
#include <wallet/wallet.h>
#include <wallet/rpc/wallet.h>
#include <wallet/rpc/util.h>

#include <optional>

using namespace OTransactions;
using wallet::CWallet;
using wallet::GetWalletForJSONRPCRequest;

// Helper function to create and broadcast O transaction
static UniValue CreateAndBroadcastOTransaction(
    const CScript& opReturnScript,
    const std::string& txType,
    CWallet* pwallet)
{
    // Create transaction
    CMutableTransaction mtx;
    mtx.version = CTransaction::CURRENT_VERSION;
    
    // Add OP_RETURN output (no value, just data)
    CTxOut opReturnOut;
    opReturnOut.nValue = 0;
    opReturnOut.scriptPubKey = opReturnScript;
    mtx.vout.push_back(opReturnOut);
    
    // Add change output (wallet will fill this in CreateTransaction)
    
    // Create funding transaction using wallet
    if (!pwallet) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Wallet not found");
    }
    
    // TODO: Implement full wallet integration
    // For now, return transaction hex for manual broadcast
    
    CTransaction tx(mtx);
    
    UniValue result(UniValue::VOBJ);
    result.pushKV("txid", tx.GetHash().GetHex());
    result.pushKV("tx_hex", EncodeHexTx(tx));
    result.pushKV("type", txType);
    result.pushKV("status", "created");
    result.pushKV("note", "Transaction created. Use sendrawtransaction to broadcast.");
    
    return result;
}

// ===== RPC Command Implementations =====

static RPCHelpMan submituserverificationtx()
{
    return RPCHelpMan{
        "submituserverificationtx",
        "\nCreate a USER_VERIFY blockchain transaction.\n"
        "This transaction will be validated and stored by all nodes.\n",
        {
            {"user_id", RPCArg::Type::STR, RPCArg::Optional::NO, "Unique user ID from identity provider"},
            {"identity_provider", RPCArg::Type::STR, RPCArg::Optional::NO, "Provider: 'brightid', 'kyc_usa', 'worldcoin', etc."},
            {"country_code", RPCArg::Type::STR, RPCArg::Optional::NO, "ISO 3166-1 alpha-3 country code (USA, FRA, MEX)"},
            {"birth_currency", RPCArg::Type::STR, RPCArg::Optional::NO, "Birth currency for UBI (OUSD, OEUR, OMXN) - IMMUTABLE!"},
            {"verification_data", RPCArg::Type::STR, RPCArg::Optional::NO, "JSON verification data from provider"},
            {"provider_signature", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Provider's signature (hex)"},
            {"expiration", RPCArg::Type::NUM, RPCArg::Default{0}, "Expiration timestamp (0 = never)"},
            {"o_pubkey", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "O Blockchain pubkey (default = wallet's pubkey)"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "txid", "Transaction ID"},
                {RPCResult::Type::STR_HEX, "tx_hex", "Transaction hex for broadcasting"},
                {RPCResult::Type::STR, "type", "Transaction type (USER_VERIFY)"},
                {RPCResult::Type::STR, "user_key", "Composite key: provider:userid"},
                {RPCResult::Type::STR, "birth_currency", "Birth currency"},
            }
        },
        RPCExamples{
            HelpExampleCli("submituserverificationtx", "\"0x1a2b...\" \"brightid\" \"USA\" \"OUSD\" '{\"score\":95}' \"0xabcd...\"")
            + HelpExampleRpc("submituserverificationtx", "\"0x1a2b...\", \"brightid\", \"USA\", \"OUSD\", '{\"score\":95}', \"0xabcd...\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            // Get wallet
            std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
            if (!pwallet) throw JSONRPCError(RPC_WALLET_NOT_FOUND, "Wallet not found");
            
            // Parse parameters
            CUserVerificationData data;
            data.user_id = request.params[0].get_str();
            data.identity_provider = request.params[1].get_str();
            data.country_code = request.params[2].get_str();
            data.birth_currency = request.params[3].get_str();
            data.verification_data = request.params[4].get_str();
            data.provider_sig = ParseHex(request.params[5].get_str());
            data.timestamp = GetTime();
            data.expiration = request.params[6].isNull() ? 0 : request.params[6].getInt<int64_t>();
            
            // Get O pubkey from wallet
            if (request.params.size() > 7 && !request.params[7].isNull()) {
                // TODO: Parse provided pubkey
                throw JSONRPCError(RPC_MISC_ERROR, "Custom pubkey not yet supported");
            }
            
            // TODO: Get pubkey from wallet
            // For now, create empty pubkey
            data.o_pubkey = CPubKey();
            
            // TODO: Sign with user's private key
            data.user_sig = {};
            
            // Validate data
            if (!data.IsValid()) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid user verification data");
            }
            
            // Create OP_RETURN script
            CScript opReturnScript = data.ToScript();
            
            // Create and broadcast transaction
            UniValue result = CreateAndBroadcastOTransaction(opReturnScript, "USER_VERIFY", pwallet.get());
            
            // Add extra info
            std::string user_key = data.identity_provider + ":" + data.user_id;
            result.pushKV("user_key", user_key);
            result.pushKV("birth_currency", data.birth_currency);
            result.pushKV("country", data.country_code);
            
            return result;
        },
    };
}

static RPCHelpMan submitwaterpricetx()
{
    return RPCHelpMan{
        "submitwaterpricetx",
        "\nCreate a WATER_PRICE blockchain transaction.\n"
        "This transaction will be validated and stored by all nodes.\n",
        {
            {"currency_code", RPCArg::Type::STR, RPCArg::Optional::NO, "Currency code (USD, EUR, JPY, etc.)"},
            {"price", RPCArg::Type::NUM, RPCArg::Optional::NO, "Price * 1,000,000 (6 decimal places)"},
            {"invite_id", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Measurement invitation ID"},
            {"proof_type", RPCArg::Type::STR, RPCArg::Optional::NO, "'url' or 'gps_photo'"},
            {"proof_data", RPCArg::Type::STR, RPCArg::Optional::NO, "URL or GPS coords + photo hash"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "txid", "Transaction ID"},
                {RPCResult::Type::STR_HEX, "tx_hex", "Transaction hex for broadcasting"},
                {RPCResult::Type::STR, "type", "Transaction type (WATER_PRICE)"},
                {RPCResult::Type::STR, "currency", "Currency measured"},
                {RPCResult::Type::NUM, "price", "Price value"},
            }
        },
        RPCExamples{
            HelpExampleCli("submitwaterpricetx", "\"USD\" 1500000 \"abc123...\" \"url\" \"https://walmart.com/water\"")
            + HelpExampleRpc("submitwaterpricetx", "\"USD\", 1500000, \"abc123...\", \"url\", \"https://walmart.com/water\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            // Get wallet
            std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
            if (!pwallet) throw JSONRPCError(RPC_WALLET_NOT_FOUND, "Wallet not found");
            
            LOCK(pwallet->cs_wallet);
            
            // Ensure wallet is unlocked for signing
            EnsureWalletIsUnlocked(*pwallet);
            
            // Parse parameters
            CWaterPriceMeasurementData data;
            data.currency_code = request.params[0].get_str();
            data.price = request.params[1].getInt<int64_t>();
            data.invite_id = ParseHashV(request.params[2], "invite_id");
            data.proof_type = request.params[3].get_str();
            data.proof_data = request.params[4].get_str();
            data.timestamp = GetTime();
            
            // Get measurer pubkey from wallet
            util::Result<CTxDestination> dest_result = pwallet->GetNewDestination(OutputType::LEGACY, "measurement");
            if (!dest_result) {
                throw JSONRPCError(RPC_WALLET_ERROR, "Failed to get wallet address: " + util::ErrorString(dest_result).original);
            }
            
            CTxDestination dest = *dest_result;
            const PKHash* pkhash = std::get_if<PKHash>(&dest);
            if (!pkhash) {
                throw JSONRPCError(RPC_WALLET_ERROR, "Address is not a valid public key hash");
            }
            
            CKeyID keyid = ToKeyID(*pkhash);
            
            // Get public key
            CPubKey measurer_pubkey;
            CScript script_pubkey = GetScriptForDestination(dest);
            std::unique_ptr<SigningProvider> provider = pwallet->GetSolvingProvider(script_pubkey);
            if (!provider || !provider->GetPubKey(keyid, measurer_pubkey)) {
                throw JSONRPCError(RPC_WALLET_ERROR, "Failed to get public key from wallet");
            }
            
            data.measurer = measurer_pubkey;
            
            // Sign the measurement with measurer's private key
            CKey private_key;
            if (!provider->GetKey(keyid, private_key)) {
                throw JSONRPCError(RPC_WALLET_ERROR, "Failed to get private key for signing");
            }
            
            uint256 hash = data.GetHash();
            std::vector<unsigned char> signature_vec;
            if (!private_key.SignCompact(hash, signature_vec)) {
                throw JSONRPCError(RPC_WALLET_ERROR, "Failed to sign measurement");
            }
            
            data.signature = signature_vec;
            
            // Validate data
            if (!data.IsValid()) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid water price measurement data");
            }
            
            // Create OP_RETURN script
            CScript opReturnScript = data.ToScript();
            
            // Create and broadcast transaction
            UniValue result = CreateAndBroadcastOTransaction(opReturnScript, "WATER_PRICE", pwallet.get());
            
            // Add extra info
            result.pushKV("currency", data.currency_code);
            result.pushKV("price", data.GetPriceAsDouble());
            result.pushKV("proof_type", data.proof_type);
            result.pushKV("measurer", HexStr(data.measurer));
            
            return result;
        },
    };
}

static RPCHelpMan submitexchangeratetx()
{
    return RPCHelpMan{
        "submitexchangeratetx",
        "\nCreate an EXCHANGE_RATE blockchain transaction.\n"
        "This transaction will be validated and stored by all nodes.\n",
        {
            {"from_currency", RPCArg::Type::STR, RPCArg::Optional::NO, "From currency (e.g., OUSD)"},
            {"to_currency", RPCArg::Type::STR, RPCArg::Optional::NO, "To currency (e.g., USD)"},
            {"exchange_rate", RPCArg::Type::NUM, RPCArg::Optional::NO, "Rate * 1,000,000 (6 decimal places)"},
            {"invite_id", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Measurement invitation ID"},
            {"proof_data", RPCArg::Type::STR, RPCArg::Optional::NO, "Exchange platform URL or proof"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "txid", "Transaction ID"},
                {RPCResult::Type::STR_HEX, "tx_hex", "Transaction hex for broadcasting"},
                {RPCResult::Type::STR, "type", "Transaction type (EXCHANGE_RATE)"},
                {RPCResult::Type::STR, "from_currency", "From currency"},
                {RPCResult::Type::STR, "to_currency", "To currency"},
                {RPCResult::Type::NUM, "rate", "Exchange rate"},
            }
        },
        RPCExamples{
            HelpExampleCli("submitexchangeratetx", "\"OUSD\" \"USD\" 1500000 \"def456...\" \"https://exchange.com/OUSD-USD\"")
            + HelpExampleRpc("submitexchangeratetx", "\"OUSD\", \"USD\", 1500000, \"def456...\", \"https://exchange.com/OUSD-USD\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            // Get wallet
            std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
            if (!pwallet) throw JSONRPCError(RPC_WALLET_NOT_FOUND, "Wallet not found");
            
            LOCK(pwallet->cs_wallet);
            
            // Ensure wallet is unlocked for signing
            EnsureWalletIsUnlocked(*pwallet);
            
            // Parse parameters
            CExchangeRateMeasurementData data;
            data.from_currency = request.params[0].get_str();
            data.to_currency = request.params[1].get_str();
            data.exchange_rate = request.params[2].getInt<int64_t>();
            data.invite_id = ParseHashV(request.params[3], "invite_id");
            data.proof_data = request.params[4].get_str();
            data.timestamp = GetTime();
            
            // Get measurer pubkey from wallet
            util::Result<CTxDestination> dest_result = pwallet->GetNewDestination(OutputType::LEGACY, "measurement");
            if (!dest_result) {
                throw JSONRPCError(RPC_WALLET_ERROR, "Failed to get wallet address: " + util::ErrorString(dest_result).original);
            }
            
            CTxDestination dest = *dest_result;
            const PKHash* pkhash = std::get_if<PKHash>(&dest);
            if (!pkhash) {
                throw JSONRPCError(RPC_WALLET_ERROR, "Address is not a valid public key hash");
            }
            
            CKeyID keyid = ToKeyID(*pkhash);
            
            // Get public key
            CPubKey measurer_pubkey;
            CScript script_pubkey = GetScriptForDestination(dest);
            std::unique_ptr<SigningProvider> provider = pwallet->GetSolvingProvider(script_pubkey);
            if (!provider || !provider->GetPubKey(keyid, measurer_pubkey)) {
                throw JSONRPCError(RPC_WALLET_ERROR, "Failed to get public key from wallet");
            }
            
            data.measurer = measurer_pubkey;
            
            // Sign the measurement with measurer's private key
            CKey private_key;
            if (!provider->GetKey(keyid, private_key)) {
                throw JSONRPCError(RPC_WALLET_ERROR, "Failed to get private key for signing");
            }
            
            uint256 hash = data.GetHash();
            std::vector<unsigned char> signature_vec;
            if (!private_key.SignCompact(hash, signature_vec)) {
                throw JSONRPCError(RPC_WALLET_ERROR, "Failed to sign measurement");
            }
            
            data.signature = signature_vec;
            
            // Validate data
            if (!data.IsValid()) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid exchange rate measurement data");
            }
            
            // Create OP_RETURN script
            CScript opReturnScript = data.ToScript();
            
            // Create and broadcast transaction
            UniValue result = CreateAndBroadcastOTransaction(opReturnScript, "EXCHANGE_RATE", pwallet.get());
            
            // Add extra info
            result.pushKV("from_currency", data.from_currency);
            result.pushKV("to_currency", data.to_currency);
            result.pushKV("rate", data.GetRateAsDouble());
            result.pushKV("measurer", HexStr(data.measurer));
            
            return result;
        },
    };
}

static RPCHelpMan submitvalidationtx()
{
    return RPCHelpMan{
        "submitvalidationtx",
        "\nCreate a MEASUREMENT_VALIDATION blockchain transaction.\n"
        "This transaction will be validated and stored by all nodes.\n",
        {
            {"measurement_id", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "ID of measurement being validated"},
            {"measurement_type", RPCArg::Type::STR, RPCArg::Optional::NO, "'water_price' or 'exchange_rate'"},
            {"validation_result", RPCArg::Type::BOOL, RPCArg::Default{true}, "true = valid, false = invalid"},
            {"validation_notes", RPCArg::Type::STR, RPCArg::Default{""}, "Optional notes (e.g., why invalid)"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "txid", "Transaction ID"},
                {RPCResult::Type::STR_HEX, "tx_hex", "Transaction hex for broadcasting"},
                {RPCResult::Type::STR, "type", "Transaction type (MEASUREMENT_VALIDATION)"},
                {RPCResult::Type::STR_HEX, "measurement_id", "Measurement being validated"},
                {RPCResult::Type::BOOL, "result", "Validation result"},
            }
        },
        RPCExamples{
            HelpExampleCli("submitvalidationtx", "\"abc123...\" \"water_price\" true \"Verified URL accessible\"")
            + HelpExampleRpc("submitvalidationtx", "\"abc123...\", \"water_price\", true, \"Verified URL accessible\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            // Get wallet
            std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
            if (!pwallet) throw JSONRPCError(RPC_WALLET_NOT_FOUND, "Wallet not found");
            
            LOCK(pwallet->cs_wallet);
            
            // Ensure wallet is unlocked for signing
            EnsureWalletIsUnlocked(*pwallet);
            
            // Parse parameters
            CMeasurementValidationData data;
            data.measurement_id = ParseHashV(request.params[0], "measurement_id");
            
            std::string type_str = request.params[1].get_str();
            if (type_str == "water_price") {
                data.measurement_type = OTxType::WATER_PRICE;
            } else if (type_str == "exchange_rate") {
                data.measurement_type = OTxType::EXCHANGE_RATE;
            } else {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "measurement_type must be 'water_price' or 'exchange_rate'");
            }
            
            data.validation_result = request.params[2].isNull() ? true : request.params[2].get_bool();
            data.validation_notes = request.params[3].isNull() ? "" : request.params[3].get_str();
            data.timestamp = GetTime();
            
            // Get validator pubkey from wallet
            util::Result<CTxDestination> dest_result = pwallet->GetNewDestination(OutputType::LEGACY, "validation");
            if (!dest_result) {
                throw JSONRPCError(RPC_WALLET_ERROR, "Failed to get wallet address: " + util::ErrorString(dest_result).original);
            }
            
            CTxDestination dest = *dest_result;
            const PKHash* pkhash = std::get_if<PKHash>(&dest);
            if (!pkhash) {
                throw JSONRPCError(RPC_WALLET_ERROR, "Address is not a valid public key hash");
            }
            
            CKeyID keyid = ToKeyID(*pkhash);
            
            // Get public key
            CPubKey validator_pubkey;
            CScript script_pubkey = GetScriptForDestination(dest);
            std::unique_ptr<SigningProvider> provider = pwallet->GetSolvingProvider(script_pubkey);
            if (!provider || !provider->GetPubKey(keyid, validator_pubkey)) {
                throw JSONRPCError(RPC_WALLET_ERROR, "Failed to get public key from wallet");
            }
            
            data.validator = validator_pubkey;
            
            // Sign the validation with validator's private key
            CKey private_key;
            if (!provider->GetKey(keyid, private_key)) {
                throw JSONRPCError(RPC_WALLET_ERROR, "Failed to get private key for signing");
            }
            
            uint256 hash = data.GetHash();
            std::vector<unsigned char> signature_vec;
            if (!private_key.SignCompact(hash, signature_vec)) {
                throw JSONRPCError(RPC_WALLET_ERROR, "Failed to sign validation");
            }
            
            data.signature = signature_vec;
            
            // Validate data
            if (!data.IsValid()) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid validation data");
            }
            
            // Create OP_RETURN script
            CScript opReturnScript = data.ToScript();
            
            // Create and broadcast transaction
            UniValue result = CreateAndBroadcastOTransaction(opReturnScript, "MEASUREMENT_VALIDATION", pwallet.get());
            
            // Add extra info
            result.pushKV("measurement_id", data.measurement_id.GetHex());
            result.pushKV("result", data.validation_result);
            result.pushKV("measurement_type", type_str);
            result.pushKV("validator", HexStr(data.validator));
            
            return result;
        },
    };
}

static RPCHelpMan submitinvitetx()
{
    return RPCHelpMan{
        "submitinvitetx",
        "\nCreate a MEASUREMENT_INVITE blockchain transaction.\n"
        "This transaction will be validated and stored by all nodes.\n"
        "NOTE: Usually called by system/miner automatically.\n",
        {
            {"invite_id", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Unique invitation ID"},
            {"invited_user_pubkey", RPCArg::Type::STR, RPCArg::Optional::NO, "Public key of invited user"},
            {"measurement_type", RPCArg::Type::STR, RPCArg::Optional::NO, "'water_price' or 'exchange_rate'"},
            {"currency_code", RPCArg::Type::STR, RPCArg::Default{""}, "Optional: specific currency"},
            {"expires_at", RPCArg::Type::NUM, RPCArg::Optional::NO, "Expiration timestamp"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "txid", "Transaction ID"},
                {RPCResult::Type::STR_HEX, "tx_hex", "Transaction hex for broadcasting"},
                {RPCResult::Type::STR, "type", "Transaction type (MEASUREMENT_INVITE)"},
                {RPCResult::Type::STR_HEX, "invite_id", "Invitation ID"},
                {RPCResult::Type::STR, "invited_user", "Invited user pubkey"},
            }
        },
        RPCExamples{
            HelpExampleCli("submitinvitetx", "\"abc123...\" \"02def456...\" \"water_price\" \"USD\" 1730604800")
            + HelpExampleRpc("submitinvitetx", "\"abc123...\", \"02def456...\", \"water_price\", \"USD\", 1730604800")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            // Get wallet
            std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
            if (!pwallet) throw JSONRPCError(RPC_WALLET_NOT_FOUND, "Wallet not found");
            
            // Parse parameters
            CMeasurementInviteData data;
            data.invite_id = ParseHashV(request.params[0], "invite_id");
            
            // Parse invited user pubkey
            std::vector<unsigned char> pubkey_bytes = ParseHex(request.params[1].get_str());
            data.invited_user = CPubKey(pubkey_bytes.begin(), pubkey_bytes.end());
            
            std::string type_str = request.params[2].get_str();
            if (type_str == "water_price") {
                data.measurement_type = 0x02;
            } else if (type_str == "exchange_rate") {
                data.measurement_type = 0x03;
            } else {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "measurement_type must be 'water_price' or 'exchange_rate'");
            }
            
            data.currency_code = request.params[3].isNull() ? "" : request.params[3].get_str();
            data.expires_at = request.params[4].getInt<int64_t>();
            data.created_at = GetTime();
            data.block_height = 0; // Will be set when included in block
            
            // TODO: Sign with system/miner key
            data.signature = {};
            
            // Validate data
            if (!data.IsValid()) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid invitation data");
            }
            
            // Create OP_RETURN script
            CScript opReturnScript = data.ToScript();
            
            // Create and broadcast transaction
            UniValue result = CreateAndBroadcastOTransaction(opReturnScript, "MEASUREMENT_INVITE", pwallet.get());
            
            // Add extra info
            result.pushKV("invite_id", data.invite_id.GetHex());
            result.pushKV("invited_user", HexStr(data.invited_user));
            result.pushKV("measurement_type", type_str);
            result.pushKV("currency", data.currency_code);
            
            return result;
        },
    };
}

// ===== RPC Command Registration =====

void RegisterOBlockchainTxRPCCommands(CRPCTable& t)
{
    static const CRPCCommand commands[] = {
        // O Blockchain Transaction Creation
        {"blockchain", &submituserverificationtx},
        {"blockchain", &submitwaterpricetx},
        {"blockchain", &submitexchangeratetx},
        {"blockchain", &submitvalidationtx},
        {"blockchain", &submitinvitetx},
    };
    
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}

