# Changelog

All notable changes to the Bitaxe G6 Brain are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [1.0.0-beta7.5] — 2026-06-11

Intensive-QA point release on top of beta7.1. No control behavior changed and no public API signatures changed (one additive named constant, `G6_SLEW_STEP_MV_MAX`). Three substantive classes: test-suite **runtime** integrity (the suite as shipped was order-dependent and re-run-unstable through NVS cross-test contamination, and one surviving integer-macro float assert fails at runtime — both invisible under compile-only CI), NVS-load hardening that makes two long-documented contracts actually true (the `NVS erase failed` ERROR line and bad-blob auto-erase for oversized blobs), and a doc-truth sweep. Suite grows 41 → 42 cases. Version stamps bumped to `v1.0.0-beta7.5` across code and docs.

### Fixed
- **Test suite: NVS cross-test contamination made the suite order-dependent and re-run-unstable.** The save/load round-trip test persists a fingerprint (`theta[0]=42`, `P[0][0]=12345`, …), `tearDown` never erased it, and `setUp → g6_brain_init()` warm-starts from NVS — so every test after the round-trip inherited that blob. Two tests fail outright in registration order: "Cold start flag clears after sufficient updates" (its first assert expects `cold_start == true`; it arrives `false` from warm-start) and the beta7 observability test (fresh-fixture `cov_condition ≈ 1.0 ± 1e-3`; the inherited `P[0][0]=12345` against the 1e5 diagonal yields ≈ 8.1). On a second run of the suite against unerased flash, the very first test ("init initializes correctly") fails too. `setUp` now erases the fingerprint key before `g6_brain_init()`, so every case cold-starts clean-room — order-independent and re-run-stable; the round-trip and bad-blob tests are unaffected (they write and read within a single test body). Never observed because CI compiles but does not execute — the latest in the line of latent runtime failures (beta6.5's covariance pair, beta7.1's slew no-op and integer assert) that the manifesto §3 execution requirement exists to catch.
- **Test suite: surviving integer-macro float assertions — one hard runtime failure, one vacuous, three fragile.** The beta7.1 pass removed the integer `TEST_ASSERT_GREATER_THAN(0.0f, model_quality)` trap from the control-mode test but missed the siblings. (a) The noiseless-convergence test's `TEST_ASSERT_GREATER_THAN(0.6f, model_quality)` truncates both sides to 0 and fails `0 > 0` even on a perfectly converged model — a guaranteed runtime failure. (b) The Dinkelbach test's `TEST_ASSERT_GREATER_OR_EQUAL(G6_EFFICIENCY_MIN_HR_THS, pred_hr)` truncates the `0.5f` floor to 0, accepting any `pred_hr > -1` — vacuous; it asserted nothing. (c) Its J/TH-improvement compare and the two proactive-margin `> 0.0f` checks passed only because the values happen to still differ after integer truncation. All five are now float-true checks (`TEST_ASSERT_TRUE(a > b)`). Whole-unit MHz/mV comparisons remain on the integer macros, where truncation is exact by construction.
- **NVS load: the documented `NVS erase failed: <esp_err>` ERROR line is now actually emitted, and oversized blobs are now actually erased.** `MONITORING.md` documents the ERROR line in a table that claims to list every line the brain emits — but no code path emitted it: a failed `nvs_erase_key`/`nvs_commit` during stale-blob cleanup was silently swallowed, leaving the bad blob to reappear every boot with no operator signal. Separately, a blob **larger than the read buffer** makes `nvs_get_blob` return `ESP_ERR_NVS_INVALID_LENGTH`, which matched neither mismatch branch — the stale blob was silently retained forever, contradicting the documented bad-blob auto-erase. Both fixed in `g6_brain_load_nvs_fingerprint()`: erase/commit failures log the documented ERROR line in both mismatch paths, and `INVALID_LENGTH` routes through the size-mismatch path (WARN + erase + cold start). Also added a `_Static_assert` that the serialized fingerprint frame fits `G6_NVS_FINGERPRINT_BUFFER_SIZE`, so a future `RLS_N` growth becomes a compile error instead of a silent stack-buffer overflow in save/load. No control behavior change. (Verify on hardware that the WARN line's "got" size reads correctly on the `INVALID_LENGTH` path — `nvs_get_blob` is documented to write the actual stored size into `blob_size` there.)
- **`TESTING.md` pointed at a changelog anchor that does not exist.** The CI-coverage note cited "B5-BUG-20 in the changelog" — no such ID appears anywhere in `CHANGELOG.md` (internal QA IDs were never carried into changelog entries). Now references the beta6.5 latent covariance-test failures and this release's NVS-contamination entry as the worked examples.
- **`TESTING.md` scenario 2 claimed `best_f` gets "clamped to 950" on an out-of-bounds input.** False: feeding `f_mhz = 1500` does not drag the recommendation anywhere — `best_f` stays where it was while `G6_SAFETY_INPUT_RANGE` is reported and optimizer movement is suspended. The clamps guarantee `best_f`/`best_v` can never *leave* hardware bounds; they do not retarget the recommendation on bad telemetry. A tester following the old text verbatim would have filed a false bug.
- **`REFERENCES.md` footer still read "May 2026".** Missed by the beta7.1 "doc footer dates corrected to June 2026" sweep — that claim was incomplete by one file. Now June 2026, with a version stamp like every other doc.

### Changed
- **AUTO-mode voltage slew literal named: `G6_SLEW_STEP_MV_MAX` (`5.0f`).** The per-tick voltage step has been a bare `5.0f` (twice) in the slew clamp since beta3 — an unnamed magic number in a safety-relevant path, while its frequency counterpart goes through Kconfig. Now a named, documented public constant; behavior identical. Promotion to a Kconfig tunable is deliberately deferred to the exploration cycle, where per-axis step sizing gets revisited as a whole.
- **Slew-freeze wording made precise (`SAFETY.md` item 9, `AGENTS.md` item 5, `GLOSSARY.md`, `TESTING.md`).** The docs said "upward slew is frozen" during anomalies; the code gates the optimizer's slew step on `AUTO && !safety_active`, suspending it in **both** directions — during an anomaly the only setpoint movement comes from the safety derates themselves. The conservative behavior was always intentional; the docs now say what the code does.
- **NER back-off `model_quality` wording corrected (`SAFETY.md`, `GLOSSARY.md`).** Both claimed the 0.25 clamp "persists until the model re-converges" — but quality is an instantaneous per-sample fit metric, so the clamp lasts exactly until the next accepted update, and one clean sample on a well-fit model can lift it back over the 0.6 solver gate. The docs now state this honestly; an EMA-smoothed quality (which would make the original claim true) is recorded in `docs/ROADMAP.md` as a later-cycle consideration, since it changes control behavior.
- **`API.md` `share_count` guidance now states the consequence of "pass 0 if unknown".** Below `MIN_SHARE_COUNT` every sample is rejected on the share gate — `update_count` stays flat, status stays `G6_SAFETY_OK`, and the brain runs as a safety monitor that never learns. The old text was technically consistent and practically a trap.
- **`last_setting_change_tick` annotated as reserved.** Written on AUTO setpoint movement but read by nothing, exposed nowhere, and absent from the Phase-2 reservation comments — the undocumented-vestige pattern. Now documented in the struct as reserved for the future settle/measure window; the remove-vs-use decision belongs to the cycle that builds that window.
- **Test-suite comment: hardcoded source line numbers removed.** The non-anomaly section header referenced "line ~444 / ~471" — the same drift hazard the beta7.1 pass removed from `g6_brain.c` — now symbolic references to the gates themselves.
- **`docs/ROADMAP.md`: two engineering follow-ups recorded.** (1) Innovation-gate deduplication — `g6_brain_update()` computes `xᵀPx` for the divergence guard, then `has_significant_innovation()` recomputes the identical quadratic form (bit-identical today: same per-element accumulation order, verified by hand-trace); pure redundancy left deliberately untouched in a point release because it sits in the estimator hot path. (2) The `model_quality` EMA consideration above.

### Added
- **Test: "NVS oversized blob (exceeds read buffer) is erased on load"** — deterministic regression for the `ESP_ERR_NVS_INVALID_LENGTH` path: writes a blob larger than the 1024-byte read buffer, asserts the WARN-path erase fires and the brain stays cold. Suite: 42 cases.

> **Note:** As with beta6.5/beta7, the Unity suite was not executed in the environment that produced these changes (no ESP-IDF/QEMU runner). Every fix above was verified by hand-trace against the shipping sources — including re-deriving the two contamination failures and the integer-truncation failure from the same Unity macro semantics the beta7.1 fix documented — but the standing gate is unchanged: run the full suite on hardware or QEMU (`idf.py -T g6_brain build flash monitor`) before trusting it. beta7.5 is the release where the suite should finally run green end-to-end for the first time; if the first real execution isn't green, that is itself a finding.

---

## [1.0.0-beta7.1] — 2026-06-10

Point release: QA-pass corrections on top of beta7. No control behavior changed and no public API signatures changed. Three substantive items — the CI-honesty relabel finally landing in the workflow file, a runtime test that was silently a no-op, and a documentation-consistency sweep — plus the routine version-stamp bump to `v1.0.0-beta7.1` across the code and docs.

### Fixed
- **CI workflow relabel finally applied to the file.** The beta7 *Changed* entry claimed the compile-and-link-only relabel was "now actually applied," but at the beta7 tag `.github/workflows/build.yml` still read `name: Build & Test` with a `"Build + Unity tests compiled and linked successfully"` verify step — the same described-but-never-landed gap beta6.5 had. It is now genuinely in the file: workflow `Build (compile + link test suite)`, job `Build & link (no test execution)`, a verify step that states the Unity suite is compiled and linked but **not executed** (a green build is not a green test run), points at `docs/TESTING.md`, and gives the `idf.py -T g6_brain build flash monitor` command, plus a commented-out `qemu-test` job skeleton documenting the on-target execution path. Serves manifesto non-negotiable 3.7.
- **Test "respects control_mode" was a runtime no-op.** The AUTO half asserted `best_f` changes, but two off-center samples left the six-coefficient quadratic underdetermined with an out-of-bounds optimum, so `g6_brain_get_optimal()` fell back to `best_f` and the slew never moved it (the historical B3/B5 slew-test failure mode, invisible while CI only compiled). It now presets a concave-down surface with an in-bounds optimum before the AUTO update so the slew clamp has a real target (`best_f` 650 → 675), mirroring "Internal Slew-Rate Limiting." Also rescaled the lone surviving `hr = 120` first sample to TH/s (`1.2`), and replaced the integer `TEST_ASSERT_GREATER_THAN(0.0f, model_quality)` (which casts a sub-1.0 quality to 0 and fails at runtime) with a float-true check. Verified on the host harness against the shipping `g6_brain.c`.
- **Documentation consistency pass.** `KCONFIG.md` efficiency-mode wording aligned with the other docs (the pre-beta6.5 "absolute minimum … exact algebraic fractional optimization solver" overclaim was the last copy still unsoftened); `GLOSSARY.md` "Trace Accumulation Recovery" / "P-Matrix Singular Recovery" entries now name the non-PSD (negative predicted-variance) trigger alongside the trace trigger, matching the enum entry; `SAFETY.md` and the `README.md` control-mode table now distinguish optimizer/slew movement (mode-gated) from the proactive safety derate (active in all modes); a `g6_brain.c` comment's hardcoded line numbers replaced with symbolic references; a `g6_brain.h` comment no longer pins on-device exploration to a specific pre-1.0 beta, matching `docs/ROADMAP.md`'s post-1.0 placement; doc footer dates corrected to June 2026; and the NER test renamed to reflect the upstream check it actually exercises.

---

## [1.0.0-beta7] — 2026-06-02

Observability on-ramp for the operating-point identifiability limitation, plus the CI-honesty relabel that the beta6.5 entry described but never actually landed in the workflow file, and two version/link consistency fixes. No control behavior changed and no public API signatures changed — the new telemetry fields are appended to `G6BrainTelemetry`. (Active on-device exploration, which supplies the operating-point variation this release only *measures the absence of*, remains a separate later cycle — see `docs/ROADMAP.md`.)

### Added
- **Under-excitation observability (telemetry only).** Two fields appended to `G6BrainTelemetry`: `cov_condition` (the Gershgorin condition-number estimate of the hashrate `P`, same value as `g6_brain_get_cov_condition()`, now readable from the snapshot directly) and `model_under_excited` (a `bool`, `true` once past cold start and `cov_condition > G6_EXCITATION_COND_WARN`). At a fixed operating point the six-coefficient quadratic basis is unidentifiable, so the optimizer's recommended `best_f`/`best_v` are undetermined — but `model_quality` reads *high* in that state because it measures fit at the single visited point, not identifiability, and the covariance only trips `G6_SAFETY_P_MATRIX_SINGULAR` at the fully-indefinite extreme. `model_under_excited` surfaces the wide intermediate band where recommendations are not yet trustworthy. **This changes no control behavior** — both fields are pure reads of `P` computed in `g6_brain_get_telemetry()`. New constant `G6_EXCITATION_COND_WARN` (`1e5`) is a conservative starting threshold pending closed-loop field calibration (tracked in `docs/ROADMAP.md`), and is gated on `!cold_start` so a fresh, well-conditioned-but-uninformed model is not mislabelled.
- **Test: "Telemetry exposes cov_condition and under-excitation flag (observability)."** Asserts `cov_condition` mirrors `g6_brain_get_cov_condition()` exactly, that a fresh fixture reports `cov_condition ≈ 1` with the flag clear, that an ill-conditioned covariance past the threshold sets the flag once out of cold start, and that the same ill-conditioning stays unflagged while still in cold start.

### Changed
- **CI workflow relabeled to reflect compile-and-link-only (now actually applied).** The beta6.5 changelog claimed this relabel, but the workflow file still read `name: Build & Test` with a `"Build + Unity tests compiled and linked successfully"` verify step — i.e. the doc described a change that was never made. Now corrected in the file: workflow is `Build (compile + link test suite)`, the job is `Build & link (no test execution)`, and the verify step states explicitly that the Unity suite is compiled and linked but **not executed** (a green build is not a green test run), points at `docs/TESTING.md`, and gives the `idf.py -T g6_brain build flash monitor` command. Added a commented-out `qemu-test` job skeleton documenting the real on-target execution path. Directly serves manifesto non-negotiable 3.7.
- **Documentation for the new observability fields.** `API.md` and `MONITORING.md` gain telemetry-table rows and a "Model identifiability & under-excitation" / "Is the brain's recommendation trustworthy?" subsection each; `GLOSSARY.md` gains an "Under-excitation / Identifiability" entry and a clarifying note on `model_quality`. All frame the fields as observability-only and point at `docs/ROADMAP.md` for the planned exploration.

### Fixed
- **Stale `Kconfig` version stamp.** `components/g6_brain/Kconfig` still read `# G6 Brain Configuration — v1.0.0-beta6` — it was missed in the beta6.5 version bump (only the `KCONFIG.md` doc was updated). Now `v1.0.0-beta7`.
- **`docs/README.md` did not link `ROADMAP.md`.** The roadmap was wired into the root `README.md` and `MANIFESTO.md` when it was split out, but the docs-folder index never got the entry. Added to its project-root link list.

---

## [1.0.0-beta6.5] — 2026-05-29

Correctness pass on the RLS covariance math and the efficiency-mode unit contract, plus the test/CI integrity gaps that let two covariance-dependent test failures ship undetected, and a covariance-divergence guard for the fixed-operating-point case surfaced by real baseline telemetry. No public API signatures changed. Validated against 2,190 samples of real BM1370 baseline telemetry (fixed 815 MHz / 1210 mV); see the validation notes under each item.

### Fixed
- **RLS covariance update was not the exact RLS posterior — missing measurement-noise injection term.** Both the hashrate (`P`) and power (`power_P`) updates computed only the symmetric congruence `(M P Mᵀ)/λ` and omitted the `+ k kᵀ` (R=1) injection term. The two forms are *not* equal: `(I − k xᵀ)P/λ ≡ (M P Mᵀ)/λ + k kᵀ`. Omitting the term biased the covariance downward (trace ran ~7× low on a representative sequence) and shrank the Kalman gain faster than RLS prescribes, slowing adaptation after an operating-point change. Now uses the full Joseph form including the injection term. Validated against an independent analytic RLS reference (host-side) to ~1e-13 in double precision.
  - **Side effect: this repaired two latent test failures.** With the under-sized covariance, a constant-basis sample drove `P[5][5]` to ~2e-5 after a *single* update — below `RLS_INNOVATION_THRESHOLD` (1e-4). The existing "Statistical Outlier Gating rejects severe sensor anomalies" and "Cold start flag clears after sufficient updates" tests feed repeated same-point samples and therefore *would have failed if executed* (the anomaly sample was innovation-gated before reaching the outlier check; only 1 of 30 cold-start samples was accepted). Both pass with the corrected covariance (30/30 accepted; `P[5][5]` stays O(1)). These failures were never observed because CI compiles tests but does not run them (see below).
- **Efficiency mode was silently dead on real hardware (unit mismatch).** `G6_EFFICIENCY_MIN_HR_THS` was `8.0` TH/s, but a stock BM1370 runs ~1.0–1.2 TH/s (~1.5 overclocked), so the Dinkelbach J/TH solver's viability gate rejected every real operating point and efficiency optimization never engaged. Lowered to `0.5` TH/s. TH/s is now the canonical hashrate unit throughout the code, tests, and docs.
- **`docs/INTEGRATION_EXAMPLE.c` fed hashrate in the wrong unit.** AxeOS reports `hashRate`/`hashRate_10m` in GH/s (e.g. `1639.8`), but the example passed them straight into `g6_brain_update()`'s `hr_ths` (TH/s) parameter — a 1000× error that also corrupted the example's NER computation (`hashrate_10m * 1e12f` assumes TH/s). The example now converts GH/s → TH/s on read.
- **`g6_brain_get_cov_condition()` / `g6_brain_self_test()` reported a meaningless "condition number."** Both used `max_diag / min_diag`, which is only the true 2-norm condition number for a diagonal matrix and can label a near-singular `P` as well-conditioned. Replaced with a Gershgorin-disc upper-bound estimate (`max_i(P_ii + Σ_{j≠i}|P_ij|) / min_i(P_ii − Σ_{j≠i}|P_ij|)`), returning a large sentinel when the lower bound is non-positive so callers treat the matrix as ill-conditioned.
- **Covariance could go indefinite at a fixed operating point and silently freeze the estimator (found via real baseline telemetry).** At a single (f, v) the six-term quadratic basis is unidentifiable; `P`/`power_P` grow extremely ill-conditioned and the diagonal-only clamp in `rls_symmetrize_clamp_and_stabilize()` cannot keep them positive-semidefinite. The trace-divergence check does **not** catch this (the trace stays bounded), so the predicted variance `xᵀPx` would go negative and fall through the innovation gate — silently treated as "insignificant", which freezes the channel while still reporting `G6_SAFETY_OK`. In efficiency mode the power-channel gate runs before the hashrate update, so a degenerate `power_P` froze **both** channels. Replaying the real fixed-point log (efficiency mode) reproduced this exactly: updates stopped at sample 170 with no recovery. Added an explicit non-PSD guard — a strictly negative `xPx` (or `power_xPx`) now routes to the existing `g6_brain_recover_cold_start()` path and surfaces `G6_SAFETY_P_MATRIX_SINGULAR`, so a fixed-point stall is visible and self-healing instead of silent. With the guard, the same replay processed all 2,181 in-range samples, tripping recovery 9 times over the week instead of freezing. (Pre-existing — not introduced by the `+ k kᵀ` fix; both the old and new covariance enter the same unidentifiable regime. The robust mitigation is operating-point variation: even ±5 MHz / ±5 mV keeps the basis identifiable and `P` well-conditioned.)

### Changed
- **Outlier-gate variance floors and model-quality denominators split into channel-appropriate named constants.** The single `0.5` outlier floor and `+1.0` quality denominator were scale-coupled to the wrong (non-TH/s) units. Introduced `G6_HR_OUTLIER_VAR_FLOOR_THS2` (0.01), `G6_PW_OUTLIER_VAR_FLOOR_W2` (0.5), `G6_QUALITY_DENOM_FLOOR_HR_THS` (0.1), and `G6_QUALITY_DENOM_FLOOR_PW_W` (1.0), applied to the hashrate and power channels respectively. **These are physically-reasoned starting values, not field-calibrated — validate against real telemetry noise before relying on the gates.**
- **CI workflow no longer implies it runs the tests.** Renamed from "Build & Test" to "Build (compile + link test suite)"; the job is "Build & link (no test execution)"; the final step states explicitly that the Unity suite is compiled and linked but **not executed** (execution needs QEMU or hardware), and points at `docs/TESTING.md`. Added a commented-out `qemu-test` job skeleton documenting the real-execution path. This is the gap that allowed the two covariance-dependent test failures above to go unnoticed.
- **Documentation reconciled with the corrected math.** Covariance described as the full Joseph form (exact RLS posterior, including the `k kᵀ` injection term) in `SAFETY.md`, `AGENTS.md`, `GLOSSARY.md`, `README.md`, and `docs/README.md`. The "exact `O(1)`" Dinkelbach claim softened to "`O(1)`-per-step; exact closed form for interior optima, clamped boundary point otherwise" (the inner solve is only the exact unconstrained optimum when it lands inside the box). `g6_brain_get_cov_condition()` documented as a Gershgorin upper-bound estimate in `API.md` and `SAFETY.md`. `G6_EFFICIENCY_MIN_HR_THS` value and TH/s-vs-GH/s guidance corrected in `API.md` and `GLOSSARY.md`.
- **`AGENTS.md` "no dead enum" rule reconciled with the reserved `G6_SAFETY_VOLTAGE` value.** The Forbidden-Patterns rule now carves out an explicit exception for a value deliberately reserved for backward-compatible integer mapping, provided it is documented as reserved everywhere it appears (as `G6_SAFETY_VOLTAGE` already is in `MONITORING.md`).

### Added
- **Optional, opt-in temperature plausibility band (default OFF).** New Kconfig options `G6_ENABLE_TEMP_PLAUSIBILITY` (bool, default `n`), `G6_TEMP_PLAUSIBILITY_MIN` (default `0 °C`, range −40–40), and `G6_TEMP_PLAUSIBILITY_MAX` (default `120 °C`, range 90–200). When enabled, a finite-but-implausible ASIC/VR temperature (stuck-low/stuck-high sensor) outside the band is routed fail-closed to `G6_SAFETY_INPUT_RANGE`, preventing the brain from training on a thermally-stressed chip that reads as cold. The VR no-sensor sentinel (`-1.0`) is always exempt. Off by default — the brain otherwise validates temperature for finiteness only and trusts the integrator's telemetry layer. Documented in `KCONFIG.md` and `MONITORING.md`.
- **Test: "RLS converges to a known quadratic surface (noiseless)."** End-to-end estimator regression guard — feeds many noiseless samples from a known quadratic surface (expressed at the real ~1.0–1.2 TH/s scale) and asserts the RLS coefficients converge to the true values and `model_quality` is high. Scope is stated honestly in-test: it guards the point-estimate (theta) recursion and the corrected unit scale, **not** the covariance magnitude — on consistent noiseless data both the correct and the (former) buggy covariance drive theta to the same fixed point. A deterministic tracking-based covariance regression test is recommended as future work.
- **Test: `g6_brain_self_test` "good vs degraded" now actually tests both states.** Replaced a tautological `TEST_ASSERT(ret == ESP_OK || ret == ESP_FAIL)` with a real two-state check: a fresh fixture must return `ESP_OK`, and a fixture with a diagonal driven past `RLS_P_CLAMP_MAX` must return `ESP_FAIL`.
- **Test: "Negative predicted variance triggers P-matrix recovery (not silent freeze)."** Deterministic regression for the non-PSD guard above: forces `power_P` indefinite in the constant-term direction (`power_P[5][5] = -1` at the basis center, where `xᵀ·power_P·x = power_P[5][5]`) and asserts the next update trips `G6_SAFETY_P_MATRIX_SINGULAR` recovery (cold start re-armed, `P`/`power_P` reset to 1e5·I, theta zeroed) rather than silently skipping.

### Test suite
- Rescaled the suite to TH/s: 34 `g6_brain_update()` hashrate arguments and both Dinkelbach surface fixtures moved from the old ~100× scale to realistic ~1.0–1.2 TH/s (numerically re-verified: hashrate-only optimum unchanged at 775 MHz, J/TH improves 17.91 → 15.35 W/TH, predicted HR stays above the 0.5 gate). Fixed a malformed float literal (`1f`) introduced during rescaling and tightened the covariance-symmetry test's per-sample spread to stay in-range.

> **Note:** The Unity suite was not executed in the environment that produced these changes (no ESP-IDF/QEMU runner). The covariance fix is corroborated three ways — the algebraic identity, an independent host-side numerical check, and the repair of the two latent test failures above — and the unit/floor/thermal changes were validated against 2,190 samples of real BM1370 baseline telemetry, and the fixed-point divergence guard was confirmed on a faithful host replay of that log (float64). The full suite, the rescaled fixtures, and the new tests should still be run on QEMU or hardware (`idf.py -T g6_brain build flash monitor`) before this is trusted in the field — on-device float32 makes the fixed-point covariance even more fragile than the host replay, so the divergence guard matters more there, not less.

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
