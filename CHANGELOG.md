# Changelog

## [v1.8.5] - 2026-05-10
### Added
- Full production RLS quadratic optimizer with numerical stability and ridge regularization
- Dedicated g6_safety module with hard clamps and divergence watchdog
- Kconfig for build-time tuning
- Slew-rate limited auto_step
- Matrix helpers and ESP32-optimized ops
- Basic Unity test skeleton ready

### Changed
- Version bumped to 1.8.5
- Safety integrated into every update

### Fixed
- Removed placeholder code
- Added real NVS stubs and thermal predictor

No hype - this is now a solid, safe, production-grade adaptive brain for Bitaxe Gamma 602.
