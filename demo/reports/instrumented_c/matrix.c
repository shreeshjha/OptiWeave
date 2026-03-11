#include <optiweave/prelude_c.h>
/*
 * matrix.c — OptiWeave demo: performance hotspots, array subscripts,
 *             arithmetic ops, division in hot loop, O(n³) pattern,
 *             poor memory locality, integer overflow risk.
 *
 * Features triggered:
 *   [instrumentation] --array-subscripts --arithmetic-ops --assignment-ops
 *   [auto-patch]      Division in Hot Loop, SIMD Vectorization, O(n²) Algorithm,
 *                     Poor Memory Locality, Strength Reduction, Loop Interchange,
 *                     Loop Unroll Hint, Prefetch Hint, Restrict Qualifier
 *   [static analysis] --analyze-complexity --detect-overflow --memory-profile
 *                     --data-flow-analysis
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define N 256

/* ─── 1. Naive O(n³) matrix multiply ──────────────────────────────────────
 * Access pattern:  A[i][k] is fine, B[k][j] is column-stride → cache misses.
 * Detected by:     ComplexityDetector (O(n³)), MemoryAccessDetector (poor locality).
 * Auto-patch:      SIMD pragma + advisory comments inserted by PatchVisitor.
 */
void matmul_naive(float C[N][N], const float A[N][N], const float B[N][N]) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            float sum = 0.0f;
            for (int k = 0; k < N; k++) {
                ow_add_assign(sum, ow_mul(__ow_subscript_impl(__ow_subscript_impl(A, i, "/Users/shreesh/Dev/Github/OptiWeave/demo/src/matrix.c", 32, __FUNCTION__), k, "/Users/shreesh/Dev/Github/OptiWeave/demo/src/matrix.c", 32, __FUNCTION__), __ow_subscript_impl(__ow_subscript_impl(B, k, "/Users/shreesh/Dev/Github/OptiWeave/demo/src/matrix.c", 32, __FUNCTION__), j, "/Users/shreesh/Dev/Github/OptiWeave/demo/src/matrix.c", 32, __FUNCTION__)));   /* <-- array subscripts + arithmetic */
            }
            ({ __optiweave_record_subscript("/Users/shreesh/Dev/Github/OptiWeave/demo/src/matrix.c", 34, __FUNCTION__); C[i][j] = sum; });                   /* <-- assignment */
        }
    }
}

/* ─── 2. Division in a hot loop ───────────────────────────────────────────
 * Each iteration divides by the same invariant `scale`.
 * Detected by:     DivisionInLoopDetector.
 * Auto-patch:      replaces  x[i] / scale  with  x[i] * __ow_recip_0
 *                  and hoists  const auto __ow_recip_0 = 1.0 / scale;
 *                  before the loop.
 */
void normalize(float *x, int n, float scale) {
    for (int i = 0; i < n; i++) {
        ({ __optiweave_record_subscript("/Users/shreesh/Dev/Github/OptiWeave/demo/src/matrix.c", 48, __FUNCTION__); x[i] = x[i] / scale; });               /* <-- division in loop body */
    }
}

/* ─── 3. O(n²) pairwise distance ─────────────────────────────────────────
 * Detected by:     ComplexityDetector (O(n²) pattern).
 * Auto-patch:      advisory comment inserted.
 */
void pairwise_dist(float *dist, const float *pts, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            float dx = ow_sub(__ow_subscript_impl(pts, i, "/Users/shreesh/Dev/Github/OptiWeave/demo/src/matrix.c", 59, __FUNCTION__), __ow_subscript_impl(pts, j, "/Users/shreesh/Dev/Github/OptiWeave/demo/src/matrix.c", 59, __FUNCTION__));     /* <-- arithmetic */
            ({ __optiweave_record_subscript("/Users/shreesh/Dev/Github/OptiWeave/demo/src/matrix.c", 60, __FUNCTION__); dist[i * n + j] = dx * dx; });      /* <-- array subscript + arithmetic */
        }
    }
}

/* ─── 4. Integer overflow risks ───────────────────────────────────────────
 * Detected by:     OverflowDetector.
 * Patterns:        signed multiplication that can overflow, unsigned wrap.
 */
int accumulate(int *arr, int n) {
    int sum = 0;
    for (int i = 0; i < n; i++) {
        ow_add_assign(sum, __ow_subscript_impl(arr, i, "/Users/shreesh/Dev/Github/OptiWeave/demo/src/matrix.c", 72, __FUNCTION__));                      /* potential signed overflow */
    }
    return sum;
}

unsigned count_mismatches(unsigned a, unsigned b) {
    return ow_sub(a, b);                           /* unsigned wraparound when b > a */
}

/* ─── 5. Poor memory locality: column-major scan ──────────────────────────
 * Accesses a row-major array in column-major order → every access is a
 * cache-line miss for large N.
 * Detected by:     MemoryAccessDetector (strided access pattern).
 */
void column_sum(float result[N], const float M[N][N]) {
    for (int j = 0; j < N; j++) {
        float s = 0.0f;
        for (int i = 0; i < N; i++) {
            ow_add_assign(s, __ow_subscript_impl(__ow_subscript_impl(M, i, "/Users/shreesh/Dev/Github/OptiWeave/demo/src/matrix.c", 90, __FUNCTION__), j, "/Users/shreesh/Dev/Github/OptiWeave/demo/src/matrix.c", 90, __FUNCTION__));                   /* stride = N floats between accesses */
        }
        ({ __optiweave_record_subscript("/Users/shreesh/Dev/Github/OptiWeave/demo/src/matrix.c", 92, __FUNCTION__); result[j] = s; });
    }
}

/* ─── 6. Dynamic allocation (memory profile) ──────────────────────────────
 * Detected by:     MemoryProfiler (malloc / free tracking).
 */
float *alloc_vector(int n) {
    return (float *)malloc(ow_mul(n, sizeof(float)));
}

/* no matching free → potential leak detected by --memory-profile */
float *leaky_alloc(int n) {
    float *p = (float *)malloc(ow_mul(n, sizeof(float)));
    float *q = (float *)malloc(ow_mul(n, sizeof(float))); /* q is never freed */
    (void)q;
    return p;
}

/* ─── 7. Strength reduction: i*K in hot loop ─────────────────────────────
 * The multiplication `i * stride` can be replaced with an accumulator:
 *   long __ow_acc = 0; ... __ow_acc += stride;
 * Detected by:     StrengthReductionDetector (operations_in_loop has "multiplication").
 * Auto-patch:      replaces i*stride with accumulator variable.
 */
void gather_strided(const float *src, float *dst, int n, int stride) {
    for (int i = 0; i < n; i++) {
        ({ __optiweave_record_subscript("/Users/shreesh/Dev/Github/OptiWeave/demo/src/matrix.c", 119, __FUNCTION__); dst[i] = src[i * stride]; });       /* <-- multiplication in hot loop */
    }
}

/* ─── 8. Loop interchange: nested loop with poor cache locality ──────────
 * The inner loop walks columns (stride = M elements apart).
 * Swapping inner/outer loops converts to row-major access.
 * Detected by:     LoopInterchangeDetector (nesting >= 2, strided, stride > cache line).
 * Auto-patch:      swaps inner and outer for-loop headers.
 */
#define M 128
void transpose(float dst[M][M], const float src[M][M]) {
    for (int j = 0; j < M; j++) {       /* outer walks columns */
        for (int i = 0; i < M; i++) {   /* inner walks rows of src */
            ({ __optiweave_record_subscript("/Users/shreesh/Dev/Github/OptiWeave/demo/src/matrix.c", 133, __FUNCTION__); dst[i][j] = src[j][i]; });      /* <-- column-stride access on dst */
        }
    }
}

/* ─── 9. Loop unroll hint: small bounded loop ────────────────────────────
 * Body is tiny (3 statements) and trip count is fixed at 4.
 * Compiler unroll pragma can eliminate loop overhead entirely.
 * Detected by:     LoopUnrollHintDetector (body <= 8 stmts, trip_count <= 64).
 * Auto-patch:      inserts #pragma clang loop unroll(full).
 */
void rgba_to_gray(const unsigned char *rgba, unsigned char *gray, int pixels) {
    for (int p = 0; p < pixels; p++) {
        int sum = 0;
        for (int c = 0; c < 3; c++) {   /* <-- 3 iterations, tiny body */
            ow_add_assign(sum, __ow_subscript_impl(rgba, ow_add(ow_mul(p, 4), c), "/Users/shreesh/Dev/Github/OptiWeave/demo/src/matrix.c", 148, __FUNCTION__));
        }
        ({ __optiweave_record_subscript("/Users/shreesh/Dev/Github/OptiWeave/demo/src/matrix.c", 150, __FUNCTION__); gray[p] = (unsigned char)(sum / 3); });
    }
}

/* ─── 10. Prefetch hint: single-level strided access ─────────────────────
 * Accesses every 16th element (stride=16, 16*8=128 >= 64 byte cache line).
 * A __builtin_prefetch ahead of the access can hide memory latency.
 * Detected by:     PrefetchHintDetector (strided, nesting < 2, stride >= prefetch_min).
 * Auto-patch:      inserts __builtin_prefetch(&data[i + 16], 0, 3) before access.
 */
float sparse_sum(const float *data, int n) {
    float total = 0.0f;
    for (int i = 0; i < n; ow_add_assign(i, 16)) {   /* <-- stride of 16 elements */
        ow_add_assign(total, __ow_subscript_impl(data, i, "/Users/shreesh/Dev/Github/OptiWeave/demo/src/matrix.c", 163, __FUNCTION__));
    }
    return total;
}

/* ─── 11. Restrict qualifier: multi-pointer function ─────────────────────
 * Three pointer params without __restrict → compiler cannot prove no-alias,
 * preventing SIMD vectorization of the loop body.
 * Detected by:     RestrictQualifierDetector (num_pointer_params >= 2).
 * Auto-patch:      adds __restrict to pointer parameters.
 */
void vec_add(float *out, const float *a, const float *b, int n) {
    for (int i = 0; i < n; i++) {
        ({ __optiweave_record_subscript("/Users/shreesh/Dev/Github/OptiWeave/demo/src/matrix.c", 176, __FUNCTION__); out[i] = a[i] + b[i]; });           /* <-- aliasing prevents vectorization */
    }
}

/* ─── main ────────────────────────────────────────────────────────────────*/
int main(void) {
    /* Stack-allocate small matrices to keep the demo fast */
    static float A[N][N], B[N][N], C[N][N];
    static float col_result[N];

    /* Fill A and B with simple values */
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++) {
            ({ __optiweave_record_subscript("/Users/shreesh/Dev/Github/OptiWeave/demo/src/matrix.c", 189, __FUNCTION__); A[i][j] = (float)(i + j + 1); });
            ({ __optiweave_record_subscript("/Users/shreesh/Dev/Github/OptiWeave/demo/src/matrix.c", 190, __FUNCTION__); B[i][j] = (float)(i - j + 1) * 0.5f; });
        }

    matmul_naive(C, A, B);

    float *v = alloc_vector(N);
    for (int i = 0; i < N; i++) ({ __optiweave_record_subscript("/Users/shreesh/Dev/Github/OptiWeave/demo/src/matrix.c", 196, __FUNCTION__); v[i] = (float)(i + 1); });
    normalize(v, N, 100.0f);

    float pts[64];
    float dist[64 * 64];
    for (int i = 0; i < 64; i++) ({ __optiweave_record_subscript("/Users/shreesh/Dev/Github/OptiWeave/demo/src/matrix.c", 201, __FUNCTION__); pts[i] = (float)i; });
    pairwise_dist(dist, pts, 64);

    column_sum(col_result, B);

    int arr[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    printf("sum = %d\n", accumulate(arr, 8));
    printf("C[0][0] = %.4f\n", __ow_subscript_impl(__ow_subscript_impl(C, 0, "/Users/shreesh/Dev/Github/OptiWeave/demo/src/matrix.c", 208, __FUNCTION__), 0, "/Users/shreesh/Dev/Github/OptiWeave/demo/src/matrix.c", 208, __FUNCTION__));
    printf("v[0] = %.4f, dist[1] = %.4f, col[0] = %.4f\n",
           __ow_subscript_impl(v, 0, "/Users/shreesh/Dev/Github/OptiWeave/demo/src/matrix.c", 210, __FUNCTION__), __ow_subscript_impl(dist, 1, "/Users/shreesh/Dev/Github/OptiWeave/demo/src/matrix.c", 210, __FUNCTION__), __ow_subscript_impl(col_result, 0, "/Users/shreesh/Dev/Github/OptiWeave/demo/src/matrix.c", 210, __FUNCTION__));

    /* ── New detector demos ── */

    /* 7. Strength reduction: gather every 4th element */
    float gathered[64];
    gather_strided(v, gathered, 64, 4);
    printf("gathered[0] = %.4f\n", __ow_subscript_impl(gathered, 0, "/Users/shreesh/Dev/Github/OptiWeave/demo/src/matrix.c", 217, __FUNCTION__));

    /* 8. Loop interchange: transpose a matrix */
    static float S[M][M], T[M][M];
    for (int i = 0; i < M; i++)
        for (int j = 0; j < M; j++)
            ({ __optiweave_record_subscript("/Users/shreesh/Dev/Github/OptiWeave/demo/src/matrix.c", 223, __FUNCTION__); S[i][j] = (float)(i * M + j); });
    transpose(T, S);
    printf("T[0][1] = %.4f\n", __ow_subscript_impl(__ow_subscript_impl(T, 0, "/Users/shreesh/Dev/Github/OptiWeave/demo/src/matrix.c", 225, __FUNCTION__), 1, "/Users/shreesh/Dev/Github/OptiWeave/demo/src/matrix.c", 225, __FUNCTION__));

    /* 9. Loop unroll hint: RGBA to grayscale conversion */
    unsigned char rgba[256 * 4], gray[256];
    for (int i = 0; i < ow_mul(256, 4); i++) ({ __optiweave_record_subscript("/Users/shreesh/Dev/Github/OptiWeave/demo/src/matrix.c", 229, __FUNCTION__); rgba[i] = (unsigned char)(i & 0xFF); });
    rgba_to_gray(rgba, gray, 256);
    printf("gray[0] = %d\n", __ow_subscript_impl(gray, 0, "/Users/shreesh/Dev/Github/OptiWeave/demo/src/matrix.c", 231, __FUNCTION__));

    /* 10. Prefetch hint: sparse summation */
    printf("sparse_sum = %.4f\n", sparse_sum(v, N));

    /* 11. Restrict qualifier: vector addition */
    float va[64], vb[64], vc[64];
    for (int i = 0; i < 64; i++) { ({ __optiweave_record_subscript("/Users/shreesh/Dev/Github/OptiWeave/demo/src/matrix.c", 238, __FUNCTION__); va[i] = (float)i; }); ({ __optiweave_record_subscript("/Users/shreesh/Dev/Github/OptiWeave/demo/src/matrix.c", 238, __FUNCTION__); vb[i] = (float)(64 - i); }); }
    vec_add(vc, va, vb, 64);
    printf("vc[0] = %.4f\n", __ow_subscript_impl(vc, 0, "/Users/shreesh/Dev/Github/OptiWeave/demo/src/matrix.c", 240, __FUNCTION__));

    free(v);
    return 0;
}
