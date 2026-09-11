#!/usr/bin/env bash
# Capture a behavioural baseline of pulsar_det_an's output files.
#
# This does NOT assert the science is correct. It records exactly what the
# current binary produces, so that later changes can be proven not to alter
# the output. Run it once on the unmodified code, keep the directory, then
# use compare_outputs.sh after every change.
#
# Usage:
#   scripts/capture_baseline.sh <baseline-dir> <data.bin> [params...]
#
# Example (the parameters from the header comment of pulsar_det_an_v4.c):
#   scripts/capture_baseline.sh baseline/run1 obs.bin \
#       16 1 714.47415 128 1024 6.5 -26.7 -1.3 6 1 2.4 422 50 0 17

set -euo pipefail

if [ "$#" -lt 3 ]; then
    sed -n '2,16p' "$0" | sed 's/^# \{0,1\}//'
    exit 1
fi

BASELINE_DIR=$1; shift
DATA_FILE=$1; shift

REPO_ROOT=$(cd "$(dirname "$0")/.." && pwd)
BIN=$REPO_ROOT/RTL/bin/pulsar_det_an_v4.out

if [ ! -x "$BIN" ]; then
    echo "error: $BIN not built. Run 'make all' in RTL/ first." >&2
    exit 1
fi
if [ ! -f "$DATA_FILE" ]; then
    echo "error: data file '$DATA_FILE' not found" >&2
    exit 1
fi

DATA_FILE=$(cd "$(dirname "$DATA_FILE")" && pwd)/$(basename "$DATA_FILE")

rm -rf "$BASELINE_DIR"
mkdir -p "$BASELINE_DIR"
BASELINE_DIR=$(cd "$BASELINE_DIR" && pwd)

# pulsar_det_an writes every output file relative to the working directory,
# and needs the two attenuation files present there.
for blank in Blankf.txt Blanks.txt; do
    if [ -f "$REPO_ROOT/RTL/bin/$blank" ]; then
        cp "$REPO_ROOT/RTL/bin/$blank" "$BASELINE_DIR/"
    else
        : > "$BASELINE_DIR/$blank"
    fi
done

# Record exactly how this baseline was produced, so it can be reproduced.
{
    echo "# captured $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "# commit  $(cd "$REPO_ROOT" && git rev-parse HEAD 2>/dev/null || echo unknown)"
    echo "# data    $DATA_FILE"
    echo "# sha256  $(sha256sum "$DATA_FILE" | cut -d' ' -f1)"
    echo "$*"
} > "$BASELINE_DIR/RUN_PARAMS.txt"

echo "Running pulsar_det_an in $BASELINE_DIR ..."
( cd "$BASELINE_DIR" && "$BIN" "$DATA_FILE" "$@" > stdout.log 2> stderr.log )
status=$?

echo "exit status: $status" >> "$BASELINE_DIR/RUN_PARAMS.txt"

# cutdat.bin is a verbatim copy of the input range. It is large and its
# content is trivially determined by the input, so hash it instead of keeping it.
if [ -f "$BASELINE_DIR/cutdat.bin" ]; then
    sha256sum "$BASELINE_DIR/cutdat.bin" | cut -d' ' -f1 > "$BASELINE_DIR/cutdat.bin.sha256"
    rm -f "$BASELINE_DIR/cutdat.bin"
fi

echo "Baseline written to $BASELINE_DIR"
ls -1 "$BASELINE_DIR" | sed 's/^/  /'
