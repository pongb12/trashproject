#!/bin/bash
# Nexus 6.1 move-legality regression test
# Searches several tactical positions and verifies that every returned
# bestmove is legal in its position (i.e., the engine never suggests an
# illegal move, even under time pressure).
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

# FEN | description
# Mix of: startpos, tactics, endgames, in-check positions, stalemate-adjacent
positions=(
  "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1|startpos"
  "r1bqkbnr/pppp1ppp/2n5/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 0 1|Ruy Lopez"
  "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -|Kiwipete"
  "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -|endgame ep"
  "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8|promotion attack"
  "4k3/8/8/8/8/8/8/4K2R w K - 0 1|K-side castling"
  "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1|both castling"
  "2r3k1/p1Q3pp/4p3/2p5/2P5/2N5/PP3PPP/6K1 b - -|black to move, tactical"
  "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10|quiet middlegame"
  "8/8/4k3/8/8/4K3/8/8 w - - 0 1|KvK endgame"
  "r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 0 1|Italian"
  "r1bqk2r/pppp1ppp/2n2n2/2b1p3/2B1P3/3P1N2/PPP2PPP/RNBQK2R w KQkq - 0 1|French-type"
)

echo "=== Move-legality regression (12 positions) ==="
for entry in "${positions[@]}"; do
  IFS='|' read -r fen desc <<< "$entry"
  out=$( (printf 'position fen %s\ngo movetime 300\nquit\n' "$fen"; sleep 1) | $NEXUS 2>&1 )
  bm=$(echo "$out" | grep '^bestmove ' | head -1 | awk '{print $2}')
  if [ -z "$bm" ]; then
    bad "$desc: no bestmove returned"
    continue
  fi
  # Verify the move is legal by applying it via the engine's own apply command
  # and checking the engine doesn't reject it.
  check=$( (printf 'position fen %s\napply %s\nquit\n' "$fen" "$bm"; sleep 1) | $NEXUS 2>&1 )
  if echo "$check" | grep -q "Invalid move"; then
    bad "$desc: bestmove $bm is illegal!"
  else
    ok "$desc: bestmove $bm is legal"
  fi
done

echo ""
echo "================================="
echo "  PASS: $pass    FAIL: $fail"
echo "================================="
exit $fail
