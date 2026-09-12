# O Blockchain

> A water-price-based stablecoin system — **early-stage prototype, looking for contributors**

[![CI](https://github.com/cno127/o-blockchain/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/cno127/o-blockchain/actions/workflows/ci.yml)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](COPYING)
[![Status](https://img.shields.io/badge/status-prototype-yellow.svg)]()
[![Wiki](https://img.shields.io/badge/docs-wiki-blue.svg)](https://github.com/cno127/o-blockchain/wiki)
[![Bitcoin Core](https://img.shields.io/badge/based%20on-Bitcoin%20Core%2027.0-orange.svg)](https://github.com/bitcoin/bitcoin)

## ⚠️ Development Status — read this first

**This is a research prototype, not production software.** It exists to explore
an idea, gather feedback, and attract collaborators. We are looking for
contributors, not customers.

- 🔬 **Stage**: architectural exploration on a Bitcoin Core 27.0 fork
- 🧪 **Consensus code**: several consensus-critical problems are known and
  **openly tracked** — see [Current Status & Open Problems](#-current-status--open-problems)
- 📏 **No performance claims**: we publish measured numbers or nothing.
  Correctness comes before throughput.
- 📝 **Feedback and contributions welcome** — the open problems below are the
  best entry points

## 🌍 What is O Blockchain?

O Blockchain explores a monetary system where value is anchored to a universal
basic need instead of fiat or scarcity.

### Core Principles

- **💧 Water-price stability**: 1 O = the price of 1 liter of bottled water in
  the local market
- **🌍 One O currency per national currency** (O_USD, O_EUR, O_JPY, …) — 142
  currencies defined
- **📏 Measurement, not oracles**: users and bots submit water-price and
  exchange-rate measurements on-chain; validated by humans, averaged
  statistically
- **⚖️ Stabilization by incentives**: when market rates drift from measured
  water-price rates, new coins are minted to users of *stable* currencies,
  diluting unstable ones — the offender's sanction becomes the reward of the
  offended
- **👤 Proof of personhood**: one person = one identity, verified through
  multiple accepted methods (government ID, video call, document review,
  biometrics, third-party KYC — BrightID among them)
- **🔓 Open governance**: economic parameters are community decisions, not
  developer decisions

The design rationale is documented in the
[O article series](https://o.international/articles/) and in the
[GitHub Wiki](https://github.com/cno127/o-blockchain/wiki).

## 🧭 Current Status & Open Problems

Honest snapshot (August 2026). Active work happens on the
[`ARCH-GlobalPaymentsV1`](https://github.com/cno127/o-blockchain/tree/ARCH-GlobalPaymentsV1)
branch; a full spec-conformance audit lives at
[`doc/SPEC_CONFORMANCE_AUDIT_2026-08.md`](https://github.com/cno127/o-blockchain/blob/ARCH-GlobalPaymentsV1/doc/SPEC_CONFORMANCE_AUDIT_2026-08.md).

### What works today

- Builds on Linux via CMake; CI (`.github/workflows/ci.yml`) builds `main`,
  runs the unit-test suites and the O regtest functional tests on every PR
- Multi-currency UTXO data structures and O-specific transaction types
  (user verification, water price, exchange rate, validation, invitations)
- Measurement submission → validation → statistical averaging → stability
  detection → stabilization minting, end-to-end **in regtest**
- Deterministic, consensus-seeded random draws for stabilization recipients
  and endorser selection (recently fixed — verification tests still wanted,
  see issue [#3](https://github.com/cno127/o-blockchain/issues/3))

### Known open problems (help wanted)

These are tracked as GitHub issues with `open-problem` labels, file:line
pointers, and concrete deliverables. Claim one by commenting.

| Issue | Problem | Severity |
|-------|---------|----------|
| [#3](https://github.com/cno127/o-blockchain/issues/3) | Verify deterministic recipient selection + adversarial tests | consensus-critical |
| [#4](https://github.com/cno127/o-blockchain/issues/4) | Stabilization validation paths need consolidation/verification | consensus-critical |
| [#5](https://github.com/cno127/o-blockchain/issues/5) | Monetary state is RAM-only — no persistence, no reorg rollback | consensus-critical |
| [#6](https://github.com/cno127/o-blockchain/issues/6) | Floating point in consensus-path mint sizing | consensus-critical |
| [#7](https://github.com/cno127/o-blockchain/issues/7) | Define "transaction volume" for mint sizing (decided: L1-only, Bitcoin-style) | design |
| [#8](https://github.com/cno127/o-blockchain/issues/8) | Wire `CMultiCurrencyTxOut` into the live transaction path | enhancement |
| [#9](https://github.com/cno127/o-blockchain/issues/9) | Stratified, objective-driven measurement invitation sampling | enhancement |
| [#10](https://github.com/cno127/o-blockchain/issues/10) | FX-triangle consistency + per-currency conservation tests | good first issue |
| [#11](https://github.com/cno127/o-blockchain/issues/11) | Imputed reference rates for unmeasurable regions | enhancement |
| [#12](https://github.com/cno127/o-blockchain/issues/12) | Monetary simulation: does incentive stabilization hold the peg? | good first issue — **no C++ required**, ideal for economists/data scientists |

See the full list: [open issues](https://github.com/cno127/o-blockchain/issues).

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

See [INSTALL.md](INSTALL.md) for platform-specific build instructions.

## 📊 How It Works

1. **Water-price measurement & rate setting**: users and bots measure bottled
   water prices (0.9–1.1 L containers) in local fiat currencies, with URL or
   photo+GPS proof, validated by humans. The statistical average of accepted
   measurements sets each O currency's value; cross-currency rates derive from
   those values.
2. **Stability monitoring**: observed market exchange rates are compared with
   the theoretical (water-price) rates to classify each currency as stable or
   unstable.
3. **Stabilization**: deviations trigger minting to holders of stable
   currencies, creating economic pressure back toward the peg. How the mint is
   sized and whether a burn path exists are open design questions
   ([#6](https://github.com/cno127/o-blockchain/issues/6),
   [#7](https://github.com/cno127/o-blockchain/issues/7)).
4. **Proof of personhood**: one person = one identity, enforced through
   endorsement-based user verification supporting several methods (government
   ID, video call, document review, biometrics, third-party KYC such as
   BrightID).
5. **Universal Basic Income** remains a **long-term vision** contingent on the
   above being proven correct; amounts would be community-governed.

### Technical Stack

- **Base**: Bitcoin Core 27.0 (C++)
- **Consensus**: hybrid PoW / PoB (proof-of-personhood-weighted), under
  active design
- **Model**: multi-currency UTXO
- **Identity**: endorsement-based proof of personhood with pluggable
  verification methods (government ID, video call, document review,
  biometrics, third-party KYC incl. BrightID)
- **Volume accounting**: L1 on-chain output value per currency only — no
  L2-reported volume in any consensus formula (decided; see
  [#7](https://github.com/cno127/o-blockchain/issues/7))

## 📚 Documentation

- **[GitHub Wiki](https://github.com/cno127/o-blockchain/wiki)** — architecture,
  measurement system, transaction types, stabilization, currency coverage,
  testing guide, RPC reference
- **[Spec-conformance audit (2026-08)](https://github.com/cno127/o-blockchain/blob/ARCH-GlobalPaymentsV1/doc/SPEC_CONFORMANCE_AUDIT_2026-08.md)** —
  what the code actually does vs. what the spec says, with file:line evidence
- **[O article series](https://o.international/articles/)** — the design
  rationale and economic model, including why correctness comes before
  throughput

## 🤝 Contributing

We welcome contributions from everyone — C++ engineers, testers, economists,
data scientists, translators, and technical writers.

**Best starting points:**

1. Pick an [open problem](https://github.com/cno127/o-blockchain/issues) —
   each has file:line pointers and a concrete deliverable; claim it by
   commenting
2. No C++? Issue [#12](https://github.com/cno127/o-blockchain/issues/12)
   (monetary simulation) needs modeling skills, not systems programming
3. Improve the wiki or add tests (regtest guide in the
   [Testing Guide](https://github.com/cno127/o-blockchain/wiki/Testing-Guide))

```bash
# Fork, branch, commit, PR
git clone https://github.com/YOUR_USERNAME/o-blockchain.git
cd o-blockchain
git checkout -b feature/your-feature-name
# ... make changes ...
git push origin feature/your-feature-name
```

CI runs automatically on every pull request (Linux build + unit tests + O
functional tests) — check the Actions tab before requesting review. See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## 📞 Community

- **📚 Wiki**: [Documentation](https://github.com/cno127/o-blockchain/wiki)
- **🐙 GitHub**: [Issues](https://github.com/cno127/o-blockchain/issues) & [Discussions](https://github.com/cno127/o-blockchain/discussions)
- **🌍 Website**: [o.international](https://o.international)
- **💼 LinkedIn**: [O International](https://www.linkedin.com/company/o-international)
- **📺 YouTube**: [O International Channel](https://www.youtube.com/@OInternational)

## ❓ FAQ

**Q: Why water price as the peg?**
A: Water is a universal basic need with consistent relative value globally.
Each O currency targets 1 O = 1 liter of bottled water in its local market;
exchange rates between O currencies derive from water-price ratios. Whether
the incentive mechanism actually holds this peg is an open research question
we are actively trying to answer
([#12](https://github.com/cno127/o-blockchain/issues/12)).

**Q: Why 142 currencies instead of one?**
A: Price levels differ by country. Each O currency targets the water price in
its own market, so O currencies have different values from each other. O_ONLY
is a universal currency exchanging 1:1 with all O currencies while keeping the
water-price peg through measurement and stabilization.

**Q: What about Universal Basic Income?**
A: A long-term vision, not a current feature. It would require the consensus
layer to be proven correct first, plus governance mechanisms that do not exist
yet. Any amount would be set by community governance.

**Q: Is unlimited supply inflationary?**
A: The design intends value to track water (a constant need) rather than
scarcity, with stabilization minting counteracting drift. Whether that holds
in practice is exactly what the monetary simulation work
([#12](https://github.com/cno127/o-blockchain/issues/12)) is meant to test.

**Q: How do you prevent fake accounts?**
A: Proof of personhood: one person = one identity. Several verification
methods are accepted — government ID, video call, document review,
biometrics, and third-party KYC providers such as BrightID (decentralized
social-graph verification).

More on the [Wiki FAQ](https://github.com/cno127/o-blockchain/wiki/FAQ).

## 📄 License

MIT — see [COPYING](COPYING).

## 🙏 Acknowledgments

- **Bitcoin Core** for the foundation
- **BrightID** and other identity providers for proof of personhood
- **Contributors** — including the ones we hope this README brings in

---

*An honest prototype exploring money anchored to human needs. If the open
problems above look interesting, we'd love your help.* 🌍
