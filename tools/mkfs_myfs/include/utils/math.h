#ifndef UTILS_MATH_H
#define UTILS_MATH_H

#include <stdint.h>

// Your kernel expects: div64_32(dividend, divisor, quotient_ptr, remainder_ptr)
#define div64_32(n, base, q_ptr, r_ptr)                                        \
  do {                                                                         \
    *(q_ptr) = (n) / (base);                                                   \
    *(r_ptr) = (n) % (base);                                                   \
  } while (0)

// Provide the MIN macro that myfs_inode_alloc is looking for
#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#endif
