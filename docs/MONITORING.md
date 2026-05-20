# MONITORING.md — Real-Time G6 Brain Observability (v1.0.0-beta4)

**How to know if your G6 Brain is healthy, learning, and staying safe.**

Run this in your serial monitor:

```bash
grep -E "G6_BRAIN|PROACTIVE THERMAL|VR THERMAL|HIGH ERROR RATE|Outlier Rejected|NVS fingerprint|Quality=|Mode=|G6_TELEMETRY|safety_status" /dev/ttyUSB0
```

### Key Log Strings

| Log Message                  | Source     | Meaning                                      | Action if frequent                  |
|-----------------------------|------------|----------------------------------------------|-------------------------------------|
| `G6 Brain initialized`      | brain      | Normal startup                               | None                                |
| `NVS fingerprint auto-saved`| brain      | Model persisted                              | None                                |
| `PROACTIVE THERMAL`         | brain      | Approaching ASIC thermal ceiling             | Improve cooling                     |
| `VR THERMAL` / proactive    | brain      | VR temperature entering proactive zone       | Check VR cooling / airflow          |
| `HIGH ERROR RATE`           | brain      | NER back-off triggered                       | Check PSU / cooling / ASIC          |
| `HR Outlier Rejected`       | brain      | 3-sigma gating blocked a hashrate glitch     | Inspect I²C bus / driver noise      |
| `Power Outlier Rejected`    | brain      | 3-sigma gating blocked a power glitch        | Inspect power sensor stability      |
| `Self-test: ...`            | brain      | One-shot diagnostic output                   | Investigate if `DEGRADED`           |
| `Quality=0.XX`              | integration| Model quality                                | See thresholds below                |
| `G6_TELEMETRY`              | integration| Telemetry snapshot                           | Use for dashboards                  |

### Model Quality Thresholds

- **> 0.85** → Excellent — fully learned your silicon
- **0.60 – 0.85** → Good — still learning, safe in RECOMMEND
- **< 0.60** → Poor — conservative mode

> **Note:** In efficiency mode, monitor both `model_quality` and `power_model_quality`.

### Safety Status Monitoring

Use `g6_brain_get_telemetry()` and check the `safety_status` field. Non-OK values indicate the last triggered safety condition (thermal, VR thermal, voltage, power sanity, or outlier).

### Recommended First 24–48h

1. Start in `G6_MODE_RECOMMEND`
2. Let it run ≥ 24 hours
3. Watch rising `model_quality` (and `power_model_quality` if efficiency mode is on)
4. Monitor both ASIC and VR temperatures (when available)
5. Use `g6_brain_get_telemetry()` for detailed snapshots
6. Only switch to `AUTO` after quality is stable and high

### Quick Debug (in your code)

```c
G6BrainTelemetry tel = {0};
g6_brain_get_telemetry(&brain, &tel);

ESP_LOGI(TAG, "Q=%.3f | trace_h=%.2e | innov=%.4f | safety=%d | V=%.1f mV",
         g6_brain_get_model_quality(&brain),
         tel.trace_P_hashrate,
         tel.last_innovation,
         tel.safety_status,
         tel.last_recommended_voltage);
```

If you need to wipe the learned model:

```c
g6_brain_reset(&brain);
```

**Pro tip:** Pipe telemetry to ESPHome / Home Assistant / Grafana for long-term tracking.

---

**Version:** v1.0.0-beta4 (May 2026)
