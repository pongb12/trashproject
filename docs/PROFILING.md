# Nexus 6.1 Profiling Guide

This document explains how to use the Nexus 6.1 profiling layer to find
performance hotspots.

## Profiling philosophy

Nexus 6.1 takes a low-overhead, compile-time-toggle approach to profiling:

- **Compile-time toggle** — profiling counters are only present when the build
  is compiled with `-DNEXUS_PROFILE`. The default release build has zero
  profiling overhead.
- **Low overhead** — when enabled, each counter increment is a single
  `inc` instruction (~0.3 cycles). The total overhead is typically <2%.
- **Honest output** — counters reflect actual execution, not estimates.

This is intentionally simpler than callgrind/perf-style sampling profilers.
The trade-off: you only see the counters we instrumented, but what you see
is exact.

## Enabling profiling

```bash
cd src
make profile CC=gcc ARCH=avx2
./nexus6.1 buildinfo
# NEXUS_PROFILE  : enabled
# NEXUS_SUMMARY  : enabled
```

The `profile` target uses `-O2` (not `-O3`) and enables both
`NEXUS_PROFILE` and `NEXUS_SUMMARY`. The result is a binary that is slightly
slower than the release build but produces the same node count (counters
don't change control flow).

## Two ways to read the counters

### 1. Per-search report via `ProfileReport` UCI option

After each `bestmove`, the engine prints a counter summary:

```
$ printf 'uci\nsetoption name ProfileReport value true\nposition startpos\ngo depth 15\nquit\n' | ./nexus6.1
...
bestmove e2e4 ponder e7e5
info string profile begin
info string profile negamax_calls      158436
info string profile quiesce_calls      87642
info string profile movegen_calls      0
info string profile eval_calls         0
info string profile tt_probes          162001
info string profile tt_hits            81234
info string profile tt_puts            73102
info string profile tt_score_cuts      14203
info string profile tt_hit_rate_pct    50.14
info string profile aspiration_retries 87
info string profile null_move_cuts     2104
info string profile lmr_reductions     42198
info string profile rfp_cuts           18234
info string profile futility_prunes    0
info string profile see_prunes         0
info string profile singular_ext       312
info string profile check_ext         0
info string profile pv_changes        12
info string profile qs_standpat_cuts  42198
info string profile qs_delta_prunes   8421
info string profile root_move_count   20
info string profile time_checks       1584
info string profile time_panic        0
info string profile end
```

### 2. Per-position report via `benchdetail`

```bash
$ ./nexus6.1 benchdetail | head -5

Nexus 6.1 BenchDetail (depth 13) — extended per-position metrics
Build: debug=0 profile=1 summary=1

BenchDetail [# 1]: depth 13 seldepth 17 nodes 16809 tt_probes 17420 tt_hits 8765 (50.3%) qsearch_nodes 7213 (42.9%) aspiration_retries 12 pv_changes 3 bestmove_fromto 31 score -29 | r3k2r/2pb1ppp/...
```

This runs the 50-position bench suite and prints the same counters per
position, so you can see how profiling metrics vary across position types.

## Counter reference

| Counter | What it counts | Typical value (depth 15 startpos) |
|---|---|---|
| `negamax_calls` | Negamax() entries | ~150k |
| `quiesce_calls` | Quiesce() entries | ~80k (40-50% of negamax) |
| `movegen_calls` | (reserved, not currently instrumented) | 0 |
| `eval_calls` | (reserved, not currently instrumented) | 0 |
| `tt_probes` | TTProbe() calls | ~1.0× negamax_calls |
| `tt_hits` | TTProbe() calls that found an entry | 30-60% of probes |
| `tt_puts` | TTPut() calls | ~0.5× probes |
| `tt_score_cuts` | Cutoffs that returned ttScore directly | 5-15% of probes |
| `aspiration_retries` | Aspiration window re-searches | 0-100 per search |
| `null_move_cuts` | Null-move pruning cutoffs | 1-5% of negamax calls |
| `lmr_reductions` | Late-move reductions applied | 20-40% of negamax calls |
| `rfp_cuts` | Reverse futility pruning cutoffs | 5-15% of negamax calls |
| `futility_prunes` | (reserved) | 0 |
| `see_prunes` | (reserved) | 0 |
| `singular_ext` | Singular extensions | 0.1-1% of negamax calls |
| `check_ext` | (reserved) | 0 |
| `pv_changes` | Best-move changes across iterations | 0-20 |
| `qs_standpat_cuts` | Quiescence stand-pat cutoffs | 30-60% of qsearch calls |
| `qs_delta_prunes` | (reserved) | 0 |
| `root_move_count` | Legal root moves | 20-50 |
| `time_checks` | CheckLimits() calls | ~0.01× negamax calls |
| `time_panic` | Hard-time-limit triggers | 0 (healthy) |

Counters marked "reserved" are declared in the enum but not yet instrumented
in the search. They will be populated in future 6.x releases.

## Interpreting the numbers

### TT hit rate

A healthy TT hit rate is 30-60%. Below 20% suggests the TT is too small for
the search (increase `Hash`). Above 70% is suspicious — it may indicate the
search is repeating itself (e.g., aspiration retry loops).

### Qsearch ratio

`quiesce_calls / negamax_calls` tells you how tactical the position is. Quiet
positions get 30-40%; sharp tactical positions get 50-60%. If qsearch
dominates (>70%), the position is extremely tactical and the search is
spending most of its time resolving captures.

### Pruning balance

Compare `null_move_cuts`, `rfp_cuts`, `tt_score_cuts`. These are the main
cutoff sources. A healthy search has a mix; if any one dominates >80%, the
others may be mis-tuned.

### Aspiration retries

`aspiration_retries` should be small relative to the final depth. For a
depth-15 search, 0-30 retries is healthy. >100 suggests the aspiration window
is too narrow or the search is unstable.

### PV changes

`pv_changes` of 0-5 is healthy (stable search). >10 suggests the search is
oscillating between moves, which may indicate evaluation instability.

## Combining with external profilers

The internal counters tell you *what* the search is doing. For *where* time
is spent (CPU cycles), combine with an external profiler:

### perf (Linux)

```bash
$ perf record -F 99 -g -- ./nexus6.1 bench
$ perf report
```

Look for hotspots in `Negamax`, `Quiesce`, `TTProbe`, `Evaluate`,
`MakeMove`, `NextMove`. The internal counters help you interpret perf output:
e.g., if `TTProbe` is hot in perf and `tt_hit_rate_pct` is high, the TT is
doing useful work; if hit rate is low, the TT may be thrashing.

### Instruments (macOS)

```bash
$ instruments -t "Time Profiler" -- ./nexus6.1 bench
```

### VTune (Windows)

Use the "Hotspots" analysis. The internal counters help you correlate VTune's
cycle counts with search behavior.

## Profiling overhead

The `profile` build has ~1-3% overhead vs release (mostly from the counter
increments, which are single `inc` instructions but still cost cycles and
may prevent some micro-optimizations). The `debug` build has much higher
overhead (10-100×) due to ASan/UBSan and `-O1`.

For accurate strength testing, always use the `release` build. For profiling
and hotspot analysis, use the `profile` build. For memory bug hunting, use
the `debug` build.

## Adding new counters

To add a new profile counter:

1. Add an entry to the `NexusProfileCounter` enum in `src/instrument.h`
   (before `NPC_COUNT`).
2. Add a `printf` line for it in `NexusProfileReport()` in `src/instrument.c`.
3. Instrument the call site with `NEXUS_PROFILE_INC(NPC_YOUR_COUNTER)`
   wrapped in `#if defined(NEXUS_PROFILE)`.
4. Rebuild with `make profile` and verify the counter appears in output.

Keep counters cheap (single `inc`). If you need to measure something
expensive (e.g., timing a function), use an external profiler instead.
