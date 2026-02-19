# Makefile for OptiWeave tiny-AES-c Demo
# =========================================

CC = clang
CFLAGS = -Wall -Wextra -O2 -DAES128=1 -DCBC=1 -DCTR=1 -DECB=1
INCLUDES = -I../../templates -I.
LDFLAGS = 

# Source files
SOURCES = aes_instrumented.c benchmark.c optiweave_runtime.c
OBJECTS = $(SOURCES:.c=.o)
TARGET = aes_benchmark

# Build rules
all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "Linking $(TARGET)..."
	$(CC) $(LDFLAGS) -o $@ $^
	@echo "Build complete: $(TARGET)"

%.o: %.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Run benchmark
run: $(TARGET)
	@echo ""
	@echo "==============================================="
	@echo "Running AES-128 Benchmark with OptiWeave"
	@echo "==============================================="
	@./$(TARGET)

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -f $(OBJECTS) $(TARGET)
	@echo "Clean complete"

# Rebuild from scratch
rebuild: clean all

# Help
help:
	@echo "OptiWeave tiny-AES-c Demo Makefile"
	@echo "==================================="
	@echo ""
	@echo "Targets:"
	@echo "  all      - Build the benchmark (default)"
	@echo "  run      - Build and run the benchmark"
	@echo "  clean    - Remove build artifacts"
	@echo "  rebuild  - Clean and rebuild from scratch"
	@echo "  help     - Show this help message"
	@echo ""
	@echo "Files:"
	@echo "  aes_instrumented.c    - OptiWeave instrumented AES"
	@echo "  benchmark.c           - Test driver with 100K iterations"
	@echo "  optiweave_runtime.c   - OptiWeave runtime support"
	@echo "  aes.h                 - AES header file"

.PHONY: all run clean rebuild help
