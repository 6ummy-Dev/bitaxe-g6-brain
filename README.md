# Bitaxe G6 Brain ⚡

**v1.0.0-beta2** — Adaptive stabilized RLS optimizer with real-time quadratic efficiency modeling for BM1370

The **G6 Brain** dynamically models the quadratic relationship between frequency, voltage, and hashrate in real time using a stabilized conventional RLS algorithm (P-matrix with symmetrization, ridge regularization, trace monitoring, Variable Forgetting Factor, and innovation gating). It continuously learns the unique efficiency surface of your ASIC and selects the most efficient operating point (J/TH) while maintaining strict numerical stability and enforcing hardware safety constraints.

This is the flagship module of the **Bitaxe Brains Project** — a clean, modular architecture designed for long-term reliability and extensibility.

---

## ✅ Status
- **CI**: Passing (Docker ESP-IDF v5.3 build + Unity compilation)
- **Version**: Hardened beta2 with critical safety fixes, cold-start improvements, and documentation alignment
- Ready for community field testing

See [CHANGELOG.md](CHANGELOG.md) for full history.

---

## ✨ Key Features
- Stabilized conventional RLS with Variable Forgetting Factor, covariance protection, and ridge regularization
- Fully self-contained safety layer (thermal ceiling, NER back-off, voltage ripple, power sanity)
- Sample quality state machine (settle + measure windows)
- NVS persistence of theta + full P-matrix (true warm-start)
- BM1370 hard limits + true J/TH efficiency optimization
- Defensive programming, extensive input validation, and self-test
- Unity test suite

---

## 🚀 Quick Start
1. Add the component to your ESP-IDF project
2. Use the recommended integration example: [`docs/INTEGRATION_EXAMPLE.c`](docs/INTEGRATION_EXAMPLE.c)
3. Start in `OBSERVE_ONLY` or `RECOMMEND` mode
4. Monitor behavior and logs before enabling `AUTO` mode

Full installation guide → [`docs/INSTALL.md`](docs/INSTALL.md)

---

## 📖 Documentation
- [Installation & Integration](docs/INSTALL.md)
- [Public API Reference](docs/API.md)
- [Kconfig Options](docs/KCONFIG.md)
- [Safety & Engineering Principles](docs/SAFETY.md)
- [AGENTS.md](AGENTS.md) — Core invariants and design decisions

---

## 🤝 Contributing
Pull requests are welcome. All changes must respect the safety invariants documented in `AGENTS.md`.

---

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Language: C](https://img.shields.io/badge/Language-C-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Platform: ESP32-S3](https://img.shields.io/badge/Platform-ESP32--S3-red.svg)](https://www.espressif.com/en/products/socs/esp32-s3)
[![CI Status](https://github.com/6ummy-Dev/bitaxe-g6-brain/actions/workflows/build.yml/badge.svg)](https://github.com/6ummy-Dev/bitaxe-g6-brain/actions/workflows/build.yml)

---

**Made with ❤️ and rigorous engineering for the Bitaxe community**  
*May 2026 • v1.0.0-beta2*

---
