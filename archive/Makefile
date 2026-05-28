

all:
	gcc pulsar_det_an_v4.c -o pulsar_det_an_v4.out  -O3  -g3 -D_FILE_OFFSET_BITS=64 -Wall -Wpedantic -Wextra   -fno-omit-frame-pointer -fopt-info-vec-missed=vec.missed -march=native -fno-inline-functions-called-once -lm
	touch Blankf.txt Blanks.txt

clean:
	rm -f pulsar_det_an_v4.out
	rm -f *.txt
