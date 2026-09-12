# RAG-Pulsar-Software

GUI (`gui.py`) plus a Python orchestration layer (`lib/`) around Peter East's
pulsar detection C code (`RTL/src/`). RTL-SDR observations are channelised,
folded at a candidate period, and searched over period / p-dot / DM space.

**Read `docs/HANDOFF.md` before starting work.** It carries the decisions, the
outstanding items in priority order, and the verification status. This file is
only the part that must never be rediscovered the hard way.

## The working rule

Behaviour-preserving and behaviour-changing changes **never share a commit**.

- **Behaviour-preserving** — must produce byte-identical output. Validation
  guards, bounds checks, allocation and read checks, exit codes, memory layout,
  build changes, anything in the Python layer. Prove it before committing:

  ```bash
  cd RTL && make clean && make all
  scripts/compare_outputs.sh ~/pulsar_baselines/item1_streaming   # pulsar_det_an
  scripts/compare_rtlchan.sh  ~/pulsar_baselines/rtlchan/base     # RTLChannel4bin
  ```

  Both must print PASS. If they don't, something changed that wasn't meant to.

- **Behaviour-changing** — alters the numbers. One commit each, stating exactly
  which numbers change and why, and it needs a validation run before merging.
  Never alter the numerical core's output without saying explicitly what moves.

## The two stages read very different volumes

Get this right before optimising anything for file size. `RTLChannel4bin` reads
the raw capture; `pulsar_det_an` reads only the channelised output, which at the
observatory's settings is **1/64 the size**.

| stage | reads | 240 min |
|---|---|---|
| `RTLChannel4bin` | raw 8-bit I/Q (`data/raw data/`) | ~59 GB |
| `pulsar_det_an` | channelised floats (`data/*.bin`) | ~920 MB |

## Parameters in use

```
RTLChannel4bin:  <raw.bin> <out.bin> 2.048 1 16
pulsar_det_an:   <chan.bin> 16 1 714.463240 128 1024 6.5 26.7 -1.3 6 1 2 409 50 0 127
```

`pulsar_det_an` applies the ppm adjustment before writing `header.txt`, so the
period it reports is slightly below the one passed in. Expected, not drift.

## CRLF trap

`RTL/src/pulsar_det_an_v4.c` and `RTL/src/RTLChannel4bin.c` are **CRLF**; the rest
of the tree is LF, and there is no `.gitattributes`.

- Apply patches with `git am --keep-cr`. Without it the context lines are mangled
  and the whole file lands in the diff.
- Edit these two in binary rather than with anything that normalises endings.
- Verify after every edit — every line must be CRLF:

  ```bash
  f=RTL/src/RTLChannel4bin.c; [ "$(wc -l < $f)" = "$(grep -c $'\r$' $f)" ] && echo OK
  ```

## Other traps

- `make clean` is `rm -f bin/*`, which deletes `bin/Blankf.txt` / `bin/Blanks.txt`
  and silently destroys hand-tuned blanking. Check they're empty before running it.
- `-march=native` means `RTL/bin/*` cannot be copied between machines.
- `pul_plot` replots whatever is left in `results/pulsar_det_an_results/`, so a
  failed run followed by a plot looks exactly like a successful one.
- Baselines live in `~/pulsar_baselines/`, deliberately outside the repo so
  `make clean` and a `results/` wipe cannot reach them.
