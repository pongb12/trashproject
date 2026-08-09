# Nexus 6.1 UCI Options Reference

This document lists every UCI option supported by Nexus 6.1, with semantics,
default values, and notes.

## Standard UCI options

These options follow the UCI protocol specification and are supported by all
UCI-compatible GUIs.

### `Hash`

| | |
|---|---|
| Type | spin |
| Default | 16 |
| Min | 2 |
| Max | 33554432 (32 GB) |
| Unit | MB |

Transposition table size in megabytes. Larger is generally better, with
diminishing returns past ~256 MB for blitz and ~1-2 GB for long time
controls. The TT is allocated on `setoption` and freed on engine exit.

Setting `Hash` to a new value reallocates the TT and reports the byte size
and entry count via `info string`.

### `Threads`

| | |
|---|---|
| Type | spin |
| Default | 1 |
| Min | 1 |
| Max | 2048 |

Number of search threads (Lazy SMP). For analysis, use 1. For tournament
play, use the number of physical cores (not hyperthreads). Beyond ~16
threads, strength gains diminish rapidly.

### `SyzygyPath`

| | |
|---|---|
| Type | string |
| Default | `<empty>` |

Directory containing Syzygy tablebase files (`.rtbw` and `.rtbz`). When set,
the engine probes tablebases for positions with ≤7 pieces (configurable in
source). Use `<empty>` to disable.

Example: `setoption name SyzygyPath value /srv/syzygy`

### `MultiPV`

| | |
|---|---|
| Type | spin |
| Default | 1 |
| Min | 1 |
| Max | 256 |

Number of principal variations to report. With `MultiPV > 1`, the engine
prints one `info ... multipv N ...` line per PV. Useful for analysis. Note:
MultiPV > 1 weakens play (the engine searches all candidate moves at full
depth rather than pruning weak ones).

### `Ponder`

| | |
|---|---|
| Type | check |
| Default | false |

Enables pondering. When true, the engine expects the GUI to send `ponderhit`
if the predicted opponent move matches. This is a hint to the engine; it does
not change search behavior on its own.

### `UCI_ShowWDL`

| | |
|---|---|
| Type | check |
| Default | true |

When true, the engine includes `wdl W D L` (Win/Draw/Loss permille) in
`info` lines, based on a model fit to the engine's eval-to-result curve.

### `UCI_Chess960`

| | |
|---|---|
| Type | check |
| Default | false |

Enables Chess960 / Fischer Random Chess rules. When toggled, the board is
reset to the standard startpos (Chess960 startpos must be set via
`position fen ...`).

### `MoveOverhead`

| | |
|---|---|
| Type | spin |
| Default | 50 |
| Min | 0 |
| Max | 10000 |
| Unit | ms |

Time safety margin subtracted from the available time. The engine will stop
searching at least `MoveOverhead` ms before the hard time limit. Increase
this if the engine loses on time in your GUI (50 ms is safe for most GUIs;
cutechess may need 100 ms).

### `Contempt`

| | |
|---|---|
| Type | spin |
| Default | 0 |
| Min | -100 |
| Max | 100 |
| Unit | centipawns |

Contempt factor. Positive values make the engine avoid draws (play riskier);
negative values make it seek draws. Useful for handicap play or when playing
a weaker opponent.

### `EvalFile`

| | |
|---|---|
| Type | string |
| Default | `<empty>` |

Path to an NNUE network file. When `<empty>`, the engine uses the embedded
network (compiled in at build time). Setting this to a file path loads that
file instead — useful for testing new networks.

The network file format is proprietary to the engine lineage (Berserk NNUE
format). Loading an incompatible file will produce incorrect evaluation.

## Nexus 6.1 instrumentation options

These options are new in Nexus 6.1. They are always declared (so UCI
handshakes are consistent across builds) but only produce output when the
matching compile flag is enabled. In a release build, setting them prints an
`info string` notice.

### `DebugModules`

| | |
|---|---|
| Type | string |
| Default | `<empty>` |

Comma-separated list of debug modules to enable. When non-empty, the engine
emits `info string debug ...` lines for the enabled modules during search.

Available modules: `tt`, `search`, `qsearch`, `root`, `time`, `movegen`,
`eval`, `movepick`, `aspiration`, `pruning`, `extensions`, `pv`, `all`,
`none`.

Example: `setoption name DebugModules value tt,aspiration,time`

**Requires:** `NEXUS_DEBUG` build (`make debug`). In other builds, the option
is accepted but produces no output (with a warning).

**Warning:** Debug logging is very verbose (one or more lines per node) and
can slow the engine by 10-100×. Use only with very short searches.

### `ExplainMode`

| | |
|---|---|
| Type | check |
| Default | false |

When true, after each `bestmove`, the engine prints an
`info string explain ...` block summarizing why it chose the move: final
depth, node count, TT hit rate, aspiration retries, PV changes, cutoff
sources, top moves by node share, and a one-line human-readable summary.

**Requires:** `NEXUS_SUMMARY` build (`make profile` or `make debug`). In a
release build, the option is accepted but produces no explain output (with
a warning).

See [DEBUG.md](./DEBUG.md) for interpreting the explain output.

### `ProfileReport`

| | |
|---|---|
| Type | check |
| Default | false |

When true, after each `bestmove`, the engine prints an
`info string profile ...` block with per-subsystem performance counters:
negamax calls, qsearch calls, TT probes/hits/puts, cutoff sources, LMR
reductions, singular extensions, time checks, etc.

**Requires:** `NEXUS_PROFILE` build (`make profile` or `make debug`). In a
release build, the option is accepted but produces no profile output.

See [PROFILING.md](./PROFILING.md) for interpreting the profile output.

## Build capability line

When the engine responds to `uci`, it includes an `info string build ...`
line reporting which instrumentation flags were active at compile time:

```
info string build debug=0 profile=0 summary=0
```

- `debug=1` — `NEXUS_DEBUG` was defined (debug build)
- `profile=1` — `NEXUS_PROFILE` was defined (profile or debug build)
- `summary=1` — `NEXUS_SUMMARY` was defined (profile or debug build)

This lets you programmatically detect whether `DebugModules`, `ExplainMode`,
and `ProfileReport` will actually produce output.

## Non-option UCI commands

In addition to the standard UCI commands (`uci`, `isready`, `ucinewgame`,
`position`, `go`, `stop`, `ponderhit`, `quit`, `setoption`), Nexus 6.1
accepts these dev commands in the UCI loop:

| Command | Description |
|---|---|
| `board` | Print the current board as ASCII |
| `cycle` | Print `yes`/`no` whether the current position is a cycle |
| `perft <depth> [fen]` | Run perft to depth on the current or given position |
| `bench <depth>` | Run the bench suite (same as `nexus6.1 bench` CLI) |
| `threats` | Print the threatened-pieces bitboard |
| `eval` | Print a trace of the NNUE evaluation |
| `see <move>` | Print the SEE score for a move |
| `apply <move>` | Apply a move to the current board (for legality checking) |

## CLI-only commands

These are only available when invoking the engine with a command-line
argument (not in UCI mode):

| Command | Description |
|---|---|
| `nexus6.1 bench [depth]` | Run bench suite (OpenBench-compatible) |
| `nexus6.1 benchdetail [depth]` | Run bench with extended per-position metrics (requires NEXUS_SUMMARY build) |
| `nexus6.1 buildinfo` | Print compile-time instrumentation capabilities |

## Time control formats (`go` command)

The `go` command accepts these time-control tokens:

| Token | Meaning |
|---|---|
| `wtime <ms>` | White's remaining time |
| `btime <ms>` | Black's remaining time |
| `winc <ms>` | White's increment per move |
| `binc <ms>` | Black's increment per move |
| `movestogo <n>` | Moves remaining until next time control |
| `movetime <ms>` | Search exactly this many ms (overrides wtime/btime) |
| `depth <d>` | Search to exactly this depth |
| `nodes <n>` | Search exactly this many nodes |
| `infinite` | Search until `stop` is received |
| `ponder` | Search in pondering mode (wait for `ponderhit` or `stop`) |
| `mate <n>` | Search for mate in N moves |
| `searchmoves <m1> <m2> ...` | Restrict search to these moves only |

All of these are exercised by `tests/timed.sh` and `tests/uci_smoke.sh`.
