#include "g6_brain.h"
#include <math.h>
#include <string.h>

// === G6 BRAIN v1.5.5 PURE MATH EXPERT CORE ===
// RLS Quadratic Response Surface Optimizer for Bitaxe Gamma 602+ (BM1370 ASIC)
// Models hashrate as quadratic surface HR(f, v) = θ · φ where φ = [f², v², f·v, f, v, 1]
// Uses Recursive Least Squares (RLS) with forgetting factor for real-time adaptation on ESP32.
// Analytically solves for the maximum of the fitted surface (closed-form critical point from partial derivatives).
// Safety-first: never updates model on unsafe telemetry (temp/error); clamps optima to proven safe ranges [500-950 MHz, 1100-1350 mV].
// Tracks best observed point and live model quality (inverse uncertainty).
// Zero bloat — removed all puzzle extras, nonce heuristics, on-device demos. This is the leanest, most mathematically rigorous version.
// Truth-seeking math only: evidence-based, no fluff.

static void mat_vec_mul(float mat[6][6], float vec[6], float out[6]) {
    for (int i = 0; i < 6; i++) {
        out[i] = 0.0f;
        for (int j = 0; j < 6; j++) {
            out[i] += mat[i][j] * vec[j];
        }
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
    brain->lambda = 0.97f;           // strong but responsive forgetting — balances stability and adaptation to silicon/temp drift
    brain->max_temp_c = 65.0f;
    brain->max_error_pct = 1.5f;
    brain->min_improvement_pct = 2.0f;
    brain->auto_tune_enabled = true;
    brain->model_quality = 0.05f;    // start low confidence, ramps up with data

    // Initial P: high uncertainty → fast initial learning (classic RLS initialization)
    for (int i = 0; i < G6_PARAMS; i++) {
        for (int j = 0; j < G6_PARAMS; j++) {
            brain->P[i][j] = (i == j) ? 1000.0f : 0.0f;
        }
    }

    // Reasonable prior around typical Gamma sweet spot (780MHz / 1.22V ~1.6TH/s class) — overwritten quickly by real telemetry
    brain->theta[0] = -0.0000012f;   // a (f² negative curvature for peak)
    brain->theta[1] = -0.00012f;     // b (v² negative curvature)
    brain->theta[2] = 0.000012f;     // c (positive interaction term)
    brain->theta[3] = 0.0021f;       // d (f linear rise)
    brain->theta[4] = 0.48f;         // e (v linear rise)
    brain->theta[5] = -80.0f;        // g (bias)
}

void g6_brain_update(G6BrainState *brain, float f, float v, float hr, float power, float temp, float err) {
    // Safety gate: never corrupt model on bad data (high temp or error rate often indicates bad shares, overheating, or stale jobs)
    if (temp > brain->max_temp_c || err > brain->max_error_pct || isnan(hr) || isinf(hr)) {
        return;
    }

    float phi[G6_PARAMS] = {f*f, v*v, f*v, f, v, 1.0f};
    float y = hr;

    // Prediction error
    float phiT_theta = 0.0f;
    for (int i = 0; i < G6_PARAMS; i++) phiT_theta += phi[i] * brain->theta[i];
    float e = y - phiT_theta;

    // Kalman gain (RLS core)
    float P_phi[G6_PARAMS];
    mat_vec_mul(brain->P, phi, P_phi);
    float denom = brain->lambda + 1e-8f;  // ridge regularization for numerical stability on ESP32 float
    for (int i = 0; i < G6_PARAMS; i++) denom += phi[i] * P_phi[i];
    float k[G6_PARAMS];
    for (int i = 0; i < G6_PARAMS; i++) k[i] = P_phi[i] / denom;

    // Update parameters and covariance (rank-1 downdate)
    for (int i = 0; i < G6_PARAMS; i++) brain->theta[i] += k[i] * e;
    rank1_update(brain->P, k, phi, brain->lambda);

    brain->update_count++;

    // Track best observed (for fallback analysis or hybrid use)
    float eff = hr / fmaxf(power, 0.1f);
    if (eff > brain->best_eff) {
        brain->best_f = f;
        brain->best_v = v;
        brain->best_hr = hr;
        brain->best_eff = eff;
    }

    // Update model confidence (inverse of average parameter uncertainty — higher = more data, better fit)
    float trace = brain->P[0][0] + brain->P[1][1] + brain->P[2][2];
    brain->model_quality = 1.0f / (1.0f + 0.001f * trace);
    if (brain->model_quality > 1.0f) brain->model_quality = 1.0f;
}

bool g6_brain_get_optimal(const G6BrainState *brain, float *out_f, float *out_v, float *out_pred_hr) {
    float a = brain->theta[0];
    float b = brain->theta[1];
    float c = brain->theta[2];
    float d = brain->theta[3];
    float e = brain->theta[4];

    float det = 4.0f * a * b - c * c;
    // Must be a true maximum (Hessian negative definite: a < 0 and det > 0)
    if (fabsf(det) < 1e-6f || a >= 0.0f || det <= 0.0f) return false;

    // Closed-form solution of ∇HR = 0 (solve 2x2 linear system from partial derivatives)
    float f_opt = (2.0f * b * (-d) - c * (-e)) / det;
    float v_opt = (2.0f * a * (-e) - c * (-d)) / det;

    // Clamp to safe, silicon-proven range for BM1370 on Gamma (avoids damage)
    f_opt = fmaxf(500.0f, fminf(f_opt, 950.0f));
    v_opt = fmaxf(1100.0f, fminf(v_opt, 1350.0f));

    // Predict HR at optimum (for gain calculation)
    float phi_opt[G6_PARAMS] = {f_opt*f_opt, v_opt*v_opt, f_opt*v_opt, f_opt, v_opt, 1.0f};
    float pred = 0.0f;
    for (int i = 0; i < G6_PARAMS; i++) pred += phi_opt[i] * brain->theta[i];

    *out_f = f_opt;
    *out_v = v_opt;
    *out_pred_hr = pred;
    return true;
}

void g6_brain_auto_step(G6BrainState *brain, float current_f, float current_v) {
    if (!brain->auto_tune_enabled) return;

    float new_f, new_v, pred_hr;
    if (!g6_brain_get_optimal(brain, &new_f, &new_v, &pred_hr)) return;

    // Compare predicted gain vs current operating point (using model, not noisy live HR)
    float phi_cur[G6_PARAMS] = {current_f*current_f, current_v*current_v, current_f*current_v, current_f, current_v, 1.0f};
    float current_pred = 0.0f;
    for (int i = 0; i < G6_PARAMS; i++) current_pred += phi_cur[i] * brain->theta[i];

    float improvement = (pred_hr - current_pred) / fmaxf(current_pred, 1.0f) * 100.0f;

    if (improvement > brain->min_improvement_pct && brain->model_quality > 0.3f) {
        // Ready to retune — integrate in main firmware: call asic_set_frequency(new_f), asic_set_core_voltage(new_v) or equivalent
        // Log it: ESP_LOGI(TAG, "G6 v%s: Optimal f=%.0f v=%.0f (gain %.1f%%) | quality=%.2f", G6_BRAIN_VERSION, new_f, new_v, improvement, brain->model_quality);
    }
}

void g6_brain_reset(G6BrainState *brain) {
    g6_brain_init(brain);
}