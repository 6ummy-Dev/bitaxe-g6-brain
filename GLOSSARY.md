# G6 Brain Glossary

**NER** — Nonce Error Rate (percentage of invalid/non-winning nonces detected in a window; used for P-VUS predictive voltage compensation)

**P-VUS** — Predictive Voltage Undershooting System (bumps Vcore preemptively based on rising NER to prevent ASIC crashes)

**DFS** — Dynamic Frequency Scaling (soft thermal throttling instead of hard shutdown)

**OCP** — Overcurrent Protection (hard trip on Vcore > max)

**PSD / PD** — Positive Semi-Definite vs Positive Definite (RLS P-matrix must be strict PD to avoid zero-eigenvalue estimator blindness; v3 fix enforces nonzero ridge)

**RLS** — Recursive Least Squares (quadratic response surface modeling for optimal f/v)

**MTBF** — Mean Time Between Failures (requires statistical data from N devices × hours; pending benchmark methodology)

**I2C Guardian** — 9-clock recovery + explicit STOP for EMI-induced bus hangs from ASIC switching

**NVS Wear-Leveling** — Circular buffer logging to extend flash life

**Cold-Start Guard** — Higher λ (0.995) for first 30 ticks to stabilize early RLS estimates
