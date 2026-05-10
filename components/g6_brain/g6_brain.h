#ifndef G6_BRAIN_H
#define G6_BRAIN_H

#include <stdint.h>
#include <stdbool.h>

#define G6_BRAIN_VERSION "1.5.5"
#define G6_PARAMS 6  // HR(f,v) = a*f² + b*v² + c*f*v + d*f + e*v + g

typedef struct {
    float theta[G6_PARAMS];      // RLS coefficients [a, b, c, d, e, g]
    float P[G6_PARAMS][G6_PARAMS]; // covariance inverse (uncertainty matrix)
    float lambda;                // forgetting factor (0.97 typical)
    float best_f, best_v;        // best observed operating point
    float best_hr, best_eff;     // best hashrate & efficiency (TH/s / W)
    uint32_t update_count;
    bool auto_tune_enabled;
    float max_temp_c;            // hard safety limit
    float max_error_pct;         // max acceptable error rate
    float min_improvement_pct;   // min % predicted gain to justify retune
    float model_quality;         // 0-1, higher = more confident in fit
} G6BrainState;

void g6_brain_init(G6BrainState *brain);
void g6_brain_update(G6BrainState *brain, float f_mhz, float v_mv, float hr_ths, float power_w, float temp_c, float err_pct);
bool g6_brain_get_optimal(const G6BrainState *brain, float *out_f, float *out_v, float *out_pred_hr);
void g6_brain_auto_step(G6BrainState *brain, float current_f, float current_v);
void g6_brain_reset(G6BrainState *brain);

#endif