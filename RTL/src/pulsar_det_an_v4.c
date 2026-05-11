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

#include "../includes/numerics/DMSch.h"
#include "../includes/numerics/conv.h"
#include "../includes/numerics/is_pow_of_2.h"
#include "../includes/numerics/pdSch.h"
#include "../includes/numerics/perSch.h"
#include "../includes/numerics/psnr.h"
#include "../includes/numerics/spectrum.h"
#include "../includes/numerics/targgaus.h"

#define SWAP(a, b)                                                                                 \
    tempr = (a);                                                                                   \
    (a) = (b);                                                                                     \
    (b) = tempr
#define PI (6.28318530717959 / 2.0)
#define MAX 1971
#define lg2 log(2)

long int file_end;
long int range, strt, stp;
int num,  mval, PTS, Mc, mbin = 0, cd, blaf[256], blas[256], mf = 0, ms = 0, couf, cous,
                              rolav = 8, cco = 0, M, N, dmp = 1,  c, s;
float thres, bnds = 0, pulw, DM, ppm, mmx = 0, nno1, nno2, dmdiv;
double period, ppmdrift, rfband, td, f0, numper, numlog, maxsnr, Tt0, freq[256];
long int cnt = 0, cnt1 = 0, xs, sm, nmax;
double comprs[32][262144], comprc[32][262144], ave[32][4096], avec[32][4096], rms[32][4096],
    mean[32], std[32], av[32][4096], rm[32][4096];
double compval[32][262144], compvald[32][262144], compvaldd[32][262144], spect[32][262144],
    puldat[256][4096], ppdot[128][128], bandat[32][4096];
double bndcum[32][4096], ftdat[1048576], ftdat2[1048576], allbands[262144], allbandsd[262144],
    allbandsdd[262144], spallbands[262144];
double ddispbands[256][131072], datout[8], dfold[4096], ddfold[256][4096], dfoldd[4096],
    dfolddd[4096], outdat[4096], fftdat[1048576];
double zero = 0.000001, bstdmprof[4096], bstdmprf[4096], blasec[256], pdat[1048576], targ[1048576],
       foldatdd[256][4096], puldatd[256][4096];
double clck, sumt[4096], count[4096], outsumt[100][4096], pkdat[4096], sectsnr[256][8],
    secavsnr[256][8], foldat[256][4096], foldatd[256][4096];

FILE *fptblnkf;
FILE *fptblnks;
FILE *fptr;
FILE *fptprof;
FILE *fptdms;
FILE *fptdmns;
FILE *fptper;
FILE *fptpd;
FILE *fptbnd;
FILE *fptbndd;
FILE *fptbndc;
FILE *fptcumf;
FILE *fptcums;
FILE *fptsec;
FILE *fptavsec;
FILE *fptavfol;
FILE *fptpuld;
FILE *fptallb;
FILE *fptspallb;
FILE *fptfold;
FILE *fptfoldd;
FILE *fptfolddd;
FILE *fptppd;
FILE *fptout;
FILE *fptraw;
FILE *fptcut;
FILE *fptdmprf;
FILE *fptrol;
FILE *fpttext;
FILE *fptmax;
FILE *fptpfold;
FILE *fptpuldd;
FILE *fptdmfold;
FILE *fptfreq;
FILE *fptdat;

int main(int argc, char *argv[]) {
    long int aux = 0, start = 0, end = 127;
    long int c;
    double b;
    int d, mm;
    float val;


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

    if ((fptblnkf = fopen("Blankf.txt", "r")) == NULL) {
        printf("No Band Attenuation file %s. \n", "Blankf.txt");
        exit(0);
    }

    if ((fptblnks = fopen("Blanks.txt", "r")) == NULL) {
        printf("No Section Attenuation file %s. \n", "Blanks.txt");
        exit(0);
    }
    fclose(fptblnkf);

    printf("Pulsar Data\n");
    printf("\nInput Data File = %s\n", argv[1]);

    // Read command line parameters
    N = atoi(argv[2]);       // number of FFT channels
    clck = atof(argv[3]);    // data sample interval in ms
    period = atof(argv[4]);  // pulsar topocentric period
    M = atoi(argv[5]);       // number of compressed blocks/sections
    const int bins = atoi(argv[6]);    // number of output fold bins
    pulw = atof(argv[7]);    // pulsar pulse width
    DM = atof(argv[8]);      // pulsar Dispersion Measure
    ppm = atof(argv[9]);     // period ppm adjustment
    nno1 = atof(argv[11]);   // period ppm divisor
    rfband = atof(argv[12]); // rf bandwidth
    f0 = atof(argv[13]);     // rf centre
    rolav = atoi(argv[14]);  // number of rolling average sections
    strt = atoi(argv[15]);   // start number of wanted sections
    stp = atoi(argv[16]);    // end number of wanted sections
    thres = atof(argv[10]);  // spike threshold

    PTS = M * bins;

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
    fptblnkf = fopen("Blankf.txt", "r");
    while (fscanf(fptblnkf, "%d", &mf) != EOF) {
        blaf[couf] = mf;
        couf += 1;
    }
    fclose(fptblnkf);
    printf("Blanked Bands: ");
    for (d = 0; d < couf; d++) {
        printf("%d ", (int)(blaf[d]));
    }

    // Read attenuated sections input data file
    fptblnks = fopen("Blanks.txt", "r");
    cous = 0;
    for (mm = 0; mm < M; mm++)
        blasec[mm] = 1;
    while (fscanf(fptblnks, "%d", &ms) != EOF) {
        blas[cous] = ms;
        blasec[ms] = 0.5;
        cous += 1;
    }
    fclose(fptblnks);
    printf("\nBlanked Sections: ");
    for (d = 0; d < cous; d++) {
        printf("%d ", (int)(blas[d]));
    }

    // Output Data file names and definitions
    fptprof = fopen("profile.txt", "w");      // pulse profile + peak rolling average profile
    fptdms = fopen("dmSearch.txt", "w");      // DM search scan
    fptdmns = fopen("dmSrchns.txt", "w");     // DM search with pulsar target blanked
    fptper = fopen("periodS.txt", "w");       // period search scan
    fptpd = fopen("pdotS.txt", "w");          // pdot search scan
    fptbnd = fopen("bandS.txt", "w");         // band search max scan
    fptbndd = fopen("bandat.txt", "w");       // band folds
    fptbndc = fopen("bandcum.txt", "w");      // cumulative band folds
    fptcumf = fopen("cumbands.txt", "w");     // cumulative SNR of band search
    fptcums = fopen("cumsec.txt", "w");       // cumulative SNR of section search
    fptsec = fopen("secsnr.txt", "w");        // SNR of individual sections
    fptavsec = fopen("secavsnr.txt", "w");    // rolling average section SNR
    fptpuld = fopen("puldat.txt", "w");       // individual section SNR folds
    fptpuldd = fopen("puldatd.txt", "w");     // individual section Dispersed SNR folds
    fptallb = fopen("allbands.txt", "w");     // partial folded data bands combined
    fptspallb = fopen("spallbands.txt", "w"); // partial folded spectrum bands combined
    fptfold = fopen("foldat.txt", "w");       // cumulative SNR of bins
    fptfoldd = fopen("foldatd.txt", "w");     // cumulative SNR of bins, dedispered SNR
    fptfolddd = fopen("foldatdd.txt", "w");   // cumulative SNR of bins, dedispered SNR
    fptppd = fopen("ppd2d.txt", "w");         // 2D period/p-dot plot
    fptout = fopen("outdat.txt", "w");        // matched-filtered output data - continuous stream
    fptraw = fopen("rawdat.txt", "w");        // raw compressed partial folded data
    fptcut = fopen("cutdat.bin", "wb");       // data file cut between start and end sections
    fptdmprf = fopen("dmprf.txt", "w");       // Optimum DM - best profile
    fptavfol = fopen("secavfol.txt", "w");    // Section average folds
    fptrol = fopen("secavrol.txt", "w");      // Section variable block average fold peak
    fpttext = fopen("header.txt", "w");       // Header for Python program
    fptmax = fopen("max.txt", "w");           // maximum bin and SNR
    fptpfold = fopen("pfold.txt", "w");       // period search folds
    fptdmfold = fopen("dmfold.txt", "w");     // dm search folds
    fptfreq = fopen("spectrum.txt", "w");     // input data FFT spectrum
    fptdat = fopen("data.txt", "w");          // command line settings

    // Copes with lower rf sideband systems
    if (DM < 0)
        dmp = -1;
    DM = DM * (float)dmp;

    // Allows for period ppm adjustment
    // period/p-dot search range is normally +-25ppm
    period = (double)period * (1.0 + (double)ppm / 1000000);

    // Parameter adjustments
    nno1 = 1 / nno1; // period ppm multiplier
    dmdiv = 10;      // DM range divider

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
    file_end = (long long)(ftello(fptr));

    // print header
    printf("Input file bytes = %ld	\n", (long)file_end);

    // adjust for 32=bit binary input data
    file_end = (long long int)((long int)file_end / 4); // 4-byte binary data
    printf("No. Data Samples = %ld\n", (long)file_end);

    // Expected dispersion delay across RF band
    td = 8.3 * rfband * DM * 1000000 / f0 / f0 / f0;
    printf("RF Centre = %.1f MHz,	RF Bandwidth = %.1f MHz\n", f0, rfband);
    printf("DM Band Delay = %.3fms, Equivalent No: of bins = %.3f\n", td, (td * bins / period));

    // Open Input 32-bit binary data file
    fptr = fopen(argv[1], "rb");

    // Command settings output data file
    fprintf(fptdat, "%d %d %d %d \n", N, M, bins, rolav);

    // Print key settings
    printf("No: FFT Bands = %d\n", N);
    printf("Input Data per FFT Band = %ld\n", file_end / N);

    // Duration of data collection (1ms or 2ms clock)
    Tt0 = clck * (double)((double)file_end) / (double)N / 1000; // clck im ms, N= number of FFT bins
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
    start = (long long int)((long int)((double)strt * (double)ratio + 0.5) * (double)per_sampls /
                            4 / N) *
            4 * N;
    end = (long long int)((long int)((double)stp * (double)ratio + 0.5) * (double)per_sampls / 4 /
                          N) *
          4 * N;
    nmax = (long long int)(end - start); // No. data file bytes
    nmax = nmax / 4 / N / M;             // No. of compressed pulsar data points

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
    numper = (int)(Tt / period); // number of periods in file

    // Calculating the p-dot exponent
    numlog = log10(numper);
    numlog = (int)(pow(10.0, (int)numlog)); // p-dot log exponent
    printf("P-dot exponent = %d\n",
           (int)numlog); // chosen to produce a -45 degree slope in 2D period/P-dot plot

    // Drift at file end due to ppm adjustment setting
    ppmdrift = ppm * Tt / 1000000;
    printf("ppm adjustment = %.2f\n", ppm);
    printf("Max Pulse ppm drift = %.2f ms\n", ppmdrift);

    // Rolling average input number
    printf("Rolling Average Number = %d\n", rolav);

    // Calculate target pulseconvolution for gaussian shaped pulse
    targgaus(pulw, PTS, targ, period, M);

    // Data Text Record
    fprintf(fpttext, "Pulsar Data	\n");
    fprintf(fpttext, "Input Data file: = %s\n", argv[1]);
    fprintf(fpttext, "Threshold = %.1f x rms	Period Range Multiplier = %.2f\n", thres, 1 / nno1);
    fprintf(fpttext, "Pulsar Period = %.6f ms	\n", period);
    fprintf(fpttext, "Data clock= %.2f ms \n", clck);
    fprintf(fpttext, "Pulsar Pulse Width = %.2f ms	\n", pulw);
    fprintf(fpttext, "Pulsar Dispersion Measure = %.2f	\n", DM);
    fprintf(fpttext, "Input file bytes = %ld	\n", file_end * 2);
    fprintf(fpttext, "RF Centre = %.1f MHz,	RF Bandwidth = %.1f MHz\n", f0, rfband);
    fprintf(fpttext, "DM Band Delay = %.3fms, No. Delay bins = %.3f\n", td, (td * bins / period));
    fprintf(fpttext, "No. Data Samples = %ld\n", file_end);
    fprintf(fpttext, "No: FFT Bands = %d\n", N);
    fprintf(fpttext, "Input Data per FFT Band = %ld\n", file_end / N);
    fprintf(fpttext, "No. of Output Fold Sections = %d ; No. bins = %d\n", M, bins);
    fprintf(fpttext, "No. of Output samples = %d\n", M * bins);
    fprintf(fpttext, "Working file duration = %.0f secs\n", Tt / 1000);
    fprintf(fpttext, "Number of pulsar periods = %.0f\n", numper);
    fprintf(fpttext, "P-dot factor = %.0f\n", numlog);
    fprintf(fpttext, "ppm adjustment = %.2f\n", ppm);
    fprintf(fpttext, "Max Pulse ppm drift = %.2f ms\n", ppmdrift);
    fprintf(fpttext, "Compression Ratio = %.2f	\n", ratio);
    fprintf(fpttext, "Period Search Range = %.2f ppm to %.2f ppm \n", -25 / (float)nno1,
            25 / (float)nno1);
    fprintf(fpttext, "P-dot Search Range = %.2f ppm/%d to %.2f ppm/%d \n",
            -(float)25.0 * 2.0 * numlog / nno1 / numper, (int)numlog,
            25.0 * 2.0 * numlog / nno1 / numper, (int)numlog);
    fprintf(fpttext, "Rolling Average Number = %d\n", rolav);
    fprintf(fpttext, "Start section = %ld\n", strt);
    fprintf(fpttext, "End section = %ld\n", stp - 1);

    // Serially fold data into M sections -
    // http://www.y1pwe.co.uk/RAProgs/LowSNRCorrelationSearch.pdf
    for (aux = 0; aux < (nmax * M); aux += 1) { // nmax equals the number of data sets

        xs = (long int)((double)aux / (double)nmax);
        b = (double)aux * (double)clck / (double)period;
        sm = (long int)(((double)b - (double)((long int)b)) * (double)bins);
        mval = (int)((long int)sm + (long int)xs * (long int)bins);

        // Read input data FFT blocks
        for (num = 0; num < N; num += 1) // N = number of FFT channels
        {
            fread(&val, 4, 1, fptr); // read 4-byte float from file
            comprs[num][mval] =
                comprs[num][mval] +
                ((double)(val *
                          10)); // fold into
                                // bins//comprs[num][mval]=comprs[num][mval]+(double)(val-128)*100.0;
            comprc[num][mval] = comprc[num][mval] + 1; // count bin entries
            cnt1 = cnt1 + 1;
            fwrite(&val, 4, 1, fptcut); // write bin file - selected sections
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
    int bi;
    for (num = 0; num < N; num += 1) {
        for (bi = 0; bi < M * bins; bi += 1) {
            if (comprc[num][bi] == 0)
                comprc[num][bi] = 1.0;
        }
    }

    // Calculate M section folded FFT channel data and section mean and rms;  M = number of
    // sections;
    for (num = 0; num < N; num += 1) // N = number of FFT channels
    {
        for (c = 0; c < PTS; c += 1) // M = number of sections
        {
            Mc = c / bins;
            compval[num][c] =
                (float)(comprs[num][c] / comprc[num][c]); // normalise raw partially folded data
            ave[num][Mc] = ave[num][Mc] + (float)compval[num][c] / (float)bins; // section average
            rms[num][Mc] = rms[num][Mc] + ((float)(compval[num][c])) * ((float)(compval[num][c])) /
                                              (float)bins; // section squared rms
        }
    }

    // DC restore sections - removes long-term  receiver gain drift
    for (num = 0; num < N; num += 1) // number of FFT channels
    {
        for (c = 0; c < PTS; c += 1) // M = number of sections; bins = number of fold bins
        {
            Mc = c / bins;
            compval[num][c] = compval[num][c] - ave[num][Mc]; // dc restored section fold data
            avec[num][Mc] = avec[num][Mc] + compval[num][c];  // new section mean value
        }
    }

    // Calculate DC restored mean and rms for each frequency band
    int m;
    for (num = 0; num < N; num += 1) {
        for (m = 0; m < (int)M; m += 1) {
            mean[num] = mean[num] + ave[num][m] / M; // dc restored frequency channel average
            std[num] = std[num] + rms[num][m] / M;   // dc restored frequency channel rms
        }
        std[num] = sqrt((double)std[num] - (double)mean[num] * (double)mean[num]);
        freq[num] = mean[num]; // average channel frequency response
    }

    // Print Band mean and standard deviation values
    printf("\n Band Mean \n");
    for (num = 0; num < N; num += 1) {
        printf("	%.1f", (float)mean[num]);
        fprintf(fptfreq, "%f\n",
                (float)(freq[num])); /* write frequency band mean data to the output text file */
    }
    printf("\n");
    printf("\n Standard Deviation \n");
    for (num = 0; num < N; num += 1) {
        printf("	%.1f", (float)std[num]);
    }
    printf("\n");

    // Build section folded, DC restored raw text data file
    for (c = 0; c < PTS; c += 1) {
        for (num = 0; num < N; num += 1) {
            fprintf(fptraw, "	%f",
                    (float)(compval[num][c])); /* write txt raw data to the output text file */
        }
        fprintf(fptraw, "\n");
    }

    // printf("M1=%d	N=%d	bins=%d\n",M,N,bins);

    // Matched-filter data bandwidth to just pass pulsar pulse - FFT data and Convolve bands
    for (num = 0; num < N; num += 1) { // printf("	M1=%d	N=%d	bins=%d\n",M,N,bins);
        for (c = 0; c < PTS; c += 1) {
            ftdat[c] = compval[num][c];
        }
        conv(ftdat, pulw, PTS, period, M, pdat,
             targ, fftdat); // outputs ftdat input blocks asconvolved and filtered fftdat blocks
        spectrum(fftdat, PTS, pdat, ftdat, ftdat2); // outputs fftdat input block spectra as ftdat2
        for (c = 0; c < PTS; c += 1) {
            compval[num][c] = fftdat[c]; // now partially folded, dc restored and optimally filtered
            spect[num][c] = ftdat2[c];
            spallbands[c] = spallbands[c] + (spect[num][c]); // combine bands all bands spectrum
        }
    }

    // printf("M2=%d	N=%d	bins=%d\n",M,N,PTS/M);

    // Calculate FFT channel data and section match-filtered mean and mean square

    for (num = 0; num < N; num += 1) {
        for (c = 0; c < PTS; c += 1) {
            Mc = c / bins;
            // printf("%d	%d\n",c,Mc);
            av[num][Mc] = av[num][Mc] + (float)(compval[num][c]) / bins;
            rm[num][Mc] =
                rm[num][Mc] + ((float)(compval[num][c]) * ((float)(compval[num][c])) / bins);
        }
    }

    for (m = 0; m < M; m += 1) {
        for (num = 0; num < N; num += 1) {
            rm[num][m] = (double)sqrt((double)rm[num][m] -
                                      (double)(av[num][m] * (double)av[num][m])); // now true rms
        }
    }

    // Print Post DC correction and filtering mid-section, band rms, mean
    printf("\n Post DC correction and filtering band rms, mean \n");
    for (num = 0; num < N; num += 1) { // Mc=(int)(c/bins);
        printf("	%.2f, %.2f", (double)rm[num][M / 2], (double)av[num][M / 2]);
    }
    printf("\n");

    // Final DC restore
    for (num = 0; num < N; num += 1) {
        for (c = 0; c < PTS; c += 1) {
            Mc = c / bins;
            compval[num][c] = (compval[num][c] - av[num][Mc]);
        }
    }

    // Limit peaks - needed to ensure compressed file is not degraded by possible section RFI spikes
    // limits section band if greater than X standard deviations
    float X = (float)thres;
    for (num = 0; num < N; num += 1) {
        for (c = 0; c < PTS; c += 1) {
            Mc = c / bins;
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
    for (c = 0; c < PTS; c += 1) {
        for (num = 0; num < N; num += 1) {
            fprintf(fptout, "	%f",
                    (float)(compval[num][c])); /* write txt data to the output text file */
        }
        fprintf(fptout, "\n");
    }

    // Attenuate Blankf.txt frequency channels
    for (c = 0; c < PTS ; c += 1) {
        if (couf >= 1) {
            for (d = 0; d < couf; d += 1) {
                compval[blaf[d]][c] = (compval[blaf[d]][c]) / 100;
            }
        }
    }

    // Attenuate Blanks.txt sections
    if (cous >= 0) {
        for (d = 0; d < cous; d += 1) {
            for (num = 0; num < N; num += 1) {
                for (c = 0; c < PTS; c += 1) {
                    Mc = c / bins;
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
    for (num = 0; num < N; num += 1) {
        for (c = 0; c < PTS; c += 1) {
            compvaldd[num][c] =
                compval[num][(c + (int)(((float)((num - ((float)(N) / 2.0 - 0.5))) * delta)) +
                              M * bins) %
                             (M * bins)];
        }
    }

    // SNR per Band - Build bandS.txt
    printf("\n SNR per Band\n");
    for (num = 0; num < N; num += 1) {
        for (d = 0; d < bins; d += 1) {
            dfold[d] = 0;
        }
        for (s = 0; s < M; s += 1) {
            for (d = 0; d < bins; d += 1) {
                dfold[d] = dfold[d] + compval[num][s * bins + d];
            } // outsumt[num][d];outdat[d]=0;
        }
        psnr(bins, dfold, mbin, datout, pulw, outdat);
        if (datout[0] > mmx) {
            mmx = datout[0];
        }
        for (d = 0; d < bins; d += 1) {
            bandat[num][d] = outdat[d];
        }
        printf("Band = %d 	SNR =  %.2f	bin = %d\n", num, datout[0], (int)datout[1]);
        fprintf(fptbnd, "%d	%.2f	%d\n", num, datout[0],
                (int)datout[1]); /* write band number SNR and max bin to bandS.txt ext file */
    }
    printf("\n");

    // Build De-dispersed bandat.txt band folds
    for (num = 0; num < N; num += 1) {
        for (d = 0; d < bins; d += 1) {
            fprintf(fptbndd, "	%.3f",
                    bandat[num][d]); /* write band fold SNR data to the bandat.txt file */
        }
        fprintf(fptbndd, "\n");
    }

    // Make allbands files for pre and DM value dispersed
    for (c = 0; c < PTS; c += 1) {
        for (num = 0; num < N; num += 1) {
            allbands[c] = allbands[c] + compval[num][c];       // combine bands pre-dispersed
            allbandsdd[c] = allbandsdd[c] + compvaldd[num][c]; // combine bands DM value dispersed
        }
    }

    // De-dispersing search Routine
    // De-Disperse Bands - build dispersing matrix about band centre
    int emax = 0, dmx = 101,
        dmn = 50; // max and min of e range; dmp is DM polarity; dmdiv is range divider
    for (int e = 0; e < dmx; e += 1) {
        for (num = (-N / 2); num < (N / 2); num += 1) {
            for (c = 0; c < PTS; c += 1) {
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
        for (d = 0; d < (int)bins; d += 1) {
            ddfold[e][d] = 0;
        }
    }
    for (int e = 0; e < dmx; e += 1) {
        for (c = 0; c < M; c += 1) {
            for (d = 0; d < bins; d += 1) {
                ddfold[e][d] = ddfold[e][d] + ddispbands[e][(c * bins + d) % (M * bins)];
            }
        }
    } // End dispersion search

    // Build dmSearch.txt - Dispersion Search SNR text file
    float dmsrch;
    mmx = 0;
    printf("\n Dispersion Search \n");
    for (int e = 0; e < dmx; e += 1) {
        for (d = 0; d < bins; d += 1) {
            dfold[d] = ddfold[e][d];
        }
        psnr(bins, dfold, mbin, datout, pulw, outdat);
        if (datout[0] > mmx) {
            mmx = datout[0];
            mbin = datout[1];
            emax = e;
            for (d = 0; d < bins; d += 1) {
                bstdmprf[d] = outdat[d]; //*period/bins*DM/td
            }
        } // printf("td=	%.2f	mmx= %.2f",td,mmx);
        dmsrch = DMSch((float)((((float)(N) * ((float)e - dmn) / dmdiv))) * (period / (float)bins) *
                           (DM / td),
                       N, DM, td, pulw);
        printf("DM =	%.2f	SNR =  %.2f	bin = %d	e = %d	dmsrch = %.2f\n",
               (float)((float)(N) * ((float)e - dmn) / dmdiv) * (period / (float)bins) * (DM / td),
               datout[0], (int)datout[1], (int)((e - dmn) * dmdiv),
               dmsrch); //*1.0*(int)period/(float)bins*DM/td
        fprintf(fptdms, "%.2f	%.2f	%d	%d	%.2f\n",
                (float)((((float)(N) * ((float)e - dmn) / dmdiv))) * (period / (float)bins) *
                    (DM / td),
                datout[0], (int)datout[1], (int)((e - dmn) * dmdiv),
                dmsrch); /* write txt data to the output text file */

        for (d = 0; d < bins; d += 1) {
            fprintf(fptdmfold, "%f	", (float)outdat[d]);
        }
        fprintf(fptdmfold, "\n");
    } // End dispersion search SNR

    // Build dmprf.txt - DM peak best fold text file
    for (d = 0; d < bins; d += 1) {
        fprintf(fptdmprf, "	%.2f", bstdmprf[d]); //.bstdmprf[(int)mbin]
    }
    fprintf(fptdmprf, "\n");
    printf("\n");

    {
        printf("Max bin = %d	Best DM SNR = %.2f	emax = %d\n", (int)(mbin), bstdmprf[(int)mbin],
               emax);
        fprintf(fptmax, "%d	%.2f	%.2f	%.2f\n", (int)(mbin), (float)mmx,
                ((float)((float)N * ((float)emax - (float)dmn) * period / (float)dmdiv)) /
                    (float)bins,
                td);
        fprintf(fpttext, "Max bin = %d	Max SNR = %.2f\n", (int)(mbin), (float)bstdmprf[(int)mbin]);
    }

    // Build dmSrchns.txt - Dispersion Search SNR - with target pulse range blanked
    for (int e = 0; e < dmx; e += 1) {
        for (d = 0; d < bins; d += 1) {
            dfold[d] = ddfold[e][d];
            if (d > mbin - 8 * pulw && d < mbin + 8 * pulw)
                dfold[d] = 0; // 8 for large signalsmbin+4*pulw//30 small
        }
        psnr(bins, dfold, mbin, datout, pulw, outdat);
        fprintf(fptdmns, "%.2f	%.2f	%d\n",
                (float)((float)(N) * ((float)e - dmn) / dmdiv) * (period / (float)bins) * (DM / td),
                datout[0], (int)datout[1]); /* write txt data to the output text file */
    }
    printf("\n");
    for (num = (-N / 2); num < (N / 2); num += 1) {
        for (c = 0; c < PTS; c += 1) {
            compvald[num + N / 2][c] = compval[num + N / 2][(
                int)((c + M * bins +
                      (int)((int)((float)(num + 0.5) * dmp * ((float)emax - dmn) / dmdiv))) %
                     (M * bins))]; //*20/mean[num+N/2];
        }
    }

    // Build allbandsd- optimum after dispersion search
    for (c = 0; c < PTS; c += 1) {
        for (num = (0); num < (N); num += 1) {
            allbandsd[c] = allbandsd[c] + compvald[num][c];
        }
    }

    // Band/frequency channel Search
    printf("\n Band Search \n");

    float dperiod = bins;
    for (num = 0; num < N; num += 1) {
        for (s = 0; s < bins; s++) {
            sumt[s] = 0;
            count[s] = 1;
        }
        // Band folds
        int ss;
        for (ss = 0; ss < PTS; ss++) {
            ss = (double)ss;
            range = (long long int)(((double)ss / ((double)dperiod) -
                                     (long long int)((double)ss / ((double)dperiod))) *
                                    ((double)bins));
            sumt[range] = sumt[range] + compvald[num][ss];
            count[range] = count[range] + 1;
        }
        for (s = 0; s < bins; s++) {
            outsumt[num][s] = 100 * sumt[s] / count[s];
        }
    }

    // Cumulative Band SNR - Build cumbands.txt

    printf(" Cumulative Band SNR\n");
    for (d = 0; d < bins; d += 1) {
        dfold[d] = 0;
    }
    for (num = 0; num < N; num += 1) {
        for (d = 0; d < bins; d += 1) {
            dfold[d] = dfold[d] + outsumt[num][d];
        }
        psnr(bins, dfold, mbin, datout, pulw, outdat);
        for (d = 0; d < bins; d += 1) {
            bndcum[num][d] = outdat[d];
        }
        printf("Band = %d 	Cum SNR =  %.2f	bin = %d\n", num, datout[0], (int)datout[1]);
        fprintf(fptcumf, "%d	%.2f	%d\n", num, datout[0],
                (int)datout[1]); /* write cumulative band data to the cumbands.txt file */
    }
    printf("\n");

    // Build cumulative Band folds
    for (d = 0; d < bins; d += 1) {
        for (num = 0; num < N; num += 1) {
            fprintf(fptbndc, "	%.2f", bndcum[num][d]);
        }
        fprintf(fptbndc, "\n");
    }

    // SNR All Bands
    double maxx = 0;
    printf(" SNR All Bands\n");
    for (d = 0; d < bins; d += 1) {
        dfold[d] = 0;
    }
    for (num = 0; num < N; num += 1) {
        for (d = 0; d < bins; d += 1) {
            dfold[d] = dfold[d] + outsumt[num][d];
        }
    }
    psnr(bins, dfold, mbin, datout, pulw, outdat);
    maxx = datout[0];
    printf("Bands = %d 	BSNR =  %.2f	bin = %d\n", N, datout[0], (int)datout[1]);
    printf("\n");

    // Section SNR. Build puldat.txt (section folds) and secsnr.txt (section SNR)
    double thry;
    for (m = 0; m < M; m += 1) {
        for (d = 0; d < bins; d += 1) {
            dfold[d] = allbands[d + m * bins];
            dfoldd[d] = allbandsd[d + m * bins]; // dispersed version
            outdat[d] = 0;
        }
        psnr(bins, dfold, mbin, datout, pulw, outdat);
        for (d = 0; d < bins; d += 1) {
            puldat[m][d] = outdat[d];
            fprintf(fptpuld, "	%.2f", puldat[m][d]);
        }
        fprintf(fptpuld, "\n");
        thry = maxx * sqrt((double)m / (double)M);
        fprintf(fptsec, "%d	%.2f	%d	%.2f\n", m, datout[0], (int)datout[1],
                (float)thry); /* write txt data to the output text file */
        sectsnr[m][0] = m;
        sectsnr[m][1] = datout[0];
        sectsnr[m][2] = datout[1];
        psnr(bins, dfold, mbin, datout, pulw, outdat);
        for (d = 0; d < bins; d += 1) {
            puldatd[m][d] = (float)outdat[d];
            fprintf(fptpuldd, "  %.2f",
                    puldatd[m][d]); /* write dedispersed puldat data to the output text file */
        }
        fprintf(fptpuldd, "\n");
    }

    // Cumulative Section SNR - Build cumsec.txt
    for (d = 0; d < bins; d += 1) {
        dfold[d] = 0;
        dfoldd[d] = 0;
    }
    for (m = 0; m < M; m += 1) {
        for (d = 0; d < bins; d += 1) {
            dfold[d] = dfold[d] + allbands[d + m * bins];
            dfoldd[d] = dfoldd[d] + allbandsd[d + m * bins];
            dfolddd[d] = dfolddd[d] + allbandsdd[d + m * bins];
        }
        psnr(bins, dfold, mbin, datout, pulw, outdat);
        sectsnr[m][0] = m;
        sectsnr[m][1] = datout[0];
        sectsnr[m][2] = datout[1];
        for (d = 0; d < bins; d += 1) {
            foldat[m][d] = outdat[d];
            fprintf(fptfold, "%.2f  ", foldat[m][d]);
        }
        fprintf(fptfold, "\n");
        fprintf(fptcums, "%d	%.2f	%d\n", m, datout[0],
                (int)datout[1]); /* write cumulative band SNR txt data to the output text file */
        psnr(bins, dfold, mbin, datout, pulw, outdat);
        for (d = 0; d < bins; d += 1) {
            foldatd[m][d] = outdat[d];
            fprintf(fptfoldd, "%.2f  ", foldatd[m][d]); // dedispersed foldat
        }
        fprintf(fptfoldd, "\n");
        psnr(bins, dfold, mbin, datout, pulw, outdat);
        for (d = 0; d < bins; d += 1) {
            foldatdd[m][d] = outdat[d];
            fprintf(fptfolddd, "%.2f  ",
                    foldatdd[m][d]); // fprintf(fptfoldd,"\n"); // dedispersed foldat
        }
        fprintf(fptfolddd, "\n");
    }

    // Period Search Folds
    float pperiod;
    int s, ss, st;
    for (st = 0; st < 51; st++) {
        pperiod = bins * (1 + (st - 25) * 1 * ratio / 1000000 / nno1);
        for (s = 0; s < bins; s++) {
            sumt[s] = 0;
            count[s] = 1;
        }
        for (ss = 0; ss < M * bins; ss++) {
            ss = (double)ss;
            range = (long long int)(((double)ss / ((double)pperiod) -
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
    for (st = 0; st < 51; st += 1) {
        for (d = 0; d < bins; d += 1) {
            dfold[d] = outsumt[st][d];
        }
        psnr(bins, dfold, mbin, datout, pulw, outdat);
        if (datout[0] > maxx)
            maxx = datout[0];
    }
    for (st = 0; st < 51; st += 1) {
        for (d = 0; d < bins; d += 1) {
            dfold[d] = outsumt[st][d];
        }
        psnr(bins, dfold, mbin, datout, pulw, outdat);
        pspr = 3 + (maxx - 3) * perSch((double)(st - 25) * 1 / nno1, (numper), pulw, period);
        printf("ppm = %.2f	SNR =  %.2f	bin = %d\n", (float)(st - 25) / nno1, datout[0],
               (int)datout[1]);
        fprintf(fptper, " %.5f	%.5f	%d	%.3f\n", (float)(st - 25) / nno1, datout[0],
                (int)datout[1], pspr); /* write txt data to the output text file */
        if (st > 19 && st < 31) {
            for (d = 0; d < bins; d += 1) {
                fprintf(fptpfold, "%f	",
                        (float)outdat[d]); // target period folds over the selected period range
            }
            fprintf(fptpfold, "\n");
        }
    }
    printf("\n");

    // P-dot Search Fold
    printf("\n Period Rate Search, SNR v ppm/%d change \n", (int)numlog);
    double periodt;
    for (st = 0; st < 51; st++) {
        pperiod = bins * (1 + (25 - 25) * 1 * ratio / 1000000 / nno1);
        for (s = 0; s < bins; s++) {
            sumt[s] = 0;
            count[s] = 1;
        }
        for (ss = 0; ss < M * bins; ss++) {
            ss = (double)ss;
            periodt =
                pperiod *
                (1 + (double)(st - 25) * 1 *
                         (double)((ss * ratio / (double)(M * bins)) / (double)1000000 / nno1));
            range = (long long int)(((double)ss / ((double)periodt) -
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
    for (st = 0; st < 51; st += 1) {
        for (d = 0; d < bins; d += 1) {
            dfold[d] = outsumt[st][d];
        }
        psnr(bins, dfold, mbin, datout, pulw, outdat);
        pdpr = 3 + (maxx - 3) * pdSch((float)(st - 25) * 2 * numlog * .5 / (double)M / ratio / nno1,
                                      (numper), pulw, period, numlog);
        printf("pdot = %.2f	SNR =  %.2f	bin = %d\n",
               (float)(st - 25) * 2 * numlog / (double)M / ratio / 1 / nno1, datout[0],
               (int)datout[1]);
        fprintf(fptpd, " %.5f	%.5f	%d	%.5f\n",
                (float)(st - 25) * 2 * numlog / (double)M / ratio / 1 / nno1, datout[0],
                (int)datout[1], pdpr); /* write txt data to the output text file */
    }
    printf("\n");

    // Period / P-dot Search Fold. Build ppd2d.txt
    int sp;
    for (sp = 0; sp < 51; sp += 1) // p-dot range
    {
        for (st = 0; st < 51; st += 1) // period range
        {
            pperiod = bins * (1 + (sp - 25) * 1 * ratio / 1000000 / nno1);
            for (s = 0; s < bins; s++) {
                sumt[s] = 0;
                count[s] = 0;
                dfold[s] = 0;
            }
            for (ss = 0; ss < M * bins; ss++) {
                ss = (double)ss;
                periodt =
                    pperiod *
                    (1 + (double)(st - 25) * 1 *
                             (double)((ss * ratio / (double)(M * bins)) / (double)1000000 / nno1));
                range = (long long int)(((double)ss / ((double)periodt) -
                                         (long long int)((double)ss / ((double)periodt))) *
                                        ((double)bins));
                sumt[range] = sumt[range] + allbandsd[ss];
                count[range] = count[range] + 1;
            }
            for (s = 0; s < bins; s++) {
                dfold[s] = 100 * sumt[s] / count[s];
            }
            psnr(bins, dfold, mbin, datout, pulw, outdat);
            ppdot[sp][st] = datout[0];
            fprintf(fptppd, "%.2f	 ", ppdot[sp][st]); /* write txt data to the output text file */
        }
        fprintf(fptppd, "\n");
    }

    // Build secavsnr.txt - Rolling Window/Average SNR
    int xx, mp, nxx = 0;
    float max = 0, pkmax = 0;
    float bestprof[4096];
    int span = 0, centre = 0;

    for (mp = 1; mp < M + 1; mp += 1) {
        if (mp == rolav)
            printf("Set Section Rolling Average Window %d \n", mp);
        datout[0] = 0;
        max = 0;

        for (m = 0; m < M - mp + 1; m += 1) {
            for (d = 0; d < bins; d += 1) {
                dfold[d] = 0;
            }
            for (xx = 0; xx < mp + 0; xx += 1) {
                for (d = 0; d < bins; d += 1) {
                    dfold[d] = dfold[d] + allbandsd[d + (m + xx) * bins];
                }
            }
            psnr(bins, dfold, mbin, datout, pulw, outdat);
            if (outdat[(int)datout[1]] > pkmax) {
                pkmax = outdat[(int)datout[1]];
                centre = m + mp / 2;
                span = mp;

                for (d = 0; d < bins; d += 1) {
                    pkdat[d] = outdat[d];
                }
            }
            if (datout[0] > max) {
                max = datout[0];

                nxx = datout[1];
                if (mp == rolav) {
                    for (d = 0; d < bins; d += 1) {
                        bestprof[d] = outdat[d];
                    }
                }
            }
            if (mp == rolav) {
                secavsnr[m][0] = m + (int)(mp / 2);
                secavsnr[m][1] = datout[0];
                secavsnr[m][2] = datout[1];
                fprintf(fptavsec, "%d	%.2f	%d 	%.2f\n", m + (int)(mp / 2), secavsnr[m][1],
                        (int)secavsnr[m][2],
                        datout[6]); /* write txt data to the output text file */
            }
            cco = cco + 1;
            for (d = 0; d < bins; d += 1) {
                fprintf(fptavfol, "%f ", outdat[d]);
            }
            fprintf(fptavfol, "\n");
        }
        if (mp > 0)
            fprintf(fptrol, "%d	%.2f	%d\n", mp, max,
                    nxx); // rolling section block average peak SNR per section
    }
    printf("Period %f	No: bins = %d\n", period, (int)bins);

    // Build Best Result text file
    fprintf(fpttext, "Best Section Range SNR = %.2f	Section Centre %d	Best Section Range %d\n",
            (float)pkmax, centre, span);
    printf("Max SNR  %f	Spanned Sections = %d	 Section Centre = %d\n", pkmax, span, centre);

    // Build  profile.txt - output Pulse Profile text file
    printf("\n Analysed Data Output Files: \n");
    for (d = 0; d < bins; d += 1) {
        dfold[d] = outsumt[25][d]; // outsumt/outdat[d] from p-dot fold with zero pdot
    }
    // bestprof[d] best at rolling average setting. Rolling average peak = pkdat.
    psnr(bins, dfold, mbin, datout, pulw, outdat);
    for (d = 0; d < bins; d += 1) {
        fprintf(fptprof, "%.1f	%f	%f	%f\n", (float)d, (float)outdat[d], bestprof[d],
                pkdat[d]); /* write txt data to the output text file */
    }

    // Build allbands.txt - compressed, match-filtered band-combined data text file
    for (m = 0; m < M; m += 1) {
        for (c = 0; c < (int)(bins); c += 1) {
            fprintf(fptallb, "%f %f %f\n", (float)(allbands[c + m * bins]),
                    (float)(allbandsd[c + m * bins]),
                    (float)(allbandsdd[c + m * bins])); /* write txt data to the output text file */
            fprintf(fptspallb, "%f	%f\n", ((float)(c + m * bins) * 1000 / period / M),
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
    fclose(fptprof);
    fclose(fptdms);
    fclose(fptdmns);
    fclose(fptper);
    fclose(fptpd);
    fclose(fptbnd);
    fclose(fptbndd);
    fclose(fptbndc);
    fclose(fptcumf);
    fclose(fptcums);
    fclose(fptsec);
    fclose(fptavsec);
    fclose(fptpuld);
    fclose(fptpuldd);
    fclose(fptallb);
    fclose(fptspallb);
    fclose(fptfold);
    fclose(fptfoldd);
    fclose(fptfolddd);
    fclose(fptppd);
    fclose(fptout);
    fclose(fptraw);
    fclose(fptdmprf);
    fclose(fptavfol); // OVDE SE KRSHI, duplo ga zatvara negdje gore u kodu u ekoj metodi
    fclose(fpttext);
    fclose(fptmax);
    fclose(fptpfold);
    fclose(fptdmfold);
    fclose(fptfreq);
    fclose(fptcut);
    fclose(fptdat);
    exit(0);
}
