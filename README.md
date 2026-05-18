# Bitaxe G6 Brain ⚡

**v1.0.0-beta2** — Adaptive stabilized RLS optimizer with real-time quadratic efficiency modeling for BM1370

The G6 Brain dynamically models the quadratic relationship between frequency, voltage, and hashrate in real time using a stabilized conventional RLS (P-matrix with symmetrization, ridge regularization, and trace monitoring). It continuously learns the unique efficiency surface of your ASIC and selects the most efficient operating point (J/TH) while maintaining strict numerical stability and hardware safety constraints.

This is the flagship module of the **Bitaxe Brains Project** — a clean, modular architecture designed for long-term reliability and extensibility.

---

## Key Features

- Stabilized conventional RLS with Variable Forgetting Factor, innovation gating, and covariance monitoring
- Fully self-contained safety (thermal protection, voltage clamping, error rate handling)
- Sample quality state machine (settle + measure windows)
- NVS silicon fingerprinting (theta + full covariance P matrix) for warm-start
- BM1370 hard limits and efficiency optimization (J/TH)
- Defensive programming with extensive input validation
- Unity test suite

---

## Status

This is **v1.0.0-beta2**. The project has undergone significant hardening, including critical bug fixes, test expansion, and documentation alignment. It is ready for community field testing but has not yet reached stable v1.0.

See [CHANGELOG.md](CHANGELOG.md) for detailed history.

---

## Quick Start

1. Add the component to your ESP-Miner project (git submodule or copy).
2. Use the recommended integration example: `docs/INTEGRATION_EXAMPLE.c`
3. Start in `OBSERVE_ONLY` or `RECOMMEND` mode.
4. Monitor behavior before moving to `AUTO` mode.

Full installation guide: [docs/INSTALL.md](docs/INSTALL.md)

---

## Documentation

- [Installation & Integration](docs/INSTALL.md)
- [Public API](docs/API.md)
- [Configuration (Kconfig)](docs/KCONFIG.md)
- [Safety & Engineering](docs/SAFETY.md)
- [AGENTS.md](AGENTS.md) — Engineering principles

---

## Contributing

Pull requests are welcome. Please respect the safety invariants defined in `AGENTS.md`.

---

Built for the Bitaxe community with focus on mathematical correctness, reliability, and hardware safety.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Language: C](https://img.shields.io/badge/Language-C-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-red.svg)](https://www.espressif.com/en/products/socs/esp32)

**G6 Brain v1.0.0-beta2** — The brain your Bitaxe always wanted.  
*With ❤️ and rigorous engineering • May 2026*
