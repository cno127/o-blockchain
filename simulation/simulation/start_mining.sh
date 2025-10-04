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
