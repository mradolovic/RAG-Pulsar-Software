Amateur Pulsar Detection Pipeline
================================================================================
A GUI front-end for pulsar_det_an - Peter East's software alternative to PRESTO.

Developed in 2026 by:
Peter East          - pulsar detection software
Martin Ante Rogošić - parsing script and compiling
Marko Radolović     - GUI


OVERVIEW
--------
This software provides a graphical interface for running a chain of programs
that detect and analyse pulsar signals from RTL-SDR radio observations.

Pipeline stages:
  1. rtl_sdr         — Capture raw IQ data from RTL-SDR dongle (not yet implemented)
  2. RTLChannel4bin  — Channelise raw binary data into frequency bands
  3. pulsar_det_an   — Detect and analyse pulsar signal (folding, DM search, period search)
  4. pul_plot        — Plot results (pulse profile, DM curve, waterfall, period search)

TopoBary runs automatically before pulsar_det_an in order to apply barycentric to topocentric correction.


VALID PROGRAM CHAINS
--------------------
  1 → 2 → 3        Full pipeline
  2 → 3            Detection and plotting only
  1 only           Channelisation only
  2 only           Pulsar detection only
  3 only           Plotting results


OUTPUT DIRECTORIES
------------------
  data/pulsar_det_an_results/   All .txt output files from pulsar_det_an
  data/pul_plot_results/        PNG plot files from pul_plot


INFO BUTTONS
------------
  ℹ  next to each parameter — Description of that parameter (edit param_info/*.txt)
  ℹ  on each section title  — Description of that program block (edit section_info/*.txt)
  📖 README                 — This file


FILES
-----
  param_info/     — One .txt file per parameter. Edit these to update parameter descriptions.
  section_info/   — One .txt file per program block. Edit these to update block descriptions.
  README.txt      — This file. Edit freely.
