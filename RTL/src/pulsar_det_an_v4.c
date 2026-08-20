/*pulsar_det_an.c  pwe: 02/04/2022
Corrections by Martin Ante Rogosic and Marko Radolovic
March - May 2026

Pulsar Detection and Analysis
Command Format: pulsar_det_an <data file> <N-point FFT> <data clock (ms))> <pulsar period (ms)> <No: sections><No.
bins><pulse width><DM><ppm><spike threshold> <ppm range factor><RF band (MHz)><RF Centre (MHz)><roll
average No.><start section><end section>

Takes N-point FFT of 4-byte .bin data file and splits data, compresses it into .txt files suitable
for MathCad,Excel and/or Python analysis.
*/

// Example command:
// airntmon1.bin 16 2 714.492518 128 1024 6.5 26.7 0.0 6 4 10 611 37 33 68

// gcc pulsar_det_an.c -o pulsardetan -lm -D_FILE_OFFSET_BITS=64
//./pulsardetan rag_obsm.bin 16 1 714.47415 128 1024 6.5 -26.7 -1.3 6 1 2.4 422 50 0 17

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include "../includes/io/files.h"
#include "../includes/io/outputs.h"

#include "../includes/numerics/DMSch.h"
#include "../includes/numerics/conv.h"
#include "../includes/numerics/is_pow_of_2.h"
#include "../includes/numerics/pdSch.h"
#include "../includes/numerics/perSch.h"
#include "../includes/numerics/psnr.h"
#include "../includes/numerics/spectrum.h"
#include "../includes/numerics/targgaus.h"

#define BYTES_PER_SAMPLE (sizeof(float))
// It appears there is some uncecessary back and forth between dfold and ddfold which could be
// optimized

// Blaf and Blas needs to be corrected with an appropriately calculated number of blanked
// bands

// Fix so that lack of Blaf and Blas thwros a warning, not crash the program as the
// semantics all the way down allow this

// TODO rewrite the array allocations so that they allocate continuous arrays therefore we get cache
// wins

//TODO refactor all function calls to accepts only doubles, we are done with floats

int main(int argc, char *argv[]) {
    FILE **files = open_files();
    FILE *fptr;

    /*check command line arguments*/
    if (argc != 17) {
        printf("Format: pulsar_det_an <data file> <N-point FFT> <data clock (ms))> <pulsar period "
               "(ms)> <No: sections><No. bins><pulse width><DM><ppm><spike threshold><ppm range "
               "factor><RF band (MHz)><RF Centre (MHz)><roll average No.><start section><end "
               "section>\n");
        exit(0);
    }

    if ((fptr = fopen(argv[1], "r")) == NULL) {
        printf("Can't open file %s. \n", argv[1]);
        exit(0);
    }

    if ((files[FPT_BLNKF] = fopen("Blankf.txt", "r")) == NULL) {
        printf("No Band Attenuation file %s. \n", "Blankf.txt");
        exit(0);
    }
    fclose(files[FPT_BLNKF]);

    if ((files[FPT_BLNKS] = fopen("Blanks.txt", "r")) == NULL) {
        printf("No Section Attenuation file %s. \n", "Blanks.txt");
        exit(0);
    }

    printf("Pulsar Data\n");
    printf("\nInput Data File = %s\n", argv[1]);

    // Read command line parameters
    const int N = atoi(argv[2]);          // number of FFT channels
    const double clck = atof(argv[3]);    // data sample interval in ms
    double period = atof(argv[4]);        // pulsar topocentric period
    const int M = atoi(argv[5]);          // number of compressed blocks/sections
    const int bins = atoi(argv[6]);       // number of output fold bins
    const double pulw = atof(argv[7]);    // pulsar pulse width
    double DM = atof(argv[8]);            // pulsar Dispersion Measure
    const double ppm = atof(argv[9]);     // period ppm adjustment
    double nno1 = atof(argv[11]);         // period ppm divisor
    const double rfband = atof(argv[12]); // rf bandwidth
    const double f0 = atof(argv[13]);     // rf centre
    const int rolav = atoi(argv[14]);     // number of rolling average sections
    const long int strt = atoi(argv[15]); // start number of wanted sections
    long int stp = atoi(argv[16]);        // end number of wanted sections
    const double thres = atof(argv[10]);  // spike threshold

    const int PTS = M * bins;
    const double inverse_N = 1.0 / (double)N;
    // some error correction
    if (N > 100) {
        printf("No: Bands < 101 \n");
        exit(0);
    }

    // These limitations can probably removed however what needs to be checked if the algoritm
    // allows for that
    // and these were here as an implementation limitation
    // TODO
    if (is_pow_of_2(M) != 1 || is_pow_of_2(bins) != 1 || (M * N) > 262144) {
        printf("No: Sections and No of bins should be a power of 2 and their product < 262145\n");
        exit(0);
    }
    if (stp > M) {
        printf("Section range out of limit");
        exit(0);
    }

    // Read attenuated frequency channels input data file
    // TODO generate couf and cous from the file and allocate appropriate elements in file
    int mf = 0, couf = 0;
    int blaf[256];
    files[FPT_BLNKF] = fopen("Blankf.txt", "r");
    while (fscanf(files[FPT_BLNKF], "%d", &mf) != EOF) {
        blaf[couf] = mf;
        couf += 1;
    }
    fclose(files[FPT_BLNKF]);
    printf("Blanked Bands: ");
    for (int d = 0; d < couf; d++) {
        printf("%d ", blaf[d]);
    }

    // Read attenuated sections input data file
    files[FPT_BLNKS] = fopen("Blanks.txt", "r");
    int cous = 0, ms = 0;
    int blas[256];
    while (fscanf(files[FPT_BLNKS], "%d", &ms) != EOF) {
        blas[cous] = ms;
        cous += 1;
    }
    fclose(files[FPT_BLNKS]);
    printf("\nBlanked Sections: ");
    for (int d = 0; d < cous; d++) {
        printf("%d ", blas[d]);
    }

    // Copes with lower rf sideband systems
    double dmp = 1.0;
    if (DM < 0.0)
        dmp = -1.0;

    DM = fabs(DM);

    // Allows for period ppm adjustment
    // period/p-dot search range is normally +-25ppm
    period = period * (1.0 + ppm / 1000000.0);

    // Parameter adjustments
    nno1 = 1 / nno1;           // period ppm multiplier
    const double dmdiv = 10.0; // DM range divider

    // Information print
    printf("\nThreshold = %.1f x rms \nPeriod Search Range Multiplier = %.2f\n", thres, nno1);

    // Command Line Input Information print
    printf("Pulsar ATNF Fold Period = %.8lf ms	\n", period);
    printf("Data clock= %.3f ms	\n", clck);
    printf("Pulsar ATNF Pulse Width = %.2lf ms	\n", pulw);
    printf("Pulsar ATNF Dispersion Measure = %.2lf	\n", DM);
    printf("DM Range Divider = %.1lf	\n", dmdiv);

    /*find length of input file*/
    fseeko(fptr, 0L, SEEK_END);
    const long int file_end = ftello(fptr) / BYTES_PER_SAMPLE;

    // print header
    printf("Input file bytes = %ld	\n", file_end);

    // adjust for 32=bit binary input data
    printf("No. Data Samples = %ld\n", file_end);

    // Expected dispersion delay across RF band
    const double td = 8.3 * rfband * DM * 1000000.0 / pow(f0, 3.0);
    printf("RF Centre = %.1f MHz,	RF Bandwidth = %.1f MHz\n", f0, rfband);
    printf("DM Band Delay = %.3fms, Equivalent No: of bins = %.3f\n", td, (td * bins / period));

    // Command settings output data file
    fprintf(files[FPT_DAT], "%d %d %d %d \n", N, M, bins, rolav);

    // Print key settings
    printf("No: FFT Bands = %d\n", N);
    printf("Input Data per FFT Band = %ld\n", file_end / N);

    // Duration of data collection (1ms or 2ms clock)
    const double Tt0 =
        clck * (double)file_end / (double)N / 1000.0; // clck im ms, N= number of FFT bins
    printf("Data Collection Interval = %.1f secs\n", Tt0);

    // File compresion to optimise good data to maximise SNR

    stp = stp + 1;
    const double nopers =
        (double)(Tt0 * 1000.0 /
                 period); // No. of periods in data file, 1000 factor as period input in ms
    printf("No: of Pulsar Periods = %.2f\n", nopers);

    const double per_sampls = period * N / clck; // number of data
                                                 // samples in period
    const double sect_sampls =
        Tt0 * 1000.0 * (double)N / (double)M / clck; // number of data samples in section
    printf("No: of useful periods = %d\n", (int)nopers);
    double ratio = sect_sampls / per_sampls; // data compression ratio

    // Start and end section for reducing analysis section range
    long int start = ((long int)(strt * ratio + 0.5) * per_sampls / N) * BYTES_PER_SAMPLE * N;
    long int end = ((long int)(stp * ratio + 0.5) * per_sampls / N) * BYTES_PER_SAMPLE * N;
    // No. of compressed pulsar data points
    const long int nmax = (end - start) / BYTES_PER_SAMPLE / N / M;

    printf("ratio = %f\n", ratio);
    printf("Start Byte = %ld\n", start);
    printf("Start Section Period  = %f\n",
           (double)start * clck / (double)N / period / BYTES_PER_SAMPLE);
    printf("End Byte = %ld\n", end);
    printf("End Section Period Number = %f\n",
           (double)end * clck / (double)N / period / BYTES_PER_SAMPLE);

    // set file pointer to start
    fseeko(fptr, start, SEEK_SET);
    start = start / BYTES_PER_SAMPLE; // 4-byte data word
    end = end / BYTES_PER_SAMPLE;     // 4-byte data word
    printf("Start Section = %ld	Stop Section = %ld\n", strt, stp - 1);
    printf("No: of Output Fold Sections  %d; No. Fold Bins = %d\n", M, bins);
    printf("No: of Output Data Bins = %d\n", M * bins);
    printf(
        "No: of Output samples/block = %ld\n",
        nmax); // Data is divided into M blocks and nmax is the number of data samples in each block

    // Duration of new data file
    const double Tt = clck * (double)(end - start) / (double)N;
    printf("Working file duration = %.0f secs\n", Tt / 1000);
    double numper = (int)(Tt / period); // number of periods in file

    // Calculating the p-dot exponent
    // const double numlog = log10(numper);
    const double numlog = (int)(pow(10.0, (int)log10(numper))); // p-dot log exponent
    printf("P-dot exponent = %d\n",
           (int)numlog); // chosen to produce a -45 degree slope in 2D period/P-dot plot

    // Drift at file end due to ppm adjustment setting
    const double ppmdrift = ppm * Tt / 1000000.0;
    printf("ppm adjustment = %.2lf\n", ppm);
    printf("Max Pulse ppm drift = %.2lf ms\n", ppmdrift);

    // Rolling average input number
    printf("Rolling Average Number = %d\n", rolav);

    // Calculate target pulseconvolution for gaussian shaped pulse

    double *targ = calloc(2 * PTS, sizeof(double));
    targgaus(pulw, PTS, targ, period, M);

    // Data Text Record
    fprintf(files[FPT_TEXT], "Pulsar Data	\n");
    fprintf(files[FPT_TEXT], "Input Data file: = %s\n", argv[1]);
    fprintf(files[FPT_TEXT], "Threshold = %.1lf x rms	Period Range Multiplier = %.2lf\n", thres,
            1.0 / nno1);
    fprintf(files[FPT_TEXT], "Pulsar Period = %.6f ms	\n", period);
    fprintf(files[FPT_TEXT], "Data clock= %.2f ms \n", clck);
    fprintf(files[FPT_TEXT], "Pulsar Pulse Width = %.2f ms	\n", pulw);
    fprintf(files[FPT_TEXT], "Pulsar Dispersion Measure = %.2f	\n", DM);
    fprintf(files[FPT_TEXT], "Input file bytes = %ld	\n", file_end * BYTES_PER_SAMPLE);
    fprintf(files[FPT_TEXT], "RF Centre = %.1f MHz,	RF Bandwidth = %.1f MHz\n", f0, rfband);
    fprintf(files[FPT_TEXT], "DM Band Delay = %.3fms, No. Delay bins = %.3f\n", td,
            (td * bins / period));
    fprintf(files[FPT_TEXT], "No. Data Samples = %ld\n", file_end);
    fprintf(files[FPT_TEXT], "No: FFT Bands = %d\n", N);
    fprintf(files[FPT_TEXT], "Input Data per FFT Band = %ld\n", file_end / N);
    fprintf(files[FPT_TEXT], "No. of Output Fold Sections = %d ; No. bins = %d\n", M, bins);
    fprintf(files[FPT_TEXT], "No. of Output samples = %d\n", M * bins);
    fprintf(files[FPT_TEXT], "Working file duration = %.0f secs\n", Tt / 1000);
    fprintf(files[FPT_TEXT], "Number of pulsar periods = %.0f\n", numper);
    fprintf(files[FPT_TEXT], "P-dot factor = %.0f\n", numlog);
    fprintf(files[FPT_TEXT], "ppm adjustment = %.2f\n", ppm);
    fprintf(files[FPT_TEXT], "Max Pulse ppm drift = %.2f ms\n", ppmdrift);
    fprintf(files[FPT_TEXT], "Compression Ratio = %.2f	\n", ratio);
    fprintf(files[FPT_TEXT], "Period Search Range = %.2lf ppm to %.2lf ppm \n", -25.0 / nno1,
            25.0 / nno1);
    fprintf(files[FPT_TEXT], "P-dot Search Range = %.2lf ppm/%d to %.2lf ppm/%d \n",
            -25.0 * 2.0 * numlog / nno1 / numper, (int)numlog, 25.0 * 2.0 * numlog / nno1 / numper,
            (int)numlog);
    fprintf(files[FPT_TEXT], "Rolling Average Number = %d\n", rolav);
    fprintf(files[FPT_TEXT], "Start section = %ld\n", strt);
    fprintf(files[FPT_TEXT], "End section = %ld\n", stp - 1);

    // Serially fold data into M sections -
    // http://www.y1pwe.co.uk/RAProgs/LowSNRCorrelationSearch.pdf
    double *comprc = calloc(PTS, sizeof(double));
    const float *buffer = (float *)malloc(nmax * M * N * sizeof(float));
    size_t nread = fread((void *)buffer, sizeof(float), nmax * M * N, fptr);
    // fprintf(stderr, "requested=%ld read=%zu\n", nmax * M * N, nread);
    fwrite(buffer, sizeof(float), nmax * M * N, files[FPT_CUT]);

    // Invert this allocation here as it is making thing uncecessarily hard
    double **comprs = malloc(PTS * sizeof(double *));
    for (int i = 0; i < PTS; i++) {
        comprs[i] = calloc(N, sizeof(double));
    }
    for (long int aux = 0; aux < (nmax * M); aux++) { // nmax equals the number of data sets

        // the current running theory is that the channel is divided into M baskets
        // aux is the index of an element in the channel
        // nmax is the number of element in one basket
        // M is the number of baskets in one channel
        // xs is the index of each basket
        // b is the number of full pulsar periods up to index aux
        // the double long int chaos is to get the decimal part without the whole part e.g. 0.12345
        // sm has to be something with the phase of the signal...

        const long int xs = aux / nmax;
        const double b = aux * clck / period;
        const long int sm = (long int)(((double)b - (double)((long int)b)) * (double)bins);
        const int mval = sm + xs * bins;
        double *array = comprs[mval];
        long int idx = aux * N;
        // Read input data FFT blocks
        for (int num = 0; num < N; num++) {                     // N = number of FFT channels
            array[num] = array[num] + (buffer[idx + num] * 10); // fold into bins
        }
        comprc[mval] = comprc[mval] + 1; // count bin entries
    }
    free((void *)buffer);

    // I muched arround here
    // const long int cnt = nmax * M;
    printf("\n Count= %ld\n", nmax * M);
    printf("New Compression Ratio = %f \n", ratio);
    // Confirm number of periods folded etc:
    numper = ((double)(nmax * M * clck) / period);
    ratio = (double)numper / (double)M;
    printf("New Compression Ratio = %f \n", ratio);
    printf("Period Search Range = %.2lf ppm to %.2lf ppm \n", -25.0 / nno1, 25.0 / nno1);
    printf("P-dot Search Range = %.2lf ppm/%d to %.2lf ppm/%d \n",
           -25.0 * 2.0 * numlog / nno1 / numper, (int)numlog, 25.0 * 2.0 * numlog / nno1 / numper,
           (int)numlog);

    // Check no count entries are zero
    for (int bi = 0; bi < PTS; bi += 1) {
        comprc[bi] += (comprc[bi] == 0);
        // The above is the same as the lines below
        /*if (comprc[num][bi] == 0)
            comprc[num][bi] = 1.0;*/
    }

    // Calculate M section folded FFT channel data and section mean and rms;  M = number of
    // sections;
    double **compval = (double **)malloc(N * sizeof(double *));
    double **ave = (double **)malloc(N * sizeof(double *));
    double **rms = (double **)malloc(N * sizeof(double *));

    for (int i = 0; i < N; i++) {
        compval[i] = malloc(PTS * sizeof(double));
        ave[i] = calloc(M, sizeof(double));
        rms[i] = calloc(M, sizeof(double));
    }

    for (int num = 0; num < N; num++) {         // N = number of FFT channels
        for (long int c = 0; c < PTS; c += 1) { // M = number of sections
            const int Mc = c / bins;
            compval[num][c] = comprs[c][num] / comprc[c]; // normalise raw partially folded data
            ave[num][Mc] = ave[num][Mc] + compval[num][c] / bins;           // section average
            rms[num][Mc] = rms[num][Mc] + pow(compval[num][c], 2.0) / bins; // section squared rms
        }
    }
    free(comprc);

    for (int i = 0; i < PTS; i++) {
        free(comprs[i]);
    }
    free(comprs);

    // DC restore sections - removes long-term  receiver gain drift
    for (int num = 0; num < N; num++) {      // number of FFT channels
        for (long int c = 0; c < PTS; c++) { // M = number of sections; bins = number of fold bins
            const int Mc = c / bins;
            compval[num][c] = compval[num][c] - ave[num][Mc]; // dc restored section fold data
        }
    }

    // Calculate DC restored mean and rms for each frequency band
    double *std = calloc(N, sizeof(double));
    double *freq = calloc(N, sizeof(double));
    double *mean = calloc(N, sizeof(double));
    for (int num = 0; num < N; num += 1) {
        for (int m = 0; m < M; m += 1) {
            mean[num] = mean[num] + ave[num][m] / M; // dc restored frequency channel average
            std[num] = std[num] + rms[num][m] / M;   // dc restored frequency channel rms
        }
        std[num] = sqrt(std[num] - pow(mean[num], 2.0));
        freq[num] = mean[num]; // average channel frequency response
    }
    // Free ave and rms
    for (int i = 0; i < N; i++) {
        free(ave[i]);
        free(rms[i]);
    }
    free(ave);
    free(rms);

    // Print Band mean and standard deviation values
    printf("\n Band Mean \n");
    for (int num = 0; num < N; num++) {
        printf("	%.1lf", mean[num]);
        fprintf(files[FPT_FREQ], "%lf\n",
                freq[num]); /* write frequency band mean data to the output text file */
    }
    printf("\n");
    printf("\n Standard Deviation \n");
    for (int num = 0; num < N; num += 1) {
        printf("	%.1lf", std[num]);
    }
    printf("\n");
    free(std);
    free(freq);
    free(mean);
    // Build section folded, DC restored raw text data file
    for (int c = 0; c < PTS; c++) {
        for (int num = 0; num < N; num++) {
            fprintf(files[FPT_RAW], "	%lf",
                    (compval[num][c])); /* write txt raw data to the output text file */
        }
        fprintf(files[FPT_RAW], "\n");
    }
    double *ftdat = malloc(PTS * sizeof(double));
    // This pdat here serves to store temporary results in the conv and spectrum functions.
    // Ideally it could bee placed with in them
    double *pdat = malloc(2 * PTS * sizeof(double));
    double *fftdat = malloc(PTS * sizeof(double));
    double *ftdat2 = malloc((PTS / 2) * sizeof(double));
    double *spallbands = calloc(PTS, sizeof(double));

    // Matched-filter data bandwidth to just pass pulsar pulse - FFT data and Convolve bands
    for (int num = 0; num < N; num++) { // printf("	M1=%d	N=%d	bins=%d\n",M,N,bins);
        memcpy(ftdat, compval[num], PTS * sizeof(double));
        conv(ftdat, pulw, PTS, period, M, pdat, targ,
             fftdat); // outputs ftdat input blocks asconvolved and filtered fftdat blocks
        spectrum(fftdat, PTS, pdat, ftdat2); // outputs fftdat input block spectra as ftdat2
        double *cv = compval[num];
        for (int c = 0; c < PTS; c++) {
            cv[c] = fftdat[c];
        }

        for (int c = 0; c < PTS / 2; c++) {
            spallbands[c] += ftdat2[c];
        }
    }
    free(ftdat);
    free(pdat);
    free(fftdat);
    free(ftdat2);
    free(targ);

    // Calculate FFT channel data and section match-filtered mean and mean square
    double **av = (double **)malloc(N * sizeof(double *));
    double **rm = (double **)malloc(N * sizeof(double *));
    for (int i = 0; i < N; i++) {
        av[i] = calloc(M, sizeof(double));
        rm[i] = calloc(M, sizeof(double));
    }

    for (int c = 0; c < PTS; c++) {
        const int Mc = c / bins;
        for (int num = 0; num < N; num++) {
            av[num][Mc] = av[num][Mc] + compval[num][c] / bins;
            rm[num][Mc] = rm[num][Mc] + pow(compval[num][c], 2.0) / bins;
        }
    }

    for (int m = 0; m < M; m++) {
        for (int num = 0; num < N; num++) {
            rm[num][m] = sqrt(rm[num][m] - pow(av[num][m], 2.0)); // now true rms
        }
    }

    // Print Post DC correction and filtering mid-section, band rms, mean
    printf("\n Post DC correction and filtering band rms, mean \n");
    for (int num = 0; num < N; num++) {
        printf("	%.2f, %.2f", (double)rm[num][M / 2], (double)av[num][M / 2]);
    }
    printf("\n");

    // Final DC restore
    for (int c = 0; c < PTS; c++) {
        const int Mc = c / bins;
        for (int num = 0; num < N; num++) {
            compval[num][c] = compval[num][c] - av[num][Mc];
        }
    }

    // Limit peaks - needed to ensure compressed file is not degraded by possible section RFI spikes
    // limits section band if greater than X standard deviations

    // There is an absolute value hack here that can be applied and therefore this loop can be
    // reduced to a single if
    // TODO
    double X = thres;
    for (int num = 0; num < N; num++) {
        for (int c = 0; c < PTS; c++) {
            const int Mc = c / bins;
            if (compval[num][c] > (X * rm[num][Mc])) {
                compval[num][c] = X * rm[num][Mc];
            }
            if (-compval[num][c] > (X * rm[num][Mc])) {
                compval[num][c] = -X * rm[num][Mc];
            }
        }
    }

    for (int i = 0; i < N; i++) {
        free(av[i]);
        free(rm[i]);
    }
    free(av);
    free(rm);

    printf("threshold = %.1f \n", X);

    // Build outdat.txt - compressed, match-filtered, 16-channel data text file
    for (int c = 0; c < PTS; c++) {
        for (int num = 0; num < N; num++) {
            fprintf(files[FPT_OUT], "	%lf",
                    compval[num][c]); /* write txt data to the output text file */
        }
        fprintf(files[FPT_OUT], "\n");
    }

    // Attenuate Blankf.txt frequency channels
    if (couf >= 1) {
        for (int c = 0; c < PTS; c++) {
            for (int d = 0; d < couf; d++) {
                compval[blaf[d]][c] = compval[blaf[d]][c] / 100.0;
            }
        }
    }

    // Attenuate Blanks.txt sections
    if (cous >= 0) {
        for (int d = 0; d < cous; d++) {
            for (int num = 0; num < N; num++) {
                for (int c = 0; c < PTS; c++) {
                    const int Mc = c / bins;
                    if (Mc == blas[d]) {
                        compval[num][c] = compval[num][c] / 100.0;
                    }
                }
            }
        }
    }

    // De-disperse recorded data based on command line DM value - de-dispered relative to median
    // frequency
    const double delta =
        dmp * td * bins * (N - 1) / (double)N / period / (double)N; // time delay/sub-band

    double **compvald = (double **)malloc(N * sizeof(double *));
    double **compvaldd = (double **)malloc(N * sizeof(double *));
    for (int i = 0; i < N; i++) {
        compvald[i] = calloc(PTS, sizeof(double));
        compvaldd[i] = calloc(PTS, sizeof(double));
    }
    printf("De-disperse Time Delay/sub-band = %.3lf ms\n", delta);
    for (int num = 0; num < N; num++) {
        for (int c = 0; c < PTS; c++) {
            compvaldd[num][c] =
                compval[num][(c + (int)((num - ((N - 1) / 2.0)) * delta) + PTS) % PTS];
        }
    }

    // SNR per Band - Build bandS.txt
    int mbin = 0;
    double mmx = 0;
    double *dfold = calloc(bins, sizeof(double));
    double *outdat = calloc(bins, sizeof(double));
    printf("\n SNR per Band\n");
    for (int num = 0; num < N; num++) {
        memset(dfold, 0, bins * sizeof(double));
        // This loop can probablly be collasped into a single loop
        // TODO
        for (int s = 0; s < M; s++) {
            for (int d = 0; d < bins; d += 1) {
                dfold[d] = dfold[d] + compval[num][s * bins + d];
            }
        }
        psnrReturn datout;
        psnr(bins, dfold, mbin, &datout, pulw, outdat);
        if (datout.std_snr > mmx) {
            mmx = datout.std_snr;
        }
        printf("Band = %d 	SNR =  %.2f	bin = %d\n", num, datout.std_snr, (int)datout.nx);
        fprintf(files[FPT_BND], "%d	%.2f	%d\n", num, datout.std_snr,
                (int)datout.nx); /* write band number SNR and max bin to bandS.txt ext file */
                                 // Build De-dispersed bandat.txt band folds
        for (int d = 0; d < bins; d++) {
            fprintf(files[FPT_BNDD], "	%.3lf",
                    outdat[d]); /* write band fold SNR data to the bandat.txt file */
        }
        fprintf(files[FPT_BNDD], "\n");
    }
    printf("\n");
    double *allbandsdd = calloc(PTS, sizeof(double));
    double *allbands = calloc(PTS, sizeof(double));
    // Make allbands files for pre and DM value dispersed
    for (int c = 0; c < PTS; c++) {
        for (int num = 0; num < N; num++) {
            allbands[c] = allbands[c] + compval[num][c];       // combine bands pre-dispersed
            allbandsdd[c] = allbandsdd[c] + compvaldd[num][c]; // combine bands DM value dispersed
        }
    }

    // De-dispersing search Routine
    // De-Disperse Bands - build dispersing matrix about band centre
    int emax = 0, dmx = 101,
        dmn = 50; // max and min of e range; dmp is DM polarity; dmdiv is range divider

    // Optimizaiton required here, most cache misses happen here
    const int double_PTS = 2 * PTS;
    const int half_N = N / 2;
    const double inverse_dmdiv = 1.0 / dmdiv;

    double **ddispbands = (double **)malloc(dmx * sizeof(double *));
    for (int i = 0; i < dmx; i++) {
        ddispbands[i] = calloc(PTS, sizeof(double));
    }
    for (int num = -half_N; num < half_N; num++) {

        const long int idx = num + half_N;

        for (int e = 0; e < dmx; e++) {

            const int pre_compute = double_PTS + (int)(((double)num + 0.5) * dmp *
                                                       ((double)e - (double)dmn) * inverse_dmdiv);

            const int shift = pre_compute % PTS;
            const int split = PTS - shift;

            for (int c = 0; c < split; c++) {
                ddispbands[e][c] += compval[idx][c + shift] * inverse_N;
            }

            for (int c = split; c < PTS; c++) {
                ddispbands[e][c] += compval[idx][c + shift - PTS] * inverse_N;
            }
        }
    }
    // Dispersion Search
    double **ddfold = (double **)malloc(dmx * sizeof(double *));
    for (int i = 0; i < dmx; i++) {
        ddfold[i] = calloc(bins, sizeof(double));
    }

    for (int e = 0; e < dmx; e += 1) {
        for (int c = 0; c < M; c += 1) {
            for (int d = 0; d < bins; d += 1) {
                ddfold[e][d] = ddfold[e][d] + ddispbands[e][(c * bins + d) % PTS];
            }
        }
    } // End dispersion search

    for (int i = 0; i < dmx; i++) {
        free(ddispbands[i]);
    }
    free(ddispbands);
    // Build dmSearch.txt - Dispersion Search SNR text file
    double dmsrch;
    mmx = 0;
    double *bstdmprf = calloc(bins, sizeof(double));
    printf("\n Dispersion Search \n");
    for (int e = 0; e < dmx; e += 1) {
        memcpy(dfold, ddfold[e], bins * sizeof(double));
        psnrReturn datout;
        psnr(bins, dfold, mbin, &datout, pulw, outdat);
        if (datout.std_snr > mmx) {
            mmx = datout.std_snr;
            mbin = datout.nx;
            emax = e;
            memcpy(bstdmprf, outdat, bins * sizeof(double));
        }
        const double dm = (((double)N * (e - dmn) / dmdiv)) * (period / bins) * (DM / td);
        dmsrch = DMSch(dm, N, DM, td, pulw);
        printf("DM =	%.2lf	SNR =  %.2lf	bin = %d	e = %d	dmsrch = %.2lf\n",
               (double)((double)(N) * ((double)e - dmn) / dmdiv) * (period / (double)bins) *
                   (DM / td),
               datout.std_snr, (int)datout.nx, (int)((e - dmn) * dmdiv), dmsrch);
        fprintf(files[FPT_DMS], "%.2lf	%.2lf	%d	%d	%.2lf\n",
                (double)((double)(N) * ((double)e - dmn) / dmdiv) * (period / (double)bins) *
                    (DM / td),
                datout.std_snr, (int)datout.nx, (int)((e - dmn) * dmdiv),
                dmsrch); /* write txt data to the output text file */

        for (int d = 0; d < bins; d += 1) {
            fprintf(files[FPT_DMFOLD], "%lf	", outdat[d]);
        }
        fprintf(files[FPT_DMFOLD], "\n");
    } // End dispersion search SNR

    // Build dmprf.txt - DM peak best fold text file
    for (int d = 0; d < bins; d += 1) {
        fprintf(files[FPT_DMPRF], "	%.2lf", bstdmprf[d]);
    }
    fprintf(files[FPT_DMPRF], "\n");
    printf("\n");

    printf("Max bin = %d	Best DM SNR = %.2f	emax = %d\n", (int)(mbin), bstdmprf[(int)mbin],
           emax);
    fprintf(files[FPT_MAX], "%d	%.2lf	%.2f	%.2lf\n", (int)(mbin), (double)mmx,
            ((double)((double)N * ((double)emax - (double)dmn) * period / (double)dmdiv)) /
                (double)bins,
            td);
    fprintf(files[FPT_TEXT], "Max bin = %d	Max SNR = %.2lf\n", (int)(mbin),
            (double)bstdmprf[(int)mbin]);

    // Build dmSrchns.txt - Dispersion Search SNR - with target pulse range blanked
    for (int e = 0; e < dmx; e += 1) {
        memcpy(dfold, ddfold[e], bins * sizeof(double));
        for (int d = 0; d < bins; d += 1) {
            if (d > mbin - 8 * pulw && d < mbin + 8 * pulw)
                dfold[d] = 0; // 8 for large signalsmbin+4*pulw//30 small
        }
        psnrReturn datout;
        psnr(bins, dfold, mbin, &datout, pulw, outdat);
        fprintf(files[FPT_DMNS], "%.2lf	%.2lf	%d\n",
                (double)((double)(N) * ((double)e - dmn) / dmdiv) * (period / (double)bins) *
                    (DM / td),
                datout.std_snr, (int)datout.nx); /* write txt data to the output text file */
    }
    printf("\n");
    for (int num = -half_N; num < half_N; num++) {
        for (int c = 0; c < PTS; c++) {
            compvald[num + half_N][c] =
                compval[num + half_N]
                       [(c + PTS + (int)((num + 0.5) * dmp * ((double)emax - dmn) / dmdiv)) %
                        PTS]; //*20/mean[num+N/2];
        }
    }

    for (int i = 0; i < N; i++) {
        free(compval[i]);
    }
    free(compval);

    // free ddfold
    for (int i = 0; i < dmx; i++) {
        free(ddfold[i]);
    }
    free(ddfold);

    // Build allbandsd- optimum after dispersion search
    double *allbandsd = calloc(PTS, sizeof(double));
    for (int c = 0; c < PTS; c += 1) {
        for (int num = 0; num < N; num += 1) {
            allbandsd[c] = allbandsd[c] + compvald[num][c];
        }
    }

    // Band/frequency channel Search
    printf("\n Band Search \n");
    double *sumt = calloc(bins, sizeof(double));
    double *count = calloc(bins, sizeof(double));
    // From where did we get this magical constant
    // They have to do with the range of values we search over
    // TODO
    double **outsumt = malloc(MAX(51, N) * sizeof(double *));
    for (int i = 0; i < MAX(51, N); i++) {
        outsumt[i] = calloc(bins, sizeof(double));
    }
    for (int num = 0; num < N; num += 1) {
        memset(sumt, 0, bins * sizeof(double));
        for (int s = 0; s < bins; s++) {
            count[s] = 1;
        }
        // Band folds
        // We currently heed to make this run trough more predictable
        for (int ss = 0; ss < PTS; ss++) {
            // const int range = ss % bins;
            // the bellow is the same as the above
            const int range = ss & (bins - 1);
            sumt[range] = sumt[range] + compvald[num][ss];
            count[range]++;
        }
        for (int s = 0; s < bins; s++) {
            outsumt[num][s] = 100 * sumt[s] / count[s];
        }
    }

    for (int i = 0; i < N; i++) {
        free(compvald[i]);
        free(compvaldd[i]);
    }
    free(compvald);
    free(compvaldd);
    // Cumulative Band SNR - Build cumbands.txt

    printf(" Cumulative Band SNR\n");
    memset(dfold, 0, bins * sizeof(double));
    // TOOO:
    double bndcum[N][bins];
    for (int num = 0; num < N; num++) {
        for (int d = 0; d < bins; d++) {
            dfold[d] = dfold[d] + outsumt[num][d];
        }
        psnrReturn datout;
        psnr(bins, dfold, mbin, &datout, pulw, outdat);
        memcpy(bndcum[num], outdat, bins * sizeof(double));
        printf("Band = %d 	Cum SNR =  %.2f	bin = %d\n", num, datout.std_snr, (int)datout.nx);
        fprintf(files[FPT_CUMF], "%d	%.2f	%d\n", num, datout.std_snr,
                (int)datout.nx); /* write cumulative band data to the cumbands.txt file */
    }
    printf("\n");

    // Build cumulative Band folds
    for (int d = 0; d < bins; d++) {
        for (int num = 0; num < N; num++) {
            fprintf(files[FPT_BNDC], "	%.2lf", bndcum[num][d]);
        }
        fprintf(files[FPT_BNDC], "\n");
    }

    // SNR All Bands
    double maxx = 0;
    printf(" SNR All Bands\n");
    memset(dfold, 0, bins * sizeof(double));

    for (int d = 0; d < bins; d += 1) {
        for (int num = 0; num < N; num += 1) {
            dfold[d] = dfold[d] + outsumt[num][d];
        }
    }
    psnrReturn datout;
    psnr(bins, dfold, mbin, &datout, pulw, outdat);
    maxx = datout.std_snr;
    printf("Bands = %d 	BSNR =  %.2f	bin = %d\n", N, datout.std_snr, (int)datout.nx);
    printf("\n");

    // Section SNR. Build puldat.txt (section folds) and secsnr.txt (section SNR)
    double thry;
    for (int m = 0; m < M; m++) {
        memset(outdat, 0, bins * sizeof(double));
        memcpy(dfold, allbands + m * bins, bins * sizeof(double));
        // We need to think about whether we can add a local variable here, this way datout prevents
        // paralelization
        psnr(bins, dfold, mbin, &datout, pulw, outdat);
        for (int d = 0; d < bins; d++) {
            fprintf(files[FPT_PULD], "	%.2f", outdat[d]);
        }
        fprintf(files[FPT_PULD], "\n");
        thry = maxx * sqrt((double)m / (double)M);
        fprintf(files[FPT_SEC], "%d	%.2lf	%d	%.2lf\n", m, datout.std_snr, (int)datout.nx,
                (double)thry); /* write txt data to the output text file */
        psnr(bins, dfold, mbin, &datout, pulw, outdat);
        for (int d = 0; d < bins; d++) {
            fprintf(files[FPT_PULDD], "  %.2f",
                    outdat[d]); /* write dedispersed puldat data to the output text file */
        }
        fprintf(files[FPT_PULDD], "\n");
    }

    // Cumulative Section SNR - Build cumsec.txt
    memset(dfold, 0, bins * sizeof(double));
    for (int m = 0; m < M; m++) {
        for (int d = 0; d < bins; d++) {
            dfold[d] = dfold[d] + allbands[d + m * bins];
        }
        psnr(bins, dfold, mbin, &datout, pulw, outdat);
        for (int d = 0; d < bins; d++) {
            fprintf(files[FPT_FOLD], "%.2f  ", outdat[d]);
        }
        fprintf(files[FPT_FOLD], "\n");
        fprintf(files[FPT_CUMS], "%d	%.2f	%d\n", m, datout.std_snr,
                (int)datout.nx); /* write cumulative band SNR txt data to the output text file */
        psnr(bins, dfold, mbin, &datout, pulw, outdat);
        for (int d = 0; d < bins; d += 1) {
            fprintf(files[FPT_FOLDD], "%.2f  ", outdat[d]); // dedispersed foldat
        }
        fprintf(files[FPT_FOLDD], "\n");
        psnr(bins, dfold, mbin, &datout, pulw, outdat);
        for (int d = 0; d < bins; d++) {
            fprintf(files[FPT_FOLDDD], "%.2f  ",
                    outdat[d]); // fprintf(fptfoldd,"\n"); // dedispersed foldat
        }
        fprintf(files[FPT_FOLDDD], "\n");
    }

    // Period Search Folds
    for (int st = 0; st < 51; st++) {
        const double pperiod = bins * (1 + (st - 25) * 1 * ratio / 1000000 / nno1);
        memset(sumt, 0, bins * sizeof(double));
        for (int s = 0; s < bins; s++) {
            count[s] = 1;
        }
        for (int ss = 0; ss < M * bins; ss++) {
            const long int range =
                (long long int)(((double)ss / ((double)pperiod) -
                                 (long long int)((double)ss / ((double)pperiod))) *
                                ((double)bins));
            sumt[range] = sumt[range] + allbands[ss];
            count[range] = count[range] + 1;
        }
        for (int s = 0; s < bins; s++) {
            outsumt[st][s] = 100 * sumt[s] / count[s];
        }
    }

    // Period search SNR. Build periodS.txt **All period p-dot search routines prefer un
    // de-dispersed data
    double pspr;
    maxx = 0;
    printf("\n Period Search, SNR v ppm change \n");
    for (int st = 0; st < 51; st += 1) {
        memcpy(dfold, outsumt[st], bins * sizeof(double));
        psnr(bins, dfold, mbin, &datout, pulw, outdat);
        if (datout.std_snr > maxx)
            maxx = datout.std_snr;
    }
    for (int st = 0; st < 51; st += 1) {
        memcpy(dfold, outsumt[st], bins * sizeof(double));
        psnr(bins, dfold, mbin, &datout, pulw, outdat);
        pspr = 3 + (maxx - 3) * perSch((double)(st - 25) * 1 / nno1, (numper), pulw, period);
        printf("ppm = %.2lf	SNR =  %.2lf	bin = %d\n", (double)(st - 25) / nno1, datout.std_snr,
               (int)datout.nx);
        fprintf(files[FPT_PER], " %.5lf	%.5lf	%d	%.3lf\n", (double)(st - 25) / nno1,
                datout.std_snr, (int)datout.nx, pspr); /* write txt data to the output text file */
        if (st > 19 && st < 31) {
            for (int d = 0; d < bins; d += 1) {
                fprintf(files[FPT_PFOLD], "%lf	",
                        (double)outdat[d]); // target period folds over the selected period range
            }
            fprintf(files[FPT_PFOLD], "\n");
        }
    }
    printf("\n");

    // P-dot Search Fold
    printf("\n Period Rate Search, SNR v ppm/%d change \n", (int)numlog);
    double periodt;
    for (int st = 0; st < 51; st++) {
        const double pperiod = bins;
        memset(sumt, 0, bins * sizeof(double));
        for (int s = 0; s < bins; s++) {
            count[s] = 1;
        }
        for (int ss = 0; ss < M * bins; ss++) {
            periodt =
                pperiod *
                (1 + (double)(st - 25) * 1 *
                         (double)((ss * ratio / (double)(M * bins)) / (double)1000000 / nno1));
            const long int range =
                (long long int)(((double)ss / ((double)periodt) -
                                 (long long int)((double)ss / ((double)periodt))) *
                                ((double)bins));
            sumt[range] = sumt[range] + allbands[ss];
            count[range] = count[range] + 1;
        }

        for (int s = 0; s < bins; s++) {
            outsumt[st][s] = 100 * sumt[s] / count[s];
        }
    }

    // Pdot search SNR.  Build pdotS.txt
    double pdpr;
    for (int st = 0; st < 51; st += 1) {
        memcpy(dfold, outsumt[st], bins * sizeof(double));
        psnr(bins, dfold, mbin, &datout, pulw, outdat);
        const double pre_compute = (double)(st - 25) * 2 * numlog * .5 / (double)M / ratio / nno1;
        pdpr = 3 + (maxx - 3) * pdSch(pre_compute, (numper), pulw, period, numlog);
        printf("pdot = %.2lf	SNR =  %.2lf	bin = %d\n",
               (double)(st - 25) * 2 * numlog / (double)M / ratio / 1 / nno1, datout.std_snr,
               (int)datout.nx);
        fprintf(files[FPT_PD], " %.5lf	%.5lf	%d	%.5lf\n",
                (double)(st - 25) * 2 * numlog / (double)M / ratio / 1 / nno1, datout.std_snr,
                (int)datout.nx, pdpr); /* write txt data to the output text file */
    }
    printf("\n");

    // Period / P-dot Search Fold. Build ppd2d.txt
    for (int sp = 0; sp < 51; sp += 1) { // p-dot range

        const double pperiod = bins * (1 + (sp - 25.0) * ratio / 1000000.0 / nno1);

        for (int st = 0; st < 51; st += 1) { // period range

            memset(sumt, 0, bins * sizeof(double));
            memset(count, 0, bins * sizeof(double));
            for (int ss = 0; ss < PTS; ss++) {
                periodt = pperiod * (1 + (double)(st - 25) * (double)((ss * ratio / (double)(PTS)) /
                                                                      1000000.0 / nno1));
                const long int range =
                    (long long int)(((double)ss / ((double)periodt) -
                                     (long long int)((double)ss / ((double)periodt))) *
                                    ((double)bins));
                sumt[range] = sumt[range] + allbandsd[ss];
                count[range]++;
            }
            for (int s = 0; s < bins; s++) {
                dfold[s] = 100 * sumt[s] / count[s];
            }
            psnr(bins, dfold, mbin, &datout, pulw, outdat);
            fprintf(files[FPT_PPD], "%.2f	 ",
                    datout.std_snr); /* write txt data to the output text file */
        }
        fprintf(files[FPT_PPD], "\n");
    }
    free(sumt);
    free(count);
    // Build secavsnr.txt - Rolling Window/Average SNR
    int nxx = 0;
    double max = 0, pkmax = 0;
    double bestprof[bins];
    int span = 0, centre = 0;
    double pkdat[bins];
    for (int mp = 1; mp < M + 1; mp += 1) {
        if (mp == rolav)
            printf("Set Section Rolling Average Window %d \n", mp);
        max = 0;

        for (int m = 0; m < M - mp + 1; m += 1) {
            memset(dfold, 0, bins * sizeof(double));
            for (int xx = 0; xx < mp + 0; xx += 1) {
                for (int d = 0; d < bins; d += 1) {
                    dfold[d] = dfold[d] + allbandsd[d + (m + xx) * bins];
                }
            }
            psnr(bins, dfold, mbin, &datout, pulw, outdat);
            if (outdat[(int)datout.nx] > pkmax) {
                pkmax = outdat[(int)datout.nx];
                centre = m + mp / 2;
                span = mp;

                memcpy(pkdat, outdat, bins * sizeof(double));
            }
            if (datout.std_snr > max) {
                max = datout.std_snr;

                nxx = datout.nx;
                if (mp == rolav) {
                    memcpy(bestprof, outdat, bins * sizeof(double));
                }
            }
            if (mp == rolav) {
                fprintf(files[FPT_AVSEC], "%d	%.2f	%d 	%.2f\n", m + (mp / 2), datout.std_snr,
                        (int)datout.nx,
                        datout.unknown_var); /* write txt data to the output text file */
            }
            for (int d = 0; d < bins; d += 1) {
                fprintf(files[FPT_AVFOL], "%f ", outdat[d]);
            }
            fprintf(files[FPT_AVFOL], "\n");
        }
        if (mp > 0)
            // rolling section block average peak SNR per section
            fprintf(files[FPT_ROL], "%d	%.2lf	%d\n", mp, max, nxx);
    }
    printf("Period %f	No: bins = %d\n", period, (int)bins);

    // Build Best Result text file
    fprintf(files[FPT_TEXT],
            "Best Section Range SNR = %.2f	Section Centre %d	Best Section Range %d\n", pkmax,
            centre, span);
    printf("Max SNR  %f	Spanned Sections = %d	 Section Centre = %d\n", pkmax, span, centre);

    // Build  profile.txt - output Pulse Profile text file
    printf("\n Analysed Data Output Files: \n");
    memcpy(dfold, outsumt[25], bins * sizeof(double));
    // bestprof[d] best at rolling average setting. Rolling average peak = pkdat.
    psnr(bins, dfold, mbin, &datout, pulw, outdat);
    for (int d = 0; d < bins; d += 1) {
        fprintf(files[FPT_PROF], "%.1d	%lf	%lf	%lf\n", d, outdat[d], bestprof[d],
                pkdat[d]); /* write txt data to the output text file */
    }
    free(dfold);
    free(outdat);
    for (int i = 0; i < MAX(N, 51); i++) {
        free(outsumt[i]);
    }
    free(outsumt);

    // Build allbands.txt - compressed, match-filtered band-combined data text file
    for (int m = 0; m < M; m++) {
        const int idx = m * bins;
        for (int c = 0; c < bins; c += 1) {
            fprintf(files[FPT_ALLB], "%lf %lf %lf\n", (allbands[c + idx]), (allbandsd[c + idx]),
                    (allbandsdd[c + idx])); /* write txt data to the output text file */
            fprintf(files[FPT_SPALLB], "%lf	%lf\n", ((c + idx) * 1000.0 / period / M),
                    (spallbands[c + idx])); /* write txt data to the output text file */
        }
    }
    free(allbandsdd);
    free(allbands);
    free(allbandsd);
    free(spallbands);

    print_outputs(argv[1]);

    // finally close all files
    fclose(fptr);
    close_files(files);
    exit(0);
}
