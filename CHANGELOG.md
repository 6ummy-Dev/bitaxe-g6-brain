# Changelog

All notable changes to the Bitaxe G6 Brain will be documented in this file.

## [1.0.0-beta1] - 2026-05-11

### Added
- **Fully self-contained safety** — All thermal, voltage, frequency, and ASIC error handling logic is now integrated directly inside `g6_brain.c`. The separate `g6_safety.c` / `g6_safety.h` modules are no longer required (legacy stubs provided for transition).
- **NASA Level C hardening** applied throughout:
  - Proper cold-start covariance matrix initialization (`P = 1e5 × I`)
  - Strict `isfinite()` input sanitization in `g6_brain_update` (returns `ESP_ERR_INVALID_ARG` on bad data)
  - `goto safety_layer` pattern ensures boundary clamps and proactive scaling always execute
  - Single 8-second settle timing (`SETTLE_MS`) with proper `MEASURE_WINDOW` duration check (`MIN_WINDOW_MS`)
  - Meaningful share count validation (`MIN_VALID_SHARES = 30`)
  - NVS initialization guard in `g6_brain_init`
  - Lambda guard to prevent covariance collapse
- New telemetry field: `last_efficiency` (J/TH) exposed in `G6BrainState`

### Changed
- Timing macros renamed for correctness: `SETTLE_SECONDS` → `SETTLE_MS`, `MIN_WINDOW_SECONDS` → `MIN_WINDOW_MS`
- Efficiency calculation corrected to proper energy efficiency (`power_w / hr_ths` → J/TH)
- `is_sample_valid` no longer performs redundant time checks (state machine is now the single source of truth)
- Internal share threshold made explicit and documented

### Removed
- Dependency on `g6_safety.c` / `g6_safety.h` (all safety logic consolidated)
- Hardcoded `50U` share count in update path

### Fixed
- Critical RLS cold-start bug (P matrix was zeroed, preventing adaptation)
- Missing initialization of `temp_ceiling`, `ner_threshold`, and PID coefficients
- Double-settle timing issue (was effectively 16 seconds)
- `MEASURE_WINDOW` state now properly waits the configured duration
- Thermal scaling and final clamps now always execute even on rejected samples

### Notes
- Public API (`g6_brain_init`, `g6_brain_update`, `g6_brain_get_optimal`, etc.) remains unchanged for backward compatibility.
- `nvs_flash_init()` must be called before `g6_brain_init()` (the brain now returns a clear error if NVS is not ready).
- `MAX_TEMP_SLOPE` is defined but not yet actively enforced (planned for v1.1).

---

**Status**: Extensively reviewed and hardened. Ready for community field testing. Not yet deployed in production.

## [v1.0.0-beta.1] - 2026-05-11

**Documentation Overhaul & Safety Reference**

Major improvement to project documentation to match the production quality of the G6 Brain code.

**Changes:**
- Added new `docs/README.md` as the official index and quick-start landing page for the entire documentation set
- Completely rewrote `docs/API.md` — now a full public API reference with parameter details, return values, usage examples, thread-safety notes, and model quality interpretation
- Expanded `docs/INSTALL.md` into a complete, step-by-step integration guide with:
  - Prerequisites and resource requirements
  - Git submodule + manual copy instructions
  - Exact CMakeLists.txt and menuconfig steps
  - Two integration paths (recommended drop-in task vs manual)
  - Detailed troubleshooting table
  - Post-install aerospace-style QA checklist
- Expanded `docs/KCONFIG.md` with detailed explanations, real-world recommendations, interaction notes, and a "Recommended Starting Configuration" for Gamma 602+
- Added new `docs/SAFETY.md` — comprehensive safety reference covering:
  - All 7 protective layers (thermal, voltage, signal integrity, mathematical, sample quality, NVS, nonce safeguards)
  - Actionable **Triple-8 Certification Path** (8h stress / 8h WiFi interference / 8 dirty power cycles)
  - When and why the brain refuses to tune
  - Recommended production monitoring and alerting
- Updated cross-references across all docs for better navigation
- Aligned documentation tone and depth with the project's aerospace QA and modular architecture claims

These changes make the G6 Brain significantly more accessible and trustworthy for new integrators while preserving the minimalist, high-signal style of the original project.

## **Next Phase (Phase 2):** Advanced telemetry, controlled exploration, and extended soak testing (as previously noted). Documentation will continue to evolve in lockstep with code. ##

## [v1.0.0-beta.1] - 2026-05-11

**Phase 1 Complete**

Completed the initial hardening phase of the G6 Brain RLS module.

**Changes:**
- Centralized all RLS, BM1370, and safety constants in g6_brain.h
- Implemented gradient-based Variable Forgetting Factor with innovation gating
- Added covariance windup protection using trace monitoring
- Added full sample quality state machine with settle/validate gates
- Added Hessian check on analytical optimum solver with safe fallback
- Switched optimization objective to efficiency (J/GH) with fail-closed auto-apply
- Added NVS silicon fingerprint warm-start capability
- Applied BM1370-specific normalization and operating limits
- Performed final code cleanup and constant deduplication

The core RLS implementation meets the stability, safety, and modularity requirements identified in the technical audits.

**Next Phase (Phase 2):** Advanced telemetry buffering, controlled exploration policy, and extended soak testing.

## [v1.0.0-beta.0] - 2026-05-11

**Major RLS Hardening**

- Added Bierman-Thornton style numerical stability improvements
- Implemented sample quality state machine
- Added efficiency-based optimization objective
- Added NVS-based per-chip warm-start (silicon fingerprint)
- BM1370 tuning and safety limits applied

## [v1.0.0-beta.0] - 2026-05-10

**Initial RLS Core and Safety Foundations**

- Initial quadratic RLS implementation with real-time modeling
- Basic safety interlocks (thermal, voltage, NER)
- NVS persistence foundation
- Stochastic nonce support
- Modular component structure established

Previous development history (v1.8 branch and earlier) has been archived in commit history.
