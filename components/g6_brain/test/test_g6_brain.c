/*
 * Unity test suite for G6 Brain v1.0.0-beta7.5
 *
 * Validates tracking model updates, safety thresholds,
 * full G6SafetyStatus enum coverage (OK / THERMAL / VR_THERMAL /
 * POWER_SANITY / NER_BACKOFF / SAMPLE_QUALITY / P_MATRIX_SINGULAR /
 * INPUT_RANGE), outlier gating, input validation (fail-closed routing),
 * non-anomaly rejection paths (low shares, insignificant innovation —
 * status stays OK, update_count unchanged),
 * NVS round-trip and corruption recovery (bad-size and oversized blobs),
 * internal slew rate limits,
 * Dinkelbach J/TH efficiency optimization end-to-end, RLS quadratic
 * convergence on a known noiseless surface (estimator-learns regression
 * guard at the real BM1370 TH/s scale), covariance-divergence recovery on a
 * non-PSD (negative predicted-variance) covariance, and telemetry snapshot
 * (including the beta7 cov_condition / model_under_excited observability fields).
 *
 * Tests are NVS-isolated: setUp erases the fingerprint key before init, so
 * every case cold-starts clean-room — order-independent and re-run-stable.
 */

#include "unity.h"
#include "g6_brain.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <math.h>

static G6BrainState test_brain;

void setUp(void) {
    memset(&test_brain, 0, sizeof(G6BrainState));

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* Per-test NVS isolation: erase any fingerprint left by a previous test
     * (or by a previous, possibly partial, run of the suite on the same
     * flash). g6_brain_init() warm-starts from NVS, so without this erase
     * every test between the save/load round-trip and the bad-size-blob test
     * inherits the round-trip's blob — cold_start arrives false and "Cold
     * start flag clears" fails on its first assert. A full pass happens to
     * self-clean (the bad-size test's load-path erase removes the key), but
     * a partial/selective run that stops after the round-trip leaves the
     * blob, and the next session starts warm — the init and cold-start tests
     * then fail at session start. Invisible under compile-only CI. */
    nvs_handle_t iso;
    if (nvs_open("g6_brain", NVS_READWRITE, &iso) == ESP_OK) {
        nvs_erase_key(iso, "theta_fingerprint");
        nvs_commit(iso);
        nvs_close(iso);
    }

    ret = g6_brain_init(&test_brain);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
}

void tearDown(void) {
}

/* ====================== CORE ====================== */

TEST_CASE("g6_brain_init initializes correctly with Kconfig + control_mode", "[g6_brain]") {
    TEST_ASSERT_EQUAL(G6_MODE_RECOMMEND, test_brain.control_mode);
    TEST_ASSERT_EQUAL_FLOAT((float)CONFIG_G6_TEMP_CEILING, test_brain.temp_ceiling);
    TEST_ASSERT_EQUAL_FLOAT((float)CONFIG_G6_NER_THRESHOLD / 100.0f, test_brain.ner_threshold);
    TEST_ASSERT_TRUE(test_brain.cold_start);
    TEST_ASSERT_EQUAL_FLOAT(RLS_RIDGE_EPSILON, test_brain.ridge_epsilon);
}

TEST_CASE("g6_brain_update with valid synthetic data respects control_mode", "[g6_brain]") {
    float f = 650.0f, v = 1220.0f, hr = 1.2f, pwr = 15.0f, temp = 55.0f, err = 0.5f;
    uint32_t shares = 50;

    esp_err_t ret = g6_brain_update(&test_brain, f, v, hr, pwr, temp, G6_VR_TEMP_NO_SENSOR, err, shares);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    /* model_quality is a float in (0,1] here — use a float-true check, not the
       integer TEST_ASSERT_GREATER_THAN (which casts a sub-1.0 quality to 0). */
    TEST_ASSERT_TRUE(test_brain.model_quality > 0.0f);
    /* Clean telemetry, no safety conditions firing → status should be OK. */
    TEST_ASSERT_EQUAL(G6_SAFETY_OK, test_brain.last_safety_status);
    /* RECOMMEND (default fixture mode): the optimizer/slew path must not move
       best_f. (temp is below the proactive zone, so no safety derate either.) */
    TEST_ASSERT_EQUAL_FLOAT(650.0f, test_brain.best_f);

    /* Switch to AUTO and preset a concave-down response surface whose interior
       optimum is in-bounds and differs from best_f, so the slew clamp has a real
       target (mirrors "Internal Slew-Rate Limiting"). Two off-center samples
       alone leave the 6-coefficient quadratic underdetermined with an
       out-of-bounds optimum, so get_optimal falls back to best_f and the slew is
       a silent no-op — the historical B3/B5 slew-test failure mode. */
    test_brain.control_mode = G6_MODE_AUTO;
    test_brain.best_f = 650.0f;
    test_brain.best_v = 1220.0f;
    test_brain.theta[0] = -1.0f;   /* a < 0                               */
    test_brain.theta[1] = -0.5f;   /* b < 0  → Hessian negative-definite  */
    test_brain.theta[2] =  0.0f;   /* c                                   */
    test_brain.theta[3] =  1.2f;   /* d → fn* = 0.6 → f_cand = 800 MHz     */
    test_brain.theta[4] =  0.0f;   /* e → vn* = 0   → v_cand = 1220 mV     */

    float original_best_f = test_brain.best_f;

    /* AUTO update at the basis center: x = [0,0,0,0,0,1], so the single RLS step
       only nudges theta[5] and the preset surface shape survives. */
    ret = g6_brain_update(&test_brain, 650.0f, 1220.0f, 1.2f, 15.0f, 55.0f, G6_VR_TEMP_NO_SENSOR, 0.5f, 50);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    /* AUTO must slew best_f toward the in-bounds optimum, clamped to one step. */
    TEST_ASSERT_NOT_EQUAL(original_best_f, test_brain.best_f);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 675.0f, test_brain.best_f);  /* 650 + dfs_step (25) */
    TEST_ASSERT_EQUAL(G6_SAFETY_OK, test_brain.last_safety_status);
}

TEST_CASE("g6_brain_update in OBSERVE_ONLY does not mutate best_f/best_v", "[g6_brain]") {
    test_brain.control_mode = G6_MODE_OBSERVE_ONLY;

    float start_f = test_brain.best_f;
    float start_v = test_brain.best_v;

    esp_err_t ret = g6_brain_update(&test_brain, 800.0f, 1300.0f, 1.3f, 18.0f, 60.0f, G6_VR_TEMP_NO_SENSOR, 0.3f, 50);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL_FLOAT(start_f, test_brain.best_f);
    TEST_ASSERT_EQUAL_FLOAT(start_v, test_brain.best_v);
}

TEST_CASE("g6_brain_self_test detects good vs degraded state", "[g6_brain]") {
    /* Fresh fixture: P is diagonal at 1e5 — symmetric, in range, well-conditioned. */
    TEST_ASSERT_EQUAL(ESP_OK, g6_brain_self_test(&test_brain));

    /* Degrade: drive one diagonal far past RLS_P_CLAMP_MAX (1e6). This both
     * blows the diagonal-range check and pushes the Gershgorin condition
     * estimate (~1e12) past the 5e5 self-test ceiling. */
    test_brain.P[0][0] = 1e9f;
    test_brain.P[1][1] = 1e-3f;

    esp_err_t ret = g6_brain_self_test(&test_brain);
    TEST_ASSERT_EQUAL(ESP_FAIL, ret);
}

TEST_CASE("NVS fingerprint save/load round-trip", "[g6_brain]") {
    test_brain.theta[0] = 42.0f;
    test_brain.P[0][0] = 12345.0f;
    test_brain.power_theta[0] = 99.0f;
    test_brain.power_P[0][0] = 88888.0f;
    test_brain.update_count = 15;

    esp_err_t ret = g6_brain_save_nvs_fingerprint(&test_brain);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    G6BrainState loaded;
    memset(&loaded, 0, sizeof(G6BrainState));
    g6_brain_init(&loaded);
    ret = g6_brain_load_nvs_fingerprint(&loaded);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL_FLOAT(42.0f, loaded.theta[0]);
    TEST_ASSERT_EQUAL_FLOAT(12345.0f, loaded.P[0][0]);
    TEST_ASSERT_EQUAL_FLOAT(99.0f, loaded.power_theta[0]);
    TEST_ASSERT_EQUAL_FLOAT(88888.0f, loaded.power_P[0][0]);
}

TEST_CASE("g6_brain_update routes hr_ths=0 to safety layer (fail-closed)", "[g6_brain]") {
    /* hr_ths<=0 is bad telemetry, not a structurally broken call — fail
     * closed so the safety layer still runs (manifesto non-negotiable 3.7). */
    esp_err_t ret = g6_brain_update(&test_brain, 650.0f, 1220.0f, 0.0f, 15.0f, 55.0f, G6_VR_TEMP_NO_SENSOR, 0.5f, 30);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL(G6_SAFETY_INPUT_RANGE, test_brain.last_safety_status);
}

TEST_CASE("Safety layer still executes on invalid sample", "[g6_brain]") {
    test_brain.temp_ceiling = 60.0f;

    esp_err_t ret = g6_brain_update(&test_brain, 800.0f, 1300.0f, 1.2f, 20.0f, 75.0f, G6_VR_TEMP_NO_SENSOR, 1.0f, 40);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_LESS_THAN(800.0f, test_brain.best_f);
    /* temp_c=75 above ceiling=60 → hard thermal fires, status reflects that. */
    TEST_ASSERT_EQUAL(G6_SAFETY_THERMAL, test_brain.last_safety_status);
}

TEST_CASE("Covariance matrix stays symmetric after updates", "[g6_brain]") {
    for (int i = 0; i < 10; i++) {
        g6_brain_update(&test_brain, 650.0f + i*5, 1220.0f, 1.15f + i*0.01f, 16.0f, 52.0f, G6_VR_TEMP_NO_SENSOR, 0.6f, 40);
    }

    bool symmetric = true;
    for (int i = 0; i < RLS_N; i++) {
        for (int j = i + 1; j < RLS_N; j++) {
            if (fabsf(test_brain.P[i][j] - test_brain.P[j][i]) > 1e-4f) {
                symmetric = false;
            }
        }
    }
    TEST_ASSERT_TRUE(symmetric);
}

TEST_CASE("Cold start flag clears after sufficient updates", "[g6_brain]") {
    TEST_ASSERT_TRUE(test_brain.cold_start);

    for (int i = 0; i < 30; i++) {
        g6_brain_update(&test_brain, 650.0f, 1220.0f, 1.18f, 16.5f, 53.0f, G6_VR_TEMP_NO_SENSOR, 0.7f, 40);
    }

    TEST_ASSERT_FALSE(test_brain.cold_start);
}

TEST_CASE("Proactive thermal derating triggers correctly", "[g6_brain]") {
    test_brain.temp_ceiling = 65.0f;

    esp_err_t ret = g6_brain_update(&test_brain, 700.0f, 1250.0f, 1.1f, 18.0f, 62.0f, G6_VR_TEMP_NO_SENSOR, 0.8f, 50);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_LESS_OR_EQUAL(700.0f, test_brain.best_f);
}

TEST_CASE("Proactive thermal helper bails on corrupted temp_ceiling", "[g6_brain]") {
    /* B5-NIT-16 hardening: mirrors the VR helper's ceiling sanity guard.
     * If brain->temp_ceiling is ever non-finite or non-positive (a future
     * refactor corrupting the field), the proactive helper must not derate
     * the setpoint based on bogus arithmetic. The hard-thermal path will
     * still mark the tick as G6_SAFETY_THERMAL via is_thermal_safe(); the
     * assertion here is specifically that the proactive helper's body
     * does not run, so best_f / best_v are not scaled by 0.96 / 0.992. */

    /* Case 1: NaN ceiling. is_thermal_safe returns false (ceiling not finite),
     * so status becomes THERMAL via the upstream gate. Proactive helper guard
     * fires on the isfinite(temp_ceiling) check — body skipped. */
    float saved_f = test_brain.best_f;
    float saved_v = test_brain.best_v;
    test_brain.temp_ceiling = NAN;

    esp_err_t ret = g6_brain_update(&test_brain, 650.0f, 1220.0f, 1.15f, 16.0f, 55.0f, G6_VR_TEMP_NO_SENSOR, 0.5f, 50);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL_FLOAT(saved_f, test_brain.best_f);
    TEST_ASSERT_EQUAL_FLOAT(saved_v, test_brain.best_v);
    TEST_ASSERT_EQUAL(G6_SAFETY_THERMAL, test_brain.last_safety_status);

    /* Case 2: zero ceiling. is_thermal_safe sees temp_c=55 not < 0 → false →
     * status becomes THERMAL. Without the new guard, the proactive helper
     * would evaluate `55 > (0 - margin)` as true and scale the setpoint
     * down based on a bogus ceiling. With the guard, body is skipped. */
    test_brain.temp_ceiling = 0.0f;
    saved_f = test_brain.best_f;
    saved_v = test_brain.best_v;

    ret = g6_brain_update(&test_brain, 650.0f, 1220.0f, 1.15f, 16.0f, 55.0f, G6_VR_TEMP_NO_SENSOR, 0.5f, 50);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL_FLOAT(saved_f, test_brain.best_f);
    TEST_ASSERT_EQUAL_FLOAT(saved_v, test_brain.best_v);
    TEST_ASSERT_EQUAL(G6_SAFETY_THERMAL, test_brain.last_safety_status);
}

/* ====================== RLS & OUTLIER GATING ====================== */

TEST_CASE("Statistical Outlier Gating rejects severe sensor anomalies", "[g6_brain]") {
    g6_brain_update(&test_brain, 650.0f, 1220.0f, 1.2f, 15.0f, 55.0f, G6_VR_TEMP_NO_SENSOR, 0.5f, 50);
    uint32_t prev_count = test_brain.update_count;

    esp_err_t ret = g6_brain_update(&test_brain, 650.0f, 1220.0f, 99.99f, 15.0f, 55.0f, G6_VR_TEMP_NO_SENSOR, 0.5f, 50);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(prev_count, test_brain.update_count);
    /* err² greatly exceeds 9*(xPx + G6_HR_OUTLIER_VAR_FLOOR_THS2), triggering the 3-sigma gate. */
    TEST_ASSERT_EQUAL(G6_SAFETY_SAMPLE_QUALITY, test_brain.last_safety_status);
}

TEST_CASE("Internal Slew-Rate Limiting enforces step boundaries", "[g6_brain]") {
    test_brain.control_mode = G6_MODE_AUTO;
    test_brain.dfs_step_mhz = 25.0f;
    test_brain.best_f = 650.0f;
    test_brain.best_v = 1220.0f;
    /* Concave-down quadratic with unconstrained optimum at fn=0.6, vn=0
     * (i.e. f_cand=800 MHz, v_cand=1220 mV) — within hardware bounds and
     * 150 MHz above best_f, which forces the slew clamp to limit movement
     * to exactly dfs_step_mhz per tick. Earlier theta values produced an
     * out-of-bounds optimum, which fell back to best_f and made the test
     * silently a no-op. */
    test_brain.theta[0] = -1.0f;   /* a */
    test_brain.theta[1] = -0.5f;   /* b */
    test_brain.theta[2] = 0.0f;    /* c */
    test_brain.theta[3] = 1.2f;    /* d → f_norm_opt = -d/(2a) = 0.6 */
    test_brain.theta[4] = 0.0f;    /* e → v_norm_opt = 0 */

    esp_err_t ret = g6_brain_update(&test_brain, 650.0f, 1220.0f, 1.2f, 15.0f, 55.0f, G6_VR_TEMP_NO_SENSOR, 0.5f, 50);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 650.0f + test_brain.dfs_step_mhz, test_brain.best_f);
}

/* ====================== VR THERMAL SAFETY ====================== */

TEST_CASE("VR thermal sentinel (-1) is a no-op — does not affect setpoints", "[g6_brain]") {
    test_brain.control_mode = G6_MODE_AUTO;
    test_brain.vr_temp_ceiling = 85.0f;

    esp_err_t ret = g6_brain_update(&test_brain, 650.0f, 1220.0f, 1.2f, 15.0f,
                                    55.0f, G6_VR_TEMP_NO_SENSOR, 0.5f, 50);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_GREATER_OR_EQUAL(BM1370_V_MIN, test_brain.best_v);
    TEST_ASSERT_GREATER_OR_EQUAL(BM1370_V_CENTER - 10.0f, test_brain.best_v);
}

TEST_CASE("VR proactive zone steps back best_v only — frequency untouched", "[g6_brain]") {
    test_brain.control_mode = G6_MODE_AUTO;
    test_brain.vr_temp_ceiling = 85.0f;
    test_brain.best_f = 800.0f;
    test_brain.best_v = 1250.0f;

    esp_err_t ret = g6_brain_update(&test_brain, 800.0f, 1250.0f, 1.3f, 22.0f,
                                    55.0f, 82.0f, 0.5f, 50);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_LESS_THAN(1250.0f, test_brain.best_v);
    TEST_ASSERT_GREATER_OR_EQUAL(800.0f - test_brain.dfs_step_mhz, test_brain.best_f);
    /* vr_temp_c=82 in [80, 85) proactive zone → VR_THERMAL set, ASIC helper
     * runs after but temp_c=55 is below the proactive zone so it no-ops. */
    TEST_ASSERT_EQUAL(G6_SAFETY_VR_THERMAL, test_brain.last_safety_status);
}

TEST_CASE("VR hard ceiling steps back both best_v and best_f", "[g6_brain]") {
    test_brain.control_mode = G6_MODE_AUTO;
    test_brain.vr_temp_ceiling = 85.0f;
    test_brain.best_f = 850.0f;
    test_brain.best_v = 1280.0f;

    esp_err_t ret = g6_brain_update(&test_brain, 850.0f, 1280.0f, 1.4f, 26.0f,
                                    55.0f, 86.0f, 0.5f, 50);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_LESS_THAN(1280.0f, test_brain.best_v);
    TEST_ASSERT_LESS_THAN(850.0f, test_brain.best_f);
    /* vr_temp_c=86 ≥ ceiling=85 → hard VR thermal fires, status reflects it. */
    TEST_ASSERT_EQUAL(G6_SAFETY_VR_THERMAL, test_brain.last_safety_status);
}

/* ====================== RUNTIME PROACTIVE MARGINS ====================== */

TEST_CASE("vr_temp_proactive_margin field is initialized from Kconfig default", "[g6_brain]") {
    TEST_ASSERT_EQUAL_FLOAT(G6_VR_TEMP_PROACTIVE_MARGIN_DEFAULT, test_brain.vr_temp_proactive_margin);
    /* Float-true check: the generic GREATER_THAN macro compares as int. */
    TEST_ASSERT_TRUE(test_brain.vr_temp_proactive_margin > 0.0f);
}

TEST_CASE("temp_proactive_margin field is initialized from Kconfig default", "[g6_brain]") {
    TEST_ASSERT_EQUAL_FLOAT(G6_TEMP_PROACTIVE_MARGIN_DEFAULT, test_brain.temp_proactive_margin);
    /* Float-true check: the generic GREATER_THAN macro compares as int. */
    TEST_ASSERT_TRUE(test_brain.temp_proactive_margin > 0.0f);
}

TEST_CASE("Runtime vr_temp_proactive_margin change alters proactive zone", "[g6_brain]") {
    test_brain.vr_temp_ceiling = 85.0f;
    test_brain.vr_temp_proactive_margin = 10.0f;
    test_brain.best_v = 1250.0f;
    test_brain.best_f = 800.0f;

    /* 78°C is inside the widened zone (85-10=75) but outside the default zone (85-5=80) */
    esp_err_t ret = g6_brain_update(&test_brain, 800.0f, 1250.0f, 1.3f, 22.0f,
                                    55.0f, 78.0f, 0.5f, 50);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_LESS_THAN(1250.0f, test_brain.best_v);
    TEST_ASSERT_GREATER_OR_EQUAL(800.0f - test_brain.dfs_step_mhz, test_brain.best_f);
}

TEST_CASE("NER backoff fires on the upstream err_pct > ner_threshold check", "[g6_brain]") {
    /* err_pct above threshold trips the explicit upstream NER check in
       g6_brain_update (which calls g6_asic_error_handle_non_blocking and routes
       to the safety layer). The redundant NER branch inside is_sample_valid() is
       genuine defense-in-depth but is unreachable from the public API on this
       path, because the upstream check fires first. */
    uint32_t count_before = test_brain.update_count;

    esp_err_t ret = g6_brain_update(&test_brain, 650.0f, 1220.0f, 1.2f, 15.0f,
                                    55.0f, G6_VR_TEMP_NO_SENSOR,
                                    test_brain.ner_threshold + 1.0f, 50);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(count_before, test_brain.update_count);
    TEST_ASSERT_EQUAL(G6_SAFETY_NER_BACKOFF, test_brain.last_safety_status);
}

TEST_CASE("ASIC thermal status wins over VR thermal when both fire on same tick", "[g6_brain]") {
    test_brain.temp_ceiling = 65.0f;
    test_brain.temp_proactive_margin = 5.0f;
    test_brain.vr_temp_ceiling = 85.0f;
    test_brain.vr_temp_proactive_margin = 5.0f;

    /* temp_c=62 → ASIC proactive zone; vr_temp_c=82 → VR proactive zone */
    esp_err_t ret = g6_brain_update(&test_brain, 700.0f, 1250.0f, 1.1f, 18.0f,
                                    62.0f, 82.0f, 0.5f, 50);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL(G6_SAFETY_THERMAL, test_brain.last_safety_status);
}

/* ====================== INPUT VALIDATION ====================== */

TEST_CASE("g6_brain_update routes f_mhz above BM1370_F_MAX to safety layer (fail-closed)", "[g6_brain]") {
    esp_err_t ret = g6_brain_update(&test_brain, BM1370_F_MAX + 50.0f, 1220.0f,
                                    1.2f, 15.0f, 55.0f, G6_VR_TEMP_NO_SENSOR,
                                    0.5f, 50);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL(G6_SAFETY_INPUT_RANGE, test_brain.last_safety_status);
}

TEST_CASE("g6_brain_update routes v_mv above BM1370_V_MAX to safety layer (fail-closed)", "[g6_brain]") {
    esp_err_t ret = g6_brain_update(&test_brain, 650.0f, BM1370_V_MAX + 50.0f,
                                    1.2f, 15.0f, 55.0f, G6_VR_TEMP_NO_SENSOR,
                                    0.5f, 50);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL(G6_SAFETY_INPUT_RANGE, test_brain.last_safety_status);
}

TEST_CASE("g6_brain_update routes NaN vr_temp_c to safety layer (fail-closed)", "[g6_brain]") {
    /* Under manifesto non-negotiable 3.7, NaN telemetry must not skip a
     * safety tick. The brain reports INPUT_RANGE and runs the safety layer
     * (where downstream helpers no-op on NaN). */
    esp_err_t ret = g6_brain_update(&test_brain, 650.0f, 1220.0f, 1.2f, 15.0f,
                                    55.0f, NAN, 0.5f, 50);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL(G6_SAFETY_INPUT_RANGE, test_brain.last_safety_status);
}

TEST_CASE("g6_brain_update routes NaN temp_c to safety layer (fail-closed)", "[g6_brain]") {
    esp_err_t ret = g6_brain_update(&test_brain, 650.0f, 1220.0f, 1.2f, 15.0f,
                                    NAN, G6_VR_TEMP_NO_SENSOR, 0.5f, 50);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL(G6_SAFETY_INPUT_RANGE, test_brain.last_safety_status);
}

TEST_CASE("g6_brain_update routes out-of-range power_w to safety layer with POWER_SANITY", "[g6_brain]") {
    /* power_w outside the [0, 100] W physical sanity bounds is distinct from
     * the INPUT_RANGE class (which covers non-finite + frequency/voltage
     * bounds). It gets its own status because power outliers also share the
     * same enum value. */
    esp_err_t ret = g6_brain_update(&test_brain, 650.0f, 1220.0f, 1.2f, 500.0f,
                                    55.0f, G6_VR_TEMP_NO_SENSOR, 0.5f, 50);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL(G6_SAFETY_POWER_SANITY, test_brain.last_safety_status);
}

TEST_CASE("g6_brain_update routes negative power_w to safety layer with POWER_SANITY", "[g6_brain]") {
    esp_err_t ret = g6_brain_update(&test_brain, 650.0f, 1220.0f, 1.2f, -10.0f,
                                    55.0f, G6_VR_TEMP_NO_SENSOR, 0.5f, 50);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL(G6_SAFETY_POWER_SANITY, test_brain.last_safety_status);
}

TEST_CASE("g6_brain_update returns INVALID_ARG only for NULL brain pointer", "[g6_brain]") {
    /* NULL brain is the one truly structurally-broken call — no brain
     * exists to apply safety to, so INVALID_ARG is correct. */
    esp_err_t ret = g6_brain_update(NULL, 650.0f, 1220.0f, 1.2f, 15.0f,
                                    55.0f, G6_VR_TEMP_NO_SENSOR, 0.5f, 50);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

TEST_CASE("g6_brain_update accepts G6_VR_TEMP_NO_SENSOR sentinel", "[g6_brain]") {
    esp_err_t ret = g6_brain_update(&test_brain, 650.0f, 1220.0f, 1.2f, 15.0f,
                                    55.0f, G6_VR_TEMP_NO_SENSOR, 0.5f, 50);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
}

/* ====================== NON-ANOMALY REJECTION PATHS (B6 R2) ======================
 *
 * Two code paths inside g6_brain_update reject a sample WITHOUT setting a
 * safety status — by design, since they are not safety events:
 *
 *   1. share_count < MIN_SHARE_COUNT  (the share gate in is_sample_valid)
 *   2. xPx < RLS_INNOVATION_THRESHOLD (the has_significant_innovation gate)
 *
 * The contract on these paths is:
 *   - ESP_OK is returned
 *   - last_safety_status stays G6_SAFETY_OK
 *   - update_count is NOT incremented (no RLS update happened)
 *   - the safety layer still runs (clamps, slew logic) — manifesto 3.7
 *
 * Operators monitoring telemetry distinguish "accepted" from "rejected for
 * non-anomaly reasons" by watching update_count deltas, not by safety_status.
 * These tests pin that contract so it cannot drift silently.
 */

TEST_CASE("Low share count silently rejects sample with last_safety_status == OK", "[g6_brain]") {
    /* MIN_SHARE_COUNT = 20. A sample with share_count below the threshold
     * is a normal early-startup or post-pool-change case, not a safety
     * event. The brain must skip the RLS update without bumping
     * update_count or flagging a safety status. */
    uint32_t count_before = test_brain.update_count;

    esp_err_t ret = g6_brain_update(&test_brain, 650.0f, 1220.0f, 1.2f, 15.0f,
                                    55.0f, G6_VR_TEMP_NO_SENSOR, 0.5f,
                                    MIN_SHARE_COUNT - 1);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(count_before, test_brain.update_count);
    TEST_ASSERT_EQUAL(G6_SAFETY_OK, test_brain.last_safety_status);
}

TEST_CASE("Insignificant innovation silently rejects sample with last_safety_status == OK", "[g6_brain]") {
    /* Driving the gate: with f_mhz = BM1370_F_CENTER and v_mv = BM1370_V_CENTER,
     * fn=vn=0, so x = [0,0,0,0,0,1] and xPx = P[5][5]. Setting P[5][5] below
     * RLS_INNOVATION_THRESHOLD (1e-4) forces has_significant_innovation() to
     * return false, routing to safety_layer with status still OK. */
    test_brain.P[5][5] = 1e-7f;   /* below RLS_INNOVATION_THRESHOLD */
    uint32_t count_before = test_brain.update_count;

    esp_err_t ret = g6_brain_update(&test_brain, BM1370_F_CENTER, BM1370_V_CENTER,
                                    1.2f, 15.0f, 55.0f, G6_VR_TEMP_NO_SENSOR,
                                    0.5f, 50);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(count_before, test_brain.update_count);
    TEST_ASSERT_EQUAL(G6_SAFETY_OK, test_brain.last_safety_status);
}

/* ====================== NVS HARDENING ====================== */

TEST_CASE("NVS bad-size blob is erased on load (B5-NIT-2 actually fires)", "[g6_brain]") {
    /* Write a deliberately wrong-sized blob to the NVS key, then call load
     * and verify it gets erased (not silently retained across reboots). */
    nvs_handle_t nvs;
    esp_err_t ret = nvs_open("g6_brain", NVS_READWRITE, &nvs);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    uint8_t bad_blob[16] = {0xDE, 0xAD, 0xBE, 0xEF};  /* nowhere near expected_blob_size */
    TEST_ASSERT_EQUAL(ESP_OK, nvs_set_blob(nvs, "theta_fingerprint", bad_blob, sizeof(bad_blob)));
    TEST_ASSERT_EQUAL(ESP_OK, nvs_commit(nvs));
    nvs_close(nvs);

    /* Load should detect the size mismatch, log, and erase. */
    ret = g6_brain_load_nvs_fingerprint(&test_brain);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    /* Re-open and confirm the bad blob is actually gone. */
    ret = nvs_open("g6_brain", NVS_READONLY, &nvs);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    size_t blob_size = 0;
    esp_err_t get_err = nvs_get_blob(nvs, "theta_fingerprint", NULL, &blob_size);
    nvs_close(nvs);
    TEST_ASSERT_EQUAL(ESP_ERR_NVS_NOT_FOUND, get_err);
}

TEST_CASE("NVS oversized blob (exceeds read buffer) is erased on load", "[g6_brain]") {
    /* Regression for the ESP_ERR_NVS_INVALID_LENGTH path: a blob LARGER than
     * the load path's read buffer (1024 bytes) makes nvs_get_blob fail with
     * INVALID_LENGTH instead of returning ESP_OK, so the old code matched
     * neither mismatch branch and silently kept the stale blob forever —
     * every boot, no log, no erase. The load path now treats INVALID_LENGTH
     * as a size mismatch: WARN, erase, cold start. (If the internal buffer
     * ever grows past 2048, this blob degrades into an ordinary in-buffer
     * size mismatch — still erased, so the test stays valid either way.) */
    nvs_handle_t nvs;
    esp_err_t ret = nvs_open("g6_brain", NVS_READWRITE, &nvs);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    static uint8_t big_blob[2048] = {0xA5};
    TEST_ASSERT_EQUAL(ESP_OK, nvs_set_blob(nvs, "theta_fingerprint", big_blob, sizeof(big_blob)));
    TEST_ASSERT_EQUAL(ESP_OK, nvs_commit(nvs));
    nvs_close(nvs);

    ret = g6_brain_load_nvs_fingerprint(&test_brain);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_TRUE(test_brain.cold_start);

    ret = nvs_open("g6_brain", NVS_READONLY, &nvs);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    size_t blob_size = 0;
    esp_err_t get_err = nvs_get_blob(nvs, "theta_fingerprint", NULL, &blob_size);
    nvs_close(nvs);
    TEST_ASSERT_EQUAL(ESP_ERR_NVS_NOT_FOUND, get_err);
}

/* ====================== COVARIANCE RECOVERY ====================== */

TEST_CASE("Trace divergence triggers P-matrix recovery and reports P_MATRIX_SINGULAR", "[g6_brain]") {
    /* Drive P far past RLS_TRACE_MAX (1e7 default) to force the recovery path. */
    for (int i = 0; i < RLS_N; i++) test_brain.P[i][i] = 1.0e8f;
    test_brain.cold_start = false;

    esp_err_t ret = g6_brain_update(&test_brain, 650.0f, 1220.0f, 1.2f, 15.0f,
                                    55.0f, G6_VR_TEMP_NO_SENSOR, 0.5f, 50);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    /* Recovery zeroed theta and reset P diagonal to 1e5, then surfaced the
     * P_MATRIX_SINGULAR status. The status may be overwritten by a more
     * urgent safety condition (thermal/VR) — none fire in this test. */
    TEST_ASSERT_EQUAL(G6_SAFETY_P_MATRIX_SINGULAR, test_brain.last_safety_status);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, test_brain.theta[0]);
    TEST_ASSERT_EQUAL_FLOAT(1.0e5f, test_brain.P[0][0]);
    TEST_ASSERT_TRUE(test_brain.cold_start);
}

TEST_CASE("Negative predicted variance triggers P-matrix recovery (not silent freeze)", "[g6_brain]") {
    /* Regression for the fixed-operating-point covariance-divergence path.
     * At a single (f,v) the quadratic basis is unidentifiable and the power
     * covariance can drift indefinite (non-PSD) with a bounded trace — so the
     * trace check cannot catch it. The signature is a negative predicted
     * variance xᵀ·power_P·x, which is impossible for a valid covariance.
     *
     * Construct that state deterministically: at (F_CENTER, V_CENTER) the basis
     * is x = [0,0,0,0,0,1], so xᵀ·power_P·x = power_P[5][5]. A negative value
     * must trigger cold-start recovery (G6_SAFETY_P_MATRIX_SINGULAR), NOT be
     * silently swallowed by the innovation gate (which would freeze both
     * channels while still reporting OK). */
    test_brain.use_efficiency_mode = true;
    test_brain.power_P[RLS_N - 1][RLS_N - 1] = -1.0f;  /* indefinite in the constant-term direction */
    test_brain.cold_start = false;                     /* so recovery flipping it back is observable */

    esp_err_t ret = g6_brain_update(&test_brain, BM1370_F_CENTER, BM1370_V_CENTER,
                                    1.2f, 20.0f, 55.0f, G6_VR_TEMP_NO_SENSOR, 0.5f, 50);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL(G6_SAFETY_P_MATRIX_SINGULAR, test_brain.last_safety_status);
    TEST_ASSERT_TRUE(test_brain.cold_start);                       /* recovery re-armed cold start */
    TEST_ASSERT_EQUAL_FLOAT(1.0e5f, test_brain.power_P[RLS_N - 1][RLS_N - 1]); /* power_P reset to 1e5·I */
    TEST_ASSERT_EQUAL_FLOAT(1.0e5f, test_brain.P[0][0]);           /* HR P reset too */
    TEST_ASSERT_EQUAL_FLOAT(0.0f, test_brain.power_theta[0]);      /* theta zeroed by recovery */
}

TEST_CASE("P-matrix recovery preserves operator-configured use_efficiency_mode", "[g6_brain]") {
    /* Operator enabled efficiency mode at runtime. A subsequent covariance
     * divergence must not silently disable it. */
    test_brain.use_efficiency_mode = true;
    test_brain.control_mode = G6_MODE_AUTO;
    test_brain.ner_threshold = 3.0f;  /* non-default; must also survive */
    for (int i = 0; i < RLS_N; i++) test_brain.P[i][i] = 1.0e8f;

    esp_err_t ret = g6_brain_update(&test_brain, 650.0f, 1220.0f, 1.2f, 15.0f,
                                    55.0f, G6_VR_TEMP_NO_SENSOR, 0.5f, 50);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_TRUE(test_brain.use_efficiency_mode);
    TEST_ASSERT_EQUAL(G6_MODE_AUTO, test_brain.control_mode);
    TEST_ASSERT_EQUAL_FLOAT(3.0f, test_brain.ner_threshold);
}

/* ====================== DINKELBACH J/TH OPTIMIZER ====================== */

TEST_CASE("Dinkelbach optimizer improves J/TH over naive hashrate-only point", "[g6_brain]") {
    /* Synthetic surfaces designed so the hashrate-only optimum (high MHz) is
     * less efficient than a lower-MHz operating point. Hashrate is in TH/s at
     * the real BM1370 scale (~1.2 TH/s), power in W:
     *
     *   HR  surface: theta = [-0.03, -0.015, 0, 0.03, 0, 1.2]
     *     Peak at fn=0.5 (775 MHz), ~1.2075 TH/s. Concave-down in f and v.
     *
     *   PWR surface: power_theta = [0.5, 0.2, 0, 3, 0, 20]
     *     Convex (opens up): more frequency = more watts.
     *     At fn=0 (650 MHz): ~20W.  At fn=0.5 (775 MHz): ~21.6W.
     *
     * Starting J/TH at 775 MHz: 21.625/1.2075 = 17.91 W/TH.
     * Dinkelbach should find a lower-MHz point with better W/TH (min ~15.35
     * W/TH at the 400 MHz frequency floor). */
    test_brain.use_efficiency_mode = true;
    /* Bypass the quality gate — we're testing the optimizer math, not
     * the convergence of the RLS models. */
    test_brain.model_quality = 0.8f;
    test_brain.power_model_quality = 0.8f;

    test_brain.theta[0] = -0.03f;
    test_brain.theta[1] = -0.015f;
    test_brain.theta[2] =  0.0f;
    test_brain.theta[3] =  0.03f;
    test_brain.theta[4] =  0.0f;
    test_brain.theta[5] =  1.2f;

    test_brain.power_theta[0] =  0.5f;
    test_brain.power_theta[1] =  0.2f;
    test_brain.power_theta[2] =  0.0f;
    test_brain.power_theta[3] =  3.0f;
    test_brain.power_theta[4] =  0.0f;
    test_brain.power_theta[5] = 20.0f;

    /* Set starting point to the hashrate-only optimum (775 MHz) */
    test_brain.best_f = 775.0f;
    test_brain.best_v = 1220.0f;

    float opt_f, opt_v, pred_hr;
    g6_brain_get_optimal(&test_brain, &opt_f, &opt_v, &pred_hr);

    /* Dinkelbach must have moved to a lower-MHz, more efficient point. */
    TEST_ASSERT_LESS_THAN(775.0f, opt_f);
    TEST_ASSERT_GREATER_OR_EQUAL(BM1370_F_MIN, opt_f);

    /* Compute J/TH at starting point and at the optimizer's result. */
    float fn_start = (775.0f - BM1370_F_CENTER) / BM1370_F_SCALE;
    float fn_opt   = (opt_f  - BM1370_F_CENTER) / BM1370_F_SCALE;
    /* v is held at BM1370_V_CENTER (1220 mV) throughout this fixture, so
     * vn = 0 and every vn term of the quadratic vanishes; the manual J/TH
     * expressions below therefore use the fn terms only. */

    float hr_start = test_brain.theta[0]*fn_start*fn_start + test_brain.theta[3]*fn_start + test_brain.theta[5];
    float pw_start = test_brain.power_theta[0]*fn_start*fn_start + test_brain.power_theta[3]*fn_start + test_brain.power_theta[5];
    float jth_start = pw_start / hr_start;

    float hr_opt = test_brain.theta[0]*fn_opt*fn_opt + test_brain.theta[3]*fn_opt + test_brain.theta[5];
    float pw_opt = test_brain.power_theta[0]*fn_opt*fn_opt + test_brain.power_theta[3]*fn_opt + test_brain.power_theta[5];
    float jth_opt = pw_opt / hr_opt;

    /* The optimizer's point must be meaningfully more efficient (>1% better).
     * Float-true check: the integer-comparing generic macro only passed here
     * because 15.35 vs 17.73 happen to still differ after truncation. */
    TEST_ASSERT_TRUE(jth_opt < jth_start * 0.99f);
    /* And the predicted HR must be above the minimum viable threshold.
     * Float-true check: with the 0.5f floor truncating to 0, the old integer
     * compare accepted ANY pred_hr > -1 — it asserted nothing. */
    TEST_ASSERT_TRUE(pred_hr >= G6_EFFICIENCY_MIN_HR_THS);
}

TEST_CASE("Dinkelbach does not fire below model quality threshold", "[g6_brain]") {
    /* With quality below 0.6 on either model, get_optimal must return the
     * hashrate-only optimum unchanged — Dinkelbach must be a no-op. */
    test_brain.use_efficiency_mode = true;
    test_brain.model_quality = 0.8f;
    test_brain.power_model_quality = 0.4f;  /* below gate */

    test_brain.theta[0] = -0.03f; test_brain.theta[1] = -0.015f;
    test_brain.theta[3] =  0.03f; test_brain.theta[5] =  1.2f;
    test_brain.power_theta[0] = 0.5f; test_brain.power_theta[3] = 3.0f;
    test_brain.power_theta[5] = 20.0f;

    test_brain.best_f = 775.0f;
    test_brain.best_v = 1220.0f;

    float opt_f, opt_v, pred_hr;
    g6_brain_get_optimal(&test_brain, &opt_f, &opt_v, &pred_hr);

    /* Hashrate-only optimum for these theta values: fn_opt = -d/(2a) = -0.03/(2*-0.03) = 0.5
     * → f_cand = 0.5*250 + 650 = 775 MHz — within bounds, so optimizer stays there. */
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 775.0f, opt_f);
    /* Predicted HR at the hashrate-only optimum: theta[0]*0.5² + theta[3]*0.5 + theta[5]
     *   = -0.03*0.25 + 0.03*0.5 + 1.2 = 1.2075 TH/s */
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.2075f, pred_hr);
}

TEST_CASE("RLS converges to a known quadratic surface (noiseless)", "[g6_brain]") {
    /* End-to-end estimator regression guard: feed many noiseless samples drawn
     * from a known quadratic hashrate surface and confirm the RLS coefficients
     * converge to the true coefficients. The surface is expressed at the real
     * BM1370 TH/s scale (~1.0-1.2 TH/s), so this also exercises the corrected
     * outlier-gate and model-quality floors at the scale the brain sees in the
     * field — a mis-scaled gate (e.g. the old TH/s-vs-100x confusion) would
     * reject these in-distribution samples and stall convergence here.
     *
     * Scope note: this verifies the point-estimate (theta) recursion, NOT the
     * covariance magnitude. On consistent noiseless data both a correct RLS
     * covariance update and one missing the measurement-noise (+k kᵀ) injection
     * term drive theta to the same fixed point; the covariance term's
     * correctness is validated separately (see host-side numerical checks and
     * CHANGELOG). A deterministic tracking-based covariance regression test is
     * tracked as future work. */
    float th_true[RLS_N] = { -0.05f, -0.02f, 0.01f, 0.06f, 0.0f, 1.15f };

    /* Sweep a grid spanning the valid operating envelope enough times to both
     * exit cold-start (update_count > 25) and excite all six basis terms. */
    for (int sweep = 0; sweep < 12; sweep++) {
        for (int fi = 0; fi < 10; fi++) {
            for (int vi = 0; vi < 7; vi++) {
                float f = 450.0f + fi * 50.0f;     /* 450..900 MHz */
                float v = 1100.0f + vi * 40.0f;    /* 1100..1340 mV */
                float fn = (f - BM1370_F_CENTER) / BM1370_F_SCALE;
                float vn = (v - BM1370_V_CENTER) / BM1370_V_SCALE;
                float hr = th_true[0]*fn*fn + th_true[1]*vn*vn + th_true[2]*fn*vn
                         + th_true[3]*fn + th_true[4]*vn + th_true[5];
                /* temp/NER/power all comfortably in-range so only the RLS path runs. */
                g6_brain_update(&test_brain, f, v, hr, 18.0f, 50.0f,
                                G6_VR_TEMP_NO_SENSOR, 0.5f, 40);
            }
        }
    }

    /* Correct implementation converges to ~1e-7; 0.02 absolute is a generous,
     * non-flaky bound that still flags gross estimator or scaling breakage. */
    for (int i = 0; i < RLS_N; i++) {
        TEST_ASSERT_FLOAT_WITHIN(0.02f, th_true[i], test_brain.theta[i]);
    }
    /* Model quality should be high once converged on clean data. Float-true
     * check: the generic TEST_ASSERT_GREATER_THAN compares as int, so 0.6f
     * and a converged ~0.99 quality both truncate to 0 and `0 > 0` fails at
     * runtime even on a perfect model — the same integer-macro trap the
     * beta7.1 control-mode fix removed, surviving here under compile-only CI. */
    TEST_ASSERT_TRUE(test_brain.model_quality > 0.6f);
}

/* ====================== TELEMETRY SNAPSHOT ====================== */

TEST_CASE("g6_brain_get_telemetry snapshot captures all operator fields", "[g6_brain]") {
    /* Run a few updates to get non-trivial state, then verify the telemetry
     * snapshot is consistent with the struct at the moment of capture. */
    test_brain.control_mode = G6_MODE_RECOMMEND;
    for (int i = 0; i < 5; i++) {
        g6_brain_update(&test_brain, 650.0f, 1220.0f, 1.2f, 15.0f,
                        55.0f, G6_VR_TEMP_NO_SENSOR, 0.5f, 50);
    }

    G6BrainTelemetry t;
    g6_brain_get_telemetry(&test_brain, &t);

    TEST_ASSERT_EQUAL_FLOAT(test_brain.best_f,        t.best_f);
    TEST_ASSERT_EQUAL_FLOAT(test_brain.best_v,        t.best_v);
    TEST_ASSERT_EQUAL_FLOAT(test_brain.model_quality, t.model_quality);
    TEST_ASSERT_EQUAL_FLOAT(test_brain.last_efficiency, t.last_efficiency);
    TEST_ASSERT_EQUAL_UINT32(test_brain.update_count,  t.update_count);
    TEST_ASSERT_EQUAL_UINT32(test_brain.last_update_timestamp, t.last_update_timestamp);
    TEST_ASSERT_EQUAL(test_brain.last_safety_status,   t.safety_status);
    /* Backward-compat alias must mirror best_v. */
    TEST_ASSERT_EQUAL_FLOAT(t.best_v, t.last_recommended_voltage);
    TEST_ASSERT_GREATER_THAN(0u, t.update_count);
}

TEST_CASE("Telemetry exposes cov_condition and under-excitation flag (observability)", "[g6_brain]") {
    /* beta7 observability fields. cov_condition must mirror the standalone
     * accessor exactly, and model_under_excited must follow the documented
     * rule: (!cold_start) && cov_condition > G6_EXCITATION_COND_WARN. These
     * are telemetry-only and must not perturb control state. */

    /* Fresh fixture: cold_start true, P = 1e5·I so the condition number is ~1
     * (well-conditioned). Flag must be false (a fresh model is uninformed, not
     * under-excited — that early phase is covered by model_quality). */
    G6BrainTelemetry t0;
    g6_brain_get_telemetry(&test_brain, &t0);
    TEST_ASSERT_EQUAL_FLOAT(g6_brain_get_cov_condition(&test_brain), t0.cov_condition);
    TEST_ASSERT_FLOAT_WITHIN(1e-3f, 1.0f, t0.cov_condition);
    TEST_ASSERT_FALSE(t0.model_under_excited);

    /* Force an ill-conditioned covariance past the warn threshold and clear
     * cold start. cov_condition_estimate returns its large sentinel when the
     * Gershgorin lower bound is non-positive, which exceeds the warn level. */
    test_brain.cold_start = false;
    test_brain.P[0][0] = 1.0e3f;   /* small diagonal vs large off-diagonal -> lower bound <= 0 -> sentinel */
    test_brain.P[0][1] = 1.0e5f;
    test_brain.P[1][0] = 1.0e5f;

    G6BrainTelemetry t1;
    g6_brain_get_telemetry(&test_brain, &t1);
    TEST_ASSERT_EQUAL_FLOAT(g6_brain_get_cov_condition(&test_brain), t1.cov_condition);
    TEST_ASSERT_TRUE(t1.cov_condition > G6_EXCITATION_COND_WARN);
    TEST_ASSERT_TRUE(t1.model_under_excited);

    /* Same ill-conditioning but still in cold start -> flag suppressed. */
    test_brain.cold_start = true;
    G6BrainTelemetry t2;
    g6_brain_get_telemetry(&test_brain, &t2);
    TEST_ASSERT_FALSE(t2.model_under_excited);
}

TEST_CASE("last_update_timestamp advances iff update_count advances", "[g6_brain]") {
    /* Contract: the timestamp is paired with update_count. Both move on an
     * accepted RLS update; neither moves on a rejected sample. This lets
     * operators answer "is the brain learning, and when did it last learn"
     * from a single snapshot, without sampling at a rate fast enough to
     * catch the update tick directly. */

    /* Prime with one accepted update so timestamp is non-zero. */
    test_brain.control_mode = G6_MODE_RECOMMEND;
    esp_err_t ret = g6_brain_update(&test_brain, 650.0f, 1220.0f, 1.2f, 15.0f,
                                    55.0f, G6_VR_TEMP_NO_SENSOR, 0.5f, 50);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_GREATER_THAN(0u, test_brain.update_count);
    uint32_t ts_after_accept = test_brain.last_update_timestamp;
    uint32_t uc_after_accept = test_brain.update_count;

    /* Wait one tick so any subsequent write would differ. */
    vTaskDelay(1);

    /* Rejected sample: low share count. Neither update_count nor timestamp
     * should advance. Per the non-anomaly rejection contract, status stays OK. */
    ret = g6_brain_update(&test_brain, 650.0f, 1220.0f, 1.2f, 15.0f,
                          55.0f, G6_VR_TEMP_NO_SENSOR, 0.5f, /*shares=*/5);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(uc_after_accept, test_brain.update_count);
    TEST_ASSERT_EQUAL_UINT32(ts_after_accept, test_brain.last_update_timestamp);
    TEST_ASSERT_EQUAL(G6_SAFETY_OK, test_brain.last_safety_status);

    /* Rejected sample: out-of-bounds frequency (fail-closed). Same expectation. */
    ret = g6_brain_update(&test_brain, 1500.0f, 1220.0f, 1.2f, 15.0f,
                          55.0f, G6_VR_TEMP_NO_SENSOR, 0.5f, 50);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL(G6_SAFETY_INPUT_RANGE, test_brain.last_safety_status);
    TEST_ASSERT_EQUAL_UINT32(uc_after_accept, test_brain.update_count);
    TEST_ASSERT_EQUAL_UINT32(ts_after_accept, test_brain.last_update_timestamp);

    /* Accepted sample again. Both must advance together. Status resets to OK
     * from the prior INPUT_RANGE since an accepted sample clears it. */
    vTaskDelay(1);
    ret = g6_brain_update(&test_brain, 650.0f, 1220.0f, 1.21f, 15.0f,
                          55.0f, G6_VR_TEMP_NO_SENSOR, 0.5f, 50);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_GREATER_THAN(uc_after_accept, test_brain.update_count);
    TEST_ASSERT_GREATER_THAN(ts_after_accept, test_brain.last_update_timestamp);
    TEST_ASSERT_EQUAL(G6_SAFETY_OK, test_brain.last_safety_status);
}

/* ====================== CONDITIONING CERTIFICATES & TELEMETRY INTEGRITY ======================
 * Added with the beta7.5 FQA-3/FQA-5 fixes. The common thread: every fixture
 * here is a *reachable* operating state (or a corruption of one), where the
 * pre-fix suite only ever queried cov_condition / self_test on fresh or
 * hand-built diagonal matrices — the coverage hole that let a 100%
 * false-positive conditioning sentinel ship through 42 green tests. */

TEST_CASE("self_test and cov_condition stay healthy on a converged, well-excited model", "[g6_brain]") {
    /* FQA-3 regression guard. A fully converged, richly excited model is the
     * healthiest state the estimator can reach; the diagnostics must say so.
     * Pre-fix, the first accepted update pushed the minimum Gershgorin disc
     * non-positive and the estimate returned its ill-conditioned sentinel
     * forever after: this exact fixture read cov_condition = 1e12 with a
     * measured true condition number of 7.3, self_test returned ESP_FAIL, and
     * model_under_excited read true — all three asserts below failed. */
    float th_true[RLS_N] = { -0.05f, -0.02f, 0.01f, 0.06f, 0.0f, 1.15f };
    for (int sweep = 0; sweep < 12; sweep++) {
        for (int fi = 0; fi < 10; fi++) {
            for (int vi = 0; vi < 7; vi++) {
                float f = 450.0f + fi * 50.0f;
                float v = 1100.0f + vi * 40.0f;
                float fn = (f - BM1370_F_CENTER) / BM1370_F_SCALE;
                float vn = (v - BM1370_V_CENTER) / BM1370_V_SCALE;
                float hr = th_true[0]*fn*fn + th_true[1]*vn*vn + th_true[2]*fn*vn
                         + th_true[3]*fn + th_true[4]*vn + th_true[5];
                g6_brain_update(&test_brain, f, v, hr, 18.0f, 50.0f,
                                G6_VR_TEMP_NO_SENSOR, 0.5f, 40);
            }
        }
    }

    TEST_ASSERT_FALSE(test_brain.cold_start);
    TEST_ASSERT_EQUAL(ESP_OK, g6_brain_self_test(&test_brain));

    G6BrainTelemetry t;
    g6_brain_get_telemetry(&test_brain, &t);
    /* Float-true checks: certified lower bound must be sane (>= 1) and far
     * below the warn threshold on a healthy converged covariance. */
    TEST_ASSERT_TRUE(t.cov_condition >= 1.0f);
    TEST_ASSERT_TRUE(t.cov_condition < G6_EXCITATION_COND_WARN);
    TEST_ASSERT_FALSE(t.model_under_excited);
}

TEST_CASE("Under-excitation flag fires on certified degeneracy before fixed-point recovery", "[g6_brain]") {
    /* Fixed-point contract: when an unvaried operating point drives the
     * covariance numerically indefinite, the telemetry flag and self_test
     * must report it BEFORE the update path's own xPx < 0 guard self-heals
     * (Cholesky certifies loss of definiteness strictly earlier than the
     * single-direction xPx test can). Tick indices are intentionally not
     * asserted — float32 rounding on other FPUs may shift the exact onset —
     * only the ordering contract: certified-degenerate visibility, then
     * recovery. On the host the flag fires ~5 samples before recovery. */
    bool flag_seen = false;
    bool recovered = false;
    uint32_t prev_uc = 0;

    for (int i = 0; i < 200 && !recovered; i++) {
        g6_brain_update(&test_brain, 815.0f, 1210.0f, 1.21f, 19.0f, 55.0f,
                        G6_VR_TEMP_NO_SENSOR, 0.5f, 40);
        G6BrainTelemetry t;
        g6_brain_get_telemetry(&test_brain, &t);

        if (t.update_count < prev_uc) {
            recovered = true;   /* cold-start recovery wiped the counter */
            break;
        }
        prev_uc = t.update_count;

        if (t.model_under_excited) {
            flag_seen = true;
            /* Flag and diagnostics must agree while degenerate. */
            TEST_ASSERT_TRUE(t.cov_condition > G6_EXCITATION_COND_WARN);
            TEST_ASSERT_EQUAL(ESP_FAIL, g6_brain_self_test(&test_brain));
        }
    }

    TEST_ASSERT_TRUE(flag_seen);
    TEST_ASSERT_TRUE(recovered);
}

TEST_CASE("self_test detects an indefinite covariance that passes diagonal and symmetry checks", "[g6_brain]") {
    /* The definiteness certificate must catch what the older checks cannot:
     * a symmetric matrix with every diagonal inside the clamp range that is
     * nonetheless indefinite via off-diagonal correlation. The 2x2 block
     * [[1,2],[2,1]] has eigenvalues {3, -1}. Pre-fix this also failed
     * self_test, but only via the sentinel that fired on healthy matrices
     * too; post-fix the sentinel is reserved for exactly this case. */
    test_brain.P[0][0] = 1.0f;
    test_brain.P[1][1] = 1.0f;
    test_brain.P[0][1] = 2.0f;
    test_brain.P[1][0] = 2.0f;

    TEST_ASSERT_EQUAL(ESP_FAIL, g6_brain_self_test(&test_brain));

    G6BrainTelemetry t;
    g6_brain_get_telemetry(&test_brain, &t);
    TEST_ASSERT_EQUAL_FLOAT(RLS_P_CLAMP_MAX / RLS_P_CLAMP_MIN, t.cov_condition);
}

TEST_CASE("last_efficiency retains last known-good value when the sample is a rejected outlier", "[g6_brain]") {
    /* FQA-5 regression guard. A finite, in-bounds hashrate that the brain's
     * own 3-sigma gate rejects must not reach the efficiency telemetry.
     * Pre-fix, the tail gate checked only hr > 0 and power sanity, so a
     * 500 TH/s spike against a ~1.2 TH/s model wrote power/hr = 0.03 W/TH
     * into last_efficiency — a garbage ratio from a sample the brain itself
     * had just refused to learn from. */
    for (int i = 0; i < 20; i++) {
        g6_brain_update(&test_brain, 700.0f, 1180.0f, 1.0f, 15.0f, 50.0f,
                        G6_VR_TEMP_NO_SENSOR, 0.5f, 40);
    }
    G6BrainTelemetry t;
    g6_brain_get_telemetry(&test_brain, &t);
    TEST_ASSERT_EQUAL_FLOAT(15.0f, t.last_efficiency);

    /* err^2 ~ 2.5e5 against 9*(xPx + 0.01) — unambiguous 3-sigma reject. */
    g6_brain_update(&test_brain, 700.0f, 1180.0f, 500.0f, 15.0f, 50.0f,
                    G6_VR_TEMP_NO_SENSOR, 0.5f, 40);
    TEST_ASSERT_EQUAL(G6_SAFETY_SAMPLE_QUALITY, test_brain.last_safety_status);

    g6_brain_get_telemetry(&test_brain, &t);
    TEST_ASSERT_EQUAL_FLOAT(15.0f, t.last_efficiency);

    /* A subsequent accepted sample must update it again. */
    g6_brain_update(&test_brain, 700.0f, 1180.0f, 1.0f, 16.0f, 50.0f,
                    G6_VR_TEMP_NO_SENSOR, 0.5f, 40);
    g6_brain_get_telemetry(&test_brain, &t);
    TEST_ASSERT_EQUAL_FLOAT(16.0f, t.last_efficiency);
}

TEST_CASE("NVS fingerprint with corrupt in-frame size field is rejected and erased", "[g6_brain]") {
    /* The frame's payload-size field was written since the schema's
     * introduction but never read back; a frame with a corrupted interior
     * header loaded silently. Post-fix the loader verifies it and routes a
     * mismatch through the same WARN/erase/cold-start path as every other
     * bad-blob case. */
    TEST_ASSERT_EQUAL(ESP_OK, g6_brain_save_nvs_fingerprint(&test_brain));

    nvs_handle_t nvs;
    TEST_ASSERT_EQUAL(ESP_OK, nvs_open("g6_brain", NVS_READWRITE, &nvs));
    uint8_t buffer[1024];
    size_t blob_size = sizeof(buffer);
    TEST_ASSERT_EQUAL(ESP_OK, nvs_get_blob(nvs, "theta_fingerprint", buffer, &blob_size));

    buffer[sizeof(uint32_t)] ^= 0xFF;   /* corrupt the size field, keep version + length intact */
    TEST_ASSERT_EQUAL(ESP_OK, nvs_set_blob(nvs, "theta_fingerprint", buffer, blob_size));
    TEST_ASSERT_EQUAL(ESP_OK, nvs_commit(nvs));
    nvs_close(nvs);

    /* Poison a coefficient so acceptance of the corrupt frame would be visible. */
    test_brain.theta[0] = 42.0f;
    test_brain.nvs_valid = false;

    TEST_ASSERT_EQUAL(ESP_OK, g6_brain_load_nvs_fingerprint(&test_brain));
    TEST_ASSERT_FALSE(test_brain.nvs_valid);
    TEST_ASSERT_TRUE(test_brain.cold_start);
    TEST_ASSERT_EQUAL_FLOAT(42.0f, test_brain.theta[0]);   /* frame was not applied */

    /* The corrupt blob must be gone, exactly like the other bad-blob paths. */
    TEST_ASSERT_EQUAL(ESP_OK, nvs_open("g6_brain", NVS_READONLY, &nvs));
    size_t sz = 0;
    esp_err_t get_err = nvs_get_blob(nvs, "theta_fingerprint", NULL, &sz);
    nvs_close(nvs);
    TEST_ASSERT_EQUAL(ESP_ERR_NVS_NOT_FOUND, get_err);
}
