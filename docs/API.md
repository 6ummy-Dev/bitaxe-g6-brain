# G6 Brain Public API — v1.0.0-beta3

**Modular, self-optimizing control brain for Bitaxe ESP-Miner (Gamma 602+ / BM1370).** Pure Recursive Least Squares (RLS) quadratic response surface modeling with built-in predictive safety and telemetry export.

---

## Public Functions

### J/TH Optimization (Phase 2 / beta3)

The brain now includes a highly efficient **Dinkelbach-based J/TH optimizer** when `G6_ENABLE_EFFICIENCY_MODE` is enabled.

This replaces the previous brute-force grid search with an exact $O(1)$ analytical minimum solver.

**Key behaviors:**
- Protected by dual quality gates (skips optimization if `model_quality < 0.6` or `power_model_quality < 0.6`)
- Configurable via Kconfig (`G6_JTH_MAX_OUTER_ITERS`)
- Always respects safe operating limits

---

### `esp_err_t g6_brain_init(G6BrainState *brain)`

Initializes the brain state to safe defaults. Must be called after `nvs_flash_init()`.

---

### `esp_err_t g6_brain_update(...)`

Feeds one telemetry sample and runs the full optimization + safety cycle.

---

### `void g6_brain_get_optimal(const G6BrainState *brain, float *opt_f, float *opt_v, float *pred_hr)`

Returns the currently recommended safe operating point.

When efficiency mode is enabled, this now uses the Dinkelbach-based J/TH optimizer (instead of grid search).

---

### Other Functions

- `float g6_brain_get_model_quality(const G6BrainState *brain)`
- `float g6_brain_get_cov_condition(const G6BrainState *brain)`
- `esp_err_t g6_brain_self_test(G6BrainState *brain)`
- `esp_err_t g6_brain_reset(G6BrainState *brain)`
- `void g6_brain_get_telemetry(const G6BrainState *brain, G6BrainTelemetry *out)`

---

**Version:** v1.0.0-beta3 (Phase 2)  
**Single-threaded only.**
