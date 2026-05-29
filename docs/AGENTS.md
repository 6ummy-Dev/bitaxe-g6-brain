# AGENTS.md — Engineering Principles & Safety Invariants

**G6 Brain v1.0.0-beta6**

This document defines the engineering rules and safety philosophy for the Bitaxe G6 Brain project.

---

## Core Philosophy

> **"Start safe. Learn. Then optimize."**

The brain must prioritize hardware longevity and stability over marginal hashrate gains.

---

## Current Safety Behavior

The following safety behaviors are **now implemented and enforced**:

1. **Deterministic Safety Priority**: Core overrides (input-range validation, power sanity, ASIC and VR thermal protection, NER back-off, statistical outlier rejection, and P-matrix divergence recovery) run as the final stage of every `g6_brain_update()` tick — after optimization, before returning. The optimizer is never trusted to enforce its own limits.

2. **Two-tier Thermal Protection**:
   - **ASIC die temperature** gates RLS learning updates.
   - **VR regulator temperature** runs only in the safety layer as a proactive + hard constraint on setpoints.

3. **Thermal Protection**: Hard `G6_TEMP_CEILING` with proactive scaling. VR has its own independent ceiling (`G6_VR_TEMP_CEILING`) and proactive margin.

   The two proactive helpers (`g6_safety_proactive_thermal_scale` and `g6_safety_proactive_vr_thermal_scale`) deliberately differ in two ways. Both are intentional and reflect hardware realities, not asymmetries waiting to be reconciled:
   - **Sensor-sentinel guard** — VR's helper checks for the `G6_VR_TEMP_NO_SENSOR` sentinel because VR sensors are optional. The ASIC sensor is mandatory; there is no equivalent "no sensor" mode to defend against, and any negative or out-of-bounds reading is already caught by the upstream fail-closed input-range check before the helper runs.
   - **Tiered response structure** — VR's helper handles both the hard-ceiling tier (≥ ceiling: aggressive frequency + voltage derate) and the proactive tier (> ceiling − margin: voltage-only derate) in a single function, because VR temperature only ever affects setpoints. ASIC's hard-ceiling logic lives in `is_thermal_safe()` and gates RLS *learning* on a separate path (item 2 above); the ASIC proactive helper only handles the proactive tier on the setpoint path. Both helpers do share an identical entry-guard pattern: `brain != NULL`, finiteness of the input temperature, finiteness and positivity of the ceiling, finiteness of the margin.

4. **Fail-Closed Input Range Protection**: BM1370 hard limits (`BM1370_F_MIN`/`MAX`, `BM1370_V_MIN`/`MAX`) and finiteness are strictly enforced on every input. Telemetry violating these bounds is never dismissed via an early return; it triggers an explicit `goto safety_layer` with `G6_SAFETY_INPUT_RANGE` so hardware clamps and the safety helpers still run. `G6_SAFETY_VOLTAGE` is reserved in the enum for a future real VRM-ripple check.

5. **Internal Slew Limiting & Amnesia Protection**: Slew-rate constraints are managed directly inside the tracking loop. Upward slew is completely frozen during *any* safety anomaly (ASIC or VR thermal, input-range violation, power sanity, NER back-off, statistical outlier, or P-matrix recovery) to prevent the controller from advancing blindly on stale optimums.

6. **Numerical Stability**:
   - Full Joseph-form covariance update (symmetric congruence + measurement-noise `k kᵀ` injection + ridge regularization + symmetrization + per-diagonal clamping) — equals the exact RLS posterior covariance
   - Trace constraints and automatic accumulation recovery (see item 11)
   - $O(1)$-per-step Dinkelbach minimization with boundary clamping and the `G6_EFFICIENCY_MIN_HR_THS` floor (exact closed form for interior optima; clamped boundary point otherwise)
   - Adaptive Variable Forgetting Factor

7. **Statistical Outlier Gating**: 3-Sigma innovation variance validation.

8. **Error Rate Handling**: Conservative back-off when NER exceeds `G6_NER_THRESHOLD`.

9. **Control Mode Enforcement**:
   - `G6_MODE_OBSERVE_ONLY`
   - `G6_MODE_RECOMMEND` (safe default)
   - `G6_MODE_AUTO`

10. **NVS Fingerprint Checkpointing**: Background saving of model parameters.

11. **Fail-Closed Execution**: Safety handlers unconditionally run. Per manifesto non-negotiable 3.7 (*"Every safety check executes even on invalid or rejected samples"*), out-of-bounds telemetry, NaN/Inf sensor anomalies, and rejected samples are explicitly routed through the safety layer rather than bypassing it. The only call that returns an error code without running the safety layer is `brain == NULL` — and at that point no brain exists to apply safety to.

12. **P-Matrix Singular Recovery**: When `trace(P) > RLS_TRACE_MAX` (estimator divergence), `g6_brain_recover_cold_start()` zeros both response surfaces (`theta`, `power_theta`), reseeds the covariance diagonals at the cold-start value, and surfaces the event via `G6_SAFETY_P_MATRIX_SINGULAR` plus a single `ESP_LOGW`. Operator-configured fields (`control_mode`, `best_f`, `best_v`, both thermal ceilings, both proactive margins, `dfs_step_mhz`, `ner_threshold`, `use_efficiency_mode`) are explicitly snapshotted and restored across recovery — the brain re-enters cold-start without disturbing the operating point or any runtime tunables.

---

## Efficiency Optimization

True J/TH efficiency tracking uses a discrete secondary power model and an $O(1)$-per-step analytical fraction minimizer (exact closed form for an interior optimum; the clamped boundary point at the box edge). Solver updates are strictly gated by independent model convergence thresholds (`model_quality >= 0.6` and `power_model_quality >= 0.6`) and generated coordinates are bounded to the normalized physical limits to prevent solver overshoot.

---

## Planned Invariants (Phase 2)

- Active thermal slope tracking (`ΔT/dt`)
- Persistent Excitation (ESC Dither)
- PID fan control coupling
- Predictive Voltage Undershoot (P-VUS) protections

---

## Forbidden Patterns

- Never bypass safety priority checks by returning early on bounds validations.
- Never return `ESP_ERR_INVALID_ARG` for bad numeric inputs from `g6_brain_update()`. NaN, Inf, out-of-bounds, and `hr_ths <= 0` all route fail-closed to the safety layer with `G6_SAFETY_INPUT_RANGE`. Only `brain == NULL` returns the error code.
- Never clear tracking matrices without valid re-initialization invariants.
- **Never reset covariance confidence (`P` matrix) without simultaneously zeroing the corresponding response surface (`theta`), to prevent recursive gain explosions.** Use `g6_brain_recover_cold_start()` for any non-trivial reset — it handles operator-state preservation correctly.
- Never use unbound floating-point computations or unregularized updates.
- Never add a new `last_safety_status` value without either setting it from somewhere **or** documenting it as an explicitly *reserved* value in API.md, SAFETY.md, MONITORING.md, and GLOSSARY.md. Undocumented dead enum values mislead operators and waste the taxonomy; the sole sanctioned exception is a value deliberately reserved for backward-compatible integer mapping (e.g. `G6_SAFETY_VOLTAGE`, held for a future VRM-ripple check), which must be called out as reserved everywhere it appears.
- Never read brain state fields that are also exposed via `G6BrainTelemetry` from outside the brain. Use the snapshot. Direct reads break the const contract and create implicit coupling.

---

## See Also

- [`docs/SAFETY.md`](SAFETY.md) — operator-facing description of the safety mechanisms enumerated above, plus the full `G6SafetyStatus` reference table.
- [`docs/API.md`](API.md) — public function and struct definitions.
- [`docs/GLOSSARY.md`](GLOSSARY.md) — terminology used throughout the codebase and docs.
- [`MANIFESTO.md`](../MANIFESTO.md) — project philosophy, non-negotiables (especially 3.7 referenced in item 11).
