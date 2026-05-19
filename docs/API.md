# G6 Brain Public API — v1.0.0-beta3

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

### `esp_err_t g6_brain_update(G6BrainState *brain, float f_mhz, float v_mv, float hr_ths, float power_w, float temp_c, float err_pct, uint32_t share_count)`
- **Purpose:** Feeds a real-time telemetry log frame into the tracking logic. Runs 3-Sigma outlier filtering, processes recursive updates via Joseph Form covariance stabilization, evaluates internal slew limits, and executes mandatory fallback safety boundaries.
- **Returns:** `ESP_OK` on success, `ESP_ERR_INVALID_ARG` if telemetry validation checks fail.

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
- **Purpose:** Generates a zero-copy snapshot structure mapping live coefficients, matrix trace sizes, and tracking mode statistics.

---

## Storage Functions

### `esp_err_t g6_brain_load_nvs_fingerprint(G6BrainState *brain)`
- **Purpose:** Explicitly handles reading valid schema vectors from partition tables to load warm-start data.

### `esp_err_t g6_brain_save_nvs_fingerprint(const G6BrainState *brain)`
- **Purpose:** Commits active model state matrices to non-volatile storage. Automatically invoked periodically inside `g6_brain_update` once initialization criteria are reached.
