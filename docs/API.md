# G6 Brain Public API — v1.0.0-beta2

**Modular, self-optimizing control brain for Bitaxe ESP-Miner (Gamma 602+ / BM1370).**  
Pure Recursive Least Squares (RLS) quadratic response surface modeling with built-in predictive safety.

> **Thread Safety Note** (from header): `G6BrainState` is currently updated from a **single thread** (ESP-Miner main loop / dedicated brain task). No mutex is required yet. Add one only if you multi-task the brain in the future.

---

## Public Functions

### `esp_err_t g6_brain_init(G6BrainState *brain)`

Initializes the brain state to safe defaults.

**Parameters**
- `brain` — Pointer to uninitialized `G6BrainState` (must be zeroed or stack-allocated)

**Returns**
- `ESP_OK` on success
- `ESP_ERR_INVALID_ARG` if `brain` is NULL

**Usage**
Call once at startup (before any `update` calls). Performs cold-start initialization of RLS matrices and loads NVS fingerprint if enabled.

**Example**
```c
G6BrainState brain;
esp_err_t ret = g6_brain_init(&brain);
if (ret != ESP_OK) { /* handle error */ }
```

---

### `esp_err_t g6_brain_update(G6BrainState *brain, float f_mhz, float v_mv, float hr_ths, float power_w, float temp_c, float err_pct, uint32_t share_count)`

Feeds one telemetry sample into the RLS model and runs the safety + optimization loop.

**Parameters** (all in real units)
- `f_mhz` — Current ASIC frequency (MHz)
- `v_mv` — Actual measured core voltage (mV)
- `hr_ths` — Current hashrate (TH/s)
- `power_w` — Power consumption (W)
- `temp_c` — ASIC temperature (°C)
- `err_pct` — Hardware error rate (%)
- `share_count` — Number of valid shares in the current measurement window (pass `0` if unknown)

**Returns**
- `ESP_OK` — Sample accepted and model updated
- `ESP_ERR_INVALID_ARG` — NULL brain or out-of-range values

**Behavior**
- Runs quadratic RLS update (6 coefficients)
- Applies thermal ceiling, voltage protection, and safety checks
- Updates `best_f` / `best_v` when a better safe operating point is found
- Tracks model quality and cold-start state

**Recommended call rate**: Every 20–30 seconds.

---

### `void g6_brain_get_optimal(const G6BrainState *brain, float *opt_f, float *opt_v, float *pred_hr)`

Returns the currently recommended safe operating point.

**Parameters**
- `brain` — Initialized brain state
- `opt_f` — Output: recommended frequency (MHz) — **never NULL**
- `opt_v` — Output: recommended voltage (mV) — **never NULL**
- `pred_hr` — Optional output: predicted hashrate at this point (TH/s). Pass `NULL` if not needed.

**Notes**
- Values are already clamped to BM1370 safe ranges (400–950 MHz, 1050–1350 mV)
- Includes predictive safety margin
- Call after `update()` to get the next settings to apply

---

### `float g6_brain_get_model_quality(const G6BrainState *brain)`

Returns current model quality metric (0.0–1.0).

- **> 0.85** → Excellent fit, trust the optimizer
- **0.6–0.85** → Good, continue collecting samples
- **< 0.6** → Poor fit (cold start or noisy data) — fall back to conservative settings

---

### `esp_err_t g6_brain_self_test(G6BrainState *brain)`

Runs synthetic data sanity checks on the RLS solver and optimum finder.

**Returns**
- `ESP_OK` — All internal tests passed
- Error if mathematical integrity check fails

Useful during development or after major changes.

---

### NVS Silicon Fingerprint (per-chip warm-start)

```c
esp_err_t g6_brain_load_nvs_fingerprint(G6BrainState *brain);
esp_err_t g6_brain_save_nvs_fingerprint(const G6BrainState *brain);
```

- Stores learned RLS coefficients + best setpoint per physical chip
- Survives power cycles and reduces cold-start time

---

## Key Struct: `G6BrainState`

Contains:
- Full 6×6 RLS covariance matrix `P` and coefficient vector `theta`
- Best safe setpoint (`best_f`, `best_v`)
- Safety config (`temp_ceiling`, `ner_threshold`)
- Sample quality state machine (`BrainSampleState`)
- NVS and timing counters

**Do not** modify fields directly — use the public API.

Full definition in `components/g6_brain/g6_brain.h`.

---

## BM1370 Safe Operating Ranges (hard-coded)

| Parameter     | Min     | Center  | Max     |
|---------------|---------|---------|---------|
| Frequency     | 400 MHz | 650 MHz | 950 MHz |
| Voltage       | 1050 mV | 1220 mV | 1350 mV |

The brain will **never** recommend values outside these ranges.

---

## Next Steps

- **Installation & quick start** → [INSTALL.md](INSTALL.md)
- **Recommended integration example** → [INTEGRATION_EXAMPLE.c](INTEGRATION_EXAMPLE.c)
- **Kconfig options** → [KCONFIG.md](KCONFIG.md)
- **Safety & engineering principles** → [AGENTS.md](../AGENTS.md)

---

**License**: MIT  
**Version**: v1.0.0-beta2 (May 2026)  
**Maintainer**: 6ummy-Dev + Grok (xAI)
