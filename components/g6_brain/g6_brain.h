/*
 * g6_brain.h
 * Bitaxe G6 Brain — v1.0.0-beta2 (Phase 0 QA Hardened + Phase 0.1 Critical Fixes + Phase 1 Prep)
 *
 * Public interface.
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>
#include "sdkconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ====================== CRITICAL NOTES ====================== */
/*
 * SINGLE-THREADED ASSUMPTION (AGGRESSIVE WARNING)
 * ================================================
 * This module is **NOT** thread-safe. There are no mutexes.
 * All calls to g6_brain_update(), g6_brain_get_optimal(), g6_brain_reset(), etc.
 * MUST be serialized by the caller (single task or proper locking).
 * NVS save/load + RLS state mutation happens in update().
 * Violating this will corrupt the P-matrix and/or NVS fingerprint.
 * See AGENTS.md and INTEGRATION_EXAMPLE.c for recommended usage.
 */

/* ====================== CONTROL MODES ====================== */
typedef enum {
    G6_MODE_OBSERVE_ONLY = 0,   // Safety only — no best_f/v mutation
    G6_MODE_RECOMMEND,          // Compute optimal but do not mutate best_f/v (safe default)
    G6_MODE_AUTO                // Full optimizer + safety (original behavior)
} G6ControlMode;

/* ====================== NVS SCHEMA (PHASE 0.1 CRITICAL) ====================== */
#define G6_NVS_SCHEMA_VERSION 1u   // Will become 2 when Phase 1 power fields are persisted

/* ====================== RLS CONSTANTS (fully Kconfig-wired + Phase 0.1) ====================== */
#define RLS_N               6

// All RLS options now symmetric with Kconfig (QA fix)
#if defined(CONFIG_G6_RLS_LAMBDA_MIN)
#define RLS_LAMBDA_MIN      ((float)CONFIG_G6_RLS_LAMBDA_MIN / 1000.0f)
#else
#define RLS_LAMBDA_MIN      0.95f
#endif

#if defined(CONFIG_G6_RLS_RIDGE_EPSILON)
#define RLS_RIDGE_EPSILON   ((float)CONFIG_G6_RLS_RIDGE_EPSILON * 1e-6f)
#else
#define RLS_RIDGE_EPSILON   1e-5f
#endif

#if defined(CONFIG_G6_RLS_TRACE_MAX)
#define RLS_TRACE_MAX       (float)CONFIG_G6_RLS_TRACE_MAX
#else
#define RLS_TRACE_MAX       1e7f
#endif

#if defined(CONFIG_G6_RLS_VFF_SIGMA_SQ)
#define RLS_VFF_SIGMA_SQ    ((float)CONFIG_G6_RLS_VFF_SIGMA_SQ / 10000.0f)
#else
#define RLS_VFF_SIGMA_SQ    0.008f
#endif

#define RLS_INNOVATION_THRESHOLD 1e-4f
#define RLS_SYMMETRY_TOLERANCE   1e-4f
#define RLS_P_CLAMP_MIN          1e-6f
#define RLS_P_CLAMP_MAX          1e6f

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
    /* RLS core — hashrate model */
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

    /* Safety & config (all Kconfig-wired) */
    float ner_threshold;
    float Kp, Ki, Kd;
    float temp_ceiling;
    float dfs_step_mhz;

    /* Control mode — fully enforced */
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

    /* ====================== PHASE 1 — POWER MODEL (additive only) ====================== */
    float power_theta[RLS_N];           // separate quadratic model for power (W)
    float power_P[RLS_N][RLS_N];        // separate covariance matrix for power
    float power_model_quality;          // 0.0-1.0 quality of power fit
    bool  power_cold_start;
    uint32_t power_update_count;

    /* Phase 1 runtime flag (default = false for full backward compatibility) */
    bool  use_efficiency_mode;          // when true → optimize J/TH instead of raw hashrate

} G6BrainState;

/* ====================== TELEMETRY STRUCT (Phase 2 — lightweight export) ====================== */
/*
 * Zero-overhead snapshot of internal state.
 * Call this anytime (single-threaded contract still holds).
 * Perfect for MONITORING.md, logs, or external dashboard.
 */
typedef struct {
    float theta_hashrate[RLS_N];      // current hashrate quadratic coeffs
    float theta_power[RLS_N];         // current power quadratic coeffs (Phase 1)
    float trace_P_hashrate;           // covariance trace (stability metric)
    float trace_P_power;              // power model trace
    float last_innovation;            // last RLS innovation (hashrate)
    uint8_t safety_status;            // 0=OK, 1=thermal, 2=voltage, 3=power, 4=NER (expandable)
    bool efficiency_mode_active;      // is J/TH mode on?
    float last_recommended_voltage;   // last output from g6_brain_get_optimal()
} G6BrainTelemetry;
/* ====================== SAFETY STATUS ENUM (Phase 2) ====================== */
typedef enum {
    G6_SAFETY_OK = 0,              // all clear
    G6_SAFETY_THERMAL,             // thermal ceiling hit or proactive derate
    G6_SAFETY_VOLTAGE,             // voltage ripple / undershoot / out-of-range
    G6_SAFETY_POWER_SANITY,        // power model sanity fail
    G6_SAFETY_NER_BACKOFF,         // high NER → conservative back-off
    G6_SAFETY_SAMPLE_QUALITY,      // bad sample FSM (noisy / unsettled)
    G6_SAFETY_P_MATRIX_SINGULAR    // RLS covariance exploded (fallback active)
} G6SafetyStatus;
/* ====================== PUBLIC INTERFACE ====================== */

esp_err_t g6_brain_init(G6BrainState *brain);
esp_err_t g6_brain_update(G6BrainState *brain,
                          float f_mhz, float v_mv, float hr_ths,
                          float power_w, float temp_c, float err_pct,
                          uint32_t share_count);

void g6_brain_get_optimal(const G6BrainState *brain,
                          float *opt_f, float *opt_v, float *pred_hr);

float g6_brain_get_model_quality(const G6BrainState *brain);
float g6_brain_get_cov_condition(const G6BrainState *brain);
esp_err_t g6_brain_self_test(G6BrainState *brain);
esp_err_t g6_brain_reset(G6BrainState *brain);

/* NVS (now versioned) */
esp_err_t g6_brain_load_nvs_fingerprint(G6BrainState *brain);
esp_err_t g6_brain_save_nvs_fingerprint(const G6BrainState *brain);

/* Phase 2 telemetry (new) */
void g6_brain_get_telemetry(const G6BrainState *brain, G6BrainTelemetry *out);

#ifdef __cplusplus
}
#endif
