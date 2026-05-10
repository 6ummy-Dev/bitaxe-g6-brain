#ifndef G6_BRAIN_H
#define G6_BRAIN_H

#include <stdint.h>
#include <stdbool.h>

#define G6_PARAMS 6  // a f² + b v² + c f v + d f + e v + g

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
    // Puzzle Extras state
    float nonce_opt_start;
    float dup_predict_prob;
    uint32_t puzzle_runs;
} G6BrainState;

void g6_brain_init(G6BrainState *brain);
void g6_brain_update(G6BrainState *brain, float f_mhz, float v_mv, float hr_ths, float power_w, float temp_c, float err_pct);
bool g6_brain_get_optimal(const G6BrainState *brain, float *out_f, float *out_v, float *out_pred_hr);
void g6_brain_auto_step(G6BrainState *brain, float current_f, float current_v);
void g6_brain_reset_model(G6BrainState *brain);

// G6 Puzzle Extras
void g6_puzzle_nonce_optimize(G6BrainState *brain, float recent_share_rate, float error_trend);
float g6_puzzle_predict_duplicates(G6BrainState *brain, float nonce_range, float job_difficulty);
void g6_puzzle_extras_run(G6BrainState *brain);
void g6_puzzle_on_device_solver_demo(G6BrainState *brain, float target, float *solution);

#endif