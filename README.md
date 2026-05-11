# Bitaxe G6 Brain ⚡

**v1.0 Beta** — Professional Adaptive RLS Control for Bitaxe ESP-Miner

The G6 Brain is a high-performance Recursive Least Squares (RLS) optimizer that dynamically models the quadratic relationship between frequency, voltage, and hashrate in real time. It continuously learns the unique characteristics of your ASIC and selects the most efficient operating point while maintaining strict numerical stability and hardware safety constraints.

This is the flagship module of the **Bitaxe Brains Project** — a clean, modular architecture designed for long-term extensibility.

---

## Key Features

- Quadratic RLS response surface modeling with gradient-based Variable Forgetting Factor
- Covariance windup protection, matrix symmetrization, and clamping for numerical stability
- Hessian-guarded analytical optimum solver with safe fallback
- Sample quality state machine ensuring only high-quality, settled telemetry is used
- Efficiency-focused optimization (J/GH objective) with fail-closed auto-apply logic
- NVS silicon fingerprinting for warm-start capability per chip
- BM1370-specific tuning, normalization, and safe operating limits
- Clean modular interface (`G6BrainInterface`) for future brain variants

---

## Phase 1 Complete (May 2026)

All major stability, safety, and adaptation improvements identified in independent technical audits have been implemented. The core RLS engine is now production-hardened for continuous 24/7 operation.

---

## Roadmap

### Phase 1 — Core RLS Hardening (Completed)
- Numerical stability suite (VFF, windup protection, symmetrization)
- Sample quality state machine and telemetry validation
- Efficiency objective and fail-closed auto-apply
- NVS silicon fingerprint warm-start
- BM1370 optimization and hard safety limits

### Phase 2 — Advanced Telemetry & Control (In Progress)
- Timestamped telemetry buffer with differential sampling
- Controlled exploration policy (safe ε-greedy perturbation)
- Full integration of share counts and hashrate window validation
- Extended hardware-in-the-loop soak testing

### Phase 3 — Ecosystem & Modularity
- Bierman-Thornton UD factorization (optional deeper stability)
- Full "Ghost Brain" support for external MCU offload
- Multi-ASIC support (G6 Brain Quad)

### Phase 4 — Production Release
- Comprehensive documentation and automated test suite
- Community validation and long-term stability data
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
- Keep code clean and well-documented

---

## License

MIT License — see [LICENSE](LICENSE) for details.

---

Built for the Bitaxe community with focus on reliability, mathematical correctness, and long-term maintainability.

---

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Language: C](https://img.shields.io/badge/Language-C-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-red.svg)](https://www.espressif.com/en/products/socs/esp32)

Built for the Bitaxe community.  
May 2026

**G6 Brain v1.0 Beta** — *The brain your Bitaxe always wanted.*

*Built with ❤️ and way too much math by 6ummy-Dev & Grok (xAI) • May 2026*
