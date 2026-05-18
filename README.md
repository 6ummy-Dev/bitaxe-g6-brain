# Bitaxe G6 Brain ⚡

**v1.0.0-beta2** — Adaptive stabilized RLS optimizer with real-time quadratic modeling for BM1370

The **G6 Brain** dynamically models the quadratic relationship between frequency, voltage, and hashrate in real time using a stabilized conventional RLS algorithm (P-matrix with symmetrization, ridge regularization, trace monitoring, Variable Forgetting Factor, and innovation gating). It continuously learns the unique efficiency surface of your ASIC and selects the most efficient safe operating point while maintaining strict numerical stability and enforcing hardware safety constraints.

---

## ✅ Current Status

- **CI**: Passing
- **Kconfig**: Fully wired
- **Control Modes**: Enforced (`OBSERVE_ONLY` / `RECOMMEND` / `AUTO`)
- **NVS**: Full theta + P-matrix warm-start with auto-save
- **Telemetry**: Clean `g6_brain_get_telemetry()` API available
- **Code Quality**: Heavy QA + structural cleanup applied

See [CHANGELOG.md](CHANGELOG.md) for full history.

---

## ✨ Key Features

- Stabilized RLS with Variable Forgetting Factor, covariance protection, and ridge regularization
- Fully self-contained safety layer (thermal ceiling, NER back-off, voltage ripple, power sanity)
- Sample quality state machine
- **NVS persistence** of theta + full P-matrix (true warm-start)
- **Control modes** fully enforced
- **Telemetry export** via `g6_brain_get_telemetry()`
- Defensive programming and self-test

**Note on Efficiency**: When `G6_ENABLE_EFFICIENCY_MODE` is enabled, the brain uses a separate power RLS model. The current optimizer still uses a coarse grid search. A proper analytical J/TH solver is planned.

---

## 🚀 Quick Start

1. Add the component to your ESP-IDF project
2. Use the recommended integration example: [`docs/INTEGRATION_EXAMPLE.c`](docs/INTEGRATION_EXAMPLE.c)
3. **Start in `G6_MODE_RECOMMEND`** (safest)
4. Run `idf.py menuconfig` → Component config → G6 Brain Configuration
5. Monitor logs before switching to `AUTO` mode

Full installation guide → [`docs/INSTALL.md`](docs/INSTALL.md)

---

## 📖 Documentation

- [Installation & Integration](docs/INSTALL.md)
- [Public API](docs/API.md)
- [Kconfig Options](docs/KCONFIG.md)
- [Monitoring Guide](docs/MONITORING.md)
- [Safety Principles](docs/SAFETY.md)
- [Manifesto](MANIFESTO.md)

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
