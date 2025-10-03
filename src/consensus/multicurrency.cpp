// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/multicurrency.h>
#include <util/strencodings.h>

CurrencyRegistry::CurrencyRegistry() {
    InitializeDefaultCurrencies();
}

bool CurrencyRegistry::RegisterCurrency(const CurrencyMetadata& metadata) {
    if (metadata.id >= MAX_CURRENCIES) {
        return false;
    }
    
    if (currencies.find(metadata.id) != currencies.end()) {
        return false; // Currency ID already exists
    }
    
    if (symbol_to_id.find(metadata.symbol) != symbol_to_id.end()) {
        return false; // Symbol already exists
    }
    
    currencies[metadata.id] = metadata;
    symbol_to_id[metadata.symbol] = metadata.id;
    return true;
}

std::optional<CurrencyMetadata> CurrencyRegistry::GetCurrency(CurrencyId id) const {
    auto it = currencies.find(id);
    if (it != currencies.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<CurrencyId> CurrencyRegistry::GetCurrencyId(const std::string& symbol) const {
    auto it = symbol_to_id.find(symbol);
    if (it != symbol_to_id.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool CurrencyRegistry::IsSupported(CurrencyId id) const {
    return currencies.find(id) != currencies.end();
}

std::vector<CurrencyMetadata> CurrencyRegistry::GetAllCurrencies() const {
    std::vector<CurrencyMetadata> result;
    result.reserve(currencies.size());
    for (const auto& pair : currencies) {
        result.push_back(pair.second);
    }
    return result;
}

void CurrencyRegistry::InitializeDefaultCurrencies() {
    // Register Bitcoin as the base currency
    RegisterCurrency(CurrencyMetadata(
        CURRENCY_BTC, "BTC", "Bitcoin", 8, false, ""
    ));
    
    // O Blockchain Stable Coins (prefixed with O to distinguish from fiat)
    // Major Reserve Currencies
    RegisterCurrency(CurrencyMetadata(CURRENCY_USD, "OUSD", "O US Dollar (Water-based)", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_EUR, "OEUR", "O Euro (Water-based)", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_JPY, "OJPY", "O Japanese Yen (Water-based)", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_GBP, "OGBP", "O British Pound (Water-based)", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_CNY, "OCNY", "O Chinese Yuan (Water-based)", 2, true, ""));
    
    // G7/G20 Major Currencies
    RegisterCurrency(CurrencyMetadata(CURRENCY_CAD, "OCAD", "O Canadian Dollar", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_AUD, "OAUD", "O Australian Dollar", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_CHF, "OCHF", "O Swiss Franc", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_NZD, "ONZD", "O New Zealand Dollar", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_SEK, "OSEK", "O Swedish Krona", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_NOK, "ONOK", "O Norwegian Krone", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_DKK, "ODKK", "O Danish Krone", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_PLN, "OPLN", "O Polish Zloty", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_CZK, "OCZK", "O Czech Koruna", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_HUF, "OHUF", "O Hungarian Forint", 2, true, ""));
    
    // Asian Major Currencies
    RegisterCurrency(CurrencyMetadata(CURRENCY_KRW, "OKRW", "O South Korean Won", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_SGD, "OSGD", "O Singapore Dollar", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_HKD, "OHKD", "O Hong Kong Dollar", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_TWD, "OTWD", "O Taiwan Dollar", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_THB, "OTHB", "O Thai Baht", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_MYR, "OMYR", "O Malaysian Ringgit", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_IDR, "OIDR", "O Indonesian Rupiah", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_PHP, "OPHP", "O Philippine Peso", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_VND, "OVND", "O Vietnamese Dong", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_INR, "OINR", "O Indian Rupee", 2, true, ""));
    
    // Middle East & Africa
    RegisterCurrency(CurrencyMetadata(CURRENCY_AED, "OAED", "O UAE Dirham", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_SAR, "OSAR", "O Saudi Riyal", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_QAR, "OQAR", "O Qatari Riyal", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_KWD, "OKWD", "O Kuwaiti Dinar", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_BHD, "OBHD", "O Bahraini Dinar", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_OMR, "OOMR", "O Omani Rial", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_JOD, "OJOD", "O Jordanian Dinar", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_ILS, "OILS", "O Israeli Shekel", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_TRY, "OTRY", "O Turkish Lira", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_EGP, "OEGP", "O Egyptian Pound", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_ZAR, "OZAR", "O South African Rand", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_NGN, "ONGN", "O Nigerian Naira", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_KES, "OKES", "O Kenyan Shilling", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_ETB, "OETB", "O Ethiopian Birr", 2, true, ""));
    
    // Americas
    RegisterCurrency(CurrencyMetadata(CURRENCY_MXN, "OMXN", "O Mexican Peso", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_BRL, "OBRL", "O Brazilian Real", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_ARS, "OARS", "O Argentine Peso", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_CLP, "OCLP", "O Chilean Peso", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_COP, "OCOP", "O Colombian Peso", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_PEN, "OPEN", "O Peruvian Sol", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_UYU, "OUYU", "O Uruguayan Peso", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_VES, "OVES", "O Venezuelan Bolivar", 2, true, ""));
    
    // European Union & Others
    RegisterCurrency(CurrencyMetadata(CURRENCY_RON, "ORON", "O Romanian Leu", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_BGN, "OBGN", "O Bulgarian Lev", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_HRK, "OHRK", "O Croatian Kuna", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_RUB, "ORUB", "O Russian Ruble", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_UAH, "OUAH", "O Ukrainian Hryvnia", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_BYN, "OBYN", "O Belarusian Ruble", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_KZT, "OKZT", "O Kazakhstani Tenge", 2, true, ""));
    
    // Commonwealth & Others
    RegisterCurrency(CurrencyMetadata(CURRENCY_ISK, "OISK", "O Icelandic Krona", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_LKR, "OLKR", "O Sri Lankan Rupee", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_BDT, "OBDT", "O Bangladeshi Taka", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_PKR, "OPKR", "O Pakistani Rupee", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_AFN, "OAFN", "O Afghan Afghani", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_IQD, "OIQD", "O Iraqi Dinar", 3, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_IRR, "OIRR", "O Iranian Rial", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_LBP, "OLBP", "O Lebanese Pound", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_SYP, "OSYP", "O Syrian Pound", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_YER, "OYER", "O Yemeni Rial", 2, true, ""));
    
    // Note: Water prices are measured using the existing fiat currencies above
    // No separate water price currencies needed - water prices are measured in USD, EUR, JPY, etc.
}

// Global currency registry instance
CurrencyRegistry g_currency_registry;

