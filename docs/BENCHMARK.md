# Nexus 6.1 Benchmark Guide

This document explains how to use the Nexus 6.1 benchmark suite to measure
engine performance, detect regressions, and compare builds.

## Three benchmark modes

Nexus 6.1 provides three benchmark entry points:

### 1. `nexus6.1 bench [depth]` — standard bench (regression sentinel)

OpenBench-compatible. Runs the fixed 50-position suite from
`src/files/bench.csv` at the given depth (default 13). Prints one line per
position plus a `Results:` summary.

```
$ ./src/nexus6.1 bench
Bench [# 1]: bestmove  d1e1 score   -29        16809 nodes  1680900 nps | r3k2r/2pb1ppp/2pp1q2/p7/1nP1B3/1P2P3/P2N1PPP/R2QK2R w KQkq a6 0 14
...
Bench [#50]: bestmove  h1h7 score   150        45026 nodes  1731769 nps | 2r2b2/5p2/5k2/p1r1pP2/P2pB3/1P3P2/K1P3R1/7R w - - 23 93

Results:                                     2811728 nodes  1798930 nps
```

**The total node count (2,811,728 at default depth) is the canonical regression
sentinel.** Any drift indicates a behavioral change in search, evaluation, or
movegen. The `tests/bench_regression.sh` script enforces this automatically.

### 2. `nexus6.1 benchdetail [depth]` — extended bench with instrumentation

Nexus 6.1 addition. Runs the SAME 50 positions at the SAME depth, but with
per-position extended metrics. Requires a `NEXUS_SUMMARY` build (`make profile`
or `make debug`). In a release build, it falls back to standard `bench` output
with a notice.

```
$ make profile CC=gcc ARCH=avx2
$ ./src/nexus6.1 benchdetail

Nexus 6.1 BenchDetail (depth 13) — extended per-position metrics
Build: debug=0 profile=1 summary=1

BenchDetail [# 1]: depth 13 seldepth 17 nodes 16809 tt_probes 17420 tt_hits 8765 (50.3%) qsearch_nodes 7213 (42.9%) aspiration_retries 12 pv_changes 3 bestmove_fromto 31 score -29 | r3k2r/2pb1ppp/...
...
BenchDetail Results: 2811728 nodes  1402358 nps (total time 2004 ms)
(node count must match standard Bench() = 2,811,728 at default depth)
```

Per-position fields:
- `depth` / `seldepth` — final iteration depth and selective depth
- `nodes` — total nodes searched
- `time_ms` — wall-clock time spent on this position (milliseconds)
- `nps` — nodes per second for this position (derived from `nodes` / `time_ms`)
- `tt_probes` / `tt_hits` / `hit_rate` — transposition table usage
- `qsearch_nodes` / `qsearch_pct` — quiescence search load (% of total nodes)
- `aspiration_retries` — number of aspiration window re-searches
- `pv_changes` — how many times the best move changed across iterations
- `bestmove_fromto` — the chosen move in from-to square encoding (0-4095)
- `score` — final score in centipawns (normalized)

The `time_ms` and `nps` fields let you identify slow positions and compare
per-position throughput across builds, which is useful for detecting
performance regressions on specific position types.

**The total node count must still be 2,811,728** — the instrumentation does not
change search behavior, only adds counters.

### 3. `nexus6.1 buildinfo` — compile-time capabilities

Prints which instrumentation flags were active at compile time. Useful for
confirming you're running the right build before interpreting benchdetail output.

```
$ ./src/nexus6.1 buildinfo
Nexus 6.1 build capabilities:
  NEXUS_DEBUG    : disabled
  NEXUS_PROFILE  : enabled
  NEXUS_SUMMARY  : enabled
```

## What the bench measures

The standard `bench` command measures:

| Metric | What it tells you |
|---|---|
| Total nodes | Search tree size — regression sentinel (must be 2,811,728) |
| NPS | Raw search speed — depends on CPU, compiler, flags, PGO |
| Per-position bestmove | Whether the engine picks the same move on every build |
| Per-position score | Whether evaluation agrees across builds |

The `benchdetail` command additionally measures:

| Metric | What it tells you |
|---|---|
| TT probe/hit rate | How effectively the TT is being used (typical: 30-60%) |
| Qsearch node % | How much of the search is spent in quiescence (typical: 30-50%) |
| Aspiration retries | How often the aspiration window was too tight (lower = better) |
| PV changes | How stable the best move was across iterations (lower = more stable) |

## Comparing two builds

Use `scripts/bench_compare.sh` to diff two binaries:

```
$ bash scripts/bench_compare.sh ./nexus-6.0 ./nexus-6.1
############################################################
#  Nexus Bench Comparison
#  A: ./nexus-6.0
#  B: ./nexus-6.1
############################################################
...
############################################################
#  SUMMARY
############################################################
  Binary A total nodes: 2811728
  Binary B total nodes: 2811728
  Drift: 0 nodes (IDENTICAL)

############################################################
#  PER-POSITION DIFF
############################################################
  (no per-position differences)

  RESULT: IDENTICAL
```

Exit code 0 = identical, 1 = drift detected. Use this to verify that a change
truly has zero behavioral impact.

## When the bench number changes

If `bench` produces a number other than 2,811,728, one of these happened:

1. **Intentional search change** — you modified pruning, extensions, LMR, etc.
   Update `EXPECTED` in `tests/bench_regression.sh` and document the change in
   `docs/RELEASE_NOTES_*.md`.
2. **Unintentional regression** — you accidentally changed movegen, TT, or eval.
   Revert immediately and bisect.
3. **Compiler/flag change** — different `-O` level or `-march` can change
   floating-point behavior. Use the same flags as the baseline.
4. **Different depth** — `bench 13` is the canonical depth. `bench 12` gives a
   different number; always specify the depth when comparing.

## Running bench under time pressure

The bench is a fixed-depth search, not a fixed-time search. On a slow CPU it
will take longer but produce the same node count. If you want a quick
sanity check, run `bench 11` (faster, smaller node count — but update the
expected value in `bench_regression.sh`).

## Bench positions

The 50 bench positions live in `src/files/bench.csv`. They are derived from
Ethereal's bench set (which Berserk inherited). The positions cover:

- Opening positions (startpos, Ruy Lopez, etc.)
- Middlegame tactics (Kiwipete, etc.)
- Endgames (KvK, KRvK, etc.)
- Quiet positions (no immediate tactics)
- Sharp tactical positions (sacrifices, forced sequences)

**Do not modify `bench.csv` casually** — changing the position set changes the
canonical node count and breaks every regression check across the project's
history. If you must change it, do so in a dedicated commit that updates the
expected value everywhere.
