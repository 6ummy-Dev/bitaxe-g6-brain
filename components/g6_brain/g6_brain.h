/*
 * g6_brain.h
 * Bitaxe G6 Brain — v1.0.0-beta7
 */
#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>
#include "sdkconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Control Modes */
typedef enum {
    G6_MODE_OBSERVE_ONLY = 0,
    G6_MODE_RECOMMEND,
    G6_MODE_AUTO
} G6ControlMode;

/* Constants */
#define RLS_N 6

#if defined(CONFIG_G6_RLS_LAMBDA_MIN)
#define RLS_LAMBDA_MIN ((float)CONFIG_G6_RLS_LAMBDA_MIN / 1000.0f)
#else
#define RLS_LAMBDA_MIN 0.95f
#endif

#if defined(CONFIG_G6_RLS_RIDGE_EPSILON)
#define RLS_RIDGE_EPSILON ((float)CONFIG_G6_RLS_RIDGE_EPSILON * 1e-6f)
#else
#define RLS_RIDGE_EPSILON 1e-5f
#endif

#if defined(CONFIG_G6_RLS_TRACE_MAX)
#define RLS_TRACE_MAX (float)CONFIG_G6_RLS_TRACE_MAX
#else
#define RLS_TRACE_MAX 1e7f
#endif

#if defined(CONFIG_G6_RLS_VFF_SIGMA_SQ)
#define RLS_VFF_SIGMA_SQ ((float)CONFIG_G6_RLS_VFF_SIGMA_SQ / 10000.0f)
#else
#define RLS_VFF_SIGMA_SQ 0.008f
#endif

#define RLS_INNOVATION_THRESHOLD 1e-4f
#define RLS_SYMMETRY_TOLERANCE 1e-4f
#define RLS_P_CLAMP_MIN 1e-6f
#define RLS_P_CLAMP_MAX 1e6f

#define BM1370_F_CENTER 650.0f
#define BM1370_F_SCALE 250.0f
#define BM1370_V_CENTER 1220.0f
#define BM1370_V_SCALE 120.0f
#define BM1370_F_MIN 400.0f
#define BM1370_F_MAX 950.0f
#define BM1370_V_MIN 1050.0f
#define BM1370_V_MAX 1350.0f

/* Minimum predicted hashrate (TH/s) below which J/TH efficiency calculations
 * are skipped. Prevents division by near-zero and guards against operating in
 * regions where the power model has no physical meaning.
 *
 * IMPORTANT — UNITS: hr_ths is TH/s. A single BM1370 (Bitaxe Gamma) hashes
 * ~1.0-1.2 TH/s stock and ~1.5 TH/s overclocked, so this floor MUST be well
 * below the chip's real hashrate or the Dinkelbach solver never arms. The old
 * value (8.0) was ~6x the device's real hashrate and silently disabled
 * efficiency mode on the documented target. 0.5 TH/s is a physical "barely
 * hashing" floor. If you feed the brain GH/s (e.g. AxeOS `hashRate`), convert
 * to TH/s first (divide by 1000) — see docs/INTEGRATION_EXAMPLE.c. */
#define G6_EFFICIENCY_MIN_HR_THS 0.5f

#define MIN_SHARE_COUNT 20

/* Statistical outlier-gate noise-variance floors (the "+sigma^2" term added to
 * the innovation variance xPx before the 3-sigma test err^2 > 9*(xPx+floor)).
 * These are unit-coupled and MUST match the channel's physical scale:
 *   - hashrate err is in TH/s  -> floor is (TH/s)^2. ~0.1 TH/s measurement
 *     sigma => 0.01. A single 0.5f floor (the old shared value) corresponds to
 *     a 0.7 TH/s sigma, which at TH/s scale only catches absurd (>2 TH/s)
 *     deviations and lets gross glitches through.
 *   - power err is in W -> floor is W^2. ~0.7 W sigma => 0.5 is reasonable for
 *     a ~20 W board.
 * Both are best-estimate starting points; validate against real telemetry. */
#define G6_HR_OUTLIER_VAR_FLOOR_THS2 0.01f
#define G6_PW_OUTLIER_VAR_FLOOR_W2   0.5f

/* model_quality / power_model_quality use 1 - |err| / (signal + floor). The
 * floor keeps quality finite and > 0 on the first (cold) update where the
 * prediction is zero and err == signal. It is unit-coupled like the gates
 * above: the hashrate floor is in TH/s (kept small so quality tracks relative
 * error at the ~1.2 TH/s scale instead of being dominated by a fixed +1), the
 * power floor is in W. The old shared +1.0f was sized for a ~100x larger
 * hashrate scale and made the quality gate (>=0.6) trivially easy to satisfy
 * at TH/s scale. Validate against real telemetry. */
#define G6_QUALITY_DENOM_FLOOR_HR_THS 0.1f
#define G6_QUALITY_DENOM_FLOOR_PW_W   1.0f

/* Observability-only under-excitation warn level (beta7).
 *
 * The covariance condition number (Gershgorin estimate, see
 * g6_brain_get_cov_condition) is low for a fresh model (~1 at the 1e5·I cold
 * start) and for a well-excited, well-conditioned one, but climbs without
 * bound when the operating point does not vary: at a fixed (f, v) the six-term
 * quadratic basis is unidentifiable, so most parameter directions are never
 * excited and the matrix ill-conditions. Crucially, model_quality is NOT a
 * trustworthiness signal in that regime — it measures fit at the single
 * visited point and reads HIGH even though the surface (and therefore the
 * optimizer's recommended setpoint) is undetermined.
 *
 * When cov_condition exceeds this level AND the model is past cold start,
 * G6BrainTelemetry.model_under_excited is set so an operator/integrator knows
 * the recommendations are not yet trustworthy and the operating point needs to
 * vary (the beta8 on-device exploration feature will supply that variation
 * directly). This is telemetry only — it changes NO control behavior.
 *
 * Escalation ladder on cov_condition: > this warn (advisory, here) <
 * self_test fail (5e5, "degraded") < indefinite/negative-variance
 * (P_MATRIX_SINGULAR recovery). STARTING VALUE: the genuinely-converged
 * condition number is not known until a closed-loop AUTO soak; set
 * conservatively high to avoid false "untrustworthy" alarms and calibrate
 * against field data (tracked in docs/ROADMAP.md). */
#define G6_EXCITATION_COND_WARN 1.0e5f

#if defined(CONFIG_G6_JTH_MAX_OUTER_ITERS)
#define G6_JTH_MAX_OUTER_ITERS CONFIG_G6_JTH_MAX_OUTER_ITERS
#else
#define G6_JTH_MAX_OUTER_ITERS 7
#endif
#define G6_NVS_SCHEMA_VERSION 3u

#if defined(CONFIG_G6_TEMP_PROACTIVE_MARGIN)
#define G6_TEMP_PROACTIVE_MARGIN_DEFAULT ((float)CONFIG_G6_TEMP_PROACTIVE_MARGIN)
#else
#define G6_TEMP_PROACTIVE_MARGIN_DEFAULT 5.0f
#endif

#if defined(CONFIG_G6_VR_TEMP_CEILING)
#define G6_VR_TEMP_CEILING_DEFAULT ((float)CONFIG_G6_VR_TEMP_CEILING)
#else
#define G6_VR_TEMP_CEILING_DEFAULT 85.0f
#endif

#if defined(CONFIG_G6_VR_TEMP_PROACTIVE_MARGIN)
#define G6_VR_TEMP_PROACTIVE_MARGIN_DEFAULT ((float)CONFIG_G6_VR_TEMP_PROACTIVE_MARGIN)
#else
#define G6_VR_TEMP_PROACTIVE_MARGIN_DEFAULT 5.0f
#endif

#define G6_VR_TEMP_NO_SENSOR (-1.0f)

/* Safety Status */
typedef enum {
    G6_SAFETY_OK = 0,
    G6_SAFETY_THERMAL,
    G6_SAFETY_VR_THERMAL,
    G6_SAFETY_VOLTAGE,
    G6_SAFETY_POWER_SANITY,
    G6_SAFETY_NER_BACKOFF,
    G6_SAFETY_SAMPLE_QUALITY,
    G6_SAFETY_P_MATRIX_SINGULAR,
    /* Input telemetry failed validation (non-finite, hr_ths<=0, or out of
     * hardware bounds for f_mhz/v_mv). Routed fail-closed to the safety
     * layer per manifesto non-negotiable 3.7. Appended at end of enum to
     * preserve integer mappings for callers compiled against beta4. */
    G6_SAFETY_INPUT_RANGE
} G6SafetyStatus;

/* Main State Struct */
typedef struct {
    /* RLS Hashrate Model */
    float theta[RLS_N];
    float P[RLS_N][RLS_N];
    float ridge_epsilon;
    float model_quality;
    uint32_t update_count;

    /* Recommended Operating Point */
    float best_f;
    float best_v;

    /* Safety & Configuration */
    float ner_threshold;
    float temp_ceiling;
    float temp_proactive_margin;
    float vr_temp_ceiling;
    float vr_temp_proactive_margin;
    float dfs_step_mhz;
    G6ControlMode control_mode;
    G6SafetyStatus last_safety_status;

    /* Internal Timing / State */
    uint32_t last_update_timestamp;
    uint32_t nvs_last_write_tick;
    uint32_t last_setting_change_tick;

    /* Telemetry */
    float last_efficiency;
    float last_innovation;

    /* Power Model */
    float power_theta[RLS_N];
    float power_P[RLS_N][RLS_N];
    float power_model_quality;
    uint32_t power_update_count;

    /* Phase 2 Reserved */
    uint32_t nonce_offset;
    uint32_t valid_sample_count;
    float Kp, Ki, Kd;

    /* State Flags (Packed to eliminate padding) */
    bool cold_start;
    bool nvs_valid;
    bool use_efficiency_mode;
    bool enable_low_latency_jobs;
} G6BrainState;

/* Telemetry Snapshot */
typedef struct {
    /* Model internals */
    float theta_hashrate[RLS_N];
    float theta_power[RLS_N];
    float trace_P_hashrate;
    float trace_P_power;
    float last_innovation;
    /* Operating point & quality */
    float best_f;
    float best_v;
    float model_quality;
    float power_model_quality;
    float last_efficiency;
    uint32_t update_count;
    uint32_t power_update_count;
    uint32_t last_update_timestamp; /* FreeRTOS tick of the most recent accepted RLS update (paired with update_count) */
    /* Status */
    G6SafetyStatus safety_status;
    bool efficiency_mode_active;
    float last_recommended_voltage; /* kept for backward compat — mirrors best_v */
    /* Observability (beta7, appended to preserve prior field offsets) */
    float cov_condition;       /* Gershgorin condition-number estimate of the hashrate P (same value as g6_brain_get_cov_condition) */
    bool  model_under_excited; /* true when past cold start AND cov_condition > G6_EXCITATION_COND_WARN: recommendations not yet trustworthy (operating point under-varied). Telemetry only — no control effect. */
} G6BrainTelemetry;

/* Public API */
esp_err_t g6_brain_init(G6BrainState *brain);
esp_err_t g6_brain_update(G6BrainState *brain,
                          float f_mhz, float v_mv, float hr_ths,
                          float power_w, float temp_c, float vr_temp_c,
                          float err_pct, uint32_t share_count);
void g6_brain_get_optimal(const G6BrainState *brain,
                          float *opt_f, float *opt_v, float *pred_hr);
float g6_brain_get_model_quality(const G6BrainState *brain);
float g6_brain_get_cov_condition(const G6BrainState *brain);
esp_err_t g6_brain_self_test(const G6BrainState *brain);
esp_err_t g6_brain_reset(G6BrainState *brain);
esp_err_t g6_brain_load_nvs_fingerprint(G6BrainState *brain);
esp_err_t g6_brain_save_nvs_fingerprint(const G6BrainState *brain);
void g6_brain_get_telemetry(const G6BrainState *brain, G6BrainTelemetry *out);

#ifdef __cplusplus
}
#endif
