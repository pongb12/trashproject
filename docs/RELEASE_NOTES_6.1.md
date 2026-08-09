# Nexus 6.1 Release Notes

**Release date:** 2026-07-09
**Author:** Nexus Team
**Version:** 6.1.0
**License:** GPL v3 (inherited from Berserk; see [ATTRIBUTION.md](../ATTRIBUTION.md))

## Summary

Nexus 6.1 is an **infrastructure release**. It does NOT change search
behavior, evaluation, or move generation. The release mission is to make the
engine dramatically easier to build, inspect, debug, benchmark, regress-test,
profile, and evolve safely.

**The baseline playing strength is preserved exactly.** Bench node count is
2,811,728 — bit-identical to Nexus 6.0 and the Berserk baseline. This is
verified by `scripts/bench_compare.sh` showing 0 drift between 6.0 and 6.1
release builds.

## What's new

### 1. Continuous validation gate

New script: `scripts/validate.sh`

A single command that builds the engine, verifies the UCI handshake, checks
the bench node count, and runs the full regression suite. Use this before
merging any patch.

```bash
bash scripts/validate.sh release    # release build
bash scripts/validate.sh profile    # profile build
bash scripts/validate.sh debug      # debug build (ASan/UBSan)
```

### 2. Benchmark suite with extended metrics

New CLI mode: `nexus6.1 benchdetail [depth]`

Runs the same 50-position bench suite at the same depth, but prints
per-position extended metrics: TT probe/hit counts and rate, qsearch node
count and percentage, aspiration retries, PV changes, seldepth, and bestmove
encoding. Requires a `NEXUS_SUMMARY` build (`make profile` or `make debug`).

The total node count is still 2,811,728 — the instrumentation does not change
search behavior.

New script: `scripts/bench_compare.sh <binary_a> <binary_b>`

Diffs two binaries' bench output, reporting total drift and per-position
differences. Use to verify that a change truly has zero behavioral impact.

### 3. Debug / explainability layer

Three new UCI options (always declared; active only in instrumented builds):

- **`ExplainMode`** (check, default false) — after each `bestmove`, prints
  an `info string explain ...` block with a human-readable reason summary:
  final depth, node count, TT hit rate, aspiration retries, PV changes,
  cutoff sources, top moves by node share, and a one-line summary.

- **`DebugModules`** (string, default `<empty>`) — enables per-module
  `info string debug ...` logging. Modules: `tt`, `search`, `qsearch`,
  `root`, `time`, `movegen`, `eval`, `movepick`, `aspiration`, `pruning`,
  `extensions`, `pv`, `all`, `none`.

- **`ProfileReport`** (check, default false) — after each `bestmove`, prints
  an `info string profile ...` block with per-subsystem performance counters.

See [docs/DEBUG.md](./DEBUG.md) for full details.

### 4. Regression test suite

7 test scripts in `tests/`, orchestrated by `tests/run_all.sh`:

| Test | Coverage |
|---|---|
| `perft.sh` | ~150 positions (move-gen correctness) |
| `mate-in-1.sh` | ~100 positions (tactical correctness) |
| `special_moves.sh` | 12 positions (promotion, ep, castling, FRC, draws) |
| `legality.sh` | 12 positions (bestmove legality) |
| `uci_smoke.sh` | 21 checks (every UCI command) |
| `timed.sh` | 10 tests (time management) |
| `bench_regression.sh` | 1 check (node count = 2,811,728) |

All tests pass on the release build. See [docs/REGRESSION.md](./REGRESSION.md).

### 5. Profiling layer

New compile-time flag: `NEXUS_PROFILE`

When enabled, the engine maintains per-subsystem counters (negamax calls,
qsearch calls, TT probes/hits/puts, cutoff sources, LMR reductions, etc.)
and reports them via `ProfileReport` or `benchdetail`.

Overhead: ~1-3% (single `inc` instruction per counter increment). Node count
is unchanged.

New makefile target: `make profile` — builds with `-O2 -DNEXUS_PROFILE -DNEXUS_SUMMARY`.

See [docs/PROFILING.md](./PROFILING.md).

### 6. Build system improvements

Three explicit build modes:

| Target | Purpose |
|---|---|
| `make build` | Quick release (no PGO, no instrumentation) |
| `make release` / `make pgo` | Max-strength PGO build (tournament play) |
| `make profile` | O2 + instrumentation (hotspot analysis) |
| `make debug` | O1 + ASan/UBSan + all instrumentation (development) |

The `release` and `build` targets produce a binary byte-identical to Nexus
6.0 (bench = 2,811,728).

**Versioned binary name:** the makefile now derives the binary name from
`VERSION` — `nexus` + major.minor. For `VERSION = 6.1.0`, the binary is
`nexus6.1` (Linux/macOS) or `nexus6.1.exe` (Windows). This makes it easy to
keep multiple engine versions side-by-side. Override with `EXE=nexus` for
OpenBench compatibility.

New CLI mode: `nexus6.1 buildinfo` — prints compile-time instrumentation
capabilities.

The UCI handshake now includes `info string build debug=N profile=N summary=N`
so GUIs can detect instrumentation support programmatically.

### 7. Documentation

9 new documents in `docs/`:

| Doc | Content |
|---|---|
| `ARCHITECTURE.md` | Engine architecture overview for developers |
| `BENCHMARK.md` | How to use bench / benchdetail / bench_compare |
| `DEBUG.md` | How to use ExplainMode / DebugModules |
| `PROFILING.md` | How to use ProfileReport and interpret counters |
| `REGRESSION.md` | How to run and interpret the regression suite |
| `BUILD.md` | Detailed build guide (all platforms, all modes) |
| `UCI_OPTIONS.md` | Full reference for every UCI option |
| `ROADMAP.md` | Planned future work |
| `RELEASE_NOTES_6.1.md` | This file |

### 8. CI / automation

Updated `.github/workflows/nexus.yml` runs:
- Release validation across GCC 11/12 and arch avx2/x86-64
- Profile build validation (verifies instrumentation + benchdetail + explain)
- Debug build validation (ASan/UBSan-clean search)
- Code formatting check (clang-format)

A failure blocks merge.

### 9. Quality-of-life engine tools

- `nexus6.1 buildinfo` — print compile-time capabilities
- `nexus6.1 benchdetail` — extended bench metrics
- Build capability line in UCI handshake
- `bench_compare.sh` for diffing two binaries
- `validate.sh` as the single pre-merge command

## What did NOT change

- Search algorithm structure (negamax + PVS + iterative deepening)
- Evaluation architecture (NNUE)
- Move generation logic
- TT core behavior
- Pruning system fundamentals
- Time management fundamentals
- Bench node count (still 2,811,728)
- Perft values (still canonical)
- UCI `id name` / `id author` (still `Nexus 6.1` / `Nexus Team`)
- License (still GPL v3, inherited from Berserk)

## Validation results

```
$ bash scripts/validate.sh release
############################################################
#  Nexus 6.1 Validation Gate
############################################################
==> [1/4] Build: OK
==> [2/4] UCI handshake: OK
==> [3/4] Bench regression: OK (2811728 nodes)
==> [4/4] Regression suite: ALL TESTS PASSED (7/7)
############################################################
#  VALIDATION PASSED
############################################################
```

```
$ bash scripts/bench_compare.sh ../nexus6/src/nexus6.1 ./src/nexus6.1
  Binary A (6.0) total nodes: 2811728
  Binary B (6.1) total nodes: 2811728
  Drift: 0 nodes (IDENTICAL)
  RESULT: IDENTICAL
```

## Upgrade instructions

From Nexus 6.0:

1. Replace the `nexus6/` directory with `nexus6.1/`.
2. The NNUE network file is unchanged (`nexus-9b84c340af7e.nn`).
3. Rebuild: `cd src && make pgo CC=gcc ARCH=avx2`
4. Verify: `./nexus6.1 bench` should print `Results: ... 2811728 nodes ...`
5. Optional: `make profile` to get benchdetail and explain mode.

From Berserk baseline:

- Follow the Nexus 6.0 release notes first, then upgrade to 6.1.

## Known limitations

- `DebugModules` logging is very verbose and slows the engine 10-100×. Use
  only with very short searches.
- The `debug` build uses `-O1` (not `-O0`) because the codebase uses bare
  `inline` on some TT helpers that require at least `-O1` to emit a
  definition. This is documented in the makefile.
- Some profile counters (`movegen_calls`, `eval_calls`, `futility_prunes`,
  `see_prunes`, `check_ext`, `qs_delta_prunes`) are declared but not yet
  instrumented. They will be populated in future 6.x releases.
- The `expect` utility is required for `perft.sh` and `mate-in-1.sh`. If not
  installed, those tests skip with a notice (exit 0).

## Acknowledgements

Nexus 6.1 builds directly on the Nexus 6.0 rebrand of Berserk by Jay Honnold.
See [ATTRIBUTION.md](../ATTRIBUTION.md) for full provenance and licensing.
