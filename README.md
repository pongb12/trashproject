# Nexus 6.1

A UCI-compliant chess engine written in C, derived from the **Berserk** engine
by Jay Honnold and rebranded / evolved by **Nexus Team**.

Nexus 6.1 is an **infrastructure release** on top of Nexus 6.0. It does NOT
change search, evaluation, or move generation — the baseline playing strength
is preserved exactly (bench = 2,811,728 nodes, 0 drift vs 6.0). Instead, 6.1
makes the engine dramatically easier to build, inspect, debug, benchmark,
regress-test, profile, and evolve safely.

- **Engine name (UCI):** `Nexus 6.1`
- **Author (UCI):** `Nexus Team`
- **Version:** `6.1.0`
- **Language:** C 11 (gcc or clang)
- **License:** GNU General Public License v3.0 (inherited from Berserk)
- **Provenance & attributions:** see [`ATTRIBUTION.md`](./ATTRIBUTION.md)

## What's new in 6.1

| Area | New |
|---|---|
| **Validation** | `scripts/validate.sh` — single-command pre-merge gate |
| **Benchmark** | `nexus6.1 benchdetail` — per-position TT hit rate, qsearch %, aspiration retries, PV changes |
| **Debug** | `ExplainMode` UCI option — human-readable reason summary after each bestmove |
| **Debug** | `DebugModules` UCI option — per-module `info string debug ...` logging |
| **Profiling** | `ProfileReport` UCI option — per-subsystem performance counters |
| **Regression** | 7 test scripts (perft, mate, special moves, legality, UCI, timing, bench) |
| **Build** | `make debug` / `make profile` / `make release` modes |
| **Docs** | 9 new documents in `docs/` |
| **CI** | GitHub Actions runs the full validation gate on every push/PR |
| **QoL** | `nexus6.1 buildinfo`, build capability line in UCI handshake, `bench_compare.sh` |

**Strength preserved:** bench = 2,811,728 nodes (bit-identical to 6.0 and Berserk baseline).

See [`docs/RELEASE_NOTES_6.1.md`](./docs/RELEASE_NOTES_6.1.md) for full details.

## Documentation index

| Doc | Content |
|---|---|
| [`docs/ARCHITECTURE.md`](./docs/ARCHITECTURE.md) | Engine architecture overview |
| [`docs/BENCHMARK.md`](./docs/BENCHMARK.md) | Bench / benchdetail / bench_compare guide |
| [`docs/DEBUG.md`](./docs/DEBUG.md) | ExplainMode / DebugModules guide |
| [`docs/PROFILING.md`](./docs/PROFILING.md) | ProfileReport and counter interpretation |
| [`docs/REGRESSION.md`](./docs/REGRESSION.md) | Regression test suite guide |
| [`docs/BUILD.md`](./docs/BUILD.md) | Detailed build guide (all platforms, all modes) |
| [`docs/UCI_OPTIONS.md`](./docs/UCI_OPTIONS.md) | Full UCI options reference |
| [`docs/ROADMAP.md`](./docs/ROADMAP.md) | Planned future work |
| [`docs/RELEASE_NOTES_6.1.md`](./docs/RELEASE_NOTES_6.1.md) | 6.1 release notes |
| [`ATTRIBUTION.md`](./ATTRIBUTION.md) | License and provenance |

---

## Table of contents

1. [What Nexus 6.0 is](#what-nexus-60-is)
2. [Feature summary](#feature-summary)
3. [Repository layout](#repository-layout)
4. [System requirements](#system-requirements)
5. [Building on Windows with MSYS2-UCRT64 (recommended)](#building-on-windows-with-msys2-ucrt64-recommended)
6. [Building on Linux / macOS / WSL](#building-on-linux--macos--wsl)
7. [Running the engine](#running-the-engine)
8. [Verifying correctness](#verifying-correctness)
9. [UCI options reference](#uci-options-reference)
10. [Troubleshooting](#troubleshooting)
11. [Roadmap](#roadmap)
12. [License & credits](#license--credits)

---

## What Nexus 6.0 is

Nexus 6.0 starts from the Berserk codebase (commit `d94a2696`, 2026-06-23)
and rebrands it into a new engine identity. The rebrand is purely cosmetic
at this stage: every behavioral subsystem (search, evaluation, move
generation, transposition table, time management) is bit-for-bit identical
to the upstream baseline. This is verified by the bench test producing the
same 2,811,728-node total on both builds.

The plan, documented in `ATTRIBUTION.md`, is to evolve Nexus 6.0 in small,
measured steps from this stable foundation — without ever weakening real
play. Strength, correctness, and stability come first.

---

## Feature summary

Inherited from Berserk (carried over unchanged):

- **Board representation:** bitboards with magic bitboards for sliding pieces
- **Move generation:** staged, legal-move-aware at quiescence
- **Search:** negamax + PVS, iterative deepening, aspiration windows,
  internal iterative reductions, reverse futility pruning, razoring,
  null-move pruning, ProbCut, futility pruning, late-move pruning (LMP),
  history pruning, late-move reductions (LMR), killer heuristic, countermove
  heuristic, singular extensions, SEE-based pruning
- **Evaluation:** horizontally-mirrored NNUE with 16 buckets,
  2×(12288 → 512) → 1 architecture. The network is embedded in the binary
  at compile time.
- **Transposition table:** generation-based replacement, mate-score
  normalization, PV reconstruction
- **Time management:** handles `wtime`/`btime`/`winc`/`binc`/`movestogo`/
  `movetime`/`depth`/`nodes`/`infinite`/`ponder`; safe fallback bestmove
- **Threading:** pthread-based thread pool, `Threads` option 1–2048
- **Tablebases:** Syzygy probing via the bundled Pyrrhic library
- **UCI:** full UCI protocol including `MultiPV`, `Ponder`, `UCI_Chess960`,
  `UCI_ShowWDL`, `MoveOverhead`, `Contempt`, `EvalFile`, `searchmoves`
- **Tooling:** built-in `perft`, `bench`, `go perft N`

---

## Repository layout

```
nexus6.1/
├── README.md                  ← this file
├── ATTRIBUTION.md             ← consolidated license / author / provenance
├── LICENSE                    ← GPL v3 full text (inherited from Berserk)
├── .clang-format              ← formatting config (inherited)
├── .gitignore
├── .github/workflows/nexus.yml ← CI: build + bench + perft
├── docs/                      ← design and release notes
├── resources/
│   ├── build-windows.sh       ← Linux → Windows cross-compile helper
│   └── update.sh              ← in-place build helper
├── tests/
│   ├── perft.sh               ← perft test battery (expect-based)
│   └── mate-in-1.sh           ← mate-in-1 tactical test battery
└── src/
    ├── makefile               ← main build entry point
    ├── nexus.c                ← main() entry point
    ├── nexus-9b84c340af7e.nn  ← embedded NNUE network (binary)
    ├── incbin.h               ← third-party: INCBIN (Dale Weiler)
    ├── *.c, *.h               ← engine source (search, eval, uci, ...)
    ├── nn/                    ← NNUE accumulator + evaluation code
    └── pyrrhic/               ← third-party: Syzygy probing (MIT)
        ├── LICENSE
        └── *.c, *.h
```

---

## System requirements

### Build-time

- A C compiler supporting C11 and the target CPU's SIMD ISA:
  - **Linux / macOS:** gcc ≥ 11 or clang ≥ 13
  - **Windows:** MSYS2-UCRT64 toolchain (gcc 14+, make, coreutils) — see below
- GNU Make (or any compatible `make`)
- `wget` **or** `curl` (only needed if the NNUE network file is missing —
  it ships with the source tree, so this is rarely required in practice)

### Run-time

- A CPU with at least SSE 4.1 and POPCNT (any x86-64 CPU from ~2011 onward)
- AVX2 + BMI1 + FMA for the recommended `avx2` build (Intel Haswell 2013+,
  AMD Excavator 2015+, or any Ryzen)
- AVX-512 + BMI2 for the `avx512` build (Intel Skylake-X / Ice Lake / Rocket
  Lake / Tiger Lake and newer; AMD Zen 4+)
- ARM64 builds for Apple Silicon (`-arch arm64`) work on macOS

---

## Building on Windows with MSYS2-UCRT64 (recommended)

MSYS2 is the recommended way to build Nexus 6.0 natively on Windows. The
**UCRT64** environment uses Microsoft's modern Universal C Runtime (UCRT)
and the MinGW-w64 gcc toolchain — it produces fast native Windows binaries
without any Visual Studio dependency.

These instructions were written for a fresh MSYS2 install. If you already
have MSYS2, skip to step 2.

### Step 1 — Install MSYS2

1. Download the MSYS2 installer from <https://www.msys2.org/>.
2. Run the installer. Accept the default install path
   `C:\msys64` (the rest of this guide assumes this path).
3. When the installer finishes, the MSYS2 shell will open. If it does not,
   launch **MSYS2 UCRT64** from the Start menu (the icon labelled
   "MSYS2 UCRT64"). **You must use the UCRT64 shell**, not the default
   MSYS2 shell — the UCRT64 environment is what gives you the UCRT runtime
   and the right toolchain.

### Step 2 — Update the package database and core system

In the **UCRT64** shell:

```bash
pacman -Syu
```

If the update asks you to close the terminal, close it, reopen **MSYS2
UCRT64**, and run the command again until it reports nothing to do:

```bash
pacman -Su
```

### Step 3 — Install the UCRT64 toolchain

In the **UCRT64** shell, install gcc, make, and the binutils:

```bash
pacman -S --needed \
    mingw-w64-ucrt-x86_64-gcc \
    mingw-w64-ucrt-x86_64-make \
    mingw-w64-ucrt-x86_64-binutils \
    git \
    wget
```

When prompted, press `Enter` to accept the default (install all listed
packages).

Verify the install:

```bash
gcc --version     # should report gcc 14.x or newer
mingw32-make --version
```

> **Note on `make` vs `mingw32-make`:** In the UCRT64 environment, GNU Make
> is installed as `mingw32-make` (the binary name is historical — it works
> fine for 64-bit builds). You can either invoke it as `mingw32-make` or
> create an alias:
> ```bash
> alias make='mingw32-make'
> ```
> Put this alias in `~/.bashrc` to make it permanent.

### Step 4 — Get the Nexus 6.0 source

In the **UCRT64** shell, clone the repository (or extract your source
archive) into a working directory under your MSYS2 home
(`C:\msys64\home\<your-user>\` by default):

```bash
cd ~
git clone <your-nexus6.1-repo-url> nexus6
cd nexus6.1/src
```

If you received Nexus 6.0 as a `.zip` archive instead, extract it into your
home directory and `cd` into the `src` subdirectory.

### Step 5 — Pick the right `ARCH` for your CPU

The makefile accepts an `ARCH=` flag that selects the SIMD instruction set.
Pick the highest level your CPU supports:

| `ARCH=` value | Required CPU features                    | Typical CPUs                                  |
|---------------|------------------------------------------|-----------------------------------------------|
| `x86-64`      | x86-64 baseline + POPCNT                 | Any 64-bit CPU (slowest, baseline)            |
| `sse41`       | SSE 4.1 + POPCNT                         | Intel Core 2 Nehalem (2008+), AMD Bulldozer   |
| `avx2`        | AVX2 + BMI1 + FMA + POPCNT (recommended) | Intel Haswell (2013+), AMD Ryzen all          |
| `avx2-pext`   | AVX2 + BMI2 (PEXT)                       | Intel Haswell refresh, Skylake, AMD Zen 3+    |
| `avx512-pext` | AVX-512 + BMI2 (PEXT)                    | Intel Skylake-X / Ice Lake / Zen 4+           |

If you are unsure, **use `avx2`** — it runs on essentially every CPU made
in the last decade and is what the CI pipeline uses.

To check what your CPU supports on Windows, open Task Manager → Performance
→ CPU, or run this in the UCRT64 shell:

```bash
grep -oE 'avx512[fvb]+|avx2|bmi2|bmi1|fma|sse4_1' /proc/cpuinfo | sort -u
```

### Step 6 — Build

#### 6a. Standard build (no PGO)

In the **UCRT64** shell, from the `src/` directory:

```bash
mingw32-make build CC=gcc ARCH=avx2
```

This will produce `nexus6.1.exe` in the `src/` directory.

#### 6b. PGO build (recommended for best strength, ~10–15% faster)

Profile-Guided Optimization (PGO) runs a bench first to collect a profile,
then recompiles using that profile. It takes about 2× as long to build but
produces a meaningfully faster binary. Strongly recommended for tournament
play.

```bash
mingw32-make pgo CC=gcc ARCH=avx2
```

You will see two compile passes plus a `bench` run in between. When it
finishes, `nexus6.1.exe` is ready.

#### 6c. Clean rebuild

If you change `ARCH=` or `CC=`, do a clean rebuild to avoid stale objects:

```bash
mingw32-make clean
mingw32-make pgo CC=gcc ARCH=avx2
```

### Step 7 — Verify the build

```bash
./nexus6.1.exe
```

You should see nothing on stdout (the engine is waiting for UCI commands).
Type `uci` and press Enter; you should see:

```
id name Nexus 6.0 6.0.0
id author Nexus Team
option name Hash type spin default 16 min 2 max 33554432
option name Threads type spin default 1 min 1 max 2048
...
uciok
```

Type `position startpos`, then `go depth 10`. The engine should print a
series of `info depth N ... pv ...` lines ending with
`bestmove g1f3 ponder g8f6` (or another strong opening move).

To exit, type `quit` and press Enter.

### Step 8 — Use Nexus 6.0 from a chess GUI

Copy `nexus6.1.exe` somewhere convenient (e.g. `C:\Engines\Nexus\nexus6.1.exe`).
In your GUI (Cute Chess, Arena, BanksiaGUI, Nibbler, etc.) add a new UCI
engine pointing at that path. The GUI will send the standard UCI handshake
and you can start games.

### MSYS2-UCRT64 build — quick reference

```bash
# One-time setup
pacman -Syu
pacman -Su
pacman -S --needed mingw-w64-ucrt-x86_64-gcc \
                   mingw-w64-ucrt-x86_64-make \
                   mingw-w64-ucrt-x86_64-binutils \
                   git wget

# Build (release, max strength)
cd ~/nexus6.1/src
mingw32-make pgo CC=gcc ARCH=avx2

# Verify
./nexus6.1.exe
# (then type: uci → position startpos → go depth 10 → quit)

# Other build modes (Nexus 6.1):
# mingw32-make profile CC=gcc ARCH=avx2   # with instrumentation for benchdetail/explain
# mingw32-make debug CC=gcc ARCH=avx2     # ASan/UBSan for development
```

---

## Building on Linux / macOS / WSL

### Linux (apt-based)

```bash
sudo apt-get install -y build-essential git wget
git clone <your-nexus6.1-repo-url> nexus6
cd nexus6.1/src
make pgo CC=gcc ARCH=avx2
./nexus6.1
```

If you have clang installed and prefer it (it produces slightly faster code
on some workloads):

```bash
make pgo CC=clang ARCH=avx2
```

### macOS (Intel)

```bash
brew install make
cd nexus6.1/src
gmake pgo CC=clang ARCH=avx2-pext
./nexus6.1
```

### macOS (Apple Silicon)

```bash
brew install make
cd nexus6.1/src
gmake pgo CC=clang ARCH=arm64
./nexus6.1
```

### Cross-compile Windows binaries from Linux

Use the helper script in `resources/build-windows.sh`:

```bash
sudo apt-get install -y mingw-w64
cd nexus6.1
bash resources/build-windows.sh
# produces nexus-6-{x86-64,sse41,avx2,avx2-pext,avx512-pext}.exe
```

These are static, self-contained `.exe` files suitable for distribution
to Windows users. They are **not** PGO-optimized; Windows users who want
maximum speed should use the native MSYS2-UCRT64 build above.

---

## Running the engine

Nexus 6.0 is a standard UCI engine. It reads commands from stdin and writes
results to stdout. There is no interactive UI — you typically run it inside
a chess GUI (Cute Chess, Arena, BanksiaGUI, Nibbler) or pipe commands
directly:

```bash
cd nexus6.1/src
echo -e "uci\nposition startpos\ngo depth 12\nquit" | ./nexus6.1
```

### Special command-line mode: bench

For OpenBench compatibility and quick sanity checks, Nexus 6.0 accepts a
`bench` command-line argument that runs a fixed 50-position test suite and
prints node counts:

```bash
./nexus6.1 bench        # default depth (13)
./nexus6.1 bench 15     # custom depth
```

The expected total node count at the default depth is **2,811,728**. If you
see a different number, your build has a regression.

### Special command-line mode: perft

Nexus 6.0 also supports a `perft` command (in UCI mode, not as a CLI arg)
that runs a divide-style perft to a given depth and prints the per-move
node counts:

```
position startpos
go perft 5
```

The expected node count for `position startpos go perft 5` is **4,865,609**.

---

## Verifying correctness

### Perft (move-generation correctness)

The `tests/perft.sh` script runs ~150 positions covering standard chess,
Chess960, en passant, castling (including FRC castling), promotions,
repetition, and check evasions, comparing node counts against published
reference values from the Chess Programming Wiki.

Requires the `expect` utility:

```bash
# Linux
sudo apt-get install -y expect

# MSYS2-UCRT64
pacman -S --needed expect

# macOS
brew install expect
```

Then from the project root:

```bash
./tests/perft.sh
```

Expected output ends with `perft testing OK`. Any failure indicates a
move-generation bug.

### Mate-in-1 (tactical correctness)

```bash
./tests/mate-in-1.sh
```

Runs ~100 mate-in-1 positions and verifies the engine finds the mating
move. Expected output ends with `mate testing OK`.

### Bench (regression detection)

```bash
./src/nexus6.1 bench
```

Compare the `Results:` line against the expected value (2,811,728 nodes at
default depth). Any drift indicates a behavioral change in the search.

### UCI smoke test

The script at `/home/z/my-project/scripts/uci_smoke_test.sh` (in the dev
tree) runs 21 checks covering every required UCI command:
`uci`, `isready`, `ucinewgame`, `position startpos`, `position fen`,
`position startpos moves ...`, `go depth`, `go movetime`, `go infinite` +
`stop`, `go nodes`, `go wtime/btime`, `go ponder` + `stop`,
`go searchmoves`, and `quit` responsiveness.

---

## UCI options reference

| Option           | Type    | Default  | Range              | Notes |
|------------------|---------|----------|--------------------|-------|
| `Hash`           | spin    | 16       | 2 – 33554432 MB    | Transposition table size in MB |
| `Threads`        | spin    | 1        | 1 – 2048           | Search threads (use 1 for analysis) |
| `SyzygyPath`     | string  | `<empty>`| any path           | Directory containing Syzygy `.rtbw` / `.rtbz` files |
| `MultiPV`        | spin    | 1        | 1 – 256            | Number of PVs to report |
| `Ponder`         | check   | false    | true / false       | Enable pondering mode |
| `UCI_ShowWDL`    | check   | true     | true / false       | Show Win/Draw/Loss model in `info` |
| `UCI_Chess960`   | check   | false    | true / false       | Enable Chess960 / FRC rules |
| `MoveOverhead`   | spin    | 50       | 0 – 10000 ms       | Time safety margin |
| `Contempt`       | spin    | 0        | -100 – 100 cp      | Contempt factor (avoids draws) |
| `EvalFile`       | string  | `<empty>`| path               | Override the embedded NNUE network |

`EvalFile` is normally left empty — the embedded network is used
automatically. Setting it to a file path loads that file instead, which is
useful for testing new networks.

---

## Troubleshooting

### `clang: not found` or `/bin/sh: clang: not found`

The makefile defaults to `CC = clang`. Pass `CC=gcc` explicitly:

```bash
mingw32-make build CC=gcc ARCH=avx2
```

### Illegal instruction on startup

You built for an `ARCH=` your CPU doesn't support. Rebuild with a lower
arch (e.g. `x86-64` or `sse41`) or check your CPU features.

### Engine hangs / does not respond

Make sure you are sending UCI commands through stdin. The engine prints
nothing on startup by design — it waits for `uci`. If the GUI sends
`uci` and the engine doesn't respond with `uciok`, check that you are
running the right binary (`nexus6.1.exe` / `nexus6.1`, not `berserk`).

### `wget: command not found` during build

Only the `download-network` makefile target needs `wget` or `curl`, and
only when the NNUE file is missing. The file ships with the source tree
as `src/nexus-9b84c340af7e.nn`, so this is rarely triggered. If it is,
install `wget` (MSYS2: `pacman -S wget`) or `curl`.

### Bench node count differs from expected

If `./nexus6.1 bench` reports anything other than **2,811,728 nodes** at the
default depth, you have either modified search logic or built with a
different compiler/flags that changed floating-point behavior. Re-clone
clean and rebuild.

### PGO build fails

PGO requires running the binary between the two compile passes. If the
intermediate binary crashes, the PGO build will fail. Fall back to a
non-PGO build:

```bash
mingw32-make clean
mingw32-make build CC=gcc ARCH=avx2
```

### MSYS2 path issues

Always run the build inside the **UCRT64** shell, not the default MSYS2
shell or the MinGW32 shell. The UCRT64 shell sets up the correct
`PATH` and library search paths automatically.

---

## Roadmap

Nexus 6.0 is the first milestone. Planned future work, in priority order:

1. **Build reliability** — done (this release)
2. **UCI correctness** — done (21/21 smoke tests pass)
3. **Time-management safety** — done (inherited from Berserk, verified)
4. **Search stability** — done (inherited from Berserk, verified)
5. **Opening handling** — TBD (Berserk has no opening book; an optional
   move-ordering-hint book may be added without weakening play)
6. **TT reliability** — done (inherited from Berserk, verified)
7. **Move-ordering quality** — TBD (small LMR / history tuning tests)
8. **Practical tactical strength** — TBD (SPRT-verified patches only)
9. **Bench / debug tooling** — TBD (enhanced analysis output)
10. **NNUE upgrades** — future (new trainer / new network)
11. **Tuning infrastructure** — future (OpenBench or in-house)
12. **Self-play automation** — future

No patch will be merged that weakens real play. Every change is validated
by SPRT testing before landing.

---

## License & credits

Nexus 6.0 is licensed under the **GNU General Public License v3.0**. The
full license text is in [`LICENSE`](./LICENSE).

Nexus 6.0 is a rebrand and continuation line of the **Berserk** chess engine
by **Jay Honnold** (https://github.com/jhonnold/berserk), which is also
GPL v3 licensed. The full provenance, copyright, and license information
for Berserk and for the bundled third-party components (Pyrrhic tablebase
probing, INCBIN utility) is consolidated in [`ATTRIBUTION.md`](./ATTRIBUTION.md).

**Author:** Nexus Team  
**Version:** 6.0.0  
**Year:** 2026
