# Bitaxe G6 Brain ⚡

**v1.0.0-beta1** — Adaptive stabilized RLS optimizer with real-time quadratic efficiency modeling for BM1370

The G6 Brain dynamically models the quadratic relationship between frequency, voltage, and hashrate in real time using a stabilized conventional RLS (P-matrix with symmetrization, ridge regularization, and trace monitoring). It continuously learns the unique efficiency surface of your ASIC and selects the most efficient operating point (J/TH) while maintaining strict numerical stability and hardware safety constraints.

This is the flagship module of the **Bitaxe Brains Project** — a clean, modular architecture designed for long-term reliability and extensibility.

---

## Key Features

- Stabilized conventional RLS with Variable Forgetting Factor (gradient-based VFF), innovation gating, and covariance trace monitoring
- Hessian-guarded analytical optimum solver with negative-definite validation
- Fully self-contained safety (proactive thermal derating, voltage ripple/undershoot detection, NER handling) — zero external dependencies
- Sample quality state machine (8 s settle + 5 s measure window) ensuring only high-quality, settled telemetry is used
- Efficiency optimization (J/TH) with fail-closed auto-apply logic and slew-rate limiting
- BM1370-specific safe operating limits (400–950 MHz, 1050–1350 mV)
- NVS silicon fingerprinting (theta + full covariance P matrix) with warm-start per chip
- Production-hardened defensive programming (isfinite guards on all inputs, covariance symmetrization + clamping, proper cold-start initialization, NVS readiness check)

---

## v1.0.0-beta1 Release (May 2026)

All major stability, safety, and adaptation improvements identified in independent technical audits have been implemented. The core RLS engine has been extensively reviewed and hardened for 24/7 operation. **This is a beta release — it has not yet been deployed in large-scale production.** Community field testing is strongly encouraged.

**Major improvements in this release:**
- Complete consolidation of all safety logic into a single self-contained `g6_brain.c` (no more `g6_safety.c/h`)
- Proper cold-start RLS initialization (fixed zero-covariance bug that prevented adaptation)
- Single 8-second settle timing with correct `MIN_WINDOW_MS` measurement phase
- `goto safety_layer` pattern + always-on clamps and proactive scaling
- Efficiency correctly reported in **J/TH** (energy per terahash)
- NVS now persists full covariance matrix for true warm-start of both model and uncertainty
- Lambda guard + trace_P check to prevent covariance collapse

See [CHANGELOG.md](CHANGELOG.md) for the complete list of changes, fixes, and audit responses.

---

## Roadmap

### Phase 1 — Core RLS Hardening **(Completed)**
- Stabilized quadratic RLS modeling with numerical robustness (symmetrize + ridge + trace guard)
- Sample quality state machine and telemetry validation
- Efficiency optimization (J/TH) and fail-closed logic
- NVS silicon fingerprint + covariance warm-start
- BM1370 tuning and hard safety limits
- Defensive programming hardening (isfinite, clamps, NVS guard)

### Phase 2 — Advanced Telemetry & Control (Next)
- Active thermal slope detection (`MAX_TEMP_SLOPE`)
- Controlled exploration policy for model freshness
- Full PID fan integration with anti-windup
- Extended hardware soak testing + long-term stability data

### Phase 3 — Ecosystem & Modularity
- Full "Ghost Brain" external MCU / multi-ASIC support
- Pluggable optimizer interface

### Phase 4 — Production Release (v1.0)
- Comprehensive unit test suite (Unity) and CI
- Community validation and 30+ day soak data
- Official stable v1.0 release

---

## Installation & Usage

1. Add the `g6_brain` component to your ESP-Miner project (copy or git submodule).
2. Enable it via `menuconfig` → **Component config → G6 Brain Configuration**.
3. Call `nvs_flash_init()` **before** `g6_brain_init()`. The brain will return a clear `ESP_ERR_NVS_NOT_INITIALIZED` error if NVS is not ready.
4. Feed telemetry via `g6_brain_update()` in your main control loop (recommended every 20–30 s).

Detailed build instructions, Kconfig reference, integration examples, and troubleshooting are in the `docs/` folder.

---

## Contributing

Pull requests are welcome. Please:
- Preserve the public API and `G6BrainState` layout for backward compatibility
- Ensure `g6_brain_self_test()` continues to pass (symmetry + P diagonal bounds)
- Keep code clean, well-commented, and defensive

---

Built for the Bitaxe community with focus on mathematical correctness, reliability, and hardware safety.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Language: C](https://img.shields.io/badge/Language-C-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-red.svg)](https://www.espressif.com/en/products/socs/esp32)

**G6 Brain v1.0.0-beta1** — The brain your Bitaxe always wanted.  
*With ❤️ and rigorous engineering • May 2026*
