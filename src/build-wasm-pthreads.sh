#!/bin/bash
# Build Nexus 6.1 as WebAssembly with PTHREADS support
# Requires server with COOP/COEP headers (Cross-Origin-Opener-Policy: same-origin
# + Cross-Origin-Embedder-Policy: require-corp) for SharedArrayBuffer to work.
#
# Output: nexus6.1-pthreads.js + nexus6.1-pthreads.wasm + nexus6.1-pthreads.worker.js
set -e

cd "$(dirname "$0")"

EMCC=${EMCC:-emcc}
VERSION=6.1.0
NETWORK=nexus-9b84c340af7e.nn

# Source files
SRC="attacks.c bench.c nexus.c bits.c board.c eval.c history.c instrument.c move.c movegen.c movepick.c perft.c random.c search.c see.c tb.c thread.c transposition.c uci.c util.c zobrist.c nn/accumulator.c nn/evaluate_wasm.c pyrrhic/tbprobe.c"

# Compiler flags
FLAGS="-std=gnu11 -O2 -g0 -DNDEBUG"
FLAGS+=" -DVERSION=\"$VERSION\" -DEVALFILE=\"$NETWORK\""
FLAGS+=" -D__EMSCRIPTEN__"

# Emscripten linker flags — PTHREADS build
EMFLAGS="-s ENVIRONMENT=web,worker"
# UMD exports (NOT ES6) — pthreads worker requires classic script
EMFLAGS+=" -s MODULARIZE=1 -s EXPORT_ES6=0"
EMFLAGS+=" -s EXPORT_NAME=createNexusModule"
EMFLAGS+=" -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','UTF8ToString','stringToUTF8','lengthBytesUTF8','FS_createDataFile','FS_unlink','FS_stat','FS']"
EMFLAGS+=" -s EXPORTED_FUNCTIONS=['_main','_malloc','_free','_nexus_push_command','_nexus_run_uci']"
EMFLAGS+=" -s ALLOW_MEMORY_GROWTH=1 -s INITIAL_MEMORY=512mb -s MAXIMUM_MEMORY=1024mb"
EMFLAGS+=" -s FORCE_FILESYSTEM=1"
EMFLAGS+=" -s WASM=1"
EMFLAGS+=" -s SINGLE_FILE=0"
EMFLAGS+=" -s STACK_SIZE=32mb"
EMFLAGS+=" -s SUPPORT_LONGJMP=emscripten"
# PTHREADS — requires SharedArrayBuffer + COOP/COEP headers on server
EMFLAGS+=" -s USE_PTHREADS=1"
EMFLAGS+=" -s PTHREAD_POOL_SIZE=4"
EMFLAGS+=" -s ALLOW_BLOCKING_ON_MAIN_THREAD=1"

# Output: nexus6.1-pthreads.* (separate from single-threaded build)
OUTPUT=../nexus6.1-pthreads

echo "Building Nexus 6.1 WASM with PTHREADS..."
echo "Sources: $SRC"
echo "Output: $OUTPUT.js + $OUTPUT.wasm + $OUTPUT.worker.js"
echo ""

$EMCC $FLAGS $SRC $EMFLAGS -o $OUTPUT.js

echo ""
echo "Build complete!"
ls -la $OUTPUT.js $OUTPUT.wasm $OUTPUT.worker.js 2>&1
echo ""
echo "Network file (must be deployed alongside):"
ls -la $NETWORK 2>&1
echo ""
echo "IMPORTANT: Server must send these headers for SharedArrayBuffer to work:"
echo "  Cross-Origin-Opener-Policy: same-origin"
echo "  Cross-Origin-Embedder-Policy: require-corp"
