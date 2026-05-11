/*
 * g6_brain.h
 * Bitaxe G6 Brain — v1.0 Beta (Phase 1 COMPLETE — Beast RLS + BM1370 + Full Hardening)
 * Pure RLS. Clean. Light. Modular-ready. All audits addressed.
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

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

/* ====================== SAMPLE QUALITY & EFFICIENCY CONSTANTS ====================== */
#define SETTLE_SECONDS      8000
#define MIN_WINDOW_SECONDS  5000
#define MIN_SHARE_COUNT     20
#define MAX_TEMP_SLOPE      0.5f

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
    uint32_t valid_sample_count;
} G6BrainState;

/* ====================== PUBLIC INTERFACE ====================== */
esp_err_t g6_brain_init(G6BrainState *brain);

esp_err_t g6_brain_update(G6BrainState *brain,
                          float f_mhz, float v_mv, float hr_ths,
                          float power_w, float temp_c, float err_pct);

void g6_brain_get_optimal(const G6BrainState *brain,
                          float *opt_f, float *opt_v, float *pred_hr);

float g6_brain_get_model_quality(const G6BrainState *brain);

esp_err_t g6_brain_self_test(G6BrainState *brain);

/* NVS silicon fingerprint */
esp_err_t g6_brain_load_nvs_fingerprint(G6BrainState *brain);
esp_err_t g6_brain_save_nvs_fingerprint(const G6BrainState *brain);

#ifdef __cplusplus
}
#endif
