#!/bin/bash

# Run comprehensive tests on O Blockchain simulation

echo "🧪 Running O Blockchain simulation tests..."

cd "$(dirname "$0")"

# Function to run RPC command on a node
rpc_call() {
    local node_id=$1
    local method=$2
    shift 2
    local params="$@"
    
    ../../build/src/bitcoin-cli -conf="config/node_$node_id.conf" \
        $method $params
}

# Test 1: Basic blockchain functionality
echo "🔍 Test 1: Basic blockchain functionality"
echo "Getting blockchain info from node 1..."
rpc_call 1 getblockchaininfo
echo ""

# Test 2: O Blockchain specific features
echo "🔍 Test 2: O Blockchain specific features"

echo "Testing PoW/PoB consensus..."
rpc_call 1 getpowpobstats
echo ""

echo "Testing business miner stats..."
rpc_call 1 getbusinessminerstats
echo ""

echo "Testing measurement system..."
rpc_call 1 getmeasurementstats
echo ""

echo "Testing stabilization mining..."
rpc_call 1 getstabilizationstats
echo ""

echo "Testing currency exchange..."
rpc_call 1 getsupportedpairs
echo ""

echo "Testing geographic access control..."
rpc_call 1 getaccessstatistics
echo ""

# Test 3: Multi-currency support
echo "🔍 Test 3: Multi-currency support"
echo "Getting supported currencies..."
rpc_call 1 getcurrencies
echo ""

# Test 4: User consensus
echo "🔍 Test 4: User consensus"
echo "Getting user statistics..."
rpc_call 1 getuserstatistics
echo ""

# Test 5: BrightID integration
echo "🔍 Test 5: BrightID integration"
echo "Getting BrightID status..."
rpc_call 1 getbrightidstatus
echo ""

# Test 6: Network connectivity
echo "🔍 Test 6: Network connectivity"
echo "Getting peer information..."
for i in {1..5}; do
    echo "Node $i peers:"
    rpc_call $i getpeerinfo | grep -E '"addr"|"connected"'
    echo ""
done

# Test 7: Transaction testing
echo "🔍 Test 7: Transaction testing"
echo "Getting wallet info..."
rpc_call 1 getwalletinfo
echo ""

echo "Getting balance..."
rpc_call 1 getbalance
echo ""

# Test 8: Mining rewards
echo "🔍 Test 8: Mining rewards"
echo "Getting mining info..."
rpc_call 1 getmininginfo
echo ""

echo "✅ All tests completed!"
echo "📊 Check logs in logs/ directory for detailed information"
