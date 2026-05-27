#ifndef __OUTPUTS__
#define __OUTPUTS__
#include <stdio.h>
#include <stdlib.h>

// This definition needs to be up to date
#define NUM_OF_FILES 34

typedef enum OUTPUT_FILES {
    FPT_BLNKF,
    FPT_BLNKS,
    FPT_PROF,
    FPT_DMS,
    FPT_DMNS,
    FPT_PER,
    FPT_PD,
    FPT_BND,
    FPT_BNDD,
    FPT_BNDC,
    FPT_CUMF,
    FPT_CUMS,
    FPT_SEC,
    FPT_AVSEC,
    FPT_AVFOL,
    FPT_PULD,
    FPT_ALLB,
    FPT_SPALLB,
    FPT_FOLD,
    FPT_FOLDD,
    FPT_FOLDDD,
    FPT_PPD,
    FPT_OUT,
    FPT_RAW,
    FPT_CUT,
    FPT_DMPRF,
    FPT_ROL,
    FPT_TEXT,
    FPT_MAX,
    FPT_PFOLD,
    FPT_PULDD,
    FPT_DMFOLD,
    FPT_FREQ,
    FPT_DAT
} OUTPUT_FILES;

FILE **open_files();
void close_files(FILE **files);
#endif
