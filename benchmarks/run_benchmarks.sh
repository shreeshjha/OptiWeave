#!/usr/bin/env bash
#
# OptiWeave Benchmark Suite
# Runs OptiWeave, clang-tidy, and cppcheck on 6 open-source C projects
# and collects metrics for thesis Chapter 5.
#
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR/projects"
RESULTS_DIR="$SCRIPT_DIR/results"
OPTIWEAVE="$SCRIPT_DIR/../build/optiweave"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RUN_DIR="$RESULTS_DIR/$TIMESTAMP"

# Tools
CLANG_TIDY=$(which clang-tidy 2>/dev/null || echo "")
CPPCHECK=$(which cppcheck 2>/dev/null || echo "")
VALGRIND=$(which valgrind 2>/dev/null || echo "")

# Benchmark projects and their main source files
declare -A PROJECTS
PROJECTS[tinyexpr]="tinyexpr.c"
PROJECTS[cJSON]="cJSON.c cJSON_Utils.c"
PROJECTS[hashmap_c]="hashmap.c"  # directory is hashmap.c
PROJECTS[lz4]="lib/lz4.c lib/lz4hc.c lib/lz4frame.c lib/xxhash.c"
PROJECTS[xxHash]="xxhash.c"
PROJECTS[lua]="*.c"

# Map project keys to directory names
declare -A PROJECT_DIRS
PROJECT_DIRS[tinyexpr]="tinyexpr"
PROJECT_DIRS[cJSON]="cJSON"
PROJECT_DIRS[hashmap_c]="hashmap.c"
PROJECT_DIRS[lz4]="lz4"
PROJECT_DIRS[xxHash]="xxHash"
PROJECT_DIRS[lua]="lua"

mkdir -p "$RUN_DIR"

log() { echo "[$(date +%H:%M:%S)] $*"; }
log_json() { echo "$*" >> "$RUN_DIR/metrics.jsonl"; }

# ============================================================
#  Helper: count lines of C code
# ============================================================
count_loc() {
    local dir="$1"
    find "$dir" -maxdepth 3 -name "*.c" -exec cat {} + 2>/dev/null | wc -l
}

count_c_files() {
    local dir="$1"
    find "$dir" -maxdepth 3 -name "*.c" | wc -l
}

# ============================================================
#  RQ2: Run OptiWeave analysis on each project
# ============================================================
run_optiweave() {
    local project="$1"
    local proj_dir="$PROJECT_DIR/${PROJECT_DIRS[$project]}"
    local out_dir="$RUN_DIR/$project/optiweave"
    mkdir -p "$out_dir"

    log "  OptiWeave on $project..."

    # Collect all .c files (exclude test files)
    local c_files=()
    while IFS= read -r f; do
        c_files+=("$f")
    done < <(find "$proj_dir" -maxdepth 2 -name "*.c" \
        -not -name "onelua.c" \
        -not -path "*/.git/*")

    if [ ${#c_files[@]} -eq 0 ]; then
        log "    No C files found for $project"
        echo '{"project":"'"$project"'","tool":"optiweave","error":"no_c_files"}' >> "$RUN_DIR/metrics.jsonl"
        return
    fi

    # Run OptiWeave with all operator types enabled
    local start_time=$(date +%s%N)
    local ow_output
    ow_output=$("$OPTIWEAVE" \
        --array-subscripts \
        --arithmetic-ops \
        --comparison-ops \
        --assignment-ops \
        --dry-run \
        --verbose \
        --print-stats \
        "${c_files[@]}" \
        -- -std=c11 -w 2>&1) || true
    local end_time=$(date +%s%N)
    local elapsed_ms=$(( (end_time - start_time) / 1000000 ))

    echo "$ow_output" > "$out_dir/output.txt"

    # Parse transformation stats (sum across all files)
    local array_subs arith_ops templates_skipped errors total_transforms
    array_subs=$(echo "$ow_output" | grep -oP 'Array subscripts transformed: \K[0-9]+' | awk '{s+=$1} END {print s+0}')
    arith_ops=$(echo "$ow_output" | grep -oP 'Arithmetic operators transformed: \K[0-9]+' | awk '{s+=$1} END {print s+0}')
    templates_skipped=$(echo "$ow_output" | grep -oP 'Template instantiations skipped: \K[0-9]+' | awk '{s+=$1} END {print s+0}')
    errors=$(echo "$ow_output" | grep -oP 'Errors encountered: \K[0-9]+' | awk '{s+=$1} END {print s+0}')
    total_transforms=$((array_subs + arith_ops))

    local loc=$(count_loc "$proj_dir")
    local file_count=$(count_c_files "$proj_dir")

    log_json '{"project":"'"$project"'","tool":"optiweave","time_ms":'"$elapsed_ms"',"array_subscripts":'"$array_subs"',"arithmetic_ops":'"$arith_ops"',"total_transforms":'"$total_transforms"',"templates_skipped":'"$templates_skipped"',"errors":'"$errors"',"loc":'"$loc"',"c_files":'"$file_count"'}'

    log "    Done: ${total_transforms} transforms, ${elapsed_ms}ms, ${errors} errors"
}

# ============================================================
#  RQ2: Run clang-tidy on each project
# ============================================================
run_clang_tidy() {
    local project="$1"
    local proj_dir="$PROJECT_DIR/${PROJECT_DIRS[$project]}"
    local out_dir="$RUN_DIR/$project/clang-tidy"
    mkdir -p "$out_dir"

    if [ -z "$CLANG_TIDY" ]; then
        log "  clang-tidy: not found, skipping"
        return
    fi

    log "  clang-tidy on $project..."

    local c_files=()
    while IFS= read -r f; do
        c_files+=("$f")
    done < <(find "$proj_dir" -maxdepth 2 -name "*.c" \
        -not -name "onelua.c" \
        -not -path "*/.git/*")

    if [ ${#c_files[@]} -eq 0 ]; then return; fi

    # Run clang-tidy with checks relevant to OptiWeave's detection capabilities
    local start_time=$(date +%s%N)
    local ct_output
    ct_output=$($CLANG_TIDY \
        -checks='-*,bugprone-integer-division,bugprone-narrowing-conversions,bugprone-signed-char-misuse,bugprone-suspicious-semicolon,bugprone-too-small-loop-variable,bugprone-undefined-memory-manipulation,cert-dcl03-c,cert-err34-c,cert-flp30-c,cert-int09-c,clang-analyzer-core.*,clang-analyzer-deadcode.*,clang-analyzer-security.*,misc-redundant-expression,readability-misleading-indentation' \
        --quiet \
        "${c_files[@]}" \
        -- -std=c11 -w 2>&1) || true
    local end_time=$(date +%s%N)
    local elapsed_ms=$(( (end_time - start_time) / 1000000 ))

    echo "$ct_output" > "$out_dir/output.txt"

    # Count findings by category
    local warnings errors_ct notes integer_issues null_deref dead_code security
    warnings=$(echo "$ct_output" | grep -c "warning:" 2>/dev/null) || warnings=0
    errors_ct=$(echo "$ct_output" | grep -c "error:" 2>/dev/null) || errors_ct=0
    notes=$(echo "$ct_output" | grep -c "note:" 2>/dev/null) || notes=0

    # Count specific bug categories
    integer_issues=$(echo "$ct_output" | grep -cE "bugprone-integer|bugprone-narrowing|bugprone-signed-char|bugprone-too-small" 2>/dev/null) || integer_issues=0
    null_deref=$(echo "$ct_output" | grep -cE "core.NullDereference|core.NonNullParam" 2>/dev/null) || null_deref=0
    dead_code=$(echo "$ct_output" | grep -c "deadcode\." 2>/dev/null) || dead_code=0
    security=$(echo "$ct_output" | grep -c "security\." 2>/dev/null) || security=0

    log_json '{"project":"'"$project"'","tool":"clang-tidy","time_ms":'"$elapsed_ms"',"warnings":'"$warnings"',"errors":'"$errors_ct"',"notes":'"$notes"',"integer_issues":'"$integer_issues"',"null_deref":'"$null_deref"',"dead_code":'"$dead_code"',"security":'"$security"'}'

    log "    Done: ${warnings} warnings, ${errors_ct} errors, ${elapsed_ms}ms"
}

# ============================================================
#  RQ2: Run cppcheck on each project
# ============================================================
run_cppcheck() {
    local project="$1"
    local proj_dir="$PROJECT_DIR/${PROJECT_DIRS[$project]}"
    local out_dir="$RUN_DIR/$project/cppcheck"
    mkdir -p "$out_dir"

    if [ -z "$CPPCHECK" ]; then
        log "  cppcheck: not found, skipping"
        return
    fi

    log "  cppcheck on $project..."

    # Find only main C files (skip test dirs)
    local cc_files
    cc_files=$(find "$proj_dir" -maxdepth 2 -name "*.c" \
        -not -path "*/.git/*" | tr '\n' ' ')

    local start_time=$(date +%s%N)
    local cc_output
    cc_output=$(timeout 120 $CPPCHECK \
        --enable=warning,style,performance,portability \
        --std=c11 \
        --language=c \
        --suppress=missingInclude \
        --suppress=unusedFunction \
        --force \
        --quiet \
        $cc_files 2>&1) || true
    local end_time=$(date +%s%N)
    local elapsed_ms=$(( (end_time - start_time) / 1000000 ))

    echo "$cc_output" > "$out_dir/output.txt"

    # Count findings
    local errors_cc warnings_cc style perf portability info total
    errors_cc=$(echo "$cc_output" | grep -c "(error)" 2>/dev/null) || errors_cc=0
    warnings_cc=$(echo "$cc_output" | grep -c "(warning)" 2>/dev/null) || warnings_cc=0
    style=$(echo "$cc_output" | grep -c "(style)" 2>/dev/null) || style=0
    perf=$(echo "$cc_output" | grep -c "(performance)" 2>/dev/null) || perf=0
    portability=$(echo "$cc_output" | grep -c "(portability)" 2>/dev/null) || portability=0
    info=$(echo "$cc_output" | grep -c "(information)" 2>/dev/null) || info=0
    total=$(( errors_cc + warnings_cc + style + perf + portability ))

    log_json '{"project":"'"$project"'","tool":"cppcheck","time_ms":'"$elapsed_ms"',"errors":'"$errors_cc"',"warnings":'"$warnings_cc"',"style":'"$style"',"performance":'"$perf"',"portability":'"$portability"',"information":'"$info"',"total_findings":'"$total"'}'

    log "    Done: ${total} findings (${errors_cc}E ${warnings_cc}W ${style}S ${perf}P), ${elapsed_ms}ms"
}

# ============================================================
#  RQ1: Measure instrumentation overhead
# ============================================================
run_overhead_test() {
    local project="$1"
    local proj_dir="$PROJECT_DIR/${PROJECT_DIRS[$project]}"
    local out_dir="$RUN_DIR/$project/overhead"
    mkdir -p "$out_dir"

    log "  Overhead test on $project..."

    # Find main C files (not test/bench)
    local c_files=()
    while IFS= read -r f; do
        c_files+=("$f")
    done < <(find "$proj_dir" -maxdepth 2 -name "*.c" \
        -not -name "onelua.c" \
        -not -path "*/.git/*")

    if [ ${#c_files[@]} -eq 0 ]; then return; fi

    # Measure OptiWeave transformation time (not dry-run)
    local transform_dir="$out_dir/transformed"
    mkdir -p "$transform_dir"

    local start_time=$(date +%s%N)
    "$OPTIWEAVE" \
        --array-subscripts \
        --arithmetic-ops \
        --output-dir="$transform_dir" \
        "${c_files[@]}" \
        -- -std=c11 -w 2>"$out_dir/transform_log.txt" || true
    local end_time=$(date +%s%N)
    local transform_ms=$(( (end_time - start_time) / 1000000 ))

    # Count transformed files and their size
    local orig_size=$(cat "${c_files[@]}" 2>/dev/null | wc -c)
    local transformed_size=$(find "$transform_dir" -name "*.c" -exec cat {} + 2>/dev/null | wc -c)
    local size_increase=0
    if [ "$orig_size" -gt 0 ]; then
        size_increase=$(( (transformed_size - orig_size) * 100 / orig_size ))
    fi

    log_json '{"project":"'"$project"'","tool":"optiweave-overhead","transform_time_ms":'"$transform_ms"',"original_bytes":'"$orig_size"',"transformed_bytes":'"$transformed_size"',"size_increase_pct":'"$size_increase"'}'

    log "    Transform: ${transform_ms}ms, size +${size_increase}%"
}

# ============================================================
#  Main
# ============================================================
log "============================================"
log "  OptiWeave Benchmark Suite"
log "  Results: $RUN_DIR"
log "============================================"
log ""

# Check tools
log "Tools:"
log "  OptiWeave: $OPTIWEAVE"
log "  clang-tidy: ${CLANG_TIDY:-NOT FOUND}"
log "  cppcheck: ${CPPCHECK:-NOT FOUND}"
log "  valgrind: ${VALGRIND:-NOT FOUND}"
log ""

# Check OptiWeave binary
if [ ! -x "$OPTIWEAVE" ]; then
    log "ERROR: OptiWeave binary not found at $OPTIWEAVE"
    log "Run: cd build && cmake .. && make -j\$(nproc)"
    exit 1
fi

# Run benchmarks for each project
for project in tinyexpr cJSON hashmap_c lz4 xxHash lua; do
    log "=== $project ==="
    run_optiweave "$project"
    run_clang_tidy "$project"
    run_cppcheck "$project"
    run_overhead_test "$project"
    log ""
done

# ============================================================
#  Generate summary report
# ============================================================
log "=== Generating Summary ==="

python3 - "$RUN_DIR" << 'PYEOF'
import json, sys, os

run_dir = sys.argv[1]
metrics_file = os.path.join(run_dir, "metrics.jsonl")

if not os.path.exists(metrics_file):
    print("No metrics file found")
    sys.exit(1)

metrics = []
with open(metrics_file) as f:
    for line in f:
        line = line.strip()
        if line:
            metrics.append(json.loads(line))

# Group by project
projects = {}
for m in metrics:
    p = m["project"]
    t = m["tool"]
    if p not in projects:
        projects[p] = {}
    projects[p][t] = m

# Print summary table
print("\n" + "="*80)
print("OPTIWEAVE BENCHMARK RESULTS")
print("="*80)

# RQ2: Detection comparison
print("\n--- RQ2: Bug Detection Comparison ---")
print(f"{'Project':<12} {'LOC':>6} {'OW transforms':>14} {'CT warnings':>12} {'CC findings':>12}")
print("-"*60)
for p in sorted(projects.keys()):
    d = projects[p]
    ow = d.get("optiweave", {})
    ct = d.get("clang-tidy", {})
    cc = d.get("cppcheck", {})
    print(f"{p:<12} {ow.get('loc', 0):>6} {ow.get('total_transforms', 0):>14} {ct.get('warnings', 0):>12} {cc.get('total_findings', 0):>12}")

# RQ1: Overhead
print("\n--- RQ1: Instrumentation Overhead ---")
print(f"{'Project':<12} {'Transform ms':>13} {'Orig bytes':>11} {'New bytes':>10} {'Size +%':>8}")
print("-"*60)
for p in sorted(projects.keys()):
    d = projects[p]
    oh = d.get("optiweave-overhead", {})
    if oh:
        print(f"{p:<12} {oh.get('transform_time_ms', 0):>13} {oh.get('original_bytes', 0):>11} {oh.get('transformed_bytes', 0):>10} {oh.get('size_increase_pct', 0):>7}%")

# Analysis time comparison
print("\n--- Analysis Time Comparison (ms) ---")
print(f"{'Project':<12} {'OptiWeave':>10} {'clang-tidy':>11} {'cppcheck':>10}")
print("-"*45)
for p in sorted(projects.keys()):
    d = projects[p]
    ow_t = d.get("optiweave", {}).get("time_ms", 0)
    ct_t = d.get("clang-tidy", {}).get("time_ms", 0)
    cc_t = d.get("cppcheck", {}).get("time_ms", 0)
    print(f"{p:<12} {ow_t:>10} {ct_t:>11} {cc_t:>10}")

# Save structured JSON
summary = {"timestamp": os.path.basename(run_dir), "projects": projects}
with open(os.path.join(run_dir, "summary.json"), "w") as f:
    json.dump(summary, f, indent=2)

print(f"\nResults saved to: {run_dir}/summary.json")
print("="*80)
PYEOF

log "Benchmarks complete. Results in: $RUN_DIR"
