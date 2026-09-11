#!/usr/bin/env bash
# Re-run pulsar_det_an with the parameters recorded in a baseline directory
# and compare every output file against it.
#
# Exit status 0 means the change is behaviour-preserving: every output file is
# byte-identical. Any difference is reported and the script exits non-zero.
#
# Usage:
#   scripts/compare_outputs.sh <baseline-dir> [work-dir]

set -uo pipefail

if [ "$#" -lt 1 ]; then
    sed -n '2,10p' "$0" | sed 's/^# \{0,1\}//'
    exit 1
fi

BASELINE_DIR=$(cd "$1" && pwd)
WORK_DIR=${2:-$(mktemp -d)}

REPO_ROOT=$(cd "$(dirname "$0")/.." && pwd)
BIN=$REPO_ROOT/RTL/bin/pulsar_det_an_v4.out

if [ ! -x "$BIN" ]; then
    echo "error: $BIN not built. Run 'make all' in RTL/ first." >&2
    exit 1
fi
if [ ! -f "$BASELINE_DIR/RUN_PARAMS.txt" ]; then
    echo "error: $BASELINE_DIR has no RUN_PARAMS.txt - not a baseline directory" >&2
    exit 1
fi

DATA_FILE=$(grep '^# data' "$BASELINE_DIR/RUN_PARAMS.txt" | sed 's/^# data *//')
PARAMS=$(grep -v '^#' "$BASELINE_DIR/RUN_PARAMS.txt" | grep -v '^exit status' | head -1)

if [ ! -f "$DATA_FILE" ]; then
    echo "error: baseline data file '$DATA_FILE' no longer exists" >&2
    exit 1
fi

mkdir -p "$WORK_DIR"
WORK_DIR=$(cd "$WORK_DIR" && pwd)
rm -f "$WORK_DIR"/*.txt "$WORK_DIR"/*.bin "$WORK_DIR"/*.log

for blank in Blankf.txt Blanks.txt; do
    cp "$BASELINE_DIR/$blank" "$WORK_DIR/" 2>/dev/null || : > "$WORK_DIR/$blank"
done

echo "Re-running with: $PARAMS"
# shellcheck disable=SC2086
( cd "$WORK_DIR" && "$BIN" "$DATA_FILE" $PARAMS > stdout.log 2> stderr.log )
new_status=$?

if [ -f "$WORK_DIR/cutdat.bin" ]; then
    sha256sum "$WORK_DIR/cutdat.bin" | cut -d' ' -f1 > "$WORK_DIR/cutdat.bin.sha256"
    rm -f "$WORK_DIR/cutdat.bin"
fi

differences=0

# stdout is compared separately and non-fatally: added diagnostics are expected
# and welcome, whereas a changed data file is not.
for f in "$BASELINE_DIR"/*; do
    name=$(basename "$f")
    case "$name" in
        RUN_PARAMS.txt|stdout.log|stderr.log|Blankf.txt|Blanks.txt) continue ;;
    esac
    if [ ! -f "$WORK_DIR/$name" ]; then
        echo "MISSING: $name was produced by the baseline but not by this build"
        differences=$((differences + 1))
    elif ! cmp -s "$f" "$WORK_DIR/$name"; then
        echo "CHANGED: $name"
        differences=$((differences + 1))
    fi
done

for f in "$WORK_DIR"/*; do
    name=$(basename "$f")
    case "$name" in
        RUN_PARAMS.txt|stdout.log|stderr.log|Blankf.txt|Blanks.txt) continue ;;
    esac
    if [ ! -f "$BASELINE_DIR/$name" ]; then
        echo "NEW: $name was not produced by the baseline"
        differences=$((differences + 1))
    fi
done

echo
if ! cmp -s "$BASELINE_DIR/stdout.log" "$WORK_DIR/stdout.log"; then
    echo "note: console output differs (this is fine for added diagnostics):"
    diff "$BASELINE_DIR/stdout.log" "$WORK_DIR/stdout.log" | head -20 | sed 's/^/  /'
    echo
fi

if [ "$differences" -eq 0 ]; then
    echo "PASS - all $(ls -1 "$BASELINE_DIR" | wc -l) output files are byte-identical."
    echo "       work dir: $WORK_DIR"
    exit 0
else
    echo "FAIL - $differences output file(s) differ."
    echo "       baseline: $BASELINE_DIR"
    echo "       new:      $WORK_DIR"
    exit 1
fi
