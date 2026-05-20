# Bitaxe G6 Brain Manifesto

**For new contributors, collaborators, and future maintainers**  
*Version 1.1 — May 2026*

We are building **the most trustworthy, mathematically sound, and modular control brain** for the Bitaxe ecosystem.

### 1. Our Why

Solo miners and people learning Bitcoin mining hardware deserve better tools than cowboy voltage tables and opaque autotuners.

Every chip is different. Every power supply is imperfect. Every cooling solution varies.

Our job is to give each individual ASIC the **highest stable hashrate possible while guaranteeing zero hardware risk**. We do this through rigorous math, strong safety layers, and a clean modular design that others can actually use and learn from.

> **"Start safe. Learn. Then optimize."**

This is not marketing. It is the non-negotiable operating principle.

### 2. Core Philosophy

- **Math first.** We model the system using stabilized Recursive Least Squares and response surface methods. No black boxes. Every parameter has meaning and can be inspected.
- **Modularity is sacred.** The brain is a self-contained, swappable module with a clean public API. It must be easy to integrate into existing firmware with minimal glue. This is how we serve both educational use and real production deployments.
- **Highest stable hashrate with zero hardware risk.** This is the primary objective. We will not chase marginal hashrate gains that introduce any meaningful risk to the ASIC or power delivery.
- **Safety is engineered, not added later.** Multiple independent safety layers run on every decision. Predictive techniques are welcome when they increase safety without adding complexity or coupling.
- **Educational + Production ready.** The brain should be something a solo miner can flash and trust, while also being clean enough for others to study and extend.
- **Simplicity and elegance over cleverness.** We prefer clear, maintainable code with excellent QA over complex architectures. We lay our reputation on testing, review, and defensive design.
- **Transparency.** We document limitations honestly. We do not oversell current capabilities.

### 3. Non-Negotiable Technical Principles

- The brain remains a **modular component**. It does not take ownership of hardware control, fan PWM, or mining loops. It recommends. The integrator applies.
- **Single-threaded** by design. Caller serializes all calls.
- Backward compatibility on the public API is taken seriously.
- All tunables go through Kconfig.
- NVS schema versioning is mandatory. Warm-start must survive struct evolution.
- Every safety check executes even on invalid or rejected samples.
- Tests (especially safety paths and edge cases) are required.
- Documentation is part of the deliverable (README, API.md, MONITORING.md, AGENTS.md, this manifesto).

### 4. Scope & Roadmap Intent

- **v1 goal**: A production-ready, modular brain focused on highest stable hashrate with zero hardware risk. Strong math, strong safety, clean integration, excellent diagnostics.
- Thermal awareness, predictive elements, and model health monitoring are in scope for v1 **if** they serve the primary goal without breaking modularity or elegance.
- **v1.5+**: Puzzle solver features, on-device stochastic exploration, and more advanced active learning techniques.
- Full firmware-level improvements and deeper integration are explicitly **out of scope** for the initial brain releases. We can address the broader firmware later.

We finish the brain module to a high standard before expanding scope.

### 5. How to Contribute

1. Read AGENTS.md first. The safety invariants live there.
2. Respect math and modularity. Changes that increase coupling or replace sound estimation with heuristics will be rejected.
3. Default to the safest behavior.
4. Keep the brain swappable and easy to integrate.
5. Write tests. Especially for safety and numerical edge cases.
6. Update documentation when behavior changes.
7. Be honest about experimental vs. production-ready code.
8. Run the full test suite before opening a PR.

PRs that violate these principles will be closed with a link back to this document.

### 6. Final Words

This project exists to raise the quality bar for Bitaxe tooling.

We are not trying to build the most aggressive optimizer.  
We are trying to build the **most trustworthy and usable modular brain** that solo miners and learners can actually depend on.

If you value rigorous math, clean modularity, real safety, and shipping something solid over flashy claims — welcome.

We move deliberately. We test heavily. We optimize only after safety is engineered.

**Made with care for the people who actually run this hardware**  
— 6ummy + contributors who respect the mission
