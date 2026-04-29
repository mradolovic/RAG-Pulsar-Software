Amateur Pulsar Detection Pipeline
================================================================================
A GUI front-end for pulsar_det_an - Peter East's software alternative to PRESTO.

Developed in 2026 by:
  Peter East          - pulsar detection software
  Martin Ante Rogošić - parsing script and compiling
  Marko Radolović     - GUI

================================================================================
OVERVIEW
================================================================================
This software provides a graphical interface for running a chain of programs
that detect and analyse pulsar signals from RTL-SDR radio observations. It is
designed for amateur radio astronomers who want to detect real pulsars — such as
B0329+54 — using affordable hardware and open-source software.

The pipeline takes raw IQ data from an RTL-SDR dongle, converts it into
frequency-channelled data, searches it for a periodic signal at the expected
pulsar period and dispersion measure, and produces a set of diagnostic plots.
Before the detection stage, a barycentric-to-topocentric period correction is
applied automatically using TopoBary, which accounts for the Doppler shift
caused by the Earth's motion relative to the pulsar.

================================================================================
PIPELINE STAGES
================================================================================
  1. rtl_sdr
     Captures raw 8-bit IQ samples from the RTL-SDR USB dongle and saves them
     to a binary .bin file. Not yet implemented in this GUI — use the rtl_sdr
     command-line tool directly to record your observation, then point the GUI
     at the resulting file.

  2. RTLChannel4bin
     Reads the raw .bin file and converts it into a channelised 4-byte float
     binary file. It applies an N-point FFT to split the receiver bandwidth into
     frequency channels, then time-averages groups of FFT frames to reduce the
     data rate to a manageable output rate. The result is a compact binary file
     where each row contains the power in each frequency channel at one time
     step. This file is the input to pulsar_det_an.

  3. TopoBary  (runs automatically before pulsar_det_an)
     Computes the topocentric pulse period for the target pulsar at your
     observing site and time. It uses the ATNF catalogue parameters (P0, P1,
     epoch) to find the current barycentric period, then applies a Doppler
     correction based on the Earth's radial velocity toward the pulsar. The
     resulting topocentric period is used as the centre of the period search in
     pulsar_det_an. You can also run TopoBary independently using the Run
     TopoBary button, which will update the ATNF period field in the GUI.

  4. pulsar_det_an
     The core detection and analysis program. It reads the channelised binary
     file, folds the data at a trial period, and searches over a range of
     dispersion measures, periods, and period derivatives (p-dot) to find the
     combination that gives the highest signal-to-noise ratio. A Gaussian
     matched filter shaped to the expected pulse width is used to improve
     sensitivity. Results are written to a set of text files covering the folded
     pulse profile, DM search, period search, p-dot search, band SNR, section
     SNR timeline, rolling average, and waterfalls.

  5. pul_plot
     Reads all the text output files from pulsar_det_an and renders them as a
     set of diagnostic plots. These include the folded pulse profile, DM search
     curve, period and p-dot search, band SNR chart, cumulative SNR growth,
     rolling average SNR, compressed data waterfall, and a 2D period/p-dot
     image. All plots are saved as PNG files and stitched into a single combined
     image for easy review.

================================================================================
VALID PROGRAM CHAINS
================================================================================
You can run the full pipeline or any subset of stages using the checkboxes in
the Program Selection section. The following combinations are supported:

  1 → 2 → 3        Full pipeline (RTLChannel4bin → pulsar_det_an → pul_plot)
  2 → 3            Detection and plotting from an existing channelised file
  1 only           Channelise a raw .bin file without running detection
  2 only           Run pulsar_det_an on an existing channelised file
  3 only           Re-plot existing pulsar_det_an output files

Note: stage 1 (rtl_sdr) is not yet integrated. Record your observation
separately and provide the resulting .bin file as input to RTLChannel4bin.

================================================================================
TYPICAL WORKFLOW
================================================================================
  1. Record an observation with rtl_sdr (externally) at 2.4 MSPS, tuned to
     your target frequency (e.g. 422 MHz for B0329+54). Save the .bin file.

  2. Open the GUI and set the RTLChannel4bin input file to your .bin file and
     the output file to a new .bin path (e.g. obs_channelised.bin).

  3. Set the TopoBary parameters: observation UTC time, your site latitude and
     longitude, and the ATNF catalogue values for your target pulsar.

  4. Click Run TopoBary to compute the topocentric period. The ATNF period
     field will update automatically.

  5. Set the pulsar_det_an parameters (DM, pulse width, period search range,
     etc.) and select which stages to run using the checkboxes.

  6. Click Run Pipeline. Output text files are saved to
     data/pulsar_det_an_results/ and plots to data/pul_plot_results/.

  7. Check the plot output — look for a clear peak in the pulse profile, a
     peak at the correct DM in the DM search, and a growing cumulative SNR.

================================================================================
OUTPUT DIRECTORIES
================================================================================
  data/pulsar_det_an_results/   All .txt output files from pulsar_det_an.
                                 These are intermediate files read by pul_plot.
                                 They are overwritten on each run.

  data/pul_plot_results/        PNG plot files produced by pul_plot, including
                                 the combined diagnostic image (concat3_v.png).

================================================================================
PARAMETER INFO BUTTONS
================================================================================
Every parameter field in the GUI has an ℹ button to its right. Clicking it
opens a popup with a short description of that parameter — what it does, what
units it takes, and what a typical value looks like. The text is stored in a
plain .txt file and can be edited directly in the popup by clicking Edit.

Each program block (RTLChannel4bin, pulsar_det_an, TopoBary, etc.) also has a
More info button in its title bar. This opens a description of the whole
program block. These texts are also editable .txt files.

================================================================================
FILES AND FOLDERS
================================================================================
  lib/param_info/     One .txt file per parameter. Filename matches the
                      parameter key shown in the popup title. Edit these files
                      to update the descriptions shown by the ℹ buttons.

  lib/section_info/   One .txt file per program block. Edit these to update
                      the descriptions shown by the More info buttons.

  lib/README.txt      This file. Edit freely — it is displayed by the
                      📖 README button in the top-right corner of the GUI.

  lib/topobary_config.txt   Saved TopoBary parameters (P0, P1, PEP). Written
                             automatically when TopoBary is run, and read by
                             the pipeline runner so pulsar_det_an uses the
                             same values.

================================================================================
