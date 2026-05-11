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
> **"Maximize hashrate. Minimize risk. Evolve autonomously."**

The G6 Brain replaces dumb fixed frequency/voltage tables with real-time quadratic modeling of hashrate vs frequency vs voltage. It runs an online recursive least squares estimator that actually learns your ASIC’s response surface and analytically solves for the best operating point under power and temperature limits.

This is the first module of the Bitaxe Brains Project — a clean modular architecture so different optimization strategies can share the same interface and the miner firmware doesn’t have to care which brain is plugged in.

---

## Modular Design — Bitaxe Brains Project

G6 Brain is built on a simple, strict contract:

- One `G6BrainInterface` struct with six functions (`init`, `update`, `get_optimal`, `get_model_quality`, `get_full_telemetry`, `self_test`).
- The rest of ESP-Miner only ever talks to this interface.
- Swap in a new brain (ML, heuristic, multi-ASIC, whatever) at compile time or runtime without touching the core miner code.

This keeps everything maintainable and future-proof.

---

## Implemented Features

### Core Optimizer
- Quadratic model: HR = a·f² + b·v² + c·f·v + d·f + e·v + g
- Real-time recursive least squares with cold-start, ridge regularization, and covariance safeguards
- Analytical optimum solver that respects power and temperature constraints
- Live model quality metric

### Safety and Control
- I2C guardian with hard-fault escalation
- Proactive frequency scaling on rapid temperature rise (ΔT/dt)
- Voltage ripple detection and automatic response
- +5 mV voltage auto-tune on BM1366 non-blocking errors
- NVS writes throttled to protect flash lifetime (RTC RAM for temporary counters)
- PID fan control with feed-forward

### Additional Capabilities
- Self-test mode with synthetic data injection
- Full telemetry output (JSON) ready for WebUI or external monitoring
- NVS persistence of model parameters
- All settings exposed via Kconfig
- **Stochastic Nonce Offsetting** — hardware RNG sets a unique random start nonce for every new job (better search diversity, zero overlap risk)
- Low-latency job hook (double-buffering ready for zero-stale-work)

---

## Installation and Integration

1. Drop the component into your ESP-Miner project.
2. Enable it in CMakeLists.txt.
3. Configure via menuconfig → “G6 Brain Configuration”.
4. Call `g6_brain_init()` once and `g6_brain_update()` in the main miner loop (example in docs).

Full build and flash instructions are in the docs folder.

---

## Roadmap

- WebUI integration exposing live θ matrix, P covariance and model quality
- **G6 Brain Quad** — multi-ASIC support for quad Gamma 602+ boards
- Additional brain modules (ML-based optimizer, heuristic variants)

---

## Contributing

Pull requests are welcome. Keep the modular interface intact and make sure the self-test still passes.

---

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Language: C](https://img.shields.io/badge/Language-C-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-red.svg)](https://www.espressif.com/en/products/socs/esp32)

Built for the Bitaxe community.  
May 2026

**G6 Brain v1.0 Beta** — *The brain your Bitaxe always wanted.*

*Built with ❤️ and way too much math by 6ummy-Dev & Grok (xAI) • May 2026*
