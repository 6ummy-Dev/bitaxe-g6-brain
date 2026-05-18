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

---

## [1.0.0-beta2] - 2026-05-17

**Status**: Polished beta with expanded test coverage, improved documentation, and consolidated examples. Ready for wider community testing.

### Added
- Significantly expanded Unity test suite
- Added explanatory comment on the `goto safety_layer` pattern
- Added power sanity check
- Improved Kconfig

### Changed
- Made `INTEGRATION_EXAMPLE.c` the main recommended example
- Removed redundant `main_integration_v1.0_beta.c`
- Tightened self-test condition number threshold
- Updated all documentation to v1.0.0-beta2

### Notes
- Public API remains 100% backward compatible with beta1.

---

## [1.0.0-beta1] - 2026-05-12

**Status**: Extensively reviewed and hardened.

### Added
- Fully self-contained safety inside `g6_brain.c`
- Stabilized conventional RLS with VFF, innovation gating, symmetrization, and cold-start fix
- NVS persistence of both `theta` and full `P` matrix
- Sample quality state machine

### Changed
- Switched from Bierman-Thornton UD to conventional RLS for better auditability

### Fixed
- Critical cold-start bug
- Double-settle timing bug
- Safety scaling now always executes

---

**Previous history (condensed):**

- **v1.0.0-beta.0**: Initial quadratic RLS + safety foundations + NVS fingerprint (Bierman-Thornton prototype)
- **Early May 2026**: Major hardening, sample state machine, efficiency objective, and audit-driven fixes
- All pre-v1.0 development is archived in the `v1.8` branch history

**Next Phase (Phase 2)**: Active thermal slope detection, controlled exploration, PID fan integration, and extended soak testing.
