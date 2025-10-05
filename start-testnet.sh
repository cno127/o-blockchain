#!/bin/bash
# O Blockchain Testnet Quick Start Script

set -e

echo "🚀 Starting O Blockchain on Regtest..."
echo ""

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Stop any existing instance
echo -e "${YELLOW}Stopping any existing instances...${NC}"
./build/bin/bitcoin-cli -regtest stop 2>/dev/null || true
sleep 2
pkill -f bitcoind 2>/dev/null || true
sleep 1

# Start daemon
echo -e "${BLUE}Starting O Blockchain daemon in regtest mode...${NC}"
./build/bin/bitcoind -regtest -daemon

# Wait for startup
echo "Waiting for daemon to start..."
sleep 5

# Create wallet if it doesn't exist
echo -e "${BLUE}Setting up wallet...${NC}"
./build/bin/bitcoin-cli -regtest createwallet "test_wallet" 2>/dev/null || ./build/bin/bitcoin-cli -regtest loadwallet "test_wallet" 2>/dev/null || true

# Generate initial blocks
echo -e "${BLUE}Generating initial blocks...${NC}"
ADDRESS=$(./build/bin/bitcoin-cli -regtest getnewaddress)
echo "Mining address: $ADDRESS"

./build/bin/bitcoin-cli -regtest generatetoaddress 101 $ADDRESS > /dev/null
echo -e "${GREEN}✅ Generated 101 blocks${NC}"

# Show blockchain info
echo ""
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${GREEN}🎉 O Blockchain Testnet is Ready!${NC}"
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""

# Display useful information
echo -e "${YELLOW}Blockchain Info:${NC}"
./build/bin/bitcoin-cli -regtest getblockchaininfo | grep -E "chain|blocks|headers"

echo ""
echo -e "${YELLOW}Your Balance:${NC}"
BALANCE=$(./build/bin/bitcoin-cli -regtest getbalance)
echo "$BALANCE O coins"

echo ""
echo -e "${YELLOW}Your Address:${NC}"
echo "$ADDRESS"

echo ""
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${GREEN}Quick Commands:${NC}"
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""
echo "# Generate more blocks:"
echo "./build/bin/bitcoin-cli -regtest generatetoaddress 1 $ADDRESS"
echo ""
echo "# Check measurement stats:"
echo "./build/bin/bitcoin-cli -regtest getmeasurementstatistics"
echo ""
echo "# Check stability status:"
echo "./build/bin/bitcoin-cli -regtest getstabilitystatus OUSD"
echo ""
echo "# Check KYC requirements:"
echo "./build/bin/bitcoin-cli -regtest doescountryrequirekyc US"
echo ""
echo "# View logs:"
echo "tail -f ~/Library/Application\\ Support/Bitcoin/regtest/debug.log"
echo ""
echo "# Stop daemon:"
echo "./build/bin/bitcoin-cli -regtest stop"
echo ""
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${GREEN}Happy Testing! 🎉${NC}"
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"

