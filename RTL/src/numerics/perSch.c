#include "../../includes/numerics/perSch.h"

// Period search theory plot - see http://www.y1pwe.co.uk/RAProgs/PulsarAnalysisLowSNR2.doc
float perSch(double p, int Np, float w, float P) {
    double wfac, lnf, sum = 0, nn;
    int n;
    wfac = 2000000 * (double)w / (double)Np / (double)P;
    lnf = -4.0 * log((double)2.0);
    for (n = 0; n < Np; n += 1) {
        nn = (p * (1.0 - (double)(n) / (double)Np));
        nn = nn / wfac;
        nn = nn * nn;
        sum = sum + exp(lnf * nn) / Np;
    }
    return ((float)sum);
} 

