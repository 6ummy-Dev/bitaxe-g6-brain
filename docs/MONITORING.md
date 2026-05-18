# MONITORING.md — Real-Time G6 Brain Observability (v1.0.0-beta2 Final)

**How to know if your G6 Brain is healthy, learning, and staying safe.**

Run this in your serial monitor:

```bash
grep -E "G6_BRAIN|PROACTIVE THERMAL|HIGH ERROR RATE|NVS fingerprint|Quality=|Mode=|G6_TELEMETRY" /dev/ttyUSB0
```

### Key Log Strings

| Log Message                              | Meaning                                      | Action if frequent |
|------------------------------------------|----------------------------------------------|--------------------|
| `G6 Brain initialized`                   | Normal startup                               | None |
| `NVS fingerprint auto-saved`             | Model persisted                              | None |
| `PROACTIVE THERMAL`                      | Approaching thermal ceiling                  | Improve cooling |
| `HIGH ERROR RATE`                        | NER back-off triggered                       | Check PSU / cooling / ASIC |
| `Quality=0.XX`                           | Model quality                                | See thresholds below |
| `G6_TELEMETRY` lines                     | Telemetry snapshot                           | Use for dashboards |

### Model Quality Thresholds

- **> 0.85** → Excellent — fully learned your silicon
- **0.60 – 0.85** → Good — still learning, safe in RECOMMEND
- **< 0.60** → Poor — conservative mode

### Recommended First 24–48h

1. Start in `G6_MODE_RECOMMEND`
2. Let it run ≥ 24 hours
3. Watch rising `model_quality`
4. Use `g6_brain_get_telemetry()` for detailed snapshots
5. Only switch to `AUTO` after quality is stable and high

### Quick Debug (in your code)

```c
G6BrainTelemetry tel = {0};
g6_brain_get_telemetry(&brain, &tel);

ESP_LOGI(TAG, "Quality=%.3f | trace_h=%.2e | innov=%.4f | eff_mode=%d",
         g6_brain_get_model_quality(&brain),
         tel.trace_P_hashrate,
         tel.last_innovation,
         tel.efficiency_mode_active);
```

If you need to wipe the learned model:

```c
g6_brain_reset(&brain);
```

**Pro tip:** Pipe telemetry to ESPHome / Home Assistant / Grafana for long-term tracking.