# Changelog

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

Phase 1 is now complete. The core RLS implementation meets the stability, safety, and modularity requirements identified in the technical audits.

**Next Phase (Phase 2):** Advanced telemetry buffering, controlled exploration policy, and extended soak testing.

## [v1.0 Beta] - 2026-05-11

**Major RLS Hardening**

- Added Bierman-Thornton style numerical stability improvements
- Implemented sample quality state machine
- Added efficiency-based optimization objective
- Added NVS-based per-chip warm-start (silicon fingerprint)
- BM1370 tuning and safety limits applied

## [v1.0 Beta] - 2026-05-10

**Initial RLS Core and Safety Foundations**

- Initial quadratic RLS implementation with real-time modeling
- Basic safety interlocks (thermal, voltage, NER)
- NVS persistence foundation
- Stochastic nonce support
- Modular component structure established

Previous development history (v1.8 branch and earlier) has been archived in commit history.
