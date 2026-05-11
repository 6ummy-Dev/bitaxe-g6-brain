# Bitaxe G6 Brain ⚡

**v1.0 Beta** — Fully Modular Adaptive Control Brain for Bitaxe ESP-Miner (Gamma 602+)

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Language: C](https://img.shields.io/badge/Language-C-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-red.svg)](https://www.espressif.com/en/products/socs/esp32)

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

## License

MIT License — see LICENSE file.

Built for the Bitaxe community by 6ummy+Grok.  
May 2026

**G6 Brain v1.0 Beta** — *The brain your Bitaxe always wanted.*

*Built with ❤️ and way too much math by 6ummy-Dev & Grok (xAI) • May 2026*
