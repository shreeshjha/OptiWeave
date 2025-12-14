/*
 * OptiWeave Runtime Implementation for C Programs
 * Enhanced with profiling and nice logging capabilities
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ANSI color codes for nice output
#define COLOR_RESET   "\033[0m"
#define COLOR_BOLD    "\033[1m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"

// Configuration
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
    .log_array_accesses = 0,      // Don't log every access (too verbose)
    .log_arithmetic_ops = 0,
    .log_to_stderr = 1,
    .log_to_file = 0,
    .log_file_path = "optiweave.log",
    .include_timestamps = 0,
    .include_location = 0,
    .enable_profiling = 1,        // Enable profiling
    .show_colors = 1              // Enable colored output
};

// Statistics structures
typedef struct {
  const char *function_name;
  unsigned long access_count;
} FunctionStats;

#define MAX_FUNCTIONS 100
static FunctionStats g_function_stats[MAX_FUNCTIONS];
static int g_num_functions = 0;

// Global counters
static unsigned long g_array_access_count = 0;
static unsigned long g_read_access_count = 0;
static unsigned long g_write_access_count = 0;
static struct timespec g_start_time;
static int g_initialized = 0;

// Initialize runtime
static void __attribute__((constructor)) optiweave_init(void) {
  if (!g_initialized) {
    clock_gettime(CLOCK_MONOTONIC, &g_start_time);
    g_initialized = 1;

    if (g_optiweave_config.show_colors) {
      fprintf(stderr, "%s%s", COLOR_BOLD, COLOR_CYAN);
      fprintf(stderr, "\n╔════════════════════════════════════════════════════════════╗\n");
      fprintf(stderr, "║          OptiWeave Runtime Initialized                     ║\n");
      fprintf(stderr, "║          Array Access Instrumentation Active               ║\n");
      fprintf(stderr, "╚════════════════════════════════════════════════════════════╝\n");
      fprintf(stderr, "%s\n", COLOR_RESET);
    } else {
      fprintf(stderr, "\n=== OptiWeave Runtime Initialized ===\n");
      fprintf(stderr, "Array Access Instrumentation Active\n\n");
    }
  }
}

// Find or create function stats entry
static FunctionStats* get_function_stats(const char *func_name) {
  if (!g_optiweave_config.enable_profiling) {
    return NULL;
  }

  // Search for existing entry
  for (int i = 0; i < g_num_functions; i++) {
    if (strcmp(g_function_stats[i].function_name, func_name) == 0) {
      return &g_function_stats[i];
    }
  }

  // Create new entry if space available
  if (g_num_functions < MAX_FUNCTIONS) {
    g_function_stats[g_num_functions].function_name = func_name;
    g_function_stats[g_num_functions].access_count = 0;
    return &g_function_stats[g_num_functions++];
  }

  return NULL;
}

// Log array access
void __optiweave_log_access(const char *operation, const void *ptr,
                            unsigned long index, const char *file, int line) {
  if (g_optiweave_config.log_array_accesses && g_optiweave_config.log_to_stderr) {
    if (g_optiweave_config.show_colors) {
      fprintf(stderr, "%s[Array Access]%s ", COLOR_YELLOW, COLOR_RESET);
    } else {
      fprintf(stderr, "[Array Access] ");
    }

    if (g_optiweave_config.include_location) {
      fprintf(stderr, "at %s:%d\n", file, line);
    } else {
      fprintf(stderr, "\n");
    }
  }

  // Always count
  g_array_access_count++;
}

// Enhanced subscript recording with profiling
void __optiweave_record_subscript_with_profile(const char *file, int line, const char *func) {
  __optiweave_log_access("array_subscript", NULL, 0, file, line);

  // Update per-function statistics
  if (g_optiweave_config.enable_profiling && func) {
    FunctionStats *stats = get_function_stats(func);
    if (stats) {
      stats->access_count++;
    }
  }
}

// Log arithmetic operation
void __optiweave_log_operation(const char *operation, const char *lhs_type,
                               const char *rhs_type, const char *file,
                               int line) {
  if (g_optiweave_config.log_arithmetic_ops && g_optiweave_config.log_to_stderr) {
    fprintf(stderr, "[OptiWeave] Operation %s: %s, %s at %s:%d\n",
            operation, lhs_type, rhs_type, file, line);
  }
}

// Get statistics
unsigned long optiweave_get_array_access_count(void) {
  return g_array_access_count;
}

void optiweave_reset_counters(void) {
  g_array_access_count = 0;
  g_read_access_count = 0;
  g_write_access_count = 0;
  g_num_functions = 0;
}

// Comparison function for qsort
static int compare_function_stats(const void *a, const void *b) {
  const FunctionStats *fa = (const FunctionStats *)a;
  const FunctionStats *fb = (const FunctionStats *)b;
  if (fb->access_count > fa->access_count) return 1;
  if (fb->access_count < fa->access_count) return -1;
  return 0;
}

// Print comprehensive statistics
void optiweave_print_stats(void) {
  struct timespec end_time;
  clock_gettime(CLOCK_MONOTONIC, &end_time);

  double elapsed = (end_time.tv_sec - g_start_time.tv_sec) +
                   (end_time.tv_nsec - g_start_time.tv_nsec) / 1e9;

  if (g_optiweave_config.show_colors) {
    fprintf(stderr, "\n%s%s", COLOR_BOLD, COLOR_GREEN);
    fprintf(stderr, "╔════════════════════════════════════════════════════════════╗\n");
    fprintf(stderr, "║         OptiWeave Runtime Statistics Report                ║\n");
    fprintf(stderr, "╚════════════════════════════════════════════════════════════╝%s\n\n",
            COLOR_RESET);

    // Overall statistics
    fprintf(stderr, "%s📊 Overall Statistics:%s\n", COLOR_BOLD, COLOR_RESET);
    fprintf(stderr, "   %s▸%s Total array accesses: %s%lu%s\n",
            COLOR_BLUE, COLOR_RESET, COLOR_CYAN, g_array_access_count, COLOR_RESET);
    fprintf(stderr, "   %s▸%s Execution time: %s%.4f seconds%s\n",
            COLOR_BLUE, COLOR_RESET, COLOR_CYAN, elapsed, COLOR_RESET);
    fprintf(stderr, "   %s▸%s Access rate: %s%.0f accesses/second%s\n\n",
            COLOR_BLUE, COLOR_RESET, COLOR_CYAN, g_array_access_count / elapsed, COLOR_RESET);

    // Per-function statistics
    if (g_optiweave_config.enable_profiling && g_num_functions > 0) {
      fprintf(stderr, "%s🔥 Hot Functions (Top Array Access):%s\n", COLOR_BOLD, COLOR_RESET);

      // Sort functions by access count
      qsort(g_function_stats, g_num_functions, sizeof(FunctionStats), compare_function_stats);

      // Show top 10 functions
      int display_count = g_num_functions < 10 ? g_num_functions : 10;
      for (int i = 0; i < display_count; i++) {
        double percentage = (100.0 * g_function_stats[i].access_count) / g_array_access_count;

        // Choose color based on percentage
        const char *bar_color;
        if (percentage > 10.0) bar_color = COLOR_RED;
        else if (percentage > 5.0) bar_color = COLOR_YELLOW;
        else bar_color = COLOR_GREEN;

        fprintf(stderr, "   %2d. %s%-30s%s %s%6lu%s accesses (%s%5.1f%%%s) ",
                i + 1,
                COLOR_MAGENTA, g_function_stats[i].function_name, COLOR_RESET,
                COLOR_CYAN, g_function_stats[i].access_count, COLOR_RESET,
                bar_color, percentage, COLOR_RESET);

        // Print simple bar chart
        int bar_length = (int)(percentage / 2);  // Scale to 50 chars max
        fprintf(stderr, "%s", bar_color);
        for (int j = 0; j < bar_length; j++) {
          fprintf(stderr, "█");
        }
        fprintf(stderr, "%s\n", COLOR_RESET);
      }

      if (g_num_functions > 10) {
        fprintf(stderr, "   %s... and %d more functions%s\n",
                COLOR_YELLOW, g_num_functions - 10, COLOR_RESET);
      }
    }

    fprintf(stderr, "\n%s%s", COLOR_BOLD, COLOR_GREEN);
    fprintf(stderr, "╔════════════════════════════════════════════════════════════╗\n");
    fprintf(stderr, "║              End of OptiWeave Report                       ║\n");
    fprintf(stderr, "╚════════════════════════════════════════════════════════════╝%s\n",
            COLOR_RESET);
  } else {
    // Plain text output (no colors)
    fprintf(stderr, "\n=== OptiWeave Runtime Statistics ===\n");
    fprintf(stderr, "Total array accesses: %lu\n", g_array_access_count);
    fprintf(stderr, "Execution time: %.4f seconds\n", elapsed);
    fprintf(stderr, "Access rate: %.0f accesses/second\n", g_array_access_count / elapsed);

    if (g_optiweave_config.enable_profiling && g_num_functions > 0) {
      fprintf(stderr, "\nHot Functions (Top Array Access):\n");
      qsort(g_function_stats, g_num_functions, sizeof(FunctionStats), compare_function_stats);

      int display_count = g_num_functions < 10 ? g_num_functions : 10;
      for (int i = 0; i < display_count; i++) {
        double percentage = (100.0 * g_function_stats[i].access_count) / g_array_access_count;
        fprintf(stderr, "  %2d. %-30s %6lu accesses (%.1f%%)\n",
                i + 1, g_function_stats[i].function_name,
                g_function_stats[i].access_count, percentage);
      }
    }

    fprintf(stderr, "====================================\n");
  }
}

// Cleanup function (called at exit)
static void __attribute__((destructor)) optiweave_cleanup(void) {
  // Stats are printed explicitly by demo
}
