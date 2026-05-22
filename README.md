# Bitaxe G6 Brain ⚡

**v1.0.0-beta5** — Modular adaptive RLS quadratic optimizer for real-time J/TH scaling on the BM1370

The G6 Brain is a self-contained ESP-IDF component that models the quadratic relationship between frequency, voltage, hashrate, and power using stabilized Recursive Least Squares. It learns each individual ASIC’s response surface in real time while enforcing strict hardware safety constraints on every update.

> **Start safe. Learn. Then optimize.**

---

## Status

| | |
|---|---|
| **Latest Release** | `v1.0.0-beta5` (Release Candidate) |
| **Target** | ESP32-S3 / Bitaxe Gamma (BM1370) |
| **License** | MIT |
| **QA Status** | Nine review cycles completed; full code + docs audit pass |
| **Default Mode** | `G6_MODE_RECOMMEND` |

---

## What's New in beta5

The beta5 release evolved across multiple QA rounds. Headline changes by phase:

**Initial hardening (Round 5):**

- **Fail-Closed Validation Routing** — Out-of-bounds sensor readings trigger the safety layer rather than bypassing it via early returns, ensuring hardware clamps are unconditionally enforced.
- **Trace Accumulation Recovery** — Automatically arrests covariance matrix divergence during unbounded learning loops by safely wiping polynomial surfaces and resetting matrix confidence, preventing recursive gain explosions.
- **Slew-Rate Amnesia Protection** — Upward setpoint slew is now strictly frozen during *any* active sensor anomaly, power sanity violation, or thermal event.
- **Dinkelbach Solver Bounding** — The exact $O(1)$ efficiency solver mathematically clamps normalized fractional coordinates to prevent overshoot on degraded power surfaces.
- **Configurable Thermal Margins** — `G6_TEMP_PROACTIVE_MARGIN` and `G6_VR_TEMP_PROACTIVE_MARGIN` are now Kconfig options stored in the state struct, replacing baked-in macros.
- **Safety Status Priority on Collision** — Safety helpers reordered so ASIC thermal wins in `last_safety_status` when both ASIC and VR thermal conditions fire on the same tick.
- **NER Defense-in-Depth** — `is_sample_valid()` independently gates on NER as a redundant safety check; `g6_asic_error_handle_non_blocking` uses `fmaxf(BM1370_X_MIN, ...)` consistent with all other safety helpers.
- **Unified State Flags** — `cold_start` unifies both estimators; optimized struct packing for cache-line utilization.

**Safety model integrity (Round 7) — full fail-closed contract:**

- **Uniform fail-closed input handling** — Every bad numeric input (NaN, Inf, out-of-bounds, `hr_ths <= 0`) routes to the safety layer with the new `G6_SAFETY_INPUT_RANGE` status. `ESP_ERR_INVALID_ARG` returns **only** when `brain == NULL`. Per manifesto non-negotiable 3.7, no telemetry path can skip a safety tick.
- **`G6_SAFETY_P_MATRIX_SINGULAR` wired up** — The trace-recovery path now surfaces covariance divergence events via telemetry, plus a single `WARN` log line. Operator-configured state (mode, ceilings, margins, efficiency mode, NER threshold, slew step) is explicitly preserved across recovery.
- **`G6_SAFETY_VOLTAGE` reserved** — No longer overloaded for input-range violations (was misleading operators into checking VRMs); now reserved for a future real VRM-ripple check.
- **`last_efficiency` no longer reports garbage values** on fail-closed paths where `power_w` was unvalidated.

**Pre-v1.0 polish (Round 8):**

- **`G6BrainTelemetry` extended** — `best_f`, `best_v`, `model_quality`, `power_model_quality`, `last_efficiency`, `update_count`, `power_update_count` exposed via snapshot. `last_recommended_voltage` retained as a back-compat alias for `best_v`.
- **Named constant `G6_EFFICIENCY_MIN_HR_THS`** — Replaces hardcoded `8.0f` literals in the Dinkelbach solver.
- **`g6_brain_self_test()` is const-correct** — Callers holding a `const G6BrainState *` can now invoke it.
- **End-to-end Dinkelbach test** — Verifies the J/TH solver actually improves efficiency on synthetic surfaces and respects the model-quality gate.

See [`CHANGELOG.md`](CHANGELOG.md) for the full per-bug/per-nit history.

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

Pull requests are welcome. All changes must respect the safety invariants documented in [`docs/AGENTS.md`](docs/AGENTS.md).

---

**The brain your Bitaxe always wanted.**

*May 2026*

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Language: C](https://img.shields.io/badge/Language-C-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Platform: ESP32-S3](https://img.shields.io/badge/Platform-ESP32--S3-red.svg)](https://www.espressif.com/en/products/socs/esp32-s3)
