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
// Nexus 6.1 Instrumentation Layer — implementation
// =============================================================================
//
// See instrument.h for the design. All globals are zero-initialized so the
// non-instrumented paths incur no setup cost.

#include "instrument.h"

#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

// -----------------------------------------------------------------------------
// Globals (only compiled when the matching flag is on, but we still need the
// symbols to exist for the no-op stubs below to link cleanly)
// -----------------------------------------------------------------------------
#ifdef NEXUS_DEBUG
uint32_t NexusDebugFlags = 0;
#endif

#ifdef NEXUS_PROFILE
int64_t NexusProfileCounters[NPC_COUNT] = {0};
#endif

#ifdef NEXUS_SUMMARY
NexusSearchSummary NexusSummary;
#endif

// Runtime on/off switches — always present.
int NexusExplainEnabled = 0;
int NexusProfileReportEnabled = 0;
int NexusDebugEnabledRuntime = 0;

// -----------------------------------------------------------------------------
// Build capability probes (always compiled; used by UCI to print honest
// capability info to the GUI)
// -----------------------------------------------------------------------------
int NexusBuildHasDebug(void) {
#ifdef NEXUS_DEBUG
  return 1;
#else
  return 0;
#endif
}

int NexusBuildHasProfile(void) {
#ifdef NEXUS_PROFILE
  return 1;
#else
  return 0;
#endif
}

int NexusBuildHasSummary(void) {
#ifdef NEXUS_SUMMARY
  return 1;
#else
  return 0;
#endif
}

// -----------------------------------------------------------------------------
// Debug module control
// -----------------------------------------------------------------------------
void NexusDebugSetModules(uint32_t flags) {
#ifdef NEXUS_DEBUG
  NexusDebugFlags = flags;
#else
  (void)flags;
#endif
}

// Parse a UCI-style string like "tt,search,time" or "all" or "none" into flags.
void NexusDebugSetFromUciString(const char* s) {
#ifdef NEXUS_DEBUG
  uint32_t flags = 0;
  char buf[256];
  strncpy(buf, s, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';

  char* tok = strtok(buf, " ,;");
  while (tok) {
    // lowercase compare
    for (char* p = tok; *p; p++) *p = (char)tolower((unsigned char)*p);
    if (!strcmp(tok, "tt"))              flags |= NDBG_TT;
    else if (!strcmp(tok, "search"))     flags |= NDBG_SEARCH;
    else if (!strcmp(tok, "qsearch"))    flags |= NDBG_QSEARCH;
    else if (!strcmp(tok, "root"))       flags |= NDBG_ROOT;
    else if (!strcmp(tok, "time"))       flags |= NDBG_TIME;
    else if (!strcmp(tok, "movegen"))    flags |= NDBG_MOVEGEN;
    else if (!strcmp(tok, "eval"))       flags |= NDBG_EVAL;
    else if (!strcmp(tok, "movepick"))   flags |= NDBG_MOVEPICK;
    else if (!strcmp(tok, "aspiration")) flags |= NDBG_ASPIRATION;
    else if (!strcmp(tok, "pruning"))    flags |= NDBG_PRUNING;
    else if (!strcmp(tok, "extensions")) flags |= NDBG_EXTENSIONS;
    else if (!strcmp(tok, "pv"))         flags |= NDBG_PV;
    else if (!strcmp(tok, "all"))        flags  = NDBG_ALL;
    else if (!strcmp(tok, "none"))       flags  = 0;
    tok = strtok(NULL, " ,;");
  }
  NexusDebugFlags = flags;
#else
  (void)s;
#endif
}

// -----------------------------------------------------------------------------
// Profile counters
// -----------------------------------------------------------------------------
void NexusProfileReset(void) {
#ifdef NEXUS_PROFILE
  memset(NexusProfileCounters, 0, sizeof(NexusProfileCounters));
#endif
}

void NexusProfileReport(void) {
#ifdef NEXUS_PROFILE
  printf("info string profile begin\n");
  printf("info string profile negamax_calls      %" PRId64 "\n", NexusProfileCounters[NPC_NEGAMAX_CALLS]);
  printf("info string profile quiesce_calls      %" PRId64 "\n", NexusProfileCounters[NPC_QUIESCE_CALLS]);
  printf("info string profile movegen_calls      %" PRId64 "\n", NexusProfileCounters[NPC_MOVEGEN_CALLS]);
  printf("info string profile eval_calls         %" PRId64 "\n", NexusProfileCounters[NPC_EVAL_CALLS]);
  printf("info string profile tt_probes          %" PRId64 "\n", NexusProfileCounters[NPC_TT_PROBES]);
  printf("info string profile tt_hits            %" PRId64 "\n", NexusProfileCounters[NPC_TT_HITS]);
  printf("info string profile tt_puts            %" PRId64 "\n", NexusProfileCounters[NPC_TT_PUTS]);
  printf("info string profile tt_score_cuts      %" PRId64 "\n", NexusProfileCounters[NPC_TT_SCORE_CUTS]);
  printf("info string profile tt_hit_rate_pct    %.2f\n",
         NexusProfileCounters[NPC_TT_PROBES] > 0
             ? 100.0 * NexusProfileCounters[NPC_TT_HITS] / NexusProfileCounters[NPC_TT_PROBES]
             : 0.0);
  printf("info string profile aspiration_retries %" PRId64 "\n", NexusProfileCounters[NPC_ASPIRATION_RETRIES]);
  printf("info string profile null_move_cuts     %" PRId64 "\n", NexusProfileCounters[NPC_NULL_MOVE_CUTS]);
  printf("info string profile lmr_reductions     %" PRId64 "\n", NexusProfileCounters[NPC_LMR_REDUCTIONS]);
  printf("info string profile rfp_cuts           %" PRId64 "\n", NexusProfileCounters[NPC_RFP_CUTS]);
  printf("info string profile futility_prunes    %" PRId64 "\n", NexusProfileCounters[NPC_FUTILITY_PRUNES]);
  printf("info string profile see_prunes         %" PRId64 "\n", NexusProfileCounters[NPC_SEE_PRUNES]);
  printf("info string profile singular_ext       %" PRId64 "\n", NexusProfileCounters[NPC_SINGULAR_EXTENSIONS]);
  printf("info string profile check_ext         %" PRId64 "\n", NexusProfileCounters[NPC_CHECK_EXTENSIONS]);
  printf("info string profile pv_changes        %" PRId64 "\n", NexusProfileCounters[NPC_PV_CHANGES]);
  printf("info string profile qs_standpat_cuts  %" PRId64 "\n", NexusProfileCounters[NPC_QS_STANDPAT_CUTS]);
  printf("info string profile qs_delta_prunes   %" PRId64 "\n", NexusProfileCounters[NPC_QS_DELTA_PRUNES]);
  printf("info string profile root_move_count   %" PRId64 "\n", NexusProfileCounters[NPC_ROOT_MOVE_COUNT]);
  printf("info string profile time_checks       %" PRId64 "\n", NexusProfileCounters[NPC_TIME_CHECKS]);
  printf("info string profile time_panic        %" PRId64 "\n", NexusProfileCounters[NPC_TIME_PANIC]);
  printf("info string profile end\n");
#endif
}

// -----------------------------------------------------------------------------
// Search summary
// -----------------------------------------------------------------------------
void NexusSummaryReset(void) {
#ifdef NEXUS_SUMMARY
  memset(&NexusSummary, 0, sizeof(NexusSummary));
  NexusSummary.finalDepth = 0;
  NexusSummary.finalSeldepth = 0;
#endif
}

void NexusSummaryRecordIteration(int depth, int seldepth, int64_t nodes,
                                 int bestMoveFromTo, int64_t bestMoveNodes,
                                 int rootMoveCount) {
#ifdef NEXUS_SUMMARY
  static int lastBestMove = -1;
  if (depth > 1 && bestMoveFromTo != lastBestMove && lastBestMove != -1)
    NexusSummary.pvChanges++;
  lastBestMove = bestMoveFromTo;

  NexusSummary.finalDepth = depth;
  NexusSummary.finalSeldepth = seldepth;
  NexusSummary.totalNodes = nodes;
  NexusSummary.rootMoveCount = rootMoveCount;

  // Maintain a top-5 by nodes (simple insertion into a small sorted array)
  int idx = -1;
  for (int i = 0; i < NexusSummary.topMoveCount; i++)
    if (NexusSummary.topMoveFromTo[i] == bestMoveFromTo) { idx = i; break; }
  if (idx < 0 && NexusSummary.topMoveCount < 5) {
    idx = NexusSummary.topMoveCount++;
    NexusSummary.topMoveFromTo[idx] = bestMoveFromTo;
  }
  if (idx >= 0) NexusSummary.topMoveNodes[idx] = bestMoveNodes;

  // Re-sort top-5 by nodes descending
  for (int i = 1; i < NexusSummary.topMoveCount; i++) {
    int j = i;
    while (j > 0 && NexusSummary.topMoveNodes[j-1] < NexusSummary.topMoveNodes[j]) {
      int64_t tn = NexusSummary.topMoveNodes[j]; NexusSummary.topMoveNodes[j] = NexusSummary.topMoveNodes[j-1]; NexusSummary.topMoveNodes[j-1] = tn;
      int tf = NexusSummary.topMoveFromTo[j]; NexusSummary.topMoveFromTo[j] = NexusSummary.topMoveFromTo[j-1]; NexusSummary.topMoveFromTo[j-1] = tf;
      j--;
    }
  }
#else
  (void)depth; (void)seldepth; (void)nodes; (void)bestMoveFromTo; (void)bestMoveNodes; (void)rootMoveCount;
#endif
}

void NexusSummaryRecordCutoff(int kind) {
#ifdef NEXUS_SUMMARY
  switch (kind) {
    case 0: NexusSummary.cutoffsTT++; break;
    case 1: NexusSummary.cutoffsNull++; break;
    case 2: NexusSummary.cutoffsRFP++; break;
    case 3: NexusSummary.cutoffsFutility++; break;
    case 4: NexusSummary.cutoffsSEE++; break;
    default: NexusSummary.cutoffsOther++; break;
  }
#else
  (void)kind;
#endif
}

void NexusSummaryRecordTT(int hit) {
#ifdef NEXUS_SUMMARY
  NexusSummary.ttProbes++;
  if (hit) NexusSummary.ttHits++;
#else
  (void)hit;
#endif
}

void NexusSummaryRecordQsearchNodes(int64_t n) {
#ifdef NEXUS_SUMMARY
  NexusSummary.qsearchNodes += n;
#else
  (void)n;
#endif
}

void NexusSummaryRecordAspirationRetry(void) {
#ifdef NEXUS_SUMMARY
  NexusSummary.aspirationRetries++;
#endif
}

void NexusSummaryFinalize(int timeAllocMs, int timeUsedMs) {
#ifdef NEXUS_SUMMARY
  NexusSummary.timeAllocMs = timeAllocMs;
  NexusSummary.timeUsedMs = timeUsedMs;
  NexusSummary.panicTriggered = (timeUsedMs > timeAllocMs && timeAllocMs > 0) ? 1 : 0;
#else
  (void)timeAllocMs; (void)timeUsedMs;
#endif
}

void NexusSummaryPrintExplain(void) {
#ifdef NEXUS_SUMMARY
  printf("info string explain begin\n");
  printf("info string explain final_depth      %d\n", NexusSummary.finalDepth);
  printf("info string explain final_seldepth   %d\n", NexusSummary.finalSeldepth);
  printf("info string explain total_nodes      %" PRId64 "\n", NexusSummary.totalNodes);
  printf("info string explain qsearch_nodes    %" PRId64 " (%.1f%% of total)\n",
         NexusSummary.qsearchNodes,
         NexusSummary.totalNodes > 0 ? 100.0 * NexusSummary.qsearchNodes / NexusSummary.totalNodes : 0.0);
  printf("info string explain tt_probes        %" PRId64 "\n", NexusSummary.ttProbes);
  printf("info string explain tt_hits          %" PRId64 " (%.1f%% hit rate)\n",
         NexusSummary.ttHits,
         NexusSummary.ttProbes > 0 ? 100.0 * NexusSummary.ttHits / NexusSummary.ttProbes : 0.0);
  printf("info string explain aspiration_retries %d\n", NexusSummary.aspirationRetries);
  printf("info string explain pv_changes       %d\n", NexusSummary.pvChanges);
  printf("info string explain root_move_count  %d\n", NexusSummary.rootMoveCount);
  printf("info string explain cutoffs_tt       %" PRId64 "\n", NexusSummary.cutoffsTT);
  printf("info string explain cutoffs_null     %" PRId64 "\n", NexusSummary.cutoffsNull);
  printf("info string explain cutoffs_rfp      %" PRId64 "\n", NexusSummary.cutoffsRFP);
  printf("info string explain cutoffs_futility %" PRId64 "\n", NexusSummary.cutoffsFutility);
  printf("info string explain cutoffs_see      %" PRId64 "\n", NexusSummary.cutoffsSEE);
  printf("info string explain cutoffs_other    %" PRId64 "\n", NexusSummary.cutoffsOther);
  printf("info string explain time_alloc_ms    %d\n", NexusSummary.timeAllocMs);
  printf("info string explain time_used_ms     %d\n", NexusSummary.timeUsedMs);
  printf("info string explain time_panic       %s\n", NexusSummary.panicTriggered ? "true" : "false");

  // Top moves by node share
  printf("info string explain top_moves_by_nodes");
  for (int i = 0; i < NexusSummary.topMoveCount; i++) {
    double share = NexusSummary.totalNodes > 0
                       ? 100.0 * NexusSummary.topMoveNodes[i] / NexusSummary.totalNodes
                       : 0.0;
    printf(" %d(%.1f%%)", NexusSummary.topMoveFromTo[i], share);
  }
  printf("\n");

  // Build a human-readable summary line
  int bestMove = NexusSummary.topMoveCount > 0 ? NexusSummary.topMoveFromTo[0] : 0;
  int ff = (bestMove >> 6) & 0x3f;  // from square (rough decode)
  int tt = bestMove & 0x3f;          // to square (rough decode)
  printf("info string explain summary Chose move with from-to %d-%d (sq %d->%d) "
         "at depth %d (seldepth %d), %" PRId64 " nodes, "
         "%.1f%% TT hit rate, %d aspiration retries, %d PV changes, "
         "used %dms of %dms allocated.\n",
         ff, tt, ff, tt,
         NexusSummary.finalDepth, NexusSummary.finalSeldepth,
         NexusSummary.totalNodes,
         NexusSummary.ttProbes > 0 ? 100.0 * NexusSummary.ttHits / NexusSummary.ttProbes : 0.0,
         NexusSummary.aspirationRetries, NexusSummary.pvChanges,
         NexusSummary.timeUsedMs, NexusSummary.timeAllocMs);
  printf("info string explain end\n");
#else
  printf("info string explain not_available (build was compiled without NEXUS_SUMMARY; use 'make profile' or 'make debug')\n");
#endif
}

void NexusSummaryPrintBenchDetail(int positionIdx, const char* fen,
                                  int depth, int seldepth, int64_t nodes,
                                  int bestMoveFromTo, int score,
                                  long timeMs) {
#ifdef NEXUS_SUMMARY
  // Per-position NPS, guarded against divide-by-zero.
  int nps = timeMs > 0 ? (int)(1000.0 * nodes / timeMs) : 0;

  printf("BenchDetail [#%2d]: depth %d seldepth %d nodes %" PRId64 " "
         "time_ms %ld nps %d "
         "tt_probes %" PRId64 " tt_hits %" PRId64 " (%.1f%%) "
         "qsearch_nodes %" PRId64 " (%.1f%%) "
         "aspiration_retries %d pv_changes %d "
         "bestmove_fromto %d score %d | %s\n",
         positionIdx + 1, depth, seldepth, nodes,
         timeMs, nps,
         NexusSummary.ttProbes,
         NexusSummary.ttHits,
         NexusSummary.ttProbes > 0 ? 100.0 * NexusSummary.ttHits / NexusSummary.ttProbes : 0.0,
         NexusSummary.qsearchNodes,
         NexusSummary.totalNodes > 0 ? 100.0 * NexusSummary.qsearchNodes / NexusSummary.totalNodes : 0.0,
         NexusSummary.aspirationRetries, NexusSummary.pvChanges,
         bestMoveFromTo, score, fen);
#else
  (void)positionIdx; (void)fen; (void)depth; (void)seldepth; (void)nodes;
  (void)bestMoveFromTo; (void)score; (void)timeMs;
  printf("info string benchdetail not_available (build was compiled without NEXUS_SUMMARY; use 'make profile' or 'make debug')\n");
#endif
}
