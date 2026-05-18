# Changelog

All notable changes to the Bitaxe G6 Brain will be documented in this file.

## [1.0.0-beta2] - 2026-05-18

**Status**: Hardened beta release. Critical bug fixes, expanded test coverage, documentation alignment, and CI stabilization.

### Critical Bug Fixes
- Fixed double execution of thermal safety scaling (safety functions now execute only once via `goto safety_layer` pattern).
- Fixed share count validation — `g6_brain_update()` now properly accepts and uses the real `share_count` parameter.
- Fixed cold-start initialization — `best_f` and `best_v` now initialize to safe center values instead of 0.
- Fixed NER error handler double-execution and incorrect triggering logic.
- Restored power sanity check validation in the update path.
- Fixed Unity test runner so tests actually compile and run (removed broken manual `app_main`).
- Removed duplicate/broken manual Unity test runner in `test_g6_brain.c`.

### Improvements
- Expanded Unity test coverage with meaningful safety, state machine, and numerical stability checks.
- Improved defensive programming and input validation throughout `g6_brain_update()`.
- Added safe operating point defaults after cold start.
- CI workflow stabilized using Docker (`espressif/idf:v5.3`) with proper test project scaffolding.

### Documentation & Project Hygiene
- Updated `SAFETY.md` to clearly separate implemented features from Phase 2 items.
- Cleaned up `INSTALL.md`, `AGENTS.md`, and other docs for consistency.
- Consolidated integration examples — `docs/INTEGRATION_EXAMPLE.c` is now the single recommended file.
- Removed redundant files and duplicate changelog entries.

### Notes
- Public API remains **backward compatible** with beta1 (with the addition of the required `share_count` parameter).
- All critical issues from independent code reviews have been addressed.
- Still in **beta** — extended field testing and soak testing recommended before production use.

---

## [1.0.0-beta1] - 2026-05-12

**Status**: Extensively reviewed and hardened. Ready for community field testing.

### Added
- Fully self-contained safety inside `g6_brain.c` (thermal, voltage, NER, proactive derating).
- Stabilized conventional RLS with Variable Forgetting Factor, innovation gating, covariance symmetrization, ridge regularization, and proper cold-start initialization.
- NVS persistence of both `theta` and full `P` matrix for true warm-start.
- Sample quality state machine (settle + measure windows).

### Changed
- Switched from Bierman-Thornton UD factorization to conventional stabilized RLS for better maintainability and auditability.
- Efficiency objective corrected to proper **J/TH**.

### Fixed
- Critical cold-start bug (zeroed P matrix).
- Double-settle timing bug.
- Thermal scaling and safety clamps now **always** execute even on rejected samples.

### Notes
- Public API is 100% backward compatible.
- Many improvements came from independent technical audits.

---

**Next Phase (Phase 2)**: Active thermal slope detection, controlled exploration, PID fan integration, and extended soak testing.

**Previous history**: All pre-v1.0.0 development is archived in the `v1.8` branch.
