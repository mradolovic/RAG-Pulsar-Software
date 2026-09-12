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

- **The raw captures are tens of GB.** `data/raw data/` holds 29-59 GB files. These
  are the input to **`RTLChannel4bin`**, not to `pulsar_det_an` - see the size table
  below, which is the single most important thing to get right before optimising
  anything for file size.
- **Typical parameters:** 16 channels, 128 sections, 1024 fold bins, 1 ms clock.
  Target is usually B0329+54.
- **The two stages see very different volumes.** `RTLChannel4bin` reads raw 8-bit
  I/Q and writes 4-byte floats; the size ratio is `2/dsr` where
  `dsr = clock / downsample / ftpts`. At the observatory's settings `dsr = 128`, so
  the channelised file is **1/64 the size of the raw capture**:

  | stage | reads | 240 min of observation |
  |---|---|---|
  | `RTLChannel4bin` | raw 8-bit I/Q | **~59 GB** |
  | `pulsar_det_an` | channelised floats | **~920 MB** |

  Verified: `20260820_2357Z_240min.bin` is 58,979,778,560 B and its channelised
  output is 921,559,040 B, exactly `floor(raw/4096) * 16 * 4`.

- **The parameters actually in use**, recovered from `results/pulsar_det_an_results/header.txt`
  and from the raw/channelised size ratio. Keep these; they are what the baselines
  below were captured with.

  ```
  RTLChannel4bin:  <raw.bin> <out.bin> 2.048 1 16
  pulsar_det_an:   <chan.bin> 16 1 714.463240 128 1024 6.5 26.7 -1.3 6 1 2 409 50 0 127
  ```

  `pulsar_det_an` applies the -1.3 ppm adjustment before writing its header, so
  `header.txt` reports a period 0.000929 ms lower than the one passed in. That is
  expected, not drift.
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
scripts/compare_outputs.sh <baseline-dir>      # pulsar_det_an - must print PASS
scripts/compare_rtlchan.sh  <baseline-dir>     # RTLChannel4bin - must print PASS
```

**Baselines already captured** (outside the repo, so `make clean` and a `results/`
wipe cannot reach them):

| baseline | program | input |
|---|---|---|
| `~/pulsar_baselines/item1_streaming` | `pulsar_det_an` | `data/20260822_0250Z_240min_channelised.bin`, full range |
| `~/pulsar_baselines/rtlchan/base` | `RTLChannel4bin` | 400 MiB slice of `data/raw data/20260819_0217_120min.bin` |

The `RTLChannel4bin` slice is deliberately **not** a whole number of 4096-byte
blocks, so the partial tail is exercised. A prefix of a raw capture is itself a
valid raw capture, which is what makes slicing safe.

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

### Second session - `RTLChannel4bin` (outstanding item 3)

Four more commits. The first three are behaviour-preserving and verified
byte-identical; the fourth is behaviour-changing and **still needs a validation
run before it is merged**.

5. **`tooling: add an output baseline harness for RTLChannel4bin`** - the existing
   pair hardcodes `pulsar_det_an_v4.out`, so there was no way to prove a change to
   the channelising stage. `compare_rtlchan.sh` reports how far the float samples
   moved, not just that bytes changed.
6. **`validate: bound and range-check RTLChannel4bin's parameters`** - `ftpts` was
   unchecked; `16384` overran `dats[]` and non-powers-of-two were accepted and
   silently produced garbage. Now bounded by `MAX_FTPTS` (8192) using the existing
   `is_pow_of_2()`. Also the swapped `fseeko` arguments, the unchecked reopen and
   output `fopen`, `exit(0)` on every failure path, and an unused variable.
7. **`refactor: link RTLChannel4bin against the fixed four() in numerics/`** - nine
   of that file's ten warnings came from `four(dats - 1, ...)`. Verified beyond the
   standard comparison: the pre-change binary was rebuilt and compared across
   `ftpts` = 4, 16, 64, 256, 1024, 2048 (2 through 11 butterfly stages), all
   byte-identical. `SWAP` and `PI` went with the local copy.
8. **`fix: stop RTLChannel4bin fabricating samples past end of input`** -
   **BEHAVIOUR-CHANGING, awaiting validation.** See item 5 below for exactly what
   changes.

`RTLChannel4bin.c` now builds warning-free and runs silent under ASan/UBSan.
The tree is down from 20 warnings to 10, all in `RAFFT22Lg.c` and `rapulsar2con.c`.

---

## Outstanding work, in priority order

### 1. Stream the bulk data load (behaviour-preserving) - RE-SCOPED, LOWER PRIORITY

**The original justification for this item was wrong and is corrected here.** It read:
"With observations over 10 GB this needs 10 GB of contiguous RAM and 10 GB of disk."
That is not what happens. `pulsar_det_an` never sees a 10 GB file - it reads the
*channelised* output, which at 16 channels / 1 ms is ~920 MB for a 240-minute
observation. The 10 GB+ files are the *raw* captures, and they go to
`RTLChannel4bin`, which already streams via `getc` and has no RAM ceiling. For
`pulsar_det_an`'s buffer to reach 10 GB you would need a 43-hour channelised
observation.

What remains true: `pulsar_det_an_v4.c` reads the entire selected section range into
one allocation and writes a verbatim copy to `cutdat.bin`. At the observatory's
settings that is a ~920 MB allocation plus a ~920 MB disk write per run, on a disk
that has been sitting at 94% full. Worth doing, but it is a cost reduction, not a
ceiling fix, and it should not be prioritised on the basis of the 10 GB figure.

The fold loop iterates sequentially over `aux` and reads `buffer[aux*N + num]` in
strictly increasing order, so it chunks naturally and accumulation order is
preserved exactly. Must remain byte-identical - floating-point accumulation order
in the fold must not change.

### 2. GUI: stop blocking the Tk event loop (behaviour-preserving)

`gui.py:run_process` calls `process.communicate()` directly from a button callback,
so the window freezes for the entire run — minutes to hours — with no streamed
output and no way to cancel. Needs a worker thread plus a `queue.Queue` polled from
the Tk thread via `root.after()`. Only the poller may touch widgets.

While there: `run_process` never checks `returncode`, so it prints "Done" even on
failure. That check is only meaningful now that the C side returns `EXIT_FAILURE`.

### 3. `RTLChannel4bin.c` (mixed) - MOSTLY DONE

Everything originally listed here is done, in commits 6-8 above. What is left:

- **`RAFFT22Lg.c` and `rapulsar2con.c` still carry their own `four()` copies.**
  These are the tree's remaining 10 warnings. Neither is invoked by the Python
  pipeline (`lib/funcs.py` builds them, nothing runs them), and neither has an
  output baseline, so consolidating them needs its own harness and its own
  separately-verified commit. Do not fold them into an unrelated change.
- **The non-whole-`dsr` loop bound is treated, not cured.** Commit 8 stops the
  program rather than letting it fabricate data, but the underlying defect is that
  `for (bn = 0; bn < dsr; bn++)` compares an `int` against a `float` and so runs
  `ceil(dsr)` times, while the outer bound divides by the truncated
  `(long long)(dsr * ftpts * 2)`. Making the block size a single integer used by
  both loops would cure it, and would let non-whole ratios work rather than be
  rejected. That is a behaviour change and needs its own commit and validation.

### 4. Windows 64-bit correctness (behaviour-preserving on Linux)

`long int` is 32 bits on Windows even in 64-bit builds. `file_end`, `start`, `end`
and `nmax` are all `long int` — 42 uses in `pulsar_det_an_v4.c` — and would overflow
at 2 GB. `fseeko`/`ftello` do not exist in MinGW (`_fseeki64`/`_ftelli64`). Needs
`int64_t` throughout plus a seek shim. Worth doing before more code accumulates.

### 5. Behaviour-changing fixes (each needs a validation run)

- **`RTLChannel4bin` no longer fabricates samples past end of input.** ALREADY
  COMMITTED (commit 8), awaiting your validation run. What changes, precisely:
  *nothing* when `dsr = clock / downsample / ftpts` is a whole number - measured
  with an instrumented build, 0 EOF hits over 400 MiB at the observatory's own
  `2.048 1 16`, where `dsr = 128`, and byte-identical at `2.4 1 16` and across
  `ftpts` = 4..2048. When `dsr` is *not* whole, the read overruns the file: 11,633,198
  EOF hits at `2.4 7 16`. Those reads returned `EOF`, which became a 255 sample -
  full scale - and was transformed and written as though it were signal. This is not
  "the final partial block" as previously recorded here; it is most of the tail. Such
  a run now exits `EXIT_FAILURE` with a diagnostic instead of writing a longer file
  padded with invented signal and reporting success. Refusing the run is the
  conservative reading; truncating to the last complete block instead is a one-line
  change.

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
  There is no `.gitattributes`. Two practical consequences:
  - **Applying patches needs `git am --keep-cr`.** Without it `git am` mangles the
    CRLF context lines. The tell is the diffstat: a real change to
    `pulsar_det_an_v4.c` is tens of lines, whereas a mangled apply rewrites all
    ~1195. Check with
    `git diff --numstat origin/gui..HEAD -- RTL/src/pulsar_det_an_v4.c`.
  - **Edit these two files in binary**, not with a line-oriented editor that
    normalises endings. Verify afterwards with
    `[ "$(wc -l < f)" = "$(grep -c $'\r$' f)" ]` - every line must be CRLF.
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

---

## Verification status at the end of the second session

| check | result |
|---|---|
| `cd RTL && make clean && make all` | exit 0, 10 warnings, all in `RAFFT22Lg.c` / `rapulsar2con.c` |
| `scripts/compare_outputs.sh ~/pulsar_baselines/item1_streaming` | PASS, 37/37 byte-identical |
| `scripts/compare_rtlchan.sh ~/pulsar_baselines/rtlchan/base` | PASS, 6,553,600 bytes byte-identical |
| `make asan` + run of `RTLChannel4bin` | silent |
| CRLF in `pulsar_det_an_v4.c` / `RTLChannel4bin.c` | 1195/1195 and 188/188 |

The `pulsar_det_an` baseline run is a real detection (max SNR 6.16, best section
range SNR 42.99), so the comparison is exercising a meaningful code path rather
than a degenerate one.
