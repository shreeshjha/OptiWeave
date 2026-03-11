/*
 * bugs.c — OptiWeave demo: all 6 --auto-fix kinds in one file.
 *
 * Fix kinds demonstrated:
 *   overflow       unsigned wraparound:  a - b  when b > a
 *   negation       signed negation UB:   -x     when x == INT_MIN
 *   shift          signed left shift UB: x << n on signed type
 *   uninitialized  variable used before init
 *   unused         variable initialized but never read
 *   fp-equality    direct == / != on float/double
 *
 * Also exercises:
 *   --detect-overflow   (OverflowDetector static analysis)
 *   --fp-precision-warnings (FPPrecisionDetector)
 *   --data-flow-analysis    (DataFlowAnalysis)
 */

#include <stdio.h>
#include <limits.h>

/* ─── 1. overflow: unsigned wraparound ────────────────────────────────────
 * Auto-fix rewrites:
 *   a - b   →   ((a) >= (b) ? (a) - (b) : 0)
 * and injects no extra headers.
 */
unsigned safe_subtract(unsigned a, unsigned b) {
    return a - b;       /* BUG: wraps to UINT_MAX when b > a */
}

/* ─── 2. negation: signed negation overflow ───────────────────────────────
 * Auto-fix rewrites:
 *   -x   →   ((x) == INT_MIN ? INT_MAX : -(x))
 * and injects <limits.h> if not already present.
 */
int safe_negate(int x) {
    return -x;          /* BUG: undefined behaviour when x == INT_MIN */
}

/* ─── 3. shift: signed left shift UB ─────────────────────────────────────
 * Auto-fix rewrites:
 *   x << n   →   ((unsigned)(x)) << n
 */
int make_flags(int base, int shift_amount) {
    return base << shift_amount;    /* BUG: UB if bit overflows into sign bit */
}

/* ─── 4. uninitialized: variable used before assignment ───────────────────
 * Auto-fix rewrites the declaration:
 *   int result;   →   int result = 0;
 */
int compute(int a, int b, int mode) {
    int result;                     /* BUG: potentially uninitialized */
    if (mode == 1) result = a + b;
    else if (mode == 2) result = a * b;
    /* mode == 0 path leaves result uninitialized */
    return result;
}

/* ─── 5. unused: variable initialized but never read ──────────────────────
 * Auto-fix inserts:   (void)debug_id;
 * after the declaration site.
 */
void process(int *data, int n) {
    int debug_id = 42;              /* BUG: set but never used */
    int total = 0;
    for (int i = 0; i < n; i++) {
        total += data[i];
    }
    printf("total = %d\n", total);
}

/* ─── 6. fp-equality: direct float equality comparison ────────────────────
 * Auto-fix rewrites:
 *   a == b   →   fabs((a)-(b)) < 1e-9
 *   a != b   →   fabs((a)-(b)) >= 1e-9
 * and injects <math.h>.
 */
int is_unit(double x) {
    return x == 1.0;                /* BUG: exact FP comparison */
}

int is_zero(float f) {
    return f == 0.0f;               /* BUG: exact FP comparison */
}

int values_differ(double a, double b) {
    return a != b;                  /* BUG: exact FP inequality */
}

/* ─── bonus: FP precision (detected by --fp-precision-warnings) ───────────
 * Catastrophic cancellation: subtracting two nearly-equal values.
 * Mixed precision: adding float to double.
 */
double bad_precision(double big, double small_delta, float approx) {
    double diff = big - (big + small_delta);    /* catastrophic cancellation */
    double mixed = diff + approx;               /* mixed float + double */
    return mixed;
}

/* ─── main ────────────────────────────────────────────────────────────────*/
int main(void) {
    /* Demonstrate each buggy function with typical inputs */
    printf("safe_subtract(3, 10) = %u\n", safe_subtract(3, 10));   /* wraps! */
    printf("safe_negate(INT_MIN) = %d\n", safe_negate(INT_MIN));    /* UB! */
    printf("make_flags(1, 31)    = %d\n", make_flags(1, 31));       /* UB! */
    printf("compute(5, 3, 0)     = %d\n", compute(5, 3, 0));        /* uninit! */

    int data[5] = {1, 2, 3, 4, 5};
    process(data, 5);

    printf("is_unit(1.0)         = %d\n", is_unit(1.0));
    printf("is_zero(0.1f-0.1f)   = %d\n", is_zero(0.1f - 0.1f));
    printf("values_differ(π,π)   = %d\n", values_differ(3.14159, 3.14159));

    return 0;
}
