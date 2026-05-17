# References & Scientific Foundations

G6 Brain builds on well-established research in **adaptive control**, **system identification**, and **response surface methodology**. All code is original implementation.

This file provides proper academic credit and stable references for the core techniques used.

---

## Recursive Least Squares (RLS) & Adaptive Filtering

- **Haykin, S.** (2014). *Adaptive Filter Theory* (5th ed.). Pearson.  
  Foundational text on RLS algorithms, including variable forgetting factor and numerical stability techniques.

- **Bierman, G. J.** (1977). *Factorization Methods for Discrete Sequential Estimation*. Academic Press.  
  Classic reference for numerically stable RLS implementations (UD factorization, square-root filtering).

- **Ljung, L.** (1999). *System Identification: Theory for the User* (2nd ed.). Prentice Hall.  
  Standard reference for real-time system identification and recursive estimation.

- **Åström, K. J., & Wittenmark, B.** (1995). *Adaptive Control* (2nd ed.). Addison-Wesley.  
  Foundational work on adaptive control using RLS for parameter estimation.

---

## Quadratic Response Surface Methodology (RSM)

- **Box, G. E. P., & Wilson, K. B.** (1951). "On the Experimental Attainment of Optimum Conditions". *Journal of the Royal Statistical Society. Series B (Methodological)*, 13(1), 1–45.  
  https://doi.org/10.1111/j.2517-6161.1951tb00067.x  
  The original paper that introduced response surface methodology.

- **Myers, R. H., Montgomery, D. C., & Anderson-Cook, C. M.** (2016). *Response Surface Methodology: Process and Product Optimization Using Designed Experiments* (4th ed.). Wiley.  
  Modern comprehensive textbook on quadratic RSM and optimization.

---

## Real-Time Optimization & Safety in Embedded Systems

- **Ljung, L., & Söderström, T.** (1983). *Theory and Practice of Recursive Identification*. MIT Press.  
  Practical aspects of implementing recursive estimators in real-time systems.

- **Goodwin, G. C., & Sin, K. S.** (1984). *Adaptive Filtering, Prediction and Control*. Prentice Hall.  
  Covers stability, convergence, and practical issues in adaptive control.

- **Åström, K. J., & Murray, R. M.** (2008). *Feedback Systems: An Introduction for Scientists and Engineers*. Princeton University Press.  
  Modern treatment of feedback, stability, and safety in control systems.

---

## Notes

- The G6 Brain implementation uses a **quadratic RLS model** with **variable forgetting factor**, **ridge regularization**, and **predictive safety layers** inspired by the above works.
- All numerical stability techniques (PSD safeguard, trace monitoring, denominator guards) draw from Bierman and Haykin.
- Safety interlocks (thermal ceiling, voltage undershoot prediction, slew limiting) follow fail-safe principles common in aerospace and automotive embedded control literature.
- No proprietary algorithms or closed-source code from any paper have been used.

---

**Last updated:** May 2026  
**Maintainer:** 6ummy-Dev + Grok (xAI)
