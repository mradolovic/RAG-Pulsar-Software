#ifndef __ANALYSIS_LIMITS__
#define __ANALYSIS_LIMITS__

/*
 * Limits imposed by the fixed-size working arrays in pulsar_det_an_v4.c.
 *
 * These are not physical or scientific limits. They are the dimensions the
 * arrays happen to be declared with, collected in one place so that the
 * declarations and the argument checks cannot drift apart again. Every value
 * below is derived from a declaration; the derivation is given so that anyone
 * changing an array knows which constant must change with it.
 *
 * If you raise one of these, you must also change every array listed against
 * it. Building with "make asan" and running the new parameters is the cheapest
 * way to confirm you caught them all.
 */

/*
 * Number of FFT frequency channels (argv[2], "N").
 *
 * Bounded by the per-channel dimension, which is 32 in all of:
 *   comprs[.][32]  ave[.][32]  rms[32][.]  av[32][.]  rm[32][.]
 *   mean[32]  std[32]  compval[32][.]  compvald[.][32]  compvaldd[32][.]
 *   spect[32][.]  bndcum[32][.]
 *
 * Note that outsumt[100][.] is indexed both by channel (num < N) and by period
 * step (st < 51), so its first dimension must stay >= max(MAX_CHAN, 51).
 */
#define MAX_CHAN 32

/*
 * Number of fold bins (argv[6], "bins").
 *
 * Bounded by the fold buffers, which are 4096 in all of:
 *   dfold[4096]  sumt[4096]  count[4096]  outdat[4096]  pkdat[4096]
 *   bstdmprf[4096]  bestprof[4096]  ddfold[.][4096]  outsumt[.][4096]
 *   ave[4096][.]  rms[.][4096]  av[.][4096]  rm[.][4096]  bndcum[.][4096]
 */
#define MAX_BINS 4096

/*
 * Total output points, PTS = M * bins.
 *
 * Bounded by the dispersion search matrix ddispbands[.][131072], which is the
 * tightest of the PTS-indexed arrays. The others are larger:
 *   compval[.][262144]  compvald[262144][.]  compvaldd[.][262144]
 *   spect[.][262144]  comprs[262144][.]  allbands[262144] and friends
 *   targ[1048576] and ftdat2[1048576], which hold 2*PTS doubles and so
 *   independently require PTS <= 524288
 */
#define MAX_PTS 131072

/*
 * Maximum number of entries read from Blankf.txt and Blanks.txt.
 *
 * Bounded by blaf[256] and blas[256].
 */
#define MAX_BLANK 256

#endif
