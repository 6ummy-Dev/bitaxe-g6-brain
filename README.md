# Bitaxe G6 Brain ⚡

**v1.0.0-beta2** — Adaptive stabilized RLS optimizer with real-time quadratic efficiency modeling for BM1370

The **G6 Brain** dynamically learns the unique quadratic efficiency surface (frequency × voltage × hashrate) of your ASIC in real time using a hardened conventional RLS algorithm (P-matrix stabilization, Variable Forgetting Factor, innovation gating, ridge regularization, and trace monitoring).

It continuously selects the most efficient operating point (J/TH) while enforcing strict hardware safety constraints.

This is the flagship module of the **Bitaxe Brains Project** — clean, modular, and built for long-term reliability.

---

## ✅ Status
- **CI**: Passing (Docker ESP-IDF build + Unity compilation)
- **Version**: Hardened beta2 with critical safety fixes, cold-start improvements, and documentation alignment
- Ready for community field testing

See [CHANGELOG.md](CHANGELOG.md) for full history.

---

## Key Features

- Stabilized conventional RLS with Variable Forgetting Factor & covariance protection
- Fully self-contained safety layer (thermal, NER, voltage ripple, power sanity)
- Sample quality state machine (settle + measure windows)
- NVS persistence of theta + full P matrix (true warm-start)
- BM1370 hard limits + J/TH efficiency optimization
- Defensive programming & extensive input validation
- Unity test suite

---

## Quick Start

1. Add as ESP-IDF component (`git submodule` or copy `components/g6_brain`)
2. Include the recommended integration: [`docs/INTEGRATION_EXAMPLE.c`](docs/INTEGRATION_EXAMPLE.c)
3. Start in `OBSERVE_ONLY` or `RECOMMEND` mode
4. Monitor logs and behavior before enabling `AUTO` mode

Full guide → [`docs/INSTALL.md`](docs/INSTALL.md)

---

## Documentation

- [Installation & Integration](docs/INSTALL.md)
- [Public API](docs/API.md)
- [Kconfig Options](docs/KCONFIG.md)
- [Safety & Engineering](docs/SAFETY.md)
- [AGENTS.md](AGENTS.md) — Core engineering principles

---

## Contributing

Pull requests welcome. All changes must respect the safety invariants in `AGENTS.md`.

---

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Language: C](https://img.shields.io/badge/Language-C-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Platform: ESP32-S3](https://img.shields.io/badge/Platform-ESP32--S3-red.svg)](https://www.espressif.com/en/products/socs/esp32-s3)
[![CI Status](https://github.com/6ummy-Dev/bitaxe-g6-brain/actions/workflows/build.yml/badge.svg)](https://github.com/6ummy-Dev/bitaxe-g6-brain/actions/workflows/build.yml)

**G6 Brain v1.0.0-beta2** — The brain your Bitaxe always wanted.  
*With ❤️ and rigorous engineering • May 2026*
