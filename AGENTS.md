# AGENTS.md — G6 Brain Development Guidelines

## Core Philosophy
G6 Brain is a **safety-critical real-time control system** for Bitcoin mining hardware. Every change must preserve:
- Deterministic 2-second thermal safety response
- Strict Positive Definite (PD) RLS covariance matrix
- PSRAM guard and explicit error paths
- No silent degradation of OCP / DFS / I2C Guardian

## G6 Brain Safety Invariants (DO NOT MODIFY WITHOUT HUMAN REVIEW)
- `brain_safety_task()` period **MUST** remain ≤ 2000 ms (separate from 30s optimizer)
- `RIDGE_EPSILON` **MUST** remain > 0 (strict PD, never just PSD)
- `RLS_WARMUP_TICKS` **MUST** be ≥ 30 (currently 30)
- `VCORE_SOFTWARE_MAX_MV` **MUST** remain ≤ 1250 mV (hardware fuse)
- `PSRAM guard` in `g6_brain_init()` **MUST NOT** be removed or bypassed
- `I2C Guardian` 9-clock + STOP + re-init path **MUST** stay IRAM_ATTR where possible
- No change to `g6_brain_update()` signature without updating all callers and tests

## Required Before Any Commit Touching g6_brain/
1. `idf.py build` succeeds cleanly
2. All Unity tests in `components/g6_brain/test/` pass (when implemented)
3. Manual smoke test on real Gamma 602+ hardware for ≥ 10 minutes
4. Update CHANGELOG.md with timestamped entry
5. Run `python3 tools/check_glossary.py` (future) to ensure new acronyms are defined

## AI-Assisted Development Rules
- Never disable or weaken safety checks to "make it compile"
- Never reduce `RLS_WARMUP_TICKS` below 30 without mathematical proof + hardware soak data
- If you see `SPIRAM_IGNORE_NOTFOUND=y` in any config, flag it immediately
- Prefer adding a Unity test over adding a new feature

## Current Open Critical Items (v5)
See `docs/G6_Brain_QA_Report_v5.docx` for full prioritized list.
Top 3 for next sprint:
1. Separate 2s safety watchdog task (P3)
2. Write 10 g6_brain Unity tests + CI gate (P7)
3. Fix merge_bin_all.sh in upstream fork (P1/P2 — not in this component)

## Contact
All safety-related questions → human maintainer before merging.