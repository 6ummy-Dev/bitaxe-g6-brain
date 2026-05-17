# G6 Brain Documentation — v1.0.0-beta2

**Advanced RLS-based self-optimizing module for Bitaxe ESP-Miner (Gamma 602+)**

Welcome to the official documentation for the **G6 Brain** — the flagship module of the Bitaxe Brains Project.

This is a fully modular, production-hardened control brain that uses **Recursive Least Squares (RLS) quadratic response surface modeling** to dynamically optimize frequency and voltage while enforcing strict aerospace-grade safety constraints.

---

## Quick Start

1. Read **[INSTALL.md](INSTALL.md)** — add the component and enable it
2. Use the **recommended integration example**: `docs/INTEGRATION_EXAMPLE.c`
3. Adapt it into your firmware and start in `OBSERVE_ONLY` or `RECOMMEND` mode
4. Monitor the logs. The brain will begin learning within minutes.

---

## Documentation Map

| Document                        | Purpose                                           | When to read |
|--------------------------------|---------------------------------------------------|--------------|
| **[INSTALL.md](INSTALL.md)**        | Installation & integration guide                  | Adding the brain to your firmware |
| `INTEGRATION_EXAMPLE.c`             | **Recommended integration example** (main)        | Starting point for real integration |
| **[API.md](API.md)**                | Full public API reference                         | Need function details |
| **[KCONFIG.md](KCONFIG.md)**        | All configuration options explained               | Tuning safety limits or debugging |
| **[SAFETY.md](SAFETY.md)**          | Safety mechanisms & unhappy-path engineering      | Understanding how the brain protects hardware |

**Additional project docs (root):**
- `AGENTS.md` — Safety invariants & engineering principles
- `GLOSSARY.md` — Terminology
- `CHANGELOG.md` — Version history

---

## What Makes G6 Brain Different

- Pure RLS quadratic modeling (fully explainable)
- Strong predictive safety layers
- Per-chip NVS fingerprinting (warm start)
- Modular and auditable design
- Aerospace-style QA hardening

---

## Status

- **Version:** v1.0.0-beta2 (May 2026)
- **Maturity:** Beta — ready for community field testing
- **License:** MIT

---

**The brain your Bitaxe always wanted.**  
Start safe. Learn. Then optimize. ⚡
