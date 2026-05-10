#include "g6_brain.h"
#include <math.h>
#include <string.h>

// Matrix helpers
static void mat_vec_mul(float mat[6][6], float vec[6], float out[6]) {
    for (int i = 0; i < 6; i++) {
        out[i] = 0;
        for (int j = 0; j < 6; j++) out[i] += mat[i][j] * vec[j];
    }
}

static void rank1_update(float mat[6][6], float k[6], float phi[6], float lambda) {
    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 6; j++) {
            mat[i][j] = (mat[i][j] - k[i] * phi[j]) / lambda;
        }
    }
}

void g6_brain_init(G6BrainState *brain) {
    memset(brain, 0, sizeof(G6BrainState));
    brain->lambda = 0.97f;
    brain->max_temp_c = 65.0f;
    brain->max_error_pct = 1.5f;
    brain->min_improvement_pct = 2.0f;
    brain->auto_tune_enabled = true;
    brain->nonce_opt_start = 0.0f;
    brain->dup_predict_prob = 0.0f;

    for (int i = 0; i < G6_PARAMS; i++) {
        for (int j = 0; j < G6_PARAMS; j++) brain->P[i][j] = (i == j) ? 1000.0f : 0.0f;
    }
    brain->theta[0] = -0.000001f;
    brain->theta[1] = -0.0001f;
    brain->theta[2] = 0.00001f;
    brain->theta[3] = 0.002f;
    brain->theta[4] = 0.5f;
    brain->theta[5] = 0.0f;
}

void g6_brain_update(G6BrainState *brain, float f, float v, float hr, float power, float temp, float err) {
    if (temp > brain->max_temp_c || err > brain->max_error_pct) return;

    float phi[G6_PARAMS] = {f*f, v*v, f*v, f, v, 1.0f};
    float y = hr;

    float phiT_theta = 0;
    for (int i = 0; i < G6_PARAMS; i++) phiT_theta += phi[i] * brain->theta[i];
    float e = y - phiT_theta;

    float P_phi[G6_PARAMS];
    mat_vec_mul(brain->P, phi, P_phi);
    float denom = brain->lambda;
    for (int i = 0; i < G6_PARAMS; i++) denom += phi[i] * P_phi[i];
    float k[G6_PARAMS];
    for (int i = 0; i < G6_PARAMS; i++) k[i] = P_phi[i] / denom;

    for (int i = 0; i < G6_PARAMS; i++) brain->theta[i] += k[i] * e;
    rank1_update(brain->P, k, phi, brain->lambda);

    brain->update_count++;

    float eff = hr / fmaxf(power, 0.1f);
    if (eff > brain->best_eff) {
        brain->best_f = f; brain->best_v = v; brain->best_hr = hr; brain->best_eff = eff;
    }
}

bool g6_brain_get_optimal(const G6BrainState *brain, float *out_f, float *out_v, float *out_pred_hr) {
    float a = brain->theta[0], b = brain->theta[1], c = brain->theta[2];
    float d = brain->theta[3], e = brain->theta[4];
    float det = 4*a*b - c*c;
    if (fabsf(det) < 1e-6f) return false;

    float f_opt = (2*b * (-d) - c * (-e)) / det;
    float v_opt = (2*a * (-e) - c * (-d)) / det;

    f_opt = fmaxf(500.0f, fminf(f_opt, 950.0f));
    v_opt = fmaxf(1100.0f, fminf(v_opt, 1350.0f));

    float phi_opt[G6_PARAMS] = {f_opt*f_opt, v_opt*v_opt, f_opt*v_opt, f_opt, v_opt, 1.0f};
    float pred = 0;
    for (int i = 0; i < G6_PARAMS; i++) pred += phi_opt[i] * brain->theta[i];

    *out_f = f_opt; *out_v = v_opt; *out_pred_hr = pred;
    return true;
}

void g6_brain_auto_step(G6BrainState *brain, float current_f, float current_v) {
    if (!brain->auto_tune_enabled) return;

    float new_f, new_v, pred_hr;
    if (!g6_brain_get_optimal(brain, &new_f, &new_v, &pred_hr)) return;

    float current_pred = 0;
    float phi_cur[G6_PARAMS] = {current_f*current_f, current_v*current_v, current_f*current_v, current_f, current_v, 1.0f};
    for (int i = 0; i < G6_PARAMS; i++) current_pred += phi_cur[i] * brain->theta[i];

    float improvement = (pred_hr - current_pred) / fmaxf(current_pred, 1.0f) * 100.0f;

    if (improvement > brain->min_improvement_pct) {
        // TODO: integrate with actual ASIC set_freq_voltage() or work_queue
    }
}

void g6_brain_reset_model(G6BrainState *brain) {
    g6_brain_init(brain);
}

// ==================== G6 Puzzle Extras ====================

void g6_puzzle_nonce_optimize(G6BrainState *brain, float recent_share_rate, float error_trend) {
    if (error_trend > 0.5f) {
        brain->nonce_opt_start = fmodf(brain->nonce_opt_start + 0x10000000, 0xFFFFFFFF);
    } else {
        brain->nonce_opt_start = 0.0f;
    }
}

float g6_puzzle_predict_duplicates(G6BrainState *brain, float nonce_range, float job_difficulty) {
    float base_prob = 0.01f * (nonce_range / 1e9f);
    float diff_factor = 1.0f / (1.0f + job_difficulty / 1e12f);
    brain->dup_predict_prob = base_prob * diff_factor;
    return brain->dup_predict_prob;
}

void g6_puzzle_extras_run(G6BrainState *brain) {
    g6_puzzle_nonce_optimize(brain, 1.6f, 0.3f);
    float dup_prob = g6_puzzle_predict_duplicates(brain, 1e8f, 1e13f);
    if (dup_prob > 0.05f) {
        // Trigger smarter job handling
    }
    brain->puzzle_runs++;
}

void g6_puzzle_on_device_solver_demo(G6BrainState *brain, float target, float *solution) {
    float x = *solution;
    for (int i = 0; i < 20; i++) {
        float grad = 2.0f * (x - target);
        x -= 0.1f * grad;
    }
    *solution = x;
}
