#define BLD_IMPLEMENTATION
#include "../../bld.h"

#define INCL "-I../../"

int
build_spiral(int argc, char *argv[])
{
    const char *out = "spiral";

    printf("%s:\n", out);

    int res = BLD_CC(INCL, "spiral.c", "-o", out);
    if (res) return res;

    if (bld_contains("run", argc, argv))
        return bld_run_program(out);

    return 0;
}

int
main(int argc, char *argv[])
{
    BLD_TRY_REBUILD_SELF(argc, argv);

    if (bld_contains("spiral", argc, argv))
        return build_spiral(argc, argv);

    // default target
    return build_spiral(argc, argv);
}

