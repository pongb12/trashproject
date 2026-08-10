#!/bin/bash
# Nexus 6.1 timed-search regression test
# Verifies time management: movetime, wtime/btime, infinite+stop, ponder+stop,
# nodes limit, and that the engine never hangs or burns absurd time.
#
# Time bounds are measured against the ENGINE-REPORTED time (the `time N` field
# in info lines), not wall-clock, because the test harness adds sleep overhead.
set -u
cd "$(dirname "$0")/../src"
NEXUS=./nexus6.1

if [ ! -x "$NEXUS" ]; then
  echo "Building nexus..."
  make build CC="${CC:-gcc}" ARCH="${ARCH:-avx2}" >/dev/null 2>&1 || { echo "BUILD FAILED"; exit 2; }
fi

pass=0; fail=0
ok()  { pass=$((pass+1)); printf '  [PASS] %s\n' "$1"; }
bad() { fail=$((fail+1)); printf '  [FAIL] %s\n' "$1"; }

# Returns the engine-reported time of the LAST info line (ms), or -1 if none.
last_engine_time() {
  echo "$1" | grep -oE 'time [0-9]+' | tail -1 | awk '{print $2}'
}

run() {
  local script="$1"; local settle="${2:-1}"
  ( printf '%b' "$script"; sleep "$settle"; printf 'quit\n' ) | $NEXUS 2>&1
}

echo "=== Test 1: go movetime 200 ==="
out=$(run 'position startpos\ngo movetime 200\n' 1)
et=$(last_engine_time "$out")
echo "$out" | grep -q '^bestmove ' && ok "movetime 200 returned (engine time ${et}ms)" || bad "movetime 200 hung"
[ "$et" -ge 0 ] && [ "$et" -le 600 ] && ok "  engine time in bounds" || bad "  engine time out of bounds (${et}ms)"

echo "=== Test 2: go movetime 1000 ==="
out=$(run 'position startpos\ngo movetime 1000\n' 2)
et=$(last_engine_time "$out")
echo "$out" | grep -q '^bestmove ' && ok "movetime 1000 returned (engine time ${et}ms)" || bad "movetime 1000 hung"
[ "$et" -ge 500 ] && [ "$et" -le 1500 ] && ok "  engine time in bounds" || bad "  engine time out of bounds (${et}ms)"

echo "=== Test 3: go wtime 1000 btime 1000 (blitz) ==="
out=$(run 'position startpos\ngo wtime 1000 btime 1000 winc 0 binc 0\n' 2)
et=$(last_engine_time "$out")
echo "$out" | grep -q '^bestmove ' && ok "blitz returned (engine time ${et}ms)" || bad "blitz hung"
[ "$et" -ge 0 ] && [ "$et" -le 800 ] && ok "  respected tight clock" || bad "  burned too much time (${et}ms)"

echo "=== Test 4: go wtime 60000 btime 60000 (rapid) ==="
out=$(run 'position startpos\ngo wtime 60000 btime 60000 winc 1000 binc 1000\n' 4)
et=$(last_engine_time "$out")
echo "$out" | grep -q '^bestmove ' && ok "rapid returned (engine time ${et}ms)" || bad "rapid hung"
[ "$et" -ge 0 ] && [ "$et" -le 15000 ] && ok "  respected clock" || bad "  burned too much time (${et}ms)"

echo "=== Test 5: go infinite + stop after 1s ==="
out=$(run 'position startpos\ngo infinite\n' 1; printf 'stop\n'; sleep 1)
et=$(last_engine_time "$out")
echo "$out" | grep -q '^bestmove ' && ok "infinite+stop returned (engine time ${et}ms)" || bad "infinite+stop no bestmove"
echo "$out" | grep -E '^info depth 1[0-9]+' >/dev/null && ok "  searched to reasonable depth" || bad "  shallow search"

echo "=== Test 6: go ponder + stop (no ponderhit) ==="
out=$(run 'setoption name Ponder value true\nposition startpos\ngo ponder wtime 5000 btime 5000\n' 1; printf 'stop\n'; sleep 1)
echo "$out" | grep -q '^bestmove ' && ok "ponder+stop returned" || bad "ponder+stop no bestmove"

echo "=== Test 7: go ponder + ponderhit ==="
out=$(run 'setoption name Ponder value true\nposition startpos\ngo ponder wtime 5000 btime 5000\n' 1; printf 'ponderhit\n'; sleep 1)
echo "$out" | grep -q '^bestmove ' && ok "ponderhit returned" || bad "ponderhit no bestmove"

echo "=== Test 8: go nodes 10000 ==="
out=$(run 'position startpos\ngo nodes 10000\n' 2)
echo "$out" | grep -q '^bestmove ' && ok "go nodes returned" || bad "go nodes hung"

echo "=== Test 9: very small movetime (50ms) — must not hang ==="
out=$(run 'position startpos\ngo movetime 50\n' 1)
et=$(last_engine_time "$out")
echo "$out" | grep -q '^bestmove ' && ok "movetime 50 returned (engine time ${et}ms)" || bad "movetime 50 hung"
[ "$et" -ge 0 ] && [ "$et" -le 400 ] && ok "  no hang on tiny budget" || bad "  hung (${et}ms)"

echo "=== Test 10: many quick moves in sequence (no state leak) ==="
script='uci\nucinewgame\n'
for i in $(seq 1 5); do
  script+="position startpos\ngo movetime 100\n"
done
out=$(run "$script" 2)
bm_count=$(echo "$out" | grep -c '^bestmove ')
[ "$bm_count" -eq 5 ] && ok "5 sequential moves returned" || bad "expected 5 bestmoves, got $bm_count"

echo ""
echo "================================="
echo "  PASS: $pass    FAIL: $fail"
echo "================================="
exit $fail
