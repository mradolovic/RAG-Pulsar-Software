#!/usr/bin/env bash

set -uo pipefail   # NOTE: no -e so script won't stop on program failures

# ==========================================
# Configuration
# ==========================================

RUNS=5

ROOT_DIR="$(pwd)"
RESULTS_DIR="$ROOT_DIR/timing_results"

mkdir -p "$RESULTS_DIR"

ARCHIVE_DIR="$ROOT_DIR/archive"
RTL_DIR="$ROOT_DIR/RTL"

ARCHIVE_EXE="./pulsar_det_an_v4.out"
RTL_EXE="./bin/pulsar_det_an_v4.out"

ARCHIVE_ARGS="../data/data.bin 16 1 714.47415 128 1024 6.5 -26.7 -1.3 6 1 2.4 422 50 0 127"
RTL_ARGS="../../data/data.bin 16 1 714.47415 128 1024 6.5 -26.7 -1.3 6 1 2.4 422 50 0 127"

# ==========================================
# Cleanup (preserves required files)
# ==========================================

cleanup_outputs() {
    find . -maxdepth 1 \
        -type f \
        ! -name "*.c" \
        ! -name "*.cpp" \
        ! -name "*.h" \
        ! -name "*.hpp" \
        ! -name "*.mk" \
        ! -name "Makefile" \
        ! -name "*.o" \
        ! -name "*.out" \
        ! -name "Blankf.txt" \
        ! -name "Blanks.txt" \
        -delete
}

# ==========================================
# Archive benchmark
# ==========================================

benchmark_archive() {

    echo "========================================"
    echo "Benchmarking archive"
    echo "========================================"

    pushd "$ARCHIVE_DIR" > /dev/null

    make clean
    make all

    output_file="$RESULTS_DIR/archive_times.txt"
    error_file="$RESULTS_DIR/archive_errors.txt"

    : > "$output_file"
    : > "$error_file"

    for ((i=1; i<=RUNS; i++)); do

        echo "archive run $i/$RUNS"

        cleanup_outputs

        time_output=$(
            { /usr/bin/time -f "%e" \
                $ARCHIVE_EXE $ARCHIVE_ARGS \
                > /dev/null; } 2>&1
        )

        status=$?

        echo "$time_output" >> "$output_file"

        if [[ $status -ne 0 ]]; then
            echo "run $i failed" >> "$error_file"
        fi

    done

    popd > /dev/null
}

# ==========================================
# RTL benchmark
# ==========================================

benchmark_rtl() {

    echo "========================================"
    echo "Benchmarking RTL"
    echo "========================================"

    pushd "$RTL_DIR" > /dev/null

    make clean
    make all

    output_file="$RESULTS_DIR/RTL_times.txt"
    error_file="$RESULTS_DIR/RTL_errors.txt"

    : > "$output_file"
    : > "$error_file"

    for ((i=1; i<=RUNS; i++)); do

        echo "RTL run $i/$RUNS"

        cleanup_outputs

        time_output=$(
            { /usr/bin/time -f "%e" \
                $RTL_EXE $RTL_ARGS \
                > /dev/null; } 2>&1
        )

        status=$?

        echo "$time_output" >> "$output_file"

        if [[ $status -ne 0 ]]; then
            echo "run $i failed" >> "$error_file"
        fi

    done

    popd > /dev/null
}

# ==========================================
# Run benchmarks
# ==========================================

benchmark_archive
benchmark_rtl

echo "========================================"
echo "All benchmarks completed"
echo "Results:"
echo "  $RESULTS_DIR/archive_times.txt"
echo "  $RESULTS_DIR/RTL_times.txt"
echo "Errors (if any):"
echo "  $RESULTS_DIR/archive_errors.txt"
echo "  $RESULTS_DIR/RTL_errors.txt"
echo "========================================"
