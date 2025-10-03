# O Blockchain Currency Registry

## Overview

The O Blockchain Currency Registry is a comprehensive system for managing and supporting multiple currencies within the O blockchain ecosystem. This registry enables the O blockchain to function as a multi-currency platform with special focus on water price calibration and stabilization.

## Supported Currencies

### Major Reserve Currencies
- **USD** - US Dollar (ID: 1)
- **EUR** - Euro (ID: 2)
- **JPY** - Japanese Yen (ID: 3)
- **GBP** - British Pound Sterling (ID: 4)
- **CNY** - Chinese Yuan Renminbi (ID: 5)

### G7/G20 Major Currencies
- **CAD** - Canadian Dollar (ID: 6)
- **AUD** - Australian Dollar (ID: 7)
- **CHF** - Swiss Franc (ID: 8)
- **NZD** - New Zealand Dollar (ID: 9)
- **SEK** - Swedish Krona (ID: 10)
- **NOK** - Norwegian Krone (ID: 11)
- **DKK** - Danish Krone (ID: 12)
- **PLN** - Polish Zloty (ID: 13)
- **CZK** - Czech Koruna (ID: 14)
- **HUF** - Hungarian Forint (ID: 15)

### Asian Major Currencies
- **KRW** - South Korean Won (ID: 16)
- **SGD** - Singapore Dollar (ID: 17)
- **HKD** - Hong Kong Dollar (ID: 18)
- **TWD** - Taiwan Dollar (ID: 19)
- **THB** - Thai Baht (ID: 20)
- **MYR** - Malaysian Ringgit (ID: 21)
- **IDR** - Indonesian Rupiah (ID: 22)
- **PHP** - Philippine Peso (ID: 23)
- **VND** - Vietnamese Dong (ID: 24)
- **INR** - Indian Rupee (ID: 25)

### Middle East & Africa
- **AED** - UAE Dirham (ID: 26)
- **SAR** - Saudi Riyal (ID: 27)
- **QAR** - Qatari Riyal (ID: 28)
- **KWD** - Kuwaiti Dinar (ID: 29)
- **BHD** - Bahraini Dinar (ID: 30)
- **OMR** - Omani Rial (ID: 31)
- **JOD** - Jordanian Dinar (ID: 32)
- **ILS** - Israeli Shekel (ID: 33)
- **TRY** - Turkish Lira (ID: 34)
- **EGP** - Egyptian Pound (ID: 35)
- **ZAR** - South African Rand (ID: 36)
- **NGN** - Nigerian Naira (ID: 37)
- **KES** - Kenyan Shilling (ID: 38)
- **ETB** - Ethiopian Birr (ID: 39)

### Americas
- **MXN** - Mexican Peso (ID: 40)
- **BRL** - Brazilian Real (ID: 41)
- **ARS** - Argentine Peso (ID: 42)
- **CLP** - Chilean Peso (ID: 43)
- **COP** - Colombian Peso (ID: 44)
- **PEN** - Peruvian Sol (ID: 45)
- **UYU** - Uruguayan Peso (ID: 46)
- **VES** - Venezuelan Bolivar (ID: 47)

### European Union & Others
- **RON** - Romanian Leu (ID: 48)
- **BGN** - Bulgarian Lev (ID: 49)
- **HRK** - Croatian Kuna (ID: 50)
- **RUB** - Russian Ruble (ID: 51)
- **UAH** - Ukrainian Hryvnia (ID: 52)
- **BYN** - Belarusian Ruble (ID: 53)
- **KZT** - Kazakhstani Tenge (ID: 54)

### Commonwealth & Others
- **ISK** - Icelandic Krona (ID: 55)
- **LKR** - Sri Lankan Rupee (ID: 56)
- **BDT** - Bangladeshi Taka (ID: 57)
- **PKR** - Pakistani Rupee (ID: 58)
- **AFN** - Afghan Afghani (ID: 59)
- **IQD** - Iraqi Dinar (ID: 60)
- **IRR** - Iranian Rial (ID: 61)
- **LBP** - Lebanese Pound (ID: 62)
- **SYP** - Syrian Pound (ID: 63)
- **YER** - Yemeni Rial (ID: 64)

### Water Price Measurement
**Water prices are measured using the existing fiat currencies above.** There are no separate "water price currencies."

Examples:
- Water costs **$0.003 USD** per liter (measured in USD)
- Water costs **€0.0028 EUR** per liter (measured in EUR)
- Water costs **¥0.45 JPY** per liter (measured in JPY)

This approach is simpler, more logical, and follows standard practices (like oil prices measured in USD).

## Currency Metadata Structure

Each currency in the registry contains the following metadata:

```cpp
struct CurrencyMetadata {
    CurrencyId id;              // Unique currency identifier
    std::string symbol;         // 3-letter currency code (ISO 4217)
    std::string name;           // Full currency name
    uint8_t decimals;           // Number of decimal places
    bool is_fiat;              // Whether this is a fiat currency
    std::string issuer_address; // Address of the currency issuer (for fiat)
}
```

## RPC Commands

### listcurrencies
Lists all registered currencies in the O blockchain system.

**Parameters:**
- `category` (string, optional): Filter by category: "all", "fiat", "crypto", "water"

**Example:**
```bash
bitcoin-cli listcurrencies
bitcoin-cli listcurrencies "fiat"
bitcoin-cli listcurrencies "water"
```

### getcurrency
Get detailed information about a specific currency.

**Parameters:**
- `symbol_or_id` (string, required): Currency symbol (e.g., 'USD') or ID (e.g., 1)

**Example:**
```bash
bitcoin-cli getcurrency "USD"
bitcoin-cli getcurrency "1"
```

### registercurrency
Register a new currency in the O blockchain system (requires special permissions).

**Parameters:**
- `id` (number, required): Currency ID (must be unique)
- `symbol` (string, required): Currency symbol (3-letter code)
- `name` (string, required): Full currency name
- `decimals` (number, required): Number of decimal places
- `is_fiat` (boolean, required): Whether this is a fiat currency
- `issuer_address` (string, optional): Issuer address

**Example:**
```bash
bitcoin-cli registercurrency 100 "TEST" "Test Currency" 2 true
```

## Integration with O Blockchain Features

### Water Price Calibration
The currency registry is essential for the O blockchain's water price calibration system:

1. **Standard Water Price Measurement**: Water prices are measured in existing fiat currencies (e.g., $0.003 USD per liter), following standard commodity pricing practices.

2. **Geographic-Specific Measurements**: Users can report water prices in their local currencies (USD, EUR, JPY, etc.), providing regional water price data.

3. **Cross-Currency Analysis**: The system can compare water prices across different economic zones by converting prices using exchange rates.

4. **Stabilization Mining**: The stabilization mining system uses currency exchange rates and regional water price data to adjust the supply of O tokens based on water price deviations.

### User Verification System
The currency registry supports the user verification system by:

1. **Geographic Diversity**: Users from different countries can register with their local currencies.
2. **KYC Integration**: Government ID verification can be linked to specific currencies.
3. **Reputation Scoring**: User reputation scores can be weighted by currency stability and economic indicators.

### Transaction Support
The multi-currency transaction system supports:

1. **Cross-Currency Transactions**: Users can send and receive payments in different currencies.
2. **Exchange Rate Integration**: Real-time exchange rates are used for currency conversions.
3. **Fee Calculation**: Transaction fees can be calculated in different currencies.

## Technical Implementation

### Files Structure
- `src/consensus/multicurrency.h` - Currency registry definitions and structures
- `src/consensus/multicurrency.cpp` - Currency registry implementation
- `src/rpc/o_currency_rpc.h` - RPC command declarations
- `src/rpc/o_currency_rpc.cpp` - RPC command implementations

### Key Classes
- `CurrencyRegistry` - Main registry class for managing currencies
- `CurrencyMetadata` - Metadata structure for each currency
- `MultiCurrencyAmount` - Amount structure supporting multiple currencies

### Global Instance
The currency registry is available globally as `g_currency_registry` and is automatically initialized with all official fiat currencies at startup.

## Future Enhancements

1. **Dynamic Currency Addition**: Support for adding new currencies through governance proposals
2. **Exchange Rate Integration**: Real-time exchange rate feeds from multiple sources
3. **Currency Stability Metrics**: Track currency stability for reputation scoring
4. **Regional Currency Groups**: Support for currency unions and regional monetary policies
5. **Cryptocurrency Support**: Extended support for major cryptocurrencies beyond Bitcoin

## Security Considerations

1. **Currency Validation**: All currency registrations are validated for proper formatting and uniqueness
2. **Permission Control**: Currency registration requires special permissions in production
3. **Audit Trail**: All currency modifications are logged for audit purposes
4. **Rate Limiting**: Currency registration is rate-limited to prevent abuse

## Compliance

The currency registry supports compliance requirements by:

1. **ISO 4217 Compliance**: All currency codes follow ISO 4217 standards
2. **Regulatory Reporting**: Currency usage can be tracked for regulatory compliance
3. **AML/KYC Integration**: Currency transactions can be linked to user verification status
4. **Tax Reporting**: Multi-currency transactions support tax calculation and reporting

This comprehensive currency registry makes the O blockchain a truly global platform capable of supporting water price calibration and stabilization across all major economies worldwide.
