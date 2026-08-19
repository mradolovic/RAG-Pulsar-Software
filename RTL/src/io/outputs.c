#include "../../includes/io/outputs.h"

void print_outputs(char *input) {
    // print output file function and names
    printf("\n Input file: = %s\n", input);
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
}
