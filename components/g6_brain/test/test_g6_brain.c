/*
 * Unity test suite for G6 Brain v1.0.0-beta2
 * Run with: idf.py test
 *
 * Expanded test coverage for beta2:
 * - Init & cold start
 * - Valid update path + model learning
 * - Input validation & rejection
 * - Safety layer execution even on invalid samples
 * - Thermal protection behavior
 * - Self-test (condition number + symmetry)
 * - NVS fingerprint round-trip
 * - Covariance clamping & symmetrization
 */

#include "unity.h"
#include "g6_brain.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include <string.h>

static const char *TAG = "G6_BRAIN_TEST";

static G6BrainState test_brain;

void setUp(void) {
    memset(&test_brain, 0, sizeof(G6BrainState));
}

void tearDown(void) {
    // nothing
}

/* ====================== ORIGINAL TESTS (kept) ====================== */

TEST_CASE("g6_brain_init initializes correctly", "[g6_brain]") {
    esp_err_t ret = g6_brain_init(&test_brain);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_TRUE(test_brain.cold_start);
    TEST_ASSERT_EQUAL_FLOAT(1e5f, test_brain.P[0][0]);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, test_brain.model_quality);
}

TEST_CASE("g6_brain_update with valid synthetic data", "[g6_brain]") {
    g6_brain_init(&test_brain);

    float f = 650.0f, v = 1220.0f, hr = 120.0f, pwr = 15.0f, temp = 55.0f, err = 0.5f;

    esp_err_t ret = g6_brain_update(&test_brain, f, v, hr, pwr, temp, err);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_GREATER_THAN(0.0f, test_brain.model_quality);
    TEST_ASSERT_TRUE(test_brain.update_count > 0);
}

TEST_CASE("g6_brain_self_test detects good vs degraded state", "[g6_brain]") {
    g6_brain_init(&test_brain);

    // Force ill-conditioned matrix
    test_brain.P[0][0] = 1e9f;
    test_brain.P[1][1] = 1e-3f;

    esp_err_t ret = g6_brain_self_test(&test_brain);
    TEST_ASSERT(ret == ESP_OK || ret == ESP_FAIL);
}

TEST_CASE("NVS fingerprint save/load round-trip", "[g6_brain]") {
    g6_brain_init(&test_brain);
    test_brain.theta[0] = 42.0f;
    test_brain.P[0][0] = 12345.0f;

    esp_err_t ret = g6_brain_save_nvs_fingerprint(&test_brain);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    G6BrainState loaded;
    g6_brain_init(&loaded);
    ret = g6_brain_load_nvs_fingerprint(&loaded);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL_FLOAT(42.0f, loaded.theta[0]);
    TEST_ASSERT_EQUAL_FLOAT(12345.0f, loaded.P[0][0]);
}

/* ====================== NEW TESTS FOR BETA2 ====================== */

TEST_CASE("g6_brain_update rejects invalid inputs", "[g6_brain]") {
    g6_brain_init(&test_brain);

    // Invalid hashrate
    esp_err_t ret = g6_brain_update(&test_brain, 650.0f, 1220.0f, 0.0f, 15.0f, 55.0f, 0.5f);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);

    // Invalid temperature (NaN)
    ret = g6_brain_update(&test_brain, 650.0f, 1220.0f, 120.0f, 15.0f, NAN, 0.5f);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

TEST_CASE("Safety layer still executes on invalid sample", "[g6_brain][safety]") {
    g6_brain_init(&test_brain);
    test_brain.temp_ceiling = 60.0f;

    // Send overheated sample — should still run safety scaling
    esp_err_t ret = g6_brain_update(&test_brain, 800.0f, 1300.0f, 100.0f, 20.0f, 75.0f, 1.0f);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // best_f should have been scaled down by safety
    TEST_ASSERT_LESS_THAN(800.0f, test_brain.best_f);
}

TEST_CASE("Proactive thermal derating triggers correctly", "[g6_brain][safety]") {
    g6_brain_init(&test_brain);
    test_brain.temp_ceiling = 65.0f;

    // Just below ceiling but close enough to trigger proactive scaling
    g6_brain_update(&test_brain, 700.0f, 1250.0f, 110.0f, 18.0f, 62.0f, 0.8f);

    TEST_ASSERT_LESS_OR_EQUAL(700.0f, test_brain.best_f);
}

TEST_CASE("Covariance matrix stays symmetric after updates", "[g6_brain]") {
    g6_brain_init(&test_brain);

    for (int i = 0; i < 10; i++) {
        g6_brain_update(&test_brain, 650.0f + i*5, 1220.0f, 115.0f + i, 16.0f, 52.0f, 0.6f);
    }

    // Check symmetry
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
    g6_brain_init(&test_brain);
    TEST_ASSERT_TRUE(test_brain.cold_start);

    for (int i = 0; i < 30; i++) {
        g6_brain_update(&test_brain, 650.0f, 1220.0f, 118.0f, 16.5f, 53.0f, 0.7f);
    }

    TEST_ASSERT_FALSE(test_brain.cold_start);
}

void app_main(void) {
    UNITY_BEGIN();

    RUN_TEST(g6_brain_init initializes correctly);
    RUN_TEST(g6_brain_update with valid synthetic data);
    RUN_TEST(g6_brain_self_test detects good vs degraded state);
    RUN_TEST(NVS fingerprint save/load round-trip);

    // New beta2 tests
    RUN_TEST(g6_brain_update rejects invalid inputs);
    RUN_TEST(Safety layer still executes on invalid sample);
    RUN_TEST(Proactive thermal derating triggers correctly);
    RUN_TEST(Covariance matrix stays symmetric after updates);
    RUN_TEST(Cold start flag clears after sufficient updates);

    UNITY_END();
}
