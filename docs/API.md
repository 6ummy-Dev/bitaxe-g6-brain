# G6 Brain Public API — v1.0.0-beta6

**Adaptive RLS optimizer with real-time quadratic modeling and analytical J/TH solver for BM1370.**

---

## Threading Contract

> **CRITICAL:** This module is completely single-threaded and contains no internal locking mechanisms. All calls to core functions must be explicitly serialized by the caller.

---

## Core Functions

### `esp_err_t g6_brain_init(G6BrainState *brain)`
- **Purpose:** Initializes internal structures, loads default Kconfig configurations, sets initial state vectors, and attempts to restore cached models from non-volatile storage.
- **Parameters:** Pointer to allocated `G6BrainState` instance.
- **Returns:** `ESP_OK` on success, `ESP_ERR_INVALID_ARG` if `brain` is `NULL`, or `ESP_ERR_NVS_NOT_INITIALIZED` if the NVS subsystem has not been initialized by the host application.

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

- **Purpose:** Feeds one real-time telemetry frame into the brain. Runs input validation, 3-sigma outlier filtering, stabilized RLS covariance update with ridge regularization, internal slew-rate limiting, and the safety layer on every call path.

- **Parameters:**

| Field | Units | Notes |
| --- | --- | --- |
| `f_mhz` | MHz | Current ASIC frequency. Validated against `BM1370_F_MIN`/`MAX`. |
| `v_mv` | mV | Current core voltage. Validated against `BM1370_V_MIN`/`MAX`. |
| `hr_ths` | TH/s | Measured hashrate. Validated as finite and `> 0`. |
| `power_w` | W | Measured power draw. Validated as finite and within `[0, 100]`. |
| `temp_c` | °C | Current ASIC die temperature. Validated as finite only — see [Sensor Sanity](#sensor-sanity) below. |
| `vr_temp_c` | °C | Voltage regulator temperature. Pass `G6_VR_TEMP_NO_SENSOR` (`-1.0f`) if no VR sensor is available — all VR thermal checks are silently skipped. Validated as finite only. |
| `err_pct` | % | Nonce error rate (0..100). Validated as finite only. |
| `share_count` | count | Shares observed during the measurement window. Pass `0` if unknown. |

- **Returns:**
  - `ESP_OK` on every call where `brain` is a valid pointer — **including** calls with bad numeric inputs. Out-of-bounds, non-finite (NaN/Inf), or otherwise unusable telemetry is routed fail-closed to the safety layer per manifesto non-negotiable 3.7 ("Every safety check executes even on invalid or rejected samples"). The caller should inspect `last_safety_status` (or the telemetry snapshot) to see how the frame was handled — see the [Safety Status Reference](#safety-status-reference) below.
  - `ESP_ERR_INVALID_ARG` **only** when `brain == NULL`. This is the sole structurally-broken call — no brain instance exists for the safety layer to act on. All other failures (NaN, Inf, out-of-bounds, `hr_ths <= 0`) report through `last_safety_status` with `G6_SAFETY_INPUT_RANGE` rather than via an error code.

#### Sensor Sanity

The brain validates **finiteness** on every input and **hardware bounds** on `f_mhz`, `v_mv`, `hr_ths`, and `power_w`. The temperature and error-rate channels (`temp_c`, `vr_temp_c`, `err_pct`) are validated as finite only — beyond that, the brain trusts that finite values within the C `float` domain represent real readings. It does **not** attempt to detect stuck-low, stuck-high, or implausible-but-finite sensor failures on these channels. A stuck-low temperature reading (e.g. `-50°C`) will pass the input gate and be treated as a cold healthy chip. Sensor health monitoring belongs in the integrator's telemetry layer; see `docs/SAFETY.md` for recommended upstream checks.

### `void g6_brain_get_optimal(const G6BrainState *brain, float *opt_f, float *opt_v, float *pred_hr)`

- **Purpose:** Retrieves the current target tracking coordinates computed by the optimizer. When efficiency mode is active *and* both `model_quality` and `power_model_quality` are at least `0.6`, this also runs the bounded Dinkelbach J/TH solver to refine the coordinates toward the minimum-W/TH operating point. Otherwise returns the hashrate-only optimum.
- `pred_hr` may be `NULL` if not needed.

---

## Utility & Telemetry Functions

### `float g6_brain_get_model_quality(const G6BrainState *brain)`

- **Purpose:** Returns the current hashrate-model fit confidence metric (0.0 to 1.0). For the power model, read `power_model_quality` from the telemetry snapshot.

### `float g6_brain_get_cov_condition(const G6BrainState *brain)`

- **Purpose:** Returns an *upper-bound estimate* of the tracking covariance matrix's 2-norm condition number, used to detect parameter-divergence boundaries. The estimate comes from Gershgorin disc bounds (`max_i(P_ii + Σ_{j≠i}|P_ij|) / min_i(P_ii − Σ_{j≠i}|P_ij|)`), so — unlike a bare `max_diag/min_diag` ratio — it accounts for off-diagonal mass and cannot report a near-singular matrix as well-conditioned. If the lower Gershgorin bound is non-positive (near-indefinite), a large sentinel (`RLS_P_CLAMP_MAX/RLS_P_CLAMP_MIN`) is returned so callers treat the matrix as ill-conditioned.

### `esp_err_t g6_brain_self_test(const G6BrainState *brain)`

- **Purpose:** Validates matrix symmetry, diagonal range, and the Gershgorin condition-number estimate (see `g6_brain_get_cov_condition` above) to determine if estimators are running normally or in a degraded state. Returns `ESP_OK` when healthy, `ESP_FAIL` when any check fails, or `ESP_ERR_INVALID_ARG` if `brain` is `NULL`.
- **Const-correct:** the function only reads from `brain`; callers holding a `const G6BrainState *` can invoke it directly.

### `esp_err_t g6_brain_reset(G6BrainState *brain)`

- **Purpose:** Wipes stored NVS parameters, re-initializes models to cold-start matrices, and resets runtime variables to defaults.

### `void g6_brain_get_telemetry(const G6BrainState *brain, G6BrainTelemetry *out)`

- **Purpose:** Populates a `G6BrainTelemetry` snapshot for monitoring and logging. Single-threaded only, like all other public calls.

The `G6BrainTelemetry` struct exposes a consistent point-in-time view of brain state:

| Field | Type | Notes |
| --- | --- | --- |
| `theta_hashrate[6]` | `float[]` | Current hashrate RLS coefficients |
| `theta_power[6]` | `float[]` | Current power RLS coefficients (zeros when efficiency mode disabled) |
| `trace_P_hashrate` | `float` | Trace of the hashrate covariance matrix |
| `trace_P_power` | `float` | Trace of the power covariance matrix |
| `last_innovation` | `float` | Most recent hashrate prediction error |
| `best_f` | `float` | Currently recommended frequency, MHz |
| `best_v` | `float` | Currently recommended core voltage, mV |
| `model_quality` | `float` | Hashrate model fit confidence, 0.0–1.0 |
| `power_model_quality` | `float` | Power model fit confidence, 0.0–1.0 |
| `last_efficiency` | `float` | Most recent W/TH ratio (only updated when `power_w` is within sanity bounds) |
| `update_count` | `uint32_t` | Number of accepted hashrate RLS updates since last reset |
| `power_update_count` | `uint32_t` | Number of accepted power RLS updates (efficiency mode only) |
| `last_update_timestamp` | `uint32_t` | FreeRTOS tick (`xTaskGetTickCount()`) of the most recent accepted RLS update. Advances iff `update_count` advances; both move together on accepted samples, neither moves on rejected ones. Wraps roughly every 49.7 days at the 1ms default tick rate — operators comparing across long windows should reset or wrap-correct. |
| `safety_status` | `G6SafetyStatus` | Current operational state (see [Safety Status Reference](#safety-status-reference)) |
| `efficiency_mode_active` | `bool` | `true` when `use_efficiency_mode` is set at runtime |
| `last_recommended_voltage` | `float` | Backward-compatibility alias — mirrors `best_v` exactly |

---

## Safety Status Reference

`G6SafetyStatus` values that may appear in `last_safety_status` or `G6BrainTelemetry.safety_status`:

| Value | Meaning |
| --- | --- |
| `G6_SAFETY_OK` | No anomaly observed this tick. Also reported on non-anomaly sample rejections (low share count, insignificant innovation) — distinguish "accepted" from "rejected for non-anomaly reasons" by watching `update_count` deltas |
| `G6_SAFETY_THERMAL` | ASIC die temperature at or near the hard ceiling (proactive zone or above) |
| `G6_SAFETY_VR_THERMAL` | VR regulator temperature at or near its ceiling |
| `G6_SAFETY_VOLTAGE` | Reserved for a future VRM ripple check (not currently set by any code path) |
| `G6_SAFETY_POWER_SANITY` | Reported `power_w` outside the physically plausible range, or a power-model outlier was rejected |
| `G6_SAFETY_NER_BACKOFF` | Nonce error rate exceeded `G6_NER_THRESHOLD`; conservative back-off applied |
| `G6_SAFETY_SAMPLE_QUALITY` | Hashrate-model statistical outlier rejected by the 3-sigma gate |
| `G6_SAFETY_P_MATRIX_SINGULAR` | Covariance matrix trace diverged; the brain auto-recovered into a fresh cold-start (preserving operator config). Telemetry exposes this so operators know recovery happened |
| `G6_SAFETY_INPUT_RANGE` | Input telemetry failed validation: non-finite, `hr_ths <= 0`, or `f_mhz`/`v_mv` outside BM1370 hardware bounds |

Status priority on a single tick: thermal/VR-thermal helpers run last in the safety layer and may overwrite earlier statuses. If both ASIC and VR thermal conditions fire on the same tick, ASIC wins.

---

## Storage Functions

### `esp_err_t g6_brain_load_nvs_fingerprint(G6BrainState *brain)`

- **Purpose:** Loads warm-start data from NVS. On schema or blob-size mismatch the stale blob is erased and the brain begins from a fresh cold start.

### `esp_err_t g6_brain_save_nvs_fingerprint(const G6BrainState *brain)`

- **Purpose:** Commits active model state matrices (`theta`, `P`, `power_theta`, `power_P`) to non-volatile storage. Automatically invoked periodically inside `g6_brain_update` once `update_count > 10`.

---

## Public Constants

Defined in `g6_brain.h`. Useful for callers that want to check input ranges or interpret telemetry:

| Macro | Value | Purpose |
| --- | --- | --- |
| `BM1370_F_MIN` / `BM1370_F_MAX` | 400 / 950 MHz | Hardware frequency bounds |
| `BM1370_V_MIN` / `BM1370_V_MAX` | 1050 / 1350 mV | Hardware voltage bounds |
| `G6_VR_TEMP_NO_SENSOR` | `-1.0f` | Sentinel for `vr_temp_c` when no VR sensor is wired |
| `G6_EFFICIENCY_MIN_HR_THS` | `0.5f` TH/s | Minimum predicted hashrate below which the Dinkelbach solver skips a candidate point (prevents near-zero division and degenerate efficiency calculations). Must sit well below the BM1370's real hashrate (~1.0–1.2 TH/s stock, ~1.5 OC); the prior `8.0` value silently disabled efficiency mode on real hardware. Feed `hr_ths` in TH/s — convert AxeOS GH/s by dividing by 1000. |
| `MIN_SHARE_COUNT` | 20 | Minimum shares before a sample is considered for RLS update |
| `RLS_N` | 6 | Number of RLS coefficients (quadratic in two variables) |
