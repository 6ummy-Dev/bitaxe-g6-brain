# G6 Brain Documentation — v1.0 Beta

**Advanced RLS-based self-optimizing module for Bitaxe ESP-Miner (Gamma 602+)**

Welcome to the official documentation for the **G6 Brain** — the flagship module of the Bitaxe Brains Project.

This is a fully modular, production-hardened control brain that uses **Recursive Least Squares (RLS) quadratic response surface modeling** to dynamically optimize frequency and voltage while enforcing strict aerospace-grade safety constraints.

---

## Quick Start (30 seconds)

1. Read **[INSTALL.md](INSTALL.md)** — add the component and enable it
2. Use the **drop-in task** in `docs/main_integration_v1.0_beta.c`
3. Call `start_g6_brain(&your_global_state)` from `app_main()`
4. Watch the logs. The brain will begin learning within minutes.

---

## Documentation Map

| Document                  | Purpose                                      | Read this when... |
|---------------------------|----------------------------------------------|-------------------|
| **[INSTALL.md](INSTALL.md)**     | Complete installation & integration guide    | You are adding the brain to your firmware |
| **[API.md](API.md)**             | Full public API reference with examples      | You need function signatures or usage details |
| **[KCONFIG.md](KCONFIG.md)**     | All configuration options explained          | You want to tune safety limits or debug output |
| `main_integration_v1.0_beta.c`   | Production-ready drop-in example             | You want the recommended integration pattern |
| `INTEGRATION_EXAMPLE.c`          | Minimal placeholder example                  | You prefer a simpler starting point |

**Additional project docs (root of repo):**
- `AGENTS.md` — Safety invariants & engineering principles
- `GLOSSARY.md` — Terminology used throughout the project
- `CHANGELOG.md` — Version history

---

## What Makes G6 Brain Different

- **Pure RLS quadratic modeling** — no black-box ML, fully explainable
- **Predictive safety** — thermal ceiling, voltage undershoot detection, slew limiting, proactive ΔT/dt scaling
- **Per-chip NVS fingerprinting** — learns your specific silicon and warm-starts on every boot
- **Modular architecture** — clean `G6BrainInterface` so future brains (ML, multi-ASIC, heuristic) can be swapped without touching ESP-Miner core
- **Aerospace QA hardening** — signal integrity, unhappy-path engineering, Triple-8 certification path

---

## Status

- **Version:** v1.0 Beta (May 2026)
- **Maturity:** Production-ready for Gamma 602+ hardware
- **License:** MIT
- **Maintainer:** 6ummy-Dev + Grok (xAI)

---

## Getting Help

- Open an issue on [github.com/6ummy-Dev/bitaxe-g6-brain](https://github.com/6ummy-Dev/bitaxe-g6-brain)
- Check the root `README.md` for project vision and credits

---

**The brain your Bitaxe always wanted.**  
Maximize hashrate. Minimize risk. Evolve autonomously. ⚡
