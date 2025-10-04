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
