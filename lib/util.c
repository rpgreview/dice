#include <stdlib.h>

#include "../include/util.h"

// For consumption by qsort_r from stdlib
// Return -1, 0, 1 depending on whether a < b, a == b, a > b
// Ref:
//   - man 3 qsort_r
//   - https://stackoverflow.com/a/1788048
int integer_difference_sign(const void *a, const void *b, void *data) {
    long n1 = *((long*)a);
    long n2 = *((long*)b);
    return (n1 > n2) - (n1 < n2);
}
