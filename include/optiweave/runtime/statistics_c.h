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
extern uint64_t g_optiweave_comparison_count;
extern uint64_t g_optiweave_assignment_count;
extern uint64_t g_optiweave_compound_assignment_count;

/* Increment functions */
void optiweave_increment_array_subscript(void);
void optiweave_increment_addition(void);
void optiweave_increment_subtraction(void);
void optiweave_increment_multiplication(void);
void optiweave_increment_division(void);
void optiweave_increment_modulo(void);
void optiweave_increment_comparison(void);
void optiweave_increment_assignment(void);
void optiweave_increment_compound_assignment(void);

/* Reset all counters */
void optiweave_reset_counters(void);

/* Print statistics report */
void optiweave_print_statistics(void);

#ifdef __cplusplus
}
#endif

#endif /* OPTIWEAVE_STATISTICS_C_H */
