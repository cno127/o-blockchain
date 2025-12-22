#!/bin/bash
set -e

CLI="./build/bin/bitcoin-cli -regtest"
WALLET_NAME="test_wallet"

# Load wallet
$CLI loadwallet "$WALLET_NAME" >/dev/null 2>&1 || $CLI createwallet "$WALLET_NAME" >/dev/null 2>&1

# Generate blocks if needed
BLOCKS=$($CLI getblockchaininfo | jq -r '.blocks')
if [ "$BLOCKS" -lt 101 ]; then
    echo "Generating blocks to maturity..."
    $CLI generatetoaddress $((101 - BLOCKS)) $($CLI getnewaddress) >/dev/null
fi

# Create user
USER_ID="test_user_$(date +%s)"
echo "Creating user: $USER_ID"

TX_RESULT=$($CLI submituserverificationtx \
  "$USER_ID" \
  "brightid" \
  "USA" \
  "OUSD" \
  '{"score":95,"verified":true}' \
  "0x$(openssl rand -hex 32)")

TXID=$(echo "$TX_RESULT" | jq -r '.txid')
echo "Transaction created: $TXID"

# Mine block
echo "Mining block to confirm..."
$CLI generatetoaddress 1 $($CLI getnewaddress) >/dev/null

echo "✅ User created successfully!"
echo "Check logs: tail -f ~/.bitcoin/regtest/debug.log | grep -i 'user verification'"
