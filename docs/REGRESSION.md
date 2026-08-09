# Nexus 6.1 Regression Testing Guide

This document explains the regression test suite and how to use it to catch
bugs before they become version-wide problems.

## The validation gate

The single command that runs before merging any patch:

```bash
$ bash scripts/validate.sh release
############################################################
#  Nexus 6.1 Validation Gate
#  Build mode: release  CC: gcc  ARCH: avx2
############################################################
==> [1/4] Building nexus (release)...
    Build OK (26100936 bytes)

==> [2/4] UCI handshake + build info...
    id name: OK
    id author: OK
    uciok: OK

==> [3/4] Bench regression (must be 2,811,728 nodes)...
    Bench: OK (2811728 nodes)

==> [4/4] Running full regression suite...
    ... (per-test output) ...

############################################################
#  VALIDATION PASSED
############################################################
```

This runs four steps:
1. Build the engine
2. Verify UCI handshake
3. Verify bench node count (the regression sentinel)
4. Run the full regression suite (7 test scripts)

Exit code 0 = pass, 1 = test failure, 2 = build failure.

For instrumented builds, use `bash scripts/validate.sh profile` or
`bash scripts/validate.sh debug`. The bench regression check is skipped for
non-release builds (instrumentation overhead changes timing).

## The regression suite

The suite lives in `tests/` and is orchestrated by `tests/run_all.sh`:

```
tests/
├── run_all.sh           orchestrator — runs everything
├── perft.sh             perft (move-gen correctness, ~150 positions)
├── mate-in-1.sh         mate-in-1 (tactical correctness, ~100 positions)
├── special_moves.sh     promotion / ep / castling / FRC perft (12 positions)
├── legality.sh          bestmove legality (12 positions)
├── uci_smoke.sh         UCI command battery (12 commands, 21 checks)
├── timed.sh             time-management battery (10 time-mgmt tests)
└── bench_regression.sh  bench node-count check (must be 2,811,728)
```

### perft.sh — move-generation correctness

Uses `expect` to drive the engine through ~150 positions from the Chess
Programming Wiki perft suite. Covers standard chess, Chess960, en passant,
castling (including FRC castling), promotions, and check evasions.

Reference values are the canonical published perft numbers. Any failure
indicates a move-generation bug.

Requires `expect` (`apt-get install expect` on Debian/Ubuntu,
`pacman -S expect` on MSYS2, `brew install expect` on macOS). If `expect` is
not installed, the test skips with a notice.

### mate-in-1.sh — tactical correctness

Uses `expect` to drive the engine through ~100 mate-in-1 positions. The
engine must find the mating move within 2 seconds per position.

Requires `expect`.

### special_moves.sh — special-moves perft regression

Runs perft on 12 positions specifically chosen to exercise promotion, en
passant, castling (both standard and FRC), and draw detection. Reference
values are the engine's own perft output (cross-checked against canonical
values where available).

This test does NOT require `expect` — it parses the engine's
`go perft N` output directly.

### legality.sh — bestmove legality

Runs a 300ms search on 12 positions and verifies that every returned
bestmove is legal (by applying it via the engine's `apply` command and
checking the engine doesn't reject it).

This catches bugs where the engine suggests an illegal move due to a
movegen or search bug.

### uci_smoke.sh — UCI command battery

Runs 12 tests covering every required UCI command:

1. `uci` + `isready` + `ucinewgame`
2. `position startpos moves ...`
3. `position fen ...`
4. `go movetime`
5. `go depth`
6. `go infinite` + `stop`
7. `go nodes`
8. `go wtime` / `btime`
9. `go ponder` + `stop`
10. `go searchmoves`
11. `ucinewgame` resets
12. `quit` responsiveness

Total: 21 individual checks. This is the most important test for catching
UCI protocol regressions.

### timed.sh — time-management battery

Runs 10 time-management tests measuring engine-reported time (not wall-clock,
to avoid sleep overhead):

1. `go movetime 200`
2. `go movetime 1000`
3. `go wtime 1000 btime 1000` (blitz)
4. `go wtime 60000 btime 60000` (rapid)
5. `go infinite` + `stop`
6. `go ponder` + `stop`
7. `go ponder` + `ponderhit`
8. `go nodes 10000`
9. `go movetime 50` (tiny budget — must not hang)
10. 5 sequential quick moves (no state leak)

This catches time-management regressions: hangs, over-budget burns, panic
mode, and state leaks between searches.

### bench_regression.sh — node-count sentinel

Runs `nexus6.1 bench` and verifies the total is exactly 2,811,728 nodes. This
is the single most important regression check — it catches ANY behavioral
change in search, evaluation, or movegen.

If this test fails, one of:
- Intentional search change (update the expected value + document)
- Unintentional regression (revert immediately)
- Compiler/flag change (use baseline flags)

## Running individual tests

Each test script can be run standalone:

```bash
$ bash tests/uci_smoke.sh
$ bash tests/timed.sh
$ bash tests/bench_regression.sh
$ bash tests/special_moves.sh
$ bash tests/legality.sh
# perft.sh and mate-in-1.sh require expect:
$ bash tests/perft.sh
$ bash tests/mate-in-1.sh
```

Each script exits 0 on pass, nonzero on fail.

## CI integration

The CI workflow (`.github/workflows/nexus.yml`) runs the validation gate on
every push and pull request, across multiple GCC versions and architectures.
A failure blocks the merge.

## When a test fails

1. **Read the failure message** — each test prints `[FAIL] <description>`.
2. **Reproduce locally** — run the failing script directly.
3. **Bisect** — if you have multiple changes, use `git bisect` with the
   failing test as the predicate.
4. **Fix or revert** — never merge with a failing test.

### Common false positives

- `perft.sh` / `mate-in-1.sh` skip if `expect` is not installed (exit 0).
  Install `expect` to actually run them.
- `timed.sh` measures engine-reported time, not wall-clock. If the engine
  reports `time 968` for `movetime 1000`, that's a pass even though
  wall-clock was ~2s (due to test harness sleep).
- `bench_regression.sh` will fail if you run `bench 12` instead of `bench 13`.
  Always use the default depth.

## Adding a new regression test

1. Create `tests/your_test.sh` following the same pattern:
   - `set -u`, cd to `src/`, build if missing
   - `pass`/`fail` counters
   - Print `[PASS]` / `[FAIL]` per check
   - Print summary, exit with fail count
2. Add it to `tests/run_all.sh`:
   ```bash
   run_test "your test name" tests/your_test.sh
   ```
3. Document it in this file.

Good candidates for new tests:
- Specific tactical positions that have regressed before
- Edge-case FENs (null moves, weird castling, etc.)
- Time-management scenarios not covered by `timed.sh`
- Tablebase probing correctness (if Syzygy tables are available)
