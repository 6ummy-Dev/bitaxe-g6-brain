# Changelog

All notable changes to the Bitaxe G6 Brain will be documented in this file.

## [1.0.0-beta2] - 2026-05-18 (Phase 0 + Phase 0.1 + Phase 1 Completed)

**Status**: Hardened beta2 with **full Phase 1 J/TH efficiency optimization** now live.

### Phase 1 — True J/TH Efficiency (Completed)
- Added separate RLS power model (`power_theta` + `power_P`).
- New Kconfig option `G6_ENABLE_EFFICIENCY_MODE` (default = `n` for full backward compatibility).
- When enabled: brain optimizes for minimum J/TH (Watts per TH/s) using predicted power surface.
- When disabled (default): behaves exactly as before (safe hashrate maximizer).
- NVS schema bumped to v2 with full power model persistence (warm-start works for both models).
- `g6_brain_reset()` extended to handle Phase 1 fields.
- No breaking changes — existing integrations continue to work unchanged.

### Phase 0.1 Critical & Code-Quality Fixes
- NVS schema versioning + size prefix (critical)
- VFF sigma_sq fully Kconfig-tunable
- New public `g6_brain_reset()` API
- All magic constants centralized
- Strong single-threaded usage warning
- Self-test + Kconfig updates

### Phase 0 Fixes
- Kconfig fully wired, control modes enforced, NVS auto-save, efficiency honesty patch, etc.

**Notes**
- Public API remains 100% backward compatible.
- Efficiency mode is opt-in and thoroughly safety-gated.
- Ready for community testing — start in RECOMMEND mode.

---

**Next Phase (Phase 2)**: Active thermal slope detection, controlled exploration, PID fan integration, and extended soak testing.

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
