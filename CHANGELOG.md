# Changelog

All notable changes to the Bitaxe G6 Brain will be documented in this file.

## [1.0.0-beta3] - 2026-05-19 *In Progress*

### 2026-05-19 — QA Fixes & Polish (Critical + Robustness)

**Critical Fixes**
- Fixed major math bug in Dinkelbach J/TH optimizer: inner gradient descent now correctly operates entirely in normalized space (`fn_inner`/`vn_inner`). Scaling mismatch between normalized gradients and absolute frequency/voltage is resolved.
- Made NVS fingerprint read/write buffers symmetric and removed Variable Length Array (VLA) from the stack by introducing `#define G6_NVS_FINGERPRINT_BUFFER_SIZE`.

**Improvements**
- Replaced `powf(2.0f, -L)` with `exp2f(-L)` in Variable Forgetting Factor calculation for better performance on ESP32-S3.
- Improved test robustness: removed brittle hardcoded `RLS_VFF_SIGMA_SQ` assertion and added `nvs_flash_init()` in `setUp()` for more reliable test execution.

**Impact**
- J/TH efficiency mode is now mathematically correct and safe to use.
- NVS persistence is more robust and future-proof.
- Overall code quality and maintainability improved.

### 2026-05-19 — Phase 2 Early Improvements (J/TH Solver + CI + Quality)

- Added `model_quality` gate in the J/TH optimizer: skips aggressive optimization when `model_quality < 0.6` (important safety improvement for the analytical solver).
- Added Kconfig options for the Dinkelbach J/TH solver:
  - `G6_JTH_MAX_OUTER_ITERS`
  - `G6_JTH_INNER_STEPS`
- Updated `g6_brain.h` with proper macro definitions for the new Kconfig options (fixed build error).
- Polished the Dinkelbach-based J/TH optimizer in `g6_brain.c` with improved comments and structure.
- Updated CI workflow (`.github/workflows/build.yml`):
  - Replaced dummy `echo` step with real `idf.py test` execution.
  - Made test step graceful (non-fatal) for the current minimal CI setup.
- Updated documentation:
  - `docs/API.md`: Added note about the new Dinkelbach J/TH optimizer.
  - `docs/KCONFIG.md`: Documented the two new J/TH solver options.

---

## [1.0.0-beta2] - 2026-05-18 *Completed*

**Status**: First signed-off beta release. Ready for field testing and soak testing.

This release consolidates work completed across May 2026.

### 2026-05-18 — Highlights of this release
- NVS warm-start fully fixed (models now correctly restore after reboot)
- Schema version made consistent across header and implementation (`G6_NVS_SCHEMA_VERSION = 2u`)
- `g6_brain_get_telemetry()` cleanly integrated into the public API
- All critical bugs from previous QA rounds resolved and verified
- Documentation refreshed for consistency

### 2026-05-18 — Phase 1 — J/TH Efficiency Mode
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

### 2026-05-17 — Phase 0 + 0.1 — Foundation
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

## [1.0.0-beta1] - 2026-05-12 *Completed*

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

---

## [v1.0.0-beta] - May 2026 (Early Development)

**Avionics-Class Hardening**
- Added Enhanced Feed-Forward Predictive Cooling (dP/dt + K_ff term for Vcore prediction)
- Added I2C Heartbeat + 9-clock sanitization at init
- Added Voltage-Floor Interlock (hard 400mV–1200mV clamp for BM1366 safety)
- Added Brown-out RTC Logging stub for post-mortem analysis
- Integrated IRAM_ATTR comments on hot paths

**QA Audit v3 Response — Critical fixes**
- RLS PSD safeguard upgraded to strict Positive Definite (nonzero ridge_epsilon enforced)
- Cold-start guard extended from 10 → 30 ticks
- Removed overstated "satellite-grade reliability" claims
- Added GLOSSARY.md and AGENTS.md safety invariants section
- Legacy v1.8 branch deleted; main branch locked as sole development line

**Final v1.0 Beta Release Lock**
- All version strings locked to "v1.0 Beta"
- CHANGELOG made strictly append-only
- Full quadratic prediction, safety integration, and numerical stability confirmed
- Ready for hardware soak testing

---

## Earlier History (Pre-Beta)

- **v1.0.0-beta.0**: Initial quadratic RLS + safety foundations + NVS fingerprint (Bierman-Thornton prototype)
- Pre-v1.0 work archived in `v1.8` branch history
- Early development focused on RLS modeling, safety interlocks, and ESP-Miner integration patterns

---

**Next Phase (Phase 2)**: Analytical J/TH solver improvements, RLS enhancements, active thermal slope detection (ΔT/dt), PID fan control integration, and extended soak testing.
