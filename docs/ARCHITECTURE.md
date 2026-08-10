# Nexus 6.1 Architecture Overview

This document describes the high-level architecture of Nexus 6.1 for developers
who need to understand the engine quickly and safely. It is intended to be
practical — every section names the files and functions you need to look at.

## Source tree layout

```
src/
├── nexus.c              main() entry — dispatches bench / benchdetail / buildinfo / UCI
├── uci.c                UCI protocol parser + option handlers
├── search.c             Negamax + PVS + iterative deepening + aspiration + TM
├── search.h             search API
├── transposition.c      TT probe / put / clear / age
├── transposition.h      TT data structures
├── thread.c             pthread-based thread pool
├── thread.h             thread pool API
├── types.h              core types: Board, ThreadData, RootMove, SearchParams, PV
├── board.c              board representation, make/undo, FEN parse
├── board.h
├── move.c               move encoding, parsing, conversion
├── move.h
├── movegen.c            staged move generation
├── movegen.h
├── movepick.c           move picker (phases: TT, noisy, killers, quiet, bad-noisy)
├── movepick.h
├── history.c            history / countermove / continuation / correction heuristics
├── history.h
├── see.c                static exchange evaluation
├── see.h
├── eval.c               evaluation wrapper (calls NNUE)
├── eval.h
├── bits.c               bitboard utilities
├── bits.h
├── attacks.c            magic bitboards for sliders, knight/king/pawn tables
├── attacks.h
├── zobrist.c            Zobrist hashing
├── zobrist.h
├── random.c             PRNG (splitmix64)
├── random.h
├── util.c               misc utilities (time, math, alloc)
├── util.h
├── perft.c              perft (move-gen verification)
├── perft.h
├── bench.c              bench + benchdetail (regression + extended metrics)
├── bench.h
├── tb.c                 Syzygy tablebase glue
├── tb.h
├── instrument.c         Nexus 6.1 instrumentation layer (debug/profile/summary)
├── instrument.h
├── incbin.h             third-party: Dale Weiler's INCBIN (embed NNUE network)
├── nn/
│   ├── accumulator.c    NNUE accumulator updates
│   ├── accumulator.h
│   ├── evaluate.c       NNUE network inference (AVX2/AVX512/ARM64 paths)
│   └── evaluate.h
├── pyrrhic/             third-party: Syzygy probing (Ronald de Man et al., MIT)
│   ├── tbprobe.c
│   ├── tbprobe.h
│   └── ...
└── files/
    └── bench.csv        the 50 bench positions (Ethereal-derived)
```

## Subsystem walkthrough

### 1. Entry point — `nexus.c`

`main()` initializes the engine (Zobrist keys, pruning tables, attacks, NNUE,
threads, TT), then dispatches based on `argv[1]`:

- `nexus6.1 bench [depth]` — runs the 50-position bench suite (OpenBench-compatible)
- `nexus6.1 benchdetail [depth]` — runs bench with extended per-position metrics
  (requires a `NEXUS_SUMMARY` build; falls back to plain bench otherwise)
- `nexus6.1 buildinfo` — prints compile-time instrumentation capabilities
- (no arg / other) — enters the UCI loop

### 2. UCI protocol — `uci.c`

`UCILoop()` reads commands from stdin in a loop and dispatches:

- `uci` → `PrintUCIOptions()` — prints `id name`, `id author`, all options, `uciok`
- `isready` → `readyok`
- `ucinewgame` → resets board, clears TT, clears search state
- `position` → `ParsePosition()` — handles `startpos`, `fen`, `moves`
- `go` → `ParseGo()` — parses time controls, depth, nodes, movetime, searchmoves, etc.
- `stop` / `quit` / `ponderhit` → thread control
- `setoption name X value Y` → per-option handlers
- `perft`, `bench`, `board`, `eval`, `see`, `apply` — dev commands

Nexus 6.1 adds three new options (always declared, active only in instrumented
builds): `DebugModules`, `ExplainMode`, `ProfileReport`.

### 3. Search — `search.c`

The search is negamax + PVS with iterative deepening. Key functions:

- `StartSearch()` — entry; resets instrumentation state; wakes main thread
- `MainSearch()` — main-thread loop; aggregates votes across threads; prints
  `bestmove`; emits explain/profile reports if enabled
- `Search()` — per-thread iterative-deepening loop; aspiration windows;
  time management; prints `info depth ...` lines
- `Negamax()` — recursive alpha-beta / PVS; TT probe; pruning (RFP, razoring,
  null-move, ProbCut, futility, LMR); extensions (singular, check)
- `Quiesce()` — quiescence search; stand-pat; delta pruning; SEE pruning
- `PrintUCI()` — formats `info depth ... pv ...` lines

### 4. Transposition table — `transposition.c`

- `TTInit(mb)` — allocates aligned TT memory (2 MB hugepages on Linux)
- `TTProbe()` — looks up a position; returns hit/miss + bound/score/move/eval
- `TTPut()` — stores a position; mate-score normalization; age-based replacement
- `TTClear()` / `TTClearPart()` — full / partitioned clear (used by `ucinewgame`)
- `TTUpdate()` — bumps generation (age) at the start of each search
- `TTFull()` — sample-based hashfull estimate

The TT uses buckets of 4 entries keyed by 16-bit hash, with age-based
replacement.

### 5. Move generation — `movegen.c` + `movepick.c`

Staged move generation. `movepick.c` drives the picker through phases:
`HASH_MOVE → GEN_NOISY_MOVES → PLAY_GOOD_NOISY → PLAY_KILLER_1 → PLAY_KILLER_2
→ GEN_QUIET_MOVES → PLAY_QUIET → PLAY_BAD_NOISY`.

Quiescence has its own picker (`InitQSMovePicker`, `InitQSEvasionsPicker`).

### 6. Evaluation — `eval.c` + `nn/`

`Evaluate()` calls the NNUE inference in `nn/evaluate.c`. The network is a
horizontally-mirrored 16-bucket design: `2×(12288 → 512) → 1`. The network is
embedded in the binary at compile time via `INCBIN` (see `nn/evaluate.c` line
36, `incbin.h`).

Accumulator updates are incremental (see `nn/accumulator.c`).

### 7. Time management — inside `search.c`

`Limits` (a `SearchParams` struct in `types.h`) holds the parsed time controls.
`Search()` computes `Limits.alloc` (the soft time budget per move) and checks
it after each iteration using stability / score-change / node-count factors.
Hard limit enforcement is in `CheckLimits()` (called from `Negamax` / `Quiesce`).

`MoveOverhead` (default 50 ms) is subtracted from the available time as a
safety margin.

### 8. Threading — `thread.c`

pthread-based thread pool. `Threads.count` threads search the same position
independently. `MainSearch()` aggregates the per-thread best moves by voting
(see `ThreadValue()`). Lazy SMP.

### 9. Tablebases — `tb.c` + `pyrrhic/`

`tb.c` is the glue to Pyrrhic (a Syzygy probing library). `TBProbe()` is called
from `Negamax()` for positions with ≤7 pieces (configurable). `SyzygyPath`
UCI option sets the directory.

### 10. Instrumentation (Nexus 6.1) — `instrument.c` + `instrument.h`

Three orthogonal subsystems, all compile-time gated:

- **NEXUS_DEBUG** — per-module `info string debug ...` logging
- **NEXUS_PROFILE** — per-subsystem performance counters (call counts, cutoffs)
- **NEXUS_SUMMARY** — search-summary tracking for `ExplainMode` and `benchdetail`

When a flag is off, every macro expands to nothing — the calling code is
byte-identical to the Nexus 6.0 baseline (verified by bench = 2,811,728 nodes).

See [DEBUG.md](./DEBUG.md), [PROFILING.md](./PROFILING.md), and
[BENCHMARK.md](./BENCHMARK.md) for details.

## Data flow: a single `go depth 10` command

1. `UCILoop()` reads `go depth 10` → `ParseGo()` sets `Limits.depth = 10`
2. `ParseGo()` calls `StartSearch(&board, 0)`
3. `StartSearch()` resets instrumentation, wakes main thread with `THREAD_SEARCH`
4. Main thread runs `MainSearch()` → calls `Search()` for each thread
5. Each thread enters the iterative-deepening loop in `Search()`:
   - For depth = 1..10:
     - For each MultiPV:
       - Open aspiration window (from depth 5)
       - `Negamax()` recursive search
       - `SortRootMoves()`
       - `PrintUCI()` if main thread and time elapsed
     - Time-management check (soft TM)
6. `MainSearch()` waits for all threads, aggregates votes, picks best thread
7. `MainSearch()` prints `bestmove X ponder Y`
8. If `ExplainMode` is on, prints `info string explain ...` summary
9. If `ProfileReport` is on, prints `info string profile ...` counters
10. `MainSearch()` returns; `UCILoop()` continues

## Key invariants

- **Bench node count = 2,811,728** at default depth 13. Any drift indicates
  a behavioral change in search, evaluation, or movegen. Enforced by
  `tests/bench_regression.sh`.
- **Perft startpos d5 = 4,865,609** and **Kiwipete d4 = 4,085,603**. Enforced
  by `tests/perft.sh`.
- **UCI handshake** must always print `id name Nexus 6.1` and
  `id author Nexus Team`. Enforced by `tests/uci_smoke.sh`.
- **Release build has no instrumentation overhead.** Verified by
  `scripts/bench_compare.sh` showing 0 drift between 6.0 and 6.1 release builds.
- **Debug/profile builds have identical node counts to release** (counters
  don't change control flow). Verified by `benchdetail` total = 2,811,728.

## Where to make changes

| You want to... | Look at... |
|---|---|
| Tune search pruning | `search.c` Negamax(); run `scripts/validate.sh release` after every change |
| Add a new UCI option | `uci.c` PrintUCIOptions() + the setoption handler chain |
| Change evaluation | `nn/evaluate.c` (inference) or train a new network (separate repo) |
| Improve time management | `search.c` Search() soft-TM block; `CheckLimits()` |
| Add instrumentation | `instrument.h` + `instrument.c`; wire via `#ifdef NEXUS_*` |
| Add a regression test | `tests/` directory; add to `tests/run_all.sh` |
| Add a bench position | `src/files/bench.csv` — but this changes the canonical node count! |

## What NOT to change in 6.1

Per the release mission, 6.1 is an infrastructure release. Do NOT:

- Rewrite alpha-beta / PVS structure
- Rewrite move generation
- Rewrite the transposition table core
- Rewrite evaluation architecture
- Add new pruning systems
- Add experimental heuristics without a bench-regression gate

These are deferred to later Nexus versions once the validation and profiling
infrastructure is proven.
