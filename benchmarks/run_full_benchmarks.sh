#!/usr/bin/env bash
#
# OptiWeave Full Benchmark Suite
# Tests all major features across 6 OSS projects for thesis Chapter 5
#
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR/projects"
OPTIWEAVE="$SCRIPT_DIR/../build/optiweave"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RUN_DIR="$SCRIPT_DIR/results/full_$TIMESTAMP"

CLANG_TIDY=$(which clang-tidy 2>/dev/null || echo "")
CPPCHECK=$(which cppcheck 2>/dev/null || echo "")

declare -A PROJECT_DIRS
PROJECT_DIRS[tinyexpr]="tinyexpr"
PROJECT_DIRS[cJSON]="cJSON"
PROJECT_DIRS[hashmap_c]="hashmap.c"
PROJECT_DIRS[lz4]="lz4"
PROJECT_DIRS[xxHash]="xxHash"
PROJECT_DIRS[lua]="lua"

mkdir -p "$RUN_DIR"

log() { echo "[$(date +%H:%M:%S)] $*" | tee -a "$RUN_DIR/log.txt"; }

get_c_files() {
    local proj_dir="$1"
    find "$proj_dir" -maxdepth 2 -name "*.c" -not -name "onelua.c" -not -path "*/.git/*" | sort
}

# ============================================================
#  RQ2: OptiWeave bug detection (--detect-overflow, --fp-precision, --data-flow)
# ============================================================
run_optiweave_detection() {
    local project="$1"
    local proj_dir="$PROJECT_DIR/${PROJECT_DIRS[$project]}"
    local out_dir="$RUN_DIR/$project"
    mkdir -p "$out_dir"

    local c_files=($(get_c_files "$proj_dir"))
    if [ ${#c_files[@]} -eq 0 ]; then return; fi
    local loc=$(cat "${c_files[@]}" | wc -l)

    log "  [$project] OptiWeave detection (overflow + FP + data-flow)..."

    # Overflow detection
    local start=$(date +%s%N)
    "$OPTIWEAVE" --detect-overflow --dry-run \
        "${c_files[@]}" -- -std=c11 -w 2>"$out_dir/ow_overflow_stderr.txt" \
        > "$out_dir/ow_overflow_stdout.txt" || true
    cp overflow.txt "$out_dir/overflow_report.txt" 2>/dev/null
    local ow_overflow_ms=$(( ($(date +%s%N) - start) / 1000000 ))

    # Parse overflow results
    local ow_critical ow_warning ow_info ow_total_issues
    ow_total_issues=$(grep -c "^\[" "$out_dir/overflow_report.txt" 2>/dev/null) || ow_total_issues=0
    ow_critical=$(grep -c "^\[critical\]" "$out_dir/overflow_report.txt" 2>/dev/null) || ow_critical=0
    ow_warning=$(grep -c "^\[warning\]" "$out_dir/overflow_report.txt" 2>/dev/null) || ow_warning=0

    # FP precision
    "$OPTIWEAVE" --fp-precision-warnings --dry-run \
        "${c_files[@]}" -- -std=c11 -w 2>"$out_dir/ow_fp_stderr.txt" \
        > /dev/null || true
    cp fp_precision.txt "$out_dir/fp_precision_report.txt" 2>/dev/null
    local ow_fp_issues
    ow_fp_issues=$(grep -c "^\[" "$out_dir/fp_precision_report.txt" 2>/dev/null) || ow_fp_issues=0

    # Data flow analysis
    "$OPTIWEAVE" --data-flow-analysis --dry-run \
        "${c_files[@]}" -- -std=c11 -w 2>"$out_dir/ow_df_stderr.txt" \
        > /dev/null || true
    cp dataflow.txt "$out_dir/dataflow_report.txt" 2>/dev/null
    local ow_uninit ow_unused
    ow_uninit=$(grep -ci "uninit" "$out_dir/dataflow_report.txt" 2>/dev/null) || ow_uninit=0
    ow_unused=$(grep -ci "unused" "$out_dir/dataflow_report.txt" 2>/dev/null) || ow_unused=0

    # Auto-fix dry run (shows what fixes would be applied)
    log "  [$project] OptiWeave auto-fix (dry-run)..."
    "$OPTIWEAVE" --auto-fix-dry-run \
        "${c_files[@]}" -- -std=c11 -w 2>"$out_dir/ow_autofix_stderr.txt" \
        > "$out_dir/ow_autofix_stdout.txt" || true

    local ow_fixes_applied
    ow_fixes_applied=$(grep -c "Applied fix\|Fix applied\|Transformed\|applied.*rule" "$out_dir/ow_autofix_stderr.txt" 2>/dev/null) || ow_fixes_applied=0

    # Complexity analysis
    log "  [$project] OptiWeave complexity analysis..."
    "$OPTIWEAVE" --analyze-complexity --complexity-format=json --complexity-output="$out_dir/complexity.json" \
        "${c_files[@]}" -- -std=c11 -w 2>/dev/null || true

    local ow_functions ow_avg_cc
    ow_functions=$(grep -oP '"total_functions":\s*\K[0-9]+' "$out_dir/complexity.json" 2>/dev/null) || ow_functions=0
    ow_avg_cc=$(grep -oP '"average_cyclomatic":\s*\K[0-9.]+' "$out_dir/complexity.json" 2>/dev/null) || ow_avg_cc=0

    log "    Overflow: ${ow_total_issues} issues (${ow_critical} critical, ${ow_warning} warning)"
    log "    FP precision: ${ow_fp_issues} issues"
    log "    Data flow: ${ow_uninit} uninit, ${ow_unused} unused"
    log "    Functions: ${ow_functions}, avg CC: ${ow_avg_cc}"

    echo '{"project":"'"$project"'","tool":"optiweave","loc":'"$loc"',"c_files":'"${#c_files[@]}"',"overflow_total":'"$ow_total_issues"',"overflow_critical":'"$ow_critical"',"overflow_warning":'"$ow_warning"',"fp_issues":'"$ow_fp_issues"',"uninit_vars":'"$ow_uninit"',"unused_vars":'"$ow_unused"',"functions":'"${ow_functions:-0}"',"avg_cyclomatic":"'"${ow_avg_cc:-0}"'","detection_time_ms":'"$ow_overflow_ms"'}' >> "$RUN_DIR/detection.jsonl"
}

# ============================================================
#  RQ2: clang-tidy comparison
# ============================================================
run_clang_tidy_detection() {
    local project="$1"
    local proj_dir="$PROJECT_DIR/${PROJECT_DIRS[$project]}"
    local out_dir="$RUN_DIR/$project"

    if [ -z "$CLANG_TIDY" ]; then return; fi

    local c_files=($(get_c_files "$proj_dir"))
    if [ ${#c_files[@]} -eq 0 ]; then return; fi

    log "  [$project] clang-tidy..."
    local start=$(date +%s%N)
    local ct_output
    ct_output=$($CLANG_TIDY \
        -checks='-*,bugprone-*,cert-*,clang-analyzer-core.*,clang-analyzer-deadcode.*,clang-analyzer-security.*,misc-redundant-expression,readability-misleading-indentation' \
        --quiet "${c_files[@]}" -- -std=c11 -w 2>&1) || true
    local ct_ms=$(( ($(date +%s%N) - start) / 1000000 ))

    echo "$ct_output" > "$out_dir/clang_tidy_output.txt"

    local ct_warnings ct_integer ct_null ct_security ct_uninit
    ct_warnings=$(echo "$ct_output" | grep -c "warning:" 2>/dev/null) || ct_warnings=0
    ct_integer=$(echo "$ct_output" | grep -cE "bugprone-integer|bugprone-narrowing|bugprone-signed-char|bugprone-too-small" 2>/dev/null) || ct_integer=0
    ct_null=$(echo "$ct_output" | grep -cE "core.NullDereference|core.NonNullParam" 2>/dev/null) || ct_null=0
    ct_security=$(echo "$ct_output" | grep -c "security\." 2>/dev/null) || ct_security=0
    ct_uninit=$(echo "$ct_output" | grep -cE "core.UndefinedBinaryOperatorResult|core.uninitialized" 2>/dev/null) || ct_uninit=0

    log "    Warnings: $ct_warnings (integer: $ct_integer, null: $ct_null, security: $ct_security) in ${ct_ms}ms"

    echo '{"project":"'"$project"'","tool":"clang-tidy","warnings":'"$ct_warnings"',"integer_issues":'"$ct_integer"',"null_deref":'"$ct_null"',"security":'"$ct_security"',"uninit":'"$ct_uninit"',"time_ms":'"$ct_ms"'}' >> "$RUN_DIR/detection.jsonl"
}

# ============================================================
#  RQ2: cppcheck comparison (with timeout)
# ============================================================
run_cppcheck_detection() {
    local project="$1"
    local proj_dir="$PROJECT_DIR/${PROJECT_DIRS[$project]}"
    local out_dir="$RUN_DIR/$project"

    if [ -z "$CPPCHECK" ]; then return; fi

    local c_files=($(get_c_files "$proj_dir"))
    if [ ${#c_files[@]} -eq 0 ]; then return; fi

    log "  [$project] cppcheck (60s timeout)..."
    local start=$(date +%s%N)
    local cc_output
    cc_output=$(timeout 60 $CPPCHECK --enable=warning,style,performance,portability \
        --std=c11 --language=c --suppress=missingInclude --suppress=unusedFunction \
        --force --quiet "${c_files[@]}" 2>&1) || true
    local cc_ms=$(( ($(date +%s%N) - start) / 1000000 ))

    echo "$cc_output" > "$out_dir/cppcheck_output.txt"

    local cc_errors cc_warnings cc_style cc_perf cc_total
    cc_errors=$(echo "$cc_output" | grep -c "(error)" 2>/dev/null) || cc_errors=0
    cc_warnings=$(echo "$cc_output" | grep -c "(warning)" 2>/dev/null) || cc_warnings=0
    cc_style=$(echo "$cc_output" | grep -c "(style)" 2>/dev/null) || cc_style=0
    cc_perf=$(echo "$cc_output" | grep -c "(performance)" 2>/dev/null) || cc_perf=0
    cc_total=$((cc_errors + cc_warnings + cc_style + cc_perf))

    log "    Findings: $cc_total (${cc_errors}E ${cc_warnings}W ${cc_style}S ${cc_perf}P) in ${cc_ms}ms"

    echo '{"project":"'"$project"'","tool":"cppcheck","errors":'"$cc_errors"',"warnings":'"$cc_warnings"',"style":'"$cc_style"',"performance":'"$cc_perf"',"total":'"$cc_total"',"time_ms":'"$cc_ms"'}' >> "$RUN_DIR/detection.jsonl"
}

# ============================================================
#  RQ1: Instrumentation overhead (transform + code size)
# ============================================================
run_instrumentation_overhead() {
    local project="$1"
    local proj_dir="$PROJECT_DIR/${PROJECT_DIRS[$project]}"
    local out_dir="$RUN_DIR/$project/overhead"
    mkdir -p "$out_dir"

    local c_files=($(get_c_files "$proj_dir"))
    if [ ${#c_files[@]} -eq 0 ]; then return; fi

    log "  [$project] Instrumentation overhead..."

    local orig_bytes=$(cat "${c_files[@]}" | wc -c)

    # Transform with all operators
    local start=$(date +%s%N)
    "$OPTIWEAVE" --array-subscripts --arithmetic-ops --comparison-ops --assignment-ops \
        --output-dir="$out_dir/transformed" \
        "${c_files[@]}" -- -std=c11 -w 2>"$out_dir/transform_log.txt" || true
    local transform_ms=$(( ($(date +%s%N) - start) / 1000000 ))

    local transformed_bytes=$(find "$out_dir/transformed" -name "*.c" -exec cat {} + 2>/dev/null | wc -c)
    local size_pct=0
    if [ "$orig_bytes" -gt 0 ] && [ "$transformed_bytes" -gt 0 ]; then
        size_pct=$(( (transformed_bytes - orig_bytes) * 100 / orig_bytes ))
    fi

    # Count transforms from log
    local total_transforms
    total_transforms=$(grep -oP 'Array subscripts transformed: \K[0-9]+' "$out_dir/transform_log.txt" | awk '{s+=$1} END {print s+0}')
    local arith=$(grep -oP 'Arithmetic operators transformed: \K[0-9]+' "$out_dir/transform_log.txt" | awk '{s+=$1} END {print s+0}')
    total_transforms=$((total_transforms + arith))

    log "    Transform: ${transform_ms}ms, ${total_transforms} ops, size ${orig_bytes}→${transformed_bytes} (+${size_pct}%)"

    echo '{"project":"'"$project"'","transform_ms":'"$transform_ms"',"total_transforms":'"$total_transforms"',"original_bytes":'"$orig_bytes"',"transformed_bytes":'"${transformed_bytes:-0}"',"size_increase_pct":'"$size_pct"'}' >> "$RUN_DIR/overhead.jsonl"
}

# ============================================================
#  Summary
# ============================================================
print_summary() {
    python3 - "$RUN_DIR" << 'PYEOF'
import json, sys, os

run_dir = sys.argv[1]

# Load detection results
det = {}
det_file = os.path.join(run_dir, "detection.jsonl")
if os.path.exists(det_file):
    with open(det_file) as f:
        for line in f:
            if line.strip():
                d = json.loads(line)
                key = (d["project"], d["tool"])
                det[key] = d

# Load overhead results
ovh = {}
ovh_file = os.path.join(run_dir, "overhead.jsonl")
if os.path.exists(ovh_file):
    with open(ovh_file) as f:
        for line in f:
            if line.strip():
                d = json.loads(line)
                ovh[d["project"]] = d

projects = sorted(set(k[0] for k in det.keys()))

print("\n" + "="*90)
print("OPTIWEAVE FULL BENCHMARK RESULTS")
print("="*90)

# RQ2: Detection comparison
print("\n--- RQ2: Bug Detection Comparison ---")
print(f"{'Project':<12} {'LOC':>6} | {'OW overflow':>11} {'OW FP':>6} {'OW uninit':>9} | {'CT warn':>8} {'CC total':>8}")
print("-"*75)
for p in projects:
    ow = det.get((p, "optiweave"), {})
    ct = det.get((p, "clang-tidy"), {})
    cc = det.get((p, "cppcheck"), {})
    print(f"{p:<12} {ow.get('loc',0):>6} | {ow.get('overflow_total',0):>11} {ow.get('fp_issues',0):>6} {ow.get('uninit_vars',0):>9} | {ct.get('warnings',0):>8} {cc.get('total',0):>8}")

# Detection time comparison
print(f"\n--- Analysis Time (ms) ---")
print(f"{'Project':<12} {'OptiWeave':>10} {'clang-tidy':>11} {'cppcheck':>10}")
print("-"*45)
for p in projects:
    ow_t = det.get((p, "optiweave"), {}).get("detection_time_ms", 0)
    ct_t = det.get((p, "clang-tidy"), {}).get("time_ms", 0)
    cc_t = det.get((p, "cppcheck"), {}).get("time_ms", 0)
    print(f"{p:<12} {ow_t:>10} {ct_t:>11} {cc_t:>10}")

# Complexity metrics
print(f"\n--- Complexity Metrics ---")
print(f"{'Project':<12} {'Functions':>10} {'Avg CC':>8}")
print("-"*32)
for p in projects:
    ow = det.get((p, "optiweave"), {})
    print(f"{p:<12} {ow.get('functions', 0):>10} {ow.get('avg_cyclomatic', 0):>8}")

# RQ1: Instrumentation overhead
if ovh:
    print(f"\n--- RQ1: Instrumentation Overhead ---")
    print(f"{'Project':<12} {'Time ms':>8} {'Transforms':>11} {'Orig':>8} {'Instr':>8} {'Size+%':>7}")
    print("-"*58)
    for p in sorted(ovh.keys()):
        o = ovh[p]
        print(f"{p:<12} {o['transform_ms']:>8} {o['total_transforms']:>11} {o['original_bytes']:>8} {o.get('transformed_bytes',0):>8} {o['size_increase_pct']:>6}%")

# Save summary
summary = {"detection": {k[0]+"/"+k[1]: v for k,v in det.items()}, "overhead": ovh}
with open(os.path.join(run_dir, "summary.json"), "w") as f:
    json.dump(summary, f, indent=2)

print(f"\nResults: {run_dir}/summary.json")
print("="*90)
PYEOF
}

# ============================================================
#  Main
# ============================================================
log "============================================"
log "  OptiWeave Full Benchmark Suite"
log "  Results: $RUN_DIR"
log "============================================"

for project in tinyexpr cJSON hashmap_c lz4 xxHash lua; do
    log "=== $project ==="
    run_optiweave_detection "$project"
    run_clang_tidy_detection "$project"
    run_cppcheck_detection "$project"
    run_instrumentation_overhead "$project"
    log ""
done

log "=== Generating Summary ==="
print_summary
log "Done."
