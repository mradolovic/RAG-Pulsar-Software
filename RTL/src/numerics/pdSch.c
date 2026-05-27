#include "../../includes/numerics/pdSch.h"
// Pdot search theoretical plot - see http://www.y1pwe.co.uk/RAProgs/PulsarAnalysisLowSNR2.doc
float pdSch(double pd, int Np, float w, float P, float cor) {
    double wfac, lnf, sum = 0, sumo = 0, nn;
    int n, sh;
    wfac = 2.0 * (double)1000000.0 * (double)cor;
    lnf = -4.0 * log(2.0);
    for (sh = -0; sh < 10; sh += 1) {
        for (n = 2; n < Np + 2; n += 1) {
            nn = (pd * P * ((double)(n - 1) * (double)(n - 2)) / wfac);
            nn = ((double)(sh / 40.0) + nn) / (double)w;
            nn = pow(nn, 2.0);
            sum = sum + exp(lnf * nn) / Np;
        }
        if (sum > sumo) {
            sumo = sum;
        }
        sum = 0;
    }
    return ((float)sumo);
}
