# G6 Brain Testing Guide (beta3)

This guide is intended for community members testing the **v1.0.0-beta3** release.

## What's New in beta3

- **Dinkelbach-based J/TH optimizer** — replaces the previous brute-force grid search for efficiency mode.
- **`power_model_quality` monitoring** — gates J/TH optimization until the power surface model is reliable.
- **Joseph Form Covariance Update** — structural math implementation to prevent tracking matrix divergence.
- **3-Sigma Statistical Outlier Gating** — filtering layer designed to automatically block corrupted sensor frames.
- **Internal Slew-Rate Limiting** — embedded loop constraints designed to regulate voltage and frequency steps relative to the live tracking state.

Efficiency changes are **opt-in** via `G6_ENABLE_EFFICIENCY_MODE`.

## Recommended Starting Point

- Start in **`G6_MODE_RECOMMEND`** (the default and safest environment).
- Do **not** switch to `AUTO` mode until you have monitored tracking logs for several hours.
- Use `G6_MODE_OBSERVE_ONLY` if you only want background metrics without tuning suggestions.

## What to Monitor

When testing, pay attention to:
- Frequency and voltage targets suggested by the brain.
- Temperature behavior and post-optimization thermal scaling actions.
- `model_quality` and `power_model_quality` values.
- Operational logs via serial terminal (specifically look for any `Outlier Rejected` logs if telemetry bus noise occurs).

## Grounded Testing Scenarios

### 1. Verification of Internal Slew Limiting
- Toggle the controller to `G6_MODE_AUTO` once metrics stabilize.
- Observe step changes when the optimization target adjusts. Frequency should step incrementally based on the `G6_DFS_STEP_MHZ` boundary rather than jumping instantly to the global maximum coordinate.

### 2. Verification of Outlier Gate Resilience
- Simulate or observe telemetry sensor anomalies (e.g., brief hardware read drops or bus interference glitches).
- Check that severe tracking anomalies trigger an `HR Outlier Rejected` or `Power Outlier Rejected` serial log log instead of twisting the tracking surface or collapsing `model_quality`.

## Reporting Issues

When reporting logs or anomalies, please include:
- ESP32 target variant and runtime version.
- Mode parameters (`RECOMMEND`, `AUTO`, efficiency status).
- Raw log output snippets around the event or unexpected step changes.
- Clear description of physical behavior vs expected state output.
