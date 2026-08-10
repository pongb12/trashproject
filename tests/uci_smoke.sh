#!/bin/bash
# Nexus 6.1 UCI smoke test — verifies every required UCI command.
# Returns 0 on all-pass, nonzero on any failure.
set -u
cd "$(dirname "$0")/../src"
NEXUS=./nexus6.1

# Build the binary if missing
if [ ! -x "$NEXUS" ]; then
  echo "Building nexus (default release)..."
  make build CC="${CC:-gcc}" ARCH="${ARCH:-avx2}" >/dev/null 2>&1 || { echo "BUILD FAILED"; exit 2; }
fi

pass=0; fail=0
ok()   { pass=$((pass+1)); printf '  [PASS] %s\n' "$1"; }
bad()  { fail=$((fail+1)); printf '  [FAIL] %s\n' "$1"; }

run() {
  local script="$1"; local settle="${2:-1}"
  ( printf '%b' "$script"; sleep "$settle"; printf 'quit\n' ) | $NEXUS 2>&1
}

echo "=== Test 1: uci + isready + ucinewgame ==="
out=$(run 'uci\nisready\nucinewgame\nisready\n' 1)
echo "$out" | grep -q '^id name Nexus 6.1'    && ok "id name Nexus 6.1"        || bad "id name missing"
echo "$out" | grep -q '^id author Nexus Team' && ok "id author Nexus Team"     || bad "id author missing"
echo "$out" | grep -q '^uciok'                && ok "uciok"                    || bad "uciok missing"
echo "$out" | grep -q '^readyok'              && ok "readyok"                  || bad "readyok missing"
echo "$out" | grep -q 'option name Hash'      && ok "Hash option"              || bad "Hash option missing"
echo "$out" | grep -q 'option name DebugModules'  && ok "DebugModules option"  || bad "DebugModules option missing"

echo "=== Test 2: position startpos + moves ==="
out=$(run 'position startpos moves e2e4 e7e5\ngo depth 6\n' 2)
echo "$out" | grep -q '^bestmove '            && ok "bestmove returned"        || bad "no bestmove"
echo "$out" | grep -E '^info depth 6 .* pv '  && ok "depth 6 pv"               || bad "no depth 6 pv"

echo "=== Test 3: position fen ==="
out=$(run 'position fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -\ngo depth 6\n' 2)
echo "$out" | grep -q '^bestmove '            && ok "bestmove from FEN"        || bad "no bestmove from FEN"

echo "=== Test 4: go movetime ==="
out=$(run 'position startpos\ngo movetime 300\n' 1)
echo "$out" | grep -q '^bestmove '            && ok "go movetime returns"      || bad "go movetime hung"

echo "=== Test 5: go depth ==="
out=$(run 'position startpos\ngo depth 8\n' 3)
echo "$out" | grep -E '^info depth 8 '        && ok "go depth 8 reached"       || bad "go depth 8 not reached"
echo "$out" | grep -q '^bestmove '            && ok "go depth 8 bestmove"      || bad "go depth 8 no bestmove"

echo "=== Test 6: go infinite + stop ==="
out=$(run 'position startpos\ngo infinite\n' 2; printf 'stop\n'; sleep 1)
echo "$out" | grep -E '^info depth 1[0-9]+ '  && ok "infinite searched past d10" || bad "infinite no progress"
echo "$out" | grep -q '^bestmove '            && ok "stop produced bestmove"   || bad "stop no bestmove"

echo "=== Test 7: go nodes ==="
out=$(run 'position startpos\ngo nodes 50000\n' 2)
echo "$out" | grep -q '^bestmove '            && ok "go nodes returns"         || bad "go nodes hung"

echo "=== Test 8: go wtime/btime (blitz) ==="
out=$(run 'position startpos\ngo wtime 5000 btime 5000 winc 100 binc 100\n' 3)
echo "$out" | grep -q '^bestmove '            && ok "wtime/btime returns"      || bad "wtime/btime hung"

echo "=== Test 9: go ponder + stop ==="
out=$(run 'setoption name Ponder value true\nposition startpos\ngo ponder wtime 5000 btime 5000\n' 1; printf 'stop\n'; sleep 1)
echo "$out" | grep -q '^bestmove '            && ok "ponder + stop returns"    || bad "ponder+stop no bestmove"

echo "=== Test 10: go searchmoves ==="
out=$(run 'position startpos\ngo depth 6 searchmoves e2e4 d2d4\n' 3)
echo "$out" | grep -q '^bestmove '            && ok "searchmoves returns"      || bad "searchmoves hung"
bm=$(echo "$out" | grep '^bestmove ' | head -1 | awk '{print $2}')
if [ "$bm" = "e2e4" ] || [ "$bm" = "d2d4" ]; then
  ok "bestmove ($bm) in searchmoves set"
else
  bad "bestmove ($bm) not in {e2e4, d2d4}"
fi

echo "=== Test 11: ucinewgame resets ==="
out=$(run 'ucinewgame\nposition startpos\ngo depth 8\n' 3)
echo "$out" | grep -q '^bestmove '            && ok "ucinewgame + go works"    || bad "ucinewgame broke search"

echo "=== Test 12: quit responsiveness ==="
start=$(date +%s)
out=$(run 'position startpos\ngo infinite\n' 0; printf 'quit\n')
end=$(date +%s); elapsed=$((end - start))
[ "$elapsed" -lt 5 ] && ok "quit killed infinite in ${elapsed}s" || bad "quit took ${elapsed}s — may hang"

echo ""
echo "================================="
echo "  PASS: $pass    FAIL: $fail"
echo "================================="
exit $fail
