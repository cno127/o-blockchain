// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_VALIDATION_MULTICURRENCY_VALIDATION_H
#define BITCOIN_VALIDATION_MULTICURRENCY_VALIDATION_H

#include <consensus/multicurrency.h>
#include <primitives/multicurrency_txout.h>
#include <coins.h>
#include <validation.h>
#include <script/script.h>
#include <script/standard.h>

#include <map>
#include <vector>
#include <optional>

/** Multi-currency coin structure */
class CMultiCurrencyCoin {
public:
    CMultiCurrencyTxOut out;
    unsigned int fCoinBase : 1;
    uint32_t nHeight : 31;
    
    CMultiCurrencyCoin() : fCoinBase(false), nHeight(0) {}
    CMultiCurrencyCoin(CMultiCurrencyTxOut&& outIn, int nHeightIn, bool fCoinBaseIn)
        : out(std::move(outIn)), fCoinBase(fCoinBaseIn), nHeight(nHeightIn) {}
    
    /** Convert from legacy Coin */
    CMultiCurrencyCoin(const Coin& coin)
        : out(coin.out), fCoinBase(coin.IsCoinBase()), nHeight(coin.nHeight) {}
    
    /** Convert to legacy Coin */
    Coin ToLegacyCoin() const {
        return Coin(out.ToLegacyTxOut(), nHeight, fCoinBase);
    }
    
    bool IsCoinBase() const { return fCoinBase; }
    bool IsSpent() const { return out.IsNull(); }
    
    void Clear() {
        out = CMultiCurrencyTxOut();
        fCoinBase = false;
        nHeight = 0;
    }
    
    SERIALIZE_METHODS(CMultiCurrencyCoin, obj) {
        uint32_t code = obj.nHeight * uint32_t{2} + obj.fCoinBase;
        READWRITE(VARINT(code));
        READWRITE(obj.out);
        obj.nHeight = code >> 1;
        obj.fCoinBase = code & 1;
    }
};

/** Multi-currency coins view interface */
class CMultiCurrencyCoinsView {
public:
    virtual ~CMultiCurrencyCoinsView() = default;
    
    /** Retrieve a coin */
    virtual std::optional<CMultiCurrencyCoin> GetCoin(const COutPoint& outpoint) const = 0;
    
    /** Check if we have a coin */
    virtual bool HaveCoin(const COutPoint& outpoint) const = 0;
    
    /** Get the block hash whose state this represents */
    virtual uint256 GetBestBlock() const = 0;
    
    /** Get the block hashes that represent the current state */
    virtual std::vector<uint256> GetHeadBlocks() const = 0;
    
    /** Batch write operations */
    virtual bool BatchWrite(CCoinsMap& mapCoins, const uint256& hashBlock) = 0;
    
    /** Get a cursor to iterate over the whole state */
    virtual std::unique_ptr<CCoinsViewCursor> Cursor() const = 0;
    
    /** Estimate memory usage */
    virtual size_t EstimateSize() const = 0;
};

/** Multi-currency validation context */
struct MultiCurrencyValidationContext {
    std::map<CurrencyId, CAmount> input_amounts;
    std::map<CurrencyId, CAmount> output_amounts;
    std::map<CurrencyId, CAmount> fees;
    bool is_valid;
    std::string error_message;
    
    MultiCurrencyValidationContext() : is_valid(true) {}
    
    /** Check if transaction is balanced for all currencies */
    bool IsBalanced() const {
        for (const auto& input_pair : input_amounts) {
            CurrencyId currency = input_pair.first;
            CAmount input_total = input_pair.second;
            CAmount output_total = GetOutputAmount(currency);
            CAmount fee = GetFee(currency);
            
            if (input_total != output_total + fee) {
                return false;
            }
        }
        return true;
    }
    
    /** Get total output amount for currency */
    CAmount GetOutputAmount(CurrencyId currency) const {
        auto it = output_amounts.find(currency);
        return (it != output_amounts.end()) ? it->second : 0;
    }
    
    /** Get fee for currency */
    CAmount GetFee(CurrencyId currency) const {
        auto it = fees.find(currency);
        return (it != fees.end()) ? it->second : 0;
    }
    
    /** Add input amount */
    void AddInput(CurrencyId currency, CAmount amount) {
        input_amounts[currency] += amount;
    }
    
    /** Add output amount */
    void AddOutput(CurrencyId currency, CAmount amount) {
        output_amounts[currency] += amount;
    }
    
    /** Set fee for currency */
    void SetFee(CurrencyId currency, CAmount fee) {
        fees[currency] = fee;
    }
    
    /** Mark as invalid with error message */
    void SetInvalid(const std::string& error) {
        is_valid = false;
        error_message = error;
    }
};

/** Multi-currency transaction validator */
class CMultiCurrencyValidator {
private:
    const CMultiCurrencyCoinsView& coins_view;
    
public:
    CMultiCurrencyValidator(const CMultiCurrencyCoinsView& view) : coins_view(view) {}
    
    /** Validate a multi-currency transaction */
    MultiCurrencyValidationContext ValidateTransaction(
        const CMultiCurrencyTransaction& tx,
        const std::vector<CMultiCurrencyCoin>& spent_coins,
        CAmount fee_rate = 0
    ) const;
    
    /** Check if transaction inputs are valid */
    bool ValidateInputs(
        const CMultiCurrencyTransaction& tx,
        const std::vector<CMultiCurrencyCoin>& spent_coins,
        MultiCurrencyValidationContext& context
    ) const;
    
    /** Check if transaction outputs are valid */
    bool ValidateOutputs(
        const CMultiCurrencyTransaction& tx,
        MultiCurrencyValidationContext& context
    ) const;
    
    /** Check if transaction is balanced */
    bool ValidateBalance(const MultiCurrencyValidationContext& context) const;
    
    /** Calculate fees for multi-currency transaction */
    std::map<CurrencyId, CAmount> CalculateFees(
        const CMultiCurrencyTransaction& tx,
        CAmount fee_rate
    ) const;
    
    /** Check for double spending within the same transaction */
    bool CheckDoubleSpending(
        const CMultiCurrencyTransaction& tx,
        const std::vector<CMultiCurrencyCoin>& spent_coins
    ) const;
    
    /** Validate currency amounts */
    bool ValidateCurrencyAmounts(const CMultiCurrencyTransaction& tx) const;
    
    /** Check if all currencies in transaction are supported */
    bool ValidateSupportedCurrencies(const CMultiCurrencyTransaction& tx) const;
};

/** Helper functions for multi-currency validation */
namespace MultiCurrencyValidation {
    /** Check if a transaction output is valid */
    bool IsValidOutput(const CMultiCurrencyTxOut& output);
    
    /** Check if currency amount is within valid range */
    bool IsValidAmount(CurrencyId currency_id, CAmount amount);
    
    /** Get maximum amount for a currency */
    CAmount GetMaxAmount(CurrencyId currency_id);
    
    /** Format currency amount for display */
    std::string FormatAmount(CurrencyId currency_id, CAmount amount);
    
    /** Parse currency amount from string */
    std::optional<CAmount> ParseAmount(CurrencyId currency_id, const std::string& str);
}

#endif // BITCOIN_VALIDATION_MULTICURRENCY_VALIDATION_H

