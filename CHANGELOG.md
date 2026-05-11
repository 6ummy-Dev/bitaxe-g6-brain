# Changelog

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

**Next Phase (Phase 2):** Advanced telemetry, controlled exploration, and extended soak testing (as previously noted). Documentation will continue to evolve in lockstep with code.

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
