# G6 Brain Testing Guide (beta5)

This guide is intended for community members testing the **v1.0.0-beta5** release.

## What's New in beta5

**Round 5 hardening** (originally shipped beta5):

- **Fail-Closed Validation Routing** — Out-of-bounds sensor readings (e.g., impossible frequencies or voltages) no longer result in ignored telemetry. They actively trigger the safety layer to freeze the optimizer and enforce hardware clamps.
- **Slew-Rate Amnesia Protection** — Upward setpoint slew is now strictly frozen during *any* safety anomaly (power sanity, statistical outliers, thermal events) rather than continuing to climb based on stale mathematical optimums.
- **Trace Accumulation Recovery** — If the estimator's covariance matrix trace exceeds safe thresholds during long unbounded learning loops, the brain now safely zeroes the polynomial surface and resets matrix confidence to prevent recursive gain explosions.
- **Dinkelbach Solver Bounding** — The exact analytical efficiency solver now explicitly clamps fractional coordinates, eliminating mathematical overshoot on degraded power surfaces.
- **Configurable ASIC Proactive Thermal Margin** — `G6_TEMP_PROACTIVE_MARGIN` is now a Kconfig option and lives in the state struct, matching the VR margin design.
- **VR Proactive Margin Runtime** — `brain->vr_temp_proactive_margin` replaces the baked-in default macro at all call sites.
- **Safety Status Priority** — Safety helpers are reordered so ASIC thermal, the higher-priority condition, wins on collision when both ASIC and VR conditions fire on the same tick.
- **Belt-and-suspenders NER and thermal gating** — `is_sample_valid()` carries the same NER and thermal predicates as the upstream fast-fail at the top of `g6_brain_update()`. Currently unreachable in normal control flow; retained to guard future refactors that might delete the upstream gates.

**Round 7 safety-model integrity** (full fail-closed contract):

- **Uniform fail-closed input handling** — Every bad numeric input (NaN, Inf, out-of-bounds, `hr_ths <= 0`) now routes to the safety layer with `G6_SAFETY_INPUT_RANGE`. `ESP_ERR_INVALID_ARG` is returned **only** when `brain == NULL`. Previous behavior of returning early on NaN was silently skipping safety ticks — that's fixed.
- **`G6_SAFETY_INPUT_RANGE` added** — new enum value, replaces the prior overload of `G6_SAFETY_VOLTAGE` for input-range violations. `G6_SAFETY_VOLTAGE` is now reserved for a future VRM-ripple check.
- **`G6_SAFETY_P_MATRIX_SINGULAR` wired up** — the trace-recovery path now sets this status so operators see covariance recovery events in telemetry, plus a single `WARN`-level log line.
- **Efficiency mode preserved across recovery** — `g6_brain_recover_cold_start()` now snapshots `use_efficiency_mode` along with the other operator-configured fields.
- **`last_efficiency` no longer reports garbage** on fail-closed paths where `power_w` was unvalidated — the field now retains its last known-good value.

**Pre-v1.0 polish** (telemetry & test maturity):

- **`G6BrainTelemetry` extended** — `best_f`, `best_v`, `model_quality`, `power_model_quality`, `last_efficiency`, `update_count`, `power_update_count` now exposed via the snapshot. `last_recommended_voltage` retained as a back-compat alias for `best_v`.
- **Named constants** — `G6_EFFICIENCY_MIN_HR_THS` (8.0 TH/s) replaces hardcoded literals in the Dinkelbach solver.
- **`g6_brain_self_test()` is now const-correct** — accepts `const G6BrainState *`, source-compatible with prior callers.
- **End-to-end Dinkelbach test** — synthetic surfaces with known-optimal points; verifies the solver actually improves J/TH and respects the quality gate. CI gap exposed: tests are compiled but not run on hardware — see [Reporting Issues](#reporting-issues).

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
- `safety_status` in telemetry — this is the **primary** signal for any safety event. The brain itself logs almost nothing (see `MONITORING.md`); telemetry is where the information lives.
- `update_count` deltas alongside `safety_status` — see `docs/MONITORING.md` for the "distinguishing accepted vs rejected samples" note: a flat `update_count` with `safety_status = G6_SAFETY_OK` means samples are being rejected on non-anomaly quality gates, not that the brain is broken.
- The `Power Outlier Rejected` and `P matrix diverged — cold-start recovery applied` log lines if they appear — those are the two warn-level events the brain emits directly.

## Grounded Testing Scenarios

### 1. VR Thermal Protection
- If your board reports VR temperature, test both the **proactive zone** and **hard ceiling** behavior.
- Verify that only voltage is reduced in the proactive zone, and both voltage + frequency are reduced at the hard ceiling.

### 2. Fail-Closed Validation & Slew Protection
- Intentionally feed the brain an out-of-bounds parameter (e.g., `f_mhz = 1500`).
- Verify that the internal tracking logic halts upward optimization, enforces strict hardware clamps (`best_f` clamped to 950), and reports a `G6_SAFETY_INPUT_RANGE` status via telemetry rather than simply ignoring the frame.
- Repeat with a NaN input (e.g., `power_w = NAN`) and confirm the same fail-closed behavior — `safety_status` reports `G6_SAFETY_INPUT_RANGE`, the safety layer still ran (proactive thermal helpers fired if conditions were met), and `ESP_OK` was returned. Only `brain == NULL` should ever return `ESP_ERR_INVALID_ARG`.

### 3. Safety Status Telemetry
- Trigger various safety conditions (high temperature, high NER, invalid power) and verify that `safety_status` in `g6_brain_get_telemetry()` reflects the exact anomaly.

### 4. Outlier Gating Resilience
- Observe or simulate telemetry anomalies (e.g., a sudden spike in `power_w` while frequency and voltage are unchanged).
- Confirm that severe power anomalies trigger the `Power Outlier Rejected` log line and that `safety_status` reflects `G6_SAFETY_POWER_SANITY`. Note: the hashrate outlier path rejects via `G6_SAFETY_SAMPLE_QUALITY` but does *not* currently emit a dedicated log line — operators must monitor `safety_status` and `update_count` (a rejected sample does not increment the counter).

### 5. P-Matrix Recovery
- The recovery path is hard to trigger by hand on real hardware (requires sustained estimator divergence), but unit tests cover it directly (see `Trace divergence triggers P-matrix recovery and reports P_MATRIX_SINGULAR` in `test_g6_brain.c`).
- On real hardware, if the brain ever recovers, you will see exactly one `WARN`-level log: `"P matrix diverged — cold-start recovery applied"` and `safety_status = G6_SAFETY_P_MATRIX_SINGULAR` on the recovery tick. Operator config (mode, ceilings, `best_f`/`best_v`, efficiency mode) is preserved across recovery — verify by checking those fields after the event.

### 6. Telemetry Snapshot Consistency
- Call `g6_brain_get_telemetry()` after a sequence of valid updates and verify the snapshot fields match what the brain reports through direct field reads (`brain->best_f`, `brain->model_quality`, etc.).
- Confirm `last_recommended_voltage == best_v` — they are intentionally aliased for backward compatibility.

## Reporting Issues

When reporting logs or anomalies, please include:
- ESP32 target variant and runtime version.
- Mode parameters (`RECOMMEND`, `AUTO`, efficiency status).
- Whether VR temperature is being passed or `G6_VR_TEMP_NO_SENSOR` is used.
- Raw log output snippets around the event.
- A `G6BrainTelemetry` snapshot taken near the event if possible — the full struct is the most useful diagnostic.
- Clear description of physical behavior vs expected state output.

### Note on CI test coverage

The Unity test suite is compiled by CI but not executed on hardware or QEMU. See the CHANGELOG for the current test case count per release. Tests that pass compile-check but fail at runtime have slipped through in the past (see B5-BUG-20 in the changelog for a worked example). If you are doing serious testing on real hardware, run the test suite locally with `idf.py -T g6_brain build flash monitor` against a known-good Bitaxe Gamma — that is the most authoritative validation we currently have.

---

**Version:** v1.0.0-beta5 (May 2026)
