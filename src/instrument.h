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

// =============================================================================
// Nexus 6.1 Instrumentation Layer
// =============================================================================
//
// This header provides three orthogonal instrumentation subsystems, each
// gated by a compile-time macro so that the DEFAULT release build remains
// byte-for-byte identical to the Nexus 6.0 baseline (verified by bench node
// count = 2,811,728).
//
//   NEXUS_DEBUG    - per-module debug logging via info string debug ...
//   NEXUS_PROFILE  - per-subsystem performance counters
//   NEXUS_SUMMARY  - search-summary tracking for explain-mode + benchdetail
//
// All three are OFF by default. Enable them with:
//   make debug     -> -O0 -g -DNEXUS_DEBUG -DNEXUS_PROFILE -DNEXUS_SUMMARY -fsanitize=address,undefined
//   make profile   -> -O2       -DNEXUS_PROFILE -DNEXUS_SUMMARY
//   make release   -> standard PGO build (no instrumentation)
//
// When a flag is OFF, every macro in this header expands to nothing, so
// the calling code is untouched and the compiler emits identical code to
// the 6.0 baseline.
//
// The runtime UCI options DebugModules / ExplainMode / ProfileReport are
// ALWAYS declared (so UCI handshakes are identical across builds) but only
// ACT on their setting when the matching compile flag is enabled. In a
// release build they print an info-string notice and no-op.

#ifndef NEXUS_INSTRUMENT_H
#define NEXUS_INSTRUMENT_H

#include <stdint.h>
#include <stdio.h>

// -----------------------------------------------------------------------------
// Debug module flags (used by NEXUS_DEBUG only)
// -----------------------------------------------------------------------------
enum NexusDebugModule {
  NDBG_TT         = 1 << 0,  // transposition table probe/put/hit/miss
  NDBG_SEARCH     = 1 << 1,  // negamax entry/exit, cutoffs, bounds
  NDBG_QSEARCH    = 1 << 2,  // quiescence entry/exit, stand-pat
  NDBG_ROOT       = 1 << 3,  // root move ordering, iteration results
  NDBG_TIME       = 1 << 4,  // time allocation, soft/hard checks, panic
  NDBG_MOVEGEN    = 1 << 5,  // move generation counts
  NDBG_EVAL       = 1 << 6,  // evaluation calls
  NDBG_MOVEPICK   = 1 << 7,  // move picker phase transitions
  NDBG_ASPIRATION = 1 << 8,  // aspiration window re-searches
  NDBG_PRUNING    = 1 << 9,  // null-move, LMR, RFP, razoring, futility
  NDBG_EXTENSIONS = 1 << 10, // singular, check, etc.
  NDBG_PV         = 1 << 11, // PV changes between iterations
  NDBG_ALL        = (1 << 12) - 1
};

// -----------------------------------------------------------------------------
// Profile counters (used by NEXUS_PROFILE only)
// -----------------------------------------------------------------------------
enum NexusProfileCounter {
  // call counts
  NPC_NEGAMAX_CALLS,
  NPC_QUIESCE_CALLS,
  NPC_MOVEGEN_CALLS,
  NPC_EVAL_CALLS,
  NPC_TT_PROBES,
  NPC_TT_HITS,
  NPC_TT_PUTS,
  NPC_TT_SCORE_CUTS,    // TT-based score cutoffs (returned ttScore directly)
  NPC_ASPIRATION_RETRIES,
  NPC_NULL_MOVE_CUTS,
  NPC_LMR_REDUCTIONS,
  NPC_RFP_CUTS,         // reverse futility pruning cuts
  NPC_FUTILITY_PRUNES,
  NPC_SEE_PRUNES,
  NPC_SINGULAR_EXTENSIONS,
  NPC_CHECK_EXTENSIONS,
  NPC_PV_CHANGES,
  NPC_QS_STANDPAT_CUTS,
  NPC_QS_DELTA_PRUNES,
  NPC_ROOT_MOVE_COUNT,
  NPC_TIME_CHECKS,
  NPC_TIME_PANIC,       // hard time limit triggered
  NPC_COUNT
};

// -----------------------------------------------------------------------------
// Search summary (used by NEXUS_SUMMARY only)
//
// This struct is populated at low frequency (once per root iteration, once
// per TT probe) and read by ExplainMode and benchdetail. It records
// information needed to answer "why did the engine choose this move?"
// without emitting per-node log noise.
// -----------------------------------------------------------------------------
typedef struct {
  // final iteration stats
  int finalDepth;
  int finalSeldepth;
  int64_t totalNodes;
  int64_t qsearchNodes;
  int64_t ttProbes;
  int64_t ttHits;
  int aspirationRetries;
  int pvChanges;          // how many times best move changed across iterations
  int rootMoveCount;

  // root move ordering: top 5 moves by node count (move in from-to form, nodes)
  int topMoveCount;
  int topMoveFromTo[5];
  int64_t topMoveNodes[5];

  // cutoff source breakdown (counts)
  int64_t cutoffsTT;
  int64_t cutoffsNull;
  int64_t cutoffsRFP;
  int64_t cutoffsFutility;
  int64_t cutoffsSEE;
  int64_t cutoffsOther;

  // time manager
  int timeAllocMs;
  int timeUsedMs;
  int panicTriggered;
} NexusSearchSummary;

// -----------------------------------------------------------------------------
// Global state (defined in instrument.c; zero-initialized)
// -----------------------------------------------------------------------------
#ifdef NEXUS_DEBUG
extern uint32_t NexusDebugFlags;
#endif

#ifdef NEXUS_PROFILE
extern int64_t NexusProfileCounters[NPC_COUNT];
#endif

#ifdef NEXUS_SUMMARY
extern NexusSearchSummary NexusSummary;
#endif

// Runtime on/off switches for the post-search reports. These are ALWAYS
// present (even in non-instrumented builds) so that the UCI options can
// be set without crashing. In a non-instrumented build, setting them to 1
// has no effect — the corresponding report function is a no-op.
extern int NexusExplainEnabled;
extern int NexusProfileReportEnabled;
extern int NexusDebugEnabledRuntime;

// -----------------------------------------------------------------------------
// Debug macros — compile to nothing unless NEXUS_DEBUG is defined
// -----------------------------------------------------------------------------
#ifdef NEXUS_DEBUG
  #define NEXUS_DEBUG_LOG(module, fmt, ...)                       \
    do {                                                          \
      if (NexusDebugFlags & (module)) {                           \
        printf("info string debug " fmt "\n", ##__VA_ARGS__);     \
      }                                                           \
    } while (0)
  #define NEXUS_DEBUG_ENABLED()  (1)
#else
  #define NEXUS_DEBUG_LOG(module, fmt, ...)  ((void)0)
  #define NEXUS_DEBUG_ENABLED()  (0)
#endif

// -----------------------------------------------------------------------------
// Profile macros — compile to nothing unless NEXUS_PROFILE is defined
// -----------------------------------------------------------------------------
#ifdef NEXUS_PROFILE
  #define NEXUS_PROFILE_INC(counter)  (NexusProfileCounters[(counter)]++)
  #define NEXUS_PROFILE_ADD(counter, n) (NexusProfileCounters[(counter)] += (n))
  #define NEXUS_PROFILE_ENABLED() (1)
#else
  #define NEXUS_PROFILE_INC(counter)    ((void)0)
  #define NEXUS_PROFILE_ADD(counter, n) ((void)0)
  #define NEXUS_PROFILE_ENABLED()       (0)
#endif

// -----------------------------------------------------------------------------
// Summary macros — compile to nothing unless NEXUS_SUMMARY is defined
// -----------------------------------------------------------------------------
#ifdef NEXUS_SUMMARY
  #define NEXUS_SUMMARY_ENABLED() (1)
#else
  #define NEXUS_SUMMARY_ENABLED() (0)
#endif

// -----------------------------------------------------------------------------
// Public API (no-ops in non-instrumented builds)
// -----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif

// Always callable; no-op when NEXUS_DEBUG is off.
void NexusDebugSetModules(uint32_t flags);
void NexusDebugSetFromUciString(const char* s);

// Always callable; no-op when NEXUS_PROFILE is off.
void NexusProfileReset(void);
void NexusProfileReport(void);

// Always callable; no-op when NEXUS_SUMMARY is off.
void NexusSummaryReset(void);
void NexusSummaryRecordIteration(int depth, int seldepth, int64_t nodes,
                                 int bestMoveFromTo, int64_t bestMoveNodes,
                                 int rootMoveCount);
void NexusSummaryRecordCutoff(int kind);  // kind: 0=TT 1=null 2=RFP 3=futility 4=SEE 5=other
void NexusSummaryRecordTT(int hit);
void NexusSummaryRecordQsearchNodes(int64_t n);
void NexusSummaryRecordAspirationRetry(void);
void NexusSummaryFinalize(int timeAllocMs, int timeUsedMs);
void NexusSummaryPrintExplain(void);   // prints info string explain ...
void NexusSummaryPrintBenchDetail(int positionIdx, const char* fen,
                                  int depth, int seldepth, int64_t nodes,
                                  int bestMoveFromTo, int score,
                                  long timeMs);

// Returns 1 if the build was compiled with NEXUS_DEBUG, else 0.
int NexusBuildHasDebug(void);
int NexusBuildHasProfile(void);
int NexusBuildHasSummary(void);

#ifdef __cplusplus
}
#endif

#endif // NEXUS_INSTRUMENT_H
