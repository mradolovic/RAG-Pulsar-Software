#include "../../includes/numerics/is_pow_of_2.h"

// Checks parameter x is a power of 2
bool is_pow_of_2(int x) {
    /* First x in the below expression is for the case when x is 0 */
    return x && (!(x & (x - 1)));
}
