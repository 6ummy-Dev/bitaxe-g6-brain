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
| **QA Status** | 6 review cycles · All critical findings resolved · Ready for field soak testing |
| **Default Mode** | `G6_MODE_RECOMMEND` |

beta3 is suitable for community field testing. The core mathematics (Joseph-form RLS, analytical Dinkelbach solver) and safety architecture have been hardened through multiple QA passes.

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

Full firmware-level work is out of scope for the brain v1 series. We ship a trustworthy, swappable brain module first.

---

## What’s New in beta3

- **O(1) Analytical J/TH Solver** — Replaced iterative gradient descent with a closed-form solution using Cramer’s rule. Constant-time, guaranteed convergence to the exact minimum of the efficiency surface.
- **Joseph Form Covariance Stabilization** — Full mathematical stabilization of the RLS covariance matrix. Guarantees symmetry and positive semi-definiteness under floating-point arithmetic.
- **3-Sigma Statistical Outlier Gating** — Dynamic innovation variance check that rejects sensor glitches and bus noise before they can corrupt the model.
- **Internal Slew-Rate Limiting** — Frequency and voltage steps are now bounded inside the brain (`dfs_step_mhz` + voltage step limit) even in `AUTO` mode.
- **Dual Model Quality Gates** — J/TH optimization now requires both `model_quality >= 0.6` **and** `power_model_quality >= 0.6`.
- **Critical Safety & NVS Fixes**:
  - Safety layer now executes unconditionally on every call path (including successful RLS updates).
  - Fixed NVS warm-start corruption that was silently breaking the power model after reboot.

---

## Quick Start

1. Drop the component into your ESP-IDF project under `components/g6_brain/`.
2. Add it to your top-level `CMakeLists.txt` via `EXTRA_COMPONENT_DIRS`.
3. Run `idf.py menuconfig` → **Component config → G6 Brain Configuration**.
4. Initialize and call `g6_brain_update()` from your control loop:

```c
#include "g6_brain.h"

static G6BrainState brain;

void app_main(void) {
    g6_brain_init(&brain);
    g6_brain_load_nvs_fingerprint(&brain);
    brain.control_mode = G6_MODE_RECOMMEND;   // safe default

    while (1) {
        g6_brain_update(&brain,
                        current_freq_mhz, current_volt_mv,
                        hashrate_ths, power_w,
                        temp_c, err_pct, share_count);

        float opt_f, opt_v;
        g6_brain_get_optimal(&brain, &opt_f, &opt_v, NULL);
        // Apply opt_f / opt_v at your own pace (or let AUTO mode handle it)

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

Full integration example → [`docs/INTEGRATION_EXAMPLE.c`](docs/INTEGRATION_EXAMPLE.c)

---

## Control Modes

| Mode                  | Behaviour |
|-----------------------|-----------|
| `G6_MODE_OBSERVE_ONLY` | Safety monitoring only. No optimizer action. |
| `G6_MODE_RECOMMEND` *(default)* | Computes optimal setpoints and exposes them via `g6_brain_get_optimal()`. Does **not** mutate `best_f` / `best_v`. |
| `G6_MODE_AUTO`        | Applies internally slew-rate-limited setpoints. Only enable after sustained observation in RECOMMEND mode. |

**All modes** run the complete safety layer (thermal, voltage, NER, outlier gating) on every update.

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
| [`docs/TESTING.md`](docs/TESTING.md) | Field testing guidance for beta3 |
| [`docs/AGENTS.md`](docs/AGENTS.md) | Engineering principles & invariants |
| [`CHANGELOG.md`](CHANGELOG.md) | Detailed version history |
| [`MANIFESTO.md`](MANIFESTO.md) | Project philosophy |

---

## Field Testing Recommendations

1. Start in `G6_MODE_RECOMMEND`.
2. Let the brain learn for at least 24 hours.
3. Monitor `model_quality` and `power_model_quality` in telemetry.
4. Only switch to `AUTO` once both qualities are stable and above 0.6.
5. Report issues with logs, mode, and whether it was a cold or warm boot.

---

## Contributing

Pull requests are welcome. All changes must preserve the safety invariants documented in [`docs/AGENTS.md`](docs/AGENTS.md). Modifications to the RLS core, Joseph update, or analytical J/TH solver require clear mathematical justification.

The `main` branch is the integration target.

---

**The brain your Bitaxe always wanted.**

*May 2026 · v1.0.0-beta3*

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Language: C](https://img.shields.io/badge/Language-C-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Platform: ESP32-S3](https://img.shields.io/badge/Platform-ESP32--S3-red.svg)](https://www.espressif.com/en/products/socs/esp32-s3)
