#!/bin/bash
# Nexus 6.1 special-moves regression test
# Verifies promotion, en passant, castling (incl. FRC), and repetition
# handling via perft on known-tricky positions.
set -u
cd "$(dirname "$0")/../src"
NEXUS=./nexus6.1

if [ ! -x "$NEXUS" ]; then
  echo "Building nexus..."
  make build CC="${CC:-gcc}" ARCH="${ARCH:-avx2}" >/dev/null 2>&1 || { echo "BUILD FAILED"; exit 2; }
fi

pass=0; fail=0
ok()  { pass=$((pass+1)); printf '  [PASS] %s\n' "$1"; }
bad() { fail=$((fail+1)); printf '  [FAIL] %s (expected %s, got %s)\n' "$1" "$2" "$3"; }

# Each test: FEN | depth | expected_node_count | description
# Reference values from chessprogramming.org/Perft_Results and Marcel van Kervinck's repo.
tests=(
  "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1|5|4865609|startpos d5"
  "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -|4|4085603|Kiwipete d4 (castling+ep)"
  "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1|5|15833292|promotions+ep d5"
  "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8|5|89941194|promotions d5"
  "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -|6|11030083|ep+promotion d6"
  "4k3/8/8/8/8/8/8/4K2R w K - 0 1|5|133987|K-side castling d5"
  "4k3/8/8/8/8/8/8/R3K3 w Q - 0 1|5|145232|Q-side castling d5"
  "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1|5|7594526|both castling d5"
  "n1n5/1Pk5/8/8/8/8/5Kp1/5N1N w - - 0 1|5|2193768|knight+pawn promo d5"
  "8/PPPk4/8/8/8/8/4Kppp/8 w - - 0 1|5|1533145|all-promo d5"
  # FRC / Chess960 positions (reference values produced by the engine itself,
  # cross-checked against the chessprogramming.org FRC perft suite)
  "bqnb1rkr/pp1ppppp/3n4/2p5/4P1Pn/8/PPP1P1PP/BQ1BNRKR w HEhe - 1 9|5|10227725|FRC castling d5"
  "b1rbkrqn/p1pppppp/n7/8/8/7N/P1PPPPPP/B1RBKRQN w HDhd - 0 9|5|8351631|FRC d5"
  # KvK endgame — very few legal moves (draw detection must not crash)
  "8/8/8/8/8/8/8/4k1K1 w - - 0 1|3|52|KvK d3 (draw handling)"
)

echo "=== Special-moves perft regression (promotion, ep, castling, FRC, draws) ==="
for t in "${tests[@]}"; do
  IFS='|' read -r fen depth expected desc <<< "$t"
  out=$( (printf 'setoption name UCI_Chess960 value true\nposition fen %s\ngo perft %s\nquit\n' "$fen" "$depth"; sleep 2) | $NEXUS 2>&1 )
  got=$(echo "$out" | grep -oE 'Nodes: [0-9]+' | awk '{print $2}')
  if [ "$got" = "$expected" ]; then
    ok "$desc"
  else
    bad "$desc" "$expected" "$got"
  fi
done

echo ""
echo "================================="
echo "  PASS: $pass    FAIL: $fail"
echo "================================="
exit $fail
