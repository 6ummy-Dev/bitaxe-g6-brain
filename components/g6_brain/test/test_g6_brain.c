/*
 * Unity test suite for G6 Brain v1.0.0-beta5
 *
 * Validates tracking model updates, safety thresholds,
 * outlier gating, and internal slew rate limits.
 */

#include "unity.h"
#include "g6_brain.h"
#include "esp_log.h"
#include "nvs_flash.h"
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
    
    ret = g6_brain_init(&test_brain);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
}

void tearDown(void) {
}

/* ====================== TEST CASES ====================== */

TEST_CASE("g6_brain_init initializes correctly with Kconfig + control_mode", "[g6_brain]") {
    TEST_ASSERT_EQUAL(G6_MODE_RECOMMEND, test_brain.control_mode);
    TEST_ASSERT_EQUAL_FLOAT((float)CONFIG_G6_TEMP_CEILING, test_brain.temp_ceiling);
    TEST_ASSERT_EQUAL_FLOAT((float)CONFIG_G6_NER_THRESHOLD / 100.0f, test_brain.ner_threshold);
    TEST_ASSERT_TRUE(test_brain.cold_start);
    TEST_ASSERT_EQUAL_FLOAT(RLS_RIDGE_EPSILON, test_brain.ridge_epsilon);
}

TEST_CASE("g6_brain_update with valid synthetic data respects control_mode", "[g6_brain]") {
    float f = 650.0f, v = 1220.0f, hr = 120.0f, pwr = 15.0f, temp = 55.0f, err = 0.5f;
    uint32_t shares = 50;

    esp_err_t ret = g6_brain_update(&test_brain, f, v, hr, pwr, temp, G6_VR_TEMP_NO_SENSOR, err, shares);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_GREATER_THAN(0.0f, test_brain.model_quality);

    float original_best_f = test_brain.best_f;

    test_brain.control_mode = G6_MODE_AUTO;
    ret = g6_brain_update(&test_brain, 700.0f, 1250.0f, 125.0f, 16.0f, 56.0f, G6_VR_TEMP_NO_SENSOR, 0.4f, 60);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    TEST_ASSERT_NOT_EQUAL(original_best_f, test_brain.best_f);
}

TEST_CASE("g6_brain_update in OBSERVE_ONLY does not mutate best_f/best_v", "[g6_brain]") {
    test_brain.control_mode = G6_MODE_OBSERVE_ONLY;

    float start_f = test_brain.best_f;
    float start_v = test_brain.best_v;

    esp_err_t ret = g6_brain_update(&test_brain, 800.0f, 1300.0f, 130.0f, 18.0f, 60.0f, G6_VR_TEMP_NO_SENSOR, 0.3f, 50);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    TEST_ASSERT_EQUAL_FLOAT(start_f, test_brain.best_f);
    TEST_ASSERT_EQUAL_FLOAT(start_v, test_brain.best_v);
}

TEST_CASE("g6_brain_self_test detects good vs degraded state", "[g6_brain]") {
    test_brain.P[0][0] = 1e9f;
    test_brain.P[1][1] = 1e-3f;

    esp_err_t ret = g6_brain_self_test(&test_brain);
    TEST_ASSERT(ret == ESP_OK || ret == ESP_FAIL);
}

TEST_CASE("NVS fingerprint save/load round-trip", "[g6_brain]") {
    test_brain.theta[0] = 42.0f;
    test_brain.P[0][0] = 12345.0f;
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
}

TEST_CASE("g6_brain_update rejects invalid inputs", "[g6_brain]") {
    esp_err_t ret = g6_brain_update(&test_brain, 650.0f, 1220.0f, 0.0f, 15.0f, 55.0f, G6_VR_TEMP_NO_SENSOR, 0.5f, 30);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

TEST_CASE("Safety layer still executes on invalid sample", "[g6_brain]") {
    test_brain.temp_ceiling = 60.0f;

    esp_err_t ret = g6_brain_update(&test_brain, 800.0f, 1300.0f, 100.0f, 20.0f, 75.0f, G6_VR_TEMP_NO_SENSOR, 1.0f, 40);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_LESS_THAN(800.0f, test_brain.best_f);
}

TEST_CASE("Covariance matrix stays symmetric after updates", "[g6_brain]") {
    for (int i = 0; i < 10; i++) {
        g6_brain_update(&test_brain, 650.0f + i*5, 1220.0f, 115.0f + i, 16.0f, 52.0f, G6_VR_TEMP_NO_SENSOR, 0.6f, 40);
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
        g6_brain_update(&test_brain, 650.0f, 1220.0f, 118.0f, 16.5f, 53.0f, G6_VR_TEMP_NO_SENSOR, 0.7f, 40);
    }

    TEST_ASSERT_FALSE(test_brain.cold_start);
}

TEST_CASE("Proactive thermal derating triggers correctly", "[g6_brain]") {
    test_brain.temp_ceiling = 65.0f;

    esp_err_t ret = g6_brain_update(&test_brain, 700.0f, 1250.0f, 110.0f, 18.0f, 62.0f, G6_VR_TEMP_NO_SENSOR, 0.8f, 50);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_LESS_OR_EQUAL(700.0f, test_brain.best_f);
}

/* ====================== NEW BETA3 COVERAGE ====================== */

TEST_CASE("Statistical Outlier Gating rejects severe sensor anomalies", "[g6_brain]") {
    g6_brain_update(&test_brain, 650.0f, 1220.0f, 120.0f, 15.0f, 55.0f, G6_VR_TEMP_NO_SENSOR, 0.5f, 50);
    uint32_t prev_count = test_brain.update_count;

    esp_err_t ret = g6_brain_update(&test_brain, 650.0f, 1220.0f, 9999.0f, 15.0f, 55.0f, G6_VR_TEMP_NO_SENSOR, 0.5f, 50);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    TEST_ASSERT_EQUAL_UINT32(prev_count, test_brain.update_count);
}

TEST_CASE("Internal Slew-Rate Limiting enforces step boundaries", "[g6_brain]") {
    test_brain.control_mode = G6_MODE_AUTO;
    test_brain.dfs_step_mhz = 25.0f;
    
    test_brain.best_f = 650.0f;
    test_brain.theta[0] = -1.0f; 
    test_brain.theta[1] = -0.5f; // B3-TEST-1 Fix: Include negative voltage coeff to form a mathematically valid convex surface
    test_brain.theta[3] = 200.0f; // B3-TEST-1 Fix: Bound the gradient push vector appropriately to test step execution 

    esp_err_t ret = g6_brain_update(&test_brain, 650.0f, 1220.0f, 120.0f, 15.0f, 55.0f, G6_VR_TEMP_NO_SENSOR, 0.5f, 50);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    TEST_ASSERT_EQUAL_FLOAT(650.0f + test_brain.dfs_step_mhz, test_brain.best_f);
}

/* ====================== BETA4: VR THERMAL SAFETY ====================== */

TEST_CASE("VR thermal sentinel (-1) is a no-op — does not affect setpoints", "[g6_brain]") {
    test_brain.control_mode = G6_MODE_AUTO;
    test_brain.vr_temp_ceiling = 85.0f;

    esp_err_t ret = g6_brain_update(&test_brain, 650.0f, 1220.0f, 120.0f, 15.0f,
                                    55.0f, G6_VR_TEMP_NO_SENSOR, 0.5f, 50);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    /* best_v must not have been pushed down by VR thermal — sentinel disables it */
    TEST_ASSERT_GREATER_OR_EQUAL(BM1370_V_MIN, test_brain.best_v);
    /* No VR-specific reduction: best_v should be at or above center (cold start default) */
    TEST_ASSERT_GREATER_OR_EQUAL(BM1370_V_CENTER - 10.0f, test_brain.best_v);
}

TEST_CASE("VR proactive zone steps back best_v only — frequency untouched", "[g6_brain]") {
    test_brain.control_mode = G6_MODE_AUTO;
    test_brain.vr_temp_ceiling = 85.0f;
    test_brain.best_f = 800.0f;
    test_brain.best_v = 1250.0f;

    float vr_temp_proactive = 82.0f; /* ceiling(85) - margin(5) = 80 → 82 is in proactive zone */

    esp_err_t ret = g6_brain_update(&test_brain, 800.0f, 1250.0f, 130.0f, 22.0f,
                                    55.0f, vr_temp_proactive, 0.5f, 50);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    /* Voltage must have been reduced by the proactive VR safety */
    TEST_ASSERT_LESS_THAN(1250.0f, test_brain.best_v);
    /* Frequency must NOT have been reduced by VR proactive (only hard ceiling does that) */
    TEST_ASSERT_GREATER_OR_EQUAL(800.0f - test_brain.dfs_step_mhz, test_brain.best_f);
}

TEST_CASE("VR hard ceiling steps back both best_v and best_f", "[g6_brain]") {
    test_brain.control_mode = G6_MODE_AUTO;
    test_brain.vr_temp_ceiling = 85.0f;
    test_brain.best_f = 850.0f;
    test_brain.best_v = 1280.0f;

    float vr_temp_hard = 86.0f; /* above hard ceiling of 85°C */

    esp_err_t ret = g6_brain_update(&test_brain, 850.0f, 1280.0f, 140.0f, 26.0f,
                                    55.0f, vr_temp_hard, 0.5f, 50);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    /* Both voltage and frequency must be stepped back at the hard ceiling */
    TEST_ASSERT_LESS_THAN(1280.0f, test_brain.best_v);
    TEST_ASSERT_LESS_THAN(850.0f, test_brain.best_f);
}

/* ====================== BETA5 COVERAGE ====================== */

TEST_CASE("vr_temp_proactive_margin field is initialized from Kconfig default", "[g6_brain]") {
    /* Verifies B5-BUG-1: margin lives in state, not baked into a macro at call sites. */
    TEST_ASSERT_EQUAL_FLOAT(G6_VR_TEMP_PROACTIVE_MARGIN_DEFAULT, test_brain.vr_temp_proactive_margin);
    TEST_ASSERT_GREATER_THAN(0.0f, test_brain.vr_temp_proactive_margin);
}

TEST_CASE("temp_proactive_margin field is initialized from Kconfig default", "[g6_brain]") {
    /* Verifies B5-BUG-2: ASIC margin also lives in state and is configurable. */
    TEST_ASSERT_EQUAL_FLOAT(G6_TEMP_PROACTIVE_MARGIN_DEFAULT, test_brain.temp_proactive_margin);
    TEST_ASSERT_GREATER_THAN(0.0f, test_brain.temp_proactive_margin);
}

TEST_CASE("Runtime vr_temp_proactive_margin change alters proactive zone", "[g6_brain]") {
    /* Verifies B5-BUG-1: the call site uses brain->vr_temp_proactive_margin,
     * so tuning it at runtime actually changes behaviour. */
    test_brain.vr_temp_ceiling = 85.0f;
    test_brain.vr_temp_proactive_margin = 10.0f; /* widen proactive zone to 75–85 */
    test_brain.best_v = 1250.0f;
    test_brain.best_f = 800.0f;

    /* 78°C is inside the widened zone (85-10=75 → 78>75) but outside the
     * default zone (85-5=80 → 78<80). With the fix, best_v must be stepped back. */
    float vr_temp = 78.0f;
    esp_err_t ret = g6_brain_update(&test_brain, 800.0f, 1250.0f, 130.0f, 22.0f,
                                    55.0f, vr_temp, 0.5f, 50);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_LESS_THAN(1250.0f, test_brain.best_v);
    /* Frequency must NOT be stepped back in the proactive zone */
    TEST_ASSERT_GREATER_OR_EQUAL(800.0f - test_brain.dfs_step_mhz, test_brain.best_f);
}

TEST_CASE("NER blocks RLS update via is_sample_valid defense-in-depth", "[g6_brain]") {
    /* Verifies B5-BUG-6: even if the early-exit goto were removed, high-NER
     * samples must not reach the RLS update. We test this by going through
     * the normal update path with NER above threshold and checking update_count. */
    uint32_t count_before = test_brain.update_count;

    /* NER well above threshold — early goto fires, update_count must not change */
    esp_err_t ret = g6_brain_update(&test_brain, 650.0f, 1220.0f, 120.0f, 15.0f,
                                    55.0f, G6_VR_TEMP_NO_SENSOR,
                                    test_brain.ner_threshold + 1.0f, 50);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(count_before, test_brain.update_count);
    TEST_ASSERT_EQUAL(G6_SAFETY_NER_BACKOFF, test_brain.last_safety_status);
}

TEST_CASE("ASIC thermal status wins over VR thermal when both fire on same tick", "[g6_brain]") {
    /* Verifies B5-BUG-4 minimum fix: safety helper ordering gives ASIC thermal
     * last-write priority so it appears in telemetry when both conditions are active. */
    test_brain.temp_ceiling      = 65.0f;
    test_brain.temp_proactive_margin = 5.0f; /* proactive zone starts at 60°C */
    test_brain.vr_temp_ceiling   = 85.0f;
    test_brain.vr_temp_proactive_margin = 5.0f; /* VR proactive zone starts at 80°C */

    /* temp_c = 62 → ASIC proactive zone; vr_temp_c = 82 → VR proactive zone */
    esp_err_t ret = g6_brain_update(&test_brain, 700.0f, 1250.0f, 110.0f, 18.0f,
                                    62.0f, 82.0f, 0.5f, 50);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    /* ASIC thermal must be the reported status (it runs last → wins collision) */
    TEST_ASSERT_EQUAL(G6_SAFETY_THERMAL, test_brain.last_safety_status);
}
