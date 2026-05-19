# Changelog

All notable changes to the Bitaxe G6 Brain will be documented in this file.

## [1.0.0-beta2] - 2026-05-18

**Status**: First signed-off beta release. Ready for field testing and soak testing.

This release consolidates work completed across May 2026.

### Highlights of this release (2026-05-18)
- NVS warm-start fully fixed (models now correctly restore after reboot)
- Schema version made consistent across header and implementation (`G6_NVS_SCHEMA_VERSION = 2u`)
- `g6_brain_get_telemetry()` cleanly integrated into the public API
- All critical bugs from previous QA rounds resolved and verified
- Documentation refreshed for consistency

### Phase 1 — J/TH Efficiency Mode (Completed 2026-05-18)
- Added separate RLS power model (`power_theta` + `power_P`)
- New Kconfig option `G6_ENABLE_EFFICIENCY_MODE` (opt-in, default = `n`)
- When enabled: brain optimizes for minimum J/TH using the predicted power surface
- When disabled: behaves exactly as before (safe hashrate maximizer)
- NVS schema bumped to v2 with full power model persistence
- `g6_brain_reset()` extended to handle Phase 1 fields
- No breaking changes — existing integrations continue to work unchanged

### 2026-05-17 — Polish, Tests & Documentation
- Significantly expanded Unity test suite:
  - Input validation and rejection tests
  - Safety layer execution on invalid/overheated samples
  - Proactive thermal derating behavior
  - Covariance matrix symmetry verification
  - Cold-start flag clearing behavior
- Added explanatory comment on the `goto safety_layer` safety pattern
- Added power sanity check in `g6_brain_update()`
- Improved Kconfig with clearer help texts and section organization
- Made `INTEGRATION_EXAMPLE.c` the main recommended integration example
- Consolidated documentation (removed redundant files)
- Tightened self-test condition number threshold (from 1e6 → 5e5)
- Updated all version strings and documentation to v1.0.0-beta2

### Phase 0 + 0.1 — Foundation (Completed earlier)
- Full Kconfig wiring and control mode enforcement (`OBSERVE_ONLY` / `RECOMMEND` default / `AUTO`)
- NVS auto-save + true warm-start
- Strong single-threaded contract documented
- Comprehensive safety layer + self-test
- Sample quality state machine with settle + measure windows
- VFF sigma_sq made fully Kconfig-tunable
- New public `g6_brain_reset()` API
- All magic constants centralized
- Strong single-threaded usage warning

**Notes**
- Public API is 100% backward compatible.
- Recommended starting mode: `G6_MODE_RECOMMEND`
- Telemetry via `g6_brain_get_telemetry()` is now part of the stable public interface.

---

**Next Phase (Phase 2)**: Analytical J/TH solver, RLS improvements, active thermal slope detection, PID fan control, and extended soak testing.

---

## [1.0.0-beta1] - 2026-05-12

**Status**: Extensively reviewed and hardened. Ready for community field testing.

### Added
- Fully self-contained safety layer (thermal, voltage ripple, NER, proactive derating)
- Stabilized conventional RLS with Variable Forgetting Factor, innovation gating, covariance symmetrization, ridge regularization, and proper cold-start initialization
- NVS persistence of both `theta` and full covariance `P` for true warm-start
- Sample quality state machine with settle + measure windows
- Lambda guard and trace monitoring to prevent covariance collapse

### Changed
- Switched from Bierman-Thornton UD factorization to conventional stabilized RLS for better maintainability
- Efficiency objective corrected to proper **J/TH**

### Fixed
- Critical cold-start bug (zeroed P matrix)
- Double-settle timing bug
- Thermal scaling and clamps now always execute via `goto safety_layer` pattern even on rejected samples

**Notes**
- Public API is 100% backward compatible.
- Many improvements came from independent technical audits.
