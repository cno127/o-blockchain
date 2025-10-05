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
    
    // Additional African Currencies
    RegisterCurrency(CurrencyMetadata(CURRENCY_MAD, "OMAD", "O Moroccan Dirham", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_DZD, "ODZD", "O Algerian Dinar", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_TND, "OTND", "O Tunisian Dinar", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_LYD, "OLYD", "O Libyan Dinar", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_GHS, "OGHS", "O Ghanaian Cedi", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_XOF, "OXOF", "O West African CFA Franc", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_XAF, "OXAF", "O Central African CFA Franc", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_UGX, "OUGX", "O Ugandan Shilling", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_TZS, "OTZS", "O Tanzanian Shilling", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_RWF, "ORWF", "O Rwandan Franc", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_BIF, "OBIF", "O Burundian Franc", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_ZMW, "OZMW", "O Zambian Kwacha", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_BWP, "OBWP", "O Botswana Pula", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_NAD, "ONAD", "O Namibian Dollar", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_SZL, "OSZL", "O Swazi Lilangeni", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_LSL, "OLSL", "O Lesotho Loti", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_MUR, "OMUR", "O Mauritian Rupee", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_SCR, "OSCR", "O Seychellois Rupee", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_MGA, "OMGA", "O Malagasy Ariary", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_AOA, "OAOA", "O Angolan Kwanza", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_MZN, "OMZN", "O Mozambican Metical", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_ZWL, "OZWL", "O Zimbabwean Dollar", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_SDG, "OSDG", "O Sudanese Pound", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_SSP, "OSSP", "O South Sudanese Pound", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_SOS, "OSOS", "O Somali Shilling", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_DJF, "ODJF", "O Djiboutian Franc", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_ERN, "OERN", "O Eritrean Nakfa", 2, true, ""));
    
    // Additional Asian & Pacific Currencies
    RegisterCurrency(CurrencyMetadata(CURRENCY_MMK, "OMMK", "O Myanmar Kyat", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_KHR, "OKHR", "O Cambodian Riel", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_LAK, "OLAK", "O Lao Kip", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_BND, "OBND", "O Brunei Dollar", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_NPR, "ONPR", "O Nepalese Rupee", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_BTN, "OBTN", "O Bhutanese Ngultrum", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_MVR, "OMVR", "O Maldivian Rufiyaa", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_MNT, "OMNT", "O Mongolian Tugrik", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_KGS, "OKGS", "O Kyrgyzstani Som", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_TJS, "OTJS", "O Tajikistani Somoni", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_TMT, "OTMT", "O Turkmenistani Manat", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_UZS, "OUZS", "O Uzbekistani Som", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_FJD, "OFJD", "O Fijian Dollar", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_PGK, "OPGK", "O Papua New Guinean Kina", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_WST, "OWST", "O Samoan Tala", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_TOP, "OTOP", "O Tongan Paʻanga", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_VUV, "OVUV", "O Vanuatu Vatu", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_SBD, "OSBD", "O Solomon Islands Dollar", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_XPF, "OXPF", "O CFP Franc", 2, true, ""));
    
    // Additional European Currencies
    RegisterCurrency(CurrencyMetadata(CURRENCY_RSD, "ORSD", "O Serbian Dinar", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_MKD, "OMKD", "O Macedonian Denar", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_ALL, "OALL", "O Albanian Lek", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_BAM, "OBAM", "O Bosnia-Herzegovina Mark", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_MDL, "OMDL", "O Moldovan Leu", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_GEL, "OGEL", "O Georgian Lari", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_AMD, "OAMD", "O Armenian Dram", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_AZN, "OAZN", "O Azerbaijani Manat", 2, true, ""));
    
    // Additional Americas Currencies
    RegisterCurrency(CurrencyMetadata(CURRENCY_GTQ, "OGTQ", "O Guatemalan Quetzal", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_HNL, "OHNL", "O Honduran Lempira", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_NIO, "ONIO", "O Nicaraguan Córdoba", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_CRC, "OCRC", "O Costa Rican Colón", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_PAB, "OPAB", "O Panamanian Balboa", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_DOP, "ODOP", "O Dominican Peso", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_HTG, "OHTG", "O Haitian Gourde", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_JMD, "OJMD", "O Jamaican Dollar", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_TTD, "OTTD", "O Trinidad & Tobago Dollar", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_BBD, "OBBD", "O Barbadian Dollar", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_XCD, "OXCD", "O East Caribbean Dollar", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_BOB, "OBOB", "O Bolivian Boliviano", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_PYG, "OPYG", "O Paraguayan Guarani", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_GYD, "OGYD", "O Guyanese Dollar", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_SRD, "OSRD", "O Surinamese Dollar", 2, true, ""));
    
    // Additional African Currencies (Continued)
    RegisterCurrency(CurrencyMetadata(CURRENCY_GNF, "OGNF", "O Guinean Franc", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_LRD, "OLRD", "O Liberian Dollar", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_SLL, "OSLL", "O Sierra Leonean Leone", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_GMD, "OGMD", "O Gambian Dalasi", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_CVE, "OCVE", "O Cape Verdean Escudo", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_STN, "OSTN", "O São Tomé Dobra", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_CDF, "OCDF", "O Congolese Franc", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_MWK, "OMWK", "O Malawian Kwacha", 2, true, ""));
    RegisterCurrency(CurrencyMetadata(CURRENCY_KMF, "OKMF", "O Comorian Franc", 2, true, ""));
    
    // Note: Water prices are measured using the existing fiat currencies above
    // No separate water price currencies needed - water prices are measured in USD, EUR, JPY, etc.
    
    LogPrintf("O Currency Registry: Initialized with %d currencies\n", static_cast<int>(currencies.size()));
}

// Global currency registry instance
CurrencyRegistry g_currency_registry;

