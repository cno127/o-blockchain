# Genesis Seed Users - TODO

## Status: ⏳ PARTIALLY IMPLEMENTED

**Code written:** ✅ Yes  
**Compiles:** ✅ Yes  
**Enabled:** ❌ No (disabled due to startup crash)  
**Priority:** 🔴 **HIGH** (critical for bootstrap)

---

## What's Implemented

✅ **Deterministic key generation** (`GenerateGenesisKey`)  
✅ **Genesis user creation** (`CreateGenesisSeedUsers`)  
✅ **50 seed users** (5 currencies × 10 users)  
✅ **USER_VERIFY transactions** with proper signatures  

---

## The Crash

**Symptom:** Node segfaults (exit code 139) on startup when genesis seed users are enabled

**Location:** During genesis block creation or validation

**Probable Causes:**
1. `Hash()` function not safe to call at genesis creation time
2. O transaction validation runs before databases initialized
3. Missing null checks in `CUserVerificationData::ToScript()`
4. Genesis block merkle root calculation with O transactions

---

## Debugging Steps Needed

### 1. Add Error Handling
```cpp
static std::vector<CMutableTransaction> CreateGenesisSeedUsers() {
    try {
        // ... seed user creation ...
        return seed_txs;
    } catch (const std::exception& e) {
        LogPrintf("ERROR: CreateGenesisSeedUsers failed: %s\n", e.what());
        return std::vector<CMutableTransaction>();  // Return empty on error
    }
}
```

### 2. Test with Single User
```cpp
// Instead of 50 users, test with just 1
for (int i = 0; i < 1; i++) {  // Only 1 user instead of 10
    // ...
}
```

### 3. Skip Genesis O Transaction Processing
```cpp
// In validation.cpp
if (pindex->nHeight > 0) {  // Skip genesis block
    OConsensus::ProcessOTransactions(block, pindex);
}
```

### 4. Defer User Processing
Instead of genesis block, add users at block 1:
```cpp
// In miner.cpp CreateNewBlock()
if (nHeight == 1) {
    auto seed_users = CreateGenesisSeedUsers();
    for (const auto& seed_tx : seed_users) {
        pblock->vtx.push_back(MakeTransactionRef(seed_tx));
    }
}
```

---

## Alternative: Manual Bootstrap

**Until genesis seeds work, use manual seeding:**

### Option 1: RPC Command
```bash
# Admin creates seed users for major currencies
bitcoin-cli creategenesisusers USD 10
bitcoin-cli creategenesisusers EUR 10
bitcoin-cli creategenesisusers JPY 10
bitcoin-cli creategenesisusers GBP 10
bitcoin-cli creategenesisusers CNY 10

# Mine block to include them
bitcoin-cli generatetoaddress 1 [address]

# Check readiness
bitcoin-cli getmeasurementreadinessstatistics
```

### Option 2: Configuration File
```
# bitcoin.conf
genesis-seed-users=USD:10,EUR:10,JPY:10,GBP:10,CNY:10
```

### Option 3: Python Script
```python
#!/usr/bin/env python3
# seed_genesis.py

for currency in ["USD", "EUR", "JPY", "GBP", "CNY"]:
    for i in range(10):
        # Generate deterministic key
        key = generate_genesis_key(currency, i)
        
        # Create USER_VERIFY transaction
        rpc.submituserverificationtx(...)
```

---

## Current Workaround

**For immediate testing and deployment:**

###

 Use Bootstrap Threshold Only (No Genesis Users)

```
Block 0: 0 users
Admin manually creates 10 users per currency via RPC
Block 1-10: Users registered
Block 10: Check readiness → 10 users >= 10 (bootstrap) ✅ READY
Block 10: Create invitations
Block 11+: System operational
```

**Pros:**
- ✅ Works immediately
- ✅ No genesis block changes
- ✅ No crash risk

**Cons:**
- ⚠️ Requires manual intervention
- ⚠️ Not automatic
- ⚠️ Admin must seed each network

---

## Fix Priority

**Impact:** 🔴 **HIGH**

**Why critical:**
- Without genesis seeds OR manual seeding, system cannot start
- Requires admin intervention for every deployment
- Not user-friendly

**Why not blocking:**
- Bootstrap threshold works (10-user minimum)
- Manual seeding is viable workaround
- Can deploy and seed manually

---

## Recommended Next Action

### Option 1: Debug Genesis (Ideal)
- Add error handling
- Find crash cause
- Fix and re-enable

### Option 2: Block 1 Seeding (Safer)
- Add seed users at block 1 instead of genesis
- Avoids genesis complexity
- Still automatic

### Option 3: Manual Seeding RPC (Pragmatic)
- Create `seedgenesisusers` RPC command
- Admin seeds when deploying
- Simple and reliable

---

**Decision needed:** Which approach to pursue?

