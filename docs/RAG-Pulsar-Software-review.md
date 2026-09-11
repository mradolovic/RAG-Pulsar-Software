# Code review — RAG-Pulsar-Software (`gui` branch, commit `bd778a0`)

Reviewed: `RTL/src/pulsar_det_an_v4.c`, `RTL/src/numerics/*`, `RTL/src/io/files.c`,
`RTL/src/RTLChannel4bin.c`, `RTL/Makefile`, `gui.py`, `lib/runners.py`, `lib/funcs.py`,
`lib/RAG_Pulsar_Software.py`.

## How I tested

Everything below marked **[reproduced]** was confirmed by running an instrumented build,
not by reading alone.

```bash
# instrumented build
gcc -O1 -g -D_FILE_OFFSET_BITS=64 -fsanitize=address,undefined -fno-omit-frame-pointer \
    src/pulsar_det_an_v4.c src/numerics/*.c src/io/files.c -o pda -lm

# synthetic 2.9 MB float32 file, 32 sections x 256 bins, 16 channels, ratio ~4
./pda test.bin 16 2 714.49 32 256 6.5 26.7 0.0 6 4 10 611 8 0 31
```

A clean, well-formed run is **completely silent under ASan + UBSan + LeakSanitizer**. No
overflows, no leaks, no undefined behaviour. That is a real result and it means the
refactor did its job on the happy path. Everything that follows is about what happens
when a parameter, or a `Blankf.txt` entry, leaves the range the arrays were sized for.

---

## The core problem: the validation checks the wrong quantities

`pulsar_det_an_v4.c:38-47` declares every working array with a hardcoded dimension:

```c
double comprs[262144][32], ave[4096][32], rms[32][4096], mean[32], std[32], ...;
double compval[32][262144], compvald[262144][32], ...;
double ddispbands[256][131072], dfold[4096], ddfold[256][4096], outdat[4096];
double sumt[4096], count[4096], outsumt[100][4096], pkdat[4096];
```

So the real constraints are `N <= 32`, `bins <= 4096`, and `M*bins <= 131072`.

But the guards at lines 101-113 check:

```c
if (N > 100) { ... exit(0); }                                  // wrong bound
if (is_pow_of_2(M) != 1 || is_pow_of_2(bins) != 1
    || (M * N) > 262144) { ... exit(0); }                      // wrong product
```

`N > 100` permits N up to 100 against `[32]` arrays. `M * N` is checked, but nothing in
the program is sized by `M*N` — the quantity that matters is `PTS = M * bins`, computed
on line 98 and never validated. And `bins` is only checked for being a power of two, not
for magnitude.

Three distinct overflows follow from that single mismatch.

### C1. `N > 32` smashes the globals — **[reproduced]**

The banner says "No: Bands < 101", so a user is invited to try 64 channels. The GUI
defaults to 16, so this is latent until someone widens the band.

```
./pda test64.bin 64 2 714.49 32 256 6.5 26.7 0.0 6 4 10 611 8 0 31

pulsar_det_an_v4.c:356: runtime error: index 32 out of bounds for type 'double [32]'
pulsar_det_an_v4.c:355: runtime error: index 32 out of bounds for type 'double [32][262144]'
ERROR: AddressSanitizer: global-buffer-overflow, WRITE of size 8 at main+355
  0 bytes after global variable 'compval' (size 67108864)
  32 bytes before global variable 'rm'
```

It writes straight off the end of `compval` into `rm`. Every `[num]` index in the file is
affected: `compval`, `compvaldd`, `spect`, `compvald[c][num+N/2]`, `ave[Mc][num]`,
`rms[num][Mc]`, `av`, `rm`, `mean[num]`, `std[num]`, `bndcum[num]`. Without ASan this is
silent corruption of one array by another — the SNR numbers would just be wrong, which
is the worst failure mode for science code.

### C2. `bins >= 8192` overflows the fold buffers — **[reproduced]**

`bins = 8192` is a power of two and passes `M*N <= 262144` easily.

```
./pda test.bin 16 2 714.49 32 8192 6.5 26.7 0.0 6 4 10 611 8 0 31
*** buffer overflow detected ***: terminated    (exit 134)
```

glibc's `_FORTIFY_SOURCE` catches it in the `memset(dfold, 0, bins*sizeof(double))` before
ASan gets a chance. `dfold`, `sumt`, `count`, `outdat`, `pkdat`, `bstdmprf`, `bestprof`,
`ddfold[][4096]`, `outsumt[][4096]` and `bndcum[][4096]` are all `4096`.

### C3. `PTS = M*bins` is unbounded

Not separately reproduced, but by inspection: `compval[32][262144]` is indexed `[0, PTS)`,
`ddispbands[256][131072]` is indexed `[0, PTS)` at line 636, and `targgaus()` writes
`targ[2*v+1]` for `v < PTS`, i.e. it needs `2*PTS` doubles from a `targ[1048576]`. So the
true ceilings are `PTS <= 131072` for the dispersion search and `PTS <= 524288` for the
target. `M = 1024, bins = 1024` gives `PTS = 1048576` and passes every existing check.

### Suggested fix

Replace the whole guard block with named limits and validate what the arrays actually
constrain:

```c
#define MAX_CHAN  32       /* second dim of comprs / first dim of compval */
#define MAX_BINS  4096     /* dfold, sumt, count, outdat, ddfold, ... */
#define MAX_PTS   131072   /* second dim of ddispbands */

if (N < 1 || N > MAX_CHAN) {
    fprintf(stderr, "Number of FFT channels must be 1..%d (got %d)\n", MAX_CHAN, N);
    return EXIT_FAILURE;
}
if (!is_pow_of_2(M) || !is_pow_of_2(bins) || bins > MAX_BINS) {
    fprintf(stderr, "Sections and bins must be powers of 2, bins <= %d\n", MAX_BINS);
    return EXIT_FAILURE;
}
if ((long)M * bins > MAX_PTS) {
    fprintf(stderr, "sections * bins must be <= %d (got %ld)\n", MAX_PTS, (long)M * bins);
    return EXIT_FAILURE;
}
if (strt < 0 || stp < strt || stp > M) { ... }
if (pulw <= 0.0f || clck <= 0.0 || period <= 0.0 || f0 <= 0.0 || nno1 == 0.0f) { ... }
```

Best of all: replace the `[32]`/`[4096]`/`[262144]` literals with `MAX_CHAN`/`MAX_BINS`/
`MAX_PTS` so the declarations and the guard can't drift apart again. If you later want
larger sizes, switch the big arrays to `calloc(N * PTS, sizeof(double))` with an index
macro — you already do this successfully for `comprc`, `ftdat`, `pdat` and `fftdat`.

---

## C4. `Blankf.txt` / `Blanks.txt` are unvalidated input — **[reproduced]**

Two separate bugs, both driven by a data file rather than the command line.

**Unbounded read into a stack array** (lines 116-141):

```c
int blaf[256];
while (fscanf(files[FPT_BLNKF], "%d", &mf) != EOF) {
    blaf[couf] = mf;     /* couf is never bounded */
    couf += 1;
}
```

With a 500-entry `Blankf.txt`:

```
pulsar_det_an_v4.c:120: runtime error: index 256 out of bounds for type 'int [256]'
ERROR: AddressSanitizer: stack-buffer-overflow
  [480, 1504) 'blaf' (line 117) <== Memory access at offset 1504 overflows this variable
```

Same pattern for `blas` at line 133.

**The values are used as array indices without any range check** (line 489):

```c
compval[blaf[d]][c] = (compval[blaf[d]][c]) / 100;
```

`Blankf.txt` containing `9999`:

```
pulsar_det_an_v4.c:489: runtime error: index 9999 out of bounds for type 'double [32][262144]'
ERROR: AddressSanitizer: SEGV on unknown address 0x55bacaa7c7a0
```

This one matters practically, not just theoretically: `Blankf.txt` is exactly the file an
observer edits by hand at 2am to blank an RFI-contaminated channel. Typing `16` when
`N = 16` (valid channels are 0-15) is an off-by-one away from a segfault, and typing `61`
instead of `6` corrupts memory silently.

Fix both loops:

```c
while (couf < (int)(sizeof blaf / sizeof blaf[0])
       && fscanf(files[FPT_BLNKF], "%d", &mf) == 1) {
    if (mf < 0 || mf >= N) {
        fprintf(stderr, "Blankf.txt: channel %d out of range 0..%d\n", mf, N - 1);
        return EXIT_FAILURE;
    }
    blaf[couf++] = mf;
}
```

Note also `!= EOF` should be `== 1`: a non-numeric token makes `fscanf` return 0 forever
without consuming it, so a stray letter in the file is an infinite loop that fills `blaf`
with stale values. Same for `blas`, where the bound must be `0 <= ms < M`.

---

## C5. The whole input file is read into one unchecked allocation

Lines 295-298:

```c
const float *buffer = (float *)malloc(nmax * M * N * sizeof(float));
fread((void *)buffer, sizeof(float), nmax * M * N, fptr);
fwrite(buffer, sizeof(float), nmax * M * N, files[FPT_CUT]);
```

Three problems:

1. **`malloc` is unchecked.** For a long observation this is the entire selected range of
   the `.bin` file in RAM — potentially many gigabytes. On failure you get `NULL` and the
   `fread` segfaults with no message.
2. **The `fread` return value is discarded.** This is the only warning your own Makefile
   flags currently emit (`-Wunused-result`). If the file is shorter than the section range
   implies — which happens whenever the user's `End section` is optimistic — the tail of
   `buffer` is uninitialised heap, and lines 317-319 fold it straight into `comprs`. The
   run completes, prints an SNR, and the number is meaningless. This is the single most
   dangerous silent-corruption path in the program, because it produces a plausible-looking
   result.
3. `nmax * M * N` is `long int * int * int`. It happens to promote to `long` because `nmax`
   is `long int`, so it's safe on LP64 — but it is worth making explicit.

```c
const size_t nsamp = (size_t)nmax * M * N;
float *buffer = malloc(nsamp * sizeof *buffer);
if (!buffer) {
    fprintf(stderr, "Out of memory: need %zu MB for the data buffer\n",
            nsamp * sizeof *buffer / (1u << 20));
    return EXIT_FAILURE;
}
const size_t got = fread(buffer, sizeof *buffer, nsamp, fptr);
if (got != nsamp) {
    fprintf(stderr,
            "Short read: wanted %zu samples, got %zu. "
            "Reduce 'End section' or check the input file.\n", nsamp, got);
    free(buffer);
    return EXIT_FAILURE;
}
```

(The `const float *` on an allocation you later `free()` is also a small wart — it forces
the `(void*)` casts on the next two lines. Drop the `const`.)

---

## C6. `spectrum()` fills half the buffer; the caller reads all of it

`spectrum.c`:

```c
for (v = 0; v < PTS / 2; v++) {
    ftdat2[v] = pdat[2 * v] / sqrt((double)PTS);
}
```

`pulsar_det_an_v4.c:417-421`:

```c
spectrum(fftdat, PTS, pdat, ftdat2);
for (long int c = 0; c < PTS; c += 1) {
    spect[num][c]   = ftdat2[c];
    spallbands[c]  += (spect[num][c]);
}
```

The upper half of `ftdat2` is never written by this call. `ftdat2` is a global, so on
channel 0 it reads zeros, and from channel 1 onwards it reads the **previous channel's**
upper half. Those stale values are accumulated into `spallbands` and written to
`spallbands.txt`.

This is not a crash and ASan won't flag it — it's a correctness bug that silently
contaminates the combined-band spectrum output. Either have `spectrum()` zero
`ftdat2[PTS/2 .. PTS-1]`, or (better) have the caller only consume `PTS/2` entries, since
the second half of a real-input spectrum is redundant anyway.

---

## C7. `psnr()` — float accumulation and an ineffective NaN guard

`psnr.c`:

```c
float mn = 0, rms = 0, mnr = 0, rmsr = 0, mx = 0, nx = 0, mb = 0;
for (t = 0; t < bins; t++) {
    mn  = mn  + dat[t];              /* double summed into a float */
    rms = rms + (dat[t] * dat[t]);
}
...
rms = rms - mn * mn;
rms = sqrt(rms) + 0.0000001;

if (sqrt(rms) < 0.00000001) {        /* sqrt of an already-sqrt'ed value */
    datout->std_snr = 0.00001;
} else {
    datout->std_snr = (mx - mn) / rms;
}
```

Two issues:

- **Precision.** `dat[]` is `double`; the accumulators are `float` (24-bit mantissa).
  Summing 4096 squared values into a `float` loses real precision in exactly the quantity
  your SNR depends on. `psnr()` is the function that decides whether you have a detection.
  Making these `double` costs nothing and is a strict improvement. The same pattern
  (`(float)` casts on double data) appears throughout `pulsar_det_an_v4.c` — the comment at
  line 351 already flags it. I'd agree with your instinct there: drop the casts.
- **The NaN guard doesn't work.** `rms - mn*mn` is a variance computed as `E[x²] - E[x]²`,
  which goes slightly negative for near-constant data. `sqrt(negative)` is `NaN`. Then
  `sqrt(NaN) < 1e-8` is `false` (all NaN comparisons are false), so it takes the `else`
  branch and `std_snr` becomes `NaN`, which propagates into every downstream comparison
  (`if (datout.std_snr > max)` is also always false against NaN, so `bestprof` never gets
  written — see C8). Guard the variance before the square root:

```c
double var = rms - mn * mn;
if (!(var > 1e-16)) {
    datout->std_snr = 0.00001;
    rms = 1.0;              /* keep the outdat[] normalisation finite */
} else {
    rms = sqrt(var);
    datout->std_snr = (mx - mn) / rms;
}
```

---

## C8. `bestprof[4096]` can be read uninitialised

Lines 961-1021:

```c
double bestprof[4096];                    /* uninitialised stack */
...
if (datout.std_snr > max) {
    max = datout.std_snr;
    if (mp == rolav) memcpy(bestprof, outdat, bins * sizeof(double));
}
...
fprintf(files[FPT_PROF], "%.1f\t%f\t%f\t%f\n", ..., (float)bestprof[d], pkdat[d]);
```

`max` is reset to 0 at the top of each `mp` iteration, so if every SNR at `mp == rolav` is
`<= 0` (all-negative data, or the NaN case from C7), `bestprof` is never written and
`profile.txt` column 3 is stack garbage. Also note `rolav` is user-supplied and never
checked against `1..M` — `rolav = 200` with `M = 128` means the `mp == rolav` branch never
fires at all. Add `double bestprof[MAX_BINS] = {0};` and validate `rolav`.

---

## C9. `conv()` — dead filter, and a `pulw` division by zero

`conv.c`:

```c
low  = (int)(0 * 2 * M * period / 1000);   /* always 0 */
high = (int)(1.1 * M * prat);              /* prat = period / pulw */
```

`low` is multiplied by a literal `0`, so the low-frequency filter loop never executes. If
that's intentional, delete it; if it's a debugging leftover, it means the LF filtering
you think is happening isn't.

`prat = period / pulw` with `pulw = 0` gives `inf`, and `(int)inf` is undefined behaviour
— in practice usually `INT_MIN`. `for (v = high; v < PTS - high; v++)` with a hugely
negative `high` then writes `pdat[2*v]` at massively negative indices. Validate `pulw > 0`
at argument-parse time (the GUI should too — it's a free-text field).

---

## C10. A small regression in the refactored dispersion loop

Lines 587-628. The optimisation of `(c + pre_compute) % PTS` into a split loop is sound
and a genuine improvement, but:

```c
const int pre_compute = double_PTS + (int)((num + 0.5f) * dmp * (e - dmn) * inverse_dmdiv);
const int shift = pre_compute % PTS;
const int split = PTS - shift;
```

The `+ 2*PTS` exists to make the offset non-negative, which assumes `|offset| < 2*PTS`.
The offset can reach `±(N/2) * 50 / dmdiv` = `±250` at `N = 100`. For small `PTS`
(e.g. `M = 2, bins = 2` gives `PTS = 4`, so `2*PTS = 8`), `pre_compute` goes negative,
`shift` is negative (C truncates toward zero for `%`), `split > PTS`, and the first loop
reads `compval[idx][c + shift]` at negative indices.

Degenerate parameters, but it's a new failure mode the original `% PTS` form partly
masked. Cheap fix:

```c
int shift = pre_compute % PTS;
if (shift < 0) shift += PTS;
```

---

## C11. Every error path returns success

This is the one that connects the C layer to the Python layer, so it's worth its own
heading.

Every failure in `pulsar_det_an_v4.c` ends in `exit(0)` — wrong argument count, missing
input file, missing `Blankf.txt`, `N > 100`, bad section range. Meanwhile
`lib/runners.py:170`:

```python
if result.returncode != 0:
    print(f"pulsar_det_an exited with code {result.returncode}")
return result.returncode == 0
```

That check can never fire for any of the conditions it was written to catch. The chain in
`run_program_chain` then happily proceeds to `pul_plot`, which reads whatever stale files
are left in `results/pulsar_det_an_results/` from a previous run and produces plots of old
data. The user sees plots and assumes the run worked.

Change all failure exits to `EXIT_FAILURE` and print to `stderr` rather than `stdout`.
`RTLChannel4bin.c` has the same pattern.

---

## C12. `io/files.c` — unchecked `fopen`, uninitialised slots, one leaked handle

```c
FILE **open_files() {
    FILE **result = (FILE **)malloc(NUM_OF_FILES * sizeof(FILE *));   /* unchecked */
    for (int i = 0; i < NUM_OF_FILES; i++) {
        ...
        case FPT_BLNKF: continue;    /* result[0] left uninitialised */
        case FPT_BLNKS: continue;    /* result[1] left uninitialised */
        ...
        result[i] = fopen(str, cmd);  /* unchecked */
    }
```

- **Not one `fopen` return is checked.** 32 output files are opened in the current working
  directory. If that directory isn't writable, or you hit the open-file limit, or the disk
  is full, `result[i]` is `NULL` and the first `fprintf` to it segfaults — after the program
  has already printed its whole banner, so it looks like a crash deep in the analysis.
- `close_files()` then calls `fclose()` on every slot including any `NULL`s, which is
  undefined behaviour on top of the original failure.
- Use `calloc` instead of `malloc` so the two `continue`d slots are deterministic `NULL`
  rather than uninitialised heap, and have `close_files` skip `NULL`.

```c
result[i] = fopen(str, cmd);
if (!result[i]) {
    fprintf(stderr, "Cannot create output file '%s' in the working directory: %s\n",
            str, strerror(errno));
    exit(EXIT_FAILURE);
}
```

**Leaked handle:** `pulsar_det_an_v4.c:73` opens `Blanks.txt` and never closes it (unlike
`Blankf.txt`, which is closed on line 71). Line 130 reopens it into the same slot,
overwriting the pointer. One `FILE` object and one fd leak per run. LeakSanitizer doesn't
report it because open `FILE`s stay reachable from glibc's internal list — so it wouldn't
have shown up in your testing. Add `fclose(files[FPT_BLNKS]);` after line 76.

---

## What's right

Worth saying explicitly, because a report like this reads as uniformly negative:

- **The `four()` rewrite in `numerics/four.c` is correct.** I checked the 0-based bit
  reversal and the Danielson–Lanczos stage against the 1-based Numerical Recipes original.
  The `i < n - mmax` bound (your "ASAN-safe bound" comment) is exactly right: it's the
  largest bound for which `data[j+1] = data[i+mmax+1]` stays inside `data[0..n-1]`, and it
  doesn't drop any butterfly. The twiddle recurrence is preserved. Good work.
- The `memset(datout, 0, sizeof(psnrReturn))` in `psnr()` fixes the classic
  `sizeof(pointer)` bug visible in the commented-out `snr()` above it.
- The per-channel `ftdat`/`pdat`/`fftdat` allocations in the match-filter loop are all
  correctly freed, and `comprc` is freed. No leaks under LSan.
- Replacing `ss % bins` with `ss & (bins - 1)` (line 735) is valid given the power-of-two
  check, and the comment says so.
- Splitting the monolith into `numerics/` and `io/` with headers, and the switch from
  globals to `calloc` for the scratch buffers, is the right direction. The remaining
  fixed-size globals are the leftovers of that migration.

---

## `RTLChannel4bin.c`

This file didn't get the treatment `pulsar_det_an_v4.c` did, and it's upstream in the
pipeline — garbage here propagates to everything downstream.

**Unvalidated FFT size — [reproduced].** `double dats[16384]` needs `2 * ftpts` entries,
so the real limit is `ftpts <= 8192`. There's no check:

```
./rtlch raw.bin out.bin 2.4 1 16384
RTLChannel4bin.c:90: runtime error: index 16384 out of bounds for type 'double [16384]'
ERROR: AddressSanitizer: global-buffer-overflow
```

**Non-power-of-two FFT size silently produces garbage — [reproduced].** `./rtlch raw.bin
out.bin 2.4 1 100` exits 0 and writes an output file. The radix-2 FFT requires a power of
two. You already have `is_pow_of_2()` in `numerics/` — use it here.

**`four(dats - 1, ftpts, -1)` (line 94).** This is the 1-based Numerical Recipes idiom.
Forming a pointer before the start of an object is undefined behaviour, and GCC flags it
without any sanitizer:

```
RTLChannel4bin.c:94:13: warning: 'four' accessing 8 bytes in a region of size 0 [-Wstringop-overflow=]
```

In practice the transform path never dereferences `data[0]`, so it works today. But the
`isign == 1` normalisation loop (`for (a = 0; a < 2*nn; a++) data[a] = ...`) *does* touch
`data[0]`, i.e. `dats[-1]` — so this becomes a genuine out-of-bounds write the moment
anyone calls it for an inverse transform. **More importantly: this is the same function you
already fixed in `numerics/four.c`, and the fix wasn't propagated.** There are four copies
of `four()` in the tree:

```
RTL/src/RAFFT22Lg.c
RTL/src/RTLChannel4bin.c
RTL/src/rapulsar2con.c
RTL/src/numerics/four.c    <- the fixed one
```

Link the other three against `numerics/four.o` and delete their local copies. That's one
Makefile change and three deletions, and it stops this class of bug recurring.

**`fseeko(fptr, SEEK_SET, SEEK_END)` (line 69).** The arguments are swapped. It works only
because `SEEK_SET == 0`, so it accidentally means "offset 0 from the end". Write
`fseeko(fptr, 0, SEEK_END)`.

**`getc()` return is not checked** (line 88). At EOF `getc` returns `-1`, which assigned to
`unsigned char uchi` becomes `255`, i.e. a full-scale sample. The final partial block is
filled with fake maximum-amplitude data.

---

## Build system

**`-march=native`.** Fine for a machine that compiles its own binaries, which is what
`compile_everything()` does — but it means `RTL/bin/*.out` cannot be copied to another
machine, and on a mixed observatory setup you'll get `SIGILL`. Worth a comment in the
Makefile at minimum.

**`make clean` destroys the user's blanking configuration.** `CLEAN = rm -f $(BIN_DIR)/*`
deletes `bin/Blanks.txt` and `bin/Blankf.txt`. `compile_everything()` runs `make clean`
whenever any binary is missing, and the Makefile then recreates them empty via `touch`.
So a rebuild silently wipes the channel/section blanking an observer has tuned. Move those
two files out of `bin/`, or exclude them: `rm -f $(BIN_DIR)/*.o $(BIN_DIR)/*$(EXT)`.

**Parallel build race.** `all: $(BIN_DIR) $(NUMERICS) $(IO) $(TARGETS)` — with `make -j`,
prerequisite *order* isn't guaranteed, so a compile can start before `bin/` exists. Use an
order-only prerequisite on the object rules: `$(BIN_DIR)/%.o: ... | $(BIN_DIR)`.

**`fseeko`/`ftello` depend on the default GNU feature macros.** With the Makefile's current
flags it's fine. Add `-std=c11` and you get:

```
warning: implicit declaration of function 'fseeko'; did you mean 'fseek'?
warning: implicit declaration of function 'ftello'; did you mean 'ftell'?
```

An implicitly declared `ftello()` is assumed to return `int`, silently truncating the
64-bit offset — which defeats the `-D_FILE_OFFSET_BITS=64` you added specifically for large
files. Add `#define _FILE_OFFSET_BITS 64` and `#include <sys/types.h>` at the top of the
source (before any header), or `-D_DEFAULT_SOURCE`, so it survives a stricter `-std`.

**`includes/common/commandLineArguments.h` does not compile.** No struct tag, no closing
semicolon:

```c
typedef struct commandLineArguments{
    
}
#endif
```

It isn't in the Makefile so nothing catches it. Either finish it or delete it and its
two-line `.c`.

---

## GUI and Python layer

### P1. The GUI freezes for the entire run

`gui.py:248`:

```python
output, error = process.communicate("\n".join(inputs) + "\n")
```

This is called directly from a Tk button callback, so it blocks the event loop until the
whole pipeline finishes — which for a real observation is minutes to hours. The window
stops redrawing, the OS marks it "Not Responding", and there's no output until the very
end and no way to cancel. For a tool whose main job is running a long analysis, this is
the biggest usability problem in the GUI.

The fix is a worker thread plus a `queue.Queue`, polled from the Tk thread with
`root.after()`:

```python
import threading, queue

log_q = queue.Queue()

def _pump():
    try:
        while True:
            line = log_q.get_nowait()
            if line is None:
                append_output("Done\n")
            else:
                append_output(line.rstrip("\n"))
    except queue.Empty:
        pass
    root.after(100, _pump)

def run_process(inputs, label="pipeline"):
    append_output(f"\nRunning {label}...\n")

    def worker():
        p = subprocess.Popen(
            [sys.executable, os.path.join(ROOT_DIR, "lib", "RAG_Pulsar_Software.py")],
            stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT, text=True, bufsize=1,
            cwd=ROOT_DIR, env=env,
        )
        p.stdin.write("\n".join(inputs) + "\n")
        p.stdin.close()
        for line in p.stdout:          # streams live
            log_q.put(line)
        rc = p.wait()
        if rc != 0:
            log_q.put(f"*** exited with code {rc} ***\n")
        log_q.put(None)

    threading.Thread(target=worker, daemon=True).start()

root.after(100, _pump)   # start the pump once, before mainloop()
```

Only the pump touches Tk widgets, which is what Tkinter requires. You also get live
progress, which matters because `pulsar_det_an` prints a lot as it goes.

Note that callers currently use the return value (`run_pipeline` parses it for the
topocentric period). With streaming you'd accumulate the lines in the worker and do that
parse at the end, inside the pump.

### P2. Failures are reported as success

`run_process` never inspects `process.returncode`; it unconditionally prints `"Done"`.
Combined with C11 (the C program exits 0 on every error) and the fact that `pul_plot` will
happily re-plot stale files, a completely failed run is visually indistinguishable from a
successful one. Fixing C11 and checking the return code here are a pair — neither is much
use alone.

### P3. The stdin protocol is a positional contract with no verification

`run_process` feeds a flat list of strings to a program that asks a sequence of interactive
questions. Your own comment documents how fragile this is, and `run_pipeline` already
contains the scar:

```python
if i == 4:
    # "ATNF pulsar period [ms]" — NOT prompted by io.py; TopoBary
    # computes it at runtime and passes it directly to pulsar_det_an.
    # Sending it here would shift every subsequent argument by one.
    continue
```

Any reordering of prompts in `io.py` silently shifts every subsequent value — RF bandwidth
becomes RF centre, start section becomes end section — and nothing errors out. You just
get wrong science. If the child asks one more question than you answered, `communicate()`
closes stdin and it dies with `EOFError`.

`lib/runners.py` already has clean keyword-based functions (`run_pulsar_det_an(combination)`
taking a dict). The right fix is for the GUI to build that dict and call `runners` directly
— in a subprocess if you want isolation, passing the dict as JSON — and retire the
interactive path for GUI use entirely. `io.py`'s prompting stays for the terminal workflow.
That deletes the positional coupling, the `i == 4` special case, and P2's return-code
problem at once.

Also: `params1_entries[4]` appears as a magic index in three places (lines 297, 314, 376).
Key the entries by name in a dict.

### P4. The barycentric correction is hardcoded to B0329+54

Both `gui.py:350` and `runners.py:60`:

```python
sc = SkyCoord(ra=3.54972*u.hr, dec=54.5786*u.deg)
```

The GUI exposes period, DM, P0, P1 and PEP as editable fields, so it presents itself as
working for any pulsar — but the Doppler correction is computed for B0329+54's sky
position regardless. Point it at B1919+21 and you get a silently wrong topocentric period,
which is precisely the parameter the whole fold depends on. The `print("B0329 ...")`
strings — which `runners.py` then parses with a regex keyed on the literal string
`"B0329 Topocentric period"` — make the assumption visible but not harmless.

Add RA/Dec fields next to P0/P1/PEP and thread them through. If the tool really is
B0329-only for now, say so in the GUI label and grey out the other fields.

### P5. Building Python source with f-strings from GUI text fields

```python
script = f"""
intm = "{obs_time}"
inlt = {latitude}
inln = {longitude}
...
P0  = {p0}
"""
subprocess.run([sys.executable, "-c", script], ...)
```

This is `eval` on user input. On a single-user observatory box the security angle is
mild, but the ergonomics are bad regardless: leave Latitude blank and you get
`inlt = ` → a `SyntaxError` traceback in the output box instead of "Latitude is required".
Type `54.5 N` and you get another one.

Write the calculation as a real function in `lib/topobary.py`, validate the inputs with
`float()` inside a `try`, and call it directly. No subprocess, no string building, no
regex to parse your own output back out.

### P6. Two divergent copies of the TopoBary calculation

`gui.py:340-363` and `runners.py:46-76` contain the same script with different parameter
sources: the GUI uses its own entry fields, `runners.py` reads `lib/topobary_config.txt`,
which the GUI writes only when you press "Run TopoBary" — not when you press "Run Full
Pipeline". So the standalone button and the pipeline can use different P0/P1/PEP values in
the same session. Extracting P5 into a shared function fixes this too.

### P7. `match`/`case` in `runners.py` breaks the 3.9 compatibility you added deliberately

Commit `ce9fa6f` — "Change io.py to be compatible with Python 3.9. The match function is
the problem, introduced in Python 3.10". But `runners.py:229` still has:

```python
match program:
    case "rtl_sdr":
```

So the 3.9 fix is incomplete; `runners.py` is imported by `RAG_Pulsar_Software.py` and will
raise `SyntaxError` at import on 3.9. Either convert it to an `if`/`elif` chain (or a dict
dispatch, which reads better here), or drop the 3.9 claim from the README and require 3.10+.

### P8. `--compile` is a dead flag and `make` failures are ignored

`RAG_Pulsar_Software.py`:

```python
parser.parse_args()          # return value discarded
compile_everything()         # runs unconditionally
```

So `make all` runs on every single button press regardless of the flag, and:

```python
subprocess.run(["make", "all"], capture_output=False, text=True, cwd=rtl_dir)
```

has no `check=True` and no return-code inspection. A compile error scrolls past and the
pipeline proceeds to run the previous (stale) binary, or a nonexistent one. Capture the
args, honour `--compile`, and check the result:

```python
args = parser.parse_args()
if args.compile or any_binary_missing():
    if not compile_everything():
        sys.exit("Build failed — see output above.")
```

Also `parser.add_argument("-compile", "-o", ...)` next to `-c/--compile` looks like a
leftover; `-o` for "compile" is actively confusing.

### P9. Smaller GUI items

- **Mouse wheel doesn't scroll on Linux.** `canvas.bind_all("<MouseWheel>")` with
  `event.delta` is Windows/macOS only; X11 delivers wheel events as `<Button-4>`/
  `<Button-5>`. Given the window is 1500x1100 and scrolls, this matters on the Linux boxes
  this is likely to run on. Bind all three and branch on `event.num`.
- **"Clear All Program Outputs" has no confirmation** and sits immediately beside
  ">> Run Full Pipeline", same size, same row. One is green, one is red, and one
  irreversibly `rmtree`s your results. Add `messagebox.askyesno`.
- **`make_section_info_button` and `make_section_info_button_grid`** are byte-identical
  apart from `.pack()` vs `.grid()`. Take the placement as a callback.
- **No `if __name__ == "__main__":`** — `gui.py` builds the UI and calls `mainloop()` at
  import time, so it can't be imported or tested. Wrap the setup in a `main()`.
- **Redundant local imports:** `subprocess`, `sys`, `re`, `shutil` are re-imported inside
  functions that already have them at module scope.
- **`requirements.txt` pins exact versions** (`numpy==2.4.4` etc.). For an amateur-facing
  tool that people will `pip install` onto whatever Python they have, `>=` with a tested
  lower bound will cause far fewer support emails.

---

## Suggested order of work

**Do first — these are reachable with valid-looking inputs:**

1. Fix the parameter guards (C1/C2/C3). Define `MAX_CHAN`/`MAX_BINS`/`MAX_PTS`, use them
   in both the declarations and the checks, and validate `PTS` rather than `M*N`.
2. Bound and range-check the `Blankf.txt`/`Blanks.txt` loops (C4). A hand-edited data file
   should never be able to segfault the analysis.
3. Check the `fread` return and the `malloc` (C5). This is the silent-wrong-answer path.
4. Change all failure exits to `EXIT_FAILURE` (C11), and check `returncode` in
   `run_process` (P2). These two go together.

**Then — correctness of the results themselves:**

5. `spectrum()` half-filled buffer (C6).
6. `psnr()` NaN guard and `float` → `double` accumulators (C7), plus `bestprof` init (C8).
7. The hardcoded B0329+54 coordinates (P4).

**Then — robustness and usability:**

8. Thread the subprocess so the GUI stays alive and streams output (P1).
9. Check `fopen` returns in `files.c` (C12), and close the leaked `Blanks.txt` handle.
10. Validate `ftpts` in `RTLChannel4bin.c` and link it against the fixed `numerics/four.o`
    along with the other two copies.
11. Replace the positional stdin protocol with direct dict-based calls into `runners.py`
    (P3), which also removes P5 and P6.

**Housekeeping:** `make clean` wiping the blank files, the parallel-build race, the
`match`/`case` 3.9 break, the dead `--compile` flag, `commandLineArguments.h`.

---

## One structural suggestion

The recurring theme across almost every C finding is that **a size lives in two places**:
a `[32]` in a declaration and a `100` in a guard; a `[4096]` in a declaration and no guard
at all; `PTS/2` written in `spectrum()` and `PTS` read in the caller; four copies of
`four()` where only one got fixed.

Two cheap habits would have caught nearly all of it:

- A single header of `#define MAX_*` limits used by both the declarations and the argument
  checks, so they can't drift.
- Add a `make asan` target and run it once per parameter change:

  ```makefile
  asan: CFLAGS = -O1 -g -D_FILE_OFFSET_BITS=64 -fsanitize=address,undefined \
                 -fno-omit-frame-pointer -Wall -Wextra
  asan: LDFLAGS = -lm -fsanitize=address,undefined
  asan: clean all
  ```

  You already have `regressionTester/regTest.py` — pointing it at an ASan build with a few
  boundary parameter sets (N = 1/32/33, bins = 4096/8192, an over-long `Blankf.txt`) turns
  every finding above into a test that fails loudly instead of corrupting a result quietly.

For a project of this size the code is in decent shape, and the refactoring direction —
splitting out `numerics/`, moving scratch buffers to `calloc`, fixing the FFT bounds — is
the right one. The gap is that the migration stopped halfway: the allocations got fixed,
the fixed-size globals and the guards that were supposed to protect them didn't.
