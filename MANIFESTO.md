# Bitaxe G6 Brain Manifesto

**For new contributors, collaborators, and future maintainers**  
*Version 1.0 — May 2026*

We are not building another miner autotuner.  
We are building **the most trustworthy, mathematically sound, and safety-first control brain** the Bitaxe ecosystem has ever seen.

### 1. Our Why

The Bitaxe community deserves better than cowboy hill-climbing scripts and “trust me bro” voltage tables.  
Every chip is unique. Every power supply is dirty. Every heatsink is different.  
Our job is to give each individual ASIC the best possible life while **never** compromising hardware longevity.

> **"Start safe. Learn. Then optimize."**

This single sentence is the unbreakable core of everything we ship.

### 2. Core Philosophy

- **Math first, heuristics second.**  
  We model the hashrate surface with stabilized Recursive Least Squares (RLS) + quadratic response surface methodology. No black-box ML. No magic. Every coefficient has meaning.

- **Safety is not a feature — it is the foundation.**  
  Thermal protection, NER back-off, voltage clamping, sample quality gates, and fail-closed design run on *every* update, even on rejected samples.

- **Modularity above all.**  
  The brain must remain a clean, swappable component. No tight coupling to any specific miner firmware. Public API is sacred.

- **Transparency and honesty.**  
  We document limitations in public (safe hashrate maximizer today, true J/TH efficiency in Phase 1). We never oversell.

- **Aerospace-grade discipline on hobby hardware.**  
  Defensive programming, numerical stability, versioned NVS persistence, self-tests, extensive logging, and CI that actually matters.

### 3. Non-Negotiable Technical Principles

- **Single-threaded only.** No mutexes. Caller must serialize all calls to `update()`, `get_optimal()`, `reset()`, etc.
- **Backward compatibility is law.** Public API changes only with major version bump.
- **Kconfig is the source of truth.** All tunables must be exposed there.
- **NVS warm-start must survive struct evolution** (schema versioning is mandatory).
- **Every safety layer runs even on invalid samples** (`goto safety_layer` pattern).
- **Tests are not optional.** New code must not break existing Unity tests.
- **Documentation is part of the code.** README, API.md, MONITORING.md, AGENTS.md, and this manifesto must stay current.

### 4. How to Contribute (The Rules)

1. **Read AGENTS.md first.** It contains the living safety invariants.
2. **Respect the math.** Do not replace RLS with a PID or simple hill-climber unless you can mathematically prove superiority *and* maintain stability guarantees.
3. **Fail safe by default.** Any new feature must default to the safest possible behavior.
4. **Keep it modular.** The brain should work in any ESP-IDF project with minimal glue.
5. **Write tests.** Especially for safety paths and edge cases.
6. **Update documentation.** If you change behavior, update the relevant .md files.
7. **Be honest.** If something is still experimental, say so in comments and changelog.
8. **Run the full test suite locally** before opening a PR.

Pull requests that violate these principles will be closed with love and a link back to this manifesto.

### 5. Development Phases (We move deliberately)

- **Phase 0 / 0.1** → Stable, safe hashrate maximizer with warm-start and full safety layer (where we are now).
- **Phase 1** → True J/TH efficiency optimization + power surface modeling.
- **Phase 2** → Active thermal slope (ΔT/dt), PID fan control, advanced P-VUS, controlled exploration.
- **Phase 3+** → Multi-ASIC coordination, predictive maintenance, etc.

We finish one phase cleanly before rushing into the next.

### 6. Final Words

This project is open-source but not “move fast and break things.”  
We move **deliberately, rigorously, and with extreme care for the hardware** that miners trust with their money and time.

If you share this engineering mindset — welcome.  
If you want to ship the next flashy “AI optimizer” without safety or math — this is not the repo for you.

We are building the gold standard brain for the Bitaxe ecosystem.  
Let’s make it something future generations of miners will still respect.

**Made with ❤️ and rigorous engineering for the Bitaxe community**  
— 6ummy-Dev + Grok (xAI) + every contributor who respects the manifesto
