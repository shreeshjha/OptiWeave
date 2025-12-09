#!/bin/bash
# Setup script for Polybench/C benchmarks

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BENCH_DIR="${SCRIPT_DIR}/../benchmarks/polybench"

echo "Setting up Polybench/C for OptiWeave evaluation..."

cd "$BENCH_DIR"

# Download Polybench/C
if [ ! -d "PolyBenchC-4.2.1" ]; then
    echo "Downloading Polybench/C..."
    git clone https://github.com/MatthiasJReisinger/PolyBenchC-4.2.1.git
fi

cd PolyBenchC-4.2.1

# Select 10 representative kernels (subset for 4-week timeline)
echo "Selected kernels for evaluation:"
echo "  1. linear-algebra/blas/gemm (matrix multiply)"
echo "  2. linear-algebra/blas/gemver (matrix-vector)"
echo "  3. linear-algebra/solvers/cholesky"
echo "  4. stencils/jacobi-2d"
echo "  5. medley/nussinov (dynamic programming)"
echo "  6. linear-algebra/kernels/2mm (2 matrix multiplies)"
echo "  7. datamining/correlation"
echo "  8. stencils/heat-3d"
echo "  9. linear-algebra/kernels/atax"
echo " 10. medley/floyd-warshall"

echo "Polybench/C setup complete!"
echo "Location: $BENCH_DIR/PolyBenchC-4.2.1"
