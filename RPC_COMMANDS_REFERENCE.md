# 🌊 O Blockchain - Complete RPC Commands Reference

This document provides a comprehensive reference for all RPC commands available in the O Blockchain system.

## 📋 **Command Categories**

1. [PoW/PoB System](#powpob-system)
2. [Measurement System](#measurement-system)
3. [Stabilization System](#stabilization-system)
4. [Currency Exchange](#currency-exchange)
5. [Exchange Rate Management](#exchange-rate-management)
6. [Measurement Readiness](#measurement-readiness)
7. [Measurement Rewards](#measurement-rewards)
8. [User Management](#user-management)
9. [Standard Bitcoin Commands](#standard-bitcoin-commands)

---

## ⚡ **PoW/PoB System**

### **Business Mining Commands**
```bash
# Get current business ratio
bitcoin-cli getbusinessratio

# Get business miner statistics
bitcoin-cli getbusinessminerstats

# List qualified business miners
bitcoin-cli listqualifiedbusinessminers

# Get business mining configuration
bitcoin-cli getbusinessminingconfig

# Update business mining parameters
bitcoin-cli updatebusinessminingparams <min_volume> <min_transactions> <min_uptime>
```

### **Mining Statistics**
```bash
# Get mining statistics
bitcoin-cli getminingstats

# Get block statistics
bitcoin-cli getblockstats

# Get network statistics
bitcoin-cli getnetworkstats
```

---

## 📊 **Measurement System**

### **Basic Measurement Commands**
```bash
# Submit water price measurement
bitcoin-cli submitwaterprice <currency> <price> <location> <source_url>

# Submit exchange rate measurement
bitcoin-cli submitexchangerate <o_currency> <fiat_currency> <rate> <location> <source_url>

# Validate measurement
bitcoin-cli validatemeasurement <measurement_id> <is_valid> <validator_notes>

# Get average water price
bitcoin-cli getaveragewaterprice <currency> <days>

# Get average exchange rate
bitcoin-cli getaverageexchangerate <o_currency> <fiat_currency> <days>

# Get measurement statistics
bitcoin-cli getmeasurementstatistics
```

### **Advanced Measurement Commands**
```bash
# Get average water price with confidence
bitcoin-cli getaveragewaterpricewithconfidence <currency> <days>

# Get average exchange rate with confidence
bitcoin-cli getaverageexchangeratewithconfidence <o_currency> <fiat_currency> <days>

# Get daily average with confidence
bitcoin-cli getdailyaveragewithconfidence <currency> <date>

# Get dynamic measurement target
bitcoin-cli getdynamicmeasurementtarget <type> <currency>

# Get measurement volatility
bitcoin-cli getmeasurementvolatility <type> <currency> <days>

# Get measurement target statistics
bitcoin-cli getmeasurementtargetstatistics
```

### **Automated Validation Commands**
```bash
# Submit water price with validation
bitcoin-cli submitwaterpricewithvalidation <currency> <price> <source_type> [source_url] [location] [image_hash] [timestamp]

# Submit exchange rate with validation
bitcoin-cli submitexchangeratewithvalidation <o_currency> <fiat_currency> <rate> <source_type> [source_url] [location] [image_hash] [timestamp]

# Get Gaussian range
bitcoin-cli getgaussianrange <type> <currency>
```

### **Automatic Invitation Triggers**
```bash
# Check and create invitations automatically
bitcoin-cli checkandcreateinvitations

# Monitor measurement targets
bitcoin-cli monitormeasurementtargets

# Get measurement gap
bitcoin-cli getmeasurementgap <type> <currency>
```

### **Invitation Management**
```bash
# Create measurement invitations
bitcoin-cli createinvites <currency> <count> [type] [target_measurements]

# Get invitation statistics
bitcoin-cli getinvitationstatistics

# Get pending invitations
bitcoin-cli getpendinginvitations

# Get invitation history
bitcoin-cli getinvitationhistory <currency> <days>
```

---

## 🎯 **Stabilization System**

### **Stabilization Status**
```bash
# Get stabilization status
bitcoin-cli getstabilizationstatus

# Get stabilization history
bitcoin-cli getstabilizationhistory <days>

# Get stabilization integration status
bitcoin-cli getstabilizationintegrationstatus

# Get stabilization statistics
bitcoin-cli getstabilizationstatistics
```

### **Stabilization Management**
```bash
# Trigger stabilization manually
bitcoin-cli triggerstabilization <currency>

# Get stabilization configuration
bitcoin-cli getstabilizationconfig

# Update stabilization parameters
bitcoin-cli updatestabilizationparams <deviation_threshold> <max_recipients> <max_reward>
```

---

## 💱 **Currency Exchange**

### **Exchange Operations**
```bash
# Exchange between O currencies
bitcoin-cli exchangecurrency <from_currency> <to_currency> <amount>

# Get exchange rate
bitcoin-cli getexchangerate <from_currency> <to_currency>

# Get supported currency pairs
bitcoin-cli getsupportedpairs

# Get exchange rate history
bitcoin-cli getexchangeratehistory <from_currency> <to_currency> <days>
```

### **Exchange Management**
```bash
# Get exchange statistics
bitcoin-cli getexchangestatistics

# Get exchange configuration
bitcoin-cli getexchangeconfig

# Update exchange parameters
bitcoin-cli updateexchangeparams <max_deviation> <min_volume> <fee_rate>
```

---

## 📈 **Exchange Rate Management**

### **Exchange Rate Operations**
```bash
# Initialize exchange rates
bitcoin-cli initializeexchangerates

# Get exchange rate status
bitcoin-cli getexchangeratestatus <o_currency> <fiat_currency>

# Detect currency disappearance
bitcoin-cli detectcurrencydisappearance <o_currency> <fiat_currency>

# Get measurement trend
bitcoin-cli getmeasurementtrend <o_currency> <fiat_currency>

# Get exchange rate statistics
bitcoin-cli getexchangeratestatistics
```

### **Exchange Rate Configuration**
```bash
# Get exchange rate configuration
bitcoin-cli getexchangerateconfig

# Update exchange rate parameters
bitcoin-cli updateexchangerateparams <min_measurements> <trend_threshold> <disappearance_threshold>
```

---

## 🚀 **Measurement Readiness**

### **Readiness Status**
```bash
# Get readiness status
bitcoin-cli getreadinessstatus <currency> <type>

# Get readiness statistics
bitcoin-cli getreadinessstatistics

# Get detailed readiness status
bitcoin-cli getdetailedreadinessstatus <currency> <type>
```

### **Readiness Management**
```bash
# Update user count
bitcoin-cli updateusercount <currency> <count>

# Update coin supply
bitcoin-cli updatecoinsupply <currency> <supply>

# Get readiness configuration
bitcoin-cli getreadinessconfig
```

---

## 🎁 **Measurement Rewards**

### **Reward Management**
```bash
# Get measurement rewards status
bitcoin-cli getmeasurementrewardsstatus

# Get measurement rewards history
bitcoin-cli getmeasurementrewardshistory <days>

# Get measurement rewards statistics
bitcoin-cli getmeasurementrewardsstatistics

# Get pending measurement rewards
bitcoin-cli getpendingmeasurementrewards
```

### **Reward Configuration**
```bash
# Get measurement rewards configuration
bitcoin-cli getmeasurementrewardsconfig

# Update measurement rewards parameters
bitcoin-cli updatemeasurementrewardsparams <initial_reward> <validation_reward> <offline_bonus>
```

---

## 👥 **User Management**

### **User Operations**
```bash
# Register user
bitcoin-cli registeruser <public_key> <country> <currency>

# Get user information
bitcoin-cli getuserinfo <user_id>

# Update user information
bitcoin-cli updateuserinfo <user_id> <field> <value>

# Get user statistics
bitcoin-cli getuserstatistics
```

### **User Validation**
```bash
# Validate user
bitcoin-cli validateuser <user_id> <is_valid> <validator_notes>

# Get user validation history
bitcoin-cli getuservalidationhistory <user_id>

# Get pending user validations
bitcoin-cli getpendinguservalidations
```

---

## 🔧 **Standard Bitcoin Commands**

### **Blockchain Information**
```bash
# Get blockchain info
bitcoin-cli getblockchaininfo

# Get block count
bitcoin-cli getblockcount

# Get block hash
bitcoin-cli getblockhash <height>

# Get block
bitcoin-cli getblock <blockhash>
```

### **Network Information**
```bash
# Get network info
bitcoin-cli getnetworkinfo

# Get peer info
bitcoin-cli getpeerinfo

# Get connection count
bitcoin-cli getconnectioncount
```

### **Wallet Operations**
```bash
# Get wallet info
bitcoin-cli getwalletinfo

# Get balance
bitcoin-cli getbalance

# Send transaction
bitcoin-cli sendtoaddress <address> <amount>

# Get transaction
bitcoin-cli gettransaction <txid>
```

---

## 📊 **Command Usage Examples**

### **Complete Workflow Example**
```bash
# 1. Initialize exchange rates
bitcoin-cli initializeexchangerates

# 2. Check readiness status
bitcoin-cli getreadinessstatus "USD" "water"

# 3. Create invitations if ready
bitcoin-cli createinvites "USD" 10

# 4. Submit measurements
bitcoin-cli submitwaterpricewithvalidation "USD" 1.50 "user_online" "https://example.com"

# 5. Validate measurements
bitcoin-cli validatemeasurement <measurement_id> true "Valid measurement"

# 6. Check averages
bitcoin-cli getaveragewaterpricewithconfidence "USD" 7

# 7. Monitor targets
bitcoin-cli monitormeasurementtargets

# 8. Check stabilization status
bitcoin-cli getstabilizationstatus
```

### **Monitoring and Maintenance**
```bash
# Daily monitoring
bitcoin-cli getmeasurementstatistics
bitcoin-cli getstabilizationstatus
bitcoin-cli getexchangeratestatistics

# Weekly analysis
bitcoin-cli getmeasurementrewardsstatistics
bitcoin-cli getreadinessstatistics
bitcoin-cli getexchangeratehistory "OUSD" "USD" 7

# Monthly review
bitcoin-cli getstabilizationhistory 30
bitcoin-cli getmeasurementrewardshistory 30
bitcoin-cli getexchangeratestatistics
```

---

## 🔍 **Command Parameters**

### **Common Parameters**
- **`<currency>`** - Currency code (e.g., "USD", "EUR", "OUSD", "OEUR")
- **`<type>`** - Measurement type ("water" or "exchange")
- **`<days>`** - Number of days to look back (integer)
- **`<amount>`** - Amount in O coins (decimal, e.g., 100.00)
- **`<count>`** - Number of items (integer)
- **`<height>`** - Block height (integer)
- **`<txid>`** - Transaction ID (hex string)

### **Measurement Types**
- **`water`** - Water price measurements
- **`exchange`** - Exchange rate measurements

### **Source Types**
- **`user_online`** - User provides online data with URL
- **`user_offline`** - User provides offline data with image and location
- **`bot_online`** - Bot scrapes online data
- **`bot_offline`** - Bot processes offline data

---

## 📝 **Notes**

- All commands return JSON responses
- Use `bitcoin-cli help <command>` for detailed help on specific commands
- Commands are case-sensitive
- Parameters in `[]` are optional
- Parameters in `<>` are required
- All amounts are in O coins with 2 decimal places
- All timestamps are Unix timestamps
- All currency codes are uppercase (e.g., "USD", "OUSD")

---

**For more detailed information about each system, see [O_BLOCKCHAIN_COMPLETE_SYSTEM.md](O_BLOCKCHAIN_COMPLETE_SYSTEM.md)**
