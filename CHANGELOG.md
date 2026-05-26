# Changelog

All notable changes to the Bitaxe G6 Brain are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [Unreleased]

_Next release will go here._

---

## [1.0.0-beta6] — 2026-05-25

Pre-field-test polish. Test coverage for previously-implicit contracts, documentation reconciliation, one targeted hardening of the ASIC proactive thermal helper, and one mild telemetry-surface expansion (`last_update_timestamp`).

### Added
- Test pinning the low-share-count rejection contract (`share_count < MIN_SHARE_COUNT` → `ESP_OK`, `update_count` unchanged, `last_safety_status = G6_SAFETY_OK`).
- Test pinning the insignificant-innovation rejection contract (`xPx < RLS_INNOVATION_THRESHOLD` → same behavior as above).
- Test pinning `G6_SAFETY_POWER_SANITY` routing for `power_w` out of `[0, 100]` (both positive and negative cases).
- Test pinning the ASIC proactive helper's ceiling sanity guard (`temp_ceiling = NaN` and `temp_ceiling = 0.0f` cases — helper body does not run; setpoints unchanged; `last_safety_status = G6_SAFETY_THERMAL` from the upstream `is_thermal_safe()` gate in both cases).
- Test pinning the `last_update_timestamp` ↔ `update_count` pairing contract (both advance on accepted samples, neither moves on rejected samples — exercised across low-share, input-range, and final-accepted paths with the full `OK → OK → INPUT_RANGE → OK` status trajectory pinned).
- `last_safety_status` assertions on four existing tests (`G6_SAFETY_THERMAL`, `G6_SAFETY_SAMPLE_QUALITY`, `G6_SAFETY_VR_THERMAL` × 2) to lock in the resulting status, not just the rejection.
- `G6BrainTelemetry.last_update_timestamp` — FreeRTOS tick of the most recent accepted RLS update, paired with `update_count`. Internal field already existed; this exposes it via the snapshot and tightens its assignment point so the pairing contract holds.
- `docs/SAFETY.md`: "Sensor Sanity — Integrator Responsibility" section documenting which input channels are finiteness-checked vs hardware-bounded.
- `docs/API.md`: "Sensor Sanity" subsection under `g6_brain_update()` with the same disclosure. `last_update_timestamp` row added to the telemetry table with semantics and rollover note.
- `docs/MONITORING.md`: "Distinguishing accepted vs rejected samples" guidance using `update_count` deltas. `last_update_timestamp` row added to the telemetry table.
- `docs/GLOSSARY.md`: full `G6SafetyStatus` enum table, "Non-Anomaly Sample Rejection" entry, `update_count` entry, `last_update_timestamp` entry.
- `docs/AGENTS.md`: explicit note under "Thermal Protection" documenting the two intentional asymmetries between the ASIC and VR proactive helpers (sensor-sentinel guard, tiered response structure) and the entry-guard pattern that is now shared by both.

### Changed
- `g6_safety_proactive_thermal_scale()` now guards on `isfinite(temp_ceiling) && temp_ceiling > 0.0f` and `isfinite(temp_proactive_margin)` before evaluating the proactive threshold — mirrors the VR helper's pattern. Defense-in-depth against a future refactor corrupting either field; no behavior change in normal operation because both are set from Kconfig-defined defaults and validated at init/reset. Closes B5-NIT-16.
- `last_update_timestamp` assignment moved from "after thermal/NER gates pass" to "immediately after `update_count++`" so the field's contract is unambiguous: it tracks accepted RLS updates, not all samples that passed safety gates.
- `G6_SAFETY_OK` description across `SAFETY.md`, `API.md`, `GLOSSARY.md`, `MONITORING.md`: now accurately reports "no anomaly observed this tick" rather than "sample accepted into the RLS update" — the status is also set on non-anomaly sample rejections.
- `G6_SAFETY_NER_BACKOFF` description: corrected to reflect actual duration ("momentarily re-enters cold-start so the next RLS update runs at the conservative learning rate"); previously oversold as a sustained re-learning window.
- `README.md`: replaced three-round "What's New in beta5" section with a single timeless Features list; trimmed Status table to three rows; QA count and test count references deflected to CHANGELOG; "per-round QA log" wording removed.
- `docs/README.md`: "eight QA rounds" and "nine QA review cycles" claims removed (round-count forensics live in git, not the docs).
- `docs/TESTING.md`: round-numbered "What's New in beta5" section replaced with a tester-focused "What to Test" summary pointing at `CHANGELOG.md` for details; stale "the new two-tier thermal protection" qualifier dropped.
- Test suite header comment refreshed to describe full coverage scope including non-anomaly rejection paths.

---

## [1.0.0-beta5] — 2026-05-22

Safety-model integrity, full fail-closed contract, telemetry expansion, and pre-tag documentation reconciliation. Tagged as release candidate.

### Added
- `G6_SAFETY_INPUT_RANGE` enum value for input-validation failures (replaces the prior `G6_SAFETY_VOLTAGE` overload).
- `G6_SAFETY_P_MATRIX_SINGULAR` is now wired up: the covariance trace-recovery path sets it and emits `WARN: "P matrix diverged — cold-start recovery applied"`.
- `G6_TEMP_PROACTIVE_MARGIN` Kconfig option and runtime field (mirrors the VR margin design).
- `G6_EFFICIENCY_MIN_HR_THS` named constant replacing hardcoded `8.0f` literals in the Dinkelbach solver.
- `G6BrainTelemetry` extended with `best_f`, `best_v`, `model_quality`, `power_model_quality`, `last_efficiency`, `update_count`, `power_update_count`. `last_recommended_voltage` retained as a back-compat alias for `best_v`.
- NVS schema versioning with bad-blob auto-erase and a `WARN` log line on size/version mismatch.
- End-to-end Dinkelbach test verifying the J/TH solver actually improves efficiency on synthetic surfaces and respects the model-quality gate.
- Tests for VR thermal sentinel/proactive/hard-ceiling behavior, P-matrix recovery + operator-state preservation, runtime margin changes, fail-closed routing on NaN/Inf/out-of-bounds inputs, and the const-correct self-test path.

### Changed
- Fail-closed contract is now uniform: every bad numeric input (NaN, Inf, out-of-bounds, `hr_ths <= 0`) routes to the safety layer with `G6_SAFETY_INPUT_RANGE`. `ESP_ERR_INVALID_ARG` returns **only** when `brain == NULL`.
- Slew-rate amnesia: upward slew is frozen during *any* active safety condition (thermal, VR thermal, input-range, power sanity, NER backoff, statistical outlier, P-matrix recovery).
- Dinkelbach solver clamps normalized fractional coordinates to physical bounds to prevent overshoot on degraded power surfaces.
- Trace accumulation recovery zeros both polynomial surfaces (`theta`, `power_theta`) when resetting P, preventing recursive gain explosions; preserves operator-configured state (mode, ceilings, margins, efficiency mode, NER threshold, slew step) across the event.
- Safety status priority on collision: ASIC thermal wins over VR thermal when both fire on the same tick.
- VR proactive margin moved from baked-in macro to per-state runtime field (`vr_temp_proactive_margin`).
- `G6_SAFETY_VOLTAGE` is now reserved (no code path sets it); kept in the enum for ABI compatibility with earlier beta consumers.
- `last_efficiency` is now gated on `power_w` being within sanity bounds; retains last known-good value on fail-closed paths instead of reporting garbage.
- `is_sample_valid()` carries NER and thermal predicates redundantly with the upstream fast-fail (belt-and-suspenders against future refactors).
- `g6_asic_error_handle_non_blocking()` floor-clamps via `fmaxf(BM1370_X_MIN, ...)` consistent with other safety helpers.
- `g6_brain_self_test()` signature is now `const`-correct.
- `G6BrainState` struct repacked: booleans grouped to eliminate compiler padding.
- `g6_brain_set_defaults()` extracted into a shared static helper between `init` and `reset`.
- VR sentinel check uses `<= G6_VR_TEMP_NO_SENSOR` / `> G6_VR_TEMP_NO_SENSOR` instead of `< 0.0f`, so a glitched sensor returning a small negative value still disables VR monitoring rather than partially engaging it.
- Public API documentation (`docs/API.md`) restored after an upload pipeline mishap replaced it with the glossary body; full content reconciliation across all docs against the shipped code.
- Documentation: softened "Joseph Form" overclaim to "Joseph-style congruence + ridge" consistently.

### Fixed
- NaN telemetry silently skipping safety ticks (manifesto §3.7 violation): non-finite inputs now route fail-closed instead of returning `ESP_ERR_INVALID_ARG`.
- `g6_brain_recover_cold_start()` silently disabling `use_efficiency_mode`: operator-set efficiency mode is now snapshotted and restored across recovery.
- Out-of-bounds telemetry incorrectly setting `G6_SAFETY_VOLTAGE`, misdirecting operators to inspect their VRM.
- Slew-limit validation test silently broken at runtime (theta values produced an out-of-bounds optimum, making the slew clamp inactive).
- Asymmetric floor clamping in NER backoff (now consistent with other safety helpers).
- Dead `power_cold_start` struct field removed (zero readers, zero writers).
- Orphaned references to deleted `power_cold_start` causing compile errors.
- Stray `}` at end of `test_g6_brain.c` breaking the build.
- `G6_VR_TEMP_PROACTIVE_MARGIN_DEFAULT` macro used at runtime instead of the per-state field, so runtime margin changes had no effect.
- ASIC proactive thermal margin duplicated at two call sites with identical `5.0f` literals (now single Kconfig-backed field).
- `G6_JTH_MAX_OUTER_ITERS` Kconfig option silently ignored due to unguarded `#define` in the header.
- Stale `v1.0.0-beta3` version stamp in `test_g6_brain.c`.

---

## [1.0.0-beta4] — 2026-05-20

VR thermal safety (two-tier protection), tick/millisecond unit corrections, integration-layer data-quality fixes.

### Added
- VR regulator temperature monitoring as a second thermal tier alongside ASIC die temperature.
  - New `vr_temp_c` parameter to `g6_brain_update()` (between `temp_c` and `err_pct`). **Breaking API change.**
  - `G6_VR_TEMP_NO_SENSOR` sentinel (`-1.0f`) for hardware without a VR sensor — all VR checks silently skip.
  - `G6_SAFETY_VR_THERMAL` enum value.
  - `G6_VR_TEMP_CEILING` (default 85°C, range 70–105) and `G6_VR_TEMP_PROACTIVE_MARGIN` (default 5°C, range 2–15) Kconfig options.
  - Proactive zone (`vr_temp_c > ceiling − margin`): steps `best_v` back by ×0.992 per cycle, frequency untouched.
  - Hard ceiling (`vr_temp_c ≥ ceiling`): steps back both `best_v` (×0.985) and `best_f` (×0.96).
- `last_safety_status` tracking wired through all safety paths (telemetry was always reporting `G6_SAFETY_OK` before).
- Symmetric `Power Outlier Rejected` WARN log line for power-model outlier rejections.
- Unit tests for VR sentinel/proactive/hard-ceiling behavior.

### Changed
- ASIC temperature gates RLS learning (sample discarded above ceiling). VR temperature *never* gates learning — only constrains setpoints in the safety layer. Rationale: VR heat doesn't corrupt hashrate or power measurements; it only means the setpoints must be reined in.
- `goto safety_layer` is now the default control flow on rejected samples (was an early `return` on power-validation failures).
- `G6_NVS_SCHEMA_VERSION` corrected from `2u` to `3u` to match `g6_brain.c`.
- Integration example (`docs/INTEGRATION_EXAMPLE.c`): pass window-delta `sharesAccepted` not cumulative; feed 10m hashrate average during warm-up, switch to live once `model_quality ≥ 0.5`; compute NER from `Δerrors / (window_hr × Δt)`; log VRM droop coefficient on first valid frame; scale shares by `poolDifficulty / G6_REF_POOL_DIFFICULTY`.

### Fixed
- Slew-rate limiter fighting safety derating: a `safety_active` flag now suspends upward slew whenever any safety condition is active, eliminating sawtooth oscillation at the thermal edge.
- Innovation dead-zone (`xᵀPx < 1e-4`) was incorrectly logged as outlier rejections; now silently skips the RLS update.
- `SETTLE_MS` and `MIN_WINDOW_MS` comparisons mixing raw tick deltas with millisecond constants.
- Invalid `power_w` fail-open (was returning early before safety layer could run).
- Redundant `NVS_SCHEMA_VERSION` dual-definition in `g6_brain.c`; replaced with the canonical macro from the header.
- `docs/API.md` out of sync with beta4 signature (missing `vr_temp_c` parameter).
- Dead `stored_size` read in `g6_brain_load_nvs_fingerprint()`.

---

## [1.0.0-beta3] — 2026-05-20

O(1) analytical J/TH solver, Joseph-form covariance stabilization, 3-sigma outlier gating.

### Added
- 3-sigma statistical outlier gating with localized coordinate-specific innovation variance mapping (`xᵀPx + 0.5f`); rejects sensor glitches without distorting the response surface.
- `model_quality ≥ 0.6` **and** `power_model_quality ≥ 0.6` gates on the J/TH efficiency solver; prevents following noisy gradients from underfit models.
- `G6_JTH_MAX_OUTER_ITERS` Kconfig option.
- `G6_NVS_FINGERPRINT_BUFFER_SIZE` define replacing a stack VLA in the NVS save/load buffers.
- `evaluate_quadratic()` and `get_quadratic_gradient()` helpers reducing duplicated quadratic evaluations.
- Phase 2 reservation comments on vestigial struct fields (`nonce_offset`, `enable_low_latency_jobs`, `valid_sample_count`, `Kp`/`Ki`/`Kd`).
- Internal slew-rate limiting in `AUTO` mode: frequency steps by `dfs_step_mhz`, voltage limited to 5 mV steps.
- Recommended minimum task stack size documentation in `INSTALL.md`.

### Changed
- **Dinkelbach J/TH solver replaced** iterative gradient descent with an O(1) analytical inner solve using Cramer's rule. Eliminated the inner loop entirely (~40+ FP ops/step → ~15 constant-time ops); guarantees convergence to the exact mathematical minimum of the quadratic sub-problem each cycle.
- Joseph-style covariance stabilization replaces standard RLS subtraction; keeps P symmetric and positive semi-definite under floating-point arithmetic.
- Outlier gating runs on both hashrate and power models *before* either is updated (eliminates J/TH drift from asymmetric rejections in efficiency mode).
- Dinkelbach outer-loop convergence detection now correctly uses `prev_lambda` (previous check was always true after any improvement).
- `G6BrainState` struct packed: 1-byte booleans grouped to eliminate compiler padding.
- `powf(2.0f, -L)` → `exp2f(-L)` in Variable Forgetting Factor (hardware-accelerated on ESP32-S3).
- `g6_brain.c` reorganized into clear logical sections (pure helpers → RLS → safety → NVS → core → public API).
- `g6_brain.h` macro groupings centralized; struct documented with section comments.
- Documentation vocabulary grounded: removed "aerospace-grade" / "avionics-class" language in favor of exact technical terms.

### Fixed
- Safety layer completely bypassed on successful RLS updates: a stray `return ESP_OK` before the `safety_layer:` label made the slew-rate limiter, thermal clamping, and voltage ripple checks unreachable on the happy path.
- NVS warm-start silently corrupting `power_theta` and `power_P`: offset advanced by `sizeof(theta)` instead of `sizeof(P)` after the P-matrix copy, reading `power_*` from the middle of P data.
- Silent power-model truncation on NVS save: missing `offset += sizeof(power_P)` saved a 200-byte frame instead of the full 344 bytes.
- Inverted safety layer execution order: thermal/ripple checks were running *before* the slew limiter, so safety reductions were immediately undone within the same cycle.
- Dinkelbach J/TH inner gradient now operates entirely in normalized space (scaling mismatch between normalized gradients and absolute MHz/mV resolved).
- Slew-rate validation test had zero-determinant mock `theta` coefficients, causing the convexity guard to short-circuit the test silently.
- `RLS_SYMMETRY_TOLERANCE` typo (`RLS_SYMMETOW_TOLERANCE`) in `g6_brain_self_test()`.
- CI test step no longer swallows failures (`|| echo` fallback removed).
- Brittle hardcoded `RLS_VFF_SIGMA_SQ` test assertion replaced.
- `nvs_flash_init()` added to test `setUp()` for reliable execution.

### Removed
- `G6_JTH_INNER_STEPS` Kconfig option (no longer meaningful with the analytical solver).
- Iterative Dinkelbach inner gradient loop (eliminated by the analytical replacement).
- Unused `TAG` static global in `test_g6_brain.c`.

---

## [1.0.0-beta2] — 2026-05-18

J/TH efficiency mode (opt-in), NVS warm-start hardening, public telemetry API.

### Added
- `G6_ENABLE_EFFICIENCY_MODE` Kconfig option (default `n`).
- Separate RLS power model (`power_theta` + `power_P`) for J/TH optimization.
- `g6_brain_get_telemetry()` and `G6BrainTelemetry` struct integrated into the public API.
- NVS schema bumped to v2 with full power-model persistence.
- Power sanity check in `g6_brain_update()`.
- Unity test coverage expansion: input validation, safety overrides, proactive thermal scale, covariance matrix metrics.
- Kconfig wiring and full control mode enforcement (`OBSERVE_ONLY` / `RECOMMEND` / `AUTO`).

### Changed
- `g6_brain_reset()` extended to clear Phase 1 fields.
- `INTEGRATION_EXAMPLE.c` promoted to the canonical integration reference.
- Kconfig help texts and section organization improved.
- Documentation refreshed for consistency.

### Fixed
- NVS warm-start (models now correctly restore after reboot).
- Schema version consistency between header and implementation.

---

## [1.0.0-beta1] — 2026-05-12

First reviewed and hardened beta. Ready for community field testing.

### Added
- Self-contained safety layer (thermal, voltage ripple, NER, proactive derating).
- Stabilized RLS with Variable Forgetting Factor, innovation gating, covariance symmetrization, ridge regularization, cold-start initialization.
- NVS persistence of `theta` + full covariance `P` for warm-start.
- Sample quality state machine (settle + measure windows).
- Lambda guard and trace monitoring against covariance collapse.

### Changed
- Bierman-Thornton UD factorization replaced with conventional stabilized RLS (maintainability).
- Efficiency objective corrected to proper J/TH.

### Fixed
- Cold-start bug (zeroed P matrix).
- Double-settle timing bug.
- Thermal scaling and clamps now always execute via `goto safety_layer` pattern, including on rejected samples.

---

## [1.0.0-beta] — May 2026

Early development. Foundations of the current architecture.

### Added
- Enhanced Feed-Forward Predictive Cooling (`dP/dt + K_ff` term for Vcore prediction).
- I2C heartbeat + 9-clock sanitization at init.
- Voltage-Floor Interlock (hard 400–1200 mV clamp for BM1366 safety).
- Self-test criteria boundaries.
- `GLOSSARY.md` and `AGENTS.md` safety-invariants section.

### Changed
- RLS PSD safeguard upgraded to strict positive definite (nonzero `ridge_epsilon` enforced).
- Cold-start guard extended from 10 → 30 ticks.
- Main branch locked as the sole development line.

---

## Pre-Beta

- **v1.0.0-beta.0**: Initial quadratic RLS + safety foundations + NVS fingerprint (Bierman-Thornton prototype).
- Earlier work focused on RLS modeling, safety interlocks, and ESP-Miner integration patterns. Archived in the `v1.8` branch.
