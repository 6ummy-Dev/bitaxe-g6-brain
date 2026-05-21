# GLOSSARY.md — Terminology

**G6 Brain v1.0.0-beta5**

This glossary defines key terms used throughout the codebase, documentation, and discussions.

---

## Core Concepts

**G6 Brain** The self-optimizing control module for Bitaxe Gamma (BM1370) that uses Recursive Least Squares (RLS) quadratic modeling to dynamically tune frequency and voltage while enforcing safety constraints.

**RLS (Recursive Least Squares)** An adaptive filtering algorithm that continuously updates a model of hashrate as a quadratic function of frequency and voltage.

**Quadratic Response Surface** The 6-coefficient mathematical model used by the brain to predict optimal operating points.

**Model Quality** A metric (0.0–1.0) indicating how well the current RLS model fits observed data.

**Cold Start** The initial phase after power-on or reset when the brain has insufficient data and operates conservatively. *This state is also dynamically triggered by Trace Accumulation Recovery if the estimator diverges, safely wiping the polynomial surface and resetting matrix confidence.*

**Warm Start / NVS Fingerprint** The learned RLS coefficients (`theta`) + full covariance matrix (`P`) + power model state, stored per physical chip in NVS.

**Stabilized Covariance Update** A Joseph-style congruence transform applied to the P-matrix on each RLS step, followed by ridge regularization, symmetrization, and per-diagonal clamping. The combination preserves symmetry and positive-definiteness of P under floating-point arithmetic. Not the full classical Joseph form (which would include a measurement-noise injection term).

**Trace Accumulation Recovery** A safety mechanism that automatically arrests covariance matrix divergence during unbounded learning loops. It zeroes the active polynomial surface and resets matrix confidence to prevent recursive gain explosions, forcing a safe cold-start.

**Statistical Outlier Gating** A data validation layer that calculates expected innovation variance and rejects samples exceeding a 3-sigma statistical bound.

---

## Safety & Thermal (beta5)

**Two-tier Thermal Safety** The beta5 architecture that treats ASIC die temperature and VR regulator temperature differently:
- ASIC temperature gates learning.
- VR temperature only constrains setpoints in the safety layer.

**VR Temperature (`vr_temp_c`)** Voltage regulator temperature passed to `g6_brain_update()`. Use `G6_VR_TEMP_NO_SENSOR` (`-1.0f`) when no VR sensor is available.

**Proactive Zone** The temperature range below the hard ceiling where the brain begins gentle voltage reduction as an early warning.

**Hard Ceiling** The temperature at which aggressive setpoint reduction is applied (both voltage and frequency for VR).

---

## Safety State & Status Codes

**Sample Validation Gates** Four independent checks the brain applies before accepting a telemetry frame into the RLS update: minimum share count, NER threshold, ASIC thermal safety, and significant innovation in the covariance projection. A frame failing any gate is routed to the safety layer without updating the model. (No internal state machine — previous versions of this glossary referenced a `BrainSampleState` type that has been removed.)

**NER (Nonce Error Rate)** Hardware error rate reported by the BM1370. Used as a key input to the safety logic.

**G6ControlMode** Runtime control modes:
- `G6_MODE_OBSERVE_ONLY`
- `G6_MODE_RECOMMEND` (safe default)
- `G6_MODE_AUTO`

**Fail-Closed Execution** The architectural principle where bad telemetry (out-of-bounds, NaN, infinite, or otherwise unusable) does not result in an early-return error code, but instead forces a jump directly into the safety layer with an appropriate `last_safety_status` so hardware clamps run and upward tracking is suspended. Enforces manifesto non-negotiable 3.7.

**last_safety_status** Internal field tracking the most recent safety condition (or `G6_SAFETY_OK` if none). Exposed via the telemetry snapshot. See `G6SafetyStatus` below for the full taxonomy.

**G6SafetyStatus** Enum of safety conditions reported via `last_safety_status`. Full reference lives in `docs/SAFETY.md`. Key values:
- `G6_SAFETY_INPUT_RANGE` — input failed validation (NaN, Inf, `hr_ths <= 0`, or out-of-bounds `f_mhz`/`v_mv`).
- `G6_SAFETY_P_MATRIX_SINGULAR` — covariance trace diverged; brain auto-recovered into a fresh cold-start while preserving operator config. Logged as `"P matrix diverged — cold-start recovery applied"`.
- `G6_SAFETY_VOLTAGE` — reserved for a future VRM-ripple check; not currently set by any code path.

**P-Matrix Singular Recovery** The automatic recovery flow triggered when `trace(P) > RLS_TRACE_MAX`. Zeros both `theta` arrays and re-seeds the P diagonals at `1e5`, then restores operator-set fields (mode, ceilings, margins, efficiency mode, etc.). The brain re-enters cold-start. See `SAFETY.md` item 5.

---

## Telemetry

**G6BrainTelemetry** A snapshot struct populated by `g6_brain_get_telemetry()`. Provides a consistent point-in-time view of brain state for monitoring and logging. Fields cover the model internals (`theta`, `trace_P`), operating point (`best_f`, `best_v`), model quality on both estimators, last efficiency, update counts, and the current safety status. See `docs/API.md` for the full field list.

**last_efficiency** The most recent `power_w / hr_ths` ratio (W/TH). Only updated when `power_w` is within sanity bounds — on fail-closed paths the field retains its last known-good value rather than reporting a garbage ratio.

**model_quality / power_model_quality** Independent fit confidence metrics (0.0–1.0) for the hashrate and power RLS models. Both must be at or above `0.6` for the Dinkelbach J/TH solver to run. Exposed via the telemetry snapshot.

---

## Configuration & Limits

**G6_TEMP_CEILING** Hard thermal ceiling for the ASIC (°C).

**G6_VR_TEMP_CEILING** Hard thermal ceiling for the voltage regulator (°C).

**G6_VR_TEMP_PROACTIVE_MARGIN** Temperature margin below the VR ceiling where proactive voltage reduction begins.

**G6_EFFICIENCY_MIN_HR_THS** Minimum predicted hashrate (`8.0` TH/s) below which the Dinkelbach J/TH solver skips a candidate point. Prevents near-zero division and guards against optimizing in regions where the power model has no physical meaning. Compile-time macro, not a Kconfig option.

**Slew Rate** The internally controlled rate of change for frequency and voltage. Upward slew is frozen during safety anomalies.

---

**Last updated:** May 2026 (v1.0.0-beta5)
