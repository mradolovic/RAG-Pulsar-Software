#!/usr/bin/env bash
# Capture a behavioural baseline of RTLChannel4bin's output.
#
# The companion to capture_baseline.sh, for the channelising stage rather than
# the analysis stage. RTLChannel4bin produces exactly one output file, so the
# baseline is that file plus the console output and enough provenance to
# reproduce the run.
#
# This does NOT assert the channelisation is correct. It records what the
# current binary produces, so a later change can be proven not to alter it.
#
# Usage:
#   scripts/capture_baseline_rtlchan.sh <baseline-dir> <raw-in.bin> \
#       <clock rate (MHz)> <downsample rate (kHz)> <No: fft points>
#
# Example (the parameters derived from the observatory's own captures):
#   scripts/capture_baseline_rtlchan.sh baseline/chan raw.bin 2.048 1 16

set -euo pipefail

if [ "$#" -ne 5 ]; then
    sed -n '2,18p' "$0" | sed 's/^# \{0,1\}//'
    exit 1
fi

BASELINE_DIR=$1; shift
RAW_FILE=$1; shift

REPO_ROOT=$(cd "$(dirname "$0")/.." && pwd)
BIN=$REPO_ROOT/RTL/bin/RTLChannel4bin.out

if [ ! -x "$BIN" ]; then
    echo "error: $BIN not built. Run 'make all' in RTL/ first." >&2
    exit 1
fi
if [ ! -f "$RAW_FILE" ]; then
    echo "error: raw input '$RAW_FILE' not found" >&2
    exit 1
fi

RAW_FILE=$(cd "$(dirname "$RAW_FILE")" && pwd)/$(basename "$RAW_FILE")

rm -rf "$BASELINE_DIR"
mkdir -p "$BASELINE_DIR"
BASELINE_DIR=$(cd "$BASELINE_DIR" && pwd)

# Record exactly how this baseline was produced, so it can be reproduced.
{
    echo "# captured $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "# commit  $(cd "$REPO_ROOT" && git rev-parse HEAD 2>/dev/null || echo unknown)"
    echo "# raw     $RAW_FILE"
    echo "# sha256  $(sha256sum "$RAW_FILE" | cut -d' ' -f1)"
    echo "$*"
} > "$BASELINE_DIR/RUN_PARAMS.txt"

echo "Running RTLChannel4bin into $BASELINE_DIR ..."
set +e
( cd "$BASELINE_DIR" && "$BIN" "$RAW_FILE" out.bin "$@" > stdout.log 2> stderr.log )
status=$?
set -e

echo "exit status: $status" >> "$BASELINE_DIR/RUN_PARAMS.txt"

if [ -f "$BASELINE_DIR/out.bin" ]; then
    sha256sum "$BASELINE_DIR/out.bin" | cut -d' ' -f1 > "$BASELINE_DIR/out.bin.sha256"
fi

echo "Baseline written to $BASELINE_DIR"
ls -1 "$BASELINE_DIR" | sed 's/^/  /'
