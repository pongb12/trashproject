#!/bin/bash
# Nexus 6.1 full regression suite — runs every test in tests/.
# Returns 0 only if every test passes.
set -u
cd "$(dirname "$0")/.."

total_pass=0
total_fail=0
failed_tests=()

run_test() {
  local name="$1"
  local script="$2"
  echo ""
  echo "############################################################"
  echo "# Running: $name"
  echo "############################################################"
  if bash "$script"; then
    total_pass=$((total_pass + 1))
  else
    total_fail=$((total_fail + 1))
    failed_tests+=("$name")
  fi
}

# Ensure binary is built once
if [ ! -x src/nexus6.1 ]; then
  echo "Building nexus (release)..."
  (cd src && make build CC="${CC:-gcc}" ARCH="${ARCH:-avx2}") || { echo "BUILD FAILED"; exit 2; }
fi

run_test "perft (move-gen correctness)"        tests/perft.sh
run_test "mate-in-1 (tactical correctness)"    tests/mate-in-1.sh
run_test "special-moves (ep/castle/promo/FRC)" tests/special_moves.sh
run_test "move-legality (12 positions)"        tests/legality.sh
run_test "UCI smoke (12 commands)"             tests/uci_smoke.sh
run_test "timed search (10 time-mgmt tests)"   tests/timed.sh
run_test "bench regression (node count)"       tests/bench_regression.sh

echo ""
echo "############################################################"
echo "#  REGRESSION SUITE SUMMARY"
echo "############################################################"
echo "  Tests passed: $total_pass"
echo "  Tests failed: $total_fail"
if [ "$total_fail" -gt 0 ]; then
  echo ""
  echo "  FAILED:"
  for t in "${failed_tests[@]}"; do
    echo "    - $t"
  done
  exit 1
fi
echo ""
echo "  ALL TESTS PASSED"
exit 0
