#ifndef __CONV__
#define __CONV__
// convolving function - used as matched filter
void conv(double *restrict sum, float pulw, int PTS, double period, int M, double *restrict pdat,
          double *restrict targ, double *restrict fftdat);
#endif
