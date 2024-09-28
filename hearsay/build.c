#define BLD_IMPLEMENTATION
#include "core/bld.h"

#include "core/utils.h"

i32
main(i32 argc, char *argv[])
{
    BLD_TRY_REBUILD_SELF(argc, argv);
    char *output = "program";

    bld_sa_t cc = {0};

    BLD_SA_PUSH(cc, "src/main.c", "../raylib/lib/libraylib.a");

    BLD_SA_PUSH(cc, "-I.", "-Isrc", "-I../raylib/include", "-o", output, BLD_WARNINGS);

    BLD_SA_PUSH(cc, "-lm", "-ldl", "-lpthread");

    if (bld_contains("debug", argc, argv)) {
        BLD_SA_PUSH(cc, "-D_DEBUG");
    }

    u32 res = bld_cc_params((const char **)cc.data, cc.count);
    if (res != 0)
        return res;

    if (bld_contains("run", argc, argv)) {
        return bld_run_program(output);
    }

    return 0;
}
