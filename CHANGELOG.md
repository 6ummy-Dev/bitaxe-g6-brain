# Changelog

All notable changes to the Bitaxe G6 Brain will be documented in this file.

## [1.0.0-beta2] - 2026-05-18 (Phase 0 QA Fixes)

**Status**: Hardened beta2 with **full Phase 0 fixes** applied (Kconfig wiring, control modes enforcement, NVS auto-save, efficiency honesty).

### Phase 0 Fixes Applied
- **Kconfig fully wired**: All `G6_*` options now read at runtime via `sdkconfig.h` with safe fallbacks (`G6_RLS_LAMBDA_MIN`, `G6_TEMP_CEILING`, `G6_NER_THRESHOLD`, `G6_DFS_STEP_MHZ`, etc.).
- **Control modes enforced**: `G6_MODE_OBSERVE_ONLY`, `G6_MODE_RECOMMEND` (new safe default), `G6_MODE_AUTO` now respected in `update()` and `get_optimal()`.
- **NVS auto-save**: Full theta + P-matrix now saved every ~5 minutes after 10 updates (true warm-start works out of the box).
- **Efficiency honesty patch**: Updated all docs and code comments — this is a **safe hashrate maximizer** (quadratic argmax of HR(f,v) with hard safety). True J/TH optimization is Phase 1.
- **Defensive improvements**: Power sanity, ridge/temp/ner/dfs now live from Kconfig, safer defaults in `init()`.
- **Documentation alignment**: KCONFIG.md, API.md, INTEGRATION_EXAMPLE.c, README.md all updated to reflect live behavior.

### Previous Changes (unchanged)
**Critical Bug Fixes** (from earlier beta2)
- Fixed double execution of thermal safety scaling.
- Fixed share count validation.
- Fixed cold-start initialization.
- Fixed NER error handler.
- Fixed Unity test runner syntax.
- Re-added power sanity check.
- Removed broken manual Unity test runner.

**Improvements**
- Expanded Unity test coverage.
- Improved defensive programming and input validation.
- Documentation alignment across all files.

**CI / Workflow**
- `build.yml` is the single source of truth.

**Notes**
- Public API remains backward compatible (with the addition of enforced control modes).
- All critical issues from independent code review have been addressed.
- Still in beta — further soak testing recommended.

---

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
