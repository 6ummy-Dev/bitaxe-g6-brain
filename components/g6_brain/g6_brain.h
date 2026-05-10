#pragma once

#include "esp_err.h"

#define G6_BRAIN_VERSION "1.8.5"

#define G6_PARAMS 6
#define G6_MAX_TEMP_C 70.0f
#define G6_MAX_VOLTAGE_MV 1350.0f

typedef struct {
    float theta[G6_PARAMS];           // [a, b, c, d, e, g] for HR = a*f² + b*v² + c*f*v + d*f + e*v + g
    float P[G6_PARAMS][G6_PARAMS];    // Covariance matrix
    float lambda;                     // Forgetting factor
    float ridge_epsilon;              // Ridge regularization
    float best_f, best_v;
    float best_score;
    float model_quality;
    bool auto_tune_enabled;
    // Multi-objective weights
    float lambda_power, lambda_temp, lambda_error;
    // Puzzle solving helpers
    uint32_t recommended_nonce_start;
    uint32_t recommended_nonce_range;
} G6BrainState;

esp_err_t g6_brain_init(G6BrainState *brain);
esp_err_t g6_brain_load_from_nvs(G6BrainState *brain);
esp_err_t g6_brain_save_to_nvs(G6BrainState *brain);

void g6_brain_update(G6BrainState *brain, float f_mhz, float v_mv, float hr_ths, float power_w, float temp_c, float err_pct);

void g6_brain_auto_step(G6BrainState *brain, float current_f, float current_v);

void g6_brain_get_optimal(G6BrainState *brain, float *opt_f, float *opt_v, float *pred_hr);

void g6_brain_print_full_status(const G6BrainState *brain);

// Puzzle solving
uint32_t g6_brain_get_recommended_nonce_start(G6BrainState *brain);
uint32_t g6_brain_get_recommended_nonce_range(G6BrainState *brain);

void g6_brain_predict_thermal_rise(G6BrainState *brain, float hr, float power, float *rise_c);

// Safety integration (new in v1.8.5)
typedef enum {
    G6_SAFETY_OK,
    G6_SAFETY_VOLTAGE_HIGH,
    G6_SAFETY_TEMP_HIGH,
    G6_SAFETY_DIVERGENCE
} g6_safety_status_t;

// Safety hook
void g6_brain_apply_safety_clamps(G6BrainState *brain, float *f, float *v);
