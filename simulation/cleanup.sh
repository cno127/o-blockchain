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
