# G6 Brain Testing Guide (beta4)

This guide is intended for community members testing the **v1.0.0-beta4** release.

## What's New in beta4

- **Two-tier Thermal Safety** — Separate handling for ASIC die temperature and voltage regulator (VR) temperature.
- **Improved Safety Telemetry** — `safety_status` now meaningfully reports triggered safety conditions.
- **Power Validation & Outlier Logging** — Fail-closed power handling and symmetric power outlier logging.
- **Timing Fix** — Corrected tick-to-millisecond conversion in sample windows.
- Continued hardening of the safety layer and CI reliability.

## Recommended Starting Point

- Start in **`G6_MODE_RECOMMEND`** (the default and safest environment).
- Do **not** switch to `AUTO` mode until you have monitored tracking logs for several hours.
- Use `G6_MODE_OBSERVE_ONLY` if you only want background metrics without tuning suggestions.

If your hardware provides VR temperature (`vrTemp`), pass it to `g6_brain_update()` so the new two-tier thermal protection can activate. Use `G6_VR_TEMP_NO_SENSOR` (`-1.0f`) otherwise.

## What to Monitor

When testing, pay attention to:
- Frequency and voltage targets suggested by the brain.
- Temperature behavior (both ASIC and VR when available) and post-optimization thermal scaling actions.
- `model_quality` and `power_model_quality` values.
- `safety_status` in telemetry.
- Operational logs via serial terminal (look for thermal, VR thermal, power sanity, and outlier messages).

## Grounded Testing Scenarios

### 1. VR Thermal Protection
- If your board reports VR temperature, test both the **proactive zone** and **hard ceiling** behavior.
- Verify that only voltage is reduced in the proactive zone, and both voltage + frequency are reduced at the hard ceiling.

### 2. Safety Status Telemetry
- Trigger various safety conditions (high temperature, high NER, invalid power) and verify that `safety_status` in `g6_brain_get_telemetry()` reflects the last triggered condition.

### 3. Outlier Gating Resilience
- Observe or simulate telemetry anomalies.
- Confirm that severe anomalies trigger `HR Outlier Rejected` or `Power Outlier Rejected` logs instead of corrupting the model.

## Reporting Issues

When reporting logs or anomalies, please include:
- ESP32 target variant and runtime version.
- Mode parameters (`RECOMMEND`, `AUTO`, efficiency status).
- Whether VR temperature is being passed or `G6_VR_TEMP_NO_SENSOR` is used.
- Raw log output snippets around the event.
- Clear description of physical behavior vs expected state output.

---

**Version:** v1.0.0-beta4 (May 2026)
