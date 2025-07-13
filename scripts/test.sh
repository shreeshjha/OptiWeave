#!/bin/bash

# OptiWeave Test Script
# Usage: ./scripts/test.sh [options]

set -e  # Exit on any error

# Configuration
BUILD_DIR="${BUILD_DIR:-build}"
TEST_PATTERN="${TEST_PATTERN:-*}"
PARALLEL_JOBS="${PARALLEL_JOBS:-$(nproc)}"
ENABLE_COVERAGE="${ENABLE_COVERAGE:-false}"
ENABLE_VALGRIND="${ENABLE_VALGRIND:-false}"
ENABLE_SANITIZERS="${ENABLE_SANITIZERS:-false}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Help function
show_help() {
    cat << EOF
OptiWeave Test Script

Usage: $0 [OPTIONS]

OPTIONS:
    -h, --help              Show this help message
    -u, --unit              Run only unit tests
    -i, --integration       Run only integration tests
    -p, --pattern PATTERN   Run tests matching pattern
    -j, --jobs NUM          Number of parallel test jobs
    -c, --coverage          Enable code coverage
    -v, --valgrind          Run tests under Valgrind
    -s, --sanitizers        Enable AddressSanitizer and UBSan
    --memcheck              Run memory leak detection
    --verbose               Enable verbose test output
    --xml                   Generate XML test reports
    --benchmark             Run performance benchmarks

EXAMPLES:
    $0                      # Run all tests
    $0 -u                   # Run only unit tests
    $0 -p "test_ast*"       # Run tests matching pattern
    $0 -c                   # Run with coverage
    $0 -v -s                # Run with Valgrind and sanitizers

EOF
}

# Parse command line arguments
RUN_UNIT_TESTS=true
RUN_INTEGRATION_TESTS=true
VERBOSE_OUTPUT=false
XML_OUTPUT=false
RUN_BENCHMARKS=false
ENABLE_MEMCHECK=false

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -u|--unit)
            RUN_UNIT_TESTS=true
            RUN_INTEGRATION_TESTS=false
            shift
            ;;
        -i|--integration)
            RUN_UNIT_TESTS=false
            RUN_INTEGRATION_TESTS=true
            shift
            ;;
        -p|--pattern)
            TEST_PATTERN="$2"
            shift 2
            ;;
        -j|--jobs)
            PARALLEL_JOBS="$2"
            shift 2
            ;;
        -c|--coverage)
            ENABLE_COVERAGE=true
            shift
            ;;
        -v|--valgrind)
            ENABLE_VALGRIND=true
            shift
            ;;
        -s|--sanitizers)
            ENABLE_SANITIZERS=true
            shift
            ;;
        --memcheck)
            ENABLE_MEMCHECK=true
            shift
            ;;
        --verbose)
            VERBOSE_OUTPUT=true
            shift
            ;;
        --xml)
            XML_OUTPUT=true
            shift
            ;;
        --benchmark)
            RUN_BENCHMARKS=true
            shift
            ;;
        *)
            log_error "Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

log_info "OptiWeave Test Configuration:"
log_info "  Project root: $PROJECT_ROOT"
log_info "  Build directory: $BUILD_DIR"
log_info "  Test pattern: $TEST_PATTERN"
log_info "  Parallel jobs: $PARALLEL_JOBS"
log_info "  Unit tests: $RUN_UNIT_TESTS"
log_info "  Integration tests: $RUN_INTEGRATION_TESTS"
log_info "  Coverage: $ENABLE_COVERAGE"
log_info "  Valgrind: $ENABLE_VALGRIND"
log_info "  Sanitizers: $ENABLE_SANITIZERS"

# Change to project root
cd "$PROJECT_ROOT"

# Check if build directory exists
if [[ ! -d "$BUILD_DIR" ]]; then
    log_error "Build directory '$BUILD_DIR' does not exist"
    log_info "Please run './scripts/build.sh' first"
    exit 1
fi

cd "$BUILD_DIR"

# Check for required tools
check_tool() {
    if ! command -v "$1" &> /dev/null; then
        log_warning "$1 not found, some features may be disabled"
        return 1
    fi
    return 0
}

# Check for optional tools
if [[ "$ENABLE_VALGRIND" == true ]]; then
    check_tool valgrind || ENABLE_VALGRIND=false
fi

if [[ "$ENABLE_COVERAGE" == true ]]; then
    check_tool gcov || check_tool llvm-cov || {
        log_warning "No coverage tool found, disabling coverage"
        ENABLE_COVERAGE=false
    }
fi

# Prepare CTest arguments
CTEST_ARGS=()
CTEST_ARGS+=("--parallel" "$PARALLEL_JOBS")

if [[ "$VERBOSE_OUTPUT" == true ]]; then
    CTEST_ARGS+=("--verbose")
else
    CTEST_ARGS+=("--output-on-failure")
fi

if [[ "$XML_OUTPUT" == true ]]; then
    CTEST_ARGS+=("-T" "Test")
fi

# Add test filters
if [[ "$RUN_UNIT_TESTS" == true && "$RUN_INTEGRATION_TESTS" == false ]]; then
    CTEST_ARGS+=("-L" "unit")
elif [[ "$RUN_UNIT_TESTS" == false && "$RUN_INTEGRATION_TESTS" == true ]]; then
    CTEST_ARGS+=("-L" "integration")
fi

if [[ "$TEST_PATTERN" != "*" ]]; then
    CTEST_ARGS+=("-R" "$TEST_PATTERN")
fi

# Setup environment for sanitizers
if [[ "$ENABLE_SANITIZERS" == true ]]; then
    export ASAN_OPTIONS="detect_leaks=1:abort_on_error=1:check_initialization_order=1"
    export UBSAN_OPTIONS="print_stacktrace=1:abort_on_error=1"
    log_info "Enabled AddressSanitizer and UndefinedBehaviorSanitizer"
fi

# Setup Valgrind if requested
if [[ "$ENABLE_VALGRIND" == true ]]; then
    VALGRIND_ARGS=(
        "--tool=memcheck"
        "--leak-check=full"
        "--show-leak-kinds=all"
        "--track-origins=yes"
        "--error-exitcode=1"
        "--suppressions=$PROJECT_ROOT/scripts/valgrind.supp"
    )
    
    # Create suppressions file if it doesn't exist
    if [[ ! -f "$PROJECT_ROOT/scripts/valgrind.supp" ]]; then
        cat > "$PROJECT_ROOT/scripts/valgrind.supp" << 'EOF'
# Valgrind suppressions for OptiWeave
{
   glibc_dlopen
   Memcheck:Leak
   ...
   fun:_dl_open
}
{
   llvm_globals
   Memcheck:Leak
   ...
   fun:*LLVM*
}
EOF
        log_info "Created Valgrind suppressions file"
    fi
    
    export CTEST_MEMORYCHECK_COMMAND="valgrind"
    export CTEST_MEMORYCHECK_COMMAND_OPTIONS="${VALGRIND_ARGS[*]}"
    CTEST_ARGS+=("-T" "MemCheck")
fi

# Run tests
log_info "Running tests..."
start_time=$(date +%s)

if ctest "${CTEST_ARGS[@]}"; then
    test_result=0
    log_success "All tests passed!"
else
    test_result=$?
    log_error "Some tests failed (exit code: $test_result)"
fi

end_time=$(date +%s)
duration=$((end_time - start_time))

log_info "Test execution completed in ${duration} seconds"

# Generate coverage report if requested
if [[ "$ENABLE_COVERAGE" == true && $test_result -eq 0 ]]; then
    log_info "Generating coverage report..."
    
    if command -v gcov &> /dev/null; then
        # GCC coverage
        gcov_files=$(find . -name "*.gcda" -o -name "*.gcno")
        if [[ -n "$gcov_files" ]]; then
            mkdir -p coverage
            gcov $gcov_files
            
            if command -v lcov &> /dev/null; then
                lcov --capture --directory . --output-file coverage/coverage.info
                lcov --remove coverage/coverage.info '/usr/*' --output-file coverage/coverage.info
                lcov --remove coverage/coverage.info '*/tests/*' --output-file coverage/coverage.info
                
                if command -v genhtml &> /dev/null; then
                    genhtml coverage/coverage.info --output-directory coverage/html
                    log_success "HTML coverage report generated in coverage/html/"
                fi
            fi
        else
            log_warning "No coverage data found"
        fi
    elif command -v llvm-cov &> /dev/null; then
        # Clang coverage
        log_info "Using llvm-cov for coverage reporting"
        # Implementation depends on build setup
    fi
fi

# Run benchmarks if requested
if [[ "$RUN_BENCHMARKS" == true ]]; then
    log_info "Running performance benchmarks..."
    
    # Look for benchmark executables
    benchmark_tests=$(find . -name "*benchmark*" -executable -type f)
    
    if [[ -n "$benchmark_tests" ]]; then
        for benchmark in $benchmark_tests; do
            log_info "Running benchmark: $(basename "$benchmark")"
            "$benchmark" --benchmark_format=json > "$(basename "$benchmark").json" || true
        done
        log_success "Benchmark results saved to JSON files"
    else
        log_warning "No benchmark executables found"
    fi
fi

# Memory check summary
if [[ "$ENABLE_MEMCHECK" == true || "$ENABLE_VALGRIND" == true ]]; then
    if [[ -f "Testing/Temporary/MemoryChecker.*.log" ]]; then
        log_info "Memory check results:"
        grep -h "ERROR SUMMARY" Testing/Temporary/MemoryChecker.*.log || true
    fi
fi

# Generate test summary
log_info "Test Summary:"
log_info "  Total duration: ${duration} seconds"
log_info "  Parallel jobs: $PARALLEL_JOBS"

if [[ -f "Testing/Temporary/LastTest.log" ]]; then
    total_tests=$(grep -c "Test.*:" Testing/Temporary/LastTest.log || echo "Unknown")
    passed_tests=$(grep -c "Passed" Testing/Temporary/LastTest.log || echo "Unknown")
    failed_tests=$(grep -c "Failed" Testing/Temporary/LastTest.log || echo "0")
    
    log_info "  Total tests: $total_tests"
    log_info "  Passed: $passed_tests"
    log_info "  Failed: $failed_tests"
fi

# Clean up temporary files
if [[ "$ENABLE_COVERAGE" == false ]]; then
    find . -name "*.gcda" -delete 2>/dev/null || true
fi

# Final status
if [[ $test_result -eq 0 ]]; then
    log_success "Test execution completed successfully!"
    
    # Print useful paths
    echo
    log_info "Useful paths:"
    if [[ "$XML_OUTPUT" == true ]]; then
        log_info "  XML test results: $BUILD_DIR/Testing/Temporary/"
    fi
    if [[ "$ENABLE_COVERAGE" == true && -d "coverage/html" ]]; then
        log_info "  Coverage report: $BUILD_DIR/coverage/html/index.html"
    fi
    if [[ "$ENABLE_VALGRIND" == true ]]; then
        log_info "  Valgrind logs: $BUILD_DIR/Testing/Temporary/MemoryChecker.*.log"
    fi
else
    log_error "Test execution failed!"
    log_info "Check the test output above for details"
    
    if [[ -f "Testing/Temporary/LastTestsFailed.log" ]]; then
        log_error "Failed tests:"
        cat "Testing/Temporary/LastTestsFailed.log"
    fi
fi

exit $test_result
