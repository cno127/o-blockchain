# O Blockchain Multi-Currency Architecture

## Overview

The O blockchain supports multiple currencies through a comprehensive multi-currency system that extends Bitcoin's single-currency model. This system enables transactions in various fiat currencies while maintaining compatibility with Bitcoin's core architecture.

## Core Components

### 1. Currency Identification System

#### CurrencyId Type
```cpp
typedef uint32_t CurrencyId;
```
- **Type**: 32-bit unsigned integer
- **Range**: 0 to 1,000 (MAX_CURRENCIES)
- **Purpose**: Unique identifier for each currency

#### Predefined Currency IDs
```cpp
// Base Currency
CURRENCY_BTC = 0;  // Bitcoin (base currency)

// Major Reserve Currencies
CURRENCY_USD = 1;  // US Dollar
CURRENCY_EUR = 2;  // Euro
CURRENCY_JPY = 3;  // Japanese Yen
CURRENCY_GBP = 4;  // British Pound Sterling
CURRENCY_CNY = 5;  // Chinese Yuan Renminbi

// ... 60+ more currencies
```

### 2. Currency Metadata Structure

```cpp
struct CurrencyMetadata {
    CurrencyId id;              // Unique identifier
    std::string symbol;         // 3-letter ISO code (USD, EUR, etc.)
    std::string name;           // Full currency name
    uint8_t decimals;           // Number of decimal places
    bool is_fiat;              // Whether this is a fiat currency
    std::string issuer_address; // Address of the currency issuer
}
```

**Key Fields:**
- **id**: Unique numeric identifier
- **symbol**: ISO 4217 currency code
- **name**: Human-readable currency name
- **decimals**: Precision (e.g., 2 for USD, 0 for JPY)
- **is_fiat**: Distinguishes fiat from crypto currencies
- **issuer_address**: For fiat currencies, identifies the issuing authority

### 3. Multi-Currency Amount Structure

```cpp
struct MultiCurrencyAmount {
    CurrencyId currency_id;  // Which currency this amount represents
    int64_t amount;         // Amount in smallest currency unit
}
```

**Example:**
- `currency_id = 1` (USD), `amount = 10000` = $100.00 USD
- `currency_id = 0` (BTC), `amount = 500000000` = 5.0 BTC

### 4. Currency Registry System

```cpp
class CurrencyRegistry {
private:
    std::map<CurrencyId, CurrencyMetadata> currencies;      // ID -> Metadata
    std::map<std::string, CurrencyId> symbol_to_id;        // Symbol -> ID
    
public:
    bool RegisterCurrency(const CurrencyMetadata& metadata);
    std::optional<CurrencyMetadata> GetCurrency(CurrencyId id) const;
    std::optional<CurrencyId> GetCurrencyId(const std::string& symbol) const;
    bool IsSupported(CurrencyId id) const;
    std::vector<CurrencyMetadata> GetAllCurrencies() const;
}
```

**Features:**
- **Registration**: Add new currencies with validation
- **Lookup**: Find currencies by ID or symbol
- **Validation**: Check if currency is supported
- **Enumeration**: List all registered currencies

## Transaction Structure

### 1. Multi-Currency Transaction Output

```cpp
class CMultiCurrencyTxOut {
public:
    CScript scriptPubKey;                           // Output script
    std::vector<MultiCurrencyAmount> amounts;      // Multiple currency amounts
    
    // Key Methods
    CAmount GetAmount(CurrencyId currency_id) const;
    void SetAmount(CurrencyId currency_id, CAmount value);
    bool HasCurrency(CurrencyId currency_id) const;
    void RemoveCurrency(CurrencyId currency_id);
}
```

**Key Features:**
- **Multiple Amounts**: Single output can contain multiple currencies
- **Currency-Specific Access**: Get/set amounts for specific currencies
- **Legacy Compatibility**: Converts to/from Bitcoin's CTxOut

### 2. Multi-Currency Transaction Input

```cpp
class CMultiCurrencyTxIn {
public:
    COutPoint prevout;        // Reference to previous output
    CScript scriptSig;        // Input script
    uint32_t nSequence;       // Sequence number
    CScriptWitness scriptWitness; // Witness data
    
    // Legacy compatibility
    CTxIn ToLegacyTxIn() const;
}
```

### 3. Multi-Currency Transaction

```cpp
class CMultiCurrencyTransaction {
public:
    int32_t nVersion;                              // Transaction version
    std::vector<CMultiCurrencyTxIn> vin;          // Inputs
    std::vector<CMultiCurrencyTxOut> vout;        // Outputs
    uint32_t nLockTime;                           // Lock time
    
    // Key Methods
    uint256 GetHash() const;
    CTransaction ToLegacyTransaction() const;
}
```

## How Multiple Currencies Work

### 1. Currency Registration Process

```cpp
// Register a new currency
CurrencyMetadata usd_metadata(
    CURRENCY_USD,           // ID: 1
    "USD",                  // Symbol
    "US Dollar",            // Name
    2,                      // 2 decimal places
    true,                   // Is fiat currency
    ""                      // Issuer address
);

g_currency_registry.RegisterCurrency(usd_metadata);
```

### 2. Transaction Creation

```cpp
// Create a multi-currency transaction
CMultiCurrencyTransaction tx;

// Add input (spending previous output)
CMultiCurrencyTxIn input(prevout, scriptSig, sequence);
tx.vin.push_back(input);

// Add output with multiple currencies
CMultiCurrencyTxOut output;
output.scriptPubKey = recipient_script;
output.SetAmount(CURRENCY_USD, 10000);    // $100.00 USD
output.SetAmount(CURRENCY_EUR, 8500);     // €85.00 EUR
tx.vout.push_back(output);
```

### 3. Amount Handling

```cpp
// Get amount in specific currency
CAmount usd_amount = output.GetAmount(CURRENCY_USD);

// Set amount in specific currency
output.SetAmount(CURRENCY_JPY, 10000);  // ¥100.00 JPY

// Check if output contains currency
if (output.HasCurrency(CURRENCY_GBP)) {
    // Handle GBP amount
}

// Remove currency from output
output.RemoveCurrency(CURRENCY_CAD);
```

### 4. Legacy Compatibility

```cpp
// Convert to Bitcoin transaction (BTC amounts only)
CTransaction legacy_tx = multi_tx.ToLegacyTransaction();

// Convert from Bitcoin transaction
CMultiCurrencyTransaction multi_tx(legacy_tx);
```

## Supported Currencies

### Current Registry (65 currencies)

#### Major Reserve Currencies (5)
- USD, EUR, JPY, GBP, CNY

#### G7/G20 Major Currencies (10)
- CAD, AUD, CHF, NZD, SEK, NOK, DKK, PLN, CZK, HUF

#### Asian Major Currencies (10)
- KRW, SGD, HKD, TWD, THB, MYR, IDR, PHP, VND, INR

#### Middle East & Africa (14)
- AED, SAR, QAR, KWD, BHD, OMR, JOD, ILS, TRY, EGP, ZAR, NGN, KES, ETB

#### Americas (8)
- MXN, BRL, ARS, CLP, COP, PEN, UYU, VES

#### European Union & Others (7)
- RON, BGN, HRK, RUB, UAH, BYN, KZT

#### Commonwealth & Others (10)
- ISK, LKR, BDT, PKR, AFN, IQD, IRR, LBP, SYP, YER

#### Cryptocurrency (1)
- BTC (Bitcoin as base currency)

## Water Price Integration

### Water Price Measurement
Water prices are measured using existing fiat currencies:
- Water costs **$0.003 USD** per liter
- Water costs **€0.0028 EUR** per liter
- Water costs **¥0.45 JPY** per liter

### Benefits
- **No duplicate currencies**: Uses existing fiat currencies
- **Standard approach**: Like measuring oil prices in USD
- **Cross-currency analysis**: Easy comparison using exchange rates
- **Simplified system**: No separate "water price currencies"

## RPC Interface

### Currency Management Commands

```bash
# List all currencies
bitcoin-cli listcurrencies

# List only fiat currencies
bitcoin-cli listcurrencies "fiat"

# Get specific currency info
bitcoin-cli getcurrency "USD"
bitcoin-cli getcurrency "1"

# Register new currency (requires permissions)
bitcoin-cli registercurrency 100 "TEST" "Test Currency" 2 true
```

## Validation and Consensus

### Currency Validation
- **ID Uniqueness**: Each currency ID must be unique
- **Symbol Uniqueness**: Each currency symbol must be unique
- **Range Validation**: Currency ID must be < MAX_CURRENCIES
- **Metadata Validation**: All required fields must be present

### Transaction Validation
- **Currency Support**: All currencies in transaction must be registered
- **Amount Validation**: Amounts must be within valid ranges
- **Script Validation**: Output scripts must be valid
- **Legacy Compatibility**: Must convert properly to/from Bitcoin transactions

## Security Considerations

### Currency Registry Security
- **Registration Control**: New currency registration requires special permissions
- **Immutable Core**: Core currencies (USD, EUR, etc.) cannot be modified
- **Validation**: All currency metadata is validated before registration

### Transaction Security
- **Currency Isolation**: Each currency is handled independently
- **Amount Validation**: Prevents overflow and invalid amounts
- **Script Security**: Standard Bitcoin script validation applies

## Performance Considerations

### Memory Usage
- **Currency Registry**: Minimal memory footprint (65 currencies)
- **Transaction Size**: Slightly larger than Bitcoin transactions
- **Amount Storage**: Efficient storage using currency IDs

### Processing Speed
- **Currency Lookup**: O(1) lookup by ID, O(log n) by symbol
- **Amount Processing**: Linear time for multiple currencies per output
- **Legacy Conversion**: Minimal overhead for Bitcoin compatibility

## Future Enhancements

### Planned Features
1. **Dynamic Currency Addition**: Governance-based currency registration
2. **Exchange Rate Integration**: Real-time exchange rate feeds
3. **Currency Stability Metrics**: Track currency stability for reputation
4. **Regional Currency Groups**: Support for currency unions

### Scalability
- **Currency Limit**: Currently 1,000 currencies maximum
- **Transaction Size**: Optimized for multiple currencies per output
- **Network Efficiency**: Minimal impact on network performance

## Conclusion

The O blockchain's multi-currency system provides:

1. **Comprehensive Support**: 65+ currencies from all major economic regions
2. **Bitcoin Compatibility**: Full compatibility with existing Bitcoin infrastructure
3. **Flexible Architecture**: Easy to add new currencies and features
4. **Water Price Integration**: Seamless integration with water price measurement
5. **Global Scalability**: Ready for worldwide deployment

This architecture enables the O blockchain to function as a truly global platform for water price calibration and stabilization, supporting users and businesses worldwide with their local currencies.

