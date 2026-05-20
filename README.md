# Bitaxe G6 Brain ⚡

**v1.0.0-beta4** — Modular adaptive RLS brain with analytical J/TH optimization for BM1370

The G6 Brain is a self-contained ESP-IDF component that models the quadratic relationship between frequency, voltage, hashrate, and power using stabilized Recursive Least Squares. It learns each individual ASIC’s response surface in real time while enforcing strict hardware safety constraints on every update.

> **Start safe. Learn. Then optimize.**

---

## Status

| | |
|---|---|
| **Latest Release** | `v1.0.0-beta3` |
| **Current Version** | `v1.0.0-beta4` (in progress) |
| **Target** | ESP32-S3 / Bitaxe Gamma (BM1370) |
| **License** | MIT |
| **QA Status** | Code verified · Documentation updates in progress |
| **Default Mode** | `G6_MODE_RECOMMEND` |

**beta3** remains the latest stable release. This branch contains work toward **beta4**, which introduces two-tier thermal safety and several robustness improvements.

---

## What’s New in beta4

- **Two-tier Thermal Safety** — Added dedicated monitoring and protection for the voltage regulator (`vr_temp_c`). The brain now distinguishes between ASIC die temperature and VR regulator temperature:
  - ASIC temperature gates learning (prevents training on thermally stressed samples).
  - VR temperature runs only in the safety layer as a proactive and hard constraint on voltage (and frequency at ceiling).
- **Improved Safety Telemetry** — `safety_status` in telemetry is now meaningful and reports the last triggered safety condition.
- **Power Validation Hardening** — Invalid power readings no longer bypass the safety layer.
- **Power Outlier Logging** — Added symmetric logging for power model outliers.
- **Timing Fix** — Corrected tick-to-millisecond conversion in the sample state machine.
- General QA hardening and CI improvements.

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
| [`docs/SAFETY.md`](docs/SAFETY.md) | Safety behavior |
| [`CHANGELOG.md`](CHANGELOG.md) | Version history |

---

## Field Testing Recommendations

- Start in `G6_MODE_RECOMMEND`.
- Observe for at least 24–48 hours before considering `AUTO`.
- When using hardware with a VR temperature sensor, pass the `vr_temp_c` value to `g6_brain_update()`.
- Use `G6_VR_TEMP_NO_SENSOR` (`-1.0f`) if no VR sensor is available.

---

## Contributing

Pull requests are welcome. All changes must respect the safety invariants documented in [`docs/AGENTS.md`](docs/AGENTS.md).

---

**The brain your Bitaxe always wanted.**

*May 2026*

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Language: C](https://img.shields.io/badge/Language-C-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Platform: ESP32-S3](https://img.shields.io/badge/Platform-ESP32--S3-red.svg)](https://www.espressif.com/en/products/socs/esp32-s3)
