#include "../../includes/numerics/four.h"

#define SWAP(a, b)                                                                                 \
    do {                                                                                           \
        tempr = (a);                                                                               \
        (a) = (b);                                                                                 \
        (b) = tempr;                                                                               \
    } while (0);

// fast fourier transform routine - see Numerical Methods in C, WH Press et al, Cambridge University
// Press
/*void four(double data[], int PTS, int isign) {
    int n, mmax, m, j, istep, i, a;
    double wtemp, wr, wpr, wpi, wi, theta;
    double tempr, tempi;
    n = PTS << 1;
    j = 1;
    for (i = 1; i < n; i += 2) {
        if (j > i) {
            SWAP(data[j], data[i]);
            SWAP(data[j + 1], data[i + 1]);
        }
        m = n >> 1;
        while (m >= 2 && j > m) {
            j -= m;
            m >>= 1;
        }
        j += m;
    }
    mmax = 2;
    while (n > mmax) {
        istep = 2 * mmax;
        theta = 6.28318530717959 / (isign * mmax);
        wtemp = sin(0.5 * theta);
        wpr = -2.0 * wtemp * wtemp;
        wpi = sin(theta);
        wr = 1.0;
        wi = 0.0;
        for (m = 1; m < mmax; m += 2) {
            for (i = m; i <= n; i += istep) {
                j = i + mmax;
                tempr = wr * data[j] - wi * data[j + 1];
                tempi = wr * data[j + 1] + wi * data[j];
                data[j] = data[i] - tempr;
                data[j + 1] = data[i + 1] - tempi;
                data[i] += tempr;
                data[i + 1] += tempi;
                if (j < 0)
                    j = 0;
            }
            wr = (wtemp = wr) * wpr - wi * wpi + wr;
            wi = wi * wpr + wtemp * wpi + wi;
        }
        mmax = istep;
    }
    if (isign == 1) {
        for (a = 0; a < 2 * PTS; a++) {
            data[a] = data[a] / PTS;
        }
    }
    return;
}

*/
void four(double data[], int PTS, int isign) {
    int n, mmax, m, j, istep, i, a;
    double wtemp, wr, wpr, wpi, wi, theta;
    double tempr, tempi;

    n = PTS << 1;

    j = 0;

    // Bit reversal
    for (i = 0; i < n; i += 2) {

        if (j > i) {
            SWAP(data[j], data[i]);
            SWAP(data[j + 1], data[i + 1]);
        }

        m = n >> 1;

        while (m >= 2 && j >= m) {
            j -= m;
            m >>= 1;
        }

        j += m;
    }

    mmax = 2;

    while (n > mmax) {

        istep = mmax << 1;

        theta = 6.28318530717959 / (isign * mmax);

        wtemp = sin(0.5 * theta);

        wpr = -2.0 * wtemp * wtemp;
        wpi = sin(theta);

        wr = 1.0;
        wi = 0.0;

        for (m = 0; m < mmax; m += 2) {

            // ASAN-safe bound
            for (i = m; i < n - mmax; i += istep) {

                j = i + mmax;

                tempr = wr * data[j] - wi * data[j + 1];
                tempi = wr * data[j + 1] + wi * data[j];

                data[j] = data[i] - tempr;
                data[j + 1] = data[i + 1] - tempi;

                data[i] += tempr;
                data[i + 1] += tempi;
            }

            wr = (wtemp = wr) * wpr - wi * wpi + wr;
            wi = wi * wpr + wtemp * wpi + wi;
        }

        mmax = istep;
    }

    if (isign == 1) {
        for (a = 0; a < n; ++a)
            data[a] /= PTS;
    }
}
