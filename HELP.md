# 🌊 O Blockchain - Help & Support

## 🚀 **Quick Start Guide**

### **1. Building the O Blockchain**
```bash
# Clone the repository
git clone <repository-url>
cd bitcoin

# Build the system
mkdir build && cd build
cmake ..
make -j$(nproc)

# Run a single node
cd ..
./src/bitcoind -regtest -daemon
```

### **2. Running Multi-Node Simulation**
```bash
# Setup simulation environment
cd simulation
./setup_simulation.sh

# Start 5 nodes
./start_nodes.sh

# Connect nodes
./connect_nodes.sh

# Start mining
./start_mining.sh
```

### **3. Basic Operations**
```bash
# Check system status
bitcoin-cli getblockchaininfo
bitcoin-cli getmeasurementstatistics
bitcoin-cli getstabilizationstatus

# Initialize exchange rates
bitcoin-cli initializeexchangerates

# Create measurement invitations
bitcoin-cli createinvites "USD" 10
```

---

## 📚 **Documentation Structure**

### **Main Documentation Files**
- **[README.md](README.md)** - Project overview and quick start
- **[O_BLOCKCHAIN_COMPLETE_SYSTEM.md](O_BLOCKCHAIN_COMPLETE_SYSTEM.md)** - Complete system documentation
- **[RPC_COMMANDS_REFERENCE.md](RPC_COMMANDS_REFERENCE.md)** - All RPC commands reference
- **[HELP.md](HELP.md)** - This help file

### **Simulation & Testing**
- **[simulation/README.md](simulation/README.md)** - Multi-node simulation guide
- **[simulation/BUILD_AND_TEST_RESULTS.md](simulation/BUILD_AND_TEST_RESULTS.md)** - Build and test results
- **[simulation/COMPREHENSIVE_TEST_RESULTS.md](simulation/COMPREHENSIVE_TEST_RESULTS.md)** - Comprehensive test results

---

## 🔧 **Common Tasks**

### **System Monitoring**
```bash
# Check overall system health
bitcoin-cli getblockchaininfo
bitcoin-cli getmeasurementstatistics
bitcoin-cli getstabilizationstatus
bitcoin-cli getexchangeratestatistics

# Monitor measurement targets
bitcoin-cli monitormeasurementtargets
bitcoin-cli getmeasurementtargetstatistics

# Check readiness status
bitcoin-cli getreadinessstatistics
```

### **Measurement Management**
```bash
# Submit measurements
bitcoin-cli submitwaterpricewithvalidation "USD" 1.50 "user_online" "https://example.com"
bitcoin-cli submitexchangeratewithvalidation "OUSD" "USD" 1.0 "user_offline" "" "New York, USA"

# Validate measurements
bitcoin-cli validatemeasurement <measurement_id> true "Valid measurement"

# Check averages
bitcoin-cli getaveragewaterpricewithconfidence "USD" 7
bitcoin-cli getaverageexchangeratewithconfidence "OUSD" "USD" 7
```

### **Invitation Management**
```bash
# Create invitations
bitcoin-cli createinvites "USD" 10

# Check automatic invitations
bitcoin-cli checkandcreateinvitations

# Get invitation statistics
bitcoin-cli getinvitationstatistics
```

### **Stabilization Monitoring**
```bash
# Check stabilization status
bitcoin-cli getstabilizationstatus

# Get stabilization history
bitcoin-cli getstabilizationhistory 7

# Check integration status
bitcoin-cli getstabilizationintegrationstatus
```

---

## 🆘 **Troubleshooting**

### **Common Issues**

#### **Build Issues**
```bash
# Clean build
rm -rf build
mkdir build && cd build
cmake ..
make -j$(nproc)
```

#### **Node Connection Issues**
```bash
# Check node status
bitcoin-cli getpeerinfo
bitcoin-cli getconnectioncount

# Restart nodes
cd simulation
./stop_nodes.sh
./start_nodes.sh
./connect_nodes.sh
```

#### **Measurement Issues**
```bash
# Check measurement statistics
bitcoin-cli getmeasurementstatistics

# Check readiness status
bitcoin-cli getreadinessstatus "USD" "water"

# Check measurement gaps
bitcoin-cli getmeasurementgap "water" "USD"
```

#### **Stabilization Issues**
```bash
# Check stabilization status
bitcoin-cli getstabilizationstatus

# Check exchange rate status
bitcoin-cli getexchangeratestatus "OUSD" "USD"

# Check currency disappearance
bitcoin-cli detectcurrencydisappearance "OUSD" "USD"
```

### **Log Files**
```bash
# Check main log
tail -f ~/.bitcoin/regtest/debug.log

# Check simulation logs
tail -f simulation/logs/node_1.log
tail -f simulation/logs/node_2.log
```

---

## 📊 **System Status Commands**

### **Health Check Commands**
```bash
# Overall system health
bitcoin-cli getblockchaininfo
bitcoin-cli getnetworkinfo
bitcoin-cli getwalletinfo

# Measurement system health
bitcoin-cli getmeasurementstatistics
bitcoin-cli getmeasurementtargetstatistics
bitcoin-cli getinvitationstatistics

# Stabilization system health
bitcoin-cli getstabilizationstatus
bitcoin-cli getstabilizationintegrationstatus

# Exchange rate system health
bitcoin-cli getexchangeratestatistics
bitcoin-cli getexchangerateconfig
```

### **Performance Monitoring**
```bash
# Mining performance
bitcoin-cli getminingstats
bitcoin-cli getbusinessminerstats

# Network performance
bitcoin-cli getpeerinfo
bitcoin-cli getconnectioncount

# Measurement performance
bitcoin-cli getmeasurementrewardsstatistics
bitcoin-cli getmeasurementvolatility "water" "USD" 7
```

---

## 🔍 **Debugging Commands**

### **Detailed Information**
```bash
# Get detailed measurement information
bitcoin-cli getaveragewaterpricewithconfidence "USD" 30
bitcoin-cli getdynamicmeasurementtarget "water" "USD"
bitcoin-cli getgaussianrange "water" "USD"

# Get detailed exchange rate information
bitcoin-cli getexchangeratestatus "OUSD" "USD"
bitcoin-cli getmeasurementtrend "OUSD" "USD"

# Get detailed readiness information
bitcoin-cli getdetailedreadinessstatus "USD" "water"
bitcoin-cli getreadinessstatistics
```

### **Configuration Information**
```bash
# Get system configurations
bitcoin-cli getbusinessminingconfig
bitcoin-cli getstabilizationconfig
bitcoin-cli getexchangerateconfig
bitcoin-cli getreadinessconfig
bitcoin-cli getmeasurementrewardsconfig
```

---

## 📞 **Support Resources**

### **Documentation**
- **Complete System Documentation**: [O_BLOCKCHAIN_COMPLETE_SYSTEM.md](O_BLOCKCHAIN_COMPLETE_SYSTEM.md)
- **RPC Commands Reference**: [RPC_COMMANDS_REFERENCE.md](RPC_COMMANDS_REFERENCE.md)
- **Simulation Guide**: [simulation/README.md](simulation/README.md)

### **Testing & Validation**
- **Build Results**: [simulation/BUILD_AND_TEST_RESULTS.md](simulation/BUILD_AND_TEST_RESULTS.md)
- **Test Results**: [simulation/COMPREHENSIVE_TEST_RESULTS.md](simulation/COMPREHENSIVE_TEST_RESULTS.md)

### **Getting Help**
1. **Check Documentation**: Start with the complete system documentation
2. **Review RPC Commands**: Use the RPC commands reference
3. **Run Health Checks**: Use the system status commands
4. **Check Logs**: Review log files for error messages
5. **Test with Simulation**: Use the multi-node simulation environment

---

## 🎯 **Best Practices**

### **System Management**
- **Regular Monitoring**: Use health check commands daily
- **Backup Data**: Regularly backup wallet and blockchain data
- **Update Documentation**: Keep documentation current
- **Test Changes**: Use simulation environment for testing

### **Measurement Management**
- **Validate Measurements**: Always validate submitted measurements
- **Monitor Targets**: Check measurement targets regularly
- **Check Readiness**: Ensure readiness conditions are met
- **Review Statistics**: Monitor measurement statistics

### **Stabilization Management**
- **Monitor Status**: Check stabilization status regularly
- **Review History**: Analyze stabilization history
- **Check Integration**: Verify stabilization integration
- **Monitor Exchange Rates**: Watch for currency instability

---

## 🚀 **Advanced Usage**

### **Custom Configurations**
```bash
# Update business mining parameters
bitcoin-cli updatebusinessminingparams 10000 100 3600

# Update stabilization parameters
bitcoin-cli updatestabilizationparams 0.05 1000 10000

# Update exchange rate parameters
bitcoin-cli updateexchangerateparams 10 0.1 0.2
```

### **Automated Operations**
```bash
# Set up automated monitoring
# Create cron job for regular health checks
# Set up alerts for system issues
# Automate measurement validation
```

---

**For more detailed information, see the complete system documentation and RPC commands reference.**
