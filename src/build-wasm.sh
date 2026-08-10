#!/bin/bash
# Build Nexus 6.1 as WebAssembly for browser use
# Pthread-based: the engine's search threads (thread.c uses real pthreads) run on
# Emscripten pthread workers. Requires SharedArrayBuffer, so the page must be
# served with COOP/COEP headers (crossOriginIsolated) — see nexus-worker.js.
#
# Optimizations vs previous build:
# - Remove --embed-file (network is fetched separately with progress bar)
# - Remove -g (debug info bloats WASM by ~500KB)
# - Use -Os (size optimization) instead of -O2
# - Result: WASM drops from 26MB to ~1MB; network stays 25MB but loaded with progress
set -e

cd "$(dirname "$0")"

EMCC=${EMCC:-emcc}
VERSION=6.1.0
NETWORK=nexus-9b84c340af7e.nn

# Source files
SRC="attacks.c bench.c nexus.c bits.c board.c eval.c history.c instrument.c move.c movegen.c movepick.c perft.c random.c search.c see.c tb.c thread.c transposition.c uci.c util.c zobrist.c nn/accumulator.c nn/evaluate_wasm.c pyrrhic/tbprobe.c"

# Compiler flags — -Os for size, no -g (debug info removed)
FLAGS="-std=gnu11 -Os -g0 -DNDEBUG"
FLAGS+=" -DVERSION=\"$VERSION\" -DEVALFILE=\"$NETWORK\""
FLAGS+=" -D__EMSCRIPTEN__"

# Emscripten linker flags
EMFLAGS="-s ENVIRONMENT=web,worker,node"
# Export UMD (NOT ES6) — importScripts() in classic worker doesn't support import.meta/export
EMFLAGS+=" -s MODULARIZE=1 -s EXPORT_ES6=0"
EMFLAGS+=" -s EXPORT_NAME=createNexusModule"
EMFLAGS+=" -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','UTF8ToString','stringToUTF8','lengthBytesUTF8','FS_createDataFile','FS_unlink','FS_stat','FS']"
# Pump API (JS bridge) — nexus_run_uci was removed in the uci.c refactor
EMFLAGS+=" -s EXPORTED_FUNCTIONS=['_main','_malloc','_free','_nexus_init','_nexus_push_command','_nexus_pump','_nexus_destroy']"
EMFLAGS+=" -s ALLOW_MEMORY_GROWTH=1 -s INITIAL_MEMORY=512mb -s MAXIMUM_MEMORY=1024mb"
EMFLAGS+=" -s FORCE_FILESYSTEM=1"
# Removed: --embed-file $NETWORK  (network is now fetched separately with progress)
EMFLAGS+=" -s WASM=1"
EMFLAGS+=" -s SINGLE_FILE=0"
EMFLAGS+=" -s STACK_SIZE=32mb"
EMFLAGS+=" -s SUPPORT_LONGJMP=emscripten"
# Real threads: thread.c uses pthread_create/cond_wait unconditionally, so a
# single-threaded build cannot search. main() must not auto-run either — it
# would block in UCILoop()'s fgets loop. The worker calls nexus_init() instead.
EMFLAGS+=" -pthread -sPTHREAD_POOL_SIZE=4 -sINVOKE_RUN=0"

echo "Building Nexus 6.1 WASM (no embedded network, pthread search)..."
echo "Sources: $SRC"
echo ""

$EMCC $FLAGS $SRC $EMFLAGS -o ../nexus6.1.js

echo ""
echo "Build complete!"
echo "Output: ../nexus6.1.js + ../nexus6.1.wasm (network NOT embedded — fetched separately)"
echo "NOTE: requires SharedArrayBuffer — serve with COOP/COEP headers"
ls -la ../nexus6.1.js ../nexus6.1.wasm 2>&1
echo ""
echo "Network file (must be deployed alongside):"
ls -la $NETWORK 2>&1
