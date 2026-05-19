# Bitaxe G6 Brain ⚡ 

**v1.0.0-beta3** — Aerospace-Hardened adaptive RLS optimizer with analytical J/TH solver for BM1370

The **G6 Brain** dynamically models the quadratic relationship between frequency, voltage, and hashrate using stabilized Recursive Least Squares (RLS). It learns your ASIC’s unique efficiency surface and selects safe operating points while enforcing strict safety constraints.

In **beta3**, the optional J/TH efficiency mode uses a fast **O(1) analytical Dinkelbach solver** (exact 2×2 quadratic minimization), backed by **Joseph Stabilized Covariance** updates and **3-Sigma Statistical Outlier Gating**.

_Start safe. Learn. Then optimize._

---

## ✅ Current Status

- **Version**: v1.0.0-beta3
- **CI**: Passing
- **Kconfig**: Fully wired
- **Control Modes**: Enforced (`OBSERVE_ONLY` / `RECOMMEND` default / `AUTO`)
- **NVS**: Full theta + P-matrix + power model warm-start with auto-save
- **J/TH Solver**: Analytical Dinkelbach (O(1) exact) with `model_quality` + `power_model_quality` guards
- **QA Status**: Signed off in aerospace-hardening pass — ready for field deployment

See [CHANGELOG.md](../CHANGELOG.md) for full history.

---

## ✨ Key Features

- **Aerospace-Hardened RLS**: Powered by Joseph Form covariance stabilization to prevent floating-point numerical degradation and matrix collapse.
- **Statistical Outlier Gating**: 3-Sigma innovation variance validation to reject physical sensor and telemetry glitches before updating estimators.
- **Internal Slew-Rate Limiting**: Slew logic embedded directly inside the tracking loop to closely align internal models with physical state transitions.
- **Self-Contained Safety Layer**: Post-optimization priority enforcement for thermal ceiling, NER back-off, voltage ripple, and power sanity.
- **NVS Persistence**: Persistent tracking of hashrate and power surfaces for seamless warm-starts across power cycles.
- **Control Modes**: Enforced operating environments with secure defaults (`G6_MODE_RECOMMEND`).
- **Telemetry Export**: Zero-copy snapshot interface via `g6_brain_get_telemetry()`.
- **O(1) Analytical J/TH Solver**: Fractional optimization via exact 2x2 parametric minimization.

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
