# G6 Brain Testing Guide (beta5)

This guide is intended for community members testing the **v1.0.0-beta5** release.

## What's New in beta5

- **Configurable ASIC Proactive Thermal Margin** — `G6_TEMP_PROACTIVE_MARGIN` is now a Kconfig option and lives in the state struct (`brain->temp_proactive_margin`), matching the VR margin design.
- **VR Proactive Margin Properly Honored at Runtime** — `brain->vr_temp_proactive_margin` replaces the baked-in default macro at all call sites. Runtime mutation of the field now correctly affects the proactive zone.
- **Safety Status Priority** — Safety helpers are reordered so ASIC thermal wins on collision when both ASIC and VR conditions fire on the same tick.
- **NER Defense-in-Depth** — `is_sample_valid()` now gates on NER as a redundant safety check.
- **NER Backoff Floor Clamps** — `g6_asic_error_handle_non_blocking` uses `fmaxf` floor clamping, consistent with the other safety helpers.
- **NVS Blob-Size Mismatch Logging** — Corrupt or mismatched NVS blobs are now logged and erased rather than silently dropped.
- **Code Quality** — `g6_brain_set_defaults()` extracted; `g6_brain_init` and `g6_brain_reset` no longer duplicate 50 lines of default-setting logic.

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

**Version:** v1.0.0-beta5 (May 2026)
