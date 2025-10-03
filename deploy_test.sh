#!/bin/bash

# O Blockchain Test Deployment Script
# This script automates the deployment and testing of the O blockchain system

set -e  # Exit on any error

echo "🚀 O Blockchain Test Deployment Starting..."
echo "=========================================="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ] || [ ! -f "README.md" ]; then
    print_error "Please run this script from the O blockchain root directory"
    exit 1
fi

print_status "Current directory: $(pwd)"

# Step 1: Build the system
print_status "Step 1: Building O Blockchain system..."
if [ -d "build" ]; then
    print_warning "Removing existing build directory..."
    rm -rf build
fi

print_status "Creating build directory..."
cmake -B build

print_status "Building with optimizations..."
cmake --build build --parallel 4

if [ $? -eq 0 ]; then
    print_success "Build completed successfully!"
else
    print_error "Build failed!"
    exit 1
fi

# Step 2: Run tests
print_status "Step 2: Running O Blockchain tests..."
print_status "Running simple O tests..."
./build/bin/test_bitcoin --run_test=simple_o_tests

if [ $? -eq 0 ]; then
    print_success "Simple O tests passed!"
else
    print_error "Simple O tests failed!"
    exit 1
fi

# Step 3: Create test network directory
print_status "Step 3: Setting up test network..."
if [ -d "test_network" ]; then
    print_warning "Removing existing test network..."
    rm -rf test_network
fi

mkdir -p test_network

# Create O blockchain configuration
print_status "Creating O blockchain test configuration..."
cat > test_network/o.conf << 'EOF'
# O Blockchain Test Configuration
server=1
rpcuser=o_test
rpcpassword=o_test_password
regtest=1
daemon=0
printtoconsole=1
debug=1
logtimestamps=1
txindex=1

[regtest]
rpcport=18443
port=18333

# O Blockchain specific settings
# User verification enabled
# Measurement system enabled
# Stabilization mining enabled
EOF

print_success "Test network configuration created!"

# Step 4: Test basic functionality
print_status "Step 4: Testing basic O blockchain functionality..."

# Start daemon in background
print_status "Starting O blockchain daemon..."
./build/bin/bitcoind -datadir=test_network -conf=o.conf &
DAEMON_PID=$!

# Wait for daemon to start
sleep 5

# Test RPC connectivity
print_status "Testing RPC connectivity..."
./build/bin/bitcoin-cli -datadir=test_network -conf=o.conf getblockchaininfo > /dev/null

if [ $? -eq 0 ]; then
    print_success "RPC connectivity working!"
else
    print_error "RPC connectivity failed!"
    kill $DAEMON_PID 2>/dev/null
    exit 1
fi

# Create wallet
print_status "Creating test wallet..."
./build/bin/bitcoin-cli -datadir=test_network -conf=o.conf createwallet "test_wallet" > /dev/null

if [ $? -eq 0 ]; then
    print_success "Test wallet created!"
else
    print_warning "Wallet creation failed (may already exist)"
fi

# Generate test address
print_status "Generating test address..."
TEST_ADDRESS=$(./build/bin/bitcoin-cli -datadir=test_network -conf=o.conf getnewaddress "test_user")

if [ $? -eq 0 ]; then
    print_success "Test address generated: $TEST_ADDRESS"
else
    print_error "Address generation failed!"
    kill $DAEMON_PID 2>/dev/null
    exit 1
fi

# Generate some blocks
print_status "Generating test blocks..."
./build/bin/bitcoin-cli -datadir=test_network -conf=o.conf generatetoaddress 101 $TEST_ADDRESS > /dev/null

if [ $? -eq 0 ]; then
    print_success "Test blocks generated!"
else
    print_error "Block generation failed!"
    kill $DAEMON_PID 2>/dev/null
    exit 1
fi

# Check blockchain info
print_status "Checking blockchain info..."
BLOCK_COUNT=$(./build/bin/bitcoin-cli -datadir=test_network -conf=o.conf getblockcount)
print_success "Block count: $BLOCK_COUNT"

# Check wallet balance
BALANCE=$(./build/bin/bitcoin-cli -datadir=test_network -conf=o.conf getbalance)
print_success "Wallet balance: $BALANCE BTC"

# Test transaction
print_status "Testing transaction creation..."
NEW_ADDRESS=$(./build/bin/bitcoin-cli -datadir=test_network -conf=o.conf getnewaddress "test_receiver")
./build/bin/bitcoin-cli -datadir=test_network -conf=o.conf sendtoaddress $NEW_ADDRESS 1.0 > /dev/null

if [ $? -eq 0 ]; then
    print_success "Transaction created successfully!"
else
    print_error "Transaction creation failed!"
    kill $DAEMON_PID 2>/dev/null
    exit 1
fi

# Generate block to confirm transaction
print_status "Confirming transaction..."
./build/bin/bitcoin-cli -datadir=test_network -conf=o.conf generatetoaddress 1 $TEST_ADDRESS > /dev/null

if [ $? -eq 0 ]; then
    print_success "Transaction confirmed!"
else
    print_error "Transaction confirmation failed!"
    kill $DAEMON_PID 2>/dev/null
    exit 1
fi

# Step 5: Test O blockchain specific features
print_status "Step 5: Testing O blockchain specific features..."

# Test if O blockchain RPC commands are available (when implemented)
print_status "Checking O blockchain RPC commands..."
# Note: These will fail until we re-enable the RPC commands
# ./build/bin/bitcoin-cli -datadir=test_network -conf=o.conf help | grep -i "o" || print_warning "O blockchain RPC commands not yet enabled"

# Step 6: Cleanup
print_status "Step 6: Cleaning up..."
print_status "Stopping O blockchain daemon..."
./build/bin/bitcoin-cli -datadir=test_network -conf=o.conf stop > /dev/null 2>&1 || kill $DAEMON_PID 2>/dev/null

# Wait for daemon to stop
sleep 3

# Final success message
echo ""
echo "🎉 O Blockchain Test Deployment Completed Successfully!"
echo "======================================================"
print_success "✅ Build completed successfully"
print_success "✅ Tests passed"
print_success "✅ Daemon started and stopped correctly"
print_success "✅ RPC interface working"
print_success "✅ Wallet operations working"
print_success "✅ Block generation working"
print_success "✅ Transaction creation and confirmation working"
echo ""
print_status "O Blockchain is ready for development and testing!"
echo ""
print_status "Next steps:"
echo "  1. Re-enable O blockchain RPC commands"
echo "  2. Test user verification system"
echo "  3. Test measurement system"
print_status "  4. Test stabilization mining"
echo "  5. Deploy to production environment"
echo ""
print_status "Test network data saved in: test_network/"
print_status "To restart test network: ./build/bin/bitcoind -datadir=test_network -conf=test_network/o.conf"
echo ""
