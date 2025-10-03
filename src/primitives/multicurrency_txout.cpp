// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/multicurrency_txout.h>
#include <consensus/multicurrency.h>
#include <serialize.h>
#include <streams.h>

#include <algorithm>

// Conversion functions
CMultiCurrencyTransaction CMultiCurrencyTransaction::FromLegacyTransaction(const CTransaction& tx) {
    CMultiCurrencyTransaction multi_tx;
    multi_tx.nVersion = tx.version;
    multi_tx.nLockTime = tx.nLockTime;
    
    // Convert inputs
    for (const CTxIn& input : tx.vin) {
        CMultiCurrencyTxIn multi_input;
        multi_input.prevout = input.prevout;
        multi_input.scriptSig = input.scriptSig;
        multi_input.nSequence = input.nSequence;
        multi_tx.vin.push_back(multi_input);
    }
    
    // Convert outputs
    for (const CTxOut& output : tx.vout) {
        CMultiCurrencyTxOut multi_output;
        multi_output.nValue = output.nValue;
        multi_output.scriptPubKey = output.scriptPubKey;
        
        // Create default BTC amount
        std::vector<MultiCurrencyAmount> amounts;
        amounts.emplace_back(CURRENCY_BTC, output.nValue);
        multi_output.amounts = amounts;
        
        multi_tx.vout.push_back(multi_output);
    }
    
    return multi_tx;
}

CTransaction CMultiCurrencyTransaction::ToLegacyTransaction() const {
    CMutableTransaction tx;
    tx.version = nVersion;
    tx.nLockTime = nLockTime;
    
    // Convert inputs
    for (const CMultiCurrencyTxIn& input : vin) {
        CTxIn legacy_input;
        legacy_input.prevout = input.prevout;
        legacy_input.scriptSig = input.scriptSig;
        legacy_input.nSequence = input.nSequence;
        tx.vin.push_back(legacy_input);
    }
    
    // Convert outputs
    for (const CMultiCurrencyTxOut& output : vout) {
        CTxOut legacy_output;
        legacy_output.nValue = output.nValue;
        legacy_output.scriptPubKey = output.scriptPubKey;
        tx.vout.push_back(legacy_output);
    }
    
    return CTransaction(tx);
}

// Utility functions
CAmount CMultiCurrencyTxOut::GetTotalValue() const {
    CAmount total = 0;
    for (const MultiCurrencyAmount& amount : amounts) {
        if (amount.currency_id == CURRENCY_BTC) {
            total += amount.amount;
        }
    }
    return total;
}

bool CMultiCurrencyTxOut::HasCurrency(CurrencyId currency_id) const {
    return std::any_of(amounts.begin(), amounts.end(),
                      [currency_id](const MultiCurrencyAmount& amount) {
                          return amount.currency_id == currency_id;
                      });
}

CAmount CMultiCurrencyTxOut::GetCurrencyAmount(CurrencyId currency_id) const {
    auto it = std::find_if(amounts.begin(), amounts.end(),
                          [currency_id](const MultiCurrencyAmount& amount) {
                              return amount.currency_id == currency_id;
                          });
    return it != amounts.end() ? it->amount : 0;
}