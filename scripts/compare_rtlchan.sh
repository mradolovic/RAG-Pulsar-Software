#!/usr/bin/env bash
# Re-run RTLChannel4bin with the parameters recorded in a baseline directory
# and compare its output against it.
#
# Exit status 0 means the change is behaviour-preserving: the channelised
# output is byte-identical. Any difference is reported, with a summary of how
# far the float samples actually moved, and the script exits non-zero.
#
# Usage:
#   scripts/compare_rtlchan.sh <baseline-dir> [work-dir]

set -uo pipefail

if [ "$#" -lt 1 ]; then
    sed -n '2,11p' "$0" | sed 's/^# \{0,1\}//'
    exit 1
fi

BASELINE_DIR=$(cd "$1" && pwd)
WORK_DIR=${2:-$(mktemp -d)}

REPO_ROOT=$(cd "$(dirname "$0")/.." && pwd)
BIN=$REPO_ROOT/RTL/bin/RTLChannel4bin.out

if [ ! -x "$BIN" ]; then
    echo "error: $BIN not built. Run 'make all' in RTL/ first." >&2
    exit 1
fi
if [ ! -f "$BASELINE_DIR/RUN_PARAMS.txt" ]; then
    echo "error: $BASELINE_DIR has no RUN_PARAMS.txt - not a baseline directory" >&2
    exit 1
fi

RAW_FILE=$(grep '^# raw' "$BASELINE_DIR/RUN_PARAMS.txt" | sed 's/^# raw *//')
PARAMS=$(grep -v '^#' "$BASELINE_DIR/RUN_PARAMS.txt" | grep -v '^exit status' | head -1)

if [ ! -f "$RAW_FILE" ]; then
    echo "error: baseline raw input '$RAW_FILE' no longer exists" >&2
    exit 1
fi

mkdir -p "$WORK_DIR"
WORK_DIR=$(cd "$WORK_DIR" && pwd)
rm -f "$WORK_DIR"/out.bin "$WORK_DIR"/*.log

echo "Re-running with: $PARAMS"
# shellcheck disable=SC2086
( cd "$WORK_DIR" && "$BIN" "$RAW_FILE" out.bin $PARAMS > stdout.log 2> stderr.log )

if ! cmp -s "$BASELINE_DIR/stdout.log" "$WORK_DIR/stdout.log"; then
    echo "note: console output differs (fine for added diagnostics):"
    diff "$BASELINE_DIR/stdout.log" "$WORK_DIR/stdout.log" | head -20 | sed 's/^/  /'
    echo
fi

if [ ! -f "$WORK_DIR/out.bin" ]; then
    echo "FAIL - this build produced no output file."
    echo "       baseline: $BASELINE_DIR"
    exit 1
fi

if cmp -s "$BASELINE_DIR/out.bin" "$WORK_DIR/out.bin"; then
    echo "PASS - channelised output is byte-identical ($(stat -c%s "$WORK_DIR/out.bin") bytes)."
    echo "       work dir: $WORK_DIR"
    exit 0
fi

echo "FAIL - channelised output differs."
# The output is a flat array of 4-byte floats, so say how far the numbers moved
# rather than just that some bytes changed.
python3 - "$BASELINE_DIR/out.bin" "$WORK_DIR/out.bin" <<'PY'
import struct, sys
a = open(sys.argv[1], 'rb').read()
b = open(sys.argv[2], 'rb').read()
print(f"  baseline {len(a)} bytes, new {len(b)} bytes")
n = min(len(a), len(b)) // 4
fa = struct.unpack(f'<{n}f', a[:n*4])
fb = struct.unpack(f'<{n}f', b[:n*4])
diff = [(i, x, y) for i, (x, y) in enumerate(zip(fa, fb)) if x != y]
print(f"  {len(diff)} of {n} float samples differ ({100.0*len(diff)/n:.4f}%)")
if diff:
    i, x, y = max(diff, key=lambda t: abs(t[1] - t[2]))
    print(f"  first differing sample: index {diff[0][0]} ({diff[0][1]!r} -> {diff[0][2]!r})")
    print(f"  largest change:         index {i} ({x!r} -> {y!r}, delta {y-x!r})")
    rel = [abs(y-x)/abs(x) for _, x, y in diff if x != 0.0]
    if rel:
        print(f"  max relative change:    {max(rel):.6e}")
PY
echo "       baseline: $BASELINE_DIR"
echo "       new:      $WORK_DIR"
exit 1
