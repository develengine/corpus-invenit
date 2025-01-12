#ifndef BLD_H_
#define BLD_H_

/*
 * Yeah, this is directly inspired by [tsoding's](https://github.com/tsoding) nobuild/nob.
 */

/* TODO:
 * [ ] Delimeter aware contains and find.
 * [ ] Run that extracts arguments from other by delimeter.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <errno.h>


#if !defined(_WIN32)
    #include <sys/wait.h>
    #include <sys/stat.h>
    #include <unistd.h>

    typedef struct timespec file_time_t;
#else // !defined(_WIN32)
    #include <windows.h>

    typedef HANDLE   pid_t;
    typedef FILETIME file_time_t;
#endif // !defined(_WIN32)


typedef struct
{
    char *data;
    size_t size, capacity;
} bld_sb_t;

typedef struct
{
    char **data;
    size_t count, capacity;
} bld_sa_t;

typedef enum
{
    bld_path_None      = (1 << 0),
    bld_path_File      = (1 << 1),
    bld_path_Directory = (1 << 2),
} bld_path_info_t;


int
bld_wait_for_multiple(pid_t *pids, int count);

int
bld_wait_for(pid_t pid);

int
bld_exists(const char *path);

unsigned
bld_get_path_info(const char *path);

int
bld_directory_create(const char *path);

void
bld_directory_assure(const char *path);

int
bld_file_time_cmp(file_time_t a, file_time_t b);

file_time_t
bld_mod_time(const char *path);

pid_t
bld_execute_vargs_async(const char *fmt, va_list args);

pid_t
bld_execute_argv_async(char *argv[]);

int
bld_execute_argv(char *argv[]);

pid_t
bld_execute_str_async(const char *str);

int
bld_execute_str(const char *str);

pid_t
bld_execute_async(const char *fmt, ...);

int
bld_execute(const char *fmt, ...);

pid_t
bld_run_vargs_async(const char *name, const char *fmt, va_list args);

pid_t
bld_run_argv_async(const char *name, char *argv[]);

int
bld_run_argv(const char *name, char *argv[]);

pid_t
bld_run_str_async(const char *name, const char *str);

int
bld_run_str(const char *name, const char *str);

pid_t
bld_run_async(const char *name, const char *fmt, ...);

int
bld_run(const char *name, const char *fmt, ...);

pid_t
bld_run_program_async(const char *name);

int
bld_run_program(const char *name);

char *
bld_strf(const char *fmt, ...);

int
bld_str_eq(const char *str_a, const char *str_b);

int
bld_contains(const char *str, int argc, char *argv[]);

int
bld_find(const char *str, int count, char **list);

char *
bld_join(const char *delim, int count, const char **list);

pid_t
bld_cc_params_async(const char **params, int param_count);

int
bld_cc_params(const char **params, int param_count);

pid_t
bld_cc_params_nt_async(const char **params);

int
bld_cc_params_nt(const char **params);

pid_t
bld_cc_pro_async(const char *out_name,
                 char **src,   int src_count,
                 char **libs,  int lib_count,
                 char **inc,   int inc_count,
                 char **defs,  int def_count,
                 char **warns, int warn_count,
                 char **raw,   int raw_count);

int
bld_cc_pro(const char *out_name,
           char **src,   int src_count,
           char **libs,  int lib_count,
           char **inc,   int inc_count,
           char **defs,  int def_count,
           char **warns, int warn_count,
           char **raw,   int raw_count);

void
bld_sb_append(bld_sb_t *sb, const char *s);

void
bld_sb_append_multiple(bld_sb_t *sb, const char **sl, int count);

void
bld_sb_append_multiple_nt(bld_sb_t *sb, const char **sl);

int
bld_try_rebuild_self(char *self, int argc, char *argv[]);

void
bld_sa_push_multiple(bld_sa_t *sa, char **data, int count);

void
bld_sa_push_multiple_nt(bld_sa_t *sa, char **data);


#define BLD_LEN(a) (sizeof(a) / sizeof(*(a)))

#define BLD_SB_APPEND(sb, ...) \
    bld_sb_append_multiple_nt(sb, ((const char*[]) { __VA_ARGS__, NULL }))

#define BLD_SA_PUSH(sa, ...) \
    bld_sa_push_multiple_nt(&(sa), ((char*[]) { __VA_ARGS__, NULL }))

#define BLD_CC(...) \
    bld_cc_params_nt(((const char*[]){ __VA_ARGS__, NULL }))

#define BLD_CC_ASYNC(...) \
    bld_cc_params_nt_async(((const char*[]){ __VA_ARGS__, NULL }))


#define BLD_GCC_WARNINGS "-Wall", "-Wextra", "-Wpedantic", \
                         "-Wno-deprecated-declarations", "-Wno-missing-field-initializers", \
                         "-Wno-missing-braces"

#define BLD_GCC_WARNINGS_PRO "all", "extra", "pedantic", \
                             "no-deprecated-declarations", "no-missing-field-initializers", \
                             "no-missing-braces"

#define BLD_MSVC_WARNINGS "-W4"

#if !defined(_WIN32)
    #define BLD_WARNINGS BLD_GCC_WARNINGS
#else
    #define BLD_WARNINGS BLD_MSVC_WARNINGS
#endif


#define BLD_STRETCHY_T(data_type, size_type) struct { data_type *data; size_type count, capacity; }

#define BLD_STRETCHY_FOR(_dck, _type, _elem) \
    for (_type *_elem = (_dck).data; _elem < (_dck).data + (_dck).count; ++_elem)

/* argument must be an 'lvalue', except for `...` */
#define BLD_STRETCHY_PUSH(_dck, ...)                                                    \
do {                                                                                    \
    if ((_dck).count == (_dck).capacity) {                                              \
        (_dck).capacity = (_dck).capacity ? (_dck).capacity * 2                         \
                                          : 4096 / sizeof(*((_dck).data));              \
        (_dck).data = realloc((_dck).data, sizeof(*((_dck).data)) * (_dck).capacity);   \
        assert((_dck).data);                                                            \
    }                                                                                   \
    (_dck).data[(_dck).count] = __VA_ARGS__;                                            \
    (_dck).count++;                                                                     \
} while (0)

/* argument must be an 'lvalue', except for `amount` */
#define BLD_STRETCHY_RESERVE(_dck, _amount)                                             \
do {                                                                                    \
    if ((_dck).count + (_amount) > (_dck).capacity) {                                   \
        if ((_dck).capacity == 0) {                                                     \
            (_dck).capacity = 4096 / sizeof(*((_dck).data));                            \
        }                                                                               \
        while ((_dck).count + (_amount) > (_dck).capacity) {                            \
            (_dck).capacity *= 2;                                                       \
        }                                                                               \
        (_dck).data = realloc((_dck).data, sizeof(*((_dck).data)) * (_dck).capacity);   \
        assert((_dck).data);                                                            \
    }                                                                                   \
} while (0)

#define BLD_TRY_REBUILD_SELF(_argc, _argv) \
do { \
    int _res = bld_try_rebuild_self(__FILE__, _argc, _argv); \
    if (_res != -1) \
        exit(_res); \
} while (0)


#if defined(BLD_IMPLEMENTATION)

#if !defined(_WIN32)

int
bld_wait_for(pid_t pid)
{
    siginfo_t info;

    if (waitid(P_ALL, pid, &info, WEXITED) == -1) {
        fprintf(stderr, "waitid(): %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    return info.si_status;
}

int
bld_wait_for_multiple(pid_t *pids, int count)
{
    for (int i = 0; i < count; ++i) {
        int ret = bld_wait_for(pids[i]);

        if (ret)
            return ret;
    }

    return 0;
}

pid_t
bld_execute_str_async(const char *str)
{
    pid_t pid = fork();

    if (pid == -1) {
        fprintf(stderr, "fork(): %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        execl("/bin/sh", "sh", "-c", str, NULL);

        fprintf(stderr, "execve(): %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    return pid;
}

int
bld_exists(const char *path)
{
    return access(path, F_OK) == 0;
}

unsigned
bld_get_path_info(const char *path)
{
    struct stat s;

    if (stat(path, &s) != 0)
        return bld_path_None;

    unsigned res = 0;

    if (S_ISDIR(s.st_mode)) {
        res |= bld_path_Directory;
    }
    else if (S_ISREG(s.st_mode)) {
        res |= bld_path_File;
    }
}

int
bld_directory_create(const char *path)
{
    if (mkdir(path, 0777)) {
        fprintf(stderr, "mkdir(): %s\n", strerror(errno));
        return 0;
    }

    return 1;
}

pid_t
bld_execute_argv_async(char *argv[])
{
    pid_t pid = fork();

    if (pid == -1) {
        fprintf(stderr, "fork(): %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        execv(argv[0], argv);

        fprintf(stderr, "execv(): %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    return pid;
}

int
bld_file_time_cmp(file_time_t a, file_time_t b)
{
    int res = (a.tv_sec > b.tv_sec) - (a.tv_sec < b.tv_sec);

    if (res == 0)
        return (a.tv_nsec > b.tv_nsec) - (a.tv_nsec < b.tv_nsec);

    return res;
}

file_time_t
bld_mod_time(const char *path)
{
    struct stat st = {0};

    if (lstat(path, &st)) {
        fprintf(stderr, "lstat(): %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    return (file_time_t) { st.st_mtime };
}

pid_t
bld_cc_params_async(const char **params, int param_count)
{
    bld_sb_t sb = {0};

    bld_sb_append(&sb, "cc");

    for (int i = 0; i < param_count; ++i) {
        int len = strlen(params[i]);

        if (len > 3 && params[i][0] == '-') {
            if (params[i][1] == 'R') {
                bld_sb_append(&sb, " ");
                bld_sb_append(&sb, params[i] + 2);
                continue;
            }
        }

        bld_sb_append(&sb, " ");
        bld_sb_append(&sb, params[i]);
    }

    pid_t pid = bld_execute_str_async(sb.data);

    free(sb.data);

    return pid;
}

pid_t
bld_run_argv_async(const char *name, char *argv[])
{
    char *prog = bld_strf("./%s");

    BLD_STRETCHY_T (const char *, int) new_argv = {0};

    BLD_STRETCHY_PUSH(new_argv, prog);

    for (int i = 0; argv[i]; ++i) {
        BLD_STRETCHY_PUSH(new_argv, argv[i]);
    }

    BLD_STRETCHY_PUSH(new_argv, NULL);


    pid_t pid = bld_execute_argv_async((char**)new_argv.data);

    free(prog);
    free(new_argv.data);

    return pid;
}

pid_t
bld_run_str_async(const char *name, const char *str)
{
    char *new_str = bld_strf("./%s %s", name, str);

    pid_t pid = bld_execute_str_async(new_str);

    free(new_str);

    return pid;
}

pid_t
bld_run_vargs_async(const char *name, const char *fmt, va_list args)
{
    char *new_fmt = bld_strf("./%s %s", name, fmt);

    pid_t pid = bld_execute_vargs_async(new_fmt, args);

    free(new_fmt);

    return pid;
}


#else // !defined(_WIN32)

int
bld_wait_for(pid_t pid)
{
    if (WaitForSingleObject(pid, INFINITE) == WAIT_FAILED) {
        fprintf(stderr, "WaitForSingleObject(): (%d)\n", GetLastError());
        exit(EXIT_FAILURE);
    }

    int code;

    if (!GetExitCodeProcess(pid, &code)) {
        fprintf(stderr, "GetExitCodeProcess(): (%d)\n", GetLastError());
        exit(EXIT_FAILURE);
    }

    return code;
}

int
bld_wait_for_multiple(pid_t *pids, int count)
{
    if (WaitForMultipleObjects(count, pids, TRUE, INFINITE) == WAIT_FAILED) {
        fprintf(stderr, "WaitForMultipleObjects(): (%d)\n", GetLastError());
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < count; ++i) {
        int code;

        if (!GetExitCodeProcess(pids[i], &code)) {
            fprintf(stderr, "GetExitCodeProcess(): (%d)\n", GetLastError());
            exit(EXIT_FAILURE);
        }

        if (code)
            return code;
    }

    return 0;
}

pid_t
bld_execute_str_async(const char *str)
{
    char *buffer = bld_strf("cmd.exe /c \"%s\"", str);

    STARTUPINFO startup_info = {0};
    startup_info.cb = sizeof(startup_info);

    PROCESS_INFORMATION process_info = {0};

    if (!CreateProcessA(
        "C:\\Windows\\System32\\cmd.exe",
        buffer,
        NULL, NULL,
        FALSE,
        0,
        NULL, NULL,
        &startup_info,
        &process_info))
    {
        fprintf(stderr, "CreateProcessA(): (%d)\n", GetLastError());
        exit(EXIT_FAILURE);
    }

    free(buffer);

    return process_info.hProcess;
}

pid_t
bld_execute_argv_async(char *argv[])
{
    int size = 0;
    int count = 0;

    for (int i = 0; argv[i]; ++i, ++count) {
        size += strlen(argv[i]) + 1;
    }

    char *buffer = malloc(size);
    assert(buffer);

    int pos = 0;

    for (int i = 0; i < count; ++i) {
        int len = strlen(argv[i]);
        memcpy(buffer + pos, argv[i], len);
        buffer[pos + len] = (i == count - 1 ? '\0' : ' ');
        pos += len + 1;
    }

    STARTUPINFO startup_info = {0};
    startup_info.cb = sizeof(startup_info);

    PROCESS_INFORMATION process_info = {0};

    if (!CreateProcessA(
        NULL,
        buffer,
        NULL, NULL,
        FALSE,
        0,
        NULL, NULL,
        &startup_info,
        &process_info))
    {
        fprintf(stderr, "CreateProcessA(): (%d)\n", GetLastError());
        exit(EXIT_FAILURE);
    }

    return process_info.hProcess;
}

int
bld_exists(const char *path)
{
    WIN32_FIND_DATAA data = {0};

    HANDLE handle = FindFirstFileA(path, &data);

    if (handle == INVALID_HANDLE_VALUE) {
        int err = GetLastError();

        if (err == ERROR_FILE_NOT_FOUND)
            return 0;

        fprintf(stderr, "FindFirstFileA(%s): (%d)\n", path, err);
        exit(EXIT_FAILURE);
    }

    if (!FindClose(handle)) {
        fprintf(stderr, "FindClose(): (%d)\n", GetLastError());
        exit(EXIT_FAILURE);
    }

    return 1;
}

unsigned
bld_get_path_info(const char *path)
{
    unsigned ret = GetFileAttributesA(path);

    if (ret == INVALID_FILE_ATTRIBUTES)
        return bld_path_None;

    unsigned res = 0;

    if (ret & FILE_ATTRIBUTE_DIRECTORY) {
        res |= bld_path_Directory;
    } else {
        res |= bld_path_File;
    }

    return res;
}

int
bld_directory_create(const char *path)
{
    int res = CreateDirectoryA(path, NULL);

    if (!res) {
        fprintf(stderr, "CreateDirectoryA(): (%d)\n", GetLastError());
    }

    return res != 0;
}

int
bld_file_time_cmp(file_time_t a, file_time_t b)
{
    return CompareFileTime(&a, &b);
}

file_time_t
bld_mod_time(const char *path)
{
    WIN32_FIND_DATAA data = {0};

    HANDLE handle = FindFirstFileA(path, &data);

    if (handle == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "FindFirstFileA(): (%d)\n", GetLastError());
        exit(EXIT_FAILURE);
    }

    if (!FindClose(handle)) {
        fprintf(stderr, "FindClose(): (%d)\n", GetLastError());
        exit(EXIT_FAILURE);
    }

    return data.ftLastWriteTime;
}

pid_t
bld_cc_params_async(const char **params, int param_count)
{
    bld_sb_t sb = {0};

    bld_sb_append(&sb, "cl");

    for (int i = 0; i < param_count; ++i) {
        if (bld_str_eq(params[i], "-o")) {
            if (++i < param_count) {
                bld_sb_append(&sb, " /Fe");
                bld_sb_append(&sb, params[i]);
            }
            continue;
        }

        int len = strlen(params[i]);

        if (len > 3 && params[i][0] == '-') {
            if (params[i][1] == 'l') {
                bld_sb_append(&sb, " ");
                bld_sb_append(&sb, params[i] + 2);
                continue;
            }

            if (params[i][1] == 'I') {
                bld_sb_append(&sb, " /I");
                bld_sb_append(&sb, params[i] + 2);
                continue;
            }

            if (params[i][1] == 'D') {
                bld_sb_append(&sb, " /D");
                bld_sb_append(&sb, params[i] + 2);
                continue;
            }

            if (params[i][1] == 'W') {
                bld_sb_append(&sb, " /W");
                bld_sb_append(&sb, params[i] + 2);
                continue;
            }

            if (params[i][1] == 'R') {
                bld_sb_append(&sb, " ");
                bld_sb_append(&sb, params[i] + 2);
                continue;
            }

        }

        bld_sb_append(&sb, " ");
        bld_sb_append(&sb, params[i]);
    }

    pid_t pid = bld_execute_str_async(sb.data);

    free(sb.data);

    return pid;
}

pid_t
bld_run_argv_async(const char *name, char *argv[])
{
    char *prog = bld_strf("%s.exe");

    BLD_STRETCHY_T (const char *, int) new_argv = {0};

    BLD_STRETCHY_PUSH(new_argv, prog);

    for (int i = 0; argv[i]; ++i) {
        BLD_STRETCHY_PUSH(new_argv, argv[i]);
    }

    BLD_STRETCHY_PUSH(new_argv, NULL);


    pid_t pid = bld_execute_argv_async((char**)new_argv.data);

    free(prog);
    free(new_argv.data);

    return pid;
}

pid_t
bld_run_str_async(const char *name, const char *str)
{
    char *new_str = bld_strf("%s.exe %s", name, str);

    pid_t pid = bld_execute_str_async(new_str);

    free(new_str);

    return pid;
}

pid_t
bld_run_vargs_async(const char *name, const char *fmt, va_list args)
{
    char *new_fmt = bld_strf("%s.exe %s", name, fmt);

    pid_t pid = bld_execute_vargs_async(new_fmt, args);

    free(new_fmt);

    return pid;
}

#endif // !defined(_WIN32)


void
bld_directory_assure(const char *path)
{
    unsigned ret = bld_get_path_info(path);

    if (ret & bld_path_None) {
        if (!bld_directory_create(path)) {
            fprintf(stderr, "Failed to create the directory! \"%s\"\n", path);
            exit(1);
        }
    }
    else if (ret & bld_path_File) {
        fprintf(stderr, "Path already exists as a file! \"%s\"\n", path);
        exit(1);
    }
}

pid_t
bld_execute_vargs_async(const char *fmt, va_list args)
{
    va_list args_copy;
    va_copy(args_copy, args);

    int to_write = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    char *buffer = malloc(to_write + 1);
    assert(buffer);

    int written = vsnprintf(buffer, to_write + 1, fmt, args_copy);
    va_end(args_copy);

    assert(written == to_write);

    pid_t pid = bld_execute_str_async(buffer);

    free(buffer);

    return pid;
}

int
bld_execute_argv(char *argv[])
{
    return bld_wait_for(bld_execute_argv_async(argv));
}

pid_t
bld_execute_async(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    pid_t pid = bld_execute_vargs_async(fmt, args);

    va_end(args);

    return pid;
}

int
bld_execute(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    pid_t pid = bld_execute_vargs_async(fmt, args);

    va_end(args);

    return bld_wait_for(pid);
}

char *
bld_strf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    va_list args_copy;
    va_copy(args_copy, args);

    int to_write = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    va_end(args);

    char *buffer = malloc(to_write + 1);
    assert(buffer);

    vsnprintf(buffer, to_write + 1, fmt, args_copy);
    va_end(args_copy);

    return buffer;
}

int
bld_str_eq(const char *str_a, const char *str_b)
{
    for (; *str_a && *str_b; ++str_a, ++str_b) {
        if (*str_a != *str_b)
            return 0;
    }

    return *str_a == *str_b;
}

pid_t
bld_cc_params_nt_async(const char **params)
{
    int count = 0;
    for (; params[count]; ++count)
        ;;

    return bld_cc_params_async(params, count);
}

int
bld_cc_params(const char **params, int param_count)
{
    return bld_wait_for(bld_cc_params_async(params, param_count));
}

int
bld_cc_params_nt(const char **params)
{
    return bld_wait_for(bld_cc_params_nt_async(params));
}

pid_t
bld_cc_pro_async(const char *out_name,
                 char **src,   int src_count,
                 char **libs,  int lib_count,
                 char **inc,   int inc_count,
                 char **defs,  int def_count,
                 char **warns, int warn_count,
                 char **raw,   int raw_count)
{
    // NOTE: We do extra work, but it's easier to maintain,
    //       since all you need to change to port to different compiler is
    //       the `bld_cc_params_async` function.

    bld_sa_t params = {0};

    if (out_name) {
        BLD_SA_PUSH(params, bld_strf("-o"));
        // NOTE: Need to copy cuz we simply dealloc everithing and dont care.
        BLD_SA_PUSH(params, bld_strf("%s", out_name));
    }

    if (src) {
        for (int i = 0; i < src_count; ++i) {
            BLD_SA_PUSH(params, bld_strf("%s", src[i]));
        }
    }

    if (libs) {
        for (int i = 0; i < lib_count; ++i) {
            BLD_SA_PUSH(params, bld_strf("-l%s", libs[i]));
        }
    }

    if (inc) {
        for (int i = 0; i < inc_count; ++i) {
            BLD_SA_PUSH(params, bld_strf("-I%s", inc[i]));
        }
    }

    if (defs) {
        for (int i = 0; i < def_count; ++i) {
            BLD_SA_PUSH(params, bld_strf("-D%s", defs[i]));
        }
    }

    if (warns) {
        for (int i = 0; i < warn_count; ++i) {
            BLD_SA_PUSH(params, bld_strf("-W%s", warns[i]));
        }
    }

    if (raw) {
        for (int i = 0; i < raw_count; ++i) {
            BLD_SA_PUSH(params, bld_strf("-R%s", raw[i]));
        }
    }

    pid_t pid = bld_cc_params_async((const char **)params.data, params.count);

    for (int i = 0; i < params.count; ++i) {
        free(params.data[i]);
    }

    free(params.data);

    return pid;
}

int
bld_cc_pro(const char *out_name,
           char **src,  int src_count,
           char **libs, int lib_count,
           char **inc,  int inc_count,
           char **defs, int def_count,
           char **warns, int warn_count,
           char **raw,  int raw_count)
{
    return bld_wait_for(
        bld_cc_pro_async(out_name,
                         src,   src_count,
                         libs,  lib_count,
                         inc,   inc_count,
                         defs,  def_count,
                         warns, warn_count,
                         raw,   raw_count)
    );
}

int
bld_contains(const char *str, int argc, char *argv[])
{
    for (int i = 0; i < argc; ++i) {
        if (bld_str_eq(argv[i], str))
            return 1;
    }

    return 0;
}

int
bld_find(const char *str, int count, char **list)
{
    for (int i = 0; i < count; ++i) {
        if (bld_str_eq(list[i], str))
            return i;
    }

    return -1;
}

char *
bld_join(const char *delim, int count, const char **list)
{
    size_t memory_size = 1;

    if (count > 0) {
        memory_size += (count - 1) * strlen(delim);
    }

    for (int i = 0; i < count; ++i) {
        memory_size += strlen(list[i]);
    }

    char *buffer = malloc(memory_size);
    assert(buffer);

    char *p = buffer;

    for (int i = 0; i < count; ++i) {
        if (i) {
            for (const char *s = delim; *s; ++s) {
                *(p++) = *s;
            }
        }

        for (const char *s = list[i]; *s; ++s) {
            *(p++) = *s;
        }
    }

    *p = 0;

    return buffer;
}

void
bld_sb_append(bld_sb_t *sb, const char *s)
{
    size_t s_len = strlen(s) + 1;
    size_t new_size = sb->size + s_len;

    if (new_size > sb->capacity) {
        if (sb->capacity == 0) {
            sb->capacity = 4096;
        }

        while (sb->capacity < new_size) {
            sb->capacity *= 2;
        }

        sb->data = realloc(sb->data, sb->capacity);
        assert(sb->data);
    }

    for (size_t i = 0; i < s_len; ++i) {
        sb->data[sb->size + i] = s[i];
    }

    sb->size += s_len - 1;
}

void
bld_sb_append_multiple(bld_sb_t *sb, const char **sl, int count)
{
    for (int i = 0; i < count; ++i) {
        bld_sb_append(sb, sl[i]);
    }
}

int
bld_try_rebuild_self(char *self, int argc, char *argv[])
{
    if (bld_file_time_cmp(bld_mod_time(self), bld_mod_time(argv[0])) < 0)
        return -1;

    printf("Recompiling '%s'.\n", self);

    int exe_len = strlen(argv[0]);

    char *move_dest = malloc(exe_len + 5);
    assert(move_dest);

    memcpy(move_dest, argv[0], exe_len);

    move_dest[exe_len + 0] = '.';
    move_dest[exe_len + 1] = 'o';
    move_dest[exe_len + 2] = 'l';
    move_dest[exe_len + 3] = 'd';
    move_dest[exe_len + 4] = '\0';

    if (bld_exists(move_dest)) {
        remove(move_dest);
    }

    rename(argv[0], move_dest);

    int res = BLD_CC(self, "-o", argv[0]);
    if (res) {
        rename(move_dest, argv[0]);
        return res;
    }

    return bld_execute_argv(argv);
}

int
bld_run_argv(const char *name, char *argv[])
{
    return bld_wait_for(bld_run_argv_async(name, argv));
}

int
bld_run_str(const char *name, const char *str)
{
    return bld_wait_for(bld_run_str_async(name, str));
}

pid_t
bld_run_async(const char *name, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    pid_t pid = bld_run_vargs_async(name, fmt, args);

    va_end(args);

    return pid;
}

int
bld_run(const char *name, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    pid_t pid = bld_run_vargs_async(name, fmt, args);

    va_end(args);

    return bld_wait_for(pid);
}

pid_t
bld_run_program_async(const char *name)
{
    return bld_run_async(name, "");
}

int
bld_run_program(const char *name)
{
    return bld_wait_for(bld_run_program_async(name));
}

void
bld_sa_push_multiple(bld_sa_t *sa, char **data, int count)
{
    for (int i = 0; i < count; ++i) {
        BLD_STRETCHY_PUSH(*sa, data[i]);
    }
}

void
bld_sa_push_multiple_nt(bld_sa_t *sa, char **data)
{
    for (int i = 0; data[i]; ++i) {
        BLD_STRETCHY_PUSH(*sa, data[i]);
    }
}


#endif // defined(BLD_IMPLEMENTATION)

#endif // BLD_H_
