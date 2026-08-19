# O Blockchain — Spec-Conformance Audit (August 2026)

**Scope:** Does the code match the specifications published in the article series?
Primary specs audited against:

- **Article 19** — *Why Proof of Work Was Built for Security, Not Speed* (scalability architecture: conservative L1, shared multi-currency L2 executor, atomic FX, honest trust claims, "Correctness Comes Before Throughput" checklist)
- **Article 04** — *How to Secure Accurate Measurements in a Decentralized System Without Oracles* (invitation targeting, stratified sampling, Gaussian aggregation, indirect integrity, honest confidence states)
- **Article 20** — *Unlimited Supply, Balanced Books* (issuance rules "explicit, coded and publicly auditable"; no blank check)
- **Article 17 / site copy** — incentive-based stabilization ("the offender's sanction is the reward of the offended"), reference rates published not enforced.

**Branch audited:** `ARCH-GlobalPaymentsV1` (working tree, ahead of `main` only by untracked docs).
**Verdict in one line:** the *economic design* in the articles is faithfully described, but the code does not yet satisfy article 19's own "Correctness Comes Before Throughput" checklist — every item on that list is currently unmet, and two of them are consensus-breaking.

---

## A. Article 19 checklist vs. code (the core of this audit)

Article 19 commits O publicly to eight correctness items. Status of each:

| # | Article 19 commitment | Code status | Evidence |
|---|---|---|---|
| 1 | "deterministic stabilization decisions" | ❌ **FAIL (consensus-breaking)** | `src/consensus/stabilization_helpers.cpp:433` — `RandomSample()` uses an **unseeded** `FastRandomContext rng;` to pick reward recipients. Every node draws different recipients → stabilization txs differ per node → chain forks with >1 miner. Same class of bug at `user_consensus.cpp:442` (`std::shuffle(..., FastRandomContext())`) for validator/invite selection. |
| 2 | "fixed-point arithmetic instead of consensus-critical floating point" | ❌ **FAIL** | `stabilization_helpers.cpp:117–128` — mint amount computed as `double volume × double deviation × double factor` then cast to `CAmount`. `CalculateExchangeRateDeviation`, `CalculateStabilityRatio`, `CalculateDynamicStabilizationFactor` are all `double`. Platform/compiler FP differences can change minted amounts → consensus splits. |
| 3 | "durable monetary state that survives restart" | ❌ **FAIL (consensus-breaking)** | `stabilization_mining.h:201–202` — `m_currency_status` and `m_stabilization_txs` are plain in-RAM `std::map`s. Not in chainstate, not committed to headers, lost on restart. A restarted node no longer agrees which currencies are "unstable" or since when. |
| 4 | "correct rollback during chain reorganizations" | ❌ **FAIL** | No `DisconnectBlock` path touches stabilization state; `RecordStabilizationTransaction` (helpers `.cpp:296`) only ever adds. A reorg leaves minted-coin records and stability status from the orphaned branch in memory. |
| 5 | "a real multi-currency asset representation" | ❌ **FAIL** | `CMultiCurrencyTxOut` exists (`primitives/multicurrency_txout.cpp`) but stabilization rewards are paid via **legacy `CTxOut`** — acknowledged in-code at `stabilization_helpers.cpp:263–266` ("When multi-currency transactions are fully integrated, this should use CMultiCurrencyTxOut"). Value still flows as base coin. |
| 6 | "canonical transaction hashes that include every currency amount" | ❌ **FAIL** | Follows from #5 — currency id is not in the serialized value path of live payment outputs, so it cannot be part of the txid commitment. |
| 7 | "conservation tests for each currency" | ❌ **MISSING** | No per-currency conservation test exists in `src/test/` or `test/functional/`. `test_o_blockchain_sync.py` exercises the RPC flow, not monetary invariants. |
| 8 | "adversarial multi-node testing" | ❌ **MISSING** | No multi-node stabilization test. A 2-node regtest where both mine after a currency turns unstable would immediately expose bug #1. `Multi-Node-Testing.md` in the wiki describes intent, not an existing harness. |

### Additional consensus findings (same severity class)

- **Validation is a no-op.** `stabilization_helpers.cpp:465–466`:
  `ValidateStabilizationTransactions` returns `expected >= 0` — i.e., **always true**
  (it never inspects the block's actual stabilization txs). A miner can mint
  arbitrary stabilization outputs and every node accepts them. This directly
  contradicts article 20's "There is no blank check" and "issuance … coded and
  publicly auditable."
- **Wall-clock time in consensus objects.** `stabilization_helpers.cpp:284`
  uses `GetTime()` in the recorded stabilization record; anything consensus-adjacent
  must use `block.nTime`/median-time-past, never local clock.
- **Mint amount is volume × deviation, but volume is a stub.**
  `stabilization_helpers.cpp:194–207` — `GetTransactionVolumeInCurrency` returns
  `tx->GetValueOut()/10` ("assume 10% of Bitcoin volume"). So today's mint sizing is
  a placeholder number. This is the P1 "define transaction volume" decision —
  it becomes *architectural* once payments move to L2 (does L1 still see volume?).
  Article 19 implies volume will largely live on L2; the definition must be settled
  **before** the L2 executor is built.

## B. Article 04 (measurements) vs. code

Closer to spec than stabilization, but several claims are aspirational:

- ✅ Invitation-gated submission, expiry, per-currency targeting exist
  (`src/measurement/*`, invites every 10 blocks, daily targets 50–300/currency).
- ✅ Gaussian-weighted aggregation exists (`measurement_helpers.cpp`,
  `measurement_system.h`) with confidence tiers
  (`INSUFFICIENT_DATA / LOW_CONFIDENCE / HIGH_CONFIDENCE`,
  `measurement_system.h:242–244, 372–374`) — matches the article's "honest
  confidence" principle.
- ⚠️ **Stratified sampling / objective-driven targeting** (the article's central
  security claim: "who is asked is part of the security model") is only partially
  present: selection pools exist, but geographic strata, cross-jurisdiction
  bias and shortfall-driven invitation probability are not implemented as described.
- ⚠️ **FX-triangle closure / "impossible worlds" checks** — not found in code;
  plausibility bounds exist, triangular consistency does not.
- ⚠️ **Indirect integrity** (imputed values from foreign legs when a lane is
  unmeasurable) — `currency_disappearance_handling.cpp` covers currency
  *disappearance*, but the graph-based imputation with published uncertainty
  bands is not implemented.
- ⚠️ Selection randomness (`user_consensus.cpp:442`) has the same unseeded-RNG
  problem as stabilization wherever the draw must be network-agreed.

## C. Article 20 (issuance auditability) vs. code

- Stabilization issuance is neither validated (no-op validator, above) nor durably
  recorded (RAM-only history) → "publicly auditable" is not yet true in code.
- UBI / restoration issuance: not implemented (consistent with the articles, which
  present them as proposals — no action needed, just keep docs honest).
- Block reward: 2,800 OUSD/block (`01c0042df8`) — documented; fine.

## D. What the code does NOT need (per article 19)

- ❌ Do **not** shorten block time or inflate block size (wiki's High-TPS page
  suggesting 1–2 s blocks is contrary to the published spec — fixed in wiki).
- ❌ Do not start the L2 executor before items A1–A4 pass on a multi-node regtest.
- ❌ No per-currency chains, no channels-as-foundation.

## E. Recommended order of work (matches article 19 §"Not Impossible — Just Unfinished")

1. **P0-a — Deterministic recipient selection.** Seed selection from consensus
   data: `seed = SHA256(prev_block_hash ‖ currency_code ‖ height)` →
   `FastRandomContext(seed)`; sort candidate pubkeys canonically before the draw.
   Small, local patch (`RandomSample` gains a seed parameter; callers pass
   block/height/currency). Same treatment for `user_consensus.cpp:442`.
2. **P0-b — Real validation.** `ValidateStabilizationTransactions` must
   *recompute* the expected stabilization set and compare byte-for-byte with the
   block's stab txs. (Depends on P0-a, else "expected" differs per node.)
3. **P0-c — Persist + commit monetary state.** Move `m_currency_status` /
   `m_stabilization_txs` into a LevelDB keyed by block hash (like existing
   `o_brightid_db`), add Disconnect/undo handling, and commit a state hash into
   the coinbase (or a defined OP_RETURN) so divergence is detectable.
4. **P0-d — Fixed-point.** Replace consensus-path `double`s with int64 scaled
   arithmetic (e.g. deviation in parts-per-million), preserving the **signed**
   deviation (design decision: mint-only today; burn/negative path is an open
   economic question owed by Christophe).
5. **P1 — Define "transaction volume"** (L1-only vs L2-reported vs decouple) —
   economic decision, blocks the L2 spec.
6. **P2 — Tests:** per-currency conservation unit tests + a 2-node regtest that
   currently FAILS on the unseeded RNG (write it first; it's the proof).
7. **P3 — L2 shared executor** per `doc/o-global-payments-architecture-v1.md`.

Items 1, 2, 6 are safe pure-engineering patches (no economic choices embedded).
Items 3–5 need Christophe's sign-off on open questions (burn path, volume
definition, state-commitment location).

---

*Audit artifacts: this file. Companion architecture docs already in the tree:
`doc/o-global-payments-architecture-v1.md`, `O_SCALABILITY_ARCHITECTURE_SPIKE.md`,
`O_CONSENSUS_ARCHITECTURE_SUMMARY.md` (uncommitted).*
