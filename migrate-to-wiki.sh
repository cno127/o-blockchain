#!/bin/bash
# O Blockchain Wiki Migration Script
# This script automates the migration of documentation to GitHub Wiki

set -e

echo "🚀 O Blockchain Wiki Migration Script"
echo "======================================"
echo ""

# Configuration
WIKI_REPO="https://github.com/cno127/o-blockchain.wiki.git"
WIKI_DIR="/Users/o/o-blockchain.wiki"
BITCOIN_DIR="/Users/o/bitcoin"

# Step 1: Clone wiki repository
echo "📥 Step 1: Cloning wiki repository..."
if [ -d "$WIKI_DIR" ]; then
    echo "⚠️  Wiki directory already exists. Pulling latest changes..."
    cd "$WIKI_DIR"
    git pull
else
    cd /Users/o
    git clone "$WIKI_REPO"
fi

echo "✅ Wiki repository ready"
echo ""

# Step 2: Create Home page
echo "📝 Step 2: Creating Home page..."
cd "$WIKI_DIR"

cat > Home.md << 'EOF'
# Welcome to O Blockchain Wiki

> The world's first water-based stablecoin with built-in Universal Basic Income

## 🌍 What is O Blockchain?

O Blockchain is a revolutionary cryptocurrency system built on Bitcoin Core that provides:

- **💧 Stable Value**: 1 O = 1 liter of water (universal basic need)
- **🌍 Universal Coverage**: 142 currencies across 195 countries  
- **⚖️ Economic Equality**: Same UBI for all humans on Earth
- **🌱 Climate Restoration**: Unlimited, debt-free environmental funding
- **🏠 Immigration Reversal**: Local economic stability ends mass migration
- **🔓 Decentralized**: No central authority, community consensus

## 🚀 Quick Navigation

### New to O Blockchain?
- [Quick Start Guide](Quick-Start-Guide) - Get up and running in 5 minutes
- [Installation](Installation) - Build and install O Blockchain
- [Testing Guide](Testing-Guide) - Test on regtest/testnet

### Understanding the System
- [Architecture Overview](Architecture-Overview) - How O Blockchain works
- [Multi-Currency System](Multi-Currency-Architecture) - 142 global currencies
- [Measurement System](Measurement-System-Overview) - Water price calibration
- [Stabilization Mining](Stabilization-Mining) - Maintaining currency stability

### Economic & Social Impact
- [Universal Basic Income](Universal-Basic-Income) - Why O Coin is perfect for UBI
- [Immigration Reversal](Immigration-Impact) - How UBI stops mass migration
- [Climate Change Solution](Climate-Change-Solution) - Debt-free planetary restoration

### Technical Details
- [Currency Coverage](Currency-Registry) - All 142 currencies mapped
- [Scalability](Scalability-Overview) - Scaling to 8 billion users
- [High TPS Strategy](High-TPS-Strategy) - Transaction throughput plans
- [Currency Exchange](Currency-Exchange) - How O currencies trade

### Developers
- [RPC Commands](RPC-Commands) - API reference
- [Developer Promotion](Developer-Strategy) - Growing the community
- [Project Status](Implementation-Status) - Current progress

### Research
- [Adoption Simulation](Blockchain-Simulation) - Mathematical modeling
- [Transaction Analysis](Transaction-Volume-Analysis) - Global TPS requirements

## 📊 Key Statistics

- **Currencies**: 142 O currencies (one per national currency)
- **Countries**: 195+ countries and territories covered
- **Target Users**: 8 billion people (all of humanity)
- **UBI Proposal**: 420 O per month (basic needs coverage)
- **Consensus**: Hybrid PoW (33%) / PoB (67%)
- **Block Time**: 10 minutes (Bitcoin Core)
- **Inflation**: Dynamic based on currency stability needs

## 🎯 Core Features

### 1. Water-Based Stability
- 1 O = 1 liter of bottled water (universal basic need)
- Measured by actual users in each country
- Resistant to manipulation (basic necessity)
- Same relative value globally

### 2. Multi-Currency System
- 142 separate O currencies (O_USD, O_EUR, O_JPY, etc.)
- Matches every major national currency
- Exchange between O currencies on-chain
- Stabilization mining maintains parity

### 3. Proof of Personhood
- BrightID integration prevents Sybil attacks
- One person = one identity
- Privacy-preserving verification
- Decentralized trust network

### 4. Universal Basic Income
- Same purchasing power for everyone
- Paid in birth country's O currency
- No inflation (based on stable water price)
- Unlimited supply without debt

### 5. Climate Restoration
- Generate unlimited O for environmental projects
- No creditors, no interest, no debt
- Fund massive reforestation, ocean cleanup, renewables
- Local production reduces transportation emissions

## 🗺️ Documentation Map

| Section | Description | Start Here |
|---------|-------------|------------|
| **Getting Started** | Installation, setup, testing | [Quick Start](Quick-Start-Guide) |
| **Architecture** | System design and components | [Overview](Architecture-Overview) |
| **Measurement** | Water price calibration system | [Policy](Global-Measurement-Policy) |
| **Economic** | UBI, immigration, climate | [UBI](Universal-Basic-Income) |
| **Currencies** | 142 currencies, country mapping | [Registry](Currency-Registry) |
| **Scalability** | 8B users, high TPS | [Analysis](Scalability-Overview) |
| **Technical** | APIs, RPC, exchange | [Exchange](Currency-Exchange) |
| **Status** | Current implementation state | [Status](Implementation-Status) |
| **Research** | Simulations, strategies | [Simulation](Blockchain-Simulation) |

## 🔗 External Resources

### Code & Development
- **Main Repository**: [github.com/cno127/o-blockchain](https://github.com/cno127/o-blockchain)
- **Issue Tracker**: [Issues](https://github.com/cno127/o-blockchain/issues)
- **Pull Requests**: [PRs](https://github.com/cno127/o-blockchain/pulls)
- **Releases**: [Releases](https://github.com/cno127/o-blockchain/releases)

### Community
- **Discord**: discord.gg/oblockchain
- **Telegram**: t.me/oblockchain
- **Twitter/X**: [@OBlockchain](https://twitter.com/OBlockchain)

### Documentation
- **Bitcoin Core Docs**: [doc/](https://github.com/cno127/o-blockchain/tree/main/doc)
- **Developer Notes**: [developer-notes.md](https://github.com/cno127/o-blockchain/blob/main/doc/developer-notes.md)

## 💡 Key Innovations

### 1. First Water-Based Stablecoin
Traditional stablecoins peg to USD (unstable, centralized). O pegs to water (universal basic need, impossible to manipulate).

### 2. Multi-Currency Native Design
Unlike other cryptos with one token, O has 142 currencies to match real-world economic diversity while maintaining equal value.

### 3. Decentralized Measurement System
Users measure water prices globally, with cryptographic proofs and multiple validators. No central price oracle needed.

### 4. Dynamic Stabilization Mining
Miners earn rewards proportional to currency volatility. More unstable = higher rewards = faster stabilization.

### 5. Unlimited Supply Without Inflation
Can generate infinite O because value is tied to water (constant), not scarcity. Perfect for UBI and climate funding.

## 🤝 Contributing

We welcome contributions!

- **Code**: Submit pull requests to main repo
- **Documentation**: Edit wiki pages directly
- **Issues**: Report bugs or suggest features
- **Community**: Help others on Discord/Telegram

See [Contributing Guide](Contributing) for details.

## 📅 Project Timeline

- **Phase 1**: Core Development ✅ (Complete)
- **Phase 2**: Testing & Documentation ✅ (Complete)
- **Phase 3**: Testnet Launch 🔄 (Current)
- **Phase 4**: Security Audits (Q2 2025)
- **Phase 5**: Mainnet Launch (Q3 2025)
- **Phase 6**: Global Adoption (2025-2030)

## ❓ Quick FAQ

**Q: How is 1 O = 1 liter of water maintained?**  
A: Users measure bottled water prices globally. Stabilization mining adjusts supply to maintain this ratio.

**Q: Why 142 currencies instead of one?**  
A: Matches real-world economics. Different countries have different price levels. Each O currency reflects its national economy.

**Q: How does UBI work?**  
A: Everyone receives 420 O per month in their birth country's currency. Same purchasing power globally because all O currencies equal 1L of water.

**Q: Is this inflationary?**  
A: No. Value is tied to water (constant basic need), not scarcity. Can generate unlimited supply without devaluation.

**Q: How do you prevent Sybil attacks?**  
A: BrightID proof-of-personhood ensures one person = one identity. Decentralized social graph verification.

**Q: What about scalability?**  
A: Layer 1 optimizations (10x block size, 1min blocks) + Layer 2 (Lightning Network) targets 100K+ TPS.

See full [FAQ](FAQ) for more questions.

## 📖 Featured Articles

### Must-Read Documents

1. **[Why O Coin is Perfect for UBI](Universal-Basic-Income)**  
   Comprehensive analysis of O's unique suitability for universal basic income

2. **[How UBI Reverses Immigration](Immigration-Impact)**  
   Economic analysis of how global UBI stops and reverses mass migration

3. **[Fighting Climate Change with Unlimited Funding](Climate-Change-Solution)**  
   How debt-free money creation enables planetary restoration

4. **[Blockchain Adoption Simulation](Blockchain-Simulation)**  
   Mathematical modeling of O's growth from 0 to 5 billion users

5. **[User Registry Scalability](User-Registry-Scalability)**  
   Technical analysis of scaling to 8 billion user records

## 🌟 Vision

> *"Money should serve humanity, not enslave it. O Blockchain creates a financial system based on basic human needs, providing stability, equality, and hope for all 8 billion people on Earth."*

---

**Last Updated**: 2025-01-06  
**Wiki Version**: 1.0  
**O Blockchain Version**: 27.0.0 (based on Bitcoin Core 27.0)

*For the latest code and releases, visit the [main repository](https://github.com/cno127/o-blockchain)*
EOF

echo "✅ Home page created"
echo ""

# Step 3: Create Sidebar
echo "📝 Step 3: Creating Sidebar navigation..."

cat > _Sidebar.md << 'EOF'
**[🏠 Home](Home)**

---

**📚 Getting Started**
- [Quick Start](Quick-Start-Guide)
- [Installation](Installation)
- [Rename Guide](Rename-To-O-Guide)
- [Testing](Testing-Guide)
  - [Testnet](Testnet-Testing-Guide)
  - [Deployment](Test-Deployment-Guide)

---

**🏗️ Architecture**
- [Overview](Architecture-Overview)
- [Multi-Currency](Multi-Currency-Architecture)
- [Blockchain](Blockchain-Architecture)
- [Currency Storage](Currency-Storage-In-Ledger)
- [Stabilization](Stabilization-Mining)
  - [Dynamic Factor](Dynamic-Stabilization-Factor)
  - [Improvements](Stabilization-Improvements)

---

**📏 Measurement**
- [Overview](Measurement-System-Overview)
- [Global Policy](Global-Measurement-Policy)
- [Volume Validation](Volume-Validation-System)

---

**💰 Economic Model**
- [Universal Basic Income](Universal-Basic-Income)
- [Immigration Impact](Immigration-Impact)
- [Climate Solution](Climate-Change-Solution)

---

**🌍 Currency Coverage**
- [Currency Registry](Currency-Registry)
- [Country Mapping](Country-Mapping)
- [Coverage Summary](Coverage-Summary)

---

**⚡ Scalability**
- [Overview](Scalability-Overview)
- [User Registry](User-Registry-Scalability)
- [Measurements](Measurement-Scalability)
- [Transaction Volume](Transaction-Volume-Analysis)
- [High TPS Strategy](High-TPS-Strategy)
- [Lightning Layer 2](Lightning-Layer2-Feasibility)

---

**🔧 Technical Guides**
- [Currency Exchange](Currency-Exchange)
- [RPC Commands](RPC-Commands)

---

**📊 Project Status**
- [Implementation](Implementation-Status)
- [Deployment Ready](Deployment-Ready)
- [Integration](Integration-Summary)
- [Compilation](Compilation-Success)

---

**🔬 Research**
- [Adoption Simulation](Blockchain-Simulation)
- [Developer Strategy](Developer-Strategy)

---

**🤝 Community**
- [Contributing](Contributing)
- [FAQ](FAQ)
- [Contact](Contact)

---

**🔗 Links**
- [Main Repo](https://github.com/cno127/o-blockchain)
- [Discord](https://discord.gg/oblockchain)
EOF

echo "✅ Sidebar created"
echo ""

# Step 4: Commit initial structure
echo "💾 Step 4: Committing initial wiki structure..."
git add Home.md _Sidebar.md
git commit -m "Initialize O Blockchain Wiki

Created:
- Home page with overview, navigation, and quick links
- Sidebar with organized navigation structure

Ready for content migration from main repository."

git push origin master

echo "✅ Initial wiki structure committed and pushed"
echo ""

echo "=========================================="
echo "✅ Wiki initialization complete!"
echo ""
echo "Next steps:"
echo "1. Visit: https://github.com/cno127/o-blockchain/wiki"
echo "2. Verify Home page and sidebar are visible"
echo "3. Run migration script to copy documentation"
echo ""
echo "To migrate documentation, run:"
echo "  bash migrate-docs-to-wiki.sh"
echo "=========================================="
EOF

chmod +x migrate-to-wiki.sh

echo "✅ Created migration script: migrate-to-wiki.sh"

