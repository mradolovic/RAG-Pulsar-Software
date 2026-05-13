/*pulsar_det_an.c  pwe: 02/04/2022
Corrections by Martin Ante Rogosic and Marko Radolovic
March - May 2026

Pulsar Detection and Analysis
Command Format:
pulsar_det_an <data file> <N-point FFT> <data clock (ms))> <pulsar period (ms)> <No: sections><No.
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
#include <stdio.h>
#include <stdlib.h>

#include "../includes/io/files.h"

#include "../includes/numerics/DMSch.h"
#include "../includes/numerics/conv.h"
#include "../includes/numerics/is_pow_of_2.h"
#include "../includes/numerics/pdSch.h"
#include "../includes/numerics/perSch.h"
#include "../includes/numerics/psnr.h"
#include "../includes/numerics/spectrum.h"
#include "../includes/numerics/targgaus.h"

double comprs[32][262144], comprc[32][262144], ave[32][4096], avec[32][4096], rms[32][4096],
    mean[32], std[32], av[32][4096], rm[32][4096];
double compval[32][262144], compvald[32][262144], compvaldd[32][262144], spect[32][262144],
    puldat[256][4096], ppdot[128][128], bandat[32][4096];
double bndcum[32][4096], ftdat[1048576], ftdat2[1048576], allbands[262144], allbandsd[262144],
    allbandsdd[262144], spallbands[262144];
double ddispbands[256][131072], dfold[4096], ddfold[256][4096], dfoldd[4096], dfolddd[4096],
    outdat[4096], fftdat[1048576];
double bstdmprf[4096], pdat[1048576], targ[1048576],  puldatd[256][4096];
double sumt[4096], count[4096], outsumt[100][4096], pkdat[4096], foldat[256][4096],
    foldatd[256][4096];

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
    const float pulw = atof(argv[7]);     // pulsar pulse width
    float DM = atof(argv[8]);             // pulsar Dispersion Measure
    const float ppm = atof(argv[9]);      // period ppm adjustment
    float nno1 = atof(argv[11]);          // period ppm divisor
    const double rfband = atof(argv[12]); // rf bandwidth
    const double f0 = atof(argv[13]);     // rf centre
    const int rolav = atoi(argv[14]);     // number of rolling average sections
    const long int strt = atoi(argv[15]); // start number of wanted sections
    long int stp = atoi(argv[16]);        // end number of wanted sections
    const float thres = atof(argv[10]);   // spike threshold

    const int PTS = M * bins;

    // some error correction
    if (N > 100) {
        printf("No: Bands < 101 \n");
        exit(0);
    }

    if (is_pow_of_2(M) != 1 || is_pow_of_2(bins) != 1 || (M * N) > 262144) {
        printf("No: Sections and No of bins should be a power of 2 and their product < 262145\n");
        exit(0);
    }
    if (stp > M) {
        printf("Section range out of limit");
        exit(0);
    }

    // Read attenuated frequency channels input data file
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
        printf("%d ", (int)(blaf[d]));
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
        printf("%d ", (int)(blas[d]));
    }

    // Output Data file names and definitions

    // Copes with lower rf sideband systems
    int dmp = 1;
    if (DM < 0)
        dmp = -1;
    DM = DM * (float)dmp;

    // Allows for period ppm adjustment
    // period/p-dot search range is normally +-25ppm
    period = (double)period * (1.0 + (double)ppm / 1000000);

    // Parameter adjustments
    nno1 = 1 / nno1;        // period ppm multiplier
    const float dmdiv = 10; // DM range divider

    // Information print
    printf("\nThreshold = %.1f x rms \nPeriod Search Range Multiplier = %.2f\n", thres, nno1);

    // Command Line Input Information print
    printf("Pulsar ATNF Fold Period = %.8f ms	\n", period);
    printf("Data clock= %.3f ms	\n", clck);
    printf("Pulsar ATNF Pulse Width = %.2f ms	\n", pulw);
    printf("Pulsar ATNF Dispersion Measure = %.2f	\n", DM);
    printf("DM Range Divider = %.1f	\n", dmdiv);

    /*find length of input file*/
    fseeko(fptr, 0L, SEEK_END);
    const long int file_end = ftello(fptr) / 4;

    // print header
    printf("Input file bytes = %ld	\n", (long)file_end);

    // adjust for 32=bit binary input data
    printf("No. Data Samples = %ld\n", (long)file_end);

    // Expected dispersion delay across RF band
    const double td = 8.3 * rfband * DM * 1000000 / f0 / f0 / f0;
    printf("RF Centre = %.1f MHz,	RF Bandwidth = %.1f MHz\n", f0, rfband);
    printf("DM Band Delay = %.3fms, Equivalent No: of bins = %.3f\n", td, (td * bins / period));

    // Open Input 32-bit binary data file
    fptr = fopen(argv[1], "rb");

    // Command settings output data file
    fprintf(files[FPT_DAT], "%d %d %d %d \n", N, M, bins, rolav);

    // Print key settings
    printf("No: FFT Bands = %d\n", N);
    printf("Input Data per FFT Band = %ld\n", file_end / N);

    // Duration of data collection (1ms or 2ms clock)
    const double Tt0 =
        clck * (double)((double)file_end) / (double)N / 1000; // clck im ms, N= number of FFT bins
    printf("Data Collection Interval = %.1f secs\n", Tt0);

    // File compresion to optimise good data to maximise SNR
    double ratio;
    double nopers, sect_sampls, per_sampls;

    stp = stp + 1;
    nopers = (double)(Tt0 * 1000 /
                      period); // No. of periods in data file, 1000 factor as period input in ms
    printf("No: of Pulsar Periods = %.2f\n", nopers);

    per_sampls = (double)period * 4.0 * (double)N / (double)clck; // number of data samples in
                                                                  // period
    sect_sampls = (double)Tt0 * 1000.0 * 4.0 * (double)N / (double)M /
                  (double)clck; // number of data samples in section
    printf("No: of useful periods = %d\n", (int)nopers);
    ratio = (double)(sect_sampls / per_sampls); // data compression ratio

    // Start and end section for reducing analysis section range
    long int start = ((long int)((double)strt * ratio + 0.5) * (double)per_sampls / 4 / N) * 4 * N;
    long int end = ((long int)((double)stp * ratio + 0.5) * (double)per_sampls / 4 / N) * 4 * N;
    const long int nmax = (end - start) / 4 / N / M; // No. data file bytes
    // nmax = nmax / 4 / N / M;             // No. of compressed pulsar data points

    // printf("ratio = %f\n",ratio);
    printf("Start Byte = %ld\n", start);
    printf("Start Section Period  = %f\n", (double)start * clck / (double)N / period / 4);
    printf("End Byte = %ld\n", end);
    printf("End Section Period Number = %f\n", (double)end * clck / (double)N / period / 4);

    // set file pointer to start
    fseeko(fptr, start, SEEK_SET);
    start = start / 4; // 4-byte data word
    end = end / 4;     // 4-byte data word
    printf("Start Section = %ld	Stop Section = %ld\n", strt, stp - 1);
    printf("No: of Output Fold Sections  %d; No. Fold Bins = %d\n", M, bins);
    printf("No: of Output Data Bins = %d\n", M * bins);
    printf(
        "No: of Output samples/block = %ld\n",
        nmax); // Data is divided into M blocks and nmax is the number of data samples in each block

    // Duration of new data file
    double Tt = clck * (double)(end - start) / (double)N;
    printf("Working file duration = %.0f secs\n", Tt / 1000);
    double numper = (int)(Tt / period); // number of periods in file

    // Calculating the p-dot exponent
    // const double numlog = log10(numper);
    const double numlog = (int)(pow(10.0, (int)log10(numper))); // p-dot log exponent
    printf("P-dot exponent = %d\n",
           (int)numlog); // chosen to produce a -45 degree slope in 2D period/P-dot plot

    // Drift at file end due to ppm adjustment setting
    const double ppmdrift = ppm * Tt / 1000000;
    printf("ppm adjustment = %.2f\n", ppm);
    printf("Max Pulse ppm drift = %.2f ms\n", ppmdrift);

    // Rolling average input number
    printf("Rolling Average Number = %d\n", rolav);

    // Calculate target pulseconvolution for gaussian shaped pulse
    targgaus(pulw, PTS, targ, period, M);

    // Data Text Record
    fprintf(files[FPT_TEXT], "Pulsar Data	\n");
    fprintf(files[FPT_TEXT], "Input Data file: = %s\n", argv[1]);
    fprintf(files[FPT_TEXT], "Threshold = %.1f x rms	Period Range Multiplier = %.2f\n", thres,
            1 / nno1);
    fprintf(files[FPT_TEXT], "Pulsar Period = %.6f ms	\n", period);
    fprintf(files[FPT_TEXT], "Data clock= %.2f ms \n", clck);
    fprintf(files[FPT_TEXT], "Pulsar Pulse Width = %.2f ms	\n", pulw);
    fprintf(files[FPT_TEXT], "Pulsar Dispersion Measure = %.2f	\n", DM);
    fprintf(files[FPT_TEXT], "Input file bytes = %ld	\n", file_end * 2);
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
    fprintf(files[FPT_TEXT], "Period Search Range = %.2f ppm to %.2f ppm \n", -25 / (float)nno1,
            25 / (float)nno1);
    fprintf(files[FPT_TEXT], "P-dot Search Range = %.2f ppm/%d to %.2f ppm/%d \n",
            -(float)25.0 * 2.0 * numlog / nno1 / numper, (int)numlog,
            25.0 * 2.0 * numlog / nno1 / numper, (int)numlog);
    fprintf(files[FPT_TEXT], "Rolling Average Number = %d\n", rolav);
    fprintf(files[FPT_TEXT], "Start section = %ld\n", strt);
    fprintf(files[FPT_TEXT], "End section = %ld\n", stp - 1);

    // Serially fold data into M sections -
    // http://www.y1pwe.co.uk/RAProgs/LowSNRCorrelationSearch.pdf
    long int cnt = 0;
    for (long int aux = 0; aux < (nmax * M); aux += 1) { // nmax equals the number of data sets

        const long int xs = (double)aux / (double)nmax;
        const double b = (double)aux * (double)clck / (double)period;
        const long int sm = (long int)(((double)b - (double)((long int)b)) * (double)bins);
        const int mval = (int)((long int)sm + (long int)xs * (long int)bins);

        // Read input data FFT blocks
        float val = 0;
        for (int num = 0; num < N; num += 1) // N = number of FFT channels
        {
            fread(&val, 4, 1, fptr); // read 4-byte float from file
            comprs[num][mval] =
                comprs[num][mval] +
                ((double)(val *
                          10)); // fold into
                                // bins//comprs[num][mval]=comprs[num][mval]+(double)(val-128)*100.0;
            comprc[num][mval] = comprc[num][mval] + 1; // count bin entries
            fwrite(&val, 4, 1, files[FPT_CUT]);        // write bin file - selected sections
        }
        cnt = cnt + 1;
    }
    printf("\n Count= %ld\n", cnt);
    printf("New Compression Ratio = %f \n", ratio);
    // Confirm number of periods folded etc:
    numper = ((float)(cnt * clck) / period);
    ratio = (double)numper / (double)M;
    printf("New Compression Ratio = %f \n", ratio);
    printf("Period Search Range = %.2f ppm to %.2f ppm \n", -25 / (float)nno1, 25 / (float)nno1);
    printf("P-dot Search Range = %.2f ppm/%d to %.2f ppm/%d \n",
           -(float)25.0 * 2.0 * numlog / nno1 / numper, (int)numlog,
           25.0 * 2.0 * numlog / nno1 / numper, (int)numlog);

    // Check no count entries are zero
    for (int num = 0; num < N; num += 1) {
        for (int bi = 0; bi < M * bins; bi += 1) {
            if (comprc[num][bi] == 0)
                comprc[num][bi] = 1.0;
        }
    }

    // Calculate M section folded FFT channel data and section mean and rms;  M = number of
    // sections;
    for (int num = 0; num < N; num += 1) // N = number of FFT channels
    {
        for (long int c = 0; c < PTS; c += 1) // M = number of sections
        {
            const int Mc = c / bins;
            compval[num][c] =
                (float)(comprs[num][c] / comprc[num][c]); // normalise raw partially folded data
            ave[num][Mc] = ave[num][Mc] + (float)compval[num][c] / (float)bins; // section average
            rms[num][Mc] = rms[num][Mc] + ((float)(compval[num][c])) * ((float)(compval[num][c])) /
                                              (float)bins; // section squared rms
        }
    }

    // DC restore sections - removes long-term  receiver gain drift
    for (int num = 0; num < N; num += 1) // number of FFT channels
    {
        for (long int c = 0; c < PTS; c += 1) // M = number of sections; bins = number of fold bins
        {
            const int Mc = c / bins;
            compval[num][c] = compval[num][c] - ave[num][Mc]; // dc restored section fold data
            avec[num][Mc] = avec[num][Mc] + compval[num][c];  // new section mean value
        }
    }

    // Calculate DC restored mean and rms for each frequency band
    int m;
    double freq[256];
    for (int num = 0; num < N; num += 1) {
        for (m = 0; m < (int)M; m += 1) {
            mean[num] = mean[num] + ave[num][m] / M; // dc restored frequency channel average
            std[num] = std[num] + rms[num][m] / M;   // dc restored frequency channel rms
        }
        std[num] = sqrt((double)std[num] - (double)mean[num] * (double)mean[num]);
        freq[num] = mean[num]; // average channel frequency response
    }

    // Print Band mean and standard deviation values
    printf("\n Band Mean \n");
    for (int num = 0; num < N; num += 1) {
        printf("	%.1f", (float)mean[num]);
        fprintf(files[FPT_FREQ], "%f\n",
                (float)(freq[num])); /* write frequency band mean data to the output text file */
    }
    printf("\n");
    printf("\n Standard Deviation \n");
    for (int num = 0; num < N; num += 1) {
        printf("	%.1f", (float)std[num]);
    }
    printf("\n");

    // Build section folded, DC restored raw text data file
    for (long int c = 0; c < PTS; c += 1) {
        for (int num = 0; num < N; num += 1) {
            fprintf(files[FPT_RAW], "	%f",
                    (float)(compval[num][c])); /* write txt raw data to the output text file */
        }
        fprintf(files[FPT_RAW], "\n");
    }

    // printf("M1=%d	N=%d	bins=%d\n",M,N,bins);

    // Matched-filter data bandwidth to just pass pulsar pulse - FFT data and Convolve bands
    for (int num = 0; num < N; num += 1) { // printf("	M1=%d	N=%d	bins=%d\n",M,N,bins);
        for (long int c = 0; c < PTS; c += 1) {
            ftdat[c] = compval[num][c];
        }
        conv(ftdat, pulw, PTS, period, M, pdat, targ,
             fftdat); // outputs ftdat input blocks asconvolved and filtered fftdat blocks
        spectrum(fftdat, PTS, pdat, ftdat2); // outputs fftdat input block spectra as ftdat2
        for (long int c = 0; c < PTS; c += 1) {
            compval[num][c] = fftdat[c]; // now partially folded, dc restored and optimally filtered
            spect[num][c] = ftdat2[c];
            spallbands[c] = spallbands[c] + (spect[num][c]); // combine bands all bands spectrum
        }
    }

    // printf("M2=%d	N=%d	bins=%d\n",M,N,PTS/M);

    // Calculate FFT channel data and section match-filtered mean and mean square

    for (int num = 0; num < N; num += 1) {
        for (long int c = 0; c < PTS; c += 1) {
            const int Mc = c / bins;
            // printf("%d	%d\n",c,Mc);
            av[num][Mc] = av[num][Mc] + (float)(compval[num][c]) / bins;
            rm[num][Mc] =
                rm[num][Mc] + ((float)(compval[num][c]) * ((float)(compval[num][c])) / bins);
        }
    }

    for (m = 0; m < M; m += 1) {
        for (int num = 0; num < N; num += 1) {
            rm[num][m] = (double)sqrt((double)rm[num][m] -
                                      (double)(av[num][m] * (double)av[num][m])); // now true rms
        }
    }

    // Print Post DC correction and filtering mid-section, band rms, mean
    printf("\n Post DC correction and filtering band rms, mean \n");
    for (int num = 0; num < N; num += 1) { // Mc=(int)(c/bins);
        printf("	%.2f, %.2f", (double)rm[num][M / 2], (double)av[num][M / 2]);
    }
    printf("\n");

    // Final DC restore
    for (int num = 0; num < N; num += 1) {
        for (long int c = 0; c < PTS; c += 1) {
            const int Mc = c / bins;
            compval[num][c] = (compval[num][c] - av[num][Mc]);
        }
    }

    // Limit peaks - needed to ensure compressed file is not degraded by possible section RFI spikes
    // limits section band if greater than X standard deviations
    float X = (float)thres;
    for (int num = 0; num < N; num += 1) {
        for (long int c = 0; c < PTS; c += 1) {
            const int Mc = c / bins;
            if (compval[num][c] > (X * rm[num][Mc])) {
                compval[num][c] = X * rm[num][Mc];
            }
            if (-compval[num][c] > (X * rm[num][Mc])) {
                compval[num][c] = -X * rm[num][Mc];
            }
        }
    }
    printf("threshold = %.1f \n", X);

    // Build outdat.txt - compressed, match-filtered, 16-channel data text file
    for (long int c = 0; c < PTS; c += 1) {
        for (int num = 0; num < N; num += 1) {
            fprintf(files[FPT_OUT], "	%f",
                    (float)(compval[num][c])); /* write txt data to the output text file */
        }
        fprintf(files[FPT_OUT], "\n");
    }

    // Attenuate Blankf.txt frequency channels
    for (long int c = 0; c < PTS; c += 1) {
        if (couf >= 1) {
            for (int d = 0; d < couf; d += 1) {
                compval[blaf[d]][c] = (compval[blaf[d]][c]) / 100;
            }
        }
    }

    // Attenuate Blanks.txt sections
    if (cous >= 0) {
        for (int d = 0; d < cous; d += 1) {
            for (int num = 0; num < N; num += 1) {
                for (long int c = 0; c < PTS; c += 1) {
                    const int Mc = c / bins;
                    if (Mc == blas[d]) {
                        compval[num][c] = compval[num][c] / 100;
                    }
                }
            }
        }
    }

    // De-disperse recorded data based on command line DM value - de-dispered relative to median
    // frequency
    float delta = 1.f * dmp * td * (float)(bins) * (float)(N - 1) / (float)N / period /
                  (float)N; // time delay/sub-band
    printf("De-disperse Time Delay/sub-band = %.3f ms\n", delta);
    for (int num = 0; num < N; num += 1) {
        for (long int c = 0; c < PTS; c += 1) {
            compvaldd[num][c] =
                compval[num][(c + (int)(((float)((num - ((float)(N) / 2.0 - 0.5))) * delta)) +
                              M * bins) %
                             (M * bins)];
        }
    }

    // SNR per Band - Build bandS.txt
    int mbin = 0;
    float mmx = 0;
    printf("\n SNR per Band\n");
    for (int num = 0; num < N; num += 1) {
        for (int d = 0; d < bins; d += 1) {
            dfold[d] = 0;
        }
        for (int s = 0; s < M; s += 1) {
            for (int d = 0; d < bins; d += 1) {
                dfold[d] = dfold[d] + compval[num][s * bins + d];
            } // outsumt[num][d];outdat[d]=0;
        }
        psnrReturn datout;
        psnr(bins, dfold, mbin, &datout, pulw, outdat);
        if (datout.std_snr > mmx) {
            mmx = datout.std_snr;
        }
        for (int d = 0; d < bins; d += 1) {
            bandat[num][d] = outdat[d];
        }
        printf("Band = %d 	SNR =  %.2f	bin = %d\n", num, datout.std_snr, (int)datout.nx);
        fprintf(files[FPT_BND], "%d	%.2f	%d\n", num, datout.std_snr,
                (int)datout.nx); /* write band number SNR and max bin to bandS.txt ext file */
    }
    printf("\n");

    // Build De-dispersed bandat.txt band folds
    for (int num = 0; num < N; num += 1) {
        for (int d = 0; d < bins; d += 1) {
            fprintf(files[FPT_BNDD], "	%.3f",
                    bandat[num][d]); /* write band fold SNR data to the bandat.txt file */
        }
        fprintf(files[FPT_BNDD], "\n");
    }

    // Make allbands files for pre and DM value dispersed
    for (long int c = 0; c < PTS; c += 1) {
        for (int num = 0; num < N; num += 1) {
            allbands[c] = allbands[c] + compval[num][c];       // combine bands pre-dispersed
            allbandsdd[c] = allbandsdd[c] + compvaldd[num][c]; // combine bands DM value dispersed
        }
    }

    // De-dispersing search Routine
    // De-Disperse Bands - build dispersing matrix about band centre
    int emax = 0, dmx = 101,
        dmn = 50; // max and min of e range; dmp is DM polarity; dmdiv is range divider
    for (int e = 0; e < dmx; e += 1) {
        for (int num = (-N / 2); num < (N / 2); num += 1) {
            for (long int c = 0; c < PTS; c += 1) {
                ddispbands[e][c] =
                    ddispbands[e][c] +
                    compval[num + N / 2][(int)((c + 2 * M * bins +
                                                (int)((int)((float)((float)num + .5) * dmp *
                                                            ((float)e - (float)dmn) / dmdiv))) %
                                               (M * bins))] /
                        N; //*20/mean[num+N/2]/N;//N;//
            }
        }
    }

    // Dispersion Search
    for (int e = 0; e < dmx; e += 1) {
        for (int d = 0; d < (int)bins; d += 1) {
            ddfold[e][d] = 0;
        }
    }
    for (int e = 0; e < dmx; e += 1) {
        for (long int c = 0; c < M; c += 1) {
            for (int d = 0; d < bins; d += 1) {
                ddfold[e][d] = ddfold[e][d] + ddispbands[e][(c * bins + d) % (M * bins)];
            }
        }
    } // End dispersion search

    // Build dmSearch.txt - Dispersion Search SNR text file
    float dmsrch;
    mmx = 0;
    printf("\n Dispersion Search \n");
    for (int e = 0; e < dmx; e += 1) {
        for (int d = 0; d < bins; d += 1) {
            dfold[d] = ddfold[e][d];
        }
        psnrReturn datout;
        psnr(bins, dfold, mbin, &datout, pulw, outdat);
        if (datout.std_snr > mmx) {
            mmx = datout.std_snr;
            mbin = datout.nx;
            emax = e;
            for (int d = 0; d < bins; d += 1) {
                bstdmprf[d] = outdat[d]; //*period/bins*DM/td
            }
        } // printf("td=	%.2f	mmx= %.2f",td,mmx);
        dmsrch = DMSch((float)((((float)(N) * ((float)e - dmn) / dmdiv))) * (period / (float)bins) *
                           (DM / td),
                       N, DM, td, pulw);
        printf("DM =	%.2f	SNR =  %.2f	bin = %d	e = %d	dmsrch = %.2f\n",
               (float)((float)(N) * ((float)e - dmn) / dmdiv) * (period / (float)bins) * (DM / td),
               datout.std_snr, (int)datout.nx, (int)((e - dmn) * dmdiv),
               dmsrch); //*1.0*(int)period/(float)bins*DM/td
        fprintf(files[FPT_DMS], "%.2f	%.2f	%d	%d	%.2f\n",
                (float)((((float)(N) * ((float)e - dmn) / dmdiv))) * (period / (float)bins) *
                    (DM / td),
                datout.std_snr, (int)datout.nx, (int)((e - dmn) * dmdiv),
                dmsrch); /* write txt data to the output text file */

        for (int d = 0; d < bins; d += 1) {
            fprintf(files[FPT_DMFOLD], "%f	", (float)outdat[d]);
        }
        fprintf(files[FPT_DMFOLD], "\n");
    } // End dispersion search SNR

    // Build dmprf.txt - DM peak best fold text file
    for (int d = 0; d < bins; d += 1) {
        fprintf(files[FPT_DMPRF], "	%.2f", bstdmprf[d]); //.bstdmprf[(int)mbin]
    }
    fprintf(files[FPT_DMPRF], "\n");
    printf("\n");

    {
        printf("Max bin = %d	Best DM SNR = %.2f	emax = %d\n", (int)(mbin), bstdmprf[(int)mbin],
               emax);
        fprintf(files[FPT_MAX], "%d	%.2f	%.2f	%.2f\n", (int)(mbin), (float)mmx,
                ((float)((float)N * ((float)emax - (float)dmn) * period / (float)dmdiv)) /
                    (float)bins,
                td);
        fprintf(files[FPT_TEXT], "Max bin = %d	Max SNR = %.2f\n", (int)(mbin),
                (float)bstdmprf[(int)mbin]);
    }

    // Build dmSrchns.txt - Dispersion Search SNR - with target pulse range blanked
    for (int e = 0; e < dmx; e += 1) {
        for (int d = 0; d < bins; d += 1) {
            dfold[d] = ddfold[e][d];
            if (d > mbin - 8 * pulw && d < mbin + 8 * pulw)
                dfold[d] = 0; // 8 for large signalsmbin+4*pulw//30 small
        }
        psnrReturn datout;
        psnr(bins, dfold, mbin, &datout, pulw, outdat);
        fprintf(files[FPT_DMNS], "%.2f	%.2f	%d\n",
                (float)((float)(N) * ((float)e - dmn) / dmdiv) * (period / (float)bins) * (DM / td),
                datout.std_snr, (int)datout.nx); /* write txt data to the output text file */
    }
    printf("\n");
    for (int num = (-N / 2); num < (N / 2); num += 1) {
        for (long int c = 0; c < PTS; c += 1) {
            compvald[num + N / 2][c] = compval[num + N / 2][(
                int)((c + M * bins +
                      (int)((int)((float)(num + 0.5) * dmp * ((float)emax - dmn) / dmdiv))) %
                     (M * bins))]; //*20/mean[num+N/2];
        }
    }

    // Build allbandsd- optimum after dispersion search
    for (long int c = 0; c < PTS; c += 1) {
        for (int num = (0); num < (N); num += 1) {
            allbandsd[c] = allbandsd[c] + compvald[num][c];
        }
    }

    // Band/frequency channel Search
    printf("\n Band Search \n");

    float dperiod = bins;
    for (int num = 0; num < N; num += 1) {
        for (int s = 0; s < bins; s++) {
            sumt[s] = 0;
            count[s] = 1;
        }
        // Band folds
        for (int ss = 0; ss < PTS; ss++) {
            const long int range =
                (long long int)(((double)ss / ((double)dperiod) -
                                 (long long int)((double)ss / ((double)dperiod))) *
                                ((double)bins));
            sumt[range] = sumt[range] + compvald[num][ss];
            count[range] = count[range] + 1;
        }
        for (int s = 0; s < bins; s++) {
            outsumt[num][s] = 100 * sumt[s] / count[s];
        }
    }

    // Cumulative Band SNR - Build cumbands.txt

    printf(" Cumulative Band SNR\n");
    for (int d = 0; d < bins; d += 1) {
        dfold[d] = 0;
    }
    for (int num = 0; num < N; num += 1) {
        for (int d = 0; d < bins; d += 1) {
            dfold[d] = dfold[d] + outsumt[num][d];
        }
        psnrReturn datout;
        psnr(bins, dfold, mbin, &datout, pulw, outdat);
        for (int d = 0; d < bins; d += 1) {
            bndcum[num][d] = outdat[d];
        }
        printf("Band = %d 	Cum SNR =  %.2f	bin = %d\n", num, datout.std_snr, (int)datout.nx);
        fprintf(files[FPT_CUMF], "%d	%.2f	%d\n", num, datout.std_snr,
                (int)datout.nx); /* write cumulative band data to the cumbands.txt file */
    }
    printf("\n");

    // Build cumulative Band folds
    for (int d = 0; d < bins; d += 1) {
        for (int num = 0; num < N; num += 1) {
            fprintf(files[FPT_BNDC], "	%.2f", bndcum[num][d]);
        }
        fprintf(files[FPT_BNDC], "\n");
    }

    // SNR All Bands
    double maxx = 0;
    printf(" SNR All Bands\n");
    for (int d = 0; d < bins; d += 1) {
        dfold[d] = 0;
    }
    for (int num = 0; num < N; num += 1) {
        for (int d = 0; d < bins; d += 1) {
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
    for (m = 0; m < M; m += 1) {
        for (int d = 0; d < bins; d += 1) {
            dfold[d] = allbands[d + m * bins];
            dfoldd[d] = allbandsd[d + m * bins]; // dispersed version
            outdat[d] = 0;
        }
        // We need to think about whether we can add a local variable here, this way datout prevents
        // paralelization
        psnr(bins, dfold, mbin, &datout, pulw, outdat);
        for (int d = 0; d < bins; d += 1) {
            puldat[m][d] = outdat[d];
            fprintf(files[FPT_PULD], "	%.2f", puldat[m][d]);
        }
        fprintf(files[FPT_PULD], "\n");
        thry = maxx * sqrt((double)m / (double)M);
        fprintf(files[FPT_SEC], "%d	%.2f	%d	%.2f\n", m, datout.std_snr, (int)datout.nx,
                (float)thry); /* write txt data to the output text file */
        psnr(bins, dfold, mbin, &datout, pulw, outdat);
        for (int d = 0; d < bins; d += 1) {
            puldatd[m][d] = (float)outdat[d];
            fprintf(files[FPT_PULDD], "  %.2f",
                    puldatd[m][d]); /* write dedispersed puldat data to the output text file */
        }
        fprintf(files[FPT_PULDD], "\n");
    }

    // Cumulative Section SNR - Build cumsec.txt
    for (int d = 0; d < bins; d += 1) {
        dfold[d] = 0;
        dfoldd[d] = 0;
    }
    for (m = 0; m < M; m += 1) {
        for (int d = 0; d < bins; d += 1) {
            dfold[d] = dfold[d] + allbands[d + m * bins];
            dfoldd[d] = dfoldd[d] + allbandsd[d + m * bins];
            dfolddd[d] = dfolddd[d] + allbandsdd[d + m * bins];
        }
        psnr(bins, dfold, mbin, &datout, pulw, outdat);
        for (int d = 0; d < bins; d += 1) {
            foldat[m][d] = outdat[d];
            fprintf(files[FPT_FOLD], "%.2f  ", foldat[m][d]);
        }
        fprintf(files[FPT_FOLD], "\n");
        fprintf(files[FPT_CUMS], "%d	%.2f	%d\n", m, datout.std_snr,
                (int)datout.nx); /* write cumulative band SNR txt data to the output text file */
        psnr(bins, dfold, mbin, &datout, pulw, outdat);
        for (int d = 0; d < bins; d += 1) {
            //foldatd[m][d] = outdat[d];
            fprintf(files[FPT_FOLDD], "%.2f  ", outdat[d]); // dedispersed foldat
        }
        fprintf(files[FPT_FOLDD], "\n");
        psnr(bins, dfold, mbin, &datout, pulw, outdat);
        for (int d = 0; d < bins; d += 1) {
            fprintf(files[FPT_FOLDDD], "%.2f  ",
                    outdat[d]); // fprintf(fptfoldd,"\n"); // dedispersed foldat
        }
        fprintf(files[FPT_FOLDDD], "\n");
    }

    // Period Search Folds
    float pperiod;
    int s;
    for (int st = 0; st < 51; st++) {
        pperiod = bins * (1 + (st - 25) * 1 * ratio / 1000000 / nno1);
        for (s = 0; s < bins; s++) {
            sumt[s] = 0;
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
        for (s = 0; s < bins; s++) {
            outsumt[st][s] = 100 * sumt[s] / count[s];
        }
    }

    // Period search SNR. Build periodS.txt **All period p-dot search routines prefer un
    // de-dispersed data
    double pspr;
    maxx = 0;
    printf("\n Period Search, SNR v ppm change \n");
    for (int st = 0; st < 51; st += 1) {
        for (int d = 0; d < bins; d += 1) {
            dfold[d] = outsumt[st][d];
        }
        psnr(bins, dfold, mbin, &datout, pulw, outdat);
        if (datout.std_snr > maxx)
            maxx = datout.std_snr;
    }
    for (int st = 0; st < 51; st += 1) {
        for (int d = 0; d < bins; d += 1) {
            dfold[d] = outsumt[st][d];
        }
        psnr(bins, dfold, mbin, &datout, pulw, outdat);
        pspr = 3 + (maxx - 3) * perSch((double)(st - 25) * 1 / nno1, (numper), pulw, period);
        printf("ppm = %.2f	SNR =  %.2f	bin = %d\n", (float)(st - 25) / nno1, datout.std_snr,
               (int)datout.nx);
        fprintf(files[FPT_PER], " %.5f	%.5f	%d	%.3f\n", (float)(st - 25) / nno1,
                datout.std_snr, (int)datout.nx, pspr); /* write txt data to the output text file */
        if (st > 19 && st < 31) {
            for (int d = 0; d < bins; d += 1) {
                fprintf(files[FPT_PFOLD], "%f	",
                        (float)outdat[d]); // target period folds over the selected period range
            }
            fprintf(files[FPT_PFOLD], "\n");
        }
    }
    printf("\n");

    // P-dot Search Fold
    printf("\n Period Rate Search, SNR v ppm/%d change \n", (int)numlog);
    double periodt;
    for (int st = 0; st < 51; st++) {
        pperiod = bins * (1 + (25 - 25) * 1 * ratio / 1000000 / nno1);
        for (s = 0; s < bins; s++) {
            sumt[s] = 0;
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
        for (s = 0; s < bins; s++) {
            outsumt[st][s] = 100 * sumt[s] / count[s];
        }
    }

    // Pdot search SNR.  Build pdotS.txt
    double pdpr;
    for (int st = 0; st < 51; st += 1) {
        for (int d = 0; d < bins; d += 1) {
            dfold[d] = outsumt[st][d];
        }
        psnr(bins, dfold, mbin, &datout, pulw, outdat);
        pdpr = 3 + (maxx - 3) * pdSch((float)(st - 25) * 2 * numlog * .5 / (double)M / ratio / nno1,
                                      (numper), pulw, period, numlog);
        printf("pdot = %.2f	SNR =  %.2f	bin = %d\n",
               (float)(st - 25) * 2 * numlog / (double)M / ratio / 1 / nno1, datout.std_snr,
               (int)datout.nx);
        fprintf(files[FPT_PD], " %.5f	%.5f	%d	%.5f\n",
                (float)(st - 25) * 2 * numlog / (double)M / ratio / 1 / nno1, datout.std_snr,
                (int)datout.nx, pdpr); /* write txt data to the output text file */
    }
    printf("\n");

    // Period / P-dot Search Fold. Build ppd2d.txt
    for (int sp = 0; sp < 51; sp += 1) // p-dot range
    {
        for (int st = 0; st < 51; st += 1) // period range
        {
            pperiod = bins * (1 + (sp - 25) * 1 * ratio / 1000000 / nno1);
            for (s = 0; s < bins; s++) {
                sumt[s] = 0;
                count[s] = 0;
                dfold[s] = 0;
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
                sumt[range] = sumt[range] + allbandsd[ss];
                count[range] = count[range] + 1;
            }
            for (s = 0; s < bins; s++) {
                dfold[s] = 100 * sumt[s] / count[s];
            }
            psnr(bins, dfold, mbin, &datout, pulw, outdat);
            ppdot[sp][st] = datout.std_snr;
            fprintf(files[FPT_PPD], "%.2f	 ",
                    ppdot[sp][st]); /* write txt data to the output text file */
        }
        fprintf(files[FPT_PPD], "\n");
    }

    // Build secavsnr.txt - Rolling Window/Average SNR
    int xx, mp, nxx = 0;
    float max = 0, pkmax = 0;
    float bestprof[4096];
    int span = 0, centre = 0;

    for (mp = 1; mp < M + 1; mp += 1) {
        if (mp == rolav)
            printf("Set Section Rolling Average Window %d \n", mp);
        datout.std_snr = 0;
        max = 0;

        for (m = 0; m < M - mp + 1; m += 1) {
            for (int d = 0; d < bins; d += 1) {
                dfold[d] = 0;
            }
            for (xx = 0; xx < mp + 0; xx += 1) {
                for (int d = 0; d < bins; d += 1) {
                    dfold[d] = dfold[d] + allbandsd[d + (m + xx) * bins];
                }
            }
            psnr(bins, dfold, mbin, &datout, pulw, outdat);
            if (outdat[(int)datout.nx] > pkmax) {
                pkmax = outdat[(int)datout.nx];
                centre = m + mp / 2;
                span = mp;

                for (int d = 0; d < bins; d += 1) {
                    pkdat[d] = outdat[d];
                }
            }
            if (datout.std_snr > max) {
                max = datout.std_snr;

                nxx = datout.nx;
                if (mp == rolav) {
                    for (int d = 0; d < bins; d += 1) {
                        bestprof[d] = outdat[d];
                    }
                }
            }
            if (mp == rolav) {
                fprintf(files[FPT_AVSEC], "%d	%.2f	%d 	%.2f\n", m + (int)(mp / 2),
                        datout.std_snr, (int)datout.nx,
                        datout.unknown_var); /* write txt data to the output text file */
            }
            for (int d = 0; d < bins; d += 1) {
                fprintf(files[FPT_AVFOL], "%f ", outdat[d]);
            }
            fprintf(files[FPT_AVFOL], "\n");
        }
        if (mp > 0)
            fprintf(files[FPT_ROL], "%d	%.2f	%d\n", mp, max,
                    nxx); // rolling section block average peak SNR per section
    }
    printf("Period %f	No: bins = %d\n", period, (int)bins);

    // Build Best Result text file
    fprintf(files[FPT_TEXT],
            "Best Section Range SNR = %.2f	Section Centre %d	Best Section Range %d\n",
            (float)pkmax, centre, span);
    printf("Max SNR  %f	Spanned Sections = %d	 Section Centre = %d\n", pkmax, span, centre);

    // Build  profile.txt - output Pulse Profile text file
    printf("\n Analysed Data Output Files: \n");
    for (int d = 0; d < bins; d += 1) {
        dfold[d] = outsumt[25][d]; // outsumt/outdat[d] from p-dot fold with zero pdot
    }
    // bestprof[d] best at rolling average setting. Rolling average peak = pkdat.
    psnr(bins, dfold, mbin, &datout, pulw, outdat);
    for (int d = 0; d < bins; d += 1) {
        fprintf(files[FPT_PROF], "%.1f	%f	%f	%f\n", (float)d, (float)outdat[d], bestprof[d],
                pkdat[d]); /* write txt data to the output text file */
    }

    // Build allbands.txt - compressed, match-filtered band-combined data text file
    for (m = 0; m < M; m += 1) {
        for (long int c = 0; c < (int)(bins); c += 1) {
            fprintf(files[FPT_ALLB], "%f %f %f\n", (float)(allbands[c + m * bins]),
                    (float)(allbandsd[c + m * bins]),
                    (float)(allbandsdd[c + m * bins])); /* write txt data to the output text file */
            fprintf(files[FPT_SPALLB], "%f	%f\n", ((float)(c + m * bins) * 1000 / period / M),
                    (float)(spallbands[c + m * bins])); /* write txt data to the output text file */
        }
    }

    // print output file function and names
    printf("\n Input file: = %s\n", argv[1]);
    printf(" Raw compressed channelised data file: = %s\n", "rawdat.txt");
    printf(" Compressed Channelised Data file: = %s\n", "outdat.txt");
    printf(" Compressed combined bands data file: = %s\n", "allbands.txt");
    printf(" Compressed combined bands spectrum file: = %s\n", "spallbands.txt");
    printf(" Pulse profile file: = %s\n", "profile.txt");
    printf(" Period Search Peak SNR file: = %s\n", "periodS.txt");
    printf(" Target Folds over Period Range: = %s\n", "perfold.txt");
    printf(" P-dot search file: = %s\n", "pdotS.txt");
    printf(" 2-D Period/P-dot peak SNR file: = %s\n", "ppd2d.txt");
    printf(" DM search file: = %s\n", "dmSearch.txt");
    printf(" DM search target blanked file: = %s\n", "dmSrchns.txt");
    printf(" DM peak best fold data file: = %s\n", "dmprf.txt");
    printf(" Band Peak SNR file: = %s\n", "bandS.txt");
    printf(" Folded bands file: = %s\n", "bandat.txt");
    printf(" Cumulative band Peak SNR file: = %s\n", "cumbands.txt");
    printf(" Cumulative folded bands file: = %s\n", "bndcum.txt");
    printf(" Section peak SNR file: = %s\n", "secsnr.txt");
    printf(" Section folded data file: = %s\n", "puldat.txt");
    printf(" Cumulative section peak SNR file: = %s\n", "cumsec.txt");
    printf(" Cumulative folded section data file: = %s\n", "foldat.txt");
    printf(" Positive SNR Cumulative folded section data file: = %s\n", "foldatd.txt");
    printf(" Rolling average section peak SNR file: = %s\n", "secavsnr.txt");
    printf(" Rolling average section folds file: = %s\n", "secavfol.txt");
    printf(" Rolling section block peak SNR file: = %s\n", "secavrol.txt");
    printf(" Maximum SNR and Bin Number file: = %s\n", "max.txt");
    printf(" Reduced Range Raw Data file: = %s\n", "cutdat.bin");

    printf("pulsar_det_an has finished");

    // finally close all files
    fclose(fptr);
    close_files(files);
    exit(0);
}
