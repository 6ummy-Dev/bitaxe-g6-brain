# G6 Brain Safety & Unhappy-Path Engineering — v1.0.0-beta7.5

**This is not a happy-path optimizer.** The G6 Brain is deliberately engineered to **fail safe** under real-world conditions on Bitaxe Gamma hardware.

---

## Core Safety Philosophy

> **"Start safe. Learn. Then optimize. ⚡"**

The brain must prioritize hardware longevity and stability over marginal hashrate gains. Every decision passes through multiple protective layers before any frequency or voltage change is recommended.

---

## Implemented Safety Mechanisms

The following safety behaviors are **fully active**:

1. **Two-tier Thermal Protection** - **ASIC temperature** (`temp_c`): Gates RLS learning. Samples above the ceiling are discarded to prevent training on thermally stressed data.
   - **VR Regulator temperature** (`vr_temp_c`): Runs exclusively in the safety layer. Provides proactive voltage reduction and hard ceiling enforcement. Pass `G6_VR_TEMP_NO_SENSOR` (`-1.0f`) when no VR sensor is available.

2. **Fail-Closed Input Range Protection** Every numeric input is validated against BM1370 hardware bounds (400–950 MHz, 1050–1350 mV) and finiteness (NaN/Inf rejected). Out-of-bounds or non-finite telemetry does not return an error code; it triggers a `G6_SAFETY_INPUT_RANGE` status and routes directly to the fail-closed safety layer to enforce hardware clamps and suspend upward tracking. Per manifesto non-negotiable 3.7: *every safety check executes even on invalid or rejected samples*. `G6_SAFETY_VOLTAGE` is reserved in the enum for a future real VRM-ripple check and is not currently set by any code path.

3. **Sample Quality Gating** Before any RLS update, the brain enforces four independent gates: minimum share count (`MIN_SHARE_COUNT = 20`), NER below `ner_threshold`, ASIC die temperature below the hard ceiling, and significant innovation in the covariance projection (`xPx > RLS_INNOVATION_THRESHOLD`). A sample failing any gate is routed to the safety layer without updating the model. The thermal and NER gates correspond to true safety events and set their respective statuses; the share-count and innovation gates are not safety events and leave `last_safety_status = G6_SAFETY_OK` (see the Safety Status Reference below).

4. **Stabilized Covariance Update** The P-matrix update uses the full Joseph form — a symmetric congruence transform plus the measurement-noise (`k kᵀ`, R=1) injection term — followed by ridge regularization, symmetrization, and per-diagonal clamping (`RLS_P_CLAMP_MIN`..`RLS_P_CLAMP_MAX`). Including the injection term makes the update the exact RLS posterior covariance (an earlier revision computed only `(M P Mᵀ)/λ` and omitted the term, which biased P downward and shrank the gain faster than RLS prescribes); the congruence-plus-clamp structure keeps P symmetric and positive-definite under floating-point arithmetic.

5. **Covariance Divergence Recovery** The brain runs `g6_brain_recover_cold_start()` on either of two covariance-divergence signatures — (a) the trace exceeds `RLS_TRACE_MAX` (unbounded learning), or (b) the predicted variance `xᵀPx` (or `xᵀ·power_Px`) goes strictly negative, the unambiguous mark of a covariance that has lost positive-definiteness. Case (b) arises at a fixed operating point, where the six-term quadratic basis is unidentifiable and the diagonal-only clamp cannot keep the matrix PSD; the trace check alone misses it because the trace stays bounded. Either way the recovery zeros both polynomial surfaces (`theta`, `power_theta`), resets matrix confidence to the cold-start diagonal, and surfaces the event:
   - Sets `last_safety_status = G6_SAFETY_P_MATRIX_SINGULAR` so operators monitoring telemetry see the recovery happened (the status may still be overwritten by a more urgent thermal condition firing on the same tick — that priority is intentional).
   - Emits a single `ESP_LOGW`: `"P matrix diverged — cold-start recovery applied"`.
   - Preserves operator-configured runtime state across the recovery: `control_mode`, `best_f`, `best_v`, `ner_threshold`, both thermal ceilings, both proactive margins, `dfs_step_mhz`, and `use_efficiency_mode`. The recovery zeroes the learning state without disturbing the operating point or any tunables the operator set.
   - Note: at a genuinely fixed operating point case (b) can recur periodically — this is expected and correct (the surface is unidentifiable without f/v variation); the brain re-cold-starts and reports it rather than silently freezing. A little operating-point movement (even a few MHz/mV) eliminates it.
   - Suppresses NVS save for the next interval (the freshly-zeroed model isn't worth persisting).

6. **Statistical Outlier Gating** 3-Sigma innovation variance validation. Corrupted sensor frames are rejected before updating the model.

7. **Power Sanity Check** Rejects physically impossible power values and routes them directly into the fail-closed safety layer, triggering a `G6_SAFETY_POWER_SANITY` freeze.

8. **Control Mode Enforcement**
   - `G6_MODE_OBSERVE_ONLY`: the optimizer/slew path never moves `best_f` / `best_v`. (The proactive thermal/VR derate still applies in every mode as a fail-safe.)
   - `G6_MODE_RECOMMEND` (default): computes optimal setpoints; the optimizer/slew path does not move `best_f` / `best_v`. (Proactive thermal/VR derate still applies in every mode.)
   - `G6_MODE_AUTO`: Full optimizer with internal slew-rate limiting

9. **Internal Slew Limiting & Slew Amnesia Protection** Slew-rate logic is embedded directly within the tracking update loop. The optimizer's slew step is suspended in **both directions** while *any* safety anomaly is active (thermal, VR thermal, input-range violation, power sanity, NER back-off, statistical outlier, or P-matrix recovery): the controller neither steps targets upward on stale mathematical optimums nor chases a lower optimum mid-anomaly — while an anomaly is active, the only setpoint movement comes from the safety derates themselves.

10. **Analytical J/TH Solver Bounding** The analytical efficiency fractional solver runs in $O(1)$ per outer step (no iterative line search): for an *interior* optimum the Dinkelbach inner step is an exact closed-form minimizer, while at the bounding box the result is the clamped boundary point. Generated normalized coordinates are strictly clamped to the physical limits to prevent mathematical overshoot or bounding-box stalls when traversing degraded power surfaces. 

11. **Model Quality Gates** The J/TH efficiency optimizer is protected by both `model_quality >= 0.6` **and** `power_model_quality >= 0.6`.

12. **NVS Warm-Start** Full theta + P-matrix + power model auto-saved. Survives power cycles.

---

## Sensor Sanity — Integrator Responsibility

The brain validates **finiteness** on every input and **hardware bounds** on `f_mhz` and `v_mv` specifically. Beyond that, the brain trusts that finite values within the C `float` domain represent real readings. It does **not** attempt to detect stuck-low or stuck-high sensor failures on the temperature (`temp_c`, `vr_temp_c`), error rate (`err_pct`), or hashrate (`hr_ths`) channels. Concrete consequence: an ASIC temperature sensor stuck reporting an implausibly low value (e.g. `-50°C`) passes the input gate, `is_thermal_safe` returns true, and the brain happily trains on the sample as if the chip were cold and healthy. A `err_pct` channel stuck at a negative value similarly never trips the NER gate.

This is a deliberate scope boundary, not an oversight. Sensor health monitoring belongs in the integrator's telemetry layer where domain knowledge of the specific sensor hardware (INA219/INA260, BM1370 die thermal diode, etc.) lives. The brain's contract is "given truthful telemetry, optimize safely"; sanity-checking the truthfulness of that telemetry is upstream.

**Recommended upstream checks:**
- `temp_c` and `vr_temp_c` within a plausible operating range (e.g. 0–120 °C).
- `err_pct` within `[0, 100]`.
- `hr_ths` within an order of magnitude of the expected hardware capability.
- Sensor freshness (timestamp deltas) — a stale-but-finite reading is invisible to the brain.

---

## Safety Status Reference

The `last_safety_status` field (exposed via `G6BrainTelemetry.safety_status`) reports the most recent safety condition observed during the current `g6_brain_update()` tick. Values, all defined in `g6_brain.h`:

| Status | Set when |
| --- | --- |
| `G6_SAFETY_OK` | No anomaly observed this tick. The steady-state value during normal operation. Also reported on non-anomaly sample rejections (e.g. `share_count < MIN_SHARE_COUNT`, or the sample lies too close to existing training data to provide significant innovation) — both are normal during startup and after pool changes. Operators distinguish "accepted" from "rejected for non-anomaly reasons" by watching `update_count` deltas. |
| `G6_SAFETY_THERMAL` | ASIC die at or above the hard ceiling, or in the proactive zone (within `G6_TEMP_PROACTIVE_MARGIN` of the ceiling). |
| `G6_SAFETY_VR_THERMAL` | VR regulator at or near its ceiling. Triggers voltage step-back; both voltage and frequency step back at the hard ceiling. |
| `G6_SAFETY_VOLTAGE` | Reserved for a future VRM-ripple check. Currently never set by any code path. Operators can ignore this value. |
| `G6_SAFETY_POWER_SANITY` | `power_w` outside the physically plausible range (`< 0` or `> 100 W`), or a power-model statistical outlier was rejected. |
| `G6_SAFETY_NER_BACKOFF` | Nonce error rate exceeded `ner_threshold`. The brain applies a conservative ~8% frequency back-off, forces `model_quality` down to 0.25 so quality-gated features (J/TH solver) re-arm only after observable recovery, and momentarily re-enters cold-start so the next RLS update runs at the conservative learning rate (`lambda = 0.985`). If `update_count > 25` at the time of the event, the cold-start flag clears on the very next clean update — the conservative learning rate is in effect for that one update. `model_quality` is an instantaneous fit metric (not an EMA), so the 0.25 value lasts only until the next accepted update recomputes it — on a well-fit model a single clean sample can lift it back over the 0.6 solver gate. |
| `G6_SAFETY_SAMPLE_QUALITY` | Hashrate-model statistical outlier rejected by the 3-sigma gate. |
| `G6_SAFETY_P_MATRIX_SINGULAR` | Covariance diverged — either the trace exceeded `RLS_TRACE_MAX` or the predicted variance `xᵀPx` went negative (non-PSD, typical at a fixed operating point) — and the brain ran auto-recovery (see item 5 above). |
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
- `g6_brain_get_cov_condition()` (certified bound on the covariance condition number — see `API.md` for the two-path semantics)
- `safety_status` from telemetry
- `update_count` deltas (the canonical signal for "is the brain actually accepting samples" — a rising `update_count` with `safety_status = G6_SAFETY_OK` is the steady-state happy path; a flat `update_count` with `safety_status = G6_SAFETY_OK` means samples are being rejected on the non-anomaly quality gates)

---

## File References

- Core logic: `components/g6_brain/g6_brain.c`
- Public API & constants: `components/g6_brain/g6_brain.h`
- Engineering principles: `AGENTS.md`

---

**Version:** v1.0.0-beta7.5 (June 2026)  
**Philosophy:** Start safe. Learn. Then optimize. ⚡
