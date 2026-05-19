# Bitaxe G6 Brain ⚡ _Start safe. Learn. Then optimize._

**v1.0.0-beta3** — Adaptive stabilized RLS optimizer with analytical J/TH solver + telemetry for BM1370

The **G6 Brain** dynamically models the quadratic relationship between frequency, voltage, and hashrate using stabilized Recursive Least Squares (RLS). It learns your ASIC’s unique efficiency surface and selects safe operating points while enforcing strict safety constraints.

In **beta3**, the optional J/TH efficiency mode now uses a fast **O(1) analytical Dinkelbach solver** (exact 2×2 quadratic minimization) instead of grid search or gradient descent.

---

## ✅ Current Status

- **Version**: v1.0.0-beta3 (in progress)
- **CI**: Passing (with graceful test handling during setup)
- **Kconfig**: Fully wired
- **Control Modes**: Enforced (`OBSERVE_ONLY` / `RECOMMEND` default / `AUTO`)
- **NVS**: Full theta + P-matrix + power model warm-start with auto-save
- **J/TH Solver**: Analytical Dinkelbach (O(1) exact) with `model_quality` + `power_model_quality` guards
- **QA Status**: Actively hardening — ready for field testing of the new analytical solver

See [CHANGELOG.md](../CHANGELOG.md) for full history.

---

## ✨ Key Features

- Stabilized RLS with Variable Forgetting Factor, ridge regularization, trace monitoring, and innovation gating
- Fully self-contained safety layer (thermal ceiling, NER back-off, voltage ripple, power sanity)
- **NVS persistence** of hashrate + power models (true warm-start)
- **Control modes** with safe defaults
- **Telemetry export** via `g6_brain_get_telemetry()` (zero-copy snapshot)
- **Optional true J/TH efficiency mode** (opt-in) powered by fast analytical Dinkelbach solver
- Strong single-threaded contract + defensive programming

---

## 🚀 Quick Start

1. Add the component to your ESP-IDF project
2. Use the recommended integration example: [`docs/INTEGRATION_EXAMPLE.c`](docs/INTEGRATION_EXAMPLE.c)
3. **Start in `G6_MODE_RECOMMEND`** (safest default)
4. Run `idf.py menuconfig` → Component config → G6 Brain Configuration
5. Monitor logs before switching to `AUTO` mode or enabling efficiency mode

Full installation guide → [`docs/INSTALL.md`](docs/INSTALL.md)

---

## 📖 Documentation

- [Installation & Integration](docs/INSTALL.md)
- [Public API](docs/API.md)
- [Kconfig Options](docs/KCONFIG.md)
- [Monitoring Guide](docs/MONITORING.md)
- [Safety Principles](docs/SAFETY.md)
- [Manifesto](../MANIFESTO.md)

---

## 🤝 Contributing

Pull requests are welcome. All changes must respect the safety invariants documented in `AGENTS.md`.

---

**Made with ❤️ and rigorous engineering for the Bitaxe community**  
*May 2026 • v1.0.0-beta3*

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Language: C](https://img.shields.io/badge/Language-C-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Platform: ESP32-S3](https://img.shields.io/badge/Platform-ESP32--S3-red.svg)](https://www.espressif.com/en/products/socs/esp32-s3)
[![CI Status](https://github.com/6ummy-Dev/bitaxe-g6-brain/actions/workflows/build.yml/badge.svg)](https://github.com/6ummy-Dev/bitaxe-g6-brain/actions/workflows/build.yml)
