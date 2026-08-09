// Nexus 6.0 is a UCI compliant chess engine written in C
// Copyright (C) 2026 Nexus Team

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <string.h>

#include "attacks.h"
#include "bench.h"
#include "bits.h"
#include "eval.h"
#include "instrument.h"
#include "nn/evaluate.h"
#include "random.h"
#include "search.h"
#include "thread.h"
#include "transposition.h"
#include "types.h"
#include "uci.h"
#include "util.h"
#include "zobrist.h"

// Welcome to Nexus 6.1
int main(int argc, char** argv) {
  SeedRandom(0);

  InitZobristKeys();
  InitPruningAndReductionTables();
  InitAttacks();
  InitCuckoo();

  LoadDefaultNN();
  ThreadsInit();
  TTInit(16);

  // Bench mode (OpenBench-compatible): nexus bench [depth]
  if (argc > 1 && !strcmp(argv[1], "bench")) {
    int depth = DEFAULT_BENCH_DEPTH;
    if (argc > 2)
      depth = Max(1, atoi(argv[2]));

    Bench(depth);
  }
  // Detailed bench mode (Nexus 6.1): nexus benchdetail [depth]
  // Prints extended per-position metrics: TT hit rate, qsearch %, seldepth,
  // aspiration retries, PV changes. Requires NEXUS_SUMMARY build.
  else if (argc > 1 && !strcmp(argv[1], "benchdetail")) {
    int depth = DEFAULT_BENCH_DEPTH;
    if (argc > 2)
      depth = Max(1, atoi(argv[2]));

    if (!NexusBuildHasSummary()) {
      printf("info string benchdetail requires a NEXUS_SUMMARY build (use 'make profile' or 'make debug').\n");
      printf("info string falling back to standard bench output.\n\n");
      Bench(depth);
    } else {
      BenchDetail(depth);
    }
  }
  // Build info mode: nexus buildinfo — prints compile-time capabilities.
  else if (argc > 1 && !strcmp(argv[1], "buildinfo")) {
    printf("Nexus 6.1 build capabilities:\n");
    printf("  NEXUS_DEBUG    : %s\n", NexusBuildHasDebug()    ? "enabled" : "disabled");
    printf("  NEXUS_PROFILE  : %s\n", NexusBuildHasProfile()  ? "enabled" : "disabled");
    printf("  NEXUS_SUMMARY  : %s\n", NexusBuildHasSummary()  ? "enabled" : "disabled");
    printf("Recommended builds:\n");
    printf("  make release   - max strength (PGO, no instrumentation)\n");
    printf("  make profile   - O2 + profile counters + summary (for hotspot analysis)\n");
    printf("  make debug     - O0 + all instrumentation + ASan/UBSan (for development)\n");
  } else {
    UCILoop();
  }

  return 0;
}
