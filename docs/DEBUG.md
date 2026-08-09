# Nexus 6.1 Debug Guide

This document explains how to use the Nexus 6.1 debug layer to understand why
the engine chose a particular move.

## Enabling debug output

Debug instrumentation is compile-time gated. To use it, build with
`make debug` or `make profile`:

```bash
cd src
make debug CC=gcc ARCH=avx2    # full debug: ASan/UBSan + all instrumentation
# or
make profile CC=gcc ARCH=avx2  # lighter: O2 + profile counters + summary
```

Verify the build has debug enabled:

```bash
$ ./nexus6.1 buildinfo
Nexus 6.1 build capabilities:
  NEXUS_DEBUG    : enabled
  NEXUS_PROFILE  : enabled
  NEXUS_SUMMARY  : enabled
```

In a default release build (`make build` or `make release`), all three are
`disabled` — the engine is byte-identical to Nexus 6.0 and produces no debug
output.

## Three debug tools

Nexus 6.1 provides three orthogonal debug tools, each controlled by a UCI
option:

| UCI option | Type | Default | What it does |
|---|---|---|---|
| `DebugModules` | string | `<empty>` | Enables per-module `info string debug ...` logging |
| `ExplainMode` | check | false | After each `bestmove`, prints a human-readable reason summary |
| `ProfileReport` | check | false | After each `bestmove`, prints per-subsystem performance counters |

All three options are always declared (so UCI handshakes are identical across
builds), but they only produce output when the matching compile flag is active.
In a release build, setting them to a non-default value prints an
`info string` notice explaining that the build lacks instrumentation.

## ExplainMode — human-readable reason summary

The most useful debug tool. When enabled, the engine prints an
`info string explain ...` block after every `bestmove`:

```
$ printf 'uci\nsetoption name ExplainMode value true\nposition startpos\ngo depth 12\nquit\n' | ./nexus6.1
...
bestmove e2e4 ponder e7e6
info string explain begin
info string explain final_depth      12
info string explain final_seldepth   14
info string explain total_nodes      28895
info string explain qsearch_nodes    14023 (48.5% of total)
info string explain tt_probes        29877
info string explain tt_hits          11657 (39.0% hit rate)
info string explain aspiration_retries 16
info string explain pv_changes       5
info string explain root_move_count  20
info string explain cutoffs_tt       1304
info string explain cutoffs_null     148
info string explain cutoffs_rfp      7132
info string explain cutoffs_futility 0
info string explain cutoffs_see      0
info string explain cutoffs_other    0
info string explain time_alloc_ms    0
info string explain time_used_ms     27
info string explain time_panic       false
info string explain top_moves_by_nodes 2356(52.4%) 2942(23.2%) 2868(0.3%) 2291(0.0%)
info string explain summary Chose move with from-to 36-52 (sq 36->52) at depth 12 (seldepth 14), 28895 nodes, 39.0% TT hit rate, 16 aspiration retries, 5 PV changes, used 27ms of 0ms allocated.
info string explain end
```

### Interpreting the explain output

| Field | Meaning |
|---|---|
| `final_depth` | The last completed iteration depth |
| `final_seldepth` | The deepest selective depth reached (includes extensions) |
| `total_nodes` | Total nodes searched |
| `qsearch_nodes` | Nodes searched in quiescence (high % = tactical position) |
| `tt_probes` / `tt_hits` | TT usage; high hit rate = good TT reuse |
| `aspiration_retries` | How often the aspiration window was too tight (lower = better) |
| `pv_changes` | How many times the best move changed across iterations (lower = more stable) |
| `root_move_count` | Number of legal root moves |
| `cutoffs_tt` / `cutoffs_null` / `cutoffs_rfp` / `cutoffs_futility` / `cutoffs_see` | Cutoffs by source |
| `time_alloc_ms` / `time_used_ms` / `time_panic` | Time management behavior |
| `top_moves_by_nodes` | Top 5 root moves by node share (from-to + percentage) |
| `summary` | One-line human-readable reason |

The `top_moves_by_nodes` field is especially useful for understanding move
ordering quality: the best move should get the most nodes (typically 40-70%),
and a healthy search will have a clear gradient. If the best move gets <20%
of nodes, move ordering may be weak.

The `from-to` encoding in `top_moves_by_nodes` and `summary` uses the engine's
internal square numbering (a1=0, b1=1, ..., h8=63). The combined from-to value
is `from + 64*to`, so e2e4 = 12 + 64*28 = 1804. (For human-readable moves,
check the `pv` line in the regular `info depth ...` output instead.)

## DebugModules — per-module logging

For deeper inspection, `DebugModules` enables per-module
`info string debug ...` logging. This produces a lot of output (one or more
lines per node) and should only be used for very short searches.

```
$ printf 'uci\nsetoption name DebugModules value tt,aspiration\nposition startpos\ngo depth 3\nquit\n' | ./nexus6.1
...
info string debug tt_probe hash=... hit=true depth=...
info string debug tt_probe hash=... hit=false depth=...
info string debug aspiration_retry depth=3 alpha=... beta=...
...
```

### Available modules

| Module name | What it logs |
|---|---|
| `tt` | Transposition table probes and hits |
| `search` | Negamax entry/exit, cutoffs, bounds |
| `qsearch` | Quiescence entry/exit, stand-pat |
| `root` | Root move ordering, iteration results |
| `time` | Time allocation, soft/hard checks, panic mode |
| `movegen` | Move generation counts |
| `eval` | Evaluation calls |
| `movepick` | Move picker phase transitions |
| `aspiration` | Aspiration window re-searches |
| `pruning` | Null-move, LMR, RFP, razoring, futility decisions |
| `extensions` | Singular, check extensions |
| `pv` | PV changes between iterations |
| `all` | Enable everything (very noisy) |
| `none` | Disable all modules |

Multiple modules can be combined with commas:

```
setoption name DebugModules value tt,search,time
```

Note: the `DebugModules` logging is the most verbose instrumentation and can
slow the engine by 10-100× due to `printf` overhead. Use it only with very
short searches (`go depth 3` or `go nodes 1000`).

## ProfileReport — per-subsystem counters

When `ProfileReport` is enabled, the engine prints a counter summary after
each `bestmove`:

```
$ printf 'uci\nsetoption name ProfileReport value true\nposition startpos\ngo depth 12\nquit\n' | ./nexus6.1
...
bestmove e2e4 ponder e7e6
info string profile begin
info string profile negamax_calls      28895
info string profile quiesce_calls      14023
info string profile movegen_calls      0
info string profile eval_calls         0
info string profile tt_probes          29877
info string profile tt_hits            11657
info string profile tt_puts            12340
info string profile tt_score_cuts      1304
info string profile tt_hit_rate_pct    39.00
info string profile aspiration_retries 16
info string profile null_move_cuts     148
info string profile lmr_reductions     8421
info string profile rfp_cuts           7132
info string profile futility_prunes    0
info string profile see_prunes         0
info string profile singular_ext       23
info string profile check_ext         0
info string profile pv_changes        5
info string profile qs_standpat_cuts  8213
info string profile qs_delta_prunes   2104
info string profile root_move_count   20
info string profile time_checks       288
info string profile time_panic        0
info string profile end
```

These counters are the same data shown in `ExplainMode`, but in a flat
machine-readable format suitable for logging or diffing across runs.

## Combining the tools

For most debugging sessions, the recommended workflow is:

1. Build with `make profile` (good speed, all summary/profile/explain data).
2. Enable `ExplainMode` and `ProfileReport`.
3. Run `go depth N` on the position of interest.
4. Read the `explain` summary first — it answers "why this move?".
5. If you need more detail, enable specific `DebugModules` and re-run with a
   shorter search.

## Debug output is honest

All debug output is derived from actual search state. No field is faked. If
the engine reports `tt_hits 11657`, that is exactly the number of TT hits
that occurred during the search. If it reports `pv_changes 5`, that is
exactly how many times the best move changed across iterations.

The only exception is that `time_alloc_ms` may be 0 for `go depth` searches
(which don't set a time budget) — this is correct, not a bug.

## Debug build and ASan/UBSan

The `make debug` build also enables AddressSanitizer and UndefinedBehaviorSanitizer.
If the engine has a memory bug (buffer overflow, use-after-free, signed
integer overflow, etc.), ASan/UBSan will print a detailed report to stderr
and abort. This is extremely valuable for catching bugs early.

```
$ make debug CC=gcc ARCH=avx2
$ ./nexus6.1
uci
position startpos
go depth 8
# if there's a bug, you'll see something like:
# ==12345==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x...
```

Run all your test positions through the debug build regularly to catch
memory bugs that the release build would silently tolerate.
