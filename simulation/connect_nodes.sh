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
