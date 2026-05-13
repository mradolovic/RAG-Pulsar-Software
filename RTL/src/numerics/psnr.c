#include "../../includes/numerics/psnr.h"
// Simple Peak to rms noise ratio
/*void snr(int n, double dat[]) {
    int t, a;
    double mn = 0, rms = 0, mx = 0, nx = 0, mb = 0;
    memset(datout, 0, sizeof(datout));
    for (t = 0; t < n; t++) {
        mn = mn + dat[t] / n;              // calculate mean
        rms = rms + (dat[t] * dat[t]) / n; // calculate mean square
        if (dat[t] > mx)                   // find max
        {
            mx = dat[t];
            if (t == mbin)
                mb = dat[t];
            nx = (float)t;
        }
    }
    rms = sqrt(rms - mn * mn) + 0.0001;
    datout[0] = (mx - mn) / rms;
    datout[1] = nx;
    datout[3] = mn;
    datout[4] = rms;
    datout[5] = mx;
    datout[6] = (mb - mn) / rms;
    for (t = 0; t < n; t++) {
        outdat[t] = (dat[t] - mn) / rms;
    }
} // end of noise snr*/

// Pulse peak to rms noise ratio
void psnr(int bins, double * restrict dat, int mbin, psnrReturn * restrict datout, float pulw, double * restrict outdat){
    int t, n1, n2;
    float mn = 0, rms = 0, mnr = 0, rmsr = 0, mx = 0, nx = 0, mb = 0;
    // TODO: 8 is hardcoded here to be the length of the datout array this is bad practice and needs
    // to be changed
    memset(datout, 0, sizeof(psnrReturn));
    for (t = 0; t < bins; t++) {
        mn = mn + dat[t];
        rms = rms + (dat[t] * dat[t]);
        if (dat[t] > mx) {
            mx = dat[t];
            nx = (float)t;
        }
        if (t == mbin)
            mb = dat[t];
    }
    n1 =
        (int)(nx -
              (int)((float)(0 + 8 * pulw * bins * 1 / 1024))); // 35 for large signals//8 small sigs
    n2 = (int)(nx + (int)((float)(0 + 8 * pulw * bins * 1 / 1024)));
    if (n1 < 0)
        n1 = 0;
    if (n2 > bins - 1)
        n2 = bins - 1;
    for (t = n1; t < n2 - 1; t++) {
        mnr = mnr + dat[t];
        rmsr = rmsr + (dat[t] * dat[t]);
    }
    mn = (mn - mnr) / (bins - n2 + n1);
    rms = (rms - rmsr) / (bins - n2 + n1);
    rms = rms - mn * mn;
    rms = sqrt(rms) + 0.0000001;

    if (sqrt(rms) < 0.00000001) {
        datout->std_snr = 0.00001;
    } else {
        datout->std_snr = (mx - mn) / rms;
    }
    datout ->nx = nx;
    datout -> mn = mn;
    datout->rms = rms;
    datout->mx = mx;
    datout->unknown_var = (mb - mn) / rms;
    for (t = 0; t < bins; t++) {
        outdat[t] = (dat[t] - mn) / rms;
    }
    return;
}
