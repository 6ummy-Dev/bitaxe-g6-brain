# Bitaxe G6 Brain ⚡ 

**v1.0.0-beta3** — Adaptive RLS optimizer with real-time quadratic modeling and analytical J/TH solver for BM1370

The **G6 Brain** dynamically models the quadratic relationship between frequency, voltage, and hashrate using stabilized Recursive Least Squares (RLS). It learns your ASIC’s unique efficiency surface and selects stable operating points while enforcing strict hardware constraints.

In **beta3**, the optional J/TH efficiency mode uses a fast **O(1) analytical Dinkelbach solver** (exact 2×2 quadratic minimization), backed by **Joseph Form Covariance Stabilization** and **3-Sigma Statistical Outlier Gating**.

**_Start safe. Learn. Then optimize._**

---

## ✅ Current Status

- **Version**: v1.0.0-beta3
- **CI**: Passing
- **Kconfig**: Fully wired
- **Control Modes**: Enforced (`OBSERVE_ONLY` / `RECOMMEND` default / `AUTO`)
- **NVS**: Full theta + P-matrix + power model warm-start with auto-save
- **J/TH Solver**: Analytical Dinkelbach (O(1) exact) with `model_quality` + `power_model_quality` guards
- **QA Status**: Verified and signed off for field deployment

See [CHANGELOG.md](../CHANGELOG.md) for full history.

---

## ✨ Key Features

- **Stabilized Covariance Update**: Powered by the Joseph Form algorithm to mathematically eliminate floating-point truncation errors and prevent matrix divergence.
- **Statistical Outlier Gating**: 3-Sigma innovation variance filtering to automatically reject physical sensor anomalies and bus noise before updating estimators.
- **Internal Slew-Rate Limiting**: Step constraints embedded directly within the control loop to ensure voltage and frequency transitions match the estimator state.
- **Deterministic Safety Layer**: Multi-layer post-optimization validation for thermal ceilings, hardware error bounds, and power sanity.
- **NVS Surface Persistence**: Real-time checkpointing of hashrate and power models for seamless warm-starts.
- **API Telemetry Export**: Zero-copy snapshot interface via `g6_brain_get_telemetry()`.
- **O(1) Analytical J/TH Solver**: Direct parametric fractional minimization using closed-form 2x2 algebraic blocks.

---

## 🚀 Quick Start

1. Add the component to your ESP-IDF project.
2. Use the recommended integration example: [`docs/INTEGRATION_EXAMPLE.c`](docs/INTEGRATION_EXAMPLE.c).
3. **Start in `G6_MODE_RECOMMEND`** (safest default).
4. Run `idf.py menuconfig` → Component config → G6 Brain Configuration.
5. Monitor logs before switching to `AUTO` mode or enabling efficiency mode.

Full installation guide → [`docs/INSTALL.md`](docs/INSTALL.md).

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

**Made with ❤️ and rigorous engineering for the Bitaxe community** *May 2026 • v1.0.0-beta3*

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Language: C](https://img.shields.io/badge/Language-C-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Platform: ESP32-S3](https://img.shields.io/badge/Platform-ESP32--S3-red.svg)](https://www.espressif.com/en/products/socs/esp32-s3)
[![CI Status](https://github.com/6ummy-Dev/bitaxe-g6-brain/actions/workflows/build.yml/badge.svg)](https://github.com/6ummy-Dev/bitaxe-g6-brain/actions/workflows/build.yml)
