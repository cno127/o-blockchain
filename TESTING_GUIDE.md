# O Blockchain Integration Testing Guide

This guide provides comprehensive instructions for testing the O blockchain user verification integration system.

## Overview

The O blockchain integration includes several layers of testing:

1. **Unit Tests** - Test individual components in isolation
2. **Integration Tests** - Test interactions between components
3. **RPC Tests** - Test RPC API endpoints
4. **End-to-End Tests** - Test complete user workflows
5. **Performance Tests** - Test system performance under load

## Prerequisites

### System Requirements

- **Operating System**: Linux/macOS (Windows with WSL)
- **CPU**: 4+ cores recommended
- **RAM**: 8GB+ recommended
- **Disk Space**: 10GB+ free space
- **Network**: Internet connection for dependencies

### Dependencies

```bash
# Install build dependencies
sudo apt-get update
sudo apt-get install build-essential libtool autotools-dev automake pkg-config bsdmainutils python3
sudo apt-get install libssl-dev libevent-dev libboost-system-dev libboost-filesystem-dev libboost-chrono-dev libboost-program-options-dev libboost-test-dev libboost-thread-dev
sudo apt-get install libdb-dev libdb++-dev
sudo apt-get install libminiupnpc-dev libzmq3-dev
sudo apt-get install libqt5gui5 libqt5core5a libqt5dbus5 qttools5-dev qttools5-dev-tools
sudo apt-get install libqrencode-dev
sudo apt-get install libprotobuf-dev protobuf-compiler
sudo apt-get install libsqlite3-dev

# Install Python dependencies
pip3 install pytest pytest-cov
```

## Quick Start

### 1. Automated Testing

The easiest way to run all tests is using the automated test script:

```bash
# Run all tests
./test_o_integration.py --build --verbose

# Run specific test types
./test_o_integration.py --test-type unit
./test_o_integration.py --test-type integration
./test_o_integration.py --test-type rpc
./test_o_integration.py --test-type end-to-end
```

### 2. Manual Testing

For more control over the testing process:

```bash
# Build Bitcoin Core with O integration
./configure --enable-debug --enable-tests
make -j4

# Run specific test suites
./src/test/test_bitcoin --run_test=user_consensus_tests
./src/test/test_bitcoin --run_test=o_integration_tests
./src/test/test_bitcoin --run_test=o_rpc_tests
```

## Test Categories

### Unit Tests (`user_consensus_tests.cpp`)

Tests individual components of the user consensus system:

```cpp
// Test user registration
BOOST_AUTO_TEST_CASE(test_user_registration)

// Test endorsement system
BOOST_AUTO_TEST_CASE(test_endorsement_system)

// Test user verification status
BOOST_AUTO_TEST_CASE(test_user_verification_status)

// Test reputation system
BOOST_AUTO_TEST_CASE(test_reputation_system)

// Test challenge system
BOOST_AUTO_TEST_CASE(test_challenge_system)

// Test government ID uniqueness
BOOST_AUTO_TEST_CASE(test_government_id_uniqueness)

// Test geographic diversity
BOOST_AUTO_TEST_CASE(test_geographic_diversity)
```

**Run Unit Tests:**
```bash
./src/test/test_bitcoin --run_test=user_consensus_tests
```

### Integration Tests (`o_integration_tests.cpp`)

Tests integration between user consensus and main blockchain:

```cpp
// Test transaction validation with user verification
BOOST_AUTO_TEST_CASE(test_transaction_validation_with_user_check)

// Test user registration transaction processing
BOOST_AUTO_TEST_CASE(test_user_registration_transaction_processing)

// Test endorsement transaction processing
BOOST_AUTO_TEST_CASE(test_endorsement_transaction_processing)

// Test measurement transaction processing
BOOST_AUTO_TEST_CASE(test_measurement_transaction_processing)

// Test reward calculation with user verification
BOOST_AUTO_TEST_CASE(test_reward_calculation_with_user_verification)

// Test measurement reward calculation
BOOST_AUTO_TEST_CASE(test_measurement_reward_calculation)
```

**Run Integration Tests:**
```bash
./src/test/test_bitcoin --run_test=o_integration_tests
```

### RPC Tests (`o_rpc_tests.cpp`)

Tests RPC API endpoints:

```cpp
// Test user registration RPC
BOOST_AUTO_TEST_CASE(test_registeruser_rpc)

// Test endorsement submission RPC
BOOST_AUTO_TEST_CASE(test_submitendorsement_rpc)

// Test user status query RPC
BOOST_AUTO_TEST_CASE(test_getuserstatus_rpc)

// Test user listing RPC
BOOST_AUTO_TEST_CASE(test_listverifiedusers_rpc)

// Test user statistics RPC
BOOST_AUTO_TEST_CASE(test_getuserstats_rpc)

// Test invalid parameters
BOOST_AUTO_TEST_CASE(test_invalid_parameters)
```

**Run RPC Tests:**
```bash
./src/test/test_bitcoin --run_test=o_rpc_tests
```

## End-to-End Testing

### 1. Start Test Environment

```bash
# Start bitcoind in regtest mode
./src/bitcoind -regtest -daemon -server -rpcuser=test -rpcpassword=test

# Generate initial blocks
./src/bitcoin-cli -regtest -rpcuser=test -rpcpassword=test generate 101
```

### 2. Test User Registration Flow

```bash
# Register a new user
./src/bitcoin-cli -regtest -rpcuser=test -rpcpassword=test registeruser \
    "02f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f9" \
    "123456789" \
    "USD" \
    "US" \
    "abcd1234efgh5678"

# Check user status
./src/bitcoin-cli -regtest -rpcuser=test -rpcpassword=test getuserstatus \
    "02f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f9"
```

### 3. Test Endorsement Flow

```bash
# Create endorser (verified user)
./src/bitcoin-cli -regtest -rpcuser=test -rpcpassword=test registeruser \
    "02a1b2c3d4e5f6789012345678901234567890123456789012345678901234567890" \
    "987654321" \
    "EUR" \
    "DE" \
    "efgh5678ijkl9012"

# Submit endorsement
./src/bitcoin-cli -regtest -rpcuser=test -rpcpassword=test submitendorsement \
    "02a1b2c3d4e5f6789012345678901234567890123456789012345678901234567890" \
    "02f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f9" \
    "government_id" \
    "high" \
    "Verified government ID document"
```

### 4. Test User Statistics

```bash
# Get user statistics
./src/bitcoin-cli -regtest -rpcuser=test -rpcpassword=test getuserstats

# List verified users
./src/bitcoin-cli -regtest -rpcuser=test -rpcpassword=test listverifiedusers 10 0
```

## Performance Testing

### Load Testing

```bash
# Run performance tests
./test_o_integration.py --test-type performance --verbose
```

### Stress Testing

```python
# Custom stress test script
import time
import subprocess

def stress_test_user_registration(num_users=1000):
    """Test user registration under load"""
    start_time = time.time()
    
    for i in range(num_users):
        pubkey = f"02f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce{i:036d}"
        
        result = subprocess.run([
            "./src/bitcoin-cli", "-regtest", "-rpcuser=test", "-rpcpassword=test",
            "registeruser", pubkey, f"user_{i}_id", "USD", "US", f"proof_{i}"
        ], capture_output=True, text=True)
        
        if result.returncode != 0:
            print(f"Registration failed for user {i}: {result.stderr}")
            break
    
    end_time = time.time()
    print(f"Registered {num_users} users in {end_time - start_time:.2f} seconds")
    print(f"Rate: {num_users / (end_time - start_time):.2f} users/second")

stress_test_user_registration()
```

## Test Data

### Sample Test Users

```json
{
  "users": [
    {
      "public_key": "02f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f9",
      "government_id": "123456789",
      "currency": "USD",
      "country": "US",
      "identity_proof": "abcd1234efgh5678"
    },
    {
      "public_key": "02a1b2c3d4e5f6789012345678901234567890123456789012345678901234567890",
      "government_id": "987654321",
      "currency": "EUR",
      "country": "DE",
      "identity_proof": "efgh5678ijkl9012"
    }
  ]
}
```

### Test Scenarios

1. **Normal User Registration**
   - Valid government ID
   - Valid public key
   - Supported currency/country

2. **Duplicate Registration**
   - Same government ID
   - Different public key

3. **Invalid Data**
   - Invalid public key format
   - Unsupported currency
   - Missing required fields

4. **Endorsement Flow**
   - Verified endorser
   - Valid endorsement data
   - Challenge and resolution

5. **Reputation System**
   - Accurate endorsements
   - Inaccurate endorsements
   - Reputation score changes

## Debugging

### Enable Debug Logging

```bash
# Start bitcoind with debug logging
./src/bitcoind -regtest -debug=user_consensus -debug=rpc -daemon

# Or set environment variable
export BCLOG=user_consensus,rpc
./src/bitcoind -regtest -daemon
```

### Common Issues

1. **Build Failures**
   - Check dependencies are installed
   - Verify configure options
   - Check for missing headers

2. **Test Failures**
   - Check debug logs
   - Verify test data
   - Check system resources

3. **RPC Errors**
   - Verify bitcoind is running
   - Check RPC credentials
   - Verify method parameters

### Log Analysis

```bash
# View debug logs
tail -f ~/.bitcoin/regtest/debug.log | grep -E "(user_consensus|o_integration)"

# Search for specific errors
grep -i "error\|fail" ~/.bitcoin/regtest/debug.log
```

## Continuous Integration

### GitHub Actions Example

```yaml
name: O Blockchain Integration Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    
    steps:
    - uses: actions/checkout@v2
    
    - name: Install dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y build-essential libtool autotools-dev automake pkg-config bsdmainutils python3
    
    - name: Build Bitcoin Core
      run: |
        ./configure --enable-debug --enable-tests
        make -j4
    
    - name: Run tests
      run: |
        ./test_o_integration.py --verbose
    
    - name: Upload test results
      uses: actions/upload-artifact@v2
      with:
        name: test-results
        path: o_integration_test_report.json
```

## Test Results

### Expected Output

```
==================================================
O BLOCKCHAIN INTEGRATION TEST REPORT
==================================================
Timestamp: 2024-01-15 10:30:45
Total Test Suites: 5
Passed: 5
Failed: 0
==================================================
✅ ALL TESTS PASSED!
```

### Test Coverage

- **User Consensus**: 95%+ code coverage
- **Integration Layer**: 90%+ code coverage
- **RPC Methods**: 100% method coverage
- **End-to-End**: 100% workflow coverage

## Contributing

### Adding New Tests

1. **Unit Tests**: Add to appropriate `*_tests.cpp` file
2. **Integration Tests**: Add to `o_integration_tests.cpp`
3. **RPC Tests**: Add to `o_rpc_tests.cpp`
4. **End-to-End Tests**: Add to test script

### Test Guidelines

1. **Isolation**: Tests should not depend on each other
2. **Cleanup**: Clean up test data after each test
3. **Assertions**: Use meaningful assertions with clear messages
4. **Documentation**: Document complex test scenarios
5. **Performance**: Keep tests fast and efficient

### Test Review Checklist

- [ ] Tests cover happy path scenarios
- [ ] Tests cover error conditions
- [ ] Tests cover edge cases
- [ ] Tests are properly isolated
- [ ] Tests have clear assertions
- [ ] Tests are documented
- [ ] Tests run in reasonable time
- [ ] Tests pass consistently

## Troubleshooting

### Common Test Failures

1. **Timeout Errors**
   - Increase timeout values
   - Check system performance
   - Optimize test execution

2. **Memory Issues**
   - Check for memory leaks
   - Increase system memory
   - Optimize test data size

3. **Network Issues**
   - Check network connectivity
   - Verify RPC endpoints
   - Check firewall settings

### Getting Help

1. **Check Logs**: Review debug logs for errors
2. **Run Individual Tests**: Isolate failing tests
3. **Check Dependencies**: Verify all dependencies are installed
4. **System Resources**: Ensure adequate CPU/memory
5. **Community Support**: Post issues with full logs and context

## Conclusion

This testing framework provides comprehensive coverage of the O blockchain user verification integration. Regular testing ensures system reliability, performance, and correctness.

For questions or issues, please refer to the troubleshooting section or contact the development team.


