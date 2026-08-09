#!/bin/bash
# Nexus 6.1 bench regression test
# Verifies that the bench node count matches the canonical value (2,811,728
# at default depth 13). Any drift indicates a behavioral change in the search.
set -u
cd "$(dirname "$0")/../src"
NEXUS=./nexus6.1

if [ ! -x "$NEXUS" ]; then
  echo "Building nexus..."
  make build CC="${CC:-gcc}" ARCH="${ARCH:-avx2}" >/dev/null 2>&1 || { echo "BUILD FAILED"; exit 2; }
fi

EXPECTED=2811728
DEPTH=${1:-13}

echo "=== Bench regression (depth $DEPTH, expected $EXPECTED nodes) ==="
out=$($NEXUS bench $DEPTH 2>&1)
got=$(echo "$out" | grep 'Results:' | grep -oE '[0-9]+ nodes' | awk '{print $1}')

if [ -z "$got" ]; then
  echo "  [FAIL] bench produced no Results line"
  exit 1
fi

if [ "$got" = "$EXPECTED" ]; then
  echo "  [PASS] bench = $got nodes (matches canonical value)"
  exit 0
else
  echo "  [FAIL] bench = $got nodes (expected $EXPECTED)"
  echo "  Drift = $((got - EXPECTED)) nodes"
  echo ""
  echo "  This indicates a behavioral change in the search. If intentional,"
  echo "  update EXPECTED in this script and document the change in release notes."
  exit 1
fi
