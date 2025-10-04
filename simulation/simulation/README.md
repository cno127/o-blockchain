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
