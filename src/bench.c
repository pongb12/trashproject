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

#include "bench.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "board.h"
#include "instrument.h"
#include "move.h"
#include "search.h"
#include "thread.h"
#include "transposition.h"
#include "types.h"
#include "uci.h"
#include "util.h"

// Ethereal's bench set
const int NUM_BENCH_POSITIONS = 50;
char* benchmarks[]            = {
#include "files/bench.csv"
};

void Bench(int depth) {
  Board board;

  Limits.depth   = depth;
  Limits.multiPV = 1;
  Limits.hitrate = INT_MAX;
  Limits.max     = INT_MAX;
  Limits.timeset = 0;

  Move bestMoves[NUM_BENCH_POSITIONS];
  int scores[NUM_BENCH_POSITIONS];
  uint64_t nodes[NUM_BENCH_POSITIONS];
  long times[NUM_BENCH_POSITIONS];

  long startTime = GetTimeMS();
  for (int i = 0; i < NUM_BENCH_POSITIONS; i++) {
    ParseFen(benchmarks[i], &board);

    TTClear();
    SearchClear();

    Limits.start = GetTimeMS();
    StartSearch(&board, 0);
    ThreadWaitUntilSleep(Threads.threads[0]);
    times[i] = GetTimeMS() - Limits.start;

    bestMoves[i] = Threads.threads[0]->rootMoves[0].move;
    scores[i]    = Threads.threads[0]->rootMoves[0].score;
    nodes[i]     = Threads.threads[0]->nodes;
  }
  long totalTime = GetTimeMS() - startTime;

  printf("\n\n");
  for (int i = 0; i < NUM_BENCH_POSITIONS; i++) {
    printf("Bench [#%2d]: bestmove %5s score %5d %12" PRIu64 " nodes %8d nps | %71s\n",
           i + 1,
           MoveToStr(bestMoves[i], &board),
           (int) Normalize(scores[i]),
           nodes[i],
           (int) (1000.0 * nodes[i] / (times[i] + 1)),
           benchmarks[i]);
  }

  uint64_t totalNodes = 0;
  for (int i = 0; i < NUM_BENCH_POSITIONS; i++)
    totalNodes += nodes[i];

  printf("\nResults: %43" PRIu64 " nodes %8d nps\n\n", totalNodes, (int) (1000.0 * totalNodes / (totalTime + 1)));
}

// -----------------------------------------------------------------------------
// Nexus 6.1 BenchDetail — extended bench with per-position instrumentation
// -----------------------------------------------------------------------------
//
// Runs the SAME 50 positions at the SAME depth as Bench(), so the total node
// count must match (2,811,728 at default depth). In addition, prints per-
// position: TT probe/hit counts and rate, qsearch node count and percentage
// of total, aspiration retries, PV changes, seldepth, and bestmove from-to.
//
// Requires NEXUS_SUMMARY (use 'make profile' or 'make debug'). In a release
// build this function is never called from main(); the CLI handler falls
// back to standard Bench() output with a notice.
void BenchDetail(int depth) {
  Board board;

  Limits.depth   = depth;
  Limits.multiPV = 1;
  Limits.hitrate = INT_MAX;
  Limits.max     = INT_MAX;
  Limits.timeset = 0;

  Move bestMoves[NUM_BENCH_POSITIONS];
  int scores[NUM_BENCH_POSITIONS];
  int seldepths[NUM_BENCH_POSITIONS];
  uint64_t nodes[NUM_BENCH_POSITIONS];
  long times[NUM_BENCH_POSITIONS];

  long startTime = GetTimeMS();

  printf("\nNexus 6.1 BenchDetail (depth %d) — extended per-position metrics\n", depth);
  printf("Build: debug=%d profile=%d summary=%d\n\n",
         NexusBuildHasDebug(), NexusBuildHasProfile(), NexusBuildHasSummary());

  for (int i = 0; i < NUM_BENCH_POSITIONS; i++) {
    ParseFen(benchmarks[i], &board);

    TTClear();
    SearchClear();
#if defined(NEXUS_PROFILE)
    NexusProfileReset();
#endif
#if defined(NEXUS_SUMMARY)
    NexusSummaryReset();
#endif

    Limits.start = GetTimeMS();
    StartSearch(&board, 0);
    ThreadWaitUntilSleep(Threads.threads[0]);
    times[i] = GetTimeMS() - Limits.start;

    bestMoves[i]  = Threads.threads[0]->rootMoves[0].move;
    scores[i]     = Threads.threads[0]->rootMoves[0].score;
    seldepths[i]  = Threads.threads[0]->rootMoves[0].seldepth;
    nodes[i]      = Threads.threads[0]->nodes;

    NexusSummaryPrintBenchDetail(i, benchmarks[i],
                                 depth, seldepths[i], (int64_t)nodes[i],
                                 FromTo(bestMoves[i]), (int)Normalize(scores[i]),
                                 times[i]);
  }

  long totalTime = GetTimeMS() - startTime;

  uint64_t totalNodes = 0;
  for (int i = 0; i < NUM_BENCH_POSITIONS; i++)
    totalNodes += nodes[i];

  printf("\nBenchDetail Results: %" PRIu64 " nodes %8d nps (total time %ld ms)\n",
         totalNodes, (int) (1000.0 * totalNodes / (totalTime + 1)), totalTime);
  printf("(node count must match standard Bench() = 2,811,728 at default depth)\n\n");
}