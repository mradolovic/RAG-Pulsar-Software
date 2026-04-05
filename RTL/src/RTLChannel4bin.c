
/*rtlchannel4bin.c	   pwe: 10/08/2022
Takes rtlsdr .bin 8-bit single byte files, calculates n-point ffts, splits frequency components and
averages these in time, ready for folding. Outputs a 4-byte binary file of sequential frequency
channelled data at the specified clock rate.

Command format:- rtlchannel4bin <rtlfile.bin> <outfile.bin> <clock rate (MHz) <downsample clock rate
(kHz))><No: fft points> */

// Example:..
// obs_20251003_2.bin rag_obsm.bin 2.4 1 16

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SWAP(a, b)                                                                                 \
    tempr = (a);                                                                                   \
    (a) = (b);                                                                                     \
    (b) = tempr
#define PI (6.28318530717959 / 2.0)

long long int count = 0;

float dsr, RF, DCF, df, DSC;
int ftpts, DM, fn, ddt, bn, cn;
long long int an, DSR;
float clck;
double dats[16384], datr[16384];
float darray[16384];
unsigned char uchi, uchq;
long long int file_end;

FILE *fptr;
// FILE *fpto;
FILE *fptobin;

void out_dat(void);
void ftorg_dat(void);
void four(double[], int, int);

int main(int argc, char *argv[]) {

    /*check command line arguments*/
    if (argc != 6) {
        printf("Format: rtlchannel4bin.exe <infile> <outbin> <clock rate (MHz) <downsample clock "
               "rate (kHz))><No: fft points> \n");
        exit(0);
    }

    if ((fptr = fopen(argv[1], "rb")) == NULL) {
        printf("Can't open file %s. ", argv[1]);
        exit(0);
    }

    DCF = atof(argv[3]);                     // data clock frequency (MHz)
    DSC = atof(argv[4]);                     // downsample clock frequency (kHz)
    ftpts = atoi(argv[5]);                   // FFT points
    dsr = (DCF * 1000 / DSC / (float)ftpts); // ratio of clock to video number of data points
                                             // averaged - or downsampling ratio
    clck = 1 / DCF;                          // data clock interval

    printf("Data Clock Frequency=%1.2fMHz\n", DCF);
    printf("Data Clock Interval=%1.2fus\n", clck);
    printf("Data Output Frequency=%1.2fkHz\n", DSC);
    printf("Downsample ratio=%1.2f\n", dsr);

    /*find length of input file*/
    fseeko(fptr, SEEK_SET, SEEK_END);
    file_end = (long long int)(ftello(fptr));

    printf("No. Bytes = %lld\n", (long long)file_end);
    printf("FFT Points=%d\n", ftpts);

    fclose(fptr);

    fptr = fopen(argv[1], "rb");
    fptobin = fopen(argv[2], "wb");

    /*read input file,decode I and Q, determine power. Sum powers in clock rate/video band blocks.
     At end of input file, output text file with averaged data*/

    for (an = 0; an < (file_end / (long long)(dsr * ftpts * 2)); an++) /// 1000
    {
        for (bn = 0; bn < dsr; bn++) {
            for (fn = 0; fn < 2 * ftpts; fn++) { // get sdr binfile data for FFT
                uchi = getc(fptr);
                dats[fn] = (float)uchi + 0.0;
                dats[fn] = (dats[fn] - 128) / 128.0;
            }
            /*take fourier transform*/
            four(dats - 1, ftpts, -1);
            /*reorganise spectrum positive/negative data*/
            ftorg_dat();
            // summ fft outputs and place in array
            for (cn = 0; cn < ftpts; cn++) {
                darray[cn] = darray[cn] + datr[cn];
            }
        }
        /*output summed FFT data to output file*/
        out_dat();
        for (cn = 0; cn < ftpts; cn++) {
            darray[cn] = 0;
        }
    }

    fclose(fptr);
    // fclose(fpto);
    fclose(fptobin);
    printf("No. O/P samples = %lld\n", (count));
    printf("\n Infile = %s   Outfilebin = %s\n", argv[1], argv[2]);

    exit(0);
}

/*output data to file*/
void out_dat(void) {
    int tt;

    // sum data across spectrum
    for (tt = 0; tt < ftpts; tt++) {
        // fprintf(fpto,"%f\n",(float)darray[tt]);	// write text version
        fwrite(&darray[tt], 4, 1, fptobin); // write 4-byte bin file
    }
    count = count + 1;
}

void ftorg_dat(void) {
    long int tt;
    float opp;

    for (tt = 0; tt < ftpts; tt++) {
        if (tt < (ftpts / 2)) {
            datr[tt + ftpts / 2] =
                dats[2 * tt] * dats[2 * tt] + dats[2 * tt + 1] * dats[2 * tt + 1];
        } else {
            datr[tt - ftpts / 2] =
                dats[2 * tt] * dats[2 * tt] + dats[2 * tt + 1] * dats[2 * tt + 1];
        }
    }
}

/*fast fourier transform routine*/
void four(double data[], int nn, int isign)
/* float data[];
 int nn,isign;*/
{
    long int n, mmax, m, j, istep, i, a;
    double wtemp, wr, wpr, wpi, wi, theta;
    double tempr, tempi;
    n = nn << 1;
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
        /* printf("%d    %d    %d    %d    %d\n",i,n,m,pts,istep);*/
    }
    if (isign == 1) {
        for (a = 0; a < 2 * nn; a++) {
            data[a] = data[a] / nn;
        }
    }
}
