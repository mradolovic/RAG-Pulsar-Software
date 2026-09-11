# Project context and rework handoff

Context for continuing the memory-safety and robustness rework of this repository.
Written at the end of a code review session; everything below is either a decision
that was made, a fact that was verified by running the code, or an open item.

---

## What this project is

A GUI front-end (`gui.py`, by Marko Radolović) plus a Python orchestration layer
(`lib/`, by Martin Ante Rogošić) around Peter East's pulsar detection and analysis
C code (`RTL/src/`). It takes RTL-SDR observations, channelises them, folds them at
a candidate pulsar period, and searches period / p-dot / DM space for a detection.

The analysis binary is `RTL/bin/pulsar_det_an_v4.out`, built by `RTL/Makefile`.
It writes ~32 output text files to its working directory, which `lib/runners.py`
sets to `results/pulsar_det_an_results/`.

## Operating context

- **Observation files are over 10 GB.** This matters more than anything else below.
- **Typical parameters:** 16 channels, 128 sections, 1024 fold bins, 1 ms clock.
  Target is usually B0329+54.
- **There is a desire to run above 32 channels and above 4096 bins.** This has not
  been done yet. It is currently blocked, deliberately — see "Limits" below.
- **Platform:** Linux first, Windows and macOS wanted eventually.
- **Nothing has visibly gone wrong in production.** The memory work done before this
  review was precautionary, not a response to a crash.

## Decisions made

1. **The numerical core may be changed, but changes must be re-validated.** Peter
   East's algorithms are the reference. Anything that alters output needs a
   validation run before it is merged.
2. **Behaviour-preserving and behaviour-changing work must stay in separate commits.**
   This is the central working rule. See below.
3. **Do not raise MAX_CHAN / MAX_BINS yet.** Guards first, limits later. The guards
   currently reject the larger parameters with a clear message rather than
   corrupting memory. Raising them is a later, separate piece of work.
4. **A full scientific validation run is expensive**, so a *behavioural* baseline is
   used instead for day-to-day work. It does not assert the science is correct; it
   asserts that a change did not alter the output.

## The working rule

Every change belongs in one of two categories, and they must not be mixed in one
commit.

**Behaviour-preserving** — must produce byte-identical output files.
Validation guards, bounds checks, allocation and read checks, exit codes, memory
layout changes, build changes, anything in the Python layer. Prove it:

```bash
cd RTL && make clean && make all
scripts/compare_outputs.sh <baseline-dir>      # must print PASS
```

**Behaviour-changing** — alters the numbers in the output files.
One commit each, with the commit message stating what changes and why. These need
a validation run before merging. Currently outstanding: the `ftdat2` half-fill, the
float-to-double accumulators in `psnr()`, the NaN guard in `psnr()`.

Before trying any untested parameter combination:

```bash
cd RTL && make clean && make asan
# run normally; silence means clean
```

## Limits

`RTL/includes/analysis_limits.h` holds the bounds the fixed-size working arrays
impose, with each one documented next to the list of declarations it derives from.
If you change an array dimension, change the matching constant. Current values:

| Constant | Value | Bounded by |
|---|---|---|
| `MAX_CHAN` | 32 | the `[32]` per-channel dimension of `compval`, `rms`, `mean`, ... |
| `MAX_BINS` | 4096 | the `[4096]` fold buffers `dfold`, `sumt`, `outdat`, ... |
| `MAX_PTS` | 131072 | `ddispbands[.][131072]`, the tightest PTS-indexed array |
| `MAX_BLANK` | 256 | `blaf[256]`, `blas[256]` |

Note `outsumt[100][.]` is indexed *both* by channel and by period step (`st < 51`),
so its first dimension must stay `>= max(MAX_CHAN, 51)`.

---

## Work completed

Four commits, all verified byte-identical against a baseline (37 of 37 output files):

1. **`tooling`** — `make asan` target; `scripts/capture_baseline.sh` and
   `scripts/compare_outputs.sh`.
2. **`validate: enforce the limits the fixed-size arrays actually impose`** — the
   previous guards checked the wrong quantities. `N` was rejected only above 100
   though the arrays are `[32]` (N=64 wrote past `compval` into `rm`); `bins` was
   never bounded though the fold buffers are `[4096]`; the product checked was
   `M*N` though nothing is sized by `M*N` — the real quantity is `PTS = M*bins`,
   previously unchecked. Also validates section range, `rolav`, and the divisors.
   All error paths now return `EXIT_FAILURE`; previously every one called `exit(0)`.
3. **`validate: bound and range-check Blankf.txt / Blanks.txt`** — the read loop had
   no bound on its counter (stack overflow past `blaf[256]`), used `!= EOF` so a
   non-numeric token looped forever, and the values were used directly as array
   indices with no range check.
4. **`robustness: check every allocation, read and file open`** — unchecked `malloc`
   of the whole data range, discarded `fread` return (short reads folded
   uninitialised memory into the result), 32 unchecked `fopen` calls, `fclose` on
   possible NULLs, uninitialised `bestprof`.

`pulsar_det_an_v4.c` and `io/files.c` now build warning-free under
`-Wall -Wextra -Wpedantic`, and a full ASan/UBSan run is silent.

---

## Outstanding work, in priority order

### 1. Stream the bulk data load (behaviour-preserving)

`pulsar_det_an_v4.c` reads the entire selected section range into one allocation and
writes a verbatim copy to `cutdat.bin`. With observations over 10 GB this needs
10 GB of contiguous RAM and 10 GB of disk. The section range has effectively been
functioning as a workaround for this ceiling.

The fold loop iterates sequentially over `aux`, so it reads naturally in fixed-size
chunks. Doing so removes the RAM ceiling entirely. Must remain byte-identical —
floating-point accumulation order in the fold must not change.

### 2. GUI: stop blocking the Tk event loop (behaviour-preserving)

`gui.py:run_process` calls `process.communicate()` directly from a button callback,
so the window freezes for the entire run — minutes to hours — with no streamed
output and no way to cancel. Needs a worker thread plus a `queue.Queue` polled from
the Tk thread via `root.after()`. Only the poller may touch widgets.

While there: `run_process` never checks `returncode`, so it prints "Done" even on
failure. That check is only meaningful now that the C side returns `EXIT_FAILURE`.

### 3. `RTLChannel4bin.c` (mixed)

- `ftpts` is unvalidated. `dats[16384]` needs `2*ftpts`, so the real limit is 8192;
  `ftpts=16384` overflows. A non-power-of-two value silently produces garbage —
  `is_pow_of_2()` already exists in `numerics/` and should be used.
- `four(dats - 1, ...)` — GCC reports `array subscript -1 is outside array bounds`.
  This is the old copy of the FFT. **The fixed version is `numerics/four.c`**; there
  are four copies in the tree (`RTLChannel4bin.c`, `RAFFT22Lg.c`, `rapulsar2con.c`,
  and the fixed one). Link the three against `numerics/four.o` and delete their
  local copies.
- `fseeko(fptr, SEEK_SET, SEEK_END)` has its arguments swapped; it works only
  because `SEEK_SET == 0`.
- `getc()` return is unchecked, so EOF becomes a 255 sample in the final block.

### 4. Windows 64-bit correctness (behaviour-preserving on Linux)

`long int` is 32 bits on Windows even in 64-bit builds. `file_end`, `start`, `end`
and `nmax` are all `long int` — 42 uses in `pulsar_det_an_v4.c` — and would overflow
at 2 GB. `fseeko`/`ftello` do not exist in MinGW (`_fseeki64`/`_ftelli64`). Needs
`int64_t` throughout plus a seek shim. Worth doing before more code accumulates.

### 5. Behaviour-changing fixes (each needs a validation run)

- **`spectrum()` half-fills its output.** It writes `ftdat2[0 .. PTS/2-1]` but the
  caller reads `[0 .. PTS-1]`. Since `ftdat2` is a global, channels after the first
  read the *previous* channel's upper half and accumulate it into `spallbands`.
  Verified blast radius: `spect[]` feeds only `spallbands`, which is written only to
  `spallbands.txt`. The SNR and detection path is unaffected.
- **`psnr()` accumulates doubles into floats.** `mn`, `rms`, `mnr`, `rmsr` are
  `float` while `dat[]` is `double`. This is the function that decides whether there
  is a detection.
- **`psnr()` NaN guard is ineffective.** `rms - mn*mn` can go slightly negative for
  near-constant data; `sqrt()` of it is NaN; the guard tests `sqrt(rms) < 1e-8` on
  the already-square-rooted value, and NaN comparisons are false, so it takes the
  wrong branch. Guard the variance *before* the square root.
- **`conv()` low-pass filter is dead code.** `low = (int)(0 * 2 * M * period / 1000)`
  is always zero because of the literal `0`. Either intentional or a debugging
  leftover; needs a decision.

### 6. Housekeeping

- `make clean` is `rm -f bin/*`, which deletes `bin/Blanks.txt` and `bin/Blankf.txt`
  and silently destroys hand-tuned blanking configuration. Restrict it to `*.o` and
  the binaries, or move those files out of `bin/`.
- `all:` does not use an order-only prerequisite for `$(BIN_DIR)`, so `make -j` can
  race.
- `-march=native` means `RTL/bin/*` cannot be copied between machines.
- `lib/runners.py:229` uses `match`/`case` (Python 3.10+), undoing the deliberate
  3.9-compatibility fix made to `io.py` in commit `ce9fa6f`.
- `RAG_Pulsar_Software.py` discards `parse_args()` and calls `compile_everything()`
  unconditionally, so `--compile` is dead and `make` runs on every button press.
  `compile_everything()` also ignores whether `make` succeeded.
- `includes/common/commandLineArguments.h` is not valid C (no struct tag, no closing
  semicolon). It is not in the Makefile so nothing catches it. Finish or delete.
- `gui.py` and `lib/runners.py` contain two divergent copies of the TopoBary script,
  both built by f-string interpolation of GUI text into `python -c`, and both with
  **B0329+54's RA/Dec hardcoded** (`ra=3.54972*u.hr, dec=54.5786*u.deg`) while
  period and DM are user-settable. For any other pulsar the Doppler correction is
  silently wrong. Extract to a real function in `lib/`, add RA/Dec fields.
- `gui.py` binds `<MouseWheel>`, which does not exist on Linux X11 (needs
  `<Button-4>`/`<Button-5>`).
- "Clear All Program Outputs" `rmtree`s results with no confirmation, and sits beside
  "Run Full Pipeline" at the same size.

---

## Gotchas

- **`pulsar_det_an_v4.c` and `RTLChannel4bin.c` use CRLF line endings**; the rest of
  the tree uses LF. Edits must preserve them or the whole file lands in the diff.
  There is no `.gitattributes`.
- **`.bss` is 646 MB** — the fixed global arrays are allocated whether used or not.
  At the normal 16 channels / 128 sections / 1024 bins, only ~179 MB is actually
  needed. When the limits are eventually raised, dynamic allocation makes the common
  case cheaper *and* the large case possible; scaling the fixed arrays to N=64 would
  exceed 2 GB of BSS, which is not viable.
- **Every error path used to `exit(0)`**, which is why `lib/runners.py`'s
  `returncode != 0` check never fired and the chain continued to `pul_plot`, which
  replotted stale files from the previous run. Fixed in the C, still needs the
  matching check in `gui.py`.
- `pul_plot` reads whatever is in `results/pulsar_det_an_results/`, so a failed run
  followed by a plot looks like a successful run.
