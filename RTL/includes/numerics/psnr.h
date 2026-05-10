#ifndef __PSNR__
#define __PSNR__
#include <string.h>
#include <math.h>
// simple snr calculator
//it is depricated as the psnr version is supperior but it is left in for reasons of posterity
//void snr(int n, double dat[]);

//improved and used snr calculation
void psnr(int bins, double * restrict dat, int mbin, double * restrict datout, float pulw, double * restrict outdat);
#endif
