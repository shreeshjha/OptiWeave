/* OptiWeave C-compatible statistics implementation */

#include "../../../include/optiweave/runtime/statistics_c.h"
#include <stdio.h>

/* Global counters */
uint64_t g_optiweave_array_subscript_count = 0;
uint64_t g_optiweave_addition_count = 0;
uint64_t g_optiweave_subtraction_count = 0;
uint64_t g_optiweave_multiplication_count = 0;
uint64_t g_optiweave_division_count = 0;
uint64_t g_optiweave_modulo_count = 0;
uint64_t g_optiweave_comparison_count = 0;
uint64_t g_optiweave_assignment_count = 0;
uint64_t g_optiweave_compound_assignment_count = 0;

/* Increment functions */
void optiweave_increment_array_subscript(void) {
  __sync_fetch_and_add(&g_optiweave_array_subscript_count, 1);
}

/* Profile-enabled subscript recording (used by prelude_c.h) */
void __optiweave_record_subscript_with_profile(const char *file, int line, const char *func) {
  /* For now, just increment the counter.
     TODO: Add file/line/function profiling like the C++ version */
  optiweave_increment_array_subscript();
}

void optiweave_increment_addition(void) {
  __sync_fetch_and_add(&g_optiweave_addition_count, 1);
}

void optiweave_increment_subtraction(void) {
  __sync_fetch_and_add(&g_optiweave_subtraction_count, 1);
}

void optiweave_increment_multiplication(void) {
  __sync_fetch_and_add(&g_optiweave_multiplication_count, 1);
}

void optiweave_increment_division(void) {
  __sync_fetch_and_add(&g_optiweave_division_count, 1);
}

void optiweave_increment_modulo(void) {
  __sync_fetch_and_add(&g_optiweave_modulo_count, 1);
}

void optiweave_increment_comparison(void) {
  __sync_fetch_and_add(&g_optiweave_comparison_count, 1);
}

void optiweave_increment_assignment(void) {
  __sync_fetch_and_add(&g_optiweave_assignment_count, 1);
}

void optiweave_increment_compound_assignment(void) {
  __sync_fetch_and_add(&g_optiweave_compound_assignment_count, 1);
}

/* Reset all counters */
void optiweave_reset_counters(void) {
  g_optiweave_array_subscript_count = 0;
  g_optiweave_addition_count = 0;
  g_optiweave_subtraction_count = 0;
  g_optiweave_multiplication_count = 0;
  g_optiweave_division_count = 0;
  g_optiweave_modulo_count = 0;
  g_optiweave_comparison_count = 0;
  g_optiweave_assignment_count = 0;
  g_optiweave_compound_assignment_count = 0;
}

/* Print statistics report */
void optiweave_print_statistics(void) {
  printf("\n=== OptiWeave Statistics ===\n");
  printf("Array Subscripts:       %llu\n", (unsigned long long)g_optiweave_array_subscript_count);
  printf("Additions:              %llu\n", (unsigned long long)g_optiweave_addition_count);
  printf("Subtractions:           %llu\n", (unsigned long long)g_optiweave_subtraction_count);
  printf("Multiplications:        %llu\n", (unsigned long long)g_optiweave_multiplication_count);
  printf("Divisions:              %llu\n", (unsigned long long)g_optiweave_division_count);
  printf("Modulo:                 %llu\n", (unsigned long long)g_optiweave_modulo_count);
  printf("Comparisons:            %llu\n", (unsigned long long)g_optiweave_comparison_count);
  printf("Assignments:            %llu\n", (unsigned long long)g_optiweave_assignment_count);
  printf("Compound Assignments:   %llu\n", (unsigned long long)g_optiweave_compound_assignment_count);
  printf("============================\n\n");
}
