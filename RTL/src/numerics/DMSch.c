#include "../../includes/numerics/DMSch.h"

// DM search theory plot - see http://www.y1pwe.co.uk/RAProgs/PulsarAnalysisLowSNR2.doc
float DMSch(double dm, int Nf, float DM, float td, float w) {
    double wfac, lnf, sum = 0, nn, nn2;
    int n;
    wfac = (double)w * (double)DM / (double)(td * (double)(Nf - 1) / (double)(Nf));
    lnf = (double)(-4.0) * (double)log(2.0);
    for (n = 1; n < Nf + 1; n += 1) {
        nn = (((double)dm - (double)DM) * ((double)0.5 - (double)(n) / (double)Nf));
        nn = nn / wfac;
        nn2 = nn / 2;
        nn = nn * nn;
        nn2 = nn2 * nn2;
        sum = sum + (exp(lnf * nn) / (double)(Nf)) + 0.5 * (exp(lnf * nn2) / (double)(Nf));
    }  
    return ((float)sum / 1.5);
} 


