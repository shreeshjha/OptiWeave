#!/usr/bin/env bash

# OptiWeave Build Script
# Usage: ./scripts/build.sh [options]

set -e    # Exit on any error

#### Configuration defaults
BUILD_TYPE="${BUILD_TYPE:-Release}"
BUILD_DIR="${BUILD_DIR:-build}"
INSTALL_PREFIX="${INSTALL_PREFIX:-/usr/local}"
PARALLEL_JOBS="${PARALLEL_JOBS:-$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)}"
ENABLE_TESTS="${ENABLE_TESTS:-OFF}"
ENABLE_EXAMPLES="${ENABLE_EXAMPLES:-ON}"
ENABLE_DOCS="${ENABLE_DOCS:-OFF}"

#### Colorized logging
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; BLUE='\033[0;34m'; NC='\033[0m'
log_info()    { echo -e "${BLUE}[INFO]${NC}    $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
log_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
log_error()   { echo -e "${RED}[ERROR]${NC}    $1"; }

#### Help text
show_help() {
  cat <<EOF
OptiWeave Build Script

Usage: $0 [OPTIONS]

OPTIONS:
  -h, --help            Show this help
  -c, --clean           Clean build directory first
  -t, --build-type T    Build type (Debug, Release, RelWithDebInfo, MinSizeRel)
  -j, --jobs N          Parallel jobs
  -p, --prefix PATH     Install prefix
  --tests               Enable building tests (disabled by default)
  --no-examples         Disable building examples
  --docs              Enable building documentation
  --install             Install after building
  --package             Generate distribution package

ENV VARS:
  BUILD_TYPE, BUILD_DIR, INSTALL_PREFIX, PARALLEL_JOBS, ENABLE_TESTS,
  ENABLE_EXAMPLES, ENABLE_DOCS, CC, CXX, LLVM_DIR (for non-macOS/specific LLVM installs)

Requirements:
  - LLVM 13.x – 17.x (or compatible Apple Clang)
  - CMake ≥ 3.20
  - A C++20-capable compiler
EOF
}

#### Parse args
CLEAN_BUILD=false
INSTALL_AFTER_BUILD=false
CREATE_PACKAGE=false

while [[ $# -gt 0 ]]; do
  case $1 in
    -h|--help)       show_help; exit 0;;
    -c|--clean)      CLEAN_BUILD=true; shift;;
    -t|--build-type) BUILD_TYPE="$2"; shift 2;;
    -j|--jobs)       PARALLEL_JOBS="$2"; shift 2;;
    -p|--prefix)     INSTALL_PREFIX="$2"; shift 2;;
    --tests)         ENABLE_TESTS="ON"; shift;;
    --no-examples)   ENABLE_EXAMPLES="OFF"; shift;;
    --docs)          ENABLE_DOCS="ON"; shift;;
    --install)       INSTALL_AFTER_BUILD=true; shift;;
    --package)       CREATE_PACKAGE=true; shift;;
    *)               log_error "Unknown option: $1"; show_help; exit 1;;
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

log_info "Project root:      $PROJECT_ROOT"
log_info "Build type:        $BUILD_TYPE"
log_info "Build directory:   $BUILD_DIR"
log_info "Install prefix:    $INSTALL_PREFIX"
log_info "Parallel jobs:     $PARALLEL_JOBS"
log_info "Build tests:       $ENABLE_TESTS"
log_info "Build examples:    $ENABLE_EXAMPLES"
log_info "Build docs:        $ENABLE_DOCS"

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

#### Select compiler - Prioritize Apple Clang, then Homebrew, then generic.
if [[ -z "$CXX" ]]; then
  if [[ "$(uname)" == "Darwin" ]]; then
    export CXX="/usr/bin/clang++"
    export CC="/usr/bin/clang"
    log_info "Using Apple Clang from /usr/bin"
    # We will pass LLVM_DIR to CMake directly for Homebrew LLVM
  elif command -v clang++ &>/dev/null; then
    export CXX=clang++; export CC=clang; log_info "Using generic Clang from PATH"
  elif command -v g++ &>/dev/null; then
    export CXX=g++;    export CC=gcc;    log_info "Using GCC from PATH"
  else
    log_error "No C++ compiler found"; exit 1;
  fi
else
  log_info "Using explicitly set compiler: $CXX"
fi

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
  -DBUILD_EXAMPLES="$ENABLE_EXAMPLES"
  -DBUILD_DOCUMENTATION="$ENABLE_DOCS"
  -DCMAKE_C_COMPILER="$CC"
  -DCMAKE_CXX_COMPILER="$CXX"
)

# IMPORTANT: Set LLVM_DIR to the Homebrew LLVM installation's cmake directory
# Assuming llvm@17 is installed at /opt/homebrew/opt/llvm@17
# You might need to adjust this path if your Homebrew prefix is different or you installed a different llvm version.
LLVM_HOMEBREW_PATH="/opt/homebrew/opt/llvm@17" # <--- VERIFY THIS PATH
if [[ -d "${LLVM_HOMEBREW_PATH}/lib/cmake/llvm" ]]; then
    CMAKE_ARGS+=( -DLLVM_DIR="${LLVM_HOMEBREW_PATH}/lib/cmake/llvm" )
    log_info "Setting LLVM_DIR for CMake to: ${LLVM_HOMEBREW_PATH}/lib/cmake/llvm"
else
    log_warning "Homebrew LLVM cmake directory not found at ${LLVM_HOMEBREW_PATH}/lib/cmake/llvm. CMake may struggle to find LLVM."
fi
# Also hint Clang_DIR if necessary, though LLVM_DIR usually covers it.
if [[ -d "${LLVM_HOMEBREW_PATH}/lib/cmake/clang" ]]; then
    CMAKE_ARGS+=( -DClang_DIR="${LLVM_HOMEBREW_PATH}/lib/cmake/clang" )
    log_info "Setting Clang_DIR for CMake to: ${LLVM_HOMEBREW_PATH}/lib/cmake/clang"
fi

# Run CMake
if cmake "${CMAKE_ARGS[@]}" "$PROJECT_ROOT"; then
    log_success "Configured successfully"
else
    log_error "CMake configuration failed"
    log_info "Check that you have a C++20 compatible compiler and (for Linux) an LLVM development package installed."
    log_info "On macOS, ensure Xcode Command Line Tools are installed: xcode-select --install"
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