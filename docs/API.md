# G6 Brain Public API — v1.0.0-beta2 (Final)

**Modular, self-optimizing control brain for Bitaxe ESP-Miner (Gamma 602+ / BM1370).**  
Pure Recursive Least Squares (RLS) quadratic response surface modeling with built-in predictive safety and telemetry export.

---

## Public Functions

### `esp_err_t g6_brain_init(G6BrainState *brain)`

Initializes the brain state to safe defaults. Must be called after `nvs_flash_init()`.

**Returns**
- `ESP_OK` on success
- `ESP_ERR_INVALID_ARG` if `brain` is NULL
- `ESP_ERR_NVS_NOT_INITIALIZED` if NVS is not ready

---

### `esp_err_t g6_brain_update(...)`

Feeds one telemetry sample and runs the full optimization + safety cycle.

**Parameters**
- `f_mhz`, `v_mv`, `hr_ths`, `power_w`, `temp_c`, `err_pct`, `share_count`

**Returns**
- `ESP_OK` — processed
- `ESP_ERR_INVALID_ARG` — bad input

**Behavior**
- Respects `control_mode`
- All safety layers always run
- NVS auto-save every ~5 minutes after 10+ updates

---

### `void g6_brain_get_optimal(const G6BrainState *brain, float *opt_f, float *opt_v, float *pred_hr)`

Returns the currently recommended safe operating point (and optionally predicted hashrate).

---

### `float g6_brain_get_model_quality(const G6BrainState *brain)`

Returns current model quality (0.0–1.0).

---

### `float g6_brain_get_cov_condition(const G6BrainState *brain)`

Returns covariance condition number for diagnostics.

---

### `esp_err_t g6_brain_self_test(G6BrainState *brain)`

Runs internal sanity checks on RLS matrices and optimum finder.

---

### `esp_err_t g6_brain_reset(G6BrainState *brain)`

Resets the brain to cold-start state and erases NVS fingerprint.

---

### NVS Persistence

```c
esp_err_t g6_brain_load_nvs_fingerprint(G6BrainState *brain);
esp_err_t g6_brain_save_nvs_fingerprint(const G6BrainState *brain);
```

- `load` is called automatically in `init()`
- `save` is called automatically every ~5 minutes

---

### Telemetry

```c
void g6_brain_get_telemetry(const G6BrainState *brain, G6BrainTelemetry *out);
```

Lightweight zero-copy snapshot of current brain state.  
Call anytime after `g6_brain_update()`. Single-threaded contract only.

**Populates**:
- `theta_hashrate[]` / `trace_P_hashrate`
- `theta_power[]` / `trace_P_power`
- `last_innovation`
- `safety_status`
- `efficiency_mode_active`
- `last_recommended_voltage`

Perfect for logging, OLED, MQTT, or external dashboards.

---

## Key Structs

- `G6BrainState` — main brain state (do not modify fields directly)
- `G6BrainTelemetry` — snapshot returned by `g6_brain_get_telemetry()`
- `G6ControlMode` — `OBSERVE_ONLY` / `RECOMMEND` (default) / `AUTO`
- `G6SafetyStatus` — current safety state

---

**Version:** v1.0.0-beta2 (Final)  
**Single-threaded only.** All calls to `update()`, `get_optimal()`, `reset()`, `get_telemetry()` etc. must be serialized by the caller.