# Nexus 6.1 Build Guide

This document is the complete, detailed build guide for Nexus 6.1. For a
quick-start, see the [README](../README.md). For MSYS2-UCRT64-specific
instructions, see the README.

## Prerequisites

### Build-time

- A C11 compiler: gcc ≥ 11 or clang ≥ 13
- GNU Make (or compatible)
- `wget` or `curl` (only needed if the NNUE network file is missing — it
  ships with the source tree)

### Run-time

- x86-64 CPU with SSE 4.1 + POPCNT (any CPU from ~2011)
- AVX2 + BMI1 + FMA for the recommended `avx2` build (Intel Haswell 2013+,
  AMD Ryzen all)
- AVX-512 + BMI2 for the `avx512` build (Intel Skylake-X+, AMD Zen 4+)
- ARM64 build for Apple Silicon

## Build modes

Nexus 6.1 introduces three explicit build modes via makefile targets:

| Target | Flags | Use case |
|---|---|---|
| `make build` | `-O3 -flto` (release) | Quick release build, no PGO |
| `make release` | alias for `make pgo` | Max-strength PGO build (recommended for tournaments) |
| `make pgo` | PGO two-pass | Profile-guided optimization; ~10-15% faster than `build` |
| `make profile` | `-O2 -DNEXUS_PROFILE -DNEXUS_SUMMARY` | Hotspot analysis + benchdetail |
| `make debug` | `-O1 -g -DNEXUS_* -fsanitize=address,undefined` | Development with ASan/UBSan |

The `build`, `release`, and `pgo` targets produce a binary byte-identical to
the Nexus 6.0 baseline (bench = 2,811,728 nodes). The `profile` and `debug`
targets enable instrumentation but still produce the same node count.

## ARCH selection

Pick the highest ISA your CPU supports:

| `ARCH=` | Required CPU features | Typical CPUs |
|---|---|---|
| `x86-64` | x86-64 baseline + POPCNT | Any 64-bit CPU (slowest) |
| `sse41` | SSE 4.1 + POPCNT | Intel Nehalem (2008+), AMD Bulldozer |
| `avx2` | AVX2 + BMI1 + FMA + POPCNT | Intel Haswell (2013+), AMD Ryzen all (**recommended**) |
| `avx2-pext` | AVX2 + BMI2 (PEXT) | Intel Haswell refresh, Skylake, AMD Zen 3+ |
| `avx512-pext` | AVX-512 + BMI2 | Intel Skylake-X / Ice Lake / AMD Zen 4+ |
| `arm64` | ARM64 | Apple Silicon |
| `native` | auto-detect | Use only if building and running on the same CPU |

To check your CPU on Linux:

```bash
$ grep -oE 'avx512[fvb]+|avx2|bmi2|bmi1|fma|sse4_1' /proc/cpuinfo | sort -u
```

## Build commands

### Quick release build (no PGO)

```bash
cd src
make build CC=gcc ARCH=avx2
./nexus6.1
```

### PGO release build (max strength, recommended)

```bash
cd src
make pgo CC=gcc ARCH=avx2
./nexus6.1
```

PGO runs two compile passes with a bench in between to collect a profile.
Takes ~2× as long but produces a ~10-15% faster binary.

### Profile build (for hotspot analysis)

```bash
cd src
make profile CC=gcc ARCH=avx2
./nexus6.1 buildinfo    # verify NEXUS_PROFILE is enabled
./nexus6.1 benchdetail  # extended per-position metrics
```

### Debug build (for development, with ASan/UBSan)

```bash
cd src
make debug CC=gcc ARCH=avx2
./nexus6.1 buildinfo    # verify NEXUS_DEBUG is enabled
```

The debug build is much slower (~10-100×) but catches memory bugs and
undefined behavior. Do NOT use it for strength testing.

### Using clang instead of gcc

```bash
make pgo CC=clang ARCH=avx2
```

clang sometimes produces slightly faster code; gcc is more widely tested.
Both are fully supported.

## Verifying the build

### 1. UCI handshake

```bash
$ printf 'uci\nquit\n' | ./nexus6.1 | head -3
id name Nexus 6.1 6.1.0
id author Nexus Team
option name Hash type spin default 16 min 2 max 33554432
```

### 2. Bench regression check

```bash
$ ./nexus6.1 bench | grep Results
Results:                                     2811728 nodes  1798930 nps
```

The node count MUST be 2,811,728. If it isn't, the build has a problem.

### 3. buildinfo

```bash
$ ./nexus6.1 buildinfo
Nexus 6.1 build capabilities:
  NEXUS_DEBUG    : disabled
  NEXUS_PROFILE  : disabled
  NEXUS_SUMMARY  : disabled
Recommended builds:
  make release   - max strength (PGO, no instrumentation)
  make profile   - O2 + profile counters + summary (for hotspot analysis)
  make debug     - O0 + all instrumentation + ASan/UBSan (for development)
```

### 4. Full validation gate

```bash
$ bash scripts/validate.sh release
# ... runs build + UCI + bench + full regression suite
############################################################
#  VALIDATION PASSED
############################################################
```

## Platform-specific notes

### Linux

```bash
sudo apt-get install -y build-essential git wget
cd nexus6.1/src
make pgo CC=gcc ARCH=avx2
```

### macOS (Intel)

```bash
brew install make
cd nexus6.1/src
gmake pgo CC=clang ARCH=avx2-pext
```

### macOS (Apple Silicon)

```bash
brew install make
cd nexus6.1/src
gmake pgo CC=clang ARCH=arm64
```

### Windows (MSYS2-UCRT64)

See the [README](../README.md) for the full MSYS2-UCRT64 guide. Quick version:

```bash
# In the MSYS2 UCRT64 shell:
pacman -Syu
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-make git wget
cd ~/nexus6.1/src
mingw32-make pgo CC=gcc ARCH=avx2
./nexus6.1.exe
```

### Cross-compile Windows from Linux

```bash
sudo apt-get install -y mingw-w64
cd nexus6.1
bash resources/build-windows.sh
# produces nexus-6-{x86-64,sse41,avx2,avx2-pext,avx512-pext}.exe
```

These are static, self-contained `.exe` files. Not PGO-optimized; for max
strength on Windows, use the native MSYS2-UCRT64 build.

## Build artifacts

| File | Description |
|---|---|
| `src/nexus6.1` (Linux/macOS) or `src/nexus6.1.exe` (Windows) | The engine binary (versioned name) |
| `src/nexus-9b84c340af7e.nn` | The embedded NNUE network (shipped with source) |
| `src/pgo/` (temporary) | PGO profile data (deleted after build) |
| `src/nexus6.1.profdata` (temporary) | clang PGO profile (deleted after build) |

The binary embeds the NNUE network at compile time via `INCBIN`, so the
resulting `nexus6.1` / `nexus6.1.exe` is fully self-contained (~26 MB).

### Versioned binary naming

The makefile derives the binary name from `VERSION`: `nexus` + major.minor.
For `VERSION = 6.1.0`, the binary is `nexus6.1`. This makes it easy to keep
multiple engine versions side-by-side without name collisions.

To override (e.g. for OpenBench compatibility, which expects a fixed name):

```bash
make build EXE=nexus CC=gcc ARCH=avx2
```

## Troubleshooting

### `clang: not found`

The makefile defaults to `CC = clang`. Pass `CC=gcc`:

```bash
make build CC=gcc ARCH=avx2
```

### `Illegal instruction` on startup

You built for an `ARCH=` your CPU doesn't support. Rebuild with a lower arch.

### Bench node count differs from 2,811,728

If `./nexus6.1 bench` reports anything other than 2,811,728 at default depth:

1. You modified search logic — update `tests/bench_regression.sh` and document
2. Compiler/flag drift — use the same flags as the baseline
3. Different depth — `bench 13` is canonical

### PGO build fails

PGO requires running the binary between compile passes. If the intermediate
binary crashes, PGO fails. Fall back to `make build` (non-PGO).

### ASan errors in debug build

ASan errors indicate real memory bugs (buffer overflow, use-after-free, etc.).
Do NOT ignore them — fix the underlying bug before merging.

### `undefined reference to 'TTIdx'` in debug build

This was a bug in early 6.1 builds when using `-O0`. The current `make debug`
target uses `-O1` to avoid it. If you see this, ensure you're using the
latest makefile.

## Reproducible builds

For release builds, the binary is deterministic given:
- Same source tree (same git commit)
- Same compiler (gcc-X.Y or clang-X.Y)
- Same `ARCH=`
- Same `CC=` and `EXFLAGS=`

PGO introduces slight non-determinism (the profile depends on bench timing),
but the resulting node count is always 2,811,728. For bit-exact reproducibility,
use `make build` (non-PGO).
