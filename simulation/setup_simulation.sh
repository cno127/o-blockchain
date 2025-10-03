#!/bin/bash

# O Blockchain Multi-Node Simulation Setup
# This script sets up a local test network with multiple nodes

set -e

echo "🌊 Setting up O Blockchain Multi-Node Simulation..."

# Configuration
SIMULATION_DIR="simulation"
NODES_DIR="$SIMULATION_DIR/nodes"
DATA_DIR="$SIMULATION_DIR/data"
LOGS_DIR="$SIMULATION_DIR/logs"
CONFIG_DIR="$SIMULATION_DIR/config"

# Create directories
mkdir -p "$NODES_DIR"
mkdir -p "$DATA_DIR"
mkdir -p "$LOGS_DIR"
mkdir -p "$CONFIG_DIR"

echo "📁 Created simulation directories"

# Build the O Blockchain if not already built
if [ ! -f "build/bin/bitcoind" ]; then
    echo "🔨 Building O Blockchain..."
    cd build && make -j4 && cd ..
fi

echo "✅ O Blockchain build ready"

# Create node configurations
create_node_config() {
    local node_id=$1
    local port=$2
    local rpc_port=$3
    local data_dir="$DATA_DIR/node_$node_id"
    local config_file="$CONFIG_DIR/node_$node_id.conf"
    
    mkdir -p "$data_dir"
    
    cat > "$config_file" << EOF
# O Blockchain Node $node_id Configuration
# Generated for simulation testing

# Network settings
regtest=1
server=1
daemon=1
listen=1

# Data directory
datadir=$(pwd)/$data_dir

# Logging
debug=1
logtimestamps=1
logips=1

# O Blockchain specific settings
# Enable all O Blockchain features
enablepowpob=1
enablemeasurement=1
enablestabilization=1
enablecurrencyexchange=1
enablebrightid=1
enablegeographicaccess=1

# Mining settings
gen=1
miningthreads=1

# Connection settings
maxconnections=25
maxoutboundconnections=8

# RPC settings
rpcuser=o_user_$node_id
rpcpassword=o_pass_$node_id
rpcworkqueue=32

# Wallet settings
disablewallet=0

# Performance settings
dbcache=256
maxmempool=64

[regtest]
# Regtest-specific settings
port=$port
rpcport=$rpc_port
rpcbind=127.0.0.1:$rpc_port
rpcallowip=127.0.0.1
wallet=o_wallet_$node_id
EOF

    echo "📝 Created config for node $node_id"
}

# Create 5 test nodes
echo "🔧 Creating node configurations..."

create_node_config 1 18444 18332
create_node_config 2 18445 18333
create_node_config 3 18446 18334
create_node_config 4 18447 18335
create_node_config 5 18448 18336

# Create startup script
cat > "$SIMULATION_DIR/start_nodes.sh" << 'EOF'
#!/bin/bash

# Start all O Blockchain nodes for simulation

set -e

echo "🚀 Starting O Blockchain simulation nodes..."

# Function to start a node
start_node() {
    local node_id=$1
    local config_file="config/node_$node_id.conf"
    local log_file="logs/node_$node_id.log"
    
    echo "Starting node $node_id..."
    
    # Start the node
    ../build/bin/bitcoind -conf="$(pwd)/$config_file" > "$log_file" 2>&1 &
    local pid=$!
    echo $pid > "nodes/node_$node_id.pid"
    
    echo "Node $node_id started with PID $pid"
    sleep 2
}

# Start all nodes
cd "$(dirname "$0")"

start_node 1
start_node 2
start_node 3
start_node 4
start_node 5

echo "✅ All nodes started successfully!"
echo "📊 Node status:"
echo "Node 1: PID $(cat nodes/node_1.pid) - RPC: 18332"
echo "Node 2: PID $(cat nodes/node_2.pid) - RPC: 18333"
echo "Node 3: PID $(cat nodes/node_3.pid) - RPC: 18334"
echo "Node 4: PID $(cat nodes/node_4.pid) - RPC: 18335"
echo "Node 5: PID $(cat nodes/node_5.pid) - RPC: 18336"

echo ""
echo "🔗 To connect nodes, run: ./connect_nodes.sh"
echo "⛏️  To start mining, run: ./start_mining.sh"
echo "🧪 To run tests, run: ./run_tests.sh"
EOF

chmod +x "$SIMULATION_DIR/start_nodes.sh"

# Create node connection script
cat > "$SIMULATION_DIR/connect_nodes.sh" << 'EOF'
#!/bin/bash

# Connect all O Blockchain nodes in a mesh network

echo "🔗 Connecting O Blockchain nodes..."

# Function to connect nodes
connect_nodes() {
    local from_node=$1
    local to_node=$2
    local from_rpc=$3
    local to_port=$4
    
    echo "Connecting node $from_node to node $to_node..."
    
    ../build/bin/bitcoin-cli -conf="$(pwd)/config/node_$from_node.conf" \
        addnode "127.0.0.1:$to_port" "add"
    
    sleep 1
}

cd "$(dirname "$0")"

# Connect nodes in a mesh network
connect_nodes 1 2 18332 18445
connect_nodes 1 3 18332 18446
connect_nodes 1 4 18332 18447
connect_nodes 1 5 18332 18448

connect_nodes 2 1 18333 18444
connect_nodes 2 3 18333 18446
connect_nodes 2 4 18333 18447
connect_nodes 2 5 18333 18448

connect_nodes 3 1 18334 18444
connect_nodes 3 2 18334 18445
connect_nodes 3 4 18334 18447
connect_nodes 3 5 18334 18448

connect_nodes 4 1 18335 18444
connect_nodes 4 2 18335 18445
connect_nodes 4 3 18335 18446
connect_nodes 4 5 18335 18448

connect_nodes 5 1 18336 18444
connect_nodes 5 2 18336 18445
connect_nodes 5 3 18336 18446
connect_nodes 5 4 18336 18447

echo "✅ All nodes connected in mesh network!"

# Show peer connections
echo "📊 Peer connections:"
for i in {1..5}; do
    echo "Node $i peers:"
    ../build/bin/bitcoin-cli -conf="$(pwd)/config/node_$i.conf" getpeerinfo | grep -E '"addr"|"connected"'
    echo ""
done
EOF

chmod +x "$SIMULATION_DIR/connect_nodes.sh"

# Create mining script
cat > "$SIMULATION_DIR/start_mining.sh" << 'EOF'
#!/bin/bash

# Start mining on all nodes

echo "⛏️  Starting mining on O Blockchain nodes..."

cd "$(dirname "$0")"

# Function to start mining on a node
start_mining() {
    local node_id=$1
    local rpc_port=$2
    
    echo "Starting mining on node $node_id..."
    
    ../build/bin/bitcoin-cli -conf="$(pwd)/config/node_$node_id.conf" \
        setgenerate true 1
    
    echo "Mining started on node $node_id"
}

# Start mining on all nodes
start_mining 1 18332
start_mining 2 18333
start_mining 3 18334
start_mining 4 18335
start_mining 5 18336

echo "✅ Mining started on all nodes!"

# Show mining status
echo "📊 Mining status:"
for i in {1..5}; do
    echo "Node $i:"
    ../build/bin/bitcoin-cli -conf="$(pwd)/config/node_$i.conf" getmininginfo
    echo ""
done
EOF

chmod +x "$SIMULATION_DIR/start_mining.sh"

# Create test script
cat > "$SIMULATION_DIR/run_tests.sh" << 'EOF'
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
EOF

chmod +x "$SIMULATION_DIR/run_tests.sh"

# Create stop script
cat > "$SIMULATION_DIR/stop_nodes.sh" << 'EOF'
#!/bin/bash

# Stop all O Blockchain nodes

echo "🛑 Stopping O Blockchain simulation nodes..."

cd "$(dirname "$0")"

# Function to stop a node
stop_node() {
    local node_id=$1
    local pid_file="nodes/node_$node_id.pid"
    
    if [ -f "$pid_file" ]; then
        local pid=$(cat "$pid_file")
        echo "Stopping node $node_id (PID: $pid)..."
        
        # Try graceful shutdown first
        ../build/bin/bitcoin-cli -conf="$(pwd)/config/node_$node_id.conf" stop
        
        # Wait a bit for graceful shutdown
        sleep 3
        
        # Force kill if still running
        if kill -0 "$pid" 2>/dev/null; then
            echo "Force stopping node $node_id..."
            kill -9 "$pid"
        fi
        
        rm -f "$pid_file"
        echo "Node $node_id stopped"
    else
        echo "Node $node_id not running"
    fi
}

# Stop all nodes
stop_node 1
stop_node 2
stop_node 3
stop_node 4
stop_node 5

echo "✅ All nodes stopped!"
EOF

chmod +x "$SIMULATION_DIR/stop_nodes.sh"

# Create cleanup script
cat > "$SIMULATION_DIR/cleanup.sh" << 'EOF'
#!/bin/bash

# Clean up simulation data

echo "🧹 Cleaning up O Blockchain simulation..."

cd "$(dirname "$0")"

# Stop nodes first
./stop_nodes.sh

# Remove data directories
echo "Removing data directories..."
rm -rf data/node_*

# Remove log files
echo "Removing log files..."
rm -f logs/*.log

# Remove PID files
echo "Removing PID files..."
rm -f nodes/*.pid

echo "✅ Simulation cleanup completed!"
EOF

chmod +x "$SIMULATION_DIR/cleanup.sh"

# Create README
cat > "$SIMULATION_DIR/README.md" << 'EOF'
# O Blockchain Multi-Node Simulation

This directory contains a complete simulation setup for testing the O Blockchain with multiple nodes.

## Quick Start

1. **Start all nodes:**
   ```bash
   ./start_nodes.sh
   ```

2. **Connect nodes in mesh network:**
   ```bash
   ./connect_nodes.sh
   ```

3. **Start mining:**
   ```bash
   ./start_mining.sh
   ```

4. **Run comprehensive tests:**
   ```bash
   ./run_tests.sh
   ```

5. **Stop all nodes:**
   ```bash
   ./stop_nodes.sh
   ```

6. **Clean up simulation data:**
   ```bash
   ./cleanup.sh
   ```

## Node Configuration

- **Node 1:** Port 18444, RPC 18332
- **Node 2:** Port 18445, RPC 18333
- **Node 3:** Port 18446, RPC 18334
- **Node 4:** Port 18447, RPC 18335
- **Node 5:** Port 18448, RPC 18336

## Features Tested

- ✅ Basic blockchain functionality
- ✅ PoW/PoB consensus mechanism
- ✅ Business miner qualification
- ✅ Measurement system
- ✅ Stabilization mining
- ✅ Currency exchange
- ✅ Geographic access control
- ✅ Multi-currency support
- ✅ User consensus
- ✅ BrightID integration
- ✅ Network connectivity
- ✅ Transaction processing
- ✅ Mining rewards

## Logs and Data

- **Logs:** `logs/node_*.log`
- **Data:** `data/node_*/`
- **Configs:** `config/node_*.conf`

## Troubleshooting

1. **Nodes won't start:** Check if ports are available
2. **Connection issues:** Ensure all nodes are running before connecting
3. **Mining issues:** Check if nodes are connected and synced
4. **RPC errors:** Verify node is running and RPC is enabled

## Advanced Testing

You can run individual RPC commands on specific nodes:

```bash
# Get blockchain info from node 1
../build/bin/bitcoin-cli -conf="config/node_1.conf" getblockchaininfo

# Get O Blockchain specific stats
../build/bin/bitcoin-cli -conf="config/node_1.conf" getpowpobstats
../build/bin/bitcoin-cli -conf="config/node_1.conf" getmeasurementstats
../build/bin/bitcoin-cli -conf="config/node_1.conf" getstabilizationstats
```

## Customization

Edit the configuration files in `config/` to customize:
- Network settings
- RPC settings
- Mining parameters
- O Blockchain features
- Logging levels
EOF

echo "✅ O Blockchain simulation setup completed!"
echo ""
echo "📁 Simulation directory: $SIMULATION_DIR"
echo "🚀 To start: cd $SIMULATION_DIR && ./start_nodes.sh"
echo "🧪 To test: cd $SIMULATION_DIR && ./run_tests.sh"
echo "📖 For help: cd $SIMULATION_DIR && cat README.md"
