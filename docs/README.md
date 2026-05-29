# G6 Brain ⚡ Documentation — v1.0.0-beta6

Welcome to the official documentation for the **G6 Brain** — a modular adaptive RLS quadratic optimizer for real-time J/TH scaling on the BM1370.

A clean, modular ESP-IDF control component that uses **Recursive Least Squares (RLS) quadratic response surface modeling** to dynamically optimize frequency and voltage while enforcing strict hardware safety constraints. Engineered for fail-closed safety, numerical stability, and educational clarity.

---

## Quick Start
1. Read **[INSTALL.md](INSTALL.md)** — add the component and enable it.
2. Use the recommended integration example: `docs/INTEGRATION_EXAMPLE.c`.
3. Start in `OBSERVE_ONLY` or `RECOMMEND` mode.
4. Monitor the logs. The brain will begin learning within minutes.

---

## Documentation Map

| Document                        | Purpose                                           | When to read |
|--------------------------------|---------------------------------------------------|--------------|
| [INSTALL.md](INSTALL.md)        | Installation & integration guide                  | Adding the brain to your firmware |
| `INTEGRATION_EXAMPLE.c`         | Recommended integration example                   | Starting point for real integration |
| [API.md](API.md)                | Full public API reference                         | Need function details |
| [KCONFIG.md](KCONFIG.md)        | All configuration options explained               | Tuning safety limits or debugging |
| [SAFETY.md](SAFETY.md)          | Safety mechanisms & full `G6SafetyStatus` reference | Understanding how the brain protects hardware |
| [MONITORING.md](MONITORING.md)  | Real-time observability and telemetry             | Monitoring brain health |
| [TESTING.md](TESTING.md)        | Community testing guide                           | Field testing guidance |
| [AGENTS.md](AGENTS.md)          | Engineering principles & safety invariants        | Contributing code or reviewing PRs |
| [GLOSSARY.md](GLOSSARY.md)      | Terminology used across code and docs             | Looking up a term you've seen |
| [REFERENCES.md](REFERENCES.md)  | Scientific & mathematical foundations             | Understanding the RLS/Dinkelbach math |

**Project root:**

- [`CHANGELOG.md`](../CHANGELOG.md) — Version history
- [`MANIFESTO.md`](../MANIFESTO.md) — Project philosophy and non-negotiables

---

## What Makes G6 Brain Different

- Pure RLS quadratic modeling with separate hashrate and power surfaces — fully explainable.
- Full Joseph-form covariance updates (symmetric congruence + measurement-noise `k kᵀ` injection + ridge + symmetrize + clamp) — the exact RLS posterior covariance, numerically robust.
- 3-sigma statistical outlier gating on both estimators.
- **Fail-closed safety contract** — every numeric input (including NaN, Inf, out-of-bounds) routes to the safety layer. `ESP_ERR_INVALID_ARG` returns only on `brain == NULL`.
- **P-matrix divergence auto-recovery** — when the estimator goes singular, the brain re-cold-starts while preserving operator-configured state.
- **Two-tier thermal safety** — independent ASIC and VR temperature protection with configurable proactive margins.
- **Bounded Dinkelbach J/TH solver** — $O(1)$-per-step fractional minimization (exact closed form for interior optima; clamped boundary point otherwise), gated on both model qualities ≥ 0.6.
- Per-chip NVS fingerprinting (warm start across power cycles, with schema versioning and bad-blob auto-erase).
- Clean `G6BrainTelemetry` snapshot for monitoring and dashboards.
- Modular, single-threaded, swappable component.

---

## Status

- **Version:** v1.0.0-beta6 (May 2026)
- **Maturity:** Beta — ready for field testing (48h+ soak recommended before v1.0 tag)
- **License:** MIT

---

**The brain your Bitaxe always wanted.** Start safe. Learn. Then optimize. ⚡
