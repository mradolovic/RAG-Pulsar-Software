#include "../../includes/numerics/spectrum.h"

#include "../../includes/numerics/four.h"


void spectrum(double sum[], int PTS, double pdat[], double ftdat2[]) {
    int v;

    /*FT of folded data */
    for (v = 0; v < PTS; v++) {
        pdat[2 * v] = sum[v];
        pdat[2 * v + 1] = 0;
    }
    /*FT of Gaussian target data */
    four(pdat , PTS, -1);
    /*Target magnitude */
    for (v = 0; v < PTS / 2; v++) {
        ftdat2[v] = pdat[2 * v] / sqrt((double)PTS);
    }
    return;
} 


