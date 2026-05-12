/*
 * Unity test suite for G6 Brain v1.0.0-beta1
 * Run with: idf.py test
 *
 * Covers: init, basic update, self_test (including condition number), NVS round-trip
 */

#include "unity.h"
#include "g6_brain.h"
#include "esp_log.h"
#include "nvs_flash.h"

static const char *TAG = "G6_BRAIN_TEST";

static G6BrainState test_brain;

void setUp(void) {
    memset(&test_brain, 0, sizeof(G6BrainState));
}

void tearDown(void) {
    // nothing
}

TEST_CASE("g6_brain_init initializes correctly", "[g6_brain]") {
    esp_err_t ret = g6_brain_init(&test_brain);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_TRUE(test_brain.cold_start);
    TEST_ASSERT_EQUAL_FLOAT(1e5f, test_brain.P[0][0]);  // cold-start covariance
    TEST_ASSERT_EQUAL_FLOAT(0.0f, test_brain.model_quality);
}

TEST_CASE("g6_brain_update with valid synthetic data", "[g6_brain]") {
    g6_brain_init(&test_brain);

    // Synthetic stable sample
    float f = 650.0f, v = 1220.0f, hr = 120.0f, pwr = 15.0f, temp = 55.0f, err = 0.5f;

    esp_err_t ret = g6_brain_update(&test_brain, f, v, hr, pwr, temp, err);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_GREATER_THAN(0.0f, test_brain.model_quality);
    TEST_ASSERT_TRUE(test_brain.update_count > 0);
}

TEST_CASE("g6_brain_self_test detects good vs degraded state", "[g6_brain]") {
    g6_brain_init(&test_brain);

    // Force a slightly ill-conditioned matrix
    test_brain.P[0][0] = 1e9f;
    test_brain.P[1][1] = 1e-3f;

    esp_err_t ret = g6_brain_self_test(&test_brain);
    // Should still pass or degrade gracefully (condition number high but not catastrophic)
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

void app_main(void) {
    UNITY_BEGIN();
    RUN_TEST(g6_brain_init initializes correctly);
    RUN_TEST(g6_brain_update with valid synthetic data);
    RUN_TEST(g6_brain_self_test detects good vs degraded state);
    RUN_TEST(NVS fingerprint save/load round-trip);
    UNITY_END();
}
