/*
 * g6_brain.h
 * Bitaxe G6 Brain — v1.0.0-beta2 (Phase 0 QA Hardened + Phase 0.1 Critical Fixes)
 *
 * Public interface.
 *
 * PHASE 0.1 FIXES APPLIED:
 * - NVS schema versioning + size prefix (critical — prevents silent corruption on future struct changes)
 * - VFF sigma_sq now tunable via Kconfig (next step)
 * - New g6_brain_reset() API
 * - Magic constants centralized
 * - Strong single-threaded usage warning
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>
#include "sdkconfig.h"   // ← Phase 0: Full Kconfig support

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
#define G6_NVS_SCHEMA_VERSION 1u   // Increment on ANY breaking change to theta/P struct

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

// Phase 0.1: Tunable VFF sigma_sq (will be Kconfig-wired next)
#if defined(CONFIG_G6_RLS_VFF_SIGMA_SQ)
#define RLS_VFF_SIGMA_SQ    ((float)CONFIG_G6_RLS_VFF_SIGMA_SQ / 10000.0f)
#else
#define RLS_VFF_SIGMA_SQ    0.008f
#endif

#define RLS_INNOVATION_THRESHOLD 1e-4f     // Phase 0.1: centralized
#define RLS_SYMMETRY_TOLERANCE   1e-4f     // Phase 0.1: centralized
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
    /* RLS core */
    float theta[RLS_N];
    float P[RLS_N][RLS_N];
    float ridge_epsilon;        // from Kconfig via RLS_RIDGE_EPSILON
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
    float dfs_step_mhz;         // reserved for Phase 1 slew-rate limiting inside get_optimal()

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
} G6BrainState;

/* ====================== PUBLIC INTERFACE ====================== */

/**
 * Initialize the brain.
 * Must be called after nvs_flash_init().
 */
esp_err_t g6_brain_init(G6BrainState *brain);

/**
 * Feed telemetry and run one optimization + safety cycle.
 * SINGLE-THREADED ONLY.
 */
esp_err_t g6_brain_update(G6BrainState *brain,
                          float f_mhz,
                          float v_mv,
                          float hr_ths,
                          float power_w,
                          float temp_c,
                          float err_pct,
                          uint32_t share_count);

/**
 * Get the currently recommended safe operating point.
 */
void g6_brain_get_optimal(const G6BrainState *brain,
                          float *opt_f,
                          float *opt_v,
                          float *pred_hr);

float g6_brain_get_model_quality(const G6BrainState *brain);
float g6_brain_get_cov_condition(const G6BrainState *brain);
esp_err_t g6_brain_self_test(G6BrainState *brain);

/**
 * PHASE 0.1: Full reset + NVS erase (forces cold start).
 * Use this when you want to wipe learned model (e.g. ASIC swap, debugging).
 */
esp_err_t g6_brain_reset(G6BrainState *brain);

/* NVS (now versioned) */
esp_err_t g6_brain_load_nvs_fingerprint(G6BrainState *brain);
esp_err_t g6_brain_save_nvs_fingerprint(const G6BrainState *brain);

#ifdef __cplusplus
}
#endif
