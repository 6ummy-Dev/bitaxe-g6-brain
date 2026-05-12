# Bitaxe G6 Brain ⚡

**v1.0.0-beta1** — Adaptive RLS optimizer with stable real-time quadratic efficiency modeling for BM1370

The G6 Brain dynamically models the quadratic relationship between frequency, voltage, and hashrate in real time. It continuously learns the unique efficiency surface of your ASIC and selects the most efficient operating point while maintaining strict numerical stability and hardware safety constraints.

This is the flagship module of the **Bitaxe Brains Project** — a clean, modular architecture designed for long-term reliability and extensibility.

---

## Key Features

- Adaptive RLS with Variable Forgetting Factor (VFF) for fast adaptation to silicon changes
- Hessian-guarded analytical optimum solver with negative-definite validation
- Fully self-contained safety (thermal derating, voltage ripple, NER handling) — no external dependencies
- Sample quality state machine (8s settle + 5s measure window) ensuring only high-quality telemetry is used
- Efficiency optimization (J/TH) with fail-closed auto-apply logic
- Slew-rate limiting and BM1370-specific safe operating limits
- NVS silicon fingerprinting with warm-start capability per chip
- Extensively reviewed with NASA Level C-style defensive programming (isfinite guards, covariance stabilization, proper cold-start initialization)

---

## v1.0.0-beta1 Release (May 2026)

All major stability, safety, and adaptation improvements identified in independent technical audits have been implemented. The core RLS engine has been extensively reviewed and NASA Level C hardened. **This is a beta release — it has not yet been deployed in production.** Community field testing is encouraged.

**Major improvements in this release:**
- Complete consolidation of safety logic into `g6_brain.c` (g6_safety.c/h no longer required)
- Proper cold-start RLS initialization (fixed zero covariance bug)
- Single 8-second settle timing with correct `MEASURE_WINDOW` duration
- `goto safety_layer` pattern ensuring clamps always execute
- Efficiency now correctly reported in **J/TH**
- NVS initialization guard + lambda guard

See [CHANGELOG.md](CHANGELOG.md) for the full list of changes and fixes.

---

## Roadmap

### Phase 1 — Core RLS Hardening **(Completed)**
- Adaptive quadratic RLS modeling with numerical stability
- Sample quality state machine and telemetry validation
- Efficiency optimization and fail-closed logic
- NVS silicon fingerprint warm-start
- BM1370 tuning and hard safety limits
- NASA Level C hardening

### Phase 2 — Advanced Telemetry & Control (Next)
- Active thermal slope detection (`MAX_TEMP_SLOPE`)
- Controlled exploration policy
- Full PID integration with anti-windup
- Extended hardware soak testing

### Phase 3 — Ecosystem & Modularity
- Full "Ghost Brain" external MCU support
- Multi-ASIC capability

### Phase 4 — Production Release (v1.0)
- Comprehensive test suite and documentation
- Community validation and long-term stability data
- Official stable v1.0 release

---

## Installation & Usage

1. Add the `g6_brain` component to your ESP-Miner project.
2. Enable it via `menuconfig` → **G6 Brain Configuration**.
3. Call `nvs_flash_init()` first, then `g6_brain_init()` and feed telemetry via `g6_brain_update()` in your main control loop.

Detailed build instructions, Kconfig options, and integration examples are in the `docs/` folder.

---

## Contributing

Pull requests are welcome. Please:
- Preserve the public API
- Ensure `g6_brain_self_test()` continues to pass
- Keep code clean and well-documented

---

Built for the Bitaxe community with focus on mathematical correctness, reliability, and hardware safety.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Language: C](https://img.shields.io/badge/Language-C-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-red.svg)](https://www.espressif.com/en/products/socs/esp32)

**G6 Brain v1.0.0-beta1** — The brain your Bitaxe always wanted.  
*With ❤️ and rigorous engineering • May 2026*
