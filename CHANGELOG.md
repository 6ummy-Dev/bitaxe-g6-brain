# Changelog

## [v1.0 Beta] - 2026-05-10 23:25 EDT

**Final v1.0 Beta Release Lock** — All systems go
- Updated g6_brain.h version define to "v1.0 Beta"
- Updated g6_brain.c: replaced all 1.8.5 references with v1.0 Beta, added safety.h include and safety_check call in update() for full validity
- Fixed g6_safety.c logic bug (now uses model_quality for divergence check)
- Renamed docs/main_integration_1.8.5.c to docs/main_integration_v1.0_beta.c with all references updated
- CHANGELOG locked to append-only v1.0 Beta entries
- All code validated, no 1.8.5 remnants in main branch
- Ready for ESP-Miner integration and 72h+ uptime testing

**Timestamp:** 2026-05-10 23:25:00 -0400

## [v1.0 Beta] - 2026-05-10 19:15 EDT

**Re-pushed & Locked** — Final v1.0 Beta enforcement
- All version strings locked exclusively to "v1.0 Beta"
- CHANGELOG is now strictly append-only with timestamps
- No other version numbers permitted until Beta phase is removed
- Main branch + previous development branches synchronized
- Thread safety, Kconfig slew limits, numerical stability fixes confirmed
- Hardware owner tasks clearly documented

**Timestamp:** 2026-05-10 19:15:00 -0400

## Previous entries (archived for reference)
(Older 1.8.5 development notes moved to v1.8 branch history)

