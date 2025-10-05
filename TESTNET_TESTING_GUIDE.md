# O Blockchain Testnet Testing Guide

## 🧪 **Testing the O Blockchain**

This guide shows you how to test the O Blockchain on regtest (regression test network), which is perfect for development and testing all features including measurements, stabilization, and multi-currency support.

---

## 🚀 **Quick Start**

### **1. Start O Blockchain in Regtest Mode:**

```bash
cd /Users/o/bitcoin

# Start the daemon in regtest mode
./build/bin/bitcoind -regtest -daemon

# Wait a few seconds for it to start
sleep 3

# Check it's running
./build/bin/bitcoin-cli -regtest getblockchaininfo
```

### **2. Generate Some Blocks (Create Initial Coins):**

```bash
# Generate a new address to receive mining rewards
ADDRESS=$(./build/bin/bitcoin-cli -regtest getnewaddress)

# Mine 101 blocks (need 100 confirmations for coinbase maturity)
./build/bin/bitcoin-cli -regtest generatetoaddress 101 $ADDRESS

# Check your balance
./build/bin/bitcoin-cli -regtest getbalance
```

### **3. Test O Blockchain Features:**

```bash
# Test measurement readiness
./build/bin/bitcoin-cli -regtest checkmeasurementreadiness "water_price" "USD"

# Get stability status
./build/bin/bitcoin-cli -regtest getstabilitystatus "OUSD"

# Check measurement statistics
./build/bin/bitcoin-cli -regtest getmeasurementstatistics

# Get all unstable currencies
./build/bin/bitcoin-cli -regtest getunstablecurrencies
```

---

## 📋 **Complete Testing Workflow**

### **Test 1: Water Price Measurement Submission**

```bash
# Note: This is a conceptual example - actual RPC interface may vary

# Submit a water price measurement
./build/bin/bitcoin-cli -regtest submitwaterprice \
  '{
    "currency_code": "USD",
    "price": 120,
    "volume": 1.0,
    "volume_unit": "L",
    "source_url": "https://walmart.com/water",
    "invite_id": "YOUR_INVITE_ID_HERE"
  }'

# Check if it was accepted
./build/bin/bitcoin-cli -regtest getmeasurementstatistics
```

### **Test 2: Volume Validation**

```bash
# Test metric system (0.9L - 1.1L)
# Valid: 1.0L
./build/bin/bitcoin-cli -regtest submitwaterprice \
  '{"currency_code": "EUR", "price": 120, "volume": 1.0, "volume_unit": "L", ...}'

# Valid: 950mL (0.95L)
./build/bin/bitcoin-cli -regtest submitwaterprice \
  '{"currency_code": "EUR", "price": 114, "volume": 950, "volume_unit": "mL", ...}'

# Test imperial system (30-37 fl oz)
# Valid: 33.8 fl oz
./build/bin/bitcoin-cli -regtest submitwaterprice \
  '{"currency_code": "USD", "price": 100, "volume": 33.8, "volume_unit": "fl oz", ...}'

# Invalid: Too small (500mL = 0.5L)
./build/bin/bitcoin-cli -regtest submitwaterprice \
  '{"currency_code": "EUR", "price": 60, "volume": 500, "volume_unit": "mL", ...}'
# Expected: Rejected - volume out of range
```

### **Test 3: Exchange Rate Measurement**

```bash
# Submit OUSD ↔ USD exchange rate
./build/bin/bitcoin-cli -regtest submitexchangerate \
  '{
    "from_currency": "OUSD",
    "to_currency": "USD",
    "exchange_rate": 1.2,
    "source_url": "https://exchangerate.com",
    "invite_id": "YOUR_INVITE_ID_HERE"
  }'
```

### **Test 4: Currency Stability Detection**

```bash
# Manually recalculate currency stability
./build/bin/bitcoin-cli -regtest recalculatecurrencystability 1000

# Check stability status for OUSD
./build/bin/bitcoin-cli -regtest getstabilitystatus "OUSD"

# Expected output:
# {
#   "currency": "OUSD",
#   "status": "STABLE" or "UNSTABLE",
#   "expected_water_price": 1.0,
#   "observed_water_price": 1.2,
#   "deviation": 0.20 or 20%,
#   ...
# }
```

### **Test 5: Stabilization Mining**

```bash
# Get list of unstable currencies
./build/bin/bitcoin-cli -regtest getunstablecurrencies

# Get stabilization statistics
./build/bin/bitcoin-cli -regtest getstabilizationstats

# Mine a block and check if stabilization coins were created
./build/bin/bitcoin-cli -regtest generatetoaddress 1 $ADDRESS

# Check stabilization history
./build/bin/bitcoin-cli -regtest getstabilizationhistory "OUSD" 0 1000
```

### **Test 6: KYC Validation (USA)**

```bash
# Check if USA requires KYC
./build/bin/bitcoin-cli -regtest doescountryrequirekyc "US"

# Get recommended verification method for USA
./build/bin/bitcoin-cli -regtest getrecommendedverificationmethod "US"

# Get allowed verification methods
./build/bin/bitcoin-cli -regtest getallowedverificationmethods "US"

# Expected: THIRD_PARTY_KYC for USA, GOVERNMENT_ID for crypto-friendly countries
```

### **Test 7: Measurement Invitations**

```bash
# Create water price measurement invites
./build/bin/bitcoin-cli -regtest createinvites 10 "water_price" "USD"

# Create exchange rate measurement invites
./build/bin/bitcoin-cli -regtest createinvites 10 "exchange_rate" "OUSD"

# Check and create automatic invitations
./build/bin/bitcoin-cli -regtest checkandcreateinvitations

# Monitor measurement targets
./build/bin/bitcoin-cli -regtest monitormeasurementtargets
```

---

## 🔧 **Advanced Testing: Multi-Node Setup**

### **Setup Multiple Nodes:**

Create a test network with multiple nodes to test consensus and p2p features.

**Node 1 (Port 18444):**
```bash
./build/bin/bitcoind -regtest -daemon \
  -port=18444 \
  -rpcport=18443 \
  -datadir=/tmp/o-node1
```

**Node 2 (Port 18445):**
```bash
./build/bin/bitcoind -regtest -daemon \
  -port=18445 \
  -rpcport=18446 \
  -datadir=/tmp/o-node2 \
  -connect=127.0.0.1:18444
```

**Node 3 (Port 18447):**
```bash
./build/bin/bitcoind -regtest -daemon \
  -port=18447 \
  -rpcport=18448 \
  -datadir=/tmp/o-node3 \
  -connect=127.0.0.1:18444
```

### **Test Consensus:**

```bash
# Mine on node 1
./build/bin/bitcoin-cli -regtest -rpcport=18443 generatetoaddress 1 $ADDRESS

# Check all nodes have same block
./build/bin/bitcoin-cli -regtest -rpcport=18443 getblockcount
./build/bin/bitcoin-cli -regtest -rpcport=18446 getblockcount
./build/bin/bitcoin-cli -regtest -rpcport=18448 getblockcount
# All should show same height
```

---

## 📊 **Testing Scenarios**

### **Scenario 1: Test Water Price Measurement Flow**

```bash
# Step 1: Create invitations
./build/bin/bitcoin-cli -regtest createinvites 5 "water_price" "USD"

# Step 2: Submit measurements (using invite IDs from step 1)
# Submit 5 different water price measurements with different volumes
./build/bin/bitcoin-cli -regtest submitwaterprice '{...}'

# Step 3: Validate measurements (simulate peer validation)
./build/bin/bitcoin-cli -regtest validatemeasurement MEASUREMENT_ID

# Step 4: Calculate daily average
./build/bin/bitcoin-cli -regtest getaveragewaterprice "USD" 7

# Step 5: Check if average is calculated correctly
./build/bin/bitcoin-cli -regtest getdailyaveragewaterprice "USD"
```

### **Scenario 2: Test Stabilization Mining**

```bash
# Step 1: Initialize exchange rates
./build/bin/bitcoin-cli -regtest initializeexchangerate "OUSD" "USD"

# Step 2: Submit multiple exchange rate measurements
# Simulate unstable rate (e.g., 1.5:1 instead of 1.2:1)
for i in {1..10}; do
  ./build/bin/bitcoin-cli -regtest submitexchangerate '{
    "from_currency": "OUSD",
    "to_currency": "USD", 
    "exchange_rate": 1.5,
    ...
  }'
done

# Step 3: Recalculate stability
./build/bin/bitcoin-cli -regtest recalculatecurrencystability 1000

# Step 4: Check if OUSD is marked unstable
./build/bin/bitcoin-cli -regtest getstabilitystatus "OUSD"
# Expected: status = "UNSTABLE"

# Step 5: Mine a block and check stabilization coins
./build/bin/bitcoin-cli -regtest generatetoaddress 1 $ADDRESS

# Step 6: Check stabilization history
./build/bin/bitcoin-cli -regtest getstabilizationhistory "OUSD" 0 1000

# Step 7: Verify coins were distributed
./build/bin/bitcoin-cli -regtest getstabilizationstats
```

### **Scenario 3: Test Volume Conversion**

```bash
# Test 1: Metric system (1.0L)
# Price: 120 cents, Volume: 1.0L
# Expected price/L: 120 cents/L

# Test 2: Metric system (950mL)
# Price: 114 cents, Volume: 950mL = 0.95L
# Expected price/L: 114 / 0.95 = 120 cents/L

# Test 3: Imperial system (33.8 fl oz)
# Price: 100 cents, Volume: 33.8 fl oz ≈ 1.0L
# Expected price/L: 100 cents/L

# Test 4: Invalid volume (500mL = 0.5L)
# Expected: REJECTED (below 0.9L minimum)

# Test 5: Invalid volume (2.0L)
# Expected: REJECTED (above 1.1L maximum)
```

### **Scenario 4: Test Blocked Region Measurements**

```bash
# Test external user measuring blocked region
# User from Switzerland measuring China water prices

# Should work:
# - User country: CH (allowed)
# - Target region: CN (blocked)
# - User type: External
# - Expected: Measurement accepted, full rewards

# Should fail:
# - User country: CN (blocked)
# - Target region: CN (blocked)
# - User type: Local
# - Expected: Measurement rejected (legal protection)
```

---

## 🔍 **Useful Testing Commands**

### **Blockchain Status:**
```bash
# Get blockchain info
./build/bin/bitcoin-cli -regtest getblockchaininfo

# Get network info
./build/bin/bitcoin-cli -regtest getnetworkinfo

# Get peer info (for multi-node testing)
./build/bin/bitcoin-cli -regtest getpeerinfo

# Get block count
./build/bin/bitcoin-cli -regtest getblockcount

# Get block by height
./build/bin/bitcoin-cli -regtest getblockhash 100
./build/bin/bitcoin-cli -regtest getblock BLOCKHASH
```

### **Wallet Operations:**
```bash
# Create new address
./build/bin/bitcoin-cli -regtest getnewaddress

# Get balance
./build/bin/bitcoin-cli -regtest getbalance

# List transactions
./build/bin/bitcoin-cli -regtest listtransactions

# Send transaction
./build/bin/bitcoin-cli -regtest sendtoaddress ADDRESS AMOUNT
```

### **O Blockchain Specific:**
```bash
# Measurement System
./build/bin/bitcoin-cli -regtest getmeasurementstatistics
./build/bin/bitcoin-cli -regtest getaveragewaterprice CURRENCY DAYS
./build/bin/bitcoin-cli -regtest checkmeasurementreadiness TYPE CURRENCY

# Stabilization System
./build/bin/bitcoin-cli -regtest getstabilitystatus CURRENCY
./build/bin/bitcoin-cli -regtest getunstablecurrencies
./build/bin/bitcoin-cli -regtest getstabilestcurrencies
./build/bin/bitcoin-cli -regtest getstabilizationstats

# User Verification
./build/bin/bitcoin-cli -regtest getrecommendedverificationmethod COUNTRY
./build/bin/bitcoin-cli -regtest doescountryrequirekyc COUNTRY

# Exchange Rate Initialization
./build/bin/bitcoin-cli -regtest initializeexchangerate OCURRENCY FIAT
./build/bin/bitcoin-cli -regtest getexchangeratestatus OCURRENCY
```

---

## 🐛 **Debugging and Logs**

### **View Logs:**
```bash
# Real-time log watching
tail -f ~/Library/Application\ Support/Bitcoin/regtest/debug.log

# Filter for O Blockchain specific logs
tail -f ~/Library/Application\ Support/Bitcoin/regtest/debug.log | grep "O Measurement"
tail -f ~/Library/Application\ Support/Bitcoin/regtest/debug.log | grep "O Stabilization"
tail -f ~/Library/Application\ Support/Bitcoin/regtest/debug.log | grep "O Volume"
```

### **Debug Mode:**
```bash
# Start with debug logging enabled
./build/bin/bitcoind -regtest -daemon -debug=all

# Or specific debug categories
./build/bin/bitcoind -regtest -daemon -debug=net -debug=mempool
```

---

## 🧹 **Cleanup and Reset**

### **Stop Daemon:**
```bash
./build/bin/bitcoin-cli -regtest stop
```

### **Reset Testnet Data (Fresh Start):**
```bash
# Stop daemon
./build/bin/bitcoin-cli -regtest stop

# Wait for shutdown
sleep 3

# Remove regtest data directory
rm -rf ~/Library/Application\ Support/Bitcoin/regtest/

# Start fresh
./build/bin/bitcoind -regtest -daemon
```

### **Kill Stuck Processes:**
```bash
# Find bitcoind processes
ps aux | grep bitcoind

# Kill all bitcoind
pkill -f bitcoind

# Or kill specific PID
kill -9 PID
```

---

## 🔬 **Testing Configuration Options**

### **Custom Configuration File:**

Create `~/Library/Application Support/Bitcoin/bitcoin.conf`:

```ini
# O Blockchain Regtest Configuration

[regtest]
# Network
port=18444
rpcport=18443
rpcuser=youruser
rpcpassword=yourpassword

# Logging
debug=net
debug=mempool
debuglogfile=/tmp/o-blockchain-debug.log

# Mining
blockminsize=0
blockmaxsize=750000

# Performance
maxconnections=125
maxmempool=300

# O Blockchain Specific
# Add any custom O Blockchain settings here
```

### **Command Line Options:**
```bash
./build/bin/bitcoind -regtest -daemon \
  -port=18444 \
  -rpcport=18443 \
  -rpcuser=test \
  -rpcpassword=test \
  -debug=all \
  -printtoconsole
```

---

## 📊 **Testing Checklist**

### **Basic Functionality:**
- [ ] Daemon starts successfully
- [ ] Can mine blocks
- [ ] Can create addresses
- [ ] Can send transactions
- [ ] Blocks propagate between nodes

### **O Blockchain Features:**

**Measurement System:**
- [ ] Can create water price measurement invites
- [ ] Can submit water price measurements (online)
- [ ] Can submit water price measurements (offline)
- [ ] Volume validation works (0.9L-1.1L)
- [ ] Volume validation works (30-37 fl oz)
- [ ] Volume conversion to liters works correctly
- [ ] Invalid volumes are rejected
- [ ] Can validate measurements (peer validation)
- [ ] Daily averages are calculated
- [ ] Can create exchange rate measurement invites
- [ ] Can submit exchange rate measurements
- [ ] Exchange rate validation works

**Stabilization Mining:**
- [ ] Currency stability is detected correctly
- [ ] Unstable currencies are identified
- [ ] Stabilization coins are calculated correctly
- [ ] Recipient selection works (stable-region users)
- [ ] Stabilization coins are created in blocks
- [ ] Stabilization coins are distributed correctly
- [ ] Stabilization history is tracked

**Security:**
- [ ] Invite ID theft prevention works
- [ ] Submitter must match invited user
- [ ] Cannot use expired invites
- [ ] Cannot use already-used invites
- [ ] Volume range validation prevents manipulation

**Geographic Access:**
- [ ] KYC required for USA users
- [ ] Privacy methods allowed for crypto-friendly countries
- [ ] Blocked regions cannot receive local invites
- [ ] External users can measure blocked regions

---

## 🎯 **Performance Testing**

### **Stress Test: Rapid Block Generation**
```bash
# Generate 1000 blocks quickly
for i in {1..10}; do
  ./build/bin/bitcoin-cli -regtest generatetoaddress 100 $ADDRESS
done

# Check performance
./build/bin/bitcoin-cli -regtest getblockchaininfo
```

### **Load Test: Multiple Measurements**
```bash
# Simulate high measurement volume
# Create 100 invites
./build/bin/bitcoin-cli -regtest createinvites 100 "water_price" "USD"

# Submit multiple measurements (would need scripting)
# Test system can handle high volume
```

---

## 💡 **Pro Tips**

### **Fast Testing:**
```bash
# Alias for convenience
alias o-cli="./build/bin/bitcoin-cli -regtest"
alias o-daemon="./build/bin/bitcoind -regtest -daemon"

# Now you can use:
o-cli getblockchaininfo
o-cli generatetoaddress 10 $ADDRESS
```

### **Data Directory Management:**
```bash
# Use custom data directory for testing
./build/bin/bitcoind -regtest -daemon -datadir=/tmp/o-test-1

# Allows multiple independent test environments
./build/bin/bitcoind -regtest -daemon -datadir=/tmp/o-test-2
./build/bin/bitcoind -regtest -daemon -datadir=/tmp/o-test-3
```

### **Quick Reset Script:**
```bash
#!/bin/bash
# save as reset-testnet.sh

echo "Stopping O Blockchain..."
./build/bin/bitcoin-cli -regtest stop
sleep 3

echo "Cleaning data..."
rm -rf ~/Library/Application\ Support/Bitcoin/regtest/

echo "Starting fresh..."
./build/bin/bitcoind -regtest -daemon
sleep 3

echo "Generating initial blocks..."
ADDRESS=$(./build/bin/bitcoin-cli -regtest getnewaddress)
./build/bin/bitcoin-cli -regtest generatetoaddress 101 $ADDRESS

echo "Ready for testing!"
./build/bin/bitcoin-cli -regtest getblockchaininfo
```

---

## 🔍 **Troubleshooting**

### **Problem: "Cannot obtain lock on directory"**
```bash
# Another instance is running
./build/bin/bitcoin-cli -regtest stop
# Or kill it
pkill -f bitcoind
```

### **Problem: "Connection refused"**
```bash
# Daemon not started or still starting
./build/bin/bitcoind -regtest -daemon
sleep 5  # Wait longer for startup
```

### **Problem: "Method not found"**
```bash
# RPC command may not be registered or daemon needs restart
./build/bin/bitcoin-cli -regtest stop
./build/bin/bitcoind -regtest -daemon
```

### **Problem: "Insufficient funds"**
```bash
# Need to mine blocks first
ADDRESS=$(./build/bin/bitcoin-cli -regtest getnewaddress)
./build/bin/bitcoin-cli -regtest generatetoaddress 101 $ADDRESS
```

---

## 🎉 **Ready to Test!**

The O Blockchain is now ready for comprehensive testing on regtest. You can:

1. ✅ Test all measurement features
2. ✅ Test volume validation and conversion
3. ✅ Test stabilization mining
4. ✅ Test currency stability detection
5. ✅ Test multi-node consensus
6. ✅ Test KYC integration
7. ✅ Test blocked region policies

**Start testing with:**
```bash
cd /Users/o/bitcoin
./build/bin/bitcoind -regtest -daemon
sleep 3
./build/bin/bitcoin-cli -regtest getblockchaininfo
```

Happy testing! 🚀

