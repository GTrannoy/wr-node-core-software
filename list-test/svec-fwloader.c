#include "fmc-lib.h"

main(int argc, char *argv[])
{

    struct fmc_dev *fmc = fmc_svec_create(0);

    fmc->reprogram(fmc, argv[1]);
    return 0;
}
