#!/usr/bin/env bash
set -e

make clean
make all

cd bin

 perf record \
  -F 4000\
  --call-graph fp \
  -d \
  --sample-cpu \
  --sample-identifier \
  --timestamp \
  --switch-events \
  -- \
  ./pulsar_det_an_v4.out \
    ../../data/data.bin \
    16 1 714.47415 128 1024 \
    6.5 -26.7 -1.3 \
    6 1 2.4 422 50 0 127

