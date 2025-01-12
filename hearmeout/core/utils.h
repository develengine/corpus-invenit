#ifndef UTILS_H_
#define UTILS_H_

#define LENGTH_OF(x) (sizeof(x)/sizeof(*x))
#define ALIGNMENT_OF(...) _Alignof(__VA_ARGS__)


//                                 why
#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
    #define COMPILER_MSVC
#elif defined(__clang__)
    #define COMPILER_CLANG
#elif defined(__GNUC__) || defined(__GNUG__)
    #define COMPILER_GNUC
#endif


#if defined(_DEBUG)
    #include <stdio.h>
    #include <stdlib.h>

    #define UNREACHABLE()                                                           \
    do {                                                                            \
        fprintf(stderr, "%s:%d: Unreachable line reached.\n", __FILE__, __LINE__);  \
        exit(666);                                                                  \
    } while (0)
#else
    #if defined(COMPILER_MSVC)
        #define UNREACHABLE()   __assume(0)
    #elif defined(COMPILER_GNUC) || defined(COMPILER_CLANG)
        #define UNREACHABLE()   __builtin_unreachable()
    #endif
#endif

static inline void
noop(void) { }

#if defined(_DEBUG)
    #define ASSERT(...) \
    ( \
        !(__VA_ARGS__) ? ( \
            fprintf(stderr, "%s:%d: Assert failed! (%s)\n", __FILE__, __LINE__, #__VA_ARGS__), \
            exit(1) \
        ) \
        : ( \
            noop() \
        ) \
    )
#else
    #define ASSERT(...) (noop())
#endif


#if defined(COMPILER_MSVC)
    #define FMT_U64 "%llu"
    #define FMT_I64 "%lld"
    #define FMT_X64 "%llx"
#else
    #define FMT_U64 "%lu"
    #define FMT_I64 "%ld"
    #define FMT_X64 "%lx"
#endif


#include <stdint.h>

typedef double   f64;
typedef float    f32;
typedef uint64_t u64;
typedef int64_t  i64;
typedef uint32_t u32;
typedef int32_t  i32;
typedef uint16_t u16;
typedef int16_t  i16;
typedef uint8_t  u8;
typedef int8_t   i8;

#if defined(false) || defined(true)
    typedef uint32_t b32;
#else
    typedef enum { false = 0, true = 1 } b32;
#endif


#endif
