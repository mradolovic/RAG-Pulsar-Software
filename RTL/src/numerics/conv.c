#include "../../includes/numerics/conv.h"
#include "../../includes/numerics/four.h"

void conv(double sum[], float pulw, int PTS, double period, int M, double pdat[], double targ[], double fftdat[]) {
    int v, low, high;
    float prat;

    prat = period / pulw;                   // number of pulse widths in a period
    low = (int)(0 * 2 * M * period / 1000); // low frequency cut off for folded data:
    high = (int)(1.1 * M * prat);           // high frequency cut off for folded data

    /*FT of folded data */
    for (v = 0; v < PTS; v++) // fill FFT input data
    {
        pdat[2 * v] = sum[v];
        pdat[2 * v + 1] = 0;
    }

    four(pdat , PTS, -1); // FFT data
    /*Convolve fold data spectrum with pulse spectrum  */
    for (v = 0; v < PTS; v++) {
        pdat[2 * v] = pdat[2 * v] * targ[v] / targ[0];
        pdat[2 * v + 1] = pdat[2 * v + 1] * targ[v] / targ[0];
    }
    // Low and High pass filtering
    // LF Filtering
    for (v = 0; v < (int)(low); v++) {
        pdat[2 * v] = 0;
        pdat[2 * v + 1] = 0;
        pdat[2 * PTS - 1 - 2 * v] = 0;
        pdat[2 * PTS - 1 - (2 * v + 1)] = 0;
    }
    // HF Filtering
    for (v = high; v < (int)(PTS - high); v++) {
        pdat[2 * v] = 0;
        pdat[2 * v + 1] = 0;
        pdat[2 * PTS - 1 - 2 * v] = 0;
        pdat[2 * PTS - 1 - (2 * v + 1)] = 0;
    }
    /*Inverse FT of convolved and filtered data */
    four(pdat , PTS, 1);

    /*Output convolved real magnitude */
    for (v = 0; v < PTS; v++) {
        fftdat[v] = pdat[2 * v];
    }

    return;
}
