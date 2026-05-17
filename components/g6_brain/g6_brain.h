/*
 * g6_brain.h
 * Bitaxe G6 Brain — v1.0.0-beta2 (QA Hardened)
 *
 * Public interface.
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ====================== CONTROL MODES ====================== */
typedef enum {
    G6_MODE_OBSERVE_ONLY = 0,
    G6_MODE_RECOMMEND,
    G6_MODE_AUTO
} G6ControlMode;

/* ====================== RLS CONSTANTS ====================== */
#define RLS_N               6
#define RLS_LAMBDA_MIN      0.95f
#define RLS_LAMBDA_MAX      0.999f
#define RLS_TRACE_MAX       1e7f
#define RLS_P_CLAMP_MIN     1e-6f
#define RLS_P_CLAMP_MAX     1e6f

/* ====================== BM1370 TUNED CONSTANTS ====================== */
#define BM1370_F_CENTER     650.0f
#define BM1370_F_SCALE      250.0f
#define BM1370_V_CENTER     1220.0f
#define BM1370_V_SCALE      120.0f
#define BM1370_F_MIN        400.0f
#define BM1370_F_MAX        950.0f
#define BM1370_V_MIN        1050.0f
#define BM1370_V_MAX        1350.0f

/* ====================== SAMPLE QUALITY CONSTANTS ====================== */
#define SETTLE_MS           8000
#define MIN_WINDOW_MS       5000
#define MIN_SHARE_COUNT     20
#define MAX_TEMP_SLOPE      0.5f

/* ====================== EFFICIENCY & SAFETY CONSTANTS ====================== */
#define MIN_GAIN            0.5f
#define MAX_FREQ_STEP       50.0f
#define MAX_VOLT_STEP       25.0f

/* ====================== SAMPLE STATE MACHINE ====================== */
typedef enum {
    BRAIN_STATE_IDLE,
    BRAIN_STATE_APPLY_CANDIDATE,
    BRAIN_STATE_SETTLE_WAIT,
    BRAIN_STATE_MEASURE_WINDOW,
    BRAIN_STATE_VALIDATE_SAMPLE,
    BRAIN_STATE_RLS_UPDATE,
    BRAIN_STATE_DECIDE_NEXT
} BrainSampleState;

/* ====================== MAIN BRAIN STATE ====================== */
typedef struct {
    /* RLS core */
    float theta[RLS_N];
    float P[RLS_N][RLS_N];
    float ridge_epsilon;
    float model_quality;
    bool  cold_start;
    uint32_t update_count;
    bool  nvs_valid;

    /* Best safe setpoint */
    float best_f;
    float best_v;

    /* Safety & config */
    float ner_threshold;
    float Kp, Ki, Kd;
    float temp_ceiling;
    float dfs_step_mhz;

    /* Control mode */
    G6ControlMode control_mode;

    /* Nonce / extras */
    uint32_t nonce_offset;
    bool enable_low_latency_jobs;

    /* Timestamps & telemetry */
    uint32_t last_update_timestamp;
    uint32_t nvs_last_write_tick;
    uint32_t last_setting_change_tick;

    /* Sample quality state machine */
    BrainSampleState sample_state;
    uint32_t settle_start_tick;
    uint32_t measure_start_tick;
    uint32_t valid_sample_count;

    /* Telemetry */
    float last_efficiency;
} G6BrainState;

/* ====================== PUBLIC INTERFACE ====================== */

/**
 * Initialize the G6 Brain.
 * Must be called after nvs_flash_init().
 */
esp_err_t g6_brain_init(G6BrainState *brain);

/**
 * Feed telemetry and run one optimization + safety cycle.
 *
 * @param share_count  Actual number of valid shares in the current window.
 *                     Pass 0 if unknown (share quality check will be skipped).
 */
esp_err_t g6_brain_update(G6BrainState *brain,
                          float f_mhz,
                          float v_mv,
                          float hr_ths,
                          float power_w,
                          float temp_c,
                          float err_pct,
                          uint32_t share_count);   /* Added for proper share validation (BUG-2 fix) */

void g6_brain_get_optimal(const G6BrainState *brain,
                          float *opt_f,
                          float *opt_v,
                          float *pred_hr);

float g6_brain_get_model_quality(const G6BrainState *brain);

float g6_brain_get_cov_condition(const G6BrainState *brain);

esp_err_t g6_brain_self_test(G6BrainState *brain);

/* NVS */
esp_err_t g6_brain_load_nvs_fingerprint(G6BrainState *brain);
esp_err_t g6_brain_save_nvs_fingerprint(const G6BrainState *brain);

#ifdef __cplusplus
}
#endif
