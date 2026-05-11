# G6 Brain Glossary

**NER** — Nonce Error Rate. Percentage of invalid or duplicate nonces submitted by the ASIC (Z-nonces / total nonces). Used by P-VUS to predict voltage undershoot before crashes.

**P-VUS** — Predictive Voltage Undershoot System. Monitors NER and proactively increases Vcore slightly before error rate spikes cause hardware faults.

**DFS** — Dynamic Frequency Scaling. Automatically reduces ASIC frequency when temperature or error rate exceeds safe thresholds.

**OCP** — Over-Current Protection. Hardware or firmware trip that parks the miner if Vcore or current exceeds safe limits.

**RLS** — Recursive Least Squares (forgetting-factor variant). Adaptive algorithm that maintains a quadratic model of hashrate vs. frequency × voltage.

**PID** — Proportional-Integral-Derivative controller. Used for fan speed and thermal management with feed-forward power prediction.

**PD** — Positive Definite. Matrix property where all eigenvalues are strictly > 0. Required for stable RLS learning (nonzero ridge regularization guarantees this).

**PSD** — Positive Semi-Definite. Eigenvalues ≥ 0 (includes zero). Insufficient for RLS — allows permanent learning blindness in some dimensions.

**MTBF** — Mean Time Between Failures. Statistical reliability metric. Current claims are aspirational; measured data pending 72h+ soak tests.

**Cold-start guard** — Initial period (currently 30 ticks / 15 min) where RLS uses higher forgetting factor (λ=0.995) for numerical stability while the P-matrix conditions.

**RIDGE_EPSILON** — Small positive constant (currently 1e-5) added to diagonal of P after every update to guarantee strict PD and prevent zero-eigenvalue lockup.

**I2C Guardian** — 9-clock recovery + explicit STOP condition + heartbeat check to recover from EMI-induced bus hangs caused by high-current ASIC switching.

**Feed-forward** — Predictive term in PID that uses dP/dt (power change) and estimated junction temperature to preemptively adjust fan/Vcore before temperature error appears.

**Junction temperature (Tj)** — Estimated die temperature using T_j ≈ T_ambient + Power × θ_ja (θ_ja ≈ 15°C/W for typical Bitaxe heatsink).

**W/TH** — Watts per Terahash. Primary efficiency metric. Reported with fixed-point arithmetic to eliminate floating-point drift in long-running logs.