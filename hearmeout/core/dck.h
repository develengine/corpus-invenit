/* TODO:
 * [ ] Allocators.
 */

#ifndef DCK_H
#define DCK_H

#include <stdlib.h>
#include <stdio.h>


#define dck_stretchy_t(data_type, size_type) struct { data_type *data; size_type count, capacity; }

#define dck_stretchy_for(dck, type, elem) \
    for (type *elem = (dck).data; elem < (dck).data + (dck).count; ++elem)

/* argument must be an 'lvalue', except for `...` */
#define dck_stretchy_push(dck, ...)                                                     \
do {                                                                                    \
    if ((dck).count == (dck).capacity) {                                                \
        (dck).capacity = (dck).capacity ? (dck).capacity * 2                            \
                                          : 4096 / sizeof(*((dck).data));               \
        (dck).data = realloc((dck).data, sizeof(*((dck).data)) * (dck).capacity);       \
        if (!(dck).data) {                                                              \
            fprintf(stderr, "%s:%d: malloc failure! exiting...\n", __FILE__, __LINE__); \
            exit(666);                                                                  \
        }                                                                               \
    }                                                                                   \
    (dck).data[(dck).count] = __VA_ARGS__;                                              \
    (dck).count++;                                                                      \
} while (0)

/* argument must be an 'lvalue', except for `amount` */
#define dck_stretchy_reserve(dck, amount)                                               \
do {                                                                                    \
    if ((dck).count + (amount) > (dck).capacity) {                                      \
        if ((dck).capacity == 0) {                                                      \
            (dck).capacity = 4096 / sizeof(*((dck).data));                              \
        }                                                                               \
        while ((dck).count + (amount) > (dck).capacity) {                               \
            (dck).capacity *= 2;                                                        \
        }                                                                               \
        (dck).data = realloc((dck).data, sizeof(*((dck).data)) * (dck).capacity);       \
        if (!(dck).data) {                                                              \
            fprintf(stderr, "%s:%d: malloc failure! exiting...\n", __FILE__, __LINE__); \
            exit(666);                                                                  \
        }                                                                               \
    }                                                                                   \
} while (0)


#endif // DCK_H
