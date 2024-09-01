#define BLD_IMPLEMENTATION
#include "../bld.h"

#include <stdio.h>

int
main(int argc, char *argv[])
{
    const char *out_name = "program";

    if (argc > 1) {
        out_name = argv[2];
    }

    FILE *file = fopen("build.c", "w");
    if (file == NULL) {
        fprintf(stderr, "Failed to open file 'build.c'!\n");
        return 1;
    }

    fprintf(file,
        "#define BLD_IMPLEMENTATION\n"
        "#include \"core/bld.h\"\n"
        "\n"
        "#include \"core/utils.h\"\n"
        "\n"
        "i32\n"
        "main(i32 argc, char *argv[])\n"
        "{\n"
        "    BLD_TRY_REBUILD_SELF(argc, argv);\n"
        "    char *output = \"%s\";\n"
        "\n"
        "    bld_sa_t cc = {0};\n"
        "\n"
        "    BLD_SA_PUSH(cc, \"src/main.c\");\n"
        "\n"
        "    BLD_SA_PUSH(cc, \"-I.\", \"-Isrc\", \"-o\", output, BLD_WARNINGS);\n"
        "\n"
        "    if (bld_contains(\"debug\", argc, argv)) {\n"
        "        BLD_SA_PUSH(cc, \"-D_DEBUG\");\n"
        "    }\n"
        "\n"
        "    u32 res = bld_cc_params((const char **)cc.data, cc.count);\n"
        "    if (res != 0)\n"
        "        return res;\n"
        "\n"
        "    if (bld_contains(\"run\", argc, argv)) {\n"
        "        return bld_run_program(output);\n"
        "    }\n"
        "\n"
        "    return 0;\n"
        "}\n"
        ,
        out_name
    );

    fclose(file);

    bld_directory_assure("src");

    file = fopen("src/main.c", "w");
    if (file == NULL) {
        fprintf(stderr, "Failed to create file 'src/main.c'!\n");
        return 1;
    }

    fprintf(file,
        "#include \"core/utils.h\"\n"
        "\n"
        "#include <stdio.h>\n"
        "\n"
        "i32\n"
        "main(void)\n"
        "{\n"
        "    printf(\"\\\\_/\\n V\\n\");\n"
        "    return 0;\n"
        "}\n"
    );

    fclose(file);

    return BLD_CC("build.c", "-o", "build");
}
