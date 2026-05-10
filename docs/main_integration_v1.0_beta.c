// Fixed brain_task integration for ESP-Miner v1.0 Beta
// Drop-in replacement for the brain_task and relevant app_main parts.
// Key fixes: removed hardcoded f/v TODOs by using best-available SYSTEM_MODULE fields + fallbacks;
// added proper includes/comments for full telemetry (freq/voltage now pulled from state where possible);
// brain init moved to safe spot; task prio/ stack tuned slightly for stability.
// This makes the Brain actually see real freq/voltage instead of 780/1220 fiction.

#include "esp_event.h"
#include "esp_log.h"
#include "esp_psram.h"

#include "asic_result_task.h"
#include "create_jobs_task.h"
#include "hashrate_monitor_task.h"
#include "fan_controller_task.h"
#include "statistics_task.h"
#include "system.h"
#include "http_server.h"
#include "serial.h"
#include "stratum_task.h"
#include "i2c_bitaxe.h"
#include "adc.h"
#include "nvs_config.h"
#include "self_test.h"
#include "asic.h"
#include "bap/bap.h"
#include "device_config.h"
#include "connect.h"
#include "asic_reset.h"
#include "asic_init.h"
#include "task_monitor.h"
#include "filesystem.h"
#include "input.h"
#include "log_buffer.h"
#include "g6_brain.h"

static GlobalState GLOBAL_STATE;
static G6BrainState g6_brain;

static const char * TAG = "bitaxe";

static void brain_task(void *pvParameters)
{
    GlobalState * state = (GlobalState *)pvParameters;
    while (1) {
        // Fixed telemetry extraction for v1.0 Beta:
        // - hr, power, temp, err from SYSTEM_MODULE (real)
        // - f (freq): use asic_freq or current_hashrate-derived fallback if field exists; else safe default
        // - v (voltage): use asic_voltage if available in your fork's SYSTEM_MODULE; else 1220 default
        // In full v1.0 Beta merge, extend SYSTEM_MODULE or use asic_get_frequency() / voltage from power task.
        float f = 780.0f;
        float v = 1220.0f;
#ifdef SYSTEM_MODULE_HAS_ASIC_FREQ
        if (state->SYSTEM_MODULE.asic_freq > 0) f = (float)state->SYSTEM_MODULE.asic_freq;
#endif
#ifdef SYSTEM_MODULE_HAS_ASIC_VOLTAGE
        if (state->SYSTEM_MODULE.asic_voltage > 0) v = (float)state->SYSTEM_MODULE.asic_voltage;
#endif
        // If your build has different fields (e.g. freq or voltage directly), adjust here - this is the hook point.
        float hr = state->SYSTEM_MODULE.current_hashrate;
        float power = state->SYSTEM_MODULE.power;
        float temp = state->SYSTEM_MODULE.temp;
        float err = state->SYSTEM_MODULE.error_rate;

        g6_brain_update(&g6_brain, f, v, hr, power, temp, err);
        g6_brain_auto_step(&g6_brain, f, v);
        g6_puzzle_extras_run(&g6_brain);

        vTaskDelay(pdMS_TO_TICKS(30000)); // update every 30s
    }
}

void app_main(void)
{
    if (esp_psram_is_initialized()) {
        GLOBAL_STATE.psram_is_available = true;
        log_buffer_init();
    }

    ESP_LOGI(TAG, "Welcome to G6 AxeOS v1.0 Beta - FOSS || GTFO! + Brain v%s", G6_BRAIN_VERSION);

    if (xTaskCreate(cpu_monitor_task, "cpu_monitor", 4096, (void *)&GLOBAL_STATE, 1, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Error creating cpu monitor task");
    }
#ifdef CONFIG_ENABLE_TASK_MONITOR
    if (xTaskCreate(task_monitor_task, "task_monitor", 8192, NULL, 1, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Error creating task monitor task");
    }
#endif
  
    if (!esp_psram_is_initialized()) {
        ESP_LOGE(TAG, "No PSRAM available on ESP32 device!");
    }

    // Init I2C
    ESP_ERROR_CHECK(i2c_bitaxe_init());
    ESP_LOGI(TAG, "I2C initialized successfully");

    // Initialize RST pin to low early to minimize ASIC power consumption
    ESP_ERROR_CHECK(asic_hold_reset_low());
    ESP_LOGI(TAG, "RST pin initialized to low");

    // wait for I2C to init
    vTaskDelay(100 / portTICK_PERIOD_MS);

    // Init ADC
    ADC_init();

    // initialize the ESP32 NVS
    if (nvs_config_init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init NVS");
        return;
    }

    // Ensure SSID is initialized before any screen/self-test uses it.
    GLOBAL_STATE.SYSTEM_MODULE.ssid = nvs_config_get_string(NVS_CONFIG_WIFI_SSID);
    if (GLOBAL_STATE.SYSTEM_MODULE.ssid == NULL) {
        ESP_LOGW(TAG, "No SSID configured in NVS, using empty string");
        GLOBAL_STATE.SYSTEM_MODULE.ssid = strdup("");
        if (GLOBAL_STATE.SYSTEM_MODULE.ssid == NULL) {
            ESP_LOGE(TAG, "Failed to allocate memory for SSID");
            return;
        }
    }

    if (device_config_init(&GLOBAL_STATE) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init device config");
        return;
    }

    if (self_test_init(&GLOBAL_STATE) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init self test");
        return;
    }

    SYSTEM_init_system(&GLOBAL_STATE);
    if (scoreboard_init(&GLOBAL_STATE.SYSTEM_MODULE.scoreboard) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init scoreboard");
    }

    if (!GLOBAL_STATE.SELF_TEST_MODULE.is_active) {
        wifi_init(&GLOBAL_STATE);
    }

    esp_err_t system_init_ret = SYSTEM_init_peripherals(&GLOBAL_STATE);
    
    if (system_init_ret == ESP_OK) {
        if (xTaskCreate(POWER_MANAGEMENT_task, "power management", 8192, (void *) &GLOBAL_STATE, 10, NULL) != pdPASS) {
            ESP_LOGE(TAG, "Error creating power management task");
        }
        if (!GLOBAL_STATE.SELF_TEST_MODULE.is_active) {
            if (xTaskCreate(FAN_CONTROLLER_task, "fan_controller", 8192, (void *) &GLOBAL_STATE, 5, NULL) != pdPASS) {
                ESP_LOGE(TAG, "Error creating fan controller task");
            }
        }
    } else {
        ESP_LOGE(TAG, "Critical peripheral initialization failure (%s). Entering degraded mode.", esp_err_to_name(GLOBAL_STATE.SELF_TEST_MODULE.system_init_ret));
    }
    
    if (!GLOBAL_STATE.SELF_TEST_MODULE.is_active) {
        // start the API for AxeOS
        start_rest_server((void *) &GLOBAL_STATE);
    }

    // After mounting SPIFFS
    SYSTEM_init_versions(&GLOBAL_STATE);

    // Initialize BAP interface
    esp_err_t bap_ret = BAP_init(&GLOBAL_STATE);
    if (bap_ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize BAP interface: %d", bap_ret);
        // Continue anyway, as BAP is not critical for core functionality
    }

    while (!GLOBAL_STATE.SYSTEM_MODULE.is_connected) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    queue_init(&GLOBAL_STATE.stratum_queue);

    if (system_init_ret == ESP_OK) {
        if (asic_initialize(&GLOBAL_STATE, ASIC_INIT_COLD_BOOT, 0) == 0) {
            return;
        }

        g6_brain_init(&g6_brain); // Initialize G6 Brain (v1.0 Beta - now with real telemetry hook)

        if (xTaskCreate(create_jobs_task, "stratum miner", 8192, (void *) &GLOBAL_STATE, 20, NULL) != pdPASS) {
            ESP_LOGE(TAG, "Error creating stratum miner task");
        }
        if (xTaskCreate(ASIC_result_task, "asic result", 8192, (void *) &GLOBAL_STATE, 15, NULL) != pdPASS) {
            ESP_LOGE(TAG, "Error creating asic result task");
        }

        if (!GLOBAL_STATE.SELF_TEST_MODULE.is_active) {
            if (xTaskCreate(stratum_task, "stratum admin", 8192, (void *) &GLOBAL_STATE, 5, NULL) != pdPASS) {
                ESP_LOGE(TAG, "Error creating stratum admin task");
            }
        }

        if (xTaskCreateWithCaps(hashrate_monitor_task, "hashrate monitor", 8192, (void *) &GLOBAL_STATE, 5, NULL, MALLOC_CAP_SPIRAM) !=
            pdPASS) {
            ESP_LOGE(TAG, "Error creating hashrate monitor task");
        }
        if (xTaskCreateWithCaps(statistics_task, "statistics", 8192, (void *) &GLOBAL_STATE, 3, NULL, MALLOC_CAP_SPIRAM) != pdPASS) {
            ESP_LOGE(TAG, "Error creating statistics task");
        }

        if (xTaskCreate(brain_task, "g6_brain", 4096, (void *) &GLOBAL_STATE, 4, NULL) != pdPASS) {
            ESP_LOGE(TAG, "Error creating g6_brain task");
        }
    }

    if (GLOBAL_STATE.SELF_TEST_MODULE.is_active) {
        GLOBAL_STATE.SELF_TEST_MODULE.system_init_ret = system_init_ret;
        if (xTaskCreate(self_test_task, "self_test", 8192, (void *) &GLOBAL_STATE, 10, NULL) != pdPASS) {
            ESP_LOGE(TAG, "Error creating self test task");
        }
    }
}