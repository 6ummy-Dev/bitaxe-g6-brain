# G6 Brain Documentation — v1.0.0-beta4

**Advanced RLS-based self-optimizing module for Bitaxe ESP-Miner (Gamma 602+)**

Welcome to the official documentation for the **G6 Brain** — an modular adaptive RLS brain with analytical J/TH optimization for BM1370.

This is a clean, modular ESP-IDF control component that uses **Recursive Least Squares (RLS) quadratic response surface modeling** to dynamically optimize frequency and voltage while enforcing strict hardware safety constraints.

In **beta4**, the brain introduces **two-tier thermal safety**, distinguishing between ASIC die temperature and voltage regulator (VR) temperature for more robust protection.

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
| [SAFETY.md](SAFETY.md)          | Safety mechanisms & unhappy-path engineering      | Understanding how the brain protects hardware |
| [MONITORING.md](MONITORING.md)  | Real-time observability and telemetry             | Monitoring brain health |
| [TESTING.md](TESTING.md)        | Community testing guide                           | Field testing guidance |

**Additional project docs (root):**
- `AGENTS.md` — Safety invariants & engineering principles
- `GLOSSARY.md` — Terminology
- `CHANGELOG.md` — Version history
- `MANIFESTO.md` — Project philosophy

---

## What Makes G6 Brain Different

- Pure RLS quadratic modeling (fully explainable)
- Joseph-form covariance stabilization (numerically robust)
- 3-sigma statistical outlier gating
- Per-chip NVS fingerprinting (warm start)
- Clean telemetry export API
- **Two-tier thermal safety** (beta4) — Separate handling for ASIC and VR temperatures
- Modular and auditable design
- Hardened through multiple QA review cycles

---

## Status

- **Version:** v1.0.0-beta4 (May 2026)
- **Maturity:** Beta — preparing for field testing
- **QA:** Code verified, documentation updates in progress
- **License:** MIT

---

**The brain your Bitaxe always wanted.** Start safe. Learn. Then optimize. ⚡
