#!/bin/bash
# O Blockchain Testnet Stop Script

echo "🛑 Stopping O Blockchain testnet..."

# Try graceful stop first
./build/bin/bitcoin-cli -regtest stop 2>/dev/null || true

# Wait for graceful shutdown
echo "Waiting for graceful shutdown..."
sleep 3

# Check if still running
if pgrep -f "bitcoind.*regtest" > /dev/null; then
    echo "⚠️  Graceful shutdown failed, forcing stop..."
    pkill -f "bitcoind.*regtest"
    sleep 1
fi

# Verify stopped
if pgrep -f "bitcoind.*regtest" > /dev/null; then
    echo "❌ Failed to stop daemon"
    exit 1
else
    echo "✅ O Blockchain testnet stopped successfully"
fi

