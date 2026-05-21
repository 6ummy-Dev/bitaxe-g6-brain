# Bitaxe G6 Brain ⚡

**v1.0.0-beta5** — Safety layer hardening, configurable thermal margins, and code quality improvements

The G6 Brain is a self-contained ESP-IDF component that models the quadratic relationship between frequency, voltage, hashrate, and power using stabilized Recursive Least Squares. It learns each individual ASIC’s response surface in real time while enforcing strict hardware safety constraints on every update.

> **Start safe. Learn. Then optimize.**

---

## Status

| | |
|---|---|
| **Latest Release** | `v1.0.0-beta5` |
| **Target** | ESP32-S3 / Bitaxe Gamma (BM1370) |
| **License** | MIT |
| **QA Status** | Fully verified and released for field testing |
| **Default Mode** | `G6_MODE_RECOMMEND` |

**beta5** is the current release and is suitable for community field testing. This version hardens the safety layer with configurable thermal margins, improved safety status priority, NER defense-in-depth, and several code quality improvements.

---

## What’s New in beta5

- **Configurable ASIC Proactive Thermal Margin** — `G6_TEMP_PROACTIVE_MARGIN` is now a Kconfig option (default 5°C) stored in the state struct, matching the VR margin design. The hard-coded `5.0f` literal has been eliminated from all call sites.
- **VR Proactive Margin Properly Runtime-Configurable** — `brain->vr_temp_proactive_margin` replaces the baked-in default macro at both the safety helper and the slew-suspend gate. Mutating the field at runtime now correctly changes the proactive zone.
- **Safety Status Priority on Collision** — Safety helpers reordered so ASIC thermal wins in `last_safety_status` when both ASIC and VR thermal conditions fire on the same tick.
- **NER Defense-in-Depth** — `is_sample_valid()` now independently gates on NER. Dead pre-checks already enforced by upstream validation removed.
- **NER Backoff Floor Clamps** — `g6_asic_error_handle_non_blocking` now uses `fmaxf(BM1370_X_MIN, ...)` consistent with all other safety helpers.
- **NVS Blob-Size Mismatch Handling** — Corrupt or schema-mismatched NVS blobs are logged with a warning and erased rather than silently falling through.
- **`g6_brain_set_defaults()` Helper** — Extracted from the duplicated init/reset bodies. `g6_brain_init` and `g6_brain_reset` share a single defaults path with no risk of future drift.

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
