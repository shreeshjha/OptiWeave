/* OptiWeave C-compatible statistics header
 * Provides C89/C99 compatible statistics tracking
 */

#ifndef OPTIWEAVE_STATISTICS_C_H
#define OPTIWEAVE_STATISTICS_C_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Operation counters (C-compatible) */
extern uint64_t g_optiweave_array_subscript_count;
extern uint64_t g_optiweave_addition_count;
extern uint64_t g_optiweave_subtraction_count;
extern uint64_t g_optiweave_multiplication_count;
extern uint64_t g_optiweave_division_count;
extern uint64_t g_optiweave_modulo_count;

/* Comparison counters */
extern uint64_t g_optiweave_equal_count;
extern uint64_t g_optiweave_not_equal_count;
extern uint64_t g_optiweave_less_than_count;
extern uint64_t g_optiweave_greater_than_count;
extern uint64_t g_optiweave_less_equal_count;
extern uint64_t g_optiweave_greater_equal_count;

/* Assignment counters */
extern uint64_t g_optiweave_assignment_count;
extern uint64_t g_optiweave_add_assign_count;
extern uint64_t g_optiweave_sub_assign_count;
extern uint64_t g_optiweave_mul_assign_count;
extern uint64_t g_optiweave_div_assign_count;
extern uint64_t g_optiweave_mod_assign_count;

/* Arithmetic increment functions */
void optiweave_increment_array_subscript(void);
void optiweave_increment_addition(void);
void optiweave_increment_subtraction(void);
void optiweave_increment_multiplication(void);
void optiweave_increment_division(void);
void optiweave_increment_modulo(void);

/* Comparison increment functions */
void optiweave_increment_equal(void);
void optiweave_increment_not_equal(void);
void optiweave_increment_less_than(void);
void optiweave_increment_greater_than(void);
void optiweave_increment_less_equal(void);
void optiweave_increment_greater_equal(void);

/* Assignment increment functions */
void optiweave_increment_assignment(void);
void optiweave_increment_add_assign(void);
void optiweave_increment_sub_assign(void);
void optiweave_increment_mul_assign(void);
void optiweave_increment_div_assign(void);
void optiweave_increment_mod_assign(void);

/* Reset all counters */
void optiweave_reset_counters(void);

/* Print statistics report */
void optiweave_print_statistics(void);

#ifdef __cplusplus
}
#endif

#endif /* OPTIWEAVE_STATISTICS_C_H */
