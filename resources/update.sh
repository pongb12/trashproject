#!/bin/bash

set -uex

# Select a compiler, prioritize Clang
if hash clang 2>/dev/null; then
    cc=clang
elif hash gcc 2>/dev/null; then
    cc=gcc
else
    echo "Please install clang or gcc to compile Nexus 6.0!"
    exit 1
fi

# Build Nexus 6.0 from the current source tree (in-place; assumes the
# user has already cloned or extracted the Nexus 6.0 sources into the
# current working directory).
cd "$(dirname "$0")/.."
cd src
make pgo CC=$cc ARCH=native

if test -f nexus; then
    echo "Compilation complete..."
else
    echo "Compilation failed!"
    exit 1
fi

EXE=$PWD/nexus

$EXE bench 13 > bench.log 2>&1
grep Results bench.log
