
#include "../../includes/io/outputs.h"

FILE **open_files() {
    FILE **result = (FILE **)malloc(NUM_OF_FILES * sizeof(FILE *));

    for (int i = 0; i < NUM_OF_FILES; i++) {
        char *str = NULL, *cmd = "w";
        switch (i) {
        case FPT_BLNKF:
            str = "Blankf.txt";
            cmd = "r";
            break;
        case FPT_BLNKS:
            str = "Blanks.txt";
            cmd = "r";
            break; 
        case FPT_PROF:
            str = "profile.txt";
            break;
        case FPT_DMS:
            str = "dmSearch.txt";
            break;
        case FPT_DMNS:
            str = "dmSrchns.txt";
            break;
        case FPT_PER:
            str = "periodS.txt";
            break;
        case FPT_PD:
            str = "pdotS.txt";
            break;
        case FPT_BND:
            str = "bandS.txt";
            break;
        case FPT_BNDD:
            str = "bandat.txt";
            break;
        case FPT_BNDC:
            str = "bandcum.txt";
            break;
        case FPT_CUMF:
            str = "cumbands.txt";
            break;
        case FPT_CUMS:
            str = "cumsec.txt";
            break;
        case FPT_SEC:
            str = "secsnr.txt";
            break;
        case FPT_AVSEC:
            str = "secavsnr.txt";
            break;
        case FPT_AVFOL:
            str = "secavfol.txt";
            break;
        case FPT_PULD:
            str = "puldat.txt";
            break;
        case FPT_ALLB:
            str = "allbands.txt";
            break;
        case FPT_SPALLB:
            str = "spallbands.txt";
            break;
        case FPT_FOLD:
            str = "foldat.txt";
            break;
        case FPT_FOLDD:
            str = "foldatd.txt";
            break;
        case FPT_FOLDDD:
            str = "foldatdd.txt";
            break;
        case FPT_PPD:
            str = "ppd2d.txt";
            break;
        case FPT_OUT:
            str = "outdat.txt";
            break;
        case FPT_RAW:
            str = "rawdat.txt";
            break;
        case FPT_CUT:
            str = "cutdat.bin";
            cmd = "wb";
            break;
        case FPT_DMPRF:
            str = "dmprf.txt";
            break;
        case FPT_ROL:
            str = "secavrol.txt";
            break;
        case FPT_TEXT:
            str = "header.txt";
            break;
        case FPT_MAX:
            str = "max.txt";
            break;
        case FPT_PFOLD:
            str = "pfold.txt";
            break;
        case FPT_PULDD:
            str = "puldatd.txt";
            break;
        case FPT_DMFOLD:
            str = "dmfold.txt";
            break;
        case FPT_FREQ:
            str = "spectrum.txt";
            break;
        case FPT_DAT:
            str = "data.txt";
            break;
        default:
            printf("Error\n");
        }
        result[i] = fopen(str, cmd);
    }
    return result;
}
