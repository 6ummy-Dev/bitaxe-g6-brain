# G6 Brain Safety & Unhappy-Path Engineering — v1.0.0-beta5

**This is not a happy-path optimizer.** The G6 Brain is deliberately engineered to **fail safe** under real-world conditions on Bitaxe Gamma hardware.

---

## Core Safety Philosophy

> **"Start safe. Learn. Then optimize. ⚡"**

The brain must prioritize hardware longevity and stability over marginal hashrate gains. Every decision passes through multiple protective layers before any frequency or voltage change is recommended.

---

## Implemented Safety Mechanisms (Current — beta5)

The following safety behaviors are **fully active**:

1. **Two-tier Thermal Protection** - **ASIC temperature** (`temp_c`): Gates RLS learning. Samples above the ceiling are discarded to prevent training on thermally stressed data.
   - **VR Regulator temperature** (`vr_temp_c`): Runs exclusively in the safety layer. Provides proactive voltage reduction and hard ceiling enforcement. Pass `G6_VR_TEMP_NO_SENSOR` (`-1.0f`) when no VR sensor is available.

2. **Fail-Closed Input Range Protection** Every numeric input is validated against BM1370 hardware bounds (400–950 MHz, 1050–1350 mV) and finiteness (NaN/Inf rejected). Out-of-bounds or non-finite telemetry does not return an error code; it triggers a `G6_SAFETY_INPUT_RANGE` status and routes directly to the fail-closed safety layer to enforce hardware clamps and suspend upward tracking. Per manifesto non-negotiable 3.7: *every safety check executes even on invalid or rejected samples*. `G6_SAFETY_VOLTAGE` is reserved in the enum for a future real VRM-ripple check and is not currently set by any code path.

3. **Sample Quality Gating** Before any RLS update, the brain enforces four independent gates: minimum share count (`MIN_SHARE_COUNT = 20`), NER below `ner_threshold`, ASIC die temperature below the hard ceiling, and significant innovation in the covariance projection (`xPx > RLS_INNOVATION_THRESHOLD`). A sample failing any gate is routed to the safety layer without updating the model.

4. **Stabilized Covariance Update** The P-matrix update uses a Joseph-style congruence transform followed by ridge regularization, symmetrization, and per-diagonal clamping (`RLS_P_CLAMP_MIN`..`RLS_P_CLAMP_MAX`). The combination keeps P symmetric and positive-definite under floating-point arithmetic without requiring the full Joseph form's measurement-noise injection term.

5. **Trace Accumulation Recovery** If the covariance trace exceeds `RLS_TRACE_MAX` due to unbounded learning, the brain runs `g6_brain_recover_cold_start()` to safely zero both polynomial surfaces (`theta`, `power_theta`), reset matrix confidence to the cold-start diagonal, and surface the event:
   - Sets `last_safety_status = G6_SAFETY_P_MATRIX_SINGULAR` so operators monitoring telemetry see the recovery happened (the status may still be overwritten by a more urgent thermal condition firing on the same tick — that priority is intentional).
   - Emits a single `ESP_LOGW`: `"P matrix diverged — cold-start recovery applied"`.
   - Preserves operator-configured runtime state across the recovery: `control_mode`, `best_f`, `best_v`, `ner_threshold`, both thermal ceilings, both proactive margins, `dfs_step_mhz`, and `use_efficiency_mode`. The recovery zeroes the learning state without disturbing the operating point or any tunables the operator set.
   - Suppresses NVS save for the next interval (the freshly-zeroed model isn't worth persisting).

6. **Statistical Outlier Gating** 3-Sigma innovation variance validation. Corrupted sensor frames are rejected before updating the model.

7. **Power Sanity Check** Rejects physically impossible power values and routes them directly into the fail-closed safety layer, triggering a `G6_SAFETY_POWER_SANITY` freeze.

8. **Control Mode Enforcement**
   - `G6_MODE_OBSERVE_ONLY`: Safety only — no `best_f` / `best_v` mutation
   - `G6_MODE_RECOMMEND` (default): Computes optimal but never mutates setpoints
   - `G6_MODE_AUTO`: Full optimizer with internal slew-rate limiting

9. **Internal Slew Limiting & Slew Amnesia Protection** Slew-rate logic is embedded directly within the tracking update loop. Upward slew is strictly frozen if *any* safety anomaly is active (thermal, VR thermal, input-range violation, power sanity, NER back-off, statistical outlier, or P-matrix recovery), preventing the controller from stepping targets upward based on stale mathematical optimums during unstable physical conditions.

10. **Analytical J/TH Solver Bounding** The exact $O(1)$ efficiency fractional solver strictly clamps generated normalized coordinates to prevent mathematical overshoot or bounding-box stalls when traversing degraded power surfaces. 

11. **Model Quality Gates** The J/TH efficiency optimizer is protected by both `model_quality >= 0.6` **and** `power_model_quality >= 0.6`.

12. **NVS Warm-Start** Full theta + P-matrix + power model auto-saved. Survives power cycles.

---

## Safety Status Reference

The `last_safety_status` field (exposed via `G6BrainTelemetry.safety_status`) reports the most recent safety condition observed during the current `g6_brain_update()` tick. Values, all defined in `g6_brain.h`:

| Status | Set when |
| --- | --- |
| `G6_SAFETY_OK` | Sample accepted into the RLS update; no anomaly observed this tick. |
| `G6_SAFETY_THERMAL` | ASIC die at or above the hard ceiling, or in the proactive zone (within `G6_TEMP_PROACTIVE_MARGIN` of the ceiling). |
| `G6_SAFETY_VR_THERMAL` | VR regulator at or near its ceiling. Triggers voltage step-back; both voltage and frequency step back at the hard ceiling. |
| `G6_SAFETY_VOLTAGE` | Reserved for a future VRM-ripple check. Currently never set by any code path. Operators can ignore this value. |
| `G6_SAFETY_POWER_SANITY` | `power_w` outside the physically plausible range (`< 0` or `> 100 W`), or a power-model statistical outlier was rejected. |
| `G6_SAFETY_NER_BACKOFF` | Nonce error rate exceeded `ner_threshold`. Triggers a conservative ~8% frequency back-off and re-enters cold-start to re-learn under the new conditions. |
| `G6_SAFETY_SAMPLE_QUALITY` | Hashrate-model statistical outlier rejected by the 3-sigma gate. |
| `G6_SAFETY_P_MATRIX_SINGULAR` | Covariance trace diverged and the brain ran auto-recovery (see item 5 above). |
| `G6_SAFETY_INPUT_RANGE` | Input telemetry failed validation: non-finite (NaN/Inf), `hr_ths <= 0`, or `f_mhz`/`v_mv` outside BM1370 hardware bounds. |

**Same-tick priority:** if multiple conditions fire on one tick, the helpers run last in the safety layer (proactive VR thermal, then proactive ASIC thermal) and may overwrite earlier statuses. If both ASIC and VR thermal conditions fire on the same tick, ASIC wins. The recovery status (`P_MATRIX_SINGULAR`) can be overwritten by a same-tick thermal condition — that is intentional, since thermal is the more urgent operator alert.

---

## When the Brain Refuses to Tune

The brain stays conservative and suspends upward tracking when:
- Temperature is near/at ceiling (ASIC or VR)
- Error rate is elevated (`G6_SAFETY_NER_BACKOFF`)
- `model_quality` or `power_model_quality` is low
- A sensor anomaly, power sanity violation, or statistical outlier is detected
- Control mode is not `AUTO`
- The sample has not passed quality gates

---

## Recommended Monitoring

Monitor these values in production:
- `model_quality` and `power_model_quality`
- `control_mode`
- `best_f` / `best_v`
- `temp_c` and `vr_temp_c` (when available)
- NVS auto-save messages
- `g6_brain_get_cov_condition()`
- `safety_status` from telemetry

---

## File References

- Core logic: `components/g6_brain/g6_brain.c`
- Public API & constants: `components/g6_brain/g6_brain.h`
- Engineering principles: `AGENTS.md`

---

**Version:** v1.0.0-beta5 (May 2026)  
**Philosophy:** Start safe. Learn. Then optimize. ⚡
