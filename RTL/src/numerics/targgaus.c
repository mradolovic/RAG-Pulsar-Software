#include "../../includes/numerics/targgaus.h"
#include "../../includes/numerics/four.h"
#include <math.h>

#define ZERO  0.000001
// Gaussian shaped target pulse
float gauss(float t, float T, float z) {

    const float m = 4.0 * (float)log(2);
    const float out = exp(-m * (t - z) * (t - z) / T / T);
    return out;
} // end of gaussian shape

// Convolve fold data
// Gaussian target
void targgaus(float pulw, int PTS, double targ[], double period, int M) {
    int v;

    /*FT of folded data */
    for (v = 0; v < PTS; v++) {
        targ[v * 2] = gauss(v, (pulw * PTS / period / M), (PTS / 2.f));
        targ[2 * v + 1] = 0;
    }
    /*FT of Gaussian target data */
    four(targ , PTS, -1);
    /*Target magnitude */
    for (v = 0; v < PTS; v++) {
        targ[v] = sqrt(targ[2 * v] * targ[2 * v] + targ[2 * v + 1] * targ[2 * v + 1]) + ZERO;
    }
    return;
}


