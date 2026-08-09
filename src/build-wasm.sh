#!/bin/bash
# Build Nexus 6.1 as WebAssembly for browser use
# Single-threaded (stubs out pthreads)
set -e

cd "$(dirname "$0")"

EMCC=${EMCC:-emcc}
VERSION=6.1.0
NETWORK=nexus-9b84c340af7e.nn

# Source files (same as native build)
SRC="attacks.c bench.c nexus.c bits.c board.c eval.c history.c instrument.c move.c movegen.c movepick.c perft.c random.c search.c see.c tb.c thread.c transposition.c uci.c util.c zobrist.c nn/accumulator.c nn/evaluate_wasm.c pyrrhic/tbprobe.c"

# Emscripten flags
FLAGS="-std=gnu11 -O2 -DNDEBUG"
FLAGS+=" -DVERSION=\"$VERSION\" -DEVALFILE=\"$NETWORK\""
FLAGS+=" -D__EMSCRIPTEN__"  # marker for WASM-specific code
FLAGS+=" -Wall -Wno-unused-function -Wno-unused-variable"

# Output: ES6 module + separate WASM
EMFLAGS="-s ENVIRONMENT=web"
EMFLAGS+=" -s MODULARIZE=1 -s EXPORT_ES6=1"
EMFLAGS+=" -s EXPORT_NAME=createNexusModule"
EMFLAGS+=" -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','UTF8ToString','stringToUTF8','lengthBytesUTF8']"
EMFLAGS+=" -s EXPORTED_FUNCTIONS=['_main','_malloc','_free']"
EMFLAGS+=" -s ALLOW_MEMORY_GROWTH=1 -s INITIAL_MEMORY=128mb -s MAXIMUM_MEMORY=256mb"
EMFLAGS+=" -s FORCE_FILESYSTEM=1"  # for NNUE file loading
EMFLAGS+=" --embed-file $NETWORK"  # embed NNUE network
EMFLAGS+=" -s WASM=1"
EMFLAGS+=" -s SINGLE_FILE=0"  # separate .js + .wasm

echo "Building Nexus 6.1 WASM..."
echo "Sources: $SRC"
echo ""

$EMCC $FLAGS $SRC $EMFLAGS -o ../nexus6.1.js

echo ""
echo "Build complete!"
echo "Output: ../nexus6.1.js + ../nexus6.1.wasm"
ls -la ../nexus6.1.js ../nexus6.1.wasm 2>&1
