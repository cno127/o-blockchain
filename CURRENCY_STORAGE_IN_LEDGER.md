# Where Currency Code is Stored in General Ledger

## 🗄️ **Currency Storage in UTXO Database**

The currency code is stored **directly in each transaction output (UTXO)** using a `MultiCurrencyAmount` structure.

---

## 📊 **Database Structure**

### **UTXO Record in LevelDB:**

**Key:**
```
'C' + transaction_hash + output_index
```

**Value (Coin):**
```cpp
struct Coin {
  CTxOut out;          // Transaction output
  uint32_t nHeight;    // Block height
  bool fCoinBase;      // Is coinbase?
}

struct CTxOut {
  CAmount nValue;      // For backward compatibility
  CScript scriptPubKey; // Locking script
  
  // For O Blockchain (multicurrency):
  vector<MultiCurrencyAmount> amounts;  // ← Currency codes stored here!
}

struct MultiCurrencyAmount {
  uint32_t currency_id;  // ← Currency code (1=OUSD, 2=OEUR, etc.)
  int64_t amount;        // Amount in smallest unit (cents)
}
```

---

## 💾 **Example UTXO Records**

### **Example 1: Simple OUSD Output**

**Conceptual:**
```
Alice has 100.00 OUSD
```

**Actual Database Record:**
```
Key: 'C' + 0xabcd1234...ef + 0

Value: {
  out: {
    nValue: 10000,  // Legacy field (100.00 in cents)
    scriptPubKey: <Alice's locking script>,
    amounts: [
      {
        currency_id: 1,     // CURRENCY_USD = 1 (OUSD)
        amount: 10000       // 100.00 O (2 decimals)
      }
    ]
  },
  nHeight: 12345,
  fCoinBase: false
}
```

**Storage Size:**
```
Key: 33 bytes ('C' + 32-byte hash + varint)
Value: ~40-50 bytes
  - nHeight: 4 bytes
  - fCoinBase: 1 bit
  - currency_id: 4 bytes (uint32_t)
  - amount: 8 bytes (int64_t)
  - scriptPubKey: ~25-35 bytes
Total: ~73-83 bytes per UTXO
```

### **Example 2: Multiple Currencies (Future)**

**Conceptual:**
```
Bob has both 50.00 OUSD AND 30.00 OEUR in one UTXO
(Not currently used, but supported)
```

**Database Record:**
```
Key: 'C' + 0x9876fedc...ab + 1

Value: {
  out: {
    amounts: [
      {
        currency_id: 1,     // OUSD
        amount: 5000        // 50.00 O
      },
      {
        currency_id: 2,     // OEUR
        amount: 3000        // 30.00 O
      }
    ],
    scriptPubKey: <Bob's locking script>
  },
  nHeight: 12346,
  fCoinBase: false
}
```

---

## 🔍 **How to Query by Currency**

### **Finding All UTXOs for a Currency:**

**Challenge:**
```
LevelDB is key-value store
Currency is in the VALUE, not the KEY
Cannot directly query "all OUSD UTXOs"
```

**Solution 1: Full Scan (Slow)**
```cpp
// Scan entire UTXO set
for (auto& [outpoint, coin] : utxo_set) {
  for (auto& amount : coin.out.amounts) {
    if (amount.currency_id == CURRENCY_USD) {
      // Found an OUSD UTXO
      total += amount.amount;
    }
  }
}

// Works but slow for large UTXO sets
```

**Solution 2: Separate Index (Fast)**
```cpp
// Maintain currency index in separate LevelDB
Key: 'CURR' + currency_id + outpoint
Value: 1 (existence marker)

// Now can quickly query all UTXOs for a currency
Key: 'CURR' + 1 + ...  // All OUSD UTXOs
```

**Solution 3: Wallet Index (Fastest for User)**
```cpp
// Wallet maintains own index
wallet_currency_index: {
  OUSD: [outpoint1, outpoint2, ...],
  OEUR: [outpoint3, outpoint4, ...],
  ...
}

// O(1) lookup for user's currencies
```

---

## 📋 **Complete UTXO Structure**

### **Full Database Entry:**

**Key (33-35 bytes):**
```
┌──────┬────────────────────┬─────────────┐
│ 'C'  │  Transaction Hash  │ Output Index│
│ 1B   │      32 bytes      │   varint    │
└──────┴────────────────────┴─────────────┘
```

**Value (~40-50 bytes for single currency):**
```
┌─────────────────────────────────────────────┐
│ Coin Metadata                               │
├─────────────────────────────────────────────┤
│ Height: 12345 (varint)                      │
│ IsCoinbase: false (1 bit)                   │
├─────────────────────────────────────────────┤
│ CTxOut                                      │
├─────────────────────────────────────────────┤
│ Legacy nValue: 10000 (for compatibility)    │
│ scriptPubKey: <25-35 bytes>                 │
│                                             │
│ MultiCurrencyAmounts: [                     │
│   {                                         │
│     currency_id: 1 (OUSD)                   │
│     amount: 10000 (100.00 O)                │
│   }                                         │
│ ]                                           │
└─────────────────────────────────────────────┘
```

---

## 🔄 **Transaction Example with Currency Exchange**

### **OUSD → OEUR Exchange Transaction:**

**User wants to exchange 100 OUSD for OEUR**

**Transaction:**
```
{
  version: 2,
  
  vin: [  // Inputs (what user spends)
    {
      prevout: <TxHash>:<Index>,  // User's 100 OUSD UTXO
      scriptSig: <signature>,
      
      // This input references a UTXO with:
      // currency_id: 1 (OUSD)
      // amount: 10000 (100.00 OUSD)
    }
  ],
  
  vout: [  // Outputs (what gets created)
    {
      scriptPubKey: <Recipient's address>,
      amounts: [
        {
          currency_id: 2,      // OEUR
          amount: 6760         // 67.60 OEUR
        }
      ]
    },
    {
      scriptPubKey: <Change address>,
      amounts: [
        {
          currency_id: 1,      // OUSD (change, if any)
          amount: 0            // No change in this example
        }
      ]
    }
  ],
  
  nLockTime: 0
}
```

**What Gets Stored in UTXO Set:**

**Before Transaction:**
```
UTXO Set:
  'C' + 0xaaa...000 + 0 → 100.00 OUSD (User's balance)
```

**After Transaction:**
```
UTXO Set:
  'C' + 0xaaa...000 + 0 → DELETED (spent)
  'C' + 0xbbb...111 + 0 → 67.60 OEUR (Recipient's new UTXO)
```

---

## 📊 **Storage Efficiency**

### **Size Comparison:**

**Single Currency UTXO:**
```
Currency metadata: 12 bytes
  - currency_id: 4 bytes
  - amount: 8 bytes

Total UTXO: ~73-83 bytes
```

**Multi-Currency UTXO (if used):**
```
Per additional currency: +12 bytes

Example (OUSD + OEUR in one UTXO):
  Base: 73 bytes
  Second currency: +12 bytes
  Total: 85 bytes
```

**Efficiency:**
```
Separate UTXOs: 73 × 2 = 146 bytes
Combined UTXO: 85 bytes
Savings: 41% space savings (if using multi-currency outputs)
```

---

## 🎯 **Currency Code Encoding**

### **Currency ID to Code Mapping:**

```cpp
CurrencyId (uint32_t) → O Currency Code (string)

1  → "OUSD"
2  → "OEUR"
3  → "OJPY"
4  → "OGBP"
5  → "OCNY"
...
145 → "OKMF"

// Stored as uint32_t (4 bytes) in database
// Converted to string for display
```

**Why use ID instead of string?**
```
String "OUSD": 4 bytes
uint32_t: 4 bytes

Same size BUT:
  - IDs are compact
  - Fast comparison (integer vs string)
  - Easy to extend
  - Backward compatible
```

---

## 🔍 **How to Find Currency Balances**

### **Method 1: Scan UTXO Set (Complete)**

```cpp
// Find all OUSD coins
CAmount total_ousd = 0;

for (auto& [outpoint, coin] : utxo_db) {
  for (auto& amount : coin.out.amounts) {
    if (amount.currency_id == CURRENCY_USD) {  // OUSD
      total_ousd += amount.amount;
    }
  }
}

// Result: Total OUSD in circulation
```

### **Method 2: User Wallet Index (Fast)**

```cpp
// Wallet maintains index of user's UTXOs
wallet_index: {
  address1: {
    OUSD: [utxo1, utxo2, ...],  // User's OUSD UTXOs
    OEUR: [utxo3, utxo4, ...],  // User's OEUR UTXOs
  }
}

// O(1) lookup for user's balance per currency
```

### **Method 3: Currency Index (Optional)**

```cpp
// Maintain separate currency index
currency_index_db:
  'CURR' + currency_id + outpoint → 1

// Quick lookup: all UTXOs for a currency
// Useful for: total supply queries, statistics
```

---

## 💡 **Key Insights**

### **Currency Storage:**
```
✅ Stored IN each UTXO (not separate table)
✅ Uses compact uint32_t currency_id
✅ Supports multiple currencies per UTXO
✅ LevelDB handles billions of records
✅ Efficient and scalable
```

### **Balance Calculation:**
```
User balance = SUM of all UTXOs for:
  1. User's addresses
  2. Specific currency
  3. Unspent status

No "balance table" - calculated from UTXOs
```

### **Exchange Transactions:**
```
Input: UTXOs in one currency (e.g., OUSD)
Output: UTXOs in another currency (e.g., OEUR)
Validation: Blockchain enforces correct exchange rate
```

---

## 🎯 **Summary**

### **Where Currency Code is Stored:**

**Answer:** In the **`amounts` field of each CTxOut** (transaction output), stored as a **`currency_id` (uint32_t)** paired with an amount.

**Storage Location:**
```
LevelDB Database
  → UTXO Set (chainstate/)
    → Coin record
      → CTxOut
        → MultiCurrencyAmount[]
          → currency_id (4 bytes)  ← HERE!
          → amount (8 bytes)
```

**Size:** 4 bytes per currency per UTXO  
**Format:** uint32_t (1=OUSD, 2=OEUR, etc.)  
**Scalability:** ✅ Billions of UTXOs supported

The O Blockchain's multicurrency system is **efficiently integrated into the UTXO model** with minimal overhead and maximum scalability! 💾✅

