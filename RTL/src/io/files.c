#include "../../includes/io/files.h"

#include <errno.h>
#include <string.h>

FILE **open_files(void) {
    /* calloc rather than malloc: FPT_BLNKF and FPT_BLNKS are skipped by the
       loop below, so their slots were left holding uninitialised heap. */
    FILE **result = (FILE **)calloc(NUM_OF_FILES, sizeof(FILE *));
    if (result == NULL) {
        fprintf(stderr, "Out of memory allocating the output file table.\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < NUM_OF_FILES; i++) {
        char *str = NULL, *cmd = "w";
        switch (i) {
        case FPT_BLNKF:
            continue;
            break;
        case FPT_BLNKS:
            continue;
            break;
        case FPT_PROF: // pulse profile + peak rolling average profile
            str = "profile.txt";
            break;
        case FPT_DMS: // DM search scan
            str = "dmSearch.txt";
            break;
        case FPT_DMNS: // DM search with pulsar target blanked
            str = "dmSrchns.txt";
            break;
        case FPT_PER: // period search scan
            str = "periodS.txt";
            break;
        case FPT_PD: // pdot search scan
            str = "pdotS.txt";
            break;
        case FPT_BND: // band search max scan
            str = "bandS.txt";
            break;
        case FPT_BNDD: // band folds
            str = "bandat.txt";
            break;
        case FPT_BNDC: // cumulative band folds
            str = "bandcum.txt";
            break;
        case FPT_CUMF: // cumulative SNR of band search
            str = "cumbands.txt";
            break;
        case FPT_CUMS: // cumulative SNR of section search
            str = "cumsec.txt";
            break;
        case FPT_SEC: // SNR of individual sections
            str = "secsnr.txt";
            break;
        case FPT_AVSEC: // rolling average section SNR
            str = "secavsnr.txt";
            break;
        case FPT_AVFOL: // Section average folds
            str = "secavfol.txt";
            break;
        case FPT_PULD: // individual section SNR folds
            str = "puldat.txt";
            break;
        case FPT_ALLB: // partial folded data bands combined
            str = "allbands.txt";
            break;
        case FPT_SPALLB: // partial folded spectrum bands combined
            str = "spallbands.txt";
            break;
        case FPT_FOLD: // cumulative SNR of bins
            str = "foldat.txt";
            break;
        case FPT_FOLDD: // cumulative SNR of bins, dedispered SNR
            str = "foldatd.txt";
            break;
        case FPT_FOLDDD: // cumulative SNR of bins, dedispered SNR
            str = "foldatdd.txt";
            break;
        case FPT_PPD: // 2D period/p-dot plot
            str = "ppd2d.txt";
            break;
        case FPT_OUT: // matched-filtered output data - continuous stream
            str = "outdat.txt";
            break;
        case FPT_RAW: // raw compressed partial folded data
            str = "rawdat.txt";
            break;
        case FPT_CUT: // data file cut between start and end sections
            str = "cutdat.bin";
            cmd = "wb";
            break;
        case FPT_DMPRF: // Optimum DM - best profile
            str = "dmprf.txt";
            break;
        case FPT_ROL: // Section variable block average fold peak
            str = "secavrol.txt";
            break;
        case FPT_TEXT: // Header for Python program
            str = "header.txt";
            break;
        case FPT_MAX: // maximum bin and SNR
            str = "max.txt";
            break;
        case FPT_PFOLD: // period search folds
            str = "pfold.txt";
            break;
        case FPT_PULDD: // individual section Dispersed SNR folds
            str = "puldatd.txt";
            break;
        case FPT_DMFOLD: // dm search folds
            str = "dmfold.txt";
            break;
        case FPT_FREQ: // input data FFT spectrum
            str = "spectrum.txt";
            break;
        case FPT_DAT: // command line settings*/
            str = "data.txt";
            break;
        default:
            fprintf(stderr, "Internal error: no filename defined for output slot %d\n", i);
            exit(EXIT_FAILURE);
        }
        result[i] = fopen(str, cmd);
        /* Not one of these 32 opens was checked. If the working directory is
           not writable, or the disk is full, or the open-file limit is hit,
           the first fprintf to a NULL handle segfaulted - after the program
           had already printed its whole banner, so it looked like a crash
           deep inside the analysis rather than a permissions problem. */
        if (result[i] == NULL) {
            fprintf(stderr, "Cannot create output file '%s' in the working directory: %s\n", str,
                    strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
    return result;
}

void close_files(FILE **files) {
    for (int i = 0; i < NUM_OF_FILES; i++) {
        /* NULL check as well as the index check: fclose(NULL) is undefined,
           and would previously have compounded whatever went wrong first. */
        if (i != FPT_BLNKF && i != FPT_BLNKS && files[i] != NULL) {
            fclose(files[i]);
        }
    }
    free(files);
    return;
}
