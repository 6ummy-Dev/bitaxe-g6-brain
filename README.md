# Bitaxe G6 Brain ⚡

**v1.0.0-beta3** — Modular adaptive RLS brain with analytical J/TH optimization for BM1370

The G6 Brain is a self-contained ESP-IDF component that models the quadratic relationship between frequency, voltage, hashrate, and power using stabilized Recursive Least Squares. It learns each individual ASIC’s response surface in real time while enforcing strict hardware safety constraints on every update.

> **Start safe. Learn. Then optimize.**

---

## Status

| | |
|---|---|
| **Version** | `v1.0.0-beta3` |
| **Target** | ESP32-S3 / Bitaxe Gamma (BM1370) |
| **License** | MIT |
| **QA Status** | Multiple review cycles · Ready for field testing |
| **Default Mode** | `G6_MODE_RECOMMEND` |

**beta3** is the current release and is suitable for community field testing. The core mathematics (Joseph-form RLS and analytical Dinkelbach J/TH solver) and safety architecture have been hardened through multiple QA passes.

Work toward the next release is ongoing in this repository.

---

## What’s New in beta3

- **O(1) Analytical J/TH Solver** — Replaced iterative gradient descent with a closed-form solution using Cramer’s rule.
- **Joseph Form Covariance Stabilization** — Full mathematical stabilization of the RLS covariance matrix.
- **3-Sigma Statistical Outlier Gating** — Dynamic innovation variance check that rejects sensor glitches.
- **Internal Slew-Rate Limiting** — Frequency and voltage steps are bounded inside the brain.
- **Dual Model Quality Gates** — J/TH optimization requires both `model_quality >= 0.6` and `power_model_quality >= 0.6`.
- **Safety & NVS improvements** — Safety layer executes on every call path. NVS warm-start corruption fixed.

---

## Quick Start

1. Drop the component into your ESP-IDF project under `components/g6_brain/`.
2. Add it to your top-level `CMakeLists.txt` via `EXTRA_COMPONENT_DIRS`.
3. Run `idf.py menuconfig` → **Component config → G6 Brain Configuration**.
4. Initialize and call `g6_brain_update()` from your control loop.

Full integration example → [`docs/INTEGRATION_EXAMPLE.c`](docs/INTEGRATION_EXAMPLE.c)

---

## Control Modes

| Mode                  | Behaviour |
|-----------------------|-----------|
| `G6_MODE_OBSERVE_ONLY` | Safety monitoring only. No optimizer action. |
| `G6_MODE_RECOMMEND` *(default)* | Computes optimal setpoints. Does **not** mutate `best_f` / `best_v`. |
| `G6_MODE_AUTO`        | Applies internally slew-rate-limited setpoints. |

All modes run the complete safety layer on every update.

---

## Roadmap

**v1 (Current focus)**
- Production-ready modular brain component
- Highest stable hashrate with zero hardware risk
- Thermal awareness + predictive safety layers
- Clean modularity, strong diagnostics, and rigorous QA

**v1.5+ (Planned)**
- Puzzle solver features and on-device stochastic exploration
- Advanced active learning techniques

Full firmware-level work is out of scope for the brain v1 series.

---

## Documentation

| Document | Purpose |
|----------|---------|
| [`docs/INSTALL.md`](docs/INSTALL.md) | Installation & integration guide |
| [`docs/INTEGRATION_EXAMPLE.c`](docs/INTEGRATION_EXAMPLE.c) | Recommended integration pattern |
| [`docs/API.md`](docs/API.md) | Full public API reference |
| [`docs/KCONFIG.md`](docs/KCONFIG.md) | All configuration options |
| [`docs/SAFETY.md`](docs/SAFETY.md) | Safety invariants & unhappy-path behavior |
| [`docs/MONITORING.md`](docs/MONITORING.md) | Telemetry and observability |
| [`CHANGELOG.md`](CHANGELOG.md) | Detailed version history |
| [`MANIFESTO.md`](MANIFESTO.md) | Project philosophy |

---

## Field Testing Recommendations

- Start in `G6_MODE_RECOMMEND`.
- Let the brain learn for at least 24–48 hours.
- Monitor `model_quality` and `power_model_quality`.
- Only switch to `AUTO` once both qualities are stable and above 0.6.

---

## Contributing

Pull requests are welcome. All changes must preserve the safety invariants documented in [`docs/AGENTS.md`](docs/AGENTS.md).

The `main` branch is the integration target.

---
**The brain your Bitaxe always wanted.**

*May 2026*

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Language: C](https://img.shields.io/badge/Language-C-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Platform: ESP32-S3](https://img.shields.io/badge/Platform-ESP32--S3-red.svg)](https://www.espressif.com/en/products/socs/esp32-s3)
