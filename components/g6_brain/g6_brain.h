#ifndef G6_BRAIN_H
#define G6_BRAIN_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#define G6_BRAIN_VERSION "1.6.0"
#define G6_PARAMS 6

typedef struct {
    float theta[G6_PARAMS];
    float P[G6_PARAMS][G6_PARAMS];
    float lambda;
    float best_f, best_v;
    float best_hr, best_eff;
    uint32_t update_count;
    bool auto_tune_enabled;
    float max_temp_c;
    float max_error_pct;
    float min_improvement_pct;
    float model_quality;
} G6BrainState;

void g6_brain_init(G6BrainState *brain);
void g6_brain_update(G6BrainState *brain, float f_mhz, float v_mv, float hr_ths, float power_w, float temp_c, float err_pct);
bool g6_brain_get_optimal(const G6BrainState *brain, float *out_f, float *out_v, float *out_pred_hr);
void g6_brain_auto_step(G6BrainState *brain, float current_f, float current_v);
void g6_brain_reset(G6BrainState *brain);

// Persistence
esp_err_t g6_brain_save_to_nvs(const G6BrainState *brain);
esp_err_t g6_brain_load_from_nvs(G6BrainState *brain);

#endif