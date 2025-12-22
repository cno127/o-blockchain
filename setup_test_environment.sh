#!/bin/bash
# O Blockchain Testing Environment Setup Script

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
CLI_PATH="${BUILD_DIR}/bin/bitcoin-cli"
DAEMON_PATH="${BUILD_DIR}/bin/bitcoind"
DATA_DIR="${HOME}/.bitcoin/regtest"

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo "🌊 O Blockchain Testing Environment Setup"
echo "=========================================="
echo ""

# Check if binaries exist
if [ ! -f "$DAEMON_PATH" ]; then
    echo -e "${RED}❌ bitcoind not found at $DAEMON_PATH${NC}"
    echo "   Please build the project first:"
    echo "   cd build && cmake .. && make -j\$(nproc)"
    exit 1
fi

if [ ! -f "$CLI_PATH" ]; then
    echo -e "${RED}❌ bitcoin-cli not found at $CLI_PATH${NC}"
    exit 1
fi

echo -e "${GREEN}✅ Binaries found${NC}"

# Check if node is running
if $CLI_PATH -regtest getblockchaininfo >/dev/null 2>&1; then
    echo -e "${GREEN}✅ Node is already running${NC}"
    NODE_RUNNING=true
else
    echo -e "${YELLOW}⚠️  Node is not running${NC}"
    echo "   Starting node..."
    
    # Start node in daemon mode
    $DAEMON_PATH -regtest -daemon
    
    # Wait for node to start
    echo "   Waiting for node to start..."
    sleep 3
    
    if $CLI_PATH -regtest getblockchaininfo >/dev/null 2>&1; then
        echo -e "${GREEN}✅ Node started successfully${NC}"
        NODE_RUNNING=true
    else
        echo -e "${RED}❌ Failed to start node${NC}"
        exit 1
    fi
fi

# Check/create wallet
echo ""
echo "👛 Setting up wallet..."
WALLETS=$($CLI_PATH -regtest listwallets 2>/dev/null || echo "[]")
if echo "$WALLETS" | grep -q "test_wallet"; then
    echo -e "${GREEN}✅ Wallet 'test_wallet' exists${NC}"
    $CLI_PATH -regtest loadwallet "test_wallet" >/dev/null 2>&1 || true
else
    echo "   Creating wallet 'test_wallet'..."
    $CLI_PATH -regtest createwallet "test_wallet" >/dev/null 2>&1
    echo -e "${GREEN}✅ Wallet created${NC}"
fi

# Check block height
echo ""
echo "⛏️  Checking blockchain..."
BLOCK_INFO=$($CLI_PATH -regtest getblockchaininfo)
BLOCKS=$(echo "$BLOCK_INFO" | grep -o '"blocks":[0-9]*' | grep -o '[0-9]*' || echo "0")

echo "   Current blocks: $BLOCKS"

if [ "$BLOCKS" -lt 101 ]; then
    NEEDED=$((101 - BLOCKS))
    echo "   Generating $NEEDED blocks to reach maturity..."
    
    ADDRESS=$($CLI_PATH -regtest getnewaddress)
    $CLI_PATH -regtest generatetoaddress $NEEDED "$ADDRESS" >/dev/null 2>&1
    
    echo -e "${GREEN}✅ Generated $NEEDED blocks${NC}"
else
    echo -e "${GREEN}✅ Sufficient blocks ($BLOCKS >= 101)${NC}"
fi

# Check databases
echo ""
echo "💾 Checking databases..."
if [ -d "$DATA_DIR/brightid_users" ] && [ -d "$DATA_DIR/measurements" ] && [ -d "$DATA_DIR/business_miners" ]; then
    echo -e "${GREEN}✅ All databases initialized${NC}"
else
    echo -e "${YELLOW}⚠️  Some databases missing (will be created on first use)${NC}"
fi

# Check O-specific RPC commands
echo ""
echo "🔍 Verifying O Blockchain features..."

STATUS=0

# Test measurement statistics
if $CLI_PATH -regtest getmeasurementstatistics >/dev/null 2>&1; then
    echo -e "${GREEN}✅ Measurement system accessible${NC}"
else
    echo -e "${RED}❌ Measurement system not accessible${NC}"
    STATUS=1
fi

# Test stabilization stats
if $CLI_PATH -regtest getstabilizationstats >/dev/null 2>&1; then
    echo -e "${GREEN}✅ Stabilization system accessible${NC}"
else
    echo -e "${RED}❌ Stabilization system not accessible${NC}"
    STATUS=1
fi

# Test invites
if $CLI_PATH -regtest getactiveinvites >/dev/null 2>&1; then
    echo -e "${GREEN}✅ Invitation system accessible${NC}"
else
    echo -e "${RED}❌ Invitation system not accessible${NC}"
    STATUS=1
fi

# Summary
echo ""
echo "=========================================="
if [ $STATUS -eq 0 ]; then
    echo -e "${GREEN}✅ Testing environment ready!${NC}"
    echo ""
    echo "Quick commands:"
    echo "  python3 test_o_setup_helper.py test-all    # Run all tests"
    echo "  ./build/test/functional/test_runner.py test_o_integration_complete  # Run integration test"
    echo ""
    echo "See TESTING_ENVIRONMENT_SETUP.md for detailed guide"
else
    echo -e "${YELLOW}⚠️  Some features may not be fully available${NC}"
    echo "   Check debug logs: tail -f $DATA_DIR/debug.log"
fi

exit $STATUS

