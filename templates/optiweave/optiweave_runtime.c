/*
 * OptiWeave Runtime Implementation for C Programs
 * Self-contained: includes operation counters, per-function profiling, and stats reporting.
 *
 * Compile this file alongside your instrumented C source:
 *   gcc -DOPTIWEAVE_ENABLE_STATS instrumented.c optiweave_runtime.c -o program -lm
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

/* ANSI color codes */
#define COLOR_RESET   "\033[0m"
#define COLOR_BOLD    "\033[1m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"

/* =========================================================================
 * Operation counters (satisfy extern declarations in statistics_c.h)
 * ========================================================================= */

uint64_t g_optiweave_array_subscript_count = 0;
uint64_t g_optiweave_addition_count        = 0;
uint64_t g_optiweave_subtraction_count     = 0;
uint64_t g_optiweave_multiplication_count  = 0;
uint64_t g_optiweave_division_count        = 0;
uint64_t g_optiweave_modulo_count          = 0;

uint64_t g_optiweave_equal_count         = 0;
uint64_t g_optiweave_not_equal_count     = 0;
uint64_t g_optiweave_less_than_count     = 0;
uint64_t g_optiweave_greater_than_count  = 0;
uint64_t g_optiweave_less_equal_count    = 0;
uint64_t g_optiweave_greater_equal_count = 0;

uint64_t g_optiweave_assignment_count  = 0;
uint64_t g_optiweave_add_assign_count  = 0;
uint64_t g_optiweave_sub_assign_count  = 0;
uint64_t g_optiweave_mul_assign_count  = 0;
uint64_t g_optiweave_div_assign_count  = 0;
uint64_t g_optiweave_mod_assign_count  = 0;

/* Arithmetic increment functions */
void optiweave_increment_array_subscript(void) {
  __sync_fetch_and_add(&g_optiweave_array_subscript_count, 1);
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

/* =========================================================================
 * Per-function profiling (array access hotspots)
 * ========================================================================= */

typedef struct {
  const char *function_name;
  unsigned long access_count;
} FunctionStats;

#define MAX_FUNCTIONS 100
static FunctionStats g_function_stats[MAX_FUNCTIONS];
static int g_num_functions = 0;

static struct timespec g_start_time;
static int g_initialized = 0;

/* =========================================================================
 * Runtime configuration
 * ========================================================================= */

typedef struct {
  int log_array_accesses;
  int log_arithmetic_ops;
  int log_to_stderr;
  int log_to_file;
  const char *log_file_path;
  int include_timestamps;
  int include_location;
  int enable_profiling;
  int show_colors;
} OptiWeaveInstrumentationConfig;

OptiWeaveInstrumentationConfig g_optiweave_config = {
    .log_array_accesses = 0,
    .log_arithmetic_ops = 0,
    .log_to_stderr      = 1,
    .log_to_file        = 0,
    .log_file_path      = "optiweave.log",
    .include_timestamps = 0,
    .include_location   = 0,
    .enable_profiling   = 1,
    .show_colors        = 1
};

/* =========================================================================
 * Initialization
 * ========================================================================= */

static void __attribute__((constructor)) optiweave_init(void) {
  if (!g_initialized) {
    clock_gettime(CLOCK_MONOTONIC, &g_start_time);
    g_initialized = 1;

    if (g_optiweave_config.show_colors) {
      fprintf(stderr, "%s%s", COLOR_BOLD, COLOR_CYAN);
      fprintf(stderr, "\n╔════════════════════════════════════════════════════════════╗\n");
      fprintf(stderr, "║          OptiWeave Runtime Initialized                     ║\n");
      fprintf(stderr, "║          Instrumentation Active                            ║\n");
      fprintf(stderr, "╚════════════════════════════════════════════════════════════╝\n");
      fprintf(stderr, "%s\n", COLOR_RESET);
    } else {
      fprintf(stderr, "\n=== OptiWeave Runtime Initialized ===\n\n");
    }
  }
}

/* =========================================================================
 * Core instrumentation functions
 * ========================================================================= */

static FunctionStats* get_function_stats(const char *func_name) {
  if (!g_optiweave_config.enable_profiling) return NULL;

  for (int i = 0; i < g_num_functions; i++) {
    if (strcmp(g_function_stats[i].function_name, func_name) == 0)
      return &g_function_stats[i];
  }

  if (g_num_functions < MAX_FUNCTIONS) {
    g_function_stats[g_num_functions].function_name = func_name;
    g_function_stats[g_num_functions].access_count  = 0;
    return &g_function_stats[g_num_functions++];
  }
  return NULL;
}

void __optiweave_log_access(const char *operation, const void *ptr,
                            unsigned long index, const char *file, int line) {
  (void)operation; (void)ptr; (void)index;
  if (g_optiweave_config.log_array_accesses && g_optiweave_config.log_to_stderr) {
    if (g_optiweave_config.show_colors)
      fprintf(stderr, "%s[Array Access]%s ", COLOR_YELLOW, COLOR_RESET);
    else
      fprintf(stderr, "[Array Access] ");

    if (g_optiweave_config.include_location)
      fprintf(stderr, "at %s:%d\n", file, line);
    else
      fprintf(stderr, "\n");
  }
}

void __optiweave_log_operation(const char *operation, const char *lhs_type,
                               const char *rhs_type, const char *file, int line) {
  if (g_optiweave_config.log_arithmetic_ops && g_optiweave_config.log_to_stderr)
    fprintf(stderr, "[OptiWeave] Operation %s: %s, %s at %s:%d\n",
            operation, lhs_type, rhs_type, file, line);
}

/* Records an array subscript and updates per-function profiling.
 * Also increments the global subscript counter (satisfies prelude_c.h needs). */
void __optiweave_record_subscript_with_profile(const char *file, int line,
                                               const char *func) {
  __optiweave_log_access("array_subscript", NULL, 0, file, line);
  optiweave_increment_array_subscript();

  if (g_optiweave_config.enable_profiling && func) {
    FunctionStats *stats = get_function_stats(func);
    if (stats) stats->access_count++;
  }
}

/* =========================================================================
 * Statistics query helpers
 * ========================================================================= */

unsigned long optiweave_get_array_access_count(void) {
  return (unsigned long)g_optiweave_array_subscript_count;
}

void optiweave_reset_counters(void) {
  g_optiweave_array_subscript_count = 0;
  g_optiweave_addition_count        = 0;
  g_optiweave_subtraction_count     = 0;
  g_optiweave_multiplication_count  = 0;
  g_optiweave_division_count        = 0;
  g_optiweave_modulo_count          = 0;

  g_optiweave_equal_count         = 0;
  g_optiweave_not_equal_count     = 0;
  g_optiweave_less_than_count     = 0;
  g_optiweave_greater_than_count  = 0;
  g_optiweave_less_equal_count    = 0;
  g_optiweave_greater_equal_count = 0;

  g_optiweave_assignment_count  = 0;
  g_optiweave_add_assign_count  = 0;
  g_optiweave_sub_assign_count  = 0;
  g_optiweave_mul_assign_count  = 0;
  g_optiweave_div_assign_count  = 0;
  g_optiweave_mod_assign_count  = 0;

  g_num_functions = 0;
}

/* =========================================================================
 * Statistics reporting
 * ========================================================================= */

static int compare_function_stats(const void *a, const void *b) {
  const FunctionStats *fa = (const FunctionStats *)a;
  const FunctionStats *fb = (const FunctionStats *)b;
  if (fb->access_count > fa->access_count) return  1;
  if (fb->access_count < fa->access_count) return -1;
  return 0;
}

/* optiweave_print_statistics: primary stats report (also satisfies statistics_c.h) */
void optiweave_print_statistics(void) {
  struct timespec end_time;
  clock_gettime(CLOCK_MONOTONIC, &end_time);
  double elapsed = (end_time.tv_sec  - g_start_time.tv_sec) +
                   (end_time.tv_nsec - g_start_time.tv_nsec) / 1e9;

  uint64_t total_arith = g_optiweave_addition_count + g_optiweave_subtraction_count +
                         g_optiweave_multiplication_count + g_optiweave_division_count +
                         g_optiweave_modulo_count;
  uint64_t total_cmp   = g_optiweave_equal_count + g_optiweave_not_equal_count +
                         g_optiweave_less_than_count + g_optiweave_greater_than_count +
                         g_optiweave_less_equal_count + g_optiweave_greater_equal_count;
  uint64_t total_asgn  = g_optiweave_assignment_count + g_optiweave_add_assign_count +
                         g_optiweave_sub_assign_count + g_optiweave_mul_assign_count +
                         g_optiweave_div_assign_count + g_optiweave_mod_assign_count;

  if (g_optiweave_config.show_colors) {
    fprintf(stderr, "\n%s%s", COLOR_BOLD, COLOR_GREEN);
    fprintf(stderr, "╔════════════════════════════════════════════════════════════╗\n");
    fprintf(stderr, "║         OptiWeave Runtime Statistics Report                ║\n");
    fprintf(stderr, "╚════════════════════════════════════════════════════════════╝%s\n\n",
            COLOR_RESET);

    fprintf(stderr, "%s⏱  Execution time:%s  %.4f seconds\n\n",
            COLOR_BOLD, COLOR_RESET, elapsed);

    fprintf(stderr, "%s📊 Operation Counts:%s\n", COLOR_BOLD, COLOR_RESET);
    fprintf(stderr, "   %sArray Subscripts:%s  %s%llu%s\n",
            COLOR_BLUE, COLOR_RESET, COLOR_CYAN,
            (unsigned long long)g_optiweave_array_subscript_count, COLOR_RESET);

    fprintf(stderr, "\n   %sArithmetic (total: %llu):%s\n",
            COLOR_BLUE, (unsigned long long)total_arith, COLOR_RESET);
    fprintf(stderr, "     + additions:       %s%llu%s\n", COLOR_CYAN,
            (unsigned long long)g_optiweave_addition_count, COLOR_RESET);
    fprintf(stderr, "     - subtractions:    %s%llu%s\n", COLOR_CYAN,
            (unsigned long long)g_optiweave_subtraction_count, COLOR_RESET);
    fprintf(stderr, "     * multiplications: %s%llu%s\n", COLOR_CYAN,
            (unsigned long long)g_optiweave_multiplication_count, COLOR_RESET);
    fprintf(stderr, "     / divisions:       %s%llu%s\n", COLOR_CYAN,
            (unsigned long long)g_optiweave_division_count, COLOR_RESET);
    fprintf(stderr, "     %% modulo:           %s%llu%s\n", COLOR_CYAN,
            (unsigned long long)g_optiweave_modulo_count, COLOR_RESET);

    fprintf(stderr, "\n   %sComparisons (total: %llu):%s\n",
            COLOR_BLUE, (unsigned long long)total_cmp, COLOR_RESET);
    fprintf(stderr, "     ==  equal:         %s%llu%s\n", COLOR_CYAN,
            (unsigned long long)g_optiweave_equal_count, COLOR_RESET);
    fprintf(stderr, "     !=  not equal:     %s%llu%s\n", COLOR_CYAN,
            (unsigned long long)g_optiweave_not_equal_count, COLOR_RESET);
    fprintf(stderr, "     <   less than:     %s%llu%s\n", COLOR_CYAN,
            (unsigned long long)g_optiweave_less_than_count, COLOR_RESET);
    fprintf(stderr, "     >   greater than:  %s%llu%s\n", COLOR_CYAN,
            (unsigned long long)g_optiweave_greater_than_count, COLOR_RESET);
    fprintf(stderr, "     <=  less equal:    %s%llu%s\n", COLOR_CYAN,
            (unsigned long long)g_optiweave_less_equal_count, COLOR_RESET);
    fprintf(stderr, "     >=  greater equal: %s%llu%s\n", COLOR_CYAN,
            (unsigned long long)g_optiweave_greater_equal_count, COLOR_RESET);

    fprintf(stderr, "\n   %sAssignments (total: %llu):%s\n",
            COLOR_BLUE, (unsigned long long)total_asgn, COLOR_RESET);
    fprintf(stderr, "     =   assignment:    %s%llu%s\n", COLOR_CYAN,
            (unsigned long long)g_optiweave_assignment_count, COLOR_RESET);
    fprintf(stderr, "     +=  add assign:    %s%llu%s\n", COLOR_CYAN,
            (unsigned long long)g_optiweave_add_assign_count, COLOR_RESET);
    fprintf(stderr, "     -=  sub assign:    %s%llu%s\n", COLOR_CYAN,
            (unsigned long long)g_optiweave_sub_assign_count, COLOR_RESET);
    fprintf(stderr, "     *=  mul assign:    %s%llu%s\n", COLOR_CYAN,
            (unsigned long long)g_optiweave_mul_assign_count, COLOR_RESET);
    fprintf(stderr, "     /=  div assign:    %s%llu%s\n", COLOR_CYAN,
            (unsigned long long)g_optiweave_div_assign_count, COLOR_RESET);
    fprintf(stderr, "     %%=  mod assign:    %s%llu%s\n", COLOR_CYAN,
            (unsigned long long)g_optiweave_mod_assign_count, COLOR_RESET);

    /* Per-function hotspot table */
    if (g_optiweave_config.enable_profiling && g_num_functions > 0) {
      fprintf(stderr, "\n%s🔥 Hot Functions (Array Subscript Hotspots):%s\n",
              COLOR_BOLD, COLOR_RESET);
      qsort(g_function_stats, g_num_functions, sizeof(FunctionStats),
            compare_function_stats);

      uint64_t total_subscripts = g_optiweave_array_subscript_count;
      if (total_subscripts == 0) total_subscripts = 1; /* avoid div-by-zero */

      int display = g_num_functions < 10 ? g_num_functions : 10;
      for (int i = 0; i < display; i++) {
        double pct = (100.0 * g_function_stats[i].access_count) / total_subscripts;
        const char *bar_color = (pct > 10.0) ? COLOR_RED
                              : (pct >  5.0) ? COLOR_YELLOW
                              : COLOR_GREEN;
        fprintf(stderr, "   %2d. %s%-30s%s %s%6lu%s accesses (%s%5.1f%%%s) ",
                i + 1,
                COLOR_MAGENTA, g_function_stats[i].function_name, COLOR_RESET,
                COLOR_CYAN, g_function_stats[i].access_count, COLOR_RESET,
                bar_color, pct, COLOR_RESET);
        int bar = (int)(pct / 2);
        fprintf(stderr, "%s", bar_color);
        for (int j = 0; j < bar; j++) fprintf(stderr, "█");
        fprintf(stderr, "%s\n", COLOR_RESET);
      }
      if (g_num_functions > 10)
        fprintf(stderr, "   %s... and %d more functions%s\n",
                COLOR_YELLOW, g_num_functions - 10, COLOR_RESET);
    }

    fprintf(stderr, "\n%s%s", COLOR_BOLD, COLOR_GREEN);
    fprintf(stderr, "╔════════════════════════════════════════════════════════════╗\n");
    fprintf(stderr, "║              End of OptiWeave Report                       ║\n");
    fprintf(stderr, "╚════════════════════════════════════════════════════════════╝%s\n",
            COLOR_RESET);
  } else {
    /* Plain text output */
    fprintf(stderr, "\n=== OptiWeave Statistics ===\n");
    fprintf(stderr, "Execution time:         %.4f seconds\n", elapsed);
    fprintf(stderr, "Array subscripts:       %llu\n",
            (unsigned long long)g_optiweave_array_subscript_count);
    fprintf(stderr, "\n-- Arithmetic (total: %llu) --\n",
            (unsigned long long)total_arith);
    fprintf(stderr, "Additions:              %llu\n",
            (unsigned long long)g_optiweave_addition_count);
    fprintf(stderr, "Subtractions:           %llu\n",
            (unsigned long long)g_optiweave_subtraction_count);
    fprintf(stderr, "Multiplications:        %llu\n",
            (unsigned long long)g_optiweave_multiplication_count);
    fprintf(stderr, "Divisions:              %llu\n",
            (unsigned long long)g_optiweave_division_count);
    fprintf(stderr, "Modulo:                 %llu\n",
            (unsigned long long)g_optiweave_modulo_count);
    fprintf(stderr, "\n-- Comparisons (total: %llu) --\n",
            (unsigned long long)total_cmp);
    fprintf(stderr, "Equal (==):             %llu\n",
            (unsigned long long)g_optiweave_equal_count);
    fprintf(stderr, "Not Equal (!=):         %llu\n",
            (unsigned long long)g_optiweave_not_equal_count);
    fprintf(stderr, "Less Than (<):          %llu\n",
            (unsigned long long)g_optiweave_less_than_count);
    fprintf(stderr, "Greater Than (>):       %llu\n",
            (unsigned long long)g_optiweave_greater_than_count);
    fprintf(stderr, "Less Equal (<=):        %llu\n",
            (unsigned long long)g_optiweave_less_equal_count);
    fprintf(stderr, "Greater Equal (>=):     %llu\n",
            (unsigned long long)g_optiweave_greater_equal_count);
    fprintf(stderr, "\n-- Assignments (total: %llu) --\n",
            (unsigned long long)total_asgn);
    fprintf(stderr, "Assignment (=):         %llu\n",
            (unsigned long long)g_optiweave_assignment_count);
    fprintf(stderr, "Add Assign (+=):        %llu\n",
            (unsigned long long)g_optiweave_add_assign_count);
    fprintf(stderr, "Sub Assign (-=):        %llu\n",
            (unsigned long long)g_optiweave_sub_assign_count);
    fprintf(stderr, "Mul Assign (*=):        %llu\n",
            (unsigned long long)g_optiweave_mul_assign_count);
    fprintf(stderr, "Div Assign (/=):        %llu\n",
            (unsigned long long)g_optiweave_div_assign_count);
    fprintf(stderr, "Mod Assign (%%=):        %llu\n",
            (unsigned long long)g_optiweave_mod_assign_count);

    if (g_optiweave_config.enable_profiling && g_num_functions > 0) {
      fprintf(stderr, "\nHot Functions (Array Subscripts):\n");
      qsort(g_function_stats, g_num_functions, sizeof(FunctionStats),
            compare_function_stats);
      uint64_t total_subscripts = g_optiweave_array_subscript_count;
      if (total_subscripts == 0) total_subscripts = 1;
      int display = g_num_functions < 10 ? g_num_functions : 10;
      for (int i = 0; i < display; i++) {
        double pct = (100.0 * g_function_stats[i].access_count) / total_subscripts;
        fprintf(stderr, "  %2d. %-30s %6lu accesses (%.1f%%)\n",
                i + 1, g_function_stats[i].function_name,
                g_function_stats[i].access_count, pct);
      }
    }
    fprintf(stderr, "============================\n\n");
  }
}

/* Legacy alias kept for backward compatibility */
void optiweave_print_stats(void) {
  optiweave_print_statistics();
}

/* =========================================================================
 * Cleanup (called at process exit)
 * ========================================================================= */

static void __attribute__((destructor)) optiweave_cleanup(void) {
#ifdef OPTIWEAVE_ENABLE_STATS
  optiweave_print_statistics();
#endif
}
