# G6 Brain Public API

```c
esp_err_t g6_brain_init(G6BrainState *brain);
esp_err_t g6_brain_update(G6BrainState *brain, float f_mhz, float v_mv, float hr_ths, float power_w, float temp_c, float err_pct);
void g6_brain_get_optimal(const G6BrainState *brain, float *opt_f, float *opt_v, float *pred_hr);
float g6_brain_get_model_quality(const G6BrainState *brain);
```

See `g6_brain.h` for full struct and constants.
