#ifndef PARSE_H_
#define PARSE_H_

int
parse_simple_int(const char *begin, const char *end, char **restrict next_o);

float
parse_simple_float(const char *begin, const char *end, char **restrict next_o);


static inline int
is_space(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
}

static inline int
is_digit(char c)
{
    return c >= '0' && c <= '9';
}


#if defined(PARSE_IMPLEMENTATION)


int
parse_simple_int(const char *begin, const char *end, char **restrict next_o)
{
    const char *at = begin;

    while (at != end && is_space(*at)) {
        ++at;
    }

    // failed to parse
    if (at == end) {
        *next_o = 0;
        return 0;
    }

    int sign = 1;

    if (*at == '-') {
        sign = -1;
        ++at;
    }
    else if (*at == '+') {
        ++at;
    }

    int res = 0;

    for (; at != end; ++at) {
        if (!is_digit(*at))
            break;

        res = res * 10 + (*at - '0');
    }

    *next_o = (char *)at;
    return sign * res;
}

float
parse_simple_float(const char *begin, const char *end, char **restrict next_o)
{
    const char *at = begin;

    while (at != end && is_space(*at)) {
        ++at;
    }

    // failed to parse
    if (at == end) {
        *next_o = 0;
        return 0.0f;
    }

    int sign = 1;

    if (*at == '-') {
        sign = -1;
        ++at;
    }
    else if (*at == '+') {
        ++at;
    }

    int integer = 0;
    int decimal = 0;
    int denom = 1;
    int has_decimal = 0;

    for (; at != end; ++at) {
        if (!is_digit(*at)) {
            if (*at == '.') {
                has_decimal = 1;
                ++at;
            }

            break;
        }

        integer = integer * 10 + (*at - '0');
    }

    if (has_decimal) {
        for (; at != end; ++at) {
            if (!is_digit(*at))
                break;

            decimal = decimal * 10 + (*at - '0');
            denom *= 10;
        }
    }

    *next_o = (char *)at;
    return (sign) * ((float)integer + (float)decimal / denom);
}


#endif // defined(PARSE_IMPLEMENTATION)


#endif // PARSE_H_
