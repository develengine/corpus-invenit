#include <stdio.h>

/* This is an example of program that prints a spiral of numbers in O(N) comp. complexity and
 * O(1) memory complexity. Starting from the center, going right, then up.
 *
 * Try it out yourself too!
 */

/* 
 * 16 15 14 13 12
 * 17 4  3  2  11
 * 18 5  0  1  10
 * 19 6  7  8  9
 * 20 21 22 23 24
 */

int
get_spiral(int x, int y, int size)
{
    int c = size / 2;

    int ax = x - c;
    if (ax < 0) ax *= -1;
    int ay = y - c;
    if (ay < 0) ay *= -1;

    int n = ax > ay ? ax : ay;
    int o = (n * 2 + 1) * (n * 2 + 1) - 1;

    if (y > c) {
        if (ay == n) {
            return o + x - n * 2 - ((size - n * 2 - 1) >> 1);
        }
    }

    if (x < c) {
        if (ax == n) {
            return o + y - n * 4 - ((size - n * 2 - 1) >> 1);
        }
    }

    if (y < c) {
        if (ay == n) {
            return o - x - n * 4 + ((size - n * 2 - 1) >> 1);
        }
    }

    if (x > c) {
        if (ax == n) {
            return o - y - n * 6 + ((size - n * 2 - 1) >> 1);
        }
    }

    return o;
}


int
count_digits(int n)
{
    int dc = 0;

    for (int cn = n; cn; ++dc, cn /= 10)
        ;;

    if (!dc) dc = 1;

    return dc;
}


void
print_padded(int n, int s)
{
    int dc = count_digits(n);

    for (int i = 0; i < s - dc; ++i) {
        printf(" ");
    }

    printf("%d", n);
}


int
main(int argc, char *argv[])
{
    int size = 15;

    int pad = count_digits(size * size) + 1;

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            print_padded(get_spiral(x, y, size), pad);
        }

        printf("\n");
    }

    return 0;
}
