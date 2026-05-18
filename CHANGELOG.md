# Changelog

All notable changes to the Bitaxe G6 Brain will be documented in this file.

## [1.0.0-beta2] - 2026-05-18

**Status**: Hardened beta with critical bug fixes, improved test coverage, documentation alignment, and CI cleanup.

### Critical Bug Fixes
- Fixed double execution of thermal safety scaling (safety functions now execute only once).
- Fixed share count validation — `g6_brain_update()` now properly accepts and uses real share count instead of a constant.
- Fixed cold-start initialization — `best_f` and `best_v` now have safe default values instead of remaining at 0.
- Fixed NER error handler — it now correctly reacts to high error rate instead of checking model quality.
- Fixed Unity test runner syntax so tests can actually compile and run.

### Improvements
- Added safe defaults for operating point after cold start.
- Improved defensive programming and input validation.
- Expanded Unity test coverage with meaningful safety and state checks.
- Added explanatory comments for key safety patterns and known limitations (e.g. VFF sigma).

### Documentation Alignment
- Updated `SAFETY.md` to clearly separate implemented features from Phase 2 planned features.
- Cleaned up `INSTALL.md` to remove references to non-existent Kconfig symbols.
- Updated `AGENTS.md` to reflect actual current behavior vs aspirational invariants.
- Improved consistency across documentation.

### CI / Workflow
- Consolidated workflows: removed duplicate `ci.yml`.
- `build.yml` is now the single source of truth, targeting `esp32s3` with visible test results.

### Notes
- Public API remains backward compatible (with the addition of the `share_count` parameter in `g6_brain_update()`).
- All critical issues from independent code review have been addressed.
- Still in beta — further soak testing and Phase 2 features planned.

**Next Phase**: Active thermal slope detection, PID fan control, and expanded field validation.

## [1.0.0-beta2] - 2026-05-17

**Status**: Polished beta with expanded test coverage, improved documentation, and consolidated examples. Ready for wider community testing.

### Added
- Significantly expanded Unity test suite:
  - Input validation and rejection tests
  - Safety layer execution on invalid/overheated samples
  - Proactive thermal derating behavior
  - Covariance matrix symmetry verification
  - Cold-start flag clearing behavior
- Added explanatory comment on the `goto safety_layer` safety pattern for better auditability
- Added power sanity check in `g6_brain_update()`
- Improved Kconfig with clearer help texts and section organization

### Changed
- Made `INTEGRATION_EXAMPLE.c` the **main recommended integration example**
- Consolidated documentation: All `docs/` files updated and cleaned up
- Removed redundant `main_integration_v1.0_beta.c` (now using one clear example)
- Tightened self-test condition number threshold (from 1e6 → 5e5)
- Updated all version strings, headers, READMEs, and documentation to v1.0.0-beta2
- Improved `INSTALL.md` and `docs/README.md` to clearly point to the recommended example

### Notes
- Public API remains **100% backward compatible** with beta1.
- Focus of this release: Test expansion + documentation quality + example consolidation.
- Still in beta — extended field testing recommended.

---

## [1.0.0-beta1] - 2026-05-12

**Status**: Extensively reviewed and hardened. Ready for community field testing. Not yet in large-scale production.

### Added
- Fully self-contained safety — all thermal, voltage ripple/undershoot, NER, and proactive derating logic now lives inside `g6_brain.c`.
- Stabilized conventional RLS (P-matrix) with Variable Forgetting Factor, innovation gating, covariance symmetrization, ridge regularization, and proper cold-start initialization.
- NVS persistence of **both theta and full covariance P** for true warm-start.
- Sample quality state machine with settle + measure windows.
- Lambda guard and trace monitoring to prevent covariance collapse.

### Changed
- Switched from Bierman-Thornton UD factorization to conventional stabilized RLS for better maintainability and auditability.
- Efficiency objective corrected to proper **J/TH**.

### Fixed
- Critical cold-start bug (zeroed P matrix)
- Double-settle timing bug
- Thermal scaling and clamps now **always** execute via `goto safety_layer` pattern even on rejected samples.

### Notes
- Public API is 100% backward compatible.
- Many improvements came from independent technical audits.

---

**Next Phase (Phase 2)**: Active thermal slope detection, controlled exploration, PID fan integration, and extended soak testing.
