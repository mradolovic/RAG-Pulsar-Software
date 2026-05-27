#ifndef __PSNR__
#define __PSNR__
#include <string.h>
#include <math.h>
// simple snr calculator
//it is depricated as the psnr version is supperior but it is left in for reasons of posterity
//void snr(int n, double dat[]);

typedef struct psnrReturn_S{
    double std_snr; //I am uncertain whether this is the correct name for this variable
    double nx;
    double mn;
    double rms;
    double mx;
    double unknown_var;
} psnrReturn;

//improved and used snr calculation
void psnr(int bins, double * restrict dat, int mbin, psnrReturn * restrict datout, float pulw, double * restrict outdat);
#endif
