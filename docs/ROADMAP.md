# Nexus 6.1 Roadmap

This document tracks the planned evolution of the Nexus engine family. It is
intentionally conservative — Nexus 6.1 is an infrastructure release, and
future versions will build on this foundation.

## Release philosophy

Each Nexus release follows these principles:

1. **Strength, correctness, stability come first.** A patch that improves one
   metric but breaks another is rejected.
2. **Every change is validated.** The validation gate (`scripts/validate.sh`)
   must pass before merge.
3. **No fake metrics.** Depth, nodes, NPS, PV, and all debug output must be
   honest and derived from actual computation.
4. **Small, measured steps.** No large rewrites; each patch should be
   reviewable in one sitting.
5. **Bench is the sentinel.** The 2,811,728-node bench count is the canonical
   regression check. Intentional changes update the expected value and
   document the change.

## Version history

### Nexus 6.0 (initial rebrand from Berserk)

- Rebranded Berserk → Nexus 6.0 (identity only, zero behavioral changes)
- Verified bench = 2,811,728 (bit-identical to Berserk baseline)
- Full UCI command set tested (21/21 smoke tests pass)
- ATTRIBUTION.md consolidates all upstream licenses
- README.md with MSYS2-UCRT64 build guide

### Nexus 6.1 (this release — infrastructure)

- **Continuous validation:** `scripts/validate.sh` runs build + UCI + bench +
  full regression suite
- **Benchmark suite:** `benchdetail` CLI mode with extended per-position
  metrics (TT hit rate, qsearch %, aspiration retries, PV changes)
- **Debug / explainability:** `ExplainMode` UCI option prints human-readable
  reason summary; `DebugModules` enables per-module logging
- **Profiling:** `ProfileReport` UCI option prints per-subsystem counters;
  `make profile` build mode
- **Regression testing:** 7 test scripts covering perft, mate, special moves,
  legality, UCI, timing, bench
- **Build system:** `make debug` / `make profile` / `make release` targets
- **Documentation:** 9 doc files (architecture, benchmark, debug, profiling,
  regression, build, UCI options, roadmap, release notes)
- **CI / automation:** GitHub Actions workflow runs validation gate on every
  push/PR across multiple GCC versions and architectures
- **Quality of life:** `buildinfo` CLI mode, build capability line in UCI
  handshake, `bench_compare.sh` for diffing two binaries
- **Strength preserved:** bench = 2,811,728 (0 drift vs 6.0)

## Planned for future versions

### Nexus 6.2 — Search tuning (SPRT-gated)

Now that the validation infrastructure exists, we can safely tune search
parameters. Every change must pass SPRT testing before merge.

Candidates:
- LMR table re-tuning
- History heuristic scaling
- Aspiration window sizing
- Null-move reduction formula
- Singular extension thresholds

**Constraint:** No architectural changes — only parameter tuning. Every patch
must keep bench node count stable OR be justified by SPRT results.

### Nexus 6.3 — Time management refinements

- Better time allocation in sharp tactical positions
- Improved panic-mode handling
- More granular stability factors
- Time-management-specific regression tests

**Constraint:** Must not introduce hangs, deadlocks, or time losses. The
`tests/timed.sh` battery will be extended.

### Nexus 6.4 — Evaluation refinements

- NNUE network upgrades (new trainer, new network)
- Eval-time correction history improvements
- Optional HCE (handcrafted evaluation) for debugging

**Constraint:** No NNUE architecture changes. New networks must be SPRT-tested
against the baseline.

### Nexus 6.5 — Opening handling (optional)

If a usable opening system can be added without weakening play:
- Polyglot book support (read-only, as move-ordering hint)
- Book hint integration with search (not replacement)
- Strict policy: book never overrides a clearly better search move

**Constraint:** Opening support must NOT weaken real play. If SPRT shows any
strength loss, the feature is reverted.

### Nexus 7.0 — Architecture evolution (long-term)

Only after 6.x has proven stable and the validation infrastructure is mature:
- Search architecture review (informed by profiling data from 6.1)
- TT core behavior review
- Possible NNUE architecture upgrade
- Threading model review

This is explicitly OUT OF SCOPE for 6.x.

## What will NOT happen

- **No fake strength claims.** Every Elo number must come from SPRT testing.
- **No cosmetic depth inflation.** Depth must be real.
- **No silent regressions.** Bench drift must be documented.
- **No untested merges.** Validation gate is mandatory.
- **No reuse of Nexus 1-5 code.** Nexus 6+ is a fresh lineage from Berserk.

## How to propose a change

1. Open an issue describing the proposed change and expected impact.
2. Implement on a feature branch.
3. Run `bash scripts/validate.sh release` locally — must pass.
4. If the change affects bench node count, update
   `tests/bench_regression.sh` and document why.
5. If the change may affect strength, run SPRT testing (via OpenBench or
   equivalent) and include results in the PR.
6. CI must pass on all matrix entries.
7. Merge only after review.

## Long-term vision

Nexus 6.x is becoming a strong, measurable, inspectable development platform.
The goal is not to be the #1 engine — it is to be an engine that is safe to
modify, easy to benchmark, and honest in all its output. Strength gains will
come from careful, measured improvements on this foundation.
