#!/bin/bash
# Setup script for RQ3 case study (json-c)

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
CASE_STUDY_DIR="${SCRIPT_DIR}/../benchmarks/case-studies"

echo "Setting up RQ3 Case Study: json-c"
echo "=================================="
echo ""

cd "$CASE_STUDY_DIR"

# Clone json-c
if [ ! -d "json-c" ]; then
    echo "Cloning json-c repository..."
    git clone https://github.com/json-c/json-c.git
    cd json-c
    git checkout json-c-0.17-20230812  # Stable release
else
    echo "json-c already cloned"
    cd json-c
fi

# Build json-c
echo "Building json-c..."
if [ ! -d "build" ]; then
    mkdir build
    cd build
    cmake ..
    make
else
    echo "json-c already built"
fi

echo ""
echo "✓ json-c setup complete!"
echo ""
echo "Project info:"
echo "  Location: $CASE_STUDY_DIR/json-c"
echo "  Size: ~15K LOC"
echo "  Type: JSON parsing library (C)"
echo ""
echo "Next steps for RQ3 evaluation:"
echo "  1. Run OptiWeave static analysis on json-c"
echo "  2. Instrument with runtime profiling"
echo "  3. Document findings (bugs, hotspots, optimizations)"
echo "  4. Run: evaluation/scripts/run_rq3_case_study.sh"
