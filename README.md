# Bitaxe G6 Brain ⚡

**v1.0 Beta** — Professional Adaptive Recursive Least Squares (RLS) Controller for Bitaxe ESP-Miner (BM1370)

The G6 Brain dynamically models the quadratic relationship between frequency, voltage, and hashrate using Recursive Least Squares with Bierman-Thornton U-D factorization for guaranteed numerical stability. It continuously adapts to the unique characteristics of each ASIC while enforcing strict hardware safety constraints.

This is the core module of the **Bitaxe Brains Project** — built for long-term reliability, modularity, and production use.

---

## Key Features

- Quadratic response surface modeling with gradient-based Variable Forgetting Factor
- Bierman-Thornton U-D Factorization for robust covariance updates in single-precision floating-point
- Hessian-guarded analytical optimum solver with negative-definite check
- Sample quality state machine ensuring only valid, settled telemetry is used
- Efficiency-focused optimization (J/GH) with fail-closed auto-apply logic
- Slew-rate limiting and BM1370-specific safe operating limits
- NVS silicon fingerprinting with periodic save for per-chip warm-start
- Functional safety interlocks (thermal derating, voltage ripple, NER handling)
- Clean modular interface for future brain variants

---

## Phase 1 Complete (May 2026)

All critical improvements identified in independent technical audits have been implemented:
- Corrected Bierman-Thornton UD Factorization (canonical algorithm)
- Fixed C scoping issues and build warnings
- Functional safety layer with real derating logic
- Periodic NVS persistence
- Hessian validation and slew-rate protection

The core RLS engine is now numerically stable and hardware-safe for continuous 24/7 operation.

---

## Roadmap

### Phase 1 — Core RLS Hardening **(Completed)**
- Bierman-Thornton U-D Factorization
- Sample quality state machine
- Safety interlocks and fail-closed logic
- NVS warm-start with periodic save
- BM1370 tuning and clamps

### Phase 2 — Advanced Control & Validation (Next)
- Advanced telemetry buffer and differential sampling
- Controlled exploration policy (UCB)
- Full PID fan control with anti-windup
- Comprehensive unit and hardware-in-loop testing

### Phase 3 — Modularity & Ecosystem
- Ghost Brain (external MCU) support
- Multi-ASIC support

### Phase 4 — Production
- Automated test suite and long-term stability validation
- Official v1.0 release

---

## Installation & Usage

1. Add the `g6_brain` component to your ESP-Miner project.
2. Enable it via `menuconfig` → "G6 Brain Configuration".
3. Initialize with `g6_brain_init()` and call `g6_brain_update()` in the main control loop.

Detailed build instructions, Kconfig options, and integration examples are in the `docs/` folder.

---

## Contributing

Pull requests are welcome. Please:
- Preserve the modular `G6BrainInterface`
- Ensure `g6_brain_self_test()` continues to pass
- Keep code clean, well-documented, and numerically sound

---

## License

MIT License — see [LICENSE](LICENSE) for details.

---

Built for the Bitaxe community with focus on mathematical correctness, reliability, and hardware safety.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Language: C](https://img.shields.io/badge/Language-C-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-red.svg)](https://www.espressif.com/en/products/socs/esp32)

**G6 Brain v1.0 Beta** — Professional adaptive control for your Bitaxe.  
*With ❤️ and rigorous engineering by 6ummy & Grok • May 2026*
