/*
 * g6_brain.h
 * Bitaxe G6 Brain — v1.0.0-beta4 (VR Thermal Safety)
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

#define SETTLE_MS 8000
#define MIN_WINDOW_MS 5000
#define MIN_SHARE_COUNT 20
#define G6_JTH_MAX_OUTER_ITERS 7
#define G6_NVS_SCHEMA_VERSION 3u

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

/* State Machine */
typedef enum {
    BRAIN_STATE_IDLE,
    BRAIN_STATE_APPLY_CANDIDATE,
    BRAIN_STATE_SETTLE_WAIT,
    BRAIN_STATE_MEASURE_WINDOW,
    BRAIN_STATE_VALIDATE_SAMPLE,
    BRAIN_STATE_RLS_UPDATE,
    BRAIN_STATE_DECIDE_NEXT
} BrainSampleState;

/* Safety Status */
typedef enum {
    G6_SAFETY_OK = 0,
    G6_SAFETY_THERMAL,
    G6_SAFETY_VR_THERMAL,
    G6_SAFETY_VOLTAGE,
    G6_SAFETY_POWER_SANITY,
    G6_SAFETY_NER_BACKOFF,
    G6_SAFETY_SAMPLE_QUALITY,
    G6_SAFETY_P_MATRIX_SINGULAR
} G6SafetyStatus;

/* Main State Struct */
typedef struct {
    /* RLS Hashrate Model */
    float theta[RLS_N];
    float P[RLS_N][RLS_N];
    float ridge_epsilon;
    float model_quality;
    bool cold_start;
    uint32_t update_count;
    bool nvs_valid;

    /* Recommended Operating Point */
    float best_f;
    float best_v;

    /* Safety & Configuration */
    float ner_threshold;
    float temp_ceiling;
    float vr_temp_ceiling;
    float dfs_step_mhz;
    G6ControlMode control_mode;
    G6SafetyStatus last_safety_status;

    /* Internal Timing / State */
    uint32_t last_update_timestamp;
    uint32_t nvs_last_write_tick;
    uint32_t last_setting_change_tick;
    BrainSampleState sample_state;
    uint32_t settle_start_tick;
    uint32_t measure_start_tick;

    /* Telemetry */
    float last_efficiency;
    float last_innovation;

    /* Power Model */
    float power_theta[RLS_N];
    float power_P[RLS_N][RLS_N];
    float power_model_quality;
    bool power_cold_start;
    uint32_t power_update_count;
    bool use_efficiency_mode;

    /* Phase 2 Reserved */
    uint32_t nonce_offset;
    bool enable_low_latency_jobs;
    uint32_t valid_sample_count;
    float Kp, Ki, Kd;
} G6BrainState;

/* Telemetry Snapshot */
typedef struct {
    float theta_hashrate[RLS_N];
    float theta_power[RLS_N];
    float trace_P_hashrate;
    float trace_P_power;
    float last_innovation;
    G6SafetyStatus safety_status;
    bool efficiency_mode_active;
    float last_recommended_voltage;
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
esp_err_t g6_brain_self_test(G6BrainState *brain);
esp_err_t g6_brain_reset(G6BrainState *brain);
esp_err_t g6_brain_load_nvs_fingerprint(G6BrainState *brain);
esp_err_t g6_brain_save_nvs_fingerprint(const G6BrainState *brain);
void g6_brain_get_telemetry(const G6BrainState *brain, G6BrainTelemetry *out);

#ifdef __cplusplus
}
#endif
