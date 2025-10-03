// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_MULTICURRENCY_H
#define BITCOIN_CONSENSUS_MULTICURRENCY_H

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <serialize.h>

/** Currency identifier for multi-currency support */
typedef uint32_t CurrencyId;

/** Special currency IDs */
static constexpr CurrencyId CURRENCY_BTC = 0;  // Bitcoin (base currency)

// Major Reserve Currencies
static constexpr CurrencyId CURRENCY_USD = 1;  // US Dollar
static constexpr CurrencyId CURRENCY_EUR = 2;  // Euro
static constexpr CurrencyId CURRENCY_JPY = 3;  // Japanese Yen
static constexpr CurrencyId CURRENCY_GBP = 4;  // British Pound Sterling
static constexpr CurrencyId CURRENCY_CNY = 5;  // Chinese Yuan Renminbi

// G7/G20 Major Currencies
static constexpr CurrencyId CURRENCY_CAD = 6;  // Canadian Dollar
static constexpr CurrencyId CURRENCY_AUD = 7;  // Australian Dollar
static constexpr CurrencyId CURRENCY_CHF = 8;  // Swiss Franc
static constexpr CurrencyId CURRENCY_NZD = 9;  // New Zealand Dollar
static constexpr CurrencyId CURRENCY_SEK = 10; // Swedish Krona
static constexpr CurrencyId CURRENCY_NOK = 11; // Norwegian Krone
static constexpr CurrencyId CURRENCY_DKK = 12; // Danish Krone
static constexpr CurrencyId CURRENCY_PLN = 13; // Polish Zloty
static constexpr CurrencyId CURRENCY_CZK = 14; // Czech Koruna
static constexpr CurrencyId CURRENCY_HUF = 15; // Hungarian Forint

// Asian Major Currencies
static constexpr CurrencyId CURRENCY_KRW = 16; // South Korean Won
static constexpr CurrencyId CURRENCY_SGD = 17; // Singapore Dollar
static constexpr CurrencyId CURRENCY_HKD = 18; // Hong Kong Dollar
static constexpr CurrencyId CURRENCY_TWD = 19; // Taiwan Dollar
static constexpr CurrencyId CURRENCY_THB = 20; // Thai Baht
static constexpr CurrencyId CURRENCY_MYR = 21; // Malaysian Ringgit
static constexpr CurrencyId CURRENCY_IDR = 22; // Indonesian Rupiah
static constexpr CurrencyId CURRENCY_PHP = 23; // Philippine Peso
static constexpr CurrencyId CURRENCY_VND = 24; // Vietnamese Dong
static constexpr CurrencyId CURRENCY_INR = 25; // Indian Rupee

// Middle East & Africa
static constexpr CurrencyId CURRENCY_AED = 26; // UAE Dirham
static constexpr CurrencyId CURRENCY_SAR = 27; // Saudi Riyal
static constexpr CurrencyId CURRENCY_QAR = 28; // Qatari Riyal
static constexpr CurrencyId CURRENCY_KWD = 29; // Kuwaiti Dinar
static constexpr CurrencyId CURRENCY_BHD = 30; // Bahraini Dinar
static constexpr CurrencyId CURRENCY_OMR = 31; // Omani Rial
static constexpr CurrencyId CURRENCY_JOD = 32; // Jordanian Dinar
static constexpr CurrencyId CURRENCY_ILS = 33; // Israeli Shekel
static constexpr CurrencyId CURRENCY_TRY = 34; // Turkish Lira
static constexpr CurrencyId CURRENCY_EGP = 35; // Egyptian Pound
static constexpr CurrencyId CURRENCY_ZAR = 36; // South African Rand
static constexpr CurrencyId CURRENCY_NGN = 37; // Nigerian Naira
static constexpr CurrencyId CURRENCY_KES = 38; // Kenyan Shilling
static constexpr CurrencyId CURRENCY_ETB = 39; // Ethiopian Birr

// Americas
static constexpr CurrencyId CURRENCY_MXN = 40; // Mexican Peso
static constexpr CurrencyId CURRENCY_BRL = 41; // Brazilian Real
static constexpr CurrencyId CURRENCY_ARS = 42; // Argentine Peso
static constexpr CurrencyId CURRENCY_CLP = 43; // Chilean Peso
static constexpr CurrencyId CURRENCY_COP = 44; // Colombian Peso
static constexpr CurrencyId CURRENCY_PEN = 45; // Peruvian Sol
static constexpr CurrencyId CURRENCY_UYU = 46; // Uruguayan Peso
static constexpr CurrencyId CURRENCY_VES = 47; // Venezuelan Bolivar

// European Union & Others
static constexpr CurrencyId CURRENCY_RON = 48; // Romanian Leu
static constexpr CurrencyId CURRENCY_BGN = 49; // Bulgarian Lev
static constexpr CurrencyId CURRENCY_HRK = 50; // Croatian Kuna
static constexpr CurrencyId CURRENCY_RUB = 51; // Russian Ruble
static constexpr CurrencyId CURRENCY_UAH = 52; // Ukrainian Hryvnia
static constexpr CurrencyId CURRENCY_BYN = 53; // Belarusian Ruble
static constexpr CurrencyId CURRENCY_KZT = 54; // Kazakhstani Tenge

// Commonwealth & Others
static constexpr CurrencyId CURRENCY_ISK = 55; // Icelandic Krona
static constexpr CurrencyId CURRENCY_LKR = 56; // Sri Lankan Rupee
static constexpr CurrencyId CURRENCY_BDT = 57; // Bangladeshi Taka
static constexpr CurrencyId CURRENCY_PKR = 58; // Pakistani Rupee
static constexpr CurrencyId CURRENCY_AFN = 59; // Afghan Afghani
static constexpr CurrencyId CURRENCY_IQD = 60; // Iraqi Dinar
static constexpr CurrencyId CURRENCY_IRR = 61; // Iranian Rial
static constexpr CurrencyId CURRENCY_LBP = 62; // Lebanese Pound
static constexpr CurrencyId CURRENCY_SYP = 63; // Syrian Pound
static constexpr CurrencyId CURRENCY_YER = 64; // Yemeni Rial

// Note: Water prices are measured using the existing fiat currencies above
// No separate water price currencies needed - water prices are measured in USD, EUR, JPY, etc.

/** Maximum number of supported currencies */
static constexpr CurrencyId MAX_CURRENCIES = 1000;

/** Currency metadata structure */
struct CurrencyMetadata {
    CurrencyId id;
    std::string symbol;           // 3-letter currency code (USD, EUR, etc.)
    std::string name;             // Full currency name
    uint8_t decimals;             // Number of decimal places
    bool is_fiat;                 // Whether this is a fiat currency
    std::string issuer_address;   // Address of the currency issuer (for fiat)
    
    CurrencyMetadata() : id(0), decimals(8), is_fiat(false) {}
    
    CurrencyMetadata(CurrencyId id_in, const std::string& symbol_in, 
                    const std::string& name_in, uint8_t decimals_in, 
                    bool is_fiat_in, const std::string& issuer_in)
        : id(id_in), symbol(symbol_in), name(name_in), decimals(decimals_in), 
          is_fiat(is_fiat_in), issuer_address(issuer_in) {}
    
    SERIALIZE_METHODS(CurrencyMetadata, obj) {
        READWRITE(obj.id, obj.symbol, obj.name, obj.decimals, obj.is_fiat, obj.issuer_address);
    }
};

/** Multi-currency amount structure */
struct MultiCurrencyAmount {
    CurrencyId currency_id;
    int64_t amount;  // Amount in smallest currency unit (like satoshis)
    
    MultiCurrencyAmount() : currency_id(CURRENCY_BTC), amount(0) {}
    MultiCurrencyAmount(CurrencyId currency, int64_t amount_in) 
        : currency_id(currency), amount(amount_in) {}
    
    bool operator==(const MultiCurrencyAmount& other) const {
        return currency_id == other.currency_id && amount == other.amount;
    }
    
    bool operator<(const MultiCurrencyAmount& other) const {
        if (currency_id != other.currency_id) return currency_id < other.currency_id;
        return amount < other.amount;
    }
    
    SERIALIZE_METHODS(MultiCurrencyAmount, obj) {
        READWRITE(obj.currency_id, obj.amount);
    }
};

/** Currency registry for managing supported currencies */
class CurrencyRegistry {
private:
    std::map<CurrencyId, CurrencyMetadata> currencies;
    std::map<std::string, CurrencyId> symbol_to_id;
    
public:
    CurrencyRegistry();
    
    /** Register a new currency */
    bool RegisterCurrency(const CurrencyMetadata& metadata);
    
    /** Get currency metadata by ID */
    std::optional<CurrencyMetadata> GetCurrency(CurrencyId id) const;
    
    /** Get currency ID by symbol */
    std::optional<CurrencyId> GetCurrencyId(const std::string& symbol) const;
    
    /** Check if currency is supported */
    bool IsSupported(CurrencyId id) const;
    
    /** Get all registered currencies */
    std::vector<CurrencyMetadata> GetAllCurrencies() const;
    
    /** Initialize with default fiat currencies */
    void InitializeDefaultCurrencies();
};

/** Global currency registry instance */
extern CurrencyRegistry g_currency_registry;

#endif // BITCOIN_CONSENSUS_MULTICURRENCY_H

