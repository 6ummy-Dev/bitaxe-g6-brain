# Bitaxe G6 Brain ⚡

**v1.0.0-beta3** — Adaptive RLS optimizer with O(1) analytical J/TH solver for the BM1370 ASIC

The G6 Brain is a self-contained ESP-IDF component that models the quadratic relationship between frequency, voltage, hashrate, and power consumption using stabilized Recursive Least Squares. It learns each individual ASIC's response surface in real time and selects safe operating points while enforcing strict hardware constraints on every update.

> **Start safe. Learn. Then optimize.**

---

## Status

| | |
|---|---|
| **Version** | `v1.0.0-beta3` |
| **Target** | ESP32-S3 / Bitaxe Gamma (BM1370) |
| **License** | MIT |
| **QA** | 6 review cycles · 15+ resolved findings · ready for field soak testing |
| **Default mode** | `G6_MODE_RECOMMEND` — computes setpoints but does not apply them |

beta3 is suitable for community field testing. The mathematical core (RLS, Joseph form covariance, Dinkelbach solver) has been verified symbolically. The safety architecture has been audited and corrected. The next milestone is the Triple-8 soak protocol described in [`docs/SAFETY.md`](docs/SAFETY.md).

---

## What's in beta3

- **O(1) Analytical J/TH Solver** — Replaces gradient descent with a closed-form Cramer's-rule solution to the parametric Dinkelbach sub-problem. Constant-time, guaranteed convergence to the exact 2×2 minimum.
- **Joseph Form Covariance Update** — `P_new = (I − k·xᵀ)·P·(I − k·xᵀ)ᵀ / λ`. Guarantees symmetric, positive-semidefinite covariance under floating-point truncation.
- **3-Sigma Statistical Outlier Gating** — Rejects samples where `err² > 9·(xᵀPx + 0.5)`. Protects the response surface from sensor glitches and bus noise.
- **Internal Slew-Rate Limiting** — Stepwise transitions from the ASIC's current physical state toward the mathematical optimum. Bounded by `dfs_step_mhz` (frequency) and `MAX_VOLT_STEP` (voltage).
- **Dual Quality Gating** — J/TH optimization requires both `model_quality ≥ 0.6` and `power_model_quality ≥ 0.6` before any setpoint mutation.
- **NVS Schema v2 Warm-Start** — Full persistence of both RLS models (theta + P matrices) with versioned schema and auto-save throttling.

---

## Quick Start

1. Drop the component into your ESP-IDF project:
   ```
   your-project/
     components/
       g6_brain/        ← copy from this repo
   ```
2. Add `g6_brain` to your top-level `CMakeLists.txt` `EXTRA_COMPONENT_DIRS`.
3. Run `idf.py menuconfig` → **Component config → G6 Brain Configuration**.
4. Initialize the brain and call `g6_brain_update()` from your control loop:

   ```c
   #include "g6_brain.h"
   static G6BrainState brain;

   void app_main(void) {
       g6_brain_init(&brain);
       g6_brain_load_nvs_fingerprint(&brain);   // warm-start if available
       brain.control_mode = G6_MODE_RECOMMEND;  // safe default

       while (1) {
           g6_brain_update(&brain,
                           current_freq_mhz, current_volt_mv,
                           hashrate_ths, power_w,
                           temp_c, err_pct, share_count);

           float opt_f, opt_v;
           g6_brain_get_optimal(&brain, &opt_f, &opt_v, NULL);
           // ... apply opt_f / opt_v at your own pace ...

           vTaskDelay(pdMS_TO_TICKS(1000));
       }
   }
   ```

Full integration walkthrough → [`docs/INTEGRATION_EXAMPLE.c`](docs/INTEGRATION_EXAMPLE.c)

---

## Control Modes

| Mode | Behaviour |
|---|---|
| `G6_MODE_OBSERVE_ONLY` | Safety monitoring only. No optimizer mutations. Use for instrumentation and baseline measurement. |
| `G6_MODE_RECOMMEND` *(default)* | Computes the optimal setpoint and exposes it via `g6_brain_get_optimal()`. Does not mutate `best_f` / `best_v` outside safety overrides. |
| `G6_MODE_AUTO` | Applies the slew-rate-limited setpoint internally. Only enable after you have observed RECOMMEND behaviour for a sustained period. |

Thermal proactive scaling, voltage ripple clamping, and hardware-limit clamping run on every update path regardless of mode.

---

## Documentation

| Document | Read when |
|---|---|
| [`docs/INSTALL.md`](docs/INSTALL.md) | Adding the brain to your firmware |
| [`docs/INTEGRATION_EXAMPLE.c`](docs/INTEGRATION_EXAMPLE.c) | Reference integration |
| [`docs/API.md`](docs/API.md) | Function-level API reference |
| [`docs/KCONFIG.md`](docs/KCONFIG.md) | Tuning safety limits and solver parameters |
| [`docs/SAFETY.md`](docs/SAFETY.md) | Safety invariants and the Triple-8 soak protocol |
| [`docs/MONITORING.md`](docs/MONITORING.md) | Telemetry and runtime observability |
| [`docs/TESTING.md`](docs/TESTING.md) | Community field testing guide |
| [`docs/AGENTS.md`](docs/AGENTS.md) | Engineering principles and invariants |
| [`docs/GLOSSARY.md`](docs/GLOSSARY.md) | Terminology |
| [`docs/REFERENCES.md`](docs/REFERENCES.md) | Algorithmic references and citations |
| [`CHANGELOG.md`](CHANGELOG.md) | Version history |
| [`MANIFESTO.md`](MANIFESTO.md) | Project philosophy |

---

## Field Testing

If you are running beta3 on hardware, please:

1. Start in `G6_MODE_RECOMMEND` and let the brain learn for several hours.
2. Watch `model_quality` and `power_model_quality` in the telemetry log.
3. Only switch to `AUTO` once both qualities have stabilized above 0.6.
4. Report anomalies via GitHub issues — please include the mode, ESP-IDF logs around the event, and whether it was a cold or warm boot.

See [`docs/TESTING.md`](docs/TESTING.md) for the full community testing guide.

---

## Contributing

Pull requests welcome. All changes must preserve the safety invariants documented in [`docs/AGENTS.md`](docs/AGENTS.md). The mathematical core (RLS update, Joseph form, Dinkelbach solver) is treated as critical-path code — modifications require analytical justification, not just empirical tuning.

The `main` branch is the integration target. Tag releases follow `v1.0.0-betaN` semantic versioning.

---

**The brain your Bitaxe always wanted.**

*May 2026 · v1.0.0-beta3*

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Language: C](https://img.shields.io/badge/Language-C-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Platform: ESP32-S3](https://img.shields.io/badge/Platform-ESP32--S3-red.svg)](https://www.espressif.com/en/products/socs/esp32-s3)
[![CI Status](https://github.com/6ummy-Dev/bitaxe-g6-brain/actions/workflows/build.yml/badge.svg)](https://github.com/6ummy-Dev/bitaxe-g6-brain/actions/workflows/build.yml)
