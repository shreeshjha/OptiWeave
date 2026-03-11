#include <optiweave/prelude.hpp>
/*
 * compute.cpp — OptiWeave demo: C++ instrumentation, STL containers,
 *               templates, comparison ops, assignment ops, call graph.
 *
 * Features triggered:
 *   [instrumentation]  --array-subscripts (std::vector overload dispatch)
 *                      --arithmetic-ops   (template arithmetic)
 *                      --comparison-ops   (float/int comparisons)
 *                      --assignment-ops   (compound assignments)
 *   [static analysis]  --analyze-complexity  (high cyclomatic + cognitive)
 *                      --call-graph          (rich call graph)
 *                      --dependency-graph    (STL headers)
 *                      --fp-precision-warnings
 *                      --data-flow-analysis
 *   [prelude]          has_subscript_overload<vector<T>> → dispatches to
 *                      operator[] overload path in __maybe_primop_subscript
 */

#include <vector>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <cstdio>
#include <string>

/* ─── 1. Template function: pairwise dot product ──────────────────────────
 * Template-dependent expressions use __maybe_primop_* wrappers so that
 * type resolution defers to instantiation time.
 */
template <typename T>
T dot(const std::vector<T>& a, const std::vector<T>& b) {
    T result = T{};
    for (std::size_t i = 0; optiweave::ow_lt(i, a.size()); i++) {
        optiweave::ow_add_assign(result, optiweave::ow_mul(optiweave::__ow_subscript_impl(a, i, "/Users/shreesh/Dev/Github/OptiWeave/demo/src/compute.cpp", 34, __FUNCTION__), optiweave::__ow_subscript_impl(b, i, "/Users/shreesh/Dev/Github/OptiWeave/demo/src/compute.cpp", 34, __FUNCTION__)));          /* subscript + arithmetic + assignment */
    }
    return result;
}

/* ─── 2. Comparison ops on multiple types ─────────────────────────────────
 * Exercises ow_eq, ow_lt, ow_gt, ow_le, ow_ge across int and double.
 */
int classify_score(double score) {
    if (optiweave::ow_ge(score, 90.0)) return 4;        /* ow_ge */
    if (optiweave::ow_ge(score, 75.0)) return 3;        /* ow_ge */
    if (optiweave::ow_ge(score, 60.0)) return 2;        /* ow_ge */
    if (optiweave::ow_gt(score, 0.0))  return 1;        /* ow_gt */
    return 0;
}

bool in_range(int x, int lo, int hi) {
    return optiweave::ow_ge(x, lo) && optiweave::ow_le(x, hi);          /* ow_ge, ow_le */
}

/* ─── 3. Compound assignment ops ─────────────────────────────────────────
 * Exercises ow_add_assign, ow_sub_assign, ow_mul_assign, ow_div_assign.
 */
std::vector<double> scale_and_shift(std::vector<double> v,
                                    double factor, double offset) {
    for (auto& x : v) {
        optiweave::ow_mul_assign(x, factor);    /* ow_mul_assign */
        optiweave::ow_add_assign(x, offset);    /* ow_add_assign */
    }
    return v;
}

/* ─── 4. High cyclomatic complexity function ──────────────────────────────
 * Deliberately complex to exercise --analyze-complexity ratings.
 * Cyclomatic ≈ 12, Cognitive ≈ 18  →  "Complex/High" rating.
 */
std::string grade_report(double score, int attendance, bool extra_credit,
                         bool late_penalty, int retakes) {
    std::string grade;

    if (optiweave::ow_lt(score, 0.0) || optiweave::ow_gt(score, 100.0)) {
        return "invalid";
    }

    if (late_penalty) {
        optiweave::ow_sub_assign(score, 5.0);
        if (optiweave::ow_lt(score, 0.0)) optiweave::ow_assign(score, 0.0);
    }

    if (extra_credit) {
        optiweave::ow_add_assign(score, 3.0);
        if (optiweave::ow_gt(score, 100.0)) optiweave::ow_assign(score, 100.0);
    }

    for (int r = 0; optiweave::ow_lt(r, retakes) && optiweave::ow_lt(score, 60.0); r++) {
        optiweave::ow_add_assign(score, 10.0);
        if (optiweave::ow_gt(score, 70.0)) optiweave::ow_assign(score, 70.0);
    }

    int letter = classify_score(score);
    switch (letter) {
        case 4: grade = "A"; break;
        case 3: grade = "B"; break;
        case 2: grade = "C"; break;
        case 1: grade = (optiweave::ow_ge(attendance, 80)) ? "D" : "F"; break;
        default: grade = "F"; break;
    }

    if (optiweave::ow_lt(attendance, 50) && grade != "F") {
        grade = "F";    /* attendance policy override */
    }

    return grade;
}

/* ─── 5. FP precision issues ──────────────────────────────────────────────
 * Direct double equality (fp-equality auto-fix target).
 * Accumulation loop (Kahan compensation opportunity).
 */
bool is_converged(double a, double b) {
    return optiweave::ow_eq(a, b);                      /* BUG: exact double comparison */
}

double naive_sum(const std::vector<double>& v) {
    double total = 0.0;
    for (double x : v) {
        optiweave::ow_add_assign(total, x);                     /* accumulation without Kahan */
    }
    return total;
}

/* ─── 6. STL subscript overload dispatch ─────────────────────────────────
 * __maybe_primop_subscript detects has_subscript_overload<vector<int>> = true
 * and routes to the overloaded operator[] path rather than raw pointer math.
 */
int vector_sum(const std::vector<int>& v) {
    int s = 0;
    for (std::size_t i = 0; optiweave::ow_lt(i, v.size()); i++) {
        optiweave::ow_add_assign(s, v[i]);                      /* STL subscript overload path */
    }
    return s;
}

/* ─── 7. Call graph nodes ─────────────────────────────────────────────────
 * Calling chain: main → grade_report → classify_score → in_range
 *                main → dot<double>
 *                main → scale_and_shift → (lambda)
 *                main → naive_sum
 *                main → vector_sum
 */
int main() {
    /* dot product */
    std::vector<double> a = {1.0, 2.0, 3.0};
    std::vector<double> b = {4.0, 5.0, 6.0};
    printf("dot(a,b) = %.1f\n", dot(a, b));   /* 32.0 */

    /* scoring */
    double scores[] = {95.0, 82.0, 67.0, 55.0, 0.0};
    for (double s : scores) {
        printf("score %.0f → %s\n", s,
               grade_report(s, 85, false, false, 0).c_str());
    }

    /* scale and shift */
    auto shifted = scale_and_shift({1.0, 2.0, 3.0, 4.0}, 2.5, -1.0);
    for (double x : shifted) printf("%.2f ", x);
    printf("\n");

    /* FP convergence */
    printf("converged? %d\n", is_converged(optiweave::ow_div(1.0, 3.0), optiweave::ow_div(1.0, 3.0)));

    /* vector sum */
    std::vector<int> iv = {10, 20, 30, 40, 50};
    printf("vector_sum = %d\n", vector_sum(iv));

    return 0;
}
