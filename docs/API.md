# G6 Brain Public API — v1.0.0-beta5

**Adaptive RLS optimizer with real-time quadratic modeling and analytical J/TH solver for BM1370.**

---

## Threading Contract

> **CRITICAL:** This module is completely single-threaded and contains no internal locking mechanisms. All calls to core functions must be explicitly serialized by the caller.

---

## Core Functions

### `esp_err_t g6_brain_init(G6BrainState *brain)`
- **Purpose:** Initializes internal structures, loads default Kconfig configurations, sets initial state vectors, and attempts to restore cached models from non-volatile storage.
- **Parameters:** Pointer to allocated `G6BrainState` instance.
- **Returns:** `ESP_OK` on successful execution.

### `esp_err_t g6_brain_update(...)`

```c
esp_err_t g6_brain_update(G6BrainState *brain,
                          float    f_mhz,
                          float    v_mv,
                          float    hr_ths,
                          float    power_w,
                          float    temp_c,
                          float    vr_temp_c,
                          float    err_pct,
                          uint32_t share_count);
```

- **Purpose:** Feeds one real-time telemetry frame into the brain. Runs input validation, 3-sigma outlier filtering, Joseph-form covariance update, internal slew-rate limiting, and the safety layer on every call path.
- **Parameters:**

  | Field | Units | Notes |
  |---|---|---|
  | `f_mhz` | MHz | Current ASIC frequency |
  | `v_mv` | mV | Current core voltage |
  | `hr_ths` | TH/s | Measured hashrate |
  | `power_w` | W | Measured power draw (must be `0..100`) |
  | `temp_c` | °C | Current ASIC die temperature |
  | `vr_temp_c` | °C | Voltage regulator temperature. Pass `G6_VR_TEMP_NO_SENSOR` (`-1.0f`) if no VR sensor is available — all VR thermal checks are silently skipped. |
  | `err_pct` | % | Nonce error rate (0..100) |
  | `share_count` | count | Shares observed during the measurement window. Pass `0` if unknown. |

- **Returns:** `ESP_OK` on success, `ESP_ERR_INVALID_ARG` if any input fails validation (non-finite, out of range, or negative power).

### `void g6_brain_get_optimal(const G6BrainState *brain, float *opt_f, float *opt_v, float *pred_hr)`
- **Purpose:** Retrieves the current target tracking coordinates computed by the optimizer. When efficiency mode is active, this wraps the O(1) analytical solver to isolate coordinates tracking the lowest J/TH ratio.

---

## Utility & Telemetry Functions

### `float g6_brain_get_model_quality(const G6BrainState *brain)`
- **Purpose:** Returns the current model fit confidence metric (0.0 to 1.0).

### `float g6_brain_get_cov_condition(const G6BrainState *brain)`
- **Purpose:** Computes the numeric condition number of the tracking covariance matrix to detect parameter divergence boundaries.

### `esp_err_t g6_brain_self_test(G6BrainState *brain)`
- **Purpose:** Validates matrix symmetry and checks condition parameters to determine if estimators are running normally or are running in a degraded state.

### `esp_err_t g6_brain_reset(G6BrainState *brain)`
- **Purpose:** Wipes stored NVS parameters, re-initializes models to cold-start matrices, and resets runtime variables to defaults.

### `void g6_brain_get_telemetry(const G6BrainState *brain, G6BrainTelemetry *out)`
- **Purpose:** Populates a `G6BrainTelemetry` snapshot for monitoring and logging. The struct contains:
  - `theta_hashrate[6]`, `theta_power[6]` — current RLS coefficients
  - `trace_P_hashrate`, `trace_P_power` — covariance matrix traces
  - `last_innovation` — most recent hashrate prediction error
  - `safety_status` — `G6SafetyStatus` enum
  - `efficiency_mode_active` — `true` when `G6_ENABLE_EFFICIENCY_MODE` is on
  - `last_recommended_voltage` — most recent recommended core voltage in mV
- **Threading:** Single-threaded only, like all other public calls.

---

## Storage Functions

### `esp_err_t g6_brain_load_nvs_fingerprint(G6BrainState *brain)`
- **Purpose:** Explicitly handles reading valid schema vectors from partition tables to load warm-start data.

### `esp_err_t g6_brain_save_nvs_fingerprint(const G6BrainState *brain)`
- **Purpose:** Commits active model state matrices to non-volatile storage. Automatically invoked periodically inside `g6_brain_update` once initialization criteria are reached.
