# AGENTS.md — G6 Brain Safety Invariants

## DO NOT modify without explicit human engineering review:
- brain_safety_task() period: MUST remain ≤ 2000ms
- OCP hard-trip code path: MUST have IRAM_ATTR
- BRAIN_VCORE_MAX_MV: MUST NOT exceed 1300mV
- RLS ridge constant RIDGE_EPSILON: MUST remain > 0
- PSRAM init guard in g6_brain_init(): MUST NOT be removed
- P-matrix check: MUST enforce positive DEFINITE, not just PSD

## Required before any g6_brain commit:
- idf.py build test -C components/g6_brain/test → all PASS

## QA v3 Notes (May 10, 2026)
- All critical PSD/PD, cold-start, and wording fixes applied
- Remaining high-severity items (safety task split, unit tests, partitions) are next-wave priorities
