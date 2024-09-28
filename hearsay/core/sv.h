#ifndef SV_H
#define SV_H

#include <stdio.h>
#include <stdbool.h>

// usage: printf("Name: "SV_FMT"\n", (int)sv_length(sv), sv.begin);
#define SV_FMT "%.*s"

#define SV_LIT(cstr_lit) ((sv_t) { .begin = cstr_lit, .end = cstr_lit + sizeof(cstr_lit) + 1 })

typedef struct
{
    char *begin, *end;
} sv_t;

static inline void sv_fwrite(sv_t sv, FILE *file)
{
    fwrite(sv.begin, 1, sv.end - sv.begin, file);
}

static inline bool sv_empty(sv_t sv)
{
    return sv.begin == sv.end;
}

static inline size_t sv_length(sv_t sv)
{
    return sv.end - sv.begin;
}

static inline bool sv_eq(sv_t a, sv_t b)
{
    size_t a_len = sv_length(a);

    if (a_len != sv_length(b))
        return false;

    for (size_t i = 0; i < a_len; ++i) {
        if (a.begin[i] != b.begin[i])
            return false;
    }

    return true;
}

static inline bool sv_is(sv_t sv, const char *str)
{
    size_t i = 0;
    for (; sv.begin + i < sv.end && str[i] != 0; ++i) {
        if (sv.begin[i] != str[i])
            return false;
    }

    return sv.begin + i >= sv.end && str[i] == 0;
}

static inline sv_t sv_l(char *str)
{
    sv_t res = { .begin = str, .end = str };

    for (; *(res.end); ++res.end)
        ;;

    return res;
}


typedef struct
{
    size_t begin, end;
} rsv_t;

static inline rsv_t rsv_make(char *origin, sv_t sv)
{
    return (rsv_t) { sv.begin - origin, sv.end - origin };
}

static inline sv_t rsv_get(char *origin, rsv_t rsv)
{
    return (sv_t) { origin + rsv.begin, origin + rsv.end };
}

#endif // SV_H
