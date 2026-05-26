# MONITORING.md — Real-Time G6 Brain Observability (v1.0.0-beta6)

**How to know if your G6 Brain is healthy, learning, and staying safe.**

The brain itself is intentionally quiet on the serial console — only the events you genuinely need to know about are logged. Most observability comes through `g6_brain_get_telemetry()`, which your integration code is responsible for surfacing (via your own log lines, an MQTT topic, Prometheus exporter, etc.).

---

## Serial Console: What the Brain Logs Directly

These are the only log lines emitted from inside `g6_brain.c`. Tag is `G6_BRAIN`.

| Log message (verbatim) | Severity | Meaning | What to do |
|---|---|---|---|
| `P matrix diverged — cold-start recovery applied` | WARN | Covariance trace exceeded `RLS_TRACE_MAX`; brain auto-recovered into a fresh cold-start while preserving your operator config. See `SAFETY.md` item 5. | Investigate if this appears more than once per day — chronic divergence indicates an estimator-tuning issue (lambda min, ridge, or trace bound). One-off events are expected over long uptimes and the brain handles them. |
| `Power Outlier Rejected` | WARN | A power-model 3-sigma gate caught a glitched `power_w` reading. The frame was dropped before updating the power model. | Frequent occurrences → inspect power sensor (INA219/INA260) wiring, sampling rate, or PSU noise. Sporadic single events are normal. |
| `NVS schema mismatch (got vN, expected vM) — erasing` | WARN | The NVS fingerprint blob is from an older brain version. The brain erased it and starts cold. | None — automatic, one-time per upgrade. |
| `NVS blob size mismatch (got X, expected Y) — erasing` | WARN | The NVS blob is the right schema version but wrong byte size (struct evolution or partial write). Brain erased it and starts cold. | None — automatic. |
| `NVS erase failed: <esp_err>` | ERROR | NVS subsystem failed to erase a stale blob. The brain still starts cold this boot, but the bad blob persists across reboots. | Inspect NVS partition health. |

**Grep recipe** (matches every line the brain actually emits):

```bash
grep -E "G6_BRAIN" /dev/ttyUSB0
```

Anything else you see prefixed `G6_TEL` or any other tag is coming from your **integration code**, not the brain itself — see [Integration Logging](#integration-logging-recommended) below.

---

## Telemetry Snapshot (the main observability surface)

Call `g6_brain_get_telemetry()` from your control loop on whatever cadence makes sense (every tick, every minute, on demand). The returned struct gives you a consistent point-in-time view — see `API.md` for the full field list.

The key fields to surface in dashboards or alerts:

| Field | What it tells you |
|---|---|
| `best_f`, `best_v` | Current recommended operating point. In RECOMMEND mode these are what the brain *would* apply if AUTO were enabled. |
| `model_quality` | Hashrate model fit (0.0–1.0). See thresholds below. |
| `power_model_quality` | Power model fit (efficiency mode only). Must be ≥ 0.6 alongside `model_quality` for the Dinkelbach J/TH solver to run. |
| `last_efficiency` | Most recent observed W/TH ratio. Hold-over field — only updated when `power_w` is sane. |
| `update_count`, `power_update_count` | Monotonic counters of accepted RLS updates per estimator. Useful as a "is the brain actually learning" heartbeat — see [Distinguishing accepted vs rejected samples](#distinguishing-accepted-vs-rejected-samples) below. |
| `last_update_timestamp` | FreeRTOS tick of the most recent accepted RLS update. Pairs with `update_count`: both advance together on an accepted sample, neither moves on a rejection. Lets you tell "the brain learned 30s ago" from "the brain hasn't learned since boot" without sampling fast enough to catch the update tick. Wraps every ~49.7 days at the default 1ms tick rate. |
| `trace_P_hashrate`, `trace_P_power` | Covariance traces. Trending up over a long horizon is normal; spikes near `RLS_TRACE_MAX` (default 1e7) indicate the recovery path is about to fire. |
| `last_innovation` | Most recent prediction error on hashrate. Sustained large values indicate the model isn't tracking. |
| `safety_status` | Current `G6SafetyStatus`. See [Safety Status Monitoring](#safety-status-monitoring). |
| `efficiency_mode_active` | Whether the power model + Dinkelbach solver are running this boot. |

### Model Quality Thresholds

- **> 0.85** → Excellent — fully learned your silicon.
- **0.60 – 0.85** → Good — still learning. Safe in RECOMMEND.
- **< 0.60** → Poor — the brain is being conservative. In efficiency mode, the J/TH solver is gated off below 0.6 on *either* model.

### Safety Status Monitoring

`safety_status` reports the most recent safety condition observed during the last `g6_brain_update()` tick. The full reference is in `SAFETY.md`. Quick guide:

- `G6_SAFETY_OK` is the steady-state value during normal operation. **Note:** this status is also reported on non-anomaly sample rejections (low share count, insignificant innovation) — see [Distinguishing accepted vs rejected samples](#distinguishing-accepted-vs-rejected-samples) below for how to tell them apart.
- `G6_SAFETY_THERMAL` or `G6_SAFETY_VR_THERMAL` appearing intermittently → you're brushing the proactive zone. Improve cooling.
- `G6_SAFETY_NER_BACKOFF` → hardware error rate climbed past threshold. Check PSU / cooling / silicon health.
- `G6_SAFETY_INPUT_RANGE` → upstream telemetry is feeding the brain bad values (NaN, out-of-bounds). Your integration code has a bug or a sensor failed.
- `G6_SAFETY_POWER_SANITY` → `power_w` is implausible or a power outlier fired. Inspect the power sensor.
- `G6_SAFETY_SAMPLE_QUALITY` → hashrate outlier rejected. Usually transient noise; chronic occurrences mean instability.
- `G6_SAFETY_P_MATRIX_SINGULAR` → covariance recovery just fired (see also the `WARN` log line above).
- `G6_SAFETY_VOLTAGE` → not currently set by any code path; reserved for a future VRM-ripple check. You can ignore this value today.

### Distinguishing accepted vs rejected samples

`safety_status == G6_SAFETY_OK` alone does **not** mean the most recent sample was accepted into the RLS update. Two non-anomaly rejection paths leave the status at OK:

1. `share_count < MIN_SHARE_COUNT` (the measurement window had too few shares to be statistically meaningful — normal during startup or after a pool change).
2. Insignificant innovation (`xPx < RLS_INNOVATION_THRESHOLD` — the new sample is too close to an existing training point to add information).

In both cases `update_count` is not incremented and no RLS update happens. The canonical "is the brain actually accepting samples?" signal is therefore the **rising `update_count` with `safety_status = G6_SAFETY_OK`** combination, not the status alone. If `safety_status` stays at OK but `update_count` is flat for many ticks, your integration is probably feeding the brain low-share windows or repetitive operating points — not a fault, but worth knowing.

---

## Recommended First 24–48h

1. Start in `G6_MODE_RECOMMEND`.
2. Let it run ≥ 24 hours.
3. Watch `model_quality` climb (and `power_model_quality` if efficiency mode is on).
4. Monitor both ASIC and VR temperatures (when available).
5. Use `g6_brain_get_telemetry()` for detailed snapshots — log them periodically or pipe to a dashboard.
6. Only switch to `AUTO` after model quality is stable and high, and you have not seen unexpected safety statuses.

---

## Integration Logging (recommended)

The brain itself doesn't log periodic telemetry — that's by design (it would couple the brain to your logging conventions). Your integration code should do it. From `INTEGRATION_EXAMPLE.c`:

```c
G6BrainTelemetry tel = {0};
g6_brain_get_telemetry(&brain, &tel);

ESP_LOGI("G6_TEL", "Q=%.3f | trace_hr=%.2e | trace_pw=%.2e | innov=%.4f | V=%.1f mV",
         tel.model_quality,
         tel.trace_P_hashrate,
         tel.trace_P_power,
         tel.last_innovation,
         tel.best_v);

if (tel.safety_status != G6_SAFETY_OK) {
    ESP_LOGW("G6_TEL", "Safety status: %d", tel.safety_status);
}
```

If you need to wipe the learned model:

```c
g6_brain_reset(&brain);
```

**Pro tip:** Pipe the telemetry struct to ESPHome / Home Assistant / Grafana for long-term tracking. The fields are stable across beta5+ — `last_recommended_voltage` is a back-compat alias for `best_v` and will continue to work for older consumers.

---

**Version:** v1.0.0-beta6 (May 2026)
