#!/bin/bash
# Build Nexus 6.1 as WebAssembly for browser use
# Single-threaded with proper pthread stubs and setjmp support
set -e

cd "$(dirname "$0")"

EMCC=${EMCC:-emcc}
VERSION=6.1.0
NETWORK=nexus-9b84c340af7e.nn

# Source files
SRC="attacks.c bench.c nexus.c bits.c board.c eval.c history.c instrument.c move.c movegen.c movepick.c perft.c random.c search.c see.c tb.c thread.c transposition.c uci.c util.c zobrist.c nn/accumulator.c nn/evaluate_wasm.c pyrrhic/tbprobe.c"

# Compiler flags
FLAGS="-std=gnu11 -O2 -g -DNDEBUG"
FLAGS+=" -DVERSION=\"$VERSION\" -DEVALFILE=\"$NETWORK\""
FLAGS+=" -D__EMSCRIPTEN__"
FLAGS+=" "  # suppress all warnings for cleaner build

# Emscripten linker flags
EMFLAGS="-s ENVIRONMENT=web,worker,node"
# Export UMD (NOT ES6) — importScripts() in classic worker doesn't support import.meta/export
# With MODULARIZE=1 + EXPORT_ES6=0: creates global `createNexusModule` function via UMD
EMFLAGS+=" -s MODULARIZE=1 -s EXPORT_ES6=0"
EMFLAGS+=" -s EXPORT_NAME=createNexusModule"
EMFLAGS+=" -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','UTF8ToString','stringToUTF8','lengthBytesUTF8']"
EMFLAGS+=" -s EXPORTED_FUNCTIONS=['_main','_malloc','_free','_nexus_push_command','_nexus_run_uci']"
EMFLAGS+=" -s ALLOW_MEMORY_GROWTH=1 -s INITIAL_MEMORY=512mb -s MAXIMUM_MEMORY=1024mb"
EMFLAGS+=" -s FORCE_FILESYSTEM=1"
EMFLAGS+=" --embed-file $NETWORK"
EMFLAGS+=" -s WASM=1"
EMFLAGS+=" -s SINGLE_FILE=0"
EMFLAGS+=" -s STACK_SIZE=32mb"
EMFLAGS+=" -s SUPPORT_LONGJMP=emscripten"

echo "Building Nexus 6.1 WASM..."
echo "Sources: $SRC"
echo ""

$EMCC $FLAGS $SRC $EMFLAGS --emit-symbol-map -o ../nexus6.1.js

echo ""
echo "Build complete!"
echo "Output: ../nexus6.1.js + ../nexus6.1.wasm"
ls -la ../nexus6.1.js ../nexus6.1.wasm 2>&1
