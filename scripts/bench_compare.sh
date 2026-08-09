#!/bin/bash
# Nexus 6.1 bench comparison — runs the bench suite on two binaries and
# diffs the results. Used to detect search drift between versions.
#
# Usage:
#   bash scripts/bench_compare.sh ./nexus-old ./nexus-new
#
# Output: per-position bestmove + node count for each binary, plus a summary
# of any drift. Exit 0 if node counts match, 1 if they differ.
set -u

if [ $# -lt 2 ]; then
  echo "Usage: $0 <binary_a> <binary_b>"
  echo "Example: $0 ./nexus-6.0 ./nexus-6.1"
  exit 2
fi

A="$1"
B="$2"
DEPTH="${3:-13}"

if [ ! -x "$A" ] || [ ! -x "$B" ]; then
  echo "Error: both arguments must be executable binaries"
  exit 2
fi

echo "############################################################"
echo "#  Nexus Bench Comparison"
echo "#  A: $A"
echo "#  B: $B"
echo "#  Depth: $DEPTH"
echo "############################################################"
echo ""

# Run bench on each, capture full output
out_a=$("$A" bench "$DEPTH" 2>&1)
out_b=$("$B" bench "$DEPTH" 2>&1)

# Extract per-position lines (Bench [#N]: ...)
echo "--- Binary A ($A) ---"
echo "$out_a" | grep '^Bench \[' | head -5
echo "..."
echo "$out_a" | grep '^Bench \[' | tail -3
echo ""

echo "--- Binary B ($B) ---"
echo "$out_b" | grep '^Bench \[' | head -5
echo "..."
echo "$out_b" | grep '^Bench \[' | tail -3
echo ""

# Extract totals
total_a=$(echo "$out_a" | grep 'Results:' | grep -oE '[0-9]+ nodes' | awk '{print $1}')
total_b=$(echo "$out_b" | grep 'Results:' | grep -oE '[0-9]+ nodes' | awk '{print $1}')

echo "############################################################"
echo "#  SUMMARY"
echo "############################################################"
printf "  Binary A total nodes: %s\n" "$total_a"
printf "  Binary B total nodes: %s\n" "$total_b"

if [ "$total_a" = "$total_b" ]; then
  drift=0
  echo "  Drift: 0 nodes (IDENTICAL)"
else
  drift=$((total_b - total_a))
  printf "  Drift: %+d nodes (%.4f%%)\n" "$drift" "$(echo "scale=6; $drift * 100 / $total_a" | bc -l 2>/dev/null || echo 'N/A')"
fi

# Per-position diff
echo ""
echo "############################################################"
echo "#  PER-POSITION DIFF"
echo "############################################################"
diff_count=0
for i in $(seq 1 50); do
  line_a=$(echo "$out_a" | grep "^Bench [#$i]:" )
  line_b=$(echo "$out_b" | grep "^Bench [#$i]:" )
  move_a=$(echo "$line_a" | grep -oE 'bestmove +[a-h][1-8][a-h][1-8]' | awk '{print $2}')
  move_b=$(echo "$line_b" | grep -oE 'bestmove +[a-h][1-8][a-h][1-8]' | awk '{print $2}')
  nodes_a=$(echo "$line_a" | grep -oE '[0-9]+ nodes' | awk '{print $1}')
  nodes_b=$(echo "$line_b" | grep -oE '[0-9]+ nodes' | awk '{print $1}')
  if [ "$move_a" != "$move_b" ] || [ "$nodes_a" != "$nodes_b" ]; then
    diff_count=$((diff_count + 1))
    printf "  [#%2d] A: %s %s nodes | B: %s %s nodes\n" "$i" "$move_a" "$nodes_a" "$move_b" "$nodes_b"
  fi
done

if [ "$diff_count" -eq 0 ]; then
  echo "  (no per-position differences)"
else
  echo ""
  echo "  $diff_count positions differ."
fi

if [ "$total_a" = "$total_b" ] && [ "$diff_count" -eq 0 ]; then
  echo ""
  echo "  RESULT: IDENTICAL"
  exit 0
else
  echo ""
  echo "  RESULT: DRIFT DETECTED"
  exit 1
fi
