/* OptiWeave C-compatible statistics implementation */

#include "../../../include/optiweave/runtime/statistics_c.h"
#include <stdio.h>

/* Global counters - Arithmetic */
uint64_t g_optiweave_array_subscript_count = 0;
uint64_t g_optiweave_addition_count = 0;
uint64_t g_optiweave_subtraction_count = 0;
uint64_t g_optiweave_multiplication_count = 0;
uint64_t g_optiweave_division_count = 0;
uint64_t g_optiweave_modulo_count = 0;

/* Global counters - Comparison */
uint64_t g_optiweave_equal_count = 0;
uint64_t g_optiweave_not_equal_count = 0;
uint64_t g_optiweave_less_than_count = 0;
uint64_t g_optiweave_greater_than_count = 0;
uint64_t g_optiweave_less_equal_count = 0;
uint64_t g_optiweave_greater_equal_count = 0;

/* Global counters - Assignment */
uint64_t g_optiweave_assignment_count = 0;
uint64_t g_optiweave_add_assign_count = 0;
uint64_t g_optiweave_sub_assign_count = 0;
uint64_t g_optiweave_mul_assign_count = 0;
uint64_t g_optiweave_div_assign_count = 0;
uint64_t g_optiweave_mod_assign_count = 0;

/* Arithmetic increment functions */
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

/* Comparison increment functions */
void optiweave_increment_equal(void) {
  __sync_fetch_and_add(&g_optiweave_equal_count, 1);
}

void optiweave_increment_not_equal(void) {
  __sync_fetch_and_add(&g_optiweave_not_equal_count, 1);
}

void optiweave_increment_less_than(void) {
  __sync_fetch_and_add(&g_optiweave_less_than_count, 1);
}

void optiweave_increment_greater_than(void) {
  __sync_fetch_and_add(&g_optiweave_greater_than_count, 1);
}

void optiweave_increment_less_equal(void) {
  __sync_fetch_and_add(&g_optiweave_less_equal_count, 1);
}

void optiweave_increment_greater_equal(void) {
  __sync_fetch_and_add(&g_optiweave_greater_equal_count, 1);
}

/* Assignment increment functions */
void optiweave_increment_assignment(void) {
  __sync_fetch_and_add(&g_optiweave_assignment_count, 1);
}

void optiweave_increment_add_assign(void) {
  __sync_fetch_and_add(&g_optiweave_add_assign_count, 1);
}

void optiweave_increment_sub_assign(void) {
  __sync_fetch_and_add(&g_optiweave_sub_assign_count, 1);
}

void optiweave_increment_mul_assign(void) {
  __sync_fetch_and_add(&g_optiweave_mul_assign_count, 1);
}

void optiweave_increment_div_assign(void) {
  __sync_fetch_and_add(&g_optiweave_div_assign_count, 1);
}

void optiweave_increment_mod_assign(void) {
  __sync_fetch_and_add(&g_optiweave_mod_assign_count, 1);
}

/* Reset all counters */
void optiweave_reset_counters(void) {
  g_optiweave_array_subscript_count = 0;
  g_optiweave_addition_count = 0;
  g_optiweave_subtraction_count = 0;
  g_optiweave_multiplication_count = 0;
  g_optiweave_division_count = 0;
  g_optiweave_modulo_count = 0;
  g_optiweave_equal_count = 0;
  g_optiweave_not_equal_count = 0;
  g_optiweave_less_than_count = 0;
  g_optiweave_greater_than_count = 0;
  g_optiweave_less_equal_count = 0;
  g_optiweave_greater_equal_count = 0;
  g_optiweave_assignment_count = 0;
  g_optiweave_add_assign_count = 0;
  g_optiweave_sub_assign_count = 0;
  g_optiweave_mul_assign_count = 0;
  g_optiweave_div_assign_count = 0;
  g_optiweave_mod_assign_count = 0;
}

/* Print statistics report */
void optiweave_print_statistics(void) {
  uint64_t total_comparisons = g_optiweave_equal_count + g_optiweave_not_equal_count +
                               g_optiweave_less_than_count + g_optiweave_greater_than_count +
                               g_optiweave_less_equal_count + g_optiweave_greater_equal_count;
  uint64_t total_assignments = g_optiweave_assignment_count + g_optiweave_add_assign_count +
                               g_optiweave_sub_assign_count + g_optiweave_mul_assign_count +
                               g_optiweave_div_assign_count + g_optiweave_mod_assign_count;
  
  printf("\n=== OptiWeave Statistics ===\n");
  printf("Array Subscripts:       %llu\n", (unsigned long long)g_optiweave_array_subscript_count);
  printf("\n-- Arithmetic --\n");
  printf("Additions:              %llu\n", (unsigned long long)g_optiweave_addition_count);
  printf("Subtractions:           %llu\n", (unsigned long long)g_optiweave_subtraction_count);
  printf("Multiplications:        %llu\n", (unsigned long long)g_optiweave_multiplication_count);
  printf("Divisions:              %llu\n", (unsigned long long)g_optiweave_division_count);
  printf("Modulo:                 %llu\n", (unsigned long long)g_optiweave_modulo_count);
  printf("\n-- Comparisons (total: %llu) --\n", (unsigned long long)total_comparisons);
  printf("Equal (==):             %llu\n", (unsigned long long)g_optiweave_equal_count);
  printf("Not Equal (!=):         %llu\n", (unsigned long long)g_optiweave_not_equal_count);
  printf("Less Than (<):          %llu\n", (unsigned long long)g_optiweave_less_than_count);
  printf("Greater Than (>):       %llu\n", (unsigned long long)g_optiweave_greater_than_count);
  printf("Less Equal (<=):        %llu\n", (unsigned long long)g_optiweave_less_equal_count);
  printf("Greater Equal (>=):     %llu\n", (unsigned long long)g_optiweave_greater_equal_count);
  printf("\n-- Assignments (total: %llu) --\n", (unsigned long long)total_assignments);
  printf("Assignment (=):         %llu\n", (unsigned long long)g_optiweave_assignment_count);
  printf("Add Assign (+=):        %llu\n", (unsigned long long)g_optiweave_add_assign_count);
  printf("Sub Assign (-=):        %llu\n", (unsigned long long)g_optiweave_sub_assign_count);
  printf("Mul Assign (*=):        %llu\n", (unsigned long long)g_optiweave_mul_assign_count);
  printf("Div Assign (/=):        %llu\n", (unsigned long long)g_optiweave_div_assign_count);
  printf("Mod Assign (%%=):        %llu\n", (unsigned long long)g_optiweave_mod_assign_count);
  printf("============================\n\n");
}
