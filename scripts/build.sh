#!/bin/bash

# OptiWeave Build Script
# Usage: ./scripts/build.sh [options]

set -e  # Exit on any error

# Configuration
BUILD_TYPE="${BUILD_TYPE:-Release}"
BUILD_DIR="${BUILD_DIR:-build}"
INSTALL_PREFIX="${INSTALL_PREFIX:-/usr/local}"
PARALLEL_JOBS="${PARALLEL_JOBS:-$(nproc)}"
ENABLE_TESTS="${ENABLE_TESTS:-ON}"
ENABLE_EXAMPLES="${ENABLE_EXAMPLES:-ON}"
ENABLE_DOCS="${ENABLE_DOCS:-OFF}"

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
OptiWeave Build Script

Usage: $0 [OPTIONS]

OPTIONS:
    -h, --help              Show this help message
    -c, --clean             Clean build directory before building
    -t, --build-type TYPE   Build type (Debug, Release, RelWithDebInfo, MinSizeRel)
    -j, --jobs NUM          Number of parallel build jobs
    -p, --prefix PATH       Installation prefix
    --no-tests              Disable building tests
    --no-examples           Disable building examples
    --docs                  Enable building documentation
    --install               Install after building
    --package               Create distribution package

ENVIRONMENT VARIABLES:
    BUILD_TYPE              Build configuration (default: Release)
    BUILD_DIR               Build directory (default: build)
    INSTALL_PREFIX          Installation prefix (default: /usr/local)
    PARALLEL_JOBS           Number of parallel jobs (default: nproc)
    CC                      C compiler
    CXX                     C++ compiler
    LLVM_DIR                Path to LLVM installation

EXAMPLES:
    $0                      # Basic release build
    $0 -c -t Debug         # Clean debug build
    $0 --install           # Build and install
    BUILD_TYPE=Debug $0    # Debug build using environment variable

EOF
}

# Parse command line arguments
CLEAN_BUILD=false
INSTALL_AFTER_BUILD=false
CREATE_PACKAGE=false

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -c|--clean)
            CLEAN_BUILD=true
            shift
            ;;
        -t|--build-type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        -j|--jobs)
            PARALLEL_JOBS="$2"
            shift 2
            ;;
        -p|--prefix)
            INSTALL_PREFIX="$2"
            shift 2
            ;;
        --no-tests)
            ENABLE_TESTS="OFF"
            shift
            ;;
        --no-examples)
            ENABLE_EXAMPLES="OFF"
            shift
            ;;
        --docs)
            ENABLE_DOCS="ON"
            shift
            ;;
        --install)
            INSTALL_AFTER_BUILD=true
            shift
            ;;
        --package)
            CREATE_PACKAGE=true
            shift
            ;;
        *)
            log_error "Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done

# Validate build type
case $BUILD_TYPE in
    Debug|Release|RelWithDebInfo|MinSizeRel)
        ;;
    *)
        log_error "Invalid build type: $BUILD_TYPE"
        log_info "Valid types: Debug, Release, RelWithDebInfo, MinSizeRel"
        exit 1
        ;;
esac

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

log_info "OptiWeave Build Configuration:"
log_info "  Project root: $PROJECT_ROOT"
log_info "  Build type: $BUILD_TYPE"
log_info "  Build directory: $BUILD_DIR"
log_info "  Install prefix: $INSTALL_PREFIX"
log_info "  Parallel jobs: $PARALLEL_JOBS"
log_info "  Tests: $ENABLE_TESTS"
log_info "  Examples: $ENABLE_EXAMPLES"
log_info "  Documentation: $ENABLE_DOCS"

# Change to project root
cd "$PROJECT_ROOT"

# Check for required tools
check_dependency() {
    if ! command -v "$1" &> /dev/null; then
        log_error "$1 is required but not found in PATH"
        return 1
    fi
}

log_info "Checking dependencies..."
check_dependency cmake
check_dependency make

# Check for C++ compiler
if [[ -z "$CXX" ]]; then
    if command -v clang++ &> /dev/null; then
        export CXX=clang++
        export CC=clang
        log_info "Using Clang compiler"
    elif command -v g++ &> /dev/null; then
        export CXX=g++
        export CC=gcc
        log_info "Using GCC compiler"
    else
        log_error "No suitable C++ compiler found"
        exit 1
    fi
else
    log_info "Using compiler: $CXX"
fi

# Check for LLVM
if [[ -z "$LLVM_DIR" ]]; then
    if command -v llvm-config &> /dev/null; then
        LLVM_VERSION=$(llvm-config --version | cut -d. -f1)
        if [[ $LLVM_VERSION -lt 13 ]]; then
            log_warning "LLVM version $LLVM_VERSION detected, recommend 13+"
        fi
        log_info "Found LLVM version: $(llvm-config --version)"
    else
        log_error "LLVM not found. Please install LLVM development packages"
        log_info "  Ubuntu/Debian: sudo apt install llvm-dev clang-dev"
        log_info "  macOS: brew install llvm"
        log_info "  Or set LLVM_DIR environment variable"
        exit 1
    fi
fi

# Clean build directory if requested
if [[ "$CLEAN_BUILD" == true ]]; then
    log_info "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
log_info "Configuring with CMake..."

CMAKE_ARGS=(
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
    -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX"
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    -DENABLE_TESTING="$ENABLE_TESTS"
    -DBUILD_EXAMPLES="$ENABLE_EXAMPLES"
    -DBUILD_DOCUMENTATION="$ENABLE_DOCS"
)

# Add LLVM_DIR if set
if [[ -n "$LLVM_DIR" ]]; then
    CMAKE_ARGS+=(-DLLVM_DIR="$LLVM_DIR")
fi

# Add Clang_DIR if available
if [[ -n "$Clang_DIR" ]]; then
    CMAKE_ARGS+=(-DClang_DIR="$Clang_DIR")
fi

# Run CMake configuration
if ! cmake "${CMAKE_ARGS[@]}" "$PROJECT_ROOT"; then
    log_error "CMake configuration failed"
    exit 1
fi

log_success "Configuration completed"

# Build
log_info "Building with $PARALLEL_JOBS parallel jobs..."
if ! cmake --build . --parallel "$PARALLEL_JOBS"; then
    log_error "Build failed"
    exit 1
fi

log_success "Build completed"

# Run tests if enabled
if [[ "$ENABLE_TESTS" == "ON" ]]; then
    log_info "Running tests..."
    if ! ctest --parallel "$PARALLEL_JOBS" --output-on-failure; then
        log_warning "Some tests failed"
    else
        log_success "All tests passed"
    fi
fi

# Install if requested
if [[ "$INSTALL_AFTER_BUILD" == true ]]; then
    log_info "Installing to $INSTALL_PREFIX..."
    if ! cmake --install .; then
        log_error "Installation failed"
        exit 1
    fi
    log_success "Installation completed"
fi

# Create package if requested
if [[ "$CREATE_PACKAGE" == true ]]; then
    log_info "Creating distribution package..."
    if ! cpack; then
        log_error "Package creation failed"
        exit 1
    fi
    log_success "Package created"
fi

log_success "Build process completed successfully!"

# Print usage information
cat << EOF

Build completed! You can now:

1. Run OptiWeave directly:
   ./$BUILD_DIR/optiweave --help

2. Install system-wide:
   sudo cmake --install $BUILD_DIR

3. Run tests:
   cd $BUILD_DIR && ctest

4. View build artifacts:
   ls -la $BUILD_DIR/

EOF
