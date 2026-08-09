#!/bin/bash
# Nexus 6.1 validation gate — the single command to run before merging any
# patch. Builds the engine, runs the full regression suite, and reports.
#
# Usage:
#   bash scripts/validate.sh              # default release build
#   bash scripts/validate.sh profile      # profile build (with instrumentation)
#   bash scripts/validate.sh debug        # debug build (ASan/UBSan)
#
# Exit codes:
#   0 — all checks passed
#   1 — one or more checks failed
#   2 — build failed
set -u

cd "$(dirname "$0")/.."

BUILD_MODE="${1:-release}"
CC="${CC:-gcc}"
ARCH="${ARCH:-avx2}"

echo "############################################################"
echo "#  Nexus 6.1 Validation Gate"
echo "#  Build mode: $BUILD_MODE  CC: $CC  ARCH: $ARCH"
echo "############################################################"
echo ""

# ---- Step 1: Build ----
echo "==> [1/4] Building nexus ($BUILD_MODE)..."
case "$BUILD_MODE" in
  release)  (cd src && make clean CC=$CC >/dev/null 2>&1; make build CC=$CC ARCH=$ARCH) ;;
  profile)  (cd src && make clean CC=$CC >/dev/null 2>&1; make profile CC=$CC ARCH=$ARCH) ;;
  debug)    (cd src && make clean CC=$CC >/dev/null 2>&1; make debug CC=$CC ARCH=$ARCH) ;;
  *) echo "Unknown build mode: $BUILD_MODE (use release|profile|debug)"; exit 2 ;;
esac

if [ ! -x src/nexus6.1 ]; then
  echo "BUILD FAILED — no binary produced"
  exit 2
fi
echo "    Build OK ($(stat -c%s src/nexus6.1) bytes)"
echo ""

# ---- Step 2: UCI handshake + build info ----
echo "==> [2/4] UCI handshake + build info..."
out=$(printf 'uci\nquit\n' | src/nexus6.1 2>&1)
echo "$out" | grep -q '^id name Nexus 6.1'  && echo "    id name: OK"  || { echo "    id name: FAIL"; exit 1; }
echo "$out" | grep -q '^id author Nexus Team' && echo "    id author: OK" || { echo "    id author: FAIL"; exit 1; }
echo "$out" | grep -q '^uciok'              && echo "    uciok: OK"   || { echo "    uciok: FAIL"; exit 1; }
echo ""

# ---- Step 3: Bench regression (release builds only) ----
if [ "$BUILD_MODE" = "release" ]; then
  echo "==> [3/4] Bench regression (must be 2,811,728 nodes)..."
  out=$(src/nexus6.1 bench 2>&1)
  nodes=$(echo "$out" | grep 'Results:' | grep -oE '[0-9]+ nodes' | awk '{print $1}')
  if [ "$nodes" = "2811728" ]; then
    echo "    Bench: OK ($nodes nodes)"
  else
    echo "    Bench: FAIL (got $nodes, expected 2811728)"
    exit 1
  fi
else
  echo "==> [3/4] Bench regression (skipped for $BUILD_MODE build)"
fi
echo ""

# ---- Step 4: Full regression suite ----
echo "==> [4/4] Running full regression suite..."
if bash tests/run_all.sh; then
  echo ""
  echo "############################################################"
  echo "#  VALIDATION PASSED"
  echo "############################################################"
  exit 0
else
  echo ""
  echo "############################################################"
  echo "#  VALIDATION FAILED — see failures above"
  echo "############################################################"
  exit 1
fi
