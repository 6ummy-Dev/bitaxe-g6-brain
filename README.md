# Bitaxe G6 Brain ⚡

**Modular adaptive RLS quadratic optimizer for real-time J/TH scaling on the BM1370.**

The G6 Brain is a self-contained ESP-IDF component that models the quadratic relationship between frequency, voltage, hashrate, and power using stabilized Recursive Least Squares. It learns each individual ASIC's response surface in real time while enforcing strict hardware safety constraints on every update.

> **Start safe. Learn. Then optimize.**

---

## Status

| | |
|---|---|
| **Latest Release** | `v1.0.0-beta6` |
| **Target** | ESP32-S3 / Bitaxe Gamma (BM1370) |
| **License** | MIT |

See [`CHANGELOG.md`](CHANGELOG.md) for release history.

---

## Features

- Pure RLS quadratic modeling with separate hashrate and power surfaces — fully explainable, no black boxes.
- Stabilized covariance updates (Joseph-style congruence + ridge + symmetrize + clamp) for numerical robustness under floating-point arithmetic.
- 3-sigma statistical outlier gating on both estimators.
- Fail-closed safety contract — every numeric input (including NaN, Inf, out-of-bounds) routes to the safety layer. `ESP_ERR_INVALID_ARG` returns only on `brain == NULL`.
- Two-tier thermal safety — independent ASIC and VR temperature protection with configurable proactive margins and hard ceilings.
- Bounded analytical Dinkelbach J/TH solver — exact $O(1)$ fractional minimization, gated on both model qualities ≥ 0.6.
- P-matrix divergence auto-recovery — when the estimator goes singular, the brain re-cold-starts while preserving operator-configured state.
- Per-chip NVS fingerprinting with schema versioning and bad-blob auto-erase — warm-start survives power cycles, schema bumps survive cleanly.
- Clean `G6BrainTelemetry` snapshot for monitoring, dashboards, and integration logging.
- Modular, single-threaded, swappable component with a small public API and Kconfig-driven tunables.

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
| `G6_MODE_OBSERVE_ONLY` | Safety monitoring only |
| `G6_MODE_RECOMMEND` *(default)* | Computes optimal setpoints without mutating `best_f` / `best_v` |
| `G6_MODE_AUTO`        | Applies internally slew-rate-limited setpoints |

All modes run the complete safety layer on every update.

---

## Documentation

| Document | Purpose |
|----------|---------|
| [`docs/INSTALL.md`](docs/INSTALL.md) | Installation & integration guide |
| [`docs/INTEGRATION_EXAMPLE.c`](docs/INTEGRATION_EXAMPLE.c) | Recommended integration pattern |
| [`docs/API.md`](docs/API.md) | Full public API reference |
| [`docs/KCONFIG.md`](docs/KCONFIG.md) | Configuration options |
| [`docs/SAFETY.md`](docs/SAFETY.md) | Safety mechanisms & full `G6SafetyStatus` reference |
| [`docs/MONITORING.md`](docs/MONITORING.md) | Real-time observability and telemetry |
| [`docs/TESTING.md`](docs/TESTING.md) | Community testing guide |
| [`docs/AGENTS.md`](docs/AGENTS.md) | Engineering principles & safety invariants for contributors |
| [`docs/GLOSSARY.md`](docs/GLOSSARY.md) | Terminology used across code and docs |
| [`docs/REFERENCES.md`](docs/REFERENCES.md) | Scientific & mathematical foundations |
| [`CHANGELOG.md`](CHANGELOG.md) | Version history |
| [`MANIFESTO.md`](MANIFESTO.md) | Project philosophy and non-negotiables |

---

## Field Testing Recommendations

- Start in `G6_MODE_RECOMMEND`.
- Observe for at least 24–48 hours before considering `AUTO`.
- When using hardware with a VR temperature sensor, pass the `vr_temp_c` value to `g6_brain_update()`.
- Use `G6_VR_TEMP_NO_SENSOR` (`-1.0f`) if no VR sensor is available.

---

## Contributing

Pull requests are welcome. All changes must respect the safety invariants documented in [`docs/AGENTS.md`](docs/AGENTS.md) and the project principles in [`MANIFESTO.md`](MANIFESTO.md).

---

**The brain your Bitaxe always wanted.**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Language: C](https://img.shields.io/badge/Language-C-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Platform: ESP32-S3](https://img.shields.io/badge/Platform-ESP32--S3-red.svg)](https://www.espressif.com/en/products/socs/esp32-s3)
