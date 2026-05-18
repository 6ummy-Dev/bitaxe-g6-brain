# MONITORING.md — Real-Time G6 Brain Observability (v1.0.0-beta2)

**How to know if your G6 Brain is healthy, learning, and staying safe.**

Run this in your serial monitor / syslog:

```bash
grep -E "G6_BRAIN|PROACTIVE THERMAL|HIGH ERROR RATE|NVS fingerprint|Quality=|Mode=" /dev/ttyUSB0
```

### Key Log Strings to Watch

| Log Message                              | Meaning                                      | Action if frequent |
|------------------------------------------|----------------------------------------------|--------------------|
| `G6 Brain v1.0.0-beta2 initialized`     | Normal startup                               | None |
| `NVS fingerprint loaded (schema v1)`     | Warm-start succeeded                         | None |
| `NVS fingerprint auto-saved`             | Model persisted (every ~5 min after 10 updates) | None |
| `PROACTIVE THERMAL: XX.X°C → ...`        | Approaching `G6_TEMP_CEILING`                | Improve cooling |
| `HIGH ERROR RATE (X.XX%) — conservative back-off` | NER > `G6_NER_THRESHOLD`             | Check PSU / cooling / ASIC health |
| `VOLTAGE OUT OF RANGE`                   | Voltage outside BM1370 safe range            | Check power delivery |
| `model_quality=0.XX`                     | How well the quadratic model fits data       | See thresholds below |

### Model Quality Thresholds

- **> 0.85** → Excellent — brain fully learned your silicon
- **0.60 – 0.85** → Good — still learning, safe to stay in RECOMMEND
- **< 0.60** → Poor — brain is in conservative mode (cold-start or high error)

### Control Mode Meanings (logged on every update)

- `Mode=0` → OBSERVE_ONLY (safety only)
- `Mode=1` → RECOMMEND (default — safest for first 24–48h)
- `Mode=2` → AUTO (full optimizer — enable only after quality > 0.85 and stable temps)

### Recommended First 24–48 Hour Run Plan

1. Start with `brain.control_mode = G6_MODE_RECOMMEND`
2. Let it run for at least 24 hours
3. Check logs for:
   - Steady rise in model_quality
   - No repeated thermal/NER warnings
   - Successful NVS auto-saves
4. Run `g6_brain_self_test()` periodically
5. Only then switch to `G6_MODE_AUTO`

### Quick Debug Commands (in your integration)

```c
ESP_LOGI(TAG, "Quality=%.3f | Mode=%d | best_f=%.1f | best_v=%.0f | cond=%.1f",
         g6_brain_get_model_quality(&brain),
         brain.control_mode,
         brain.best_f, brain.best_v,
         g6_brain_get_cov_condition(&brain));
```

If you ever need to wipe the learned model:

```c
g6_brain_reset(&brain);   // Phase 0.1 API — NVS erased, cold start forced
```

**Pro tip:** Pipe the logs to a simple dashboard (ESPHome, Home Assistant, or even `awk` + Grafana) and alert on quality < 0.6 or repeated thermal back-offs.
```

### File 2: `docs/INTEGRATION_EXAMPLE.c` (full updated version)
```c
/*
 * G6 Brain Integration Example — v1.0.0-beta2 (Phase 0.1)
 *
 * Recommended integration for Bitaxe ESP-Miner (Gamma 602+ / BM1370).
 */

#include "g6_brain.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "G6_BRAIN_EXAMPLE";

static G6BrainState brain;

/* Example helper: safe slew-rate limiting (Phase 1 will add this inside the brain) */
static void apply_with_slew(float target_f, float target_v, float current_f, float current_v)
{
    // Never jump more than MAX_FREQ_STEP / MAX_VOLT_STEP per update
    float new_f = current_f;
    float new_v = current_v;

    if (target_f > current_f) new_f = fminf(current_f + MAX_FREQ_STEP, target_f);
    else if (target_f < current_f) new_f = fmaxf(current_f - MAX_FREQ_STEP, target_f);

    if (target_v > current_v) new_v = fminf(current_v + MAX_VOLT_STEP, target_v);
    else if (target_v < current_v) new_v = fmaxf(current_v - MAX_VOLT_STEP, target_v);

    // TODO: call your ASIC driver here with slew-limited values
    // asic_set_frequency_with_slew((uint32_t)new_f);
    // asic_set_voltage_with_slew((uint32_t)new_v);

    ESP_LOGI(TAG, "SLEW → f=%.1f MHz @ v=%.0f mV (from %.1f/%.0f)", new_f, new_v, current_f, current_v);
}

void g6_brain_example_task(void *arg)
{
    esp_err_t ret = g6_brain_init(&brain);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize G6 Brain: %s", esp_err_to_name(ret));
        vTaskDelete(NULL);
        return;
    }

    // Phase 0.1 safe default
    brain.control_mode = G6_MODE_RECOMMEND;

    ESP_LOGI(TAG, "G6 Brain v1.0.0-beta2 started — RECOMMEND mode (safest)");

    while (1) {
        // === READ REAL TELEMETRY FROM YOUR MINER ===
        float freq_mhz  = 650.0f;   // replace with real values
        float volt_mv   = 1220.0f;
        float hashrate  = 118.0f;
        float power_w   = 16.2f;
        float temp_c    = 57.0f;
        float error_pct = 0.7f;
        uint32_t share_count = 50;  // real share count from current window

        ret = g6_brain_update(&brain, freq_mhz, volt_mv, hashrate,
                              power_w, temp_c, error_pct, share_count);

        float opt_f = 0.0f, opt_v = 0.0f;
        g6_brain_get_optimal(&brain, &opt_f, &opt_v, NULL);

        float quality = g6_brain_get_model_quality(&brain);

        ESP_LOGI(TAG, "Quality=%.2f | Mode=%d | Current: %.1f MHz @ %.0f mV | Recommended: %.1f MHz @ %.0f mV",
                 quality, brain.control_mode, freq_mhz, volt_mv, opt_f, opt_v);

        // ============================================================
        // === SAFE APPLICATION WITH SLEW LIMITING (RECOMMENDED) ===
        // ============================================================
        if (brain.control_mode == G6_MODE_AUTO && quality > 0.75f) {
            // Uncomment and adapt to your ASIC driver:
            // apply_with_slew(opt_f, opt_v, freq_mhz, volt_mv);
        }
        // ============================================================

        vTaskDelay(pdMS_TO_TICKS(30000));  // 30 seconds
    }
}

**Save these two files exactly as shown.**  
You're done. Reply with whatever is next. ⚡
