# Bitaxe G6 Brain ⚡

**v1.0.0-beta5** — Modular adaptive RLS quadratic optimizer for real-time J/TH scaling on the BM1370

The G6 Brain is a self-contained ESP-IDF component that models the quadratic relationship between frequency, voltage, hashrate, and power using stabilized Recursive Least Squares. It learns each individual ASIC’s response surface in real time while enforcing strict hardware safety constraints on every update.

> **Start safe. Learn. Then optimize.**

---

## Status

| | |
|---|---|
| **Latest Release** | `v1.0.0-beta5` (In Development) |
| **Target** | ESP32-S3 / Bitaxe Gamma (BM1370) |
| **License** | MIT |
| **QA Status** | Deep QA verified, preparing for field testing |
| **Default Mode** | `G6_MODE_RECOMMEND` |

---

## What's New in beta5

- **Fail-Closed Validation Routing** — Out-of-bounds sensor readings trigger the safety layer rather than bypassing it via early returns, ensuring hardware clamps are unconditionally enforced.
- **Trace Accumulation Recovery** — Automatically arrests covariance matrix divergence during unbounded learning loops by safely wiping polynomial surfaces and resetting matrix confidence, preventing recursive gain explosions.
- **Slew-Rate Amnesia Protection** — Upward setpoint slew is now strictly frozen during *any* active sensor anomaly, power sanity violation, or thermal event.
- **Dinkelbach Solver Bounding** — The exact $O(1)$ efficiency solver mathematically clamps normalized fractional coordinates to prevent overshoot on degraded power surfaces.
- **Configurable ASIC Proactive Thermal Margin** — `G6_TEMP_PROACTIVE_MARGIN` Kconfig option (default 5°C) stored in state struct, matching the VR margin design.
- **VR Proactive Margin Runtime-Configurable** — `brain->vr_temp_proactive_margin` replaces the baked-in macro at all call sites.
- **Safety Status Priority on Collision** — Safety helpers reordered so ASIC thermal wins in `last_safety_status` when both ASIC and VR thermal conditions fire on the same tick.
- **NER Defense-in-Depth** — `is_sample_valid()` now independently gates on NER as a redundant safety check.
- **NER Backoff Floor Clamps** — `g6_asic_error_handle_non_blocking` uses `fmaxf(BM1370_X_MIN, ...)` consistent with all other safety helpers.
- **Unified State Flags** — Streamlined boolean state flags (`cold_start` unifies both estimators) and optimized struct packing for enhanced cache-line utilization.

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
