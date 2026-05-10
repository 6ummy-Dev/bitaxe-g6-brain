#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#define G6_BRAIN_VERSION "1.0.0-BETA"

#define G6_PARAMS 6
#define G6_F_MIN_MHZ     500.0f
#define G6_F_MAX_MHZ     950.0f
#define G6_V_MIN_MV     1100.0f
#define G6_V_MAX_MV     1350.0f
#define G6_MAX_TEMP_C     70.0f

// Multi-objective weights
#define G6_DEFAULT_LAMBDA_POWER  0.55f
#define G6_DEFAULT_LAMBDA_TEMP   0.30f
#define G6_DEFAULT_LAMBDA_ERROR  0.15f

typedef struct {
    float theta[G6_PARAMS];           // [a, b, c, d, e, g] for HR ≈ a*f² + b*v² + c*f*v + d*f + e*v + g
    float P[G6_PARAMS][G6_PARAMS];    // Covariance matrix for RLS
    float lambda;                     // Forgetting factor
    float ridge;                      // Ridge regularization
    float model_quality;              // 0.0 - 1.0

    float best_f_mhz;
    float best_v_mv;
    float best_pred_hr;

    bool  auto_tune_enabled;
    float lambda_power;
    float lambda_temp;
    float lambda_error;

    uint32_t recommended_nonce_start;
    uint32_t recommended_nonce_range;
} G6BrainState;

esp_err_t g6_brain_init(G6BrainState *brain);
esp_err_t g6_brain_update(G6BrainState *brain, float f_mhz, float v_mv, float hr_ths, float power_w, float temp_c, float err_pct);
esp_err_t g6_brain_get_optimal(const G6BrainState *brain, float *opt_f, float *opt_v, float *pred_hr);
esp_err_t g6_brain_auto_step(G6BrainState *brain, float current_f, float current_v, float *new_f, float *new_v);

void g6_brain_apply_safety_clamps(float *f, float *v);
void g6_brain_print_status(const G6BrainState *brain);

esp_err_t g6_brain_save_to_nvs(const G6BrainState *brain);
esp_err_t g6_brain_load_from_nvs(G6BrainState *brain);

uint32_t g6_brain_get_recommended_nonce_start(void);
uint32_t g6_brain_get_recommended_nonce_range(void);

void g6_brain_predict_thermal_rise(float power_w, float *rise_c);

#endif