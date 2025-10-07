# O Blockchain

> The world's first water price-based stablecoin - **Early Development / Prototype**

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](COPYING)
[![Status](https://img.shields.io/badge/status-prototype-yellow.svg)]()
[![Wiki](https://img.shields.io/badge/docs-wiki-blue.svg)](https://github.com/cno127/o-blockchain/wiki)
[![Bitcoin Core](https://img.shields.io/badge/based%20on-Bitcoin%20Core%2027.0-orange.svg)](https://github.com/bitcoin/bitcoin)

## ⚠️ Development Status

**This is a prototype for discussion and feedback, not production-ready software.**

- 🔬 **Stage**: Early development and architectural exploration
- 🎯 **Purpose**: Demonstrate concepts and gather community input
- ⚙️ **Code**: Written with prototyping in mind, not optimized for scale
- 🧪 **Testing**: Requires extensive testing before any production use
- 📝 **Feedback**: All suggestions and contributions welcome!

## 🌍 What is O Blockchain?

O Blockchain is a revolutionary cryptocurrency system that provides stable, universal currency for all 8 billion people on Earth.

### Core Principles

- **💧 Water Price-Based Stability**: 1 O = 1 liter of bottled water price (universal basic need)
- **🌍 142 Global Currencies**: One O currency per national currency (O_USD, O_EUR, O_JPY, etc.)
- **⚖️ Economic Equality**: Same purchasing power for everyone, everywhere
- **🌱 Climate Restoration**: Unlimited, debt-free funding for environmental projects
- **🏠 Immigration Reversal**: Local economic stability ends mass migration
- **🔓 Decentralized Governance**: No central authority, community consensus

## 🚀 Quick Start

### Build from Source

```bash
# Clone repository
git clone https://github.com/cno127/o-blockchain.git
cd o-blockchain

# Build with CMake
mkdir build && cd build
cmake ..
make -j$(nproc)

# Run in regtest mode
./src/bitcoind -regtest -daemon

# Create wallet
./src/bitcoin-cli -regtest createwallet "test"

# Generate blocks
./src/bitcoin-cli -regtest generatetoaddress 101 $(./src/bitcoin-cli -regtest getnewaddress)

# Check O currencies
./src/bitcoin-cli -regtest listocurrencies
```

### Installation

See [INSTALL.md](INSTALL.md) for detailed build instructions for your platform.

### Full Documentation

📚 **[Visit the Wiki](https://github.com/cno127/o-blockchain/wiki)** for comprehensive documentation.

## ✨ Key Features

| Feature | Description |
|---------|-------------|
| **💧 Stable Value** | Pegged to water price, not fiat currency |
| **🌍 Universal Coverage** | 142 currencies covering 195 countries |
| **👥 8 Billion Users** | Designed to scale to all of humanity |
| **🔐 Proof of Personhood** | BrightID integration prevents Sybil attacks |
| **💰 UBI Vision** | Designed with universal basic income in mind (long-term goal) |
| **♻️ Climate Funding** | Unlimited supply for environmental restoration |
| **⚡ High Performance** | Targeting 100K+ TPS with Layer 2 solutions |
| **🔓 Open Source** | MIT licensed, community-driven development |

## 📊 System Overview

### How It Works

1. **Water Price Measurement**: Users measure bottled water prices globally
2. **Multi-Currency System**: 142 separate O currencies match national currencies
3. **Stabilization Mining**: When exchange rates deviate from measured water prices, new coins are created and distributed to stable currency holders, diluting unstable holders. Core principle: offender fines reward the offended through coin creation/dilution
4. **Proof of Personhood**: BrightID ensures one person = one identity
5. **Universal Basic Income**: Everyone receives same purchasing power

### Technical Stack

- **Base**: Bitcoin Core 27.0
- **Consensus**: Hybrid PoW (33%) / PoB (67%)
- **Block Time**: 10 minutes (Bitcoin standard)
- **UTXO Model**: Multi-currency support
- **Identity**: BrightID integration
- **Scalability**: Layer 1 optimizations + Layer 2 plans

## 📚 Documentation

All comprehensive documentation is available in our **[GitHub Wiki](https://github.com/cno127/o-blockchain/wiki)**:

### 🎯 Essential Reading

- **[Quick Start Guide](https://github.com/cno127/o-blockchain/wiki/Quick-Start-Guide)** - Get up and running in 5 minutes
- **[Architecture Overview](https://github.com/cno127/o-blockchain/wiki/Architecture-Overview)** - How O Blockchain works
- **[Multi-Currency System](https://github.com/cno127/o-blockchain/wiki/Multi-Currency-Architecture)** - 142 global currencies explained
- **[Measurement System](https://github.com/cno127/o-blockchain/wiki/Measurement-System-Overview)** - Water price calibration

### 💰 Economic Model

- **[Universal Basic Income](https://github.com/cno127/o-blockchain/wiki/Universal-Basic-Income)** - Why O Coin is perfect for UBI
- **[Immigration Impact](https://github.com/cno127/o-blockchain/wiki/Immigration-Impact)** - How UBI reverses mass migration
- **[Climate Change Solution](https://github.com/cno127/o-blockchain/wiki/Climate-Change-Solution)** - Debt-free planetary restoration

### 🌍 Coverage & Scale

- **[Currency Registry](https://github.com/cno127/o-blockchain/wiki/Currency-Registry)** - All 142 currencies
- **[Country Mapping](https://github.com/cno127/o-blockchain/wiki/Country-Mapping)** - Global coverage details
- **[Scalability Analysis](https://github.com/cno127/o-blockchain/wiki/Scalability-Overview)** - 8 billion users plan

### 🔧 Developer Resources

- **[Testing Guide](https://github.com/cno127/o-blockchain/wiki/Testing-Guide)** - How to test O Blockchain
- **[RPC Commands](https://github.com/cno127/o-blockchain/wiki/RPC-Commands)** - API reference
- **[Currency Exchange](https://github.com/cno127/o-blockchain/wiki/Currency-Exchange)** - Trading O currencies

### 📊 Research & Analysis

- **[Adoption Simulation](https://github.com/cno127/o-blockchain/wiki/Blockchain-Simulation)** - Mathematical modeling
- **[Developer Strategy](https://github.com/cno127/o-blockchain/wiki/Developer-Strategy)** - Growing the ecosystem
- **[Implementation Status](https://github.com/cno127/o-blockchain/wiki/Implementation-Status)** - Current progress

## 🎯 Use Cases

### For Individuals
- Receive universal basic income (420 O/month)
- Store value in stable, non-inflationary currency
- Send money globally with minimal fees
- Participate in economic system as equals

### For Developers
- Build apps on stable multi-currency platform
- Access 142 currencies programmatically
- Create UBI-enabled applications
- Contribute to open source project

### For Governments
- Deploy national O currency
- Implement UBI programs
- Fund environmental projects debt-free
- Provide financial stability to citizens

### For Climate Projects
- Access unlimited funding for restoration
- Finance reforestation, ocean cleanup
- Deploy renewable energy at scale
- Restore soil and ecosystems

## 🏗️ Built on Bitcoin Core

O Blockchain extends Bitcoin Core with:

✅ **Multi-Currency UTXO Model** - 142 currencies in one ledger  
✅ **Stabilization Mining** - Dynamic rewards maintain stability  
✅ **Measurement System** - Decentralized water price oracles  
✅ **Proof of Personhood** - BrightID integration  
✅ **User Registry** - Global identity management  
✅ **Currency Exchange** - On-chain O currency trading  

All while maintaining Bitcoin's proven security and decentralization.

## 🤝 Contributing

We welcome contributions from everyone!

### How to Contribute

1. **Code**: Submit pull requests for features or bug fixes
2. **Documentation**: Improve wiki pages or create new guides
3. **Testing**: Help test on regtest/testnet
4. **Translation**: Translate documentation to other languages
5. **Community**: Help others on Discord/Telegram

See [CONTRIBUTING.md](CONTRIBUTING.md) for detailed guidelines.

### Development Setup

```bash
# Fork the repository
git clone https://github.com/YOUR_USERNAME/o-blockchain.git
cd o-blockchain

# Create feature branch
git checkout -b feature/your-feature-name

# Make changes and commit
git add .
git commit -m "Description of changes"

# Push and create pull request
git push origin feature/your-feature-name
```

## 📞 Community

Join our growing community:

- **📚 Wiki**: [Documentation](https://github.com/cno127/o-blockchain/wiki)
- **💬 Discord**: discord.gg/oblockchain
- **✈️ Telegram**: t.me/oblockchain
- **🐦 Twitter/X**: [@OBlockchain](https://twitter.com/OBlockchain)
- **🐙 GitHub**: [Issues](https://github.com/cno127/o-blockchain/issues) & [Discussions](https://github.com/cno127/o-blockchain/discussions)

## 📊 Key Statistics

| Metric | Value |
|--------|-------|
| **Currencies** | 142 O currencies |
| **Countries** | 195+ covered |
| **Target Users** | 8 billion (all humanity) |
| **UBI Proposal** | To be determined by community |
| **Consensus** | PoW (33%) + PoB (67%) |
| **Block Time** | 10 minutes |
| **Based On** | Bitcoin Core 27.0 |

## ❓ FAQ

**Q: Why water price as the peg?**  
A: Water is a universal basic need with consistent relative value globally. Unlike USD or gold, everyone needs it equally. Each O currency maintains 1 O = 1 liter of water in its local market. Exchange rates between O currencies are calculated from water price ratios (e.g., if water = $1.50 in USA and €1.20 in Europe, then 1 O_USD = 1.25 O_EUR). Economic incentives force market actors to maintain these theoretical rates.

**Q: Why 142 currencies instead of one?**  
A: Different countries have different price levels and fiat currencies. Each O currency maintains 1 O = 1 liter of water in its local market. O currencies have different values from each other - exchange rates are calculated from water price ratios and fiat exchange rates. Note: O_ONLY is a universal currency that exchanges 1:1 with all O currencies. It still maintains the water price peg through measurement and stabilization, allowing inflation correction regardless of fiat currency status.

**Q: What about Universal Basic Income?**  
A: UBI is a long-term vision for O Blockchain. If implemented, everyone would receive a monthly amount in their birth currency, providing equal purchasing power globally. The specific amount would be determined by community governance, not predetermined by developers. Implementing UBI requires significant development, testing, and governance mechanisms that are not yet complete.

**Q: Is this inflationary?**  
A: No. Value is tied to water (constant need), not scarcity. We can generate unlimited supply without devaluation.

**Q: How do you prevent fake accounts?**  
A: BrightID proof-of-personhood ensures one person = one identity through decentralized social graph verification.

See full [FAQ on Wiki](https://github.com/cno127/o-blockchain/wiki/FAQ) for more questions.

## 📄 License

O Blockchain is released under the terms of the MIT license. See [COPYING](COPYING) for more information.

## 🙏 Acknowledgments

- **Bitcoin Core**: For the robust foundation
- **BrightID**: For decentralized identity solution
- **Community**: For contributions and support

## 🌟 Vision

> *"Money should serve humanity, not enslave it. O Blockchain creates a financial system based on basic human needs, providing stability, equality, and hope for all 8 billion people on Earth."*

### The Problem

Current financial systems:
- ❌ Unstable currencies (inflation, manipulation)
- ❌ Wealth inequality (billionaires vs. poverty)
- ❌ Debt-based money (interest enslaves)
- ❌ Central control (banks, governments)
- ❌ Climate crisis (no funding mechanism)
- ❌ Mass immigration (economic desperation)

### The Solution

O Blockchain provides:
- ✅ Stable value (water-based peg)
- ✅ Universal basic income (equal for all)
- ✅ Debt-free money (unlimited supply)
- ✅ Decentralized (community governance)
- ✅ Climate restoration (unlimited funding)
- ✅ Local economies (immigration reversal)

## 🚀 Get Started

Ready to join the revolution?

1. **Learn**: Read the [Wiki](https://github.com/cno127/o-blockchain/wiki)
2. **Build**: Follow [Installation Guide](INSTALL.md)
3. **Test**: Use [Testing Guide](https://github.com/cno127/o-blockchain/wiki/Testing-Guide)
4. **Contribute**: See [Contributing Guidelines](CONTRIBUTING.md)
5. **Connect**: Join [Discord](https://discord.gg/oblockchain)

---

**For comprehensive documentation, visit our [GitHub Wiki](https://github.com/cno127/o-blockchain/wiki)**

*Building a better financial system for all of humanity* 🌍

