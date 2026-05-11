# Bitaxe G6 Brain ⚡

**v1.0 Beta** — Adaptive RLS optimizer with stable real-time quadratic efficiency modeling for BM1370

The G6 Brain dynamically models the quadratic relationship between frequency, voltage, and hashrate in real time. It continuously learns the unique efficiency surface of your ASIC and selects the most efficient operating point while maintaining strict numerical stability and hardware safety constraints.

This is the flagship module of the **Bitaxe Brains Project** — a clean, modular architecture designed for long-term reliability and extensibility.

---

## Key Features

- Adaptive RLS with stable real-time quadratic efficiency modeling (J/GH objective)
- Bierman-Thornton U-D Factorization for numerical stability in single-precision floating-point
- Hessian-guarded analytical optimum solver with negative-definite validation
- Sample quality state machine ensuring only high-quality, settled telemetry is used
- Efficiency-focused optimization with fail-closed auto-apply logic
- Slew-rate limiting and BM1370-specific safe operating limits
- NVS silicon fingerprinting with periodic save for warm-start capability per chip
- Functional safety interlocks (thermal derating, voltage ripple, NER handling)

---

## Phase 1 Complete (May 2026)

All major stability, safety, and adaptation improvements identified in independent technical audits have been implemented. The core RLS engine is now production-hardened for continuous 24/7 operation.

---

## Roadmap

### Phase 1 — Core RLS Hardening **(Completed)**
- Adaptive quadratic RLS modeling with numerical stability
- Sample quality state machine and telemetry validation
- Efficiency optimization and fail-closed logic
- NVS silicon fingerprint warm-start
- BM1370 tuning and hard safety limits

### Phase 2 — Advanced Telemetry & Control (Next)
- Advanced telemetry buffer and differential sampling
- Controlled exploration policy
- Full PID integration with anti-windup
- Extended hardware soak testing

### Phase 3 — Ecosystem & Modularity
- Full "Ghost Brain" external MCU support
- Multi-ASIC capability

### Phase 4 — Production Release
- Comprehensive test suite and documentation
- Community validation and long-term stability data
- Official v1.0 release

---

## Installation & Usage

1. Add the `g6_brain` component to your ESP-Miner project.
2. Enable it via `menuconfig` → "G6 Brain Configuration".
3. Initialize with `g6_brain_init()` and feed telemetry via `g6_brain_update()` in the main control loop.

Detailed build instructions, Kconfig options, and integration examples are in the `docs/` folder.

---

## Contributing

Pull requests are welcome. Please:
- Preserve the modular interface
- Ensure `g6_brain_self_test()` continues to pass
- Keep code clean and well-documented

---

## License

MIT License — see [LICENSE](LICENSE) for details.

---

Built for the Bitaxe community with focus on mathematical correctness, reliability, and hardware safety.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Language: C](https://img.shields.io/badge/Language-C-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-red.svg)](https://www.espressif.com/en/products/socs/esp32)

**G6 Brain v1.0 Beta** — The brain your Bitaxe always wanted.  
*With ❤️ and rigorous engineering by 6ummy & Grok • May 2026*
