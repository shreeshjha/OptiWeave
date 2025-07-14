#!/usr/bin/env bash

# OptiWeave Build Script
# Usage: ./scripts/build.sh [options]

set -e  # Exit on any error

#### Configuration defaults
BUILD_TYPE="${BUILD_TYPE:-Release}"
BUILD_DIR="${BUILD_DIR:-build}"
INSTALL_PREFIX="${INSTALL_PREFIX:-/usr/local}"
PARALLEL_JOBS="${PARALLEL_JOBS:-$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)}"
ENABLE_TESTS="${ENABLE_TESTS:-OFF}"
ENABLE_EXAMPLES="${ENABLE_EXAMPLES:-ON}"
ENABLE_DOCS="${ENABLE_DOCS:-OFF}"

LLVM_MIN_VERSION="13"
LLVM_MAX_VERSION="17"

#### Colorized logging
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; BLUE='\033[0;34m'; NC='\033[0m'
log_info()    { echo -e "${BLUE}[INFO]${NC}    $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
log_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
log_error()   { echo -e "${RED}[ERROR]${NC}   $1"; }

#### Help text
show_help() {
  cat <<EOF
OptiWeave Build Script

Usage: $0 [OPTIONS]

OPTIONS:
  -h, --help           Show this help
  -c, --clean          Clean build directory first
  -t, --build-type T   Build type (Debug, Release, RelWithDebInfo, MinSizeRel)
  -j, --jobs N         Parallel jobs
  -p, --prefix PATH    Install prefix
  --tests              Enable building tests (disabled by default)
  --no-examples        Disable building examples
  --docs               Enable building documentation
  --install            Install after building
  --package            Generate distribution package

ENV VARS:
  BUILD_TYPE, BUILD_DIR, INSTALL_PREFIX, PARALLEL_JOBS, ENABLE_TESTS,
  ENABLE_EXAMPLES, ENABLE_DOCS, CC, CXX, LLVM_DIR

Requirements:
  - LLVM ${LLVM_MIN_VERSION}.x – ${LLVM_MAX_VERSION}.x
  - CMake ≥ 3.20
  - A C++20‐capable compiler
EOF
}

#### Parse args
CLEAN_BUILD=false
INSTALL_AFTER_BUILD=false
CREATE_PACKAGE=false

while [[ $# -gt 0 ]]; do
  case $1 in
    -h|--help)      show_help; exit 0;;
    -c|--clean)     CLEAN_BUILD=true; shift;;
    -t|--build-type) BUILD_TYPE="$2"; shift 2;;
    -j|--jobs)      PARALLEL_JOBS="$2"; shift 2;;
    -p|--prefix)    INSTALL_PREFIX="$2"; shift 2;;
    --tests)        ENABLE_TESTS="ON"; shift;;
    --no-examples)  ENABLE_EXAMPLES="OFF"; shift;;
    --docs)         ENABLE_DOCS="ON"; shift;;
    --install)      INSTALL_AFTER_BUILD=true; shift;;
    --package)      CREATE_PACKAGE=true; shift;;
    *)              log_error "Unknown option: $1"; show_help; exit 1;;
  esac
done

#### Validate build type
case $BUILD_TYPE in
  Debug|Release|RelWithDebInfo|MinSizeRel) ;;
  *) log_error "Invalid build type: $BUILD_TYPE"; exit 1;;
esac

#### Determine project root
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &>/dev/null && pwd )"
PROJECT_ROOT="$( dirname "$SCRIPT_DIR" )"

log_info "Project root:     $PROJECT_ROOT"
log_info "Build type:       $BUILD_TYPE"
log_info "Build directory:  $BUILD_DIR"
log_info "Install prefix:   $INSTALL_PREFIX"
log_info "Parallel jobs:    $PARALLEL_JOBS"
log_info "Build tests:      $ENABLE_TESTS"
log_info "Build examples:   $ENABLE_EXAMPLES"
log_info "Build docs:       $ENABLE_DOCS"

cd "$PROJECT_ROOT"

#### Check dependencies
command -v cmake &>/dev/null || { log_error "cmake not found"; exit 1; }
# Check for make or ninja
if command -v ninja &>/dev/null; then
    GENERATOR="Ninja"
    BUILD_TOOL="ninja"
elif command -v make &>/dev/null; then
    GENERATOR="Unix Makefiles"
    BUILD_TOOL="make"
else
    log_error "Neither make nor ninja found"; exit 1;
fi

#### Select compiler
if [[ -z "$CXX" ]]; then
  if command -v clang++ &>/dev/null; then
    export CXX=clang++; export CC=clang; log_info "Using Clang"
  elif command -v g++ &>/dev/null; then
    export CXX=g++;    export CC=gcc;   log_info "Using GCC"
  else
    log_error "No C++ compiler found"; exit 1;
  fi
else
  log_info "Using compiler:   $CXX"
fi

#### Check LLVM version
check_llvm_version() {
    if [[ -n "$LLVM_DIR" ]]; then
        log_info "Using LLVM_DIR: $LLVM_DIR"
        return 0
    fi

    if command -v llvm-config &>/dev/null; then
        LV=$(llvm-config --version | cut -d. -f1)
        FULL=$(llvm-config --version)
        
        if (( LV < LLVM_MIN_VERSION )); then
            log_error "LLVM $FULL is too old (need ${LLVM_MIN_VERSION}.x - ${LLVM_MAX_VERSION}.x)"
            log_info "Install instructions:"
            log_info "  macOS: brew install llvm@${LLVM_MIN_VERSION}"
            log_info "  Ubuntu: sudo apt install llvm-${LLVM_MIN_VERSION}-dev clang-${LLVM_MIN_VERSION}-dev"
            exit 1
        elif (( LV > LLVM_MAX_VERSION )); then
            log_error "LLVM $FULL is too new (need ${LLVM_MIN_VERSION}.x - ${LLVM_MAX_VERSION}.x)"
            log_info "Install compatible version:"
            log_info "  macOS: brew install llvm@${LLVM_MAX_VERSION}"
            log_info "  Ubuntu: sudo apt install llvm-${LLVM_MAX_VERSION}-dev clang-${LLVM_MAX_VERSION}-dev"
            exit 1
        fi
        
        log_success "Found LLVM $FULL"
    else
        log_error "llvm-config not found. Please install LLVM ${LLVM_MIN_VERSION}.x - ${LLVM_MAX_VERSION}.x"
        log_info "  macOS: brew install llvm@${LLVM_MIN_VERSION}"
        log_info "  Ubuntu: sudo apt install llvm-${LLVM_MIN_VERSION}-dev clang-${LLVM_MIN_VERSION}-dev"
        exit 1
    fi
}

check_llvm_version

#### Clean?
if $CLEAN_BUILD; then
  log_info "Cleaning build dir..."
  rm -rf "$BUILD_DIR"
fi

#### Configure
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

log_info "Configuring with CMake..."

CMAKE_ARGS=(
  -G "$GENERATOR"
  -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
  -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX"
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
  -DBUILD_TESTING="$ENABLE_TESTS"
  -DENABLE_TESTING="$ENABLE_TESTS"
  -DBUILD_EXAMPLES="$ENABLE_EXAMPLES"
  -DBUILD_DOCUMENTATION="$ENABLE_DOCS"
)

# Add LLVM_DIR if set
[[ -n "$LLVM_DIR"  ]] && CMAKE_ARGS+=( -DLLVM_DIR="$LLVM_DIR" )
[[ -n "$Clang_DIR" ]] && CMAKE_ARGS+=( -DClang_DIR="$Clang_DIR" )

# Run CMake
if cmake "${CMAKE_ARGS[@]}" "$PROJECT_ROOT"; then
    log_success "Configured successfully"
else
    log_error "CMake configuration failed"
    log_info "Check that you have compatible LLVM/Clang versions installed"
    exit 1
fi

#### Build
log_info "Building (jobs=$PARALLEL_JOBS)..."

if [[ "$GENERATOR" == "Ninja" ]]; then
    ninja -j "$PARALLEL_JOBS"
else
    make -j "$PARALLEL_JOBS"
fi

if [[ $? -eq 0 ]]; then
    log_success "Built successfully"
else
    log_error "Build failed"
    exit 1
fi

#### Tests (only if explicitly enabled)
if [[ "$ENABLE_TESTS" == "ON" ]]; then
  log_info "Running tests..."
  if ctest --parallel "$PARALLEL_JOBS" --output-on-failure; then
    log_success "All tests passed"
  else
    log_warning "Some tests failed"
  fi
fi

#### Install
if $INSTALL_AFTER_BUILD; then
  log_info "Installing..."
  if cmake --install .; then
    log_success "Installed to $INSTALL_PREFIX"
  else
    log_error "Install failed"; exit 1;
  fi
fi

#### Package
if $CREATE_PACKAGE; then
  log_info "Packaging..."
  if cpack; then
    log_success "Package created"
  else
    log_error "Package failed"; exit 1;
  fi
fi

log_success "All done!"

# Show useful info
echo
log_info "Build artifacts:"
log_info "  OptiWeave executable: $BUILD_DIR/optiweave"
if [[ "$ENABLE_EXAMPLES" == "ON" ]]; then
    log_info "  Examples: $BUILD_DIR/examples/"
fi
log_info "  Libraries: $BUILD_DIR/liboptiweave_core.a"

echo
log_info "Next steps:"
log_info "  Test the build: $BUILD_DIR/optiweave --help"
log_info "  Install system-wide: sudo cmake --install $BUILD_DIR"