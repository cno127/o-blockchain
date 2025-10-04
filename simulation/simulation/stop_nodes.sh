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
