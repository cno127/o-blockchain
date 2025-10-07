# Wiki Setup Instructions

The wiki is enabled but needs to be initialized with the first page. Follow these steps:

## Step 1: Create First Wiki Page (Manual)

1. Go to: **https://github.com/cno127/o-blockchain/wiki**
2. Click the green **"Create the first page"** button
3. You'll see an editor with two fields:
   - **Page title**: Enter `Home`
   - **Content**: Copy the content from `/Users/o/bitcoin/wiki-pages/Home.md`
4. Scroll down and click **"Save Page"**

## Step 2: Clone Wiki Repository

Once the first page is created, the wiki git repository will be available:

```bash
cd /Users/o
git clone https://github.com/cno127/o-blockchain.wiki.git
cd o-blockchain.wiki
```

## Step 3: Add Sidebar

```bash
cd /Users/o/o-blockchain.wiki

# Copy sidebar content
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

# Commit and push
git add _Sidebar.md
git commit -m "Add sidebar navigation"
git push origin master
```

## Step 4: Verify

1. Go back to https://github.com/cno127/o-blockchain/wiki
2. You should now see the Home page with sidebar navigation
3. The sidebar will appear on all wiki pages

## Step 5: Start Migrating Documentation

Once the wiki is initialized, you can start adding pages. I've prepared all the content in:
- `/Users/o/bitcoin/wiki-pages/` directory

You can either:
- Copy/paste through GitHub web interface (easier for now)
- Use git commands to add pages in bulk (faster for many pages)

## Quick Page Creation Template

For each document to migrate, create a new wiki page:

1. Click "New Page" on wiki
2. Enter page title (use dashes instead of spaces, e.g., "Quick-Start-Guide")
3. Paste the markdown content
4. Click "Save Page"

## Documents Ready to Migrate (32 files)

### Getting Started (3)
- [ ] Quick-Start-Guide (QUICK_START.md)
- [ ] Installation (INSTALL.md) 
- [ ] Rename-To-O-Guide (RENAME_TO_O_GUIDE.md)

### Testing (3)
- [ ] Testing-Guide (TESTING_GUIDE.md)
- [ ] Testnet-Testing-Guide (TESTNET_TESTING_GUIDE.md)
- [ ] Test-Deployment-Guide (TEST_DEPLOYMENT_GUIDE.md)

### Architecture (7)
- [ ] Architecture-Overview (o_blockchain_architecture.md)
- [ ] Multi-Currency-Architecture (MULTICURRENCY_ARCHITECTURE.md)
- [ ] Blockchain-Architecture (combine arch docs)
- [ ] Currency-Storage-In-Ledger (CURRENCY_STORAGE_IN_LEDGER.md)
- [ ] Stabilization-Mining (create overview)
- [ ] Dynamic-Stabilization-Factor (DYNAMIC_STABILIZATION_FACTOR.md)
- [ ] Stabilization-Improvements (STABILIZATION_IMPROVEMENTS_SUMMARY.md)

### Measurement (3)
- [ ] Measurement-System-Overview (create from measurement docs)
- [ ] Global-Measurement-Policy (GLOBAL_MEASUREMENT_POLICY.md)
- [ ] Volume-Validation-System (VOLUME_VALIDATION_SYSTEM.md)

### Economic Model (3)
- [ ] Universal-Basic-Income (O_COIN_PERFECT_UBI_CANDIDATE.md)
- [ ] Immigration-Impact (O_UBI_REVERSES_IMMIGRATION.md)
- [ ] Climate-Change-Solution (O_COIN_FIGHTS_CLIMATE_CHANGE.md)

### Currency Coverage (4)
- [ ] Currency-Registry (CURRENCY_REGISTRY.md)
- [ ] Country-Mapping (COMPLETE_CURRENCY_COUNTRY_MAPPING.md)
- [ ] Coverage-Summary (GLOBAL_COVERAGE_SUMMARY.md)
- [ ] Regional-Breakdown (CURRENCY_REGISTRY_SUMMARY.md)

### Scalability (5)
- [ ] Scalability-Overview (create overview)
- [ ] User-Registry-Scalability (USER_REGISTRY_SCALABILITY_ANALYSIS.md)
- [ ] Measurement-Scalability (MEASUREMENT_SCALABILITY_ANALYSIS.md)
- [ ] Transaction-Volume-Analysis (GLOBAL_TRANSACTION_VOLUME_ANALYSIS.md)
- [ ] High-TPS-Strategy (HIGH_TPS_SCALING_STRATEGY.md)
- [ ] Lightning-Layer2-Feasibility (LIGHTNING_LAYER2_FEASIBILITY.md)

### Technical (2)
- [ ] Currency-Exchange (O_CURRENCY_EXCHANGE_EXPLAINED.md)
- [ ] RPC-Commands (create from code/docs)

### Project Status (5)
- [ ] Implementation-Status (IMPLEMENTATION_STATUS.md)
- [ ] Deployment-Ready (DEPLOYMENT_READY.md)
- [ ] Integration-Summary (INTEGRATION_SUMMARY.md)
- [ ] Compilation-Success (COMPILATION_SUCCESS.md)
- [ ] Project-Overview (PROJECT.md)

### Research (2)
- [ ] Blockchain-Simulation (O_BLOCKCHAIN_SIMULATION_PAPER.md)
- [ ] Developer-Strategy (DEVELOPER_PROMOTION_STRATEGY.md)

### Community (3)
- [ ] Contributing (CONTRIBUTING.md)
- [ ] FAQ (create from various docs)
- [ ] Contact (create new)

## After Migration Complete

1. Replace main README.md with README_NEW.md
2. Archive or delete migrated .md files from main repo
3. Commit changes
4. Push to GitHub
5. Announce wiki launch to community

---

**Start with Step 1 above - create the first page manually through GitHub!**

