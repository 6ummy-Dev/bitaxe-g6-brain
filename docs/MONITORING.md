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
