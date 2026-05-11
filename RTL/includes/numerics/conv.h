#ifndef __CONV__
#define __CONV__
// convolving function - used as matched filter
void conv(double sum[], float pulw, int PTS, double period, int M, double pdat[], double targ[], double fftdat[]);
#endif
