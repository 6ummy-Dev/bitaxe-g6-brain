# Changelog

All notable changes to the Bitaxe G6 Brain will be documented in this file.

## [1.0.0-beta1] - 2026-05-12

**Status**: Extensively reviewed and hardened. Ready for community field testing. Not yet in large-scale production.

### Added
- Fully self-contained safety — all thermal, voltage ripple/undershoot, NER, and proactive derating logic now lives inside `g6_brain.c`. No external `g6_safety.*` files required.
- Stabilized conventional RLS (P-matrix) with:
  - Gradient-based Variable Forgetting Factor (VFF) + innovation gating
  - Covariance symmetrization + diagonal clamping on every update
  - Ridge regularization (`ridge_epsilon`) + trace_P guard before update
  - Proper cold-start initialization (`P = 1e5 × I`)
- NVS persistence of **both theta and full covariance P** for true warm-start (model + uncertainty)
- `last_efficiency` telemetry field (J/TH)
- Meaningful `MIN_VALID_SHARES = 30` and 5-second `MIN_WINDOW_MS` measurement phase
- NVS readiness guard in `g6_brain_init()` (clear error if `nvs_flash_init()` not called first)
- Lambda guard to prevent covariance collapse

### Changed
- Switched from Bierman-Thornton UD factorization to stabilized conventional RLS (P-matrix form) for dramatically better maintainability and auditability while retaining strong numerical robustness via symmetrize + ridge + trace checks. (UD approach archived in commit history for reference.)
- Timing constants renamed for clarity: `SETTLE_SECONDS` → `SETTLE_MS`, `MIN_WINDOW_SECONDS` → `MIN_WINDOW_MS`
- Efficiency objective corrected to proper **J/TH** (energy per terahash)
- `is_sample_valid()` simplified; state machine is now the single source of truth for settle/measure timing
- Internal share threshold made explicit and raised to 30 for realistic stable hashrate

### Fixed
- Critical cold-start bug (P matrix was zeroed → no adaptation)
- Missing initialization of `temp_ceiling`, `ner_threshold`, PID coefficients, and `measure_start_tick`
- Double-settle timing bug (was effectively 16 s)
- `MEASURE_WINDOW` state now correctly waits the full configured duration
- Thermal scaling and final clamps now **always** execute (even on rejected samples) via `goto safety_layer` pattern
- Covariance symmetry loss over long runs (now enforced every update)

### Notes
- Public API (`g6_brain_init`, `g6_brain_update`, `g6_brain_get_optimal`, `g6_brain_self_test`, NVS functions) is **100% backward compatible**.
- `MAX_TEMP_SLOPE` is defined in header but not yet actively used in control loop (planned for Phase 2).
- `RLS_P_CLAMP_MIN/MAX` and self-test now actively protect against ill-conditioned P matrix.

---

**Previous history (condensed for clarity — full details in git log):**

- v1.0.0-beta.0: Initial quadratic RLS + safety foundations + NVS fingerprint (Bierman-Thornton prototype, later evolved)
- v1.0 Beta (early May 2026): Major hardening, sample state machine, efficiency objective, audit-driven fixes
- All pre-v1.0 development archived in `v1.8` branch history

**Next Phase (Phase 2)**: Active thermal slope detection, controlled exploration, PID integration, extended 30+ day soak testing, and Unity test suite.
