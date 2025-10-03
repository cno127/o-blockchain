// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PRIMITIVES_MULTICURRENCY_TXOUT_H
#define BITCOIN_PRIMITIVES_MULTICURRENCY_TXOUT_H

#include <consensus/multicurrency.h>
#include <primitives/transaction.h>
#include <serialize.h>
#include <script/script.h>
#include <streams.h>
#include <protocol.h>

/** Multi-currency transaction output */
class CMultiCurrencyTxOut {
public:
    CScript scriptPubKey;
    std::vector<MultiCurrencyAmount> amounts;
    
    CMultiCurrencyTxOut() {}
    CMultiCurrencyTxOut(const CScript& scriptPubKeyIn, const std::vector<MultiCurrencyAmount>& amountsIn)
        : scriptPubKey(scriptPubKeyIn), amounts(amountsIn) {}
    
    /** Convert from legacy CTxOut (BTC only) */
    CMultiCurrencyTxOut(const CTxOut& txout)
        : scriptPubKey(txout.scriptPubKey) {
        amounts.emplace_back(CURRENCY_BTC, txout.nValue);
    }
    
    /** Convert to legacy CTxOut (BTC amount only) */
    CTxOut ToLegacyTxOut() const {
        CAmount btc_amount = 0;
        for (const auto& amount : amounts) {
            if (amount.currency_id == CURRENCY_BTC) {
                btc_amount = amount.amount;
                break;
            }
        }
        return CTxOut(btc_amount, scriptPubKey);
    }
    
    /** Get amount for specific currency */
    CAmount GetAmount(CurrencyId currency_id) const {
        for (const auto& amount : amounts) {
            if (amount.currency_id == currency_id) {
                return amount.amount;
            }
        }
        return 0;
    }
    
    /** Set amount for specific currency */
    void SetAmount(CurrencyId currency_id, CAmount value) {
        for (auto& amount : amounts) {
            if (amount.currency_id == currency_id) {
                amount.amount = value;
                return;
            }
        }
        // Currency not found, add new amount
        amounts.emplace_back(currency_id, value);
    }
    
    /** Check if output has any amount */
    bool IsNull() const {
        return amounts.empty() || (amounts.size() == 1 && amounts[0].amount == 0);
    }
    
    /** Get total number of currencies in this output */
    size_t GetCurrencyCount() const {
        return amounts.size();
    }
    
    /** Check if output contains specific currency */
    bool HasCurrency(CurrencyId currency_id) const {
        for (const auto& amount : amounts) {
            if (amount.currency_id == currency_id) {
                return true;
            }
        }
        return false;
    }
    
    /** Remove currency from output */
    void RemoveCurrency(CurrencyId currency_id) {
        amounts.erase(
            std::remove_if(amounts.begin(), amounts.end(),
                [currency_id](const MultiCurrencyAmount& amount) {
                    return amount.currency_id == currency_id;
                }),
            amounts.end()
        );
    }
    
    /** Serialization */
    SERIALIZE_METHODS(CMultiCurrencyTxOut, obj) {
        READWRITE(obj.scriptPubKey, obj.amounts);
    }
    
    /** Get size for fee calculation */
    size_t GetSerializeSize() const {
        DataStream s{};
        s << *this;
        return s.size();
    }
};

/** Multi-currency transaction input */
class CMultiCurrencyTxIn {
public:
    COutPoint prevout;
    CScript scriptSig;
    uint32_t nSequence;
    CScriptWitness scriptWitness;
    
    CMultiCurrencyTxIn() : nSequence(CTxIn::SEQUENCE_FINAL) {}
    CMultiCurrencyTxIn(const COutPoint& prevoutIn, const CScript& scriptSigIn, uint32_t nSequenceIn)
        : prevout(prevoutIn), scriptSig(scriptSigIn), nSequence(nSequenceIn) {}
    
    /** Convert from legacy CTxIn */
    CMultiCurrencyTxIn(const CTxIn& txin)
        : prevout(txin.prevout), scriptSig(txin.scriptSig), nSequence(txin.nSequence), scriptWitness(txin.scriptWitness) {}
    
    /** Convert to legacy CTxIn */
    CTxIn ToLegacyTxIn() const {
        CTxIn txin(prevout, scriptSig, nSequence);
        txin.scriptWitness = scriptWitness;
        return txin;
    }
    
    /** Serialization */
    SERIALIZE_METHODS(CMultiCurrencyTxIn, obj) {
        READWRITE(obj.prevout, obj.scriptSig, obj.nSequence, obj.scriptWitness);
    }
};

/** Multi-currency transaction */
class CMultiCurrencyTransaction {
public:
    int32_t nVersion;
    std::vector<CMultiCurrencyTxIn> vin;
    std::vector<CMultiCurrencyTxOut> vout;
    uint32_t nLockTime;
    
    CMultiCurrencyTransaction() : nVersion(CTransaction::CURRENT_VERSION), nLockTime(0) {}
    
    /** Convert from legacy CTransaction */
    CMultiCurrencyTransaction(const CTransaction& tx)
        : nVersion(tx.version), nLockTime(tx.nLockTime) {
        vin.reserve(tx.vin.size());
        vout.reserve(tx.vout.size());
        
        for (const auto& input : tx.vin) {
            vin.emplace_back(input);
        }
        
        for (const auto& output : tx.vout) {
            vout.emplace_back(output);
        }
    }
    
    /** Convert to legacy CTransaction */
    CTransaction ToLegacyTransaction() const {
        CMutableTransaction tx;
        tx.version = nVersion;
        tx.nLockTime = nLockTime;
        
        tx.vin.reserve(vin.size());
        tx.vout.reserve(vout.size());
        
        for (const auto& input : vin) {
            tx.vin.push_back(input.ToLegacyTxIn());
        }
        
        for (const auto& output : vout) {
            tx.vout.push_back(output.ToLegacyTxOut());
        }
        
        return CTransaction(tx);
    }
    
    /** Get transaction hash */
    uint256 GetHash() const {
        return ToLegacyTransaction().GetHash();
    }
    
    /** Get witness hash */
    uint256 GetWitnessHash() const {
        return ToLegacyTransaction().GetWitnessHash();
    }
    
    /** Serialization */
    SERIALIZE_METHODS(CMultiCurrencyTransaction, obj) {
        READWRITE(obj.nVersion, obj.vin, obj.vout, obj.nLockTime);
    }
    
    /** Get total size */
    size_t GetSerializeSize() const {
        DataStream s{};
        s << *this;
        return s.size();
    }
    
    /** Get virtual size for fee calculation */
    size_t GetVirtualSize() const {
        return GetSerializeSize();
    }
};

#endif // BITCOIN_PRIMITIVES_MULTICURRENCY_TXOUT_H

