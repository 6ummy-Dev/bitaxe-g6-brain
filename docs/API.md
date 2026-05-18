# G6 Brain Public API — v1.0.0-beta2 (Phase 0 — fully wired)

**Modular, self-optimizing control brain for Bitaxe ESP-Miner (Gamma 602+ / BM1370).**  
Pure Recursive Least Squares (RLS) quadratic response surface modeling with built-in predictive safety.

> **Phase 0 Updates (now live):**  
> - All Kconfig options are wired and read at runtime  
> - `G6ControlMode` is **enforced** in `update()` and `get_optimal()`  
> - NVS auto-save of theta + full P-matrix every ~5 minutes (true warm-start)  
> - Efficiency honesty: This is a **safe hashrate maximizer** (quadratic argmax of HR(f,v) with hard safety). True J/TH optimization planned for Phase 1.

---

## Public Functions

### `esp_err_t g6_brain_init(G6BrainState *brain)`

Initializes the brain state to safe defaults. Must be called after `nvs_flash_init()`.

**Parameters**
- `brain` — Pointer to uninitialized `G6BrainState`

**Returns**
- `ESP_OK` on success
- `ESP_ERR_INVALID_ARG` if `brain` is NULL
- `ESP_ERR_NVS_NOT_INITIALIZED` if NVS is not ready

**Behavior (Phase 0)**
- Loads Kconfig values (`G6_TEMP_CEILING`, `G6_NER_THRESHOLD`, etc.)
- Sets default control mode to `G6_MODE_RECOMMEND`
- Loads NVS fingerprint if available
- Starts NVS auto-save timer

**Example**
```c
G6BrainState brain;
esp_err_t ret = g6_brain_init(&brain);
if (ret != ESP_OK) { /* handle error */ }
```

---

### `esp_err_t g6_brain_update(...)`

Feeds one telemetry sample and runs the full optimization + safety cycle.

**Parameters** (all in real units)
- `f_mhz`, `v_mv`, `hr_ths`, `power_w`, `temp_c`, `err_pct` — telemetry
- `share_count` — Valid shares in current window (0 = skip share validation)

**Returns**
- `ESP_OK` — processed
- `ESP_ERR_INVALID_ARG` — bad input

**Phase 0 Behavior**
- Respects `brain->control_mode`:
  - `OBSERVE_ONLY`: safety only, no optimal changes
  - `RECOMMEND`: computes optimal but does **not** mutate `best_f`/`best_v`
  - `AUTO`: full optimizer (original behavior)
- Periodic NVS auto-save after 10+ updates
- All safety layers (thermal, NER, voltage, power sanity) always run

**Recommended call rate**: Every 20–30 seconds.

---

### `void g6_brain_get_optimal(const G6BrainState *brain, float *opt_f, float *opt_v, float *pred_hr)`

Returns the currently recommended safe operating point.

**Phase 0 Note**
- `opt_f` / `opt_v` are always clamped to BM1370 safe ranges
- In `OBSERVE_ONLY`/`RECOMMEND` modes the returned values may differ from `best_f`/`best_v` (which remain unchanged)

---

### `float g6_brain_get_model_quality(const G6BrainState *brain)`

Returns current model quality (0.0–1.0).  
- > 0.85 → Excellent  
- 0.6–0.85 → Good  
- < 0.6 → Poor (conservative mode)

---

### `float g6_brain_get_cov_condition(const G6BrainState *brain)`

Returns covariance condition number for diagnostics.

---

### `esp_err_t g6_brain_self_test(G6BrainState *brain)`

Runs internal sanity checks on RLS matrices and optimum finder.

---

### NVS Silicon Fingerprint (warm-start)

```c
esp_err_t g6_brain_load_nvs_fingerprint(G6BrainState *brain);
esp_err_t g6_brain_save_nvs_fingerprint(const G6BrainState *brain);
```

- `load` is called automatically in `init()`
- `save` is now called automatically every ~5 minutes (Phase 0)

---

## Key Struct: `G6BrainState`

**Important Phase 0 fields**
- `control_mode` — now enforced (default: `G6_MODE_RECOMMEND`)
- `best_f`, `best_v` — only mutated in `AUTO` mode
- `nvs_last_write_tick` — used for auto-save
- All Kconfig-driven values are populated in `init()`

**Do not** modify fields directly — use the public API.

---

## BM1370 Safe Operating Ranges (hard-coded)

| Parameter     | Min     | Center  | Max     |
|---------------|---------|---------|---------|
| Frequency     | 400 MHz | 650 MHz | 950 MHz |
| Voltage       | 1050 mV | 1220 mV | 1350 mV |

---

## Next Steps

- Recommended integration example → [INTEGRATION_EXAMPLE.c](INTEGRATION_EXAMPLE.c)
- Kconfig options → [KCONFIG.md](KCONFIG.md)
- Safety & engineering principles → [AGENTS.md](../AGENTS.md)

**Version:** v1.0.0-beta2 (Phase 0 fixes applied — May 2026)  
**Maintainer:** 6ummy-Dev + Grok (xAI)
