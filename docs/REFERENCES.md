# References & Scientific Foundations

**G6 Brain**

G6 Brain builds on well-established research in **adaptive control**, **system identification**, and **response surface methodology**. All code is original implementation.

This file provides proper academic credit and stable references for the core techniques used.

---

## Recursive Least Squares (RLS) & Adaptive Filtering

- **Haykin, S.** (2014). *Adaptive Filter Theory* (5th ed.). Pearson.  
  Foundational text on RLS algorithms, variable forgetting factor mechanisms, and tracking stability.

- **Bierman, G. J.** (1977). *Factorization Methods for Discrete Sequential Estimation*. Academic Press.  
  Classic reference for numerically stable recursive estimation and matrix factorization principles.

- **Ljung, L.** (1999). *System Identification: Theory for the User* (2nd ed.). Prentice Hall.  
  Standard reference for real-time system identification and recursive parameter estimation.

- **Åström, K. J., & Wittenmark, B.** (1995). *Adaptive Control* (2nd ed.). Addison-Wesley.  
  Foundational work on adaptive controllers using recursive estimators for parameter tracking.

---

## Numerical Stability & Robust Estimation (beta3 Upgrades)

- **Bucy, R. S., & Joseph, P. D.** (1968). *Filtering for Stochastic Processes with Applications to Guidance*. Interscience Publishers.  
  Introduces the mathematically stabilized Joseph Form covariance equation, ensuring symmetry and positive semi-definiteness under truncation limits.

- **Peter S. Maybeck** (1979). *Stochastic Models, Estimation, and Control* (Vol. 1). Academic Press.  
  Detailed engineering analysis of the Joseph Form covariance stabilization loop in sequential digital estimators.

- **Bar-Shalom, Y., Li, X. R., & Kirubarajan, T.** (2001). *Estimation with Applications to Tracking and Navigation*. Wiley.  
  Establishes the foundational theory for innovation variance gating and mathematical outlier thresholds (3-Sigma validation regions).

---

## Quadratic Response Surface Methodology (RSM)

- **Box, G. E. P., & Wilson, K. B.** (1951). On the Experimental Attainment of Optimum Conditions. *Journal of the Royal Statistical Society. Series B (Methodological)*, 13(1), 1–45.  
  The original paper introducing empirical response surface behavior.

- **Myers, R. H., Montgomery, D. C., & Anderson-Cook, C. M.** (2016). *Response Surface Methodology: Process and Product Optimization Using Designed Experiments* (4th ed.). Wiley.  
  Comprehensive text on quadratic modeling surfaces and constrained local maximization.

---

## Real-Time Optimization & Safety in Embedded Systems

- **Goodwin, G. C., & Sin, K. S.** (1984). *Adaptive Filtering, Prediction and Control*. Prentice Hall.  
  Covers convergence properties and execution safety invariant guard patterns in real-time environments.

- **Åström, K. J., & Murray, R. M.** (2008). *Feedback Systems: An Introduction for Scientists and Engineers*. Princeton University Press.  
  Modern treatment of closed-loop stability limits, execution order priorities, and protective constraints.

---

## Analytical J/TH Optimization (Fractional Programming)

- **Dinkelbach, W.** (1967). On nonlinear fractional programming. *Management Science*, 13(7), 492–498.  
  Foundational paper on Dinkelbach’s algorithm for fractional programming. Forms the algebraic basis for the O(1) parametric solver.

- **Boyd, S., & Vandenberghe, L.** (2004). *Convex Optimization*. Cambridge University Press.  
  General reference for sub-problem convexity analysis, Cramer's matrix solutions, and quadratic forms.

---

**Last updated:** May 2026  
**Maintainer:** 6ummy-Dev
