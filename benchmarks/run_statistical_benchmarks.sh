#!/usr/bin/env bash
#
# OptiWeave Statistical Benchmark Suite (40 iterations)
# Produces publication-quality metrics for thesis Chapter 5
#
# Measures: baseline, gprof, ASan, Valgrind, perf stat, OptiWeave-instrumented
# Statistics: mean, median, stddev, min, max, p5, p95
#
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR/projects"
OPTIWEAVE="$SCRIPT_DIR/../build/optiweave"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RUN_DIR="$SCRIPT_DIR/results/statistical_$TIMESTAMP"

CC="${CC:-gcc}"
RUNTIME_ITERS=40
VALGRIND_ITERS=5
CLANG_TIDY=$(which clang-tidy 2>/dev/null || echo "")
CPPCHECK=$(which cppcheck 2>/dev/null || echo "")
PERF=$(which perf 2>/dev/null || echo "")

mkdir -p "$RUN_DIR"

log() { echo "[$(date +%H:%M:%S)] $*" | tee -a "$RUN_DIR/log.txt"; }

# ============================================================
#  Timing helper: returns wall-clock ms and max RSS KB
# ============================================================
measure_once() {
    local binary="$1"
    shift
    local time_output
    # Capture stderr (where time -v writes) while discarding stdout from binary
    time_output=$( { /usr/bin/time -v "$binary" "$@" > /dev/null; } 2>&1 )

    # Parse wall clock: format is either "h:mm:ss" or "m:ss.ss"
    local wall_str=$(echo "$time_output" | grep "Elapsed (wall clock)" | sed 's/.*): //')
    local wall_ms=$(echo "$wall_str" | awk -F'[:.]' '{
        if (NF>=4) printf "%.0f\n", ($1*3600+$2*60+$3)*1000+$4*10;
        else if (NF==3) printf "%.0f\n", ($1*60+$2)*1000+$3*10;
        else if (NF==2) printf "%.0f\n", $1*1000+$2*10;
        else printf "%.0f\n", $1*1000
    }')
    local max_rss=$(echo "$time_output" | grep "Maximum resident" | awk '{print $NF}')
    echo "${wall_ms:-0} ${max_rss:-0}"
}

# ============================================================
#  Run N iterations, collect timing data into a file
# ============================================================
run_iterations() {
    local label="$1"
    local n="$2"
    local binary="$3"
    shift 3
    local data_file="$RUN_DIR/${label}.dat"

    log "    Running $label ($n iterations)..."
    for i in $(seq 1 $n); do
        local result=$(measure_once "$binary" "$@")
        echo "$result" >> "$data_file"
        if [ $((i % 10)) -eq 0 ]; then
            log "      ... $i/$n done"
        fi
    done
}

# ============================================================
#  Run N iterations under Valgrind
# ============================================================
run_valgrind_iterations() {
    local label="$1"
    local n="$2"
    local binary="$3"
    shift 3
    local data_file="$RUN_DIR/${label}.dat"

    log "    Running $label under Valgrind ($n iterations)..."
    for i in $(seq 1 $n); do
        local result=$(measure_once valgrind --tool=memcheck --leak-check=no "$binary" "$@")
        echo "$result" >> "$data_file"
        log "      ... $i/$n done"
    done
}

# ============================================================
#  Run perf stat (single detailed run)
# ============================================================
run_perf_stat() {
    local label="$1"
    local binary="$2"
    shift 2

    if [ -z "$PERF" ]; then return; fi

    log "    Running perf stat for $label..."
    $PERF stat -e cycles,instructions,cache-references,cache-misses,branches,branch-misses \
        "$binary" "$@" > /dev/null 2> "$RUN_DIR/${label}_perf.txt" || true
}

# ============================================================
#  Compute statistics from .dat file using Python
# ============================================================
compute_stats() {
    local data_file="$1"
    local label="$2"

    python3 - "$data_file" "$label" << 'PYEOF'
import sys, json, statistics

data_file = sys.argv[1]
label = sys.argv[2]

times = []
rss_vals = []
with open(data_file) as f:
    for line in f:
        parts = line.strip().split()
        if len(parts) >= 2:
            times.append(int(parts[0]))
            rss_vals.append(int(parts[1]))

if not times:
    print(json.dumps({"label": label, "error": "no data"}))
    sys.exit(0)

times.sort()
rss_vals.sort()
n = len(times)

def stats(vals):
    if len(vals) == 0:
        return {}
    s = {
        "n": len(vals),
        "mean": round(statistics.mean(vals), 1),
        "median": round(statistics.median(vals), 1),
        "min": min(vals),
        "max": max(vals),
    }
    if len(vals) > 1:
        s["stddev"] = round(statistics.stdev(vals), 1)
        s["p5"] = vals[max(0, int(len(vals)*0.05))]
        s["p95"] = vals[min(len(vals)-1, int(len(vals)*0.95))]
    else:
        s["stddev"] = 0
        s["p5"] = vals[0]
        s["p95"] = vals[0]
    return s

result = {
    "label": label,
    "time_ms": stats(times),
    "rss_kb": stats(rss_vals),
}
print(json.dumps(result))
PYEOF
}

# ============================================================
#  Build test drivers for each project
# ============================================================
build_drivers() {
    log "=== Building test drivers ==="
    local work="$RUN_DIR/binaries"
    mkdir -p "$work"

    # --- tinyexpr (custom driver, ~2s workload) ---
    log "  Building tinyexpr drivers..."
    local te="$PROJECT_DIR/tinyexpr"
    cat > "$work/te_driver.c" << 'EOF'
#include <stdio.h>
#include "tinyexpr.h"
int main() {
    double a = 0.5;
    te_variable vars[] = {{"a", &a}};
    int err;
    te_expr *e1 = te_compile("sqrt(5^2+7^2+11^2+(8-2)^2)", 0, 0, &err);
    te_expr *e2 = te_compile("a+5", vars, 1, &err);
    te_expr *e3 = te_compile("a*sin(a)+cos(a*2)+log(a+1)", vars, 1, &err);
    volatile double total = 0;
    for (int i = 0; i < 500000; i++) {
        total += te_eval(e1);
        a = (double)i / 1000.0;
        total += te_eval(e2);
        total += te_eval(e3);
    }
    te_free(e1); te_free(e2); te_free(e3);
    printf("%f\n", total);
    return 0;
}
EOF
    $CC -O2 -I"$te" -o "$work/te_baseline" "$work/te_driver.c" "$te/tinyexpr.c" -lm 2>/dev/null
    $CC -O2 -pg -I"$te" -o "$work/te_gprof" "$work/te_driver.c" "$te/tinyexpr.c" -lm 2>/dev/null
    $CC -O2 -fsanitize=address -I"$te" -o "$work/te_asan" "$work/te_driver.c" "$te/tinyexpr.c" -lm 2>/dev/null

    # --- lz4 ---
    log "  Building lz4 drivers..."
    local lz="$PROJECT_DIR/lz4"
    cat > "$work/lz4_driver.c" << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lz4.h"
int main() {
    const int BLOCK = 65536;
    const int ROUNDS = 3000;
    char *src = malloc(BLOCK);
    char *dst = malloc(LZ4_compressBound(BLOCK));
    char *dec = malloc(BLOCK);
    for (int i = 0; i < BLOCK; i++) src[i] = (char)(i * 17 + i / 3);
    for (int r = 0; r < ROUNDS; r++) {
        int c = LZ4_compress_default(src, dst, BLOCK, LZ4_compressBound(BLOCK));
        LZ4_decompress_safe(dst, dec, c, BLOCK);
    }
    free(src); free(dst); free(dec);
    return 0;
}
EOF
    $CC -O2 -I"$lz/lib" -o "$work/lz4_baseline" "$work/lz4_driver.c" "$lz/lib/lz4.c" 2>/dev/null
    $CC -O2 -pg -I"$lz/lib" -o "$work/lz4_gprof" "$work/lz4_driver.c" "$lz/lib/lz4.c" 2>/dev/null
    $CC -O2 -fsanitize=address -I"$lz/lib" -o "$work/lz4_asan" "$work/lz4_driver.c" "$lz/lib/lz4.c" 2>/dev/null

    # --- xxHash ---
    log "  Building xxHash drivers..."
    local xh="$PROJECT_DIR/xxHash"
    cat > "$work/xxh_driver.c" << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#define XXH_INLINE_ALL
#include "xxhash.h"
int main() {
    const int BLOCK = 65536;
    const int ROUNDS = 50000;
    char *data = malloc(BLOCK);
    for (int i = 0; i < BLOCK; i++) data[i] = (char)(i * 31 + i / 7);
    unsigned long long total = 0;
    for (int r = 0; r < ROUNDS; r++) {
        total += XXH64(data, BLOCK, r);
        total += XXH3_64bits_withSeed(data, BLOCK, r);
    }
    printf("%llu\n", total);
    free(data);
    return 0;
}
EOF
    $CC -O2 -I"$xh" -o "$work/xxh_baseline" "$work/xxh_driver.c" 2>/dev/null
    $CC -O2 -pg -I"$xh" -o "$work/xxh_gprof" "$work/xxh_driver.c" 2>/dev/null
    $CC -O2 -fsanitize=address -I"$xh" -o "$work/xxh_asan" "$work/xxh_driver.c" 2>/dev/null

    # --- Lua ---
    log "  Building Lua drivers..."
    local lua="$PROJECT_DIR/lua"
    local lua_srcs=$(find "$lua" -maxdepth 1 -name "*.c" -not -name "luac.c" -not -name "onelua.c" -not -name "ltests.c" | tr '\n' ' ')
    $CC -O2 -DLUA_USE_LINUX -o "$work/lua_baseline" $lua_srcs -lm -ldl 2>/dev/null
    $CC -O2 -pg -DLUA_USE_LINUX -o "$work/lua_gprof" $lua_srcs -lm -ldl 2>/dev/null
    $CC -O2 -fsanitize=address -DLUA_USE_LINUX -o "$work/lua_asan" $lua_srcs -lm -ldl 2>/dev/null

    # Lua benchmark script (~3-5s workload)
    cat > "$work/bench.lua" << 'LUAEOF'
local function sieve(n)
    local is_prime = {}
    for i = 2, n do is_prime[i] = true end
    for i = 2, math.floor(math.sqrt(n)) do
        if is_prime[i] then
            for j = i*i, n, i do is_prime[j] = false end
        end
    end
    local count = 0
    for i = 2, n do if is_prime[i] then count = count + 1 end end
    return count
end
local function matmul(n)
    local A, B, C = {}, {}, {}
    for i = 1, n do
        A[i], B[i], C[i] = {}, {}, {}
        for j = 1, n do A[i][j] = i+j; B[i][j] = i*j; C[i][j] = 0 end
    end
    for i = 1, n do
        for j = 1, n do
            local s = 0
            for k = 1, n do s = s + A[i][k] * B[k][j] end
            C[i][j] = s
        end
    end
    return C[1][1]
end
for r = 1, 3 do sieve(50000); matmul(80) end
LUAEOF

    log "  All drivers built."
}

# ============================================================
#  RQ1: Runtime overhead benchmark (40 iterations)
# ============================================================
bench_runtime() {
    local project="$1"
    local prefix="$2"  # binary prefix
    local extra_args="${3:-}"  # extra args for lua
    local work="$RUN_DIR/binaries"

    log "=== RQ1: $project runtime overhead ($RUNTIME_ITERS iterations) ==="

    # Baseline
    if [ -f "$work/${prefix}_baseline" ]; then
        run_iterations "${project}_baseline" $RUNTIME_ITERS "$work/${prefix}_baseline" $extra_args
    fi

    # gprof (run from work dir so gmon.out goes there)
    if [ -f "$work/${prefix}_gprof" ]; then
        local orig_dir=$(pwd)
        cd "$work"
        run_iterations "${project}_gprof" $RUNTIME_ITERS "./${prefix}_gprof" $extra_args
        rm -f gmon.out
        cd "$orig_dir"
    fi

    # ASan
    if [ -f "$work/${prefix}_asan" ]; then
        run_iterations "${project}_asan" $RUNTIME_ITERS "$work/${prefix}_asan" $extra_args
    fi

    # Valgrind (fewer iterations)
    if [ -f "$work/${prefix}_baseline" ]; then
        run_valgrind_iterations "${project}_valgrind" $VALGRIND_ITERS "$work/${prefix}_baseline" $extra_args
    fi

    # perf stat (single detailed run)
    if [ -f "$work/${prefix}_baseline" ]; then
        run_perf_stat "${project}_baseline" "$work/${prefix}_baseline" $extra_args
    fi
}

# ============================================================
#  RQ2: Detection comparison (single run, deterministic)
# ============================================================
run_detection() {
    local project="$1"
    local proj_dir="$2"
    local out_dir="$RUN_DIR/detection/$project"
    mkdir -p "$out_dir"

    local c_files=($(find "$proj_dir" -maxdepth 2 -name "*.c" -not -name "onelua.c" -not -path "*/.git/*" | sort))
    if [ ${#c_files[@]} -eq 0 ]; then return; fi
    local loc=$(cat "${c_files[@]}" 2>/dev/null | wc -l)

    log "=== RQ2: $project detection comparison ==="

    # OptiWeave: overflow + FP + data-flow
    log "  OptiWeave detection..."
    local ow_start=$(date +%s%N)
    "$OPTIWEAVE" --detect-overflow --fp-precision-warnings --data-flow-analysis --dry-run \
        "${c_files[@]}" -- -std=c11 -w 2>"$out_dir/ow_stderr.txt" > /dev/null || true
    local ow_ms=$(( ($(date +%s%N) - ow_start) / 1000000 ))
    cp overflow.txt "$out_dir/" 2>/dev/null
    cp fp_precision.txt "$out_dir/" 2>/dev/null
    cp dataflow.txt "$out_dir/" 2>/dev/null

    local ow_overflow ow_critical ow_warning ow_fp ow_uninit ow_unused
    ow_overflow=$(grep -c "^\[" "$out_dir/overflow.txt" 2>/dev/null) || ow_overflow=0
    ow_critical=$(grep -c "^\[critical\]" "$out_dir/overflow.txt" 2>/dev/null) || ow_critical=0
    ow_warning=$(grep -c "^\[warning\]" "$out_dir/overflow.txt" 2>/dev/null) || ow_warning=0
    ow_fp=$(grep -c "^\[" "$out_dir/fp_precision.txt" 2>/dev/null) || ow_fp=0
    ow_uninit=$(grep -ci "uninit" "$out_dir/dataflow.txt" 2>/dev/null) || ow_uninit=0
    ow_unused=$(grep -ci "unused" "$out_dir/dataflow.txt" 2>/dev/null) || ow_unused=0

    # OptiWeave: complexity
    "$OPTIWEAVE" --analyze-complexity --complexity-format=json --complexity-output="$out_dir/complexity.json" \
        "${c_files[@]}" -- -std=c11 -w 2>/dev/null || true

    # clang-tidy
    local ct_warnings=0 ct_ms=0
    if [ -n "$CLANG_TIDY" ]; then
        log "  clang-tidy..."
        local ct_start=$(date +%s%N)
        local ct_output
        ct_output=$($CLANG_TIDY \
            -checks='-*,bugprone-*,cert-*,clang-analyzer-core.*,clang-analyzer-deadcode.*,clang-analyzer-security.*,misc-redundant-expression' \
            --quiet "${c_files[@]}" -- -std=c11 -w 2>&1) || true
        ct_ms=$(( ($(date +%s%N) - ct_start) / 1000000 ))
        echo "$ct_output" > "$out_dir/clang_tidy.txt"
        ct_warnings=$(echo "$ct_output" | grep -c "warning:" 2>/dev/null) || ct_warnings=0
    fi

    # cppcheck
    local cc_total=0 cc_ms=0
    if [ -n "$CPPCHECK" ]; then
        log "  cppcheck (60s timeout)..."
        local cc_start=$(date +%s%N)
        local cc_output
        cc_output=$(timeout 60 $CPPCHECK --enable=warning,style,performance,portability \
            --std=c11 --language=c --suppress=missingInclude --suppress=unusedFunction \
            --force --quiet "${c_files[@]}" 2>&1) || true
        cc_ms=$(( ($(date +%s%N) - cc_start) / 1000000 ))
        echo "$cc_output" > "$out_dir/cppcheck.txt"
        cc_total=$(echo "$cc_output" | grep -cE "\((error|warning|style|performance|portability)\)" 2>/dev/null) || cc_total=0
    fi

    log "  Results: OW=${ow_overflow}overflow+${ow_fp}fp+${ow_uninit}uninit (${ow_ms}ms) | CT=${ct_warnings} (${ct_ms}ms) | CC=${cc_total} (${cc_ms}ms)"

    echo '{"project":"'"$project"'","loc":'"$loc"',"files":'"${#c_files[@]}"',"ow_overflow":'"$ow_overflow"',"ow_critical":'"$ow_critical"',"ow_warning":'"$ow_warning"',"ow_fp":'"$ow_fp"',"ow_uninit":'"$ow_uninit"',"ow_unused":'"$ow_unused"',"ow_time_ms":'"$ow_ms"',"ct_warnings":'"$ct_warnings"',"ct_time_ms":'"$ct_ms"',"cc_total":'"$cc_total"',"cc_time_ms":'"$cc_ms"'}' >> "$RUN_DIR/detection.jsonl"
}

# ============================================================
#  Final summary with full statistics
# ============================================================
generate_summary() {
    log "=== Generating statistical summary ==="

    python3 - "$RUN_DIR" << 'PYEOF'
import json, sys, os, statistics, glob

run_dir = sys.argv[1]

# ---- Load runtime data ----
projects = ["tinyexpr", "lz4", "xxhash", "lua"]
configs = ["baseline", "gprof", "asan", "valgrind"]

def load_dat(path):
    times, rss = [], []
    if not os.path.exists(path):
        return times, rss
    with open(path) as f:
        for line in f:
            parts = line.strip().split()
            if len(parts) >= 2:
                times.append(int(parts[0]))
                rss.append(int(parts[1]))
    return times, rss

def calc_stats(vals):
    if not vals:
        return {"n":0,"mean":0,"median":0,"stddev":0,"min":0,"max":0,"p5":0,"p95":0}
    vals = sorted(vals)
    n = len(vals)
    s = {
        "n": n,
        "mean": round(statistics.mean(vals), 1),
        "median": round(statistics.median(vals), 1),
        "min": min(vals),
        "max": max(vals),
        "stddev": round(statistics.stdev(vals), 1) if n > 1 else 0,
        "p5": vals[max(0, int(n*0.05))],
        "p95": vals[min(n-1, int(n*0.95))],
    }
    return s

print("\n" + "="*100)
print("OPTIWEAVE STATISTICAL BENCHMARK RESULTS")
print("="*100)

# RQ1: Runtime overhead
print("\n" + "-"*100)
print("RQ1: RUNTIME OVERHEAD (median wall-clock ms, {} iterations, Valgrind {})".format(
    40, 10))
print("-"*100)
print(f"{'Project':<12} {'Baseline':>12} {'gprof':>12} {'ASan':>12} {'Valgrind':>12}  |  {'gprof':>6} {'ASan':>6} {'Valgr':>6}")
print(f"{'':12} {'median(ms)':>12} {'median(ms)':>12} {'median(ms)':>12} {'median(ms)':>12}  |  {'ratio':>6} {'ratio':>6} {'ratio':>6}")
print("-"*100)

runtime_results = {}
for proj in projects:
    row = {}
    for cfg in configs:
        times, rss = load_dat(os.path.join(run_dir, f"{proj}_{cfg}.dat"))
        row[cfg] = {"times": calc_stats(times), "rss": calc_stats(rss)}

    base_med = row["baseline"]["times"]["median"]
    gprof_med = row["gprof"]["times"]["median"]
    asan_med = row["asan"]["times"]["median"]
    valg_med = row["valgrind"]["times"]["median"]

    gprof_ratio = f"{gprof_med/base_med:.1f}x" if base_med > 0 else "N/A"
    asan_ratio = f"{asan_med/base_med:.1f}x" if base_med > 0 else "N/A"
    valg_ratio = f"{valg_med/base_med:.1f}x" if base_med > 0 else "N/A"

    print(f"{proj:<12} {base_med:>12.0f} {gprof_med:>12.0f} {asan_med:>12.0f} {valg_med:>12.0f}  |  {gprof_ratio:>6} {asan_ratio:>6} {valg_ratio:>6}")
    runtime_results[proj] = row

# Memory (RSS)
print(f"\n{'Project':<12} {'Base RSS':>10} {'gprof':>10} {'ASan':>10} {'Valgrind':>10}  (median KB)")
print("-"*55)
for proj in projects:
    row = runtime_results[proj]
    print(f"{proj:<12} {row['baseline']['rss']['median']:>10.0f} {row['gprof']['rss']['median']:>10.0f} {row['asan']['rss']['median']:>10.0f} {row['valgrind']['rss']['median']:>10.0f}")

# Detailed stats
print(f"\n--- Detailed Statistics (wall-clock ms) ---")
for proj in projects:
    print(f"\n  {proj}:")
    for cfg in configs:
        s = runtime_results[proj][cfg]["times"]
        if s["n"] > 0:
            print(f"    {cfg:<12} n={s['n']:>3}  mean={s['mean']:>8.1f}  median={s['median']:>8.1f}  stddev={s['stddev']:>7.1f}  min={s['min']:>6}  max={s['max']:>6}  p5={s['p5']:>6}  p95={s['p95']:>6}")

# ---- Load detection data ----
det_file = os.path.join(run_dir, "detection.jsonl")
if os.path.exists(det_file):
    det_results = []
    with open(det_file) as f:
        for line in f:
            if line.strip():
                det_results.append(json.loads(line))

    print("\n" + "-"*100)
    print("RQ2: BUG DETECTION COMPARISON")
    print("-"*100)
    print(f"{'Project':<12} {'LOC':>6} | {'OW overflow':>11} {'(crit)':>7} {'OW FP':>6} {'OW df':>6} | {'CT warn':>8} {'CC':>5} | {'OW ms':>7} {'CT ms':>8} {'CC ms':>7}")
    print("-"*100)

    totals = {"ow_overflow":0, "ow_fp":0, "ow_uninit":0, "ct_warnings":0, "cc_total":0}
    for d in sorted(det_results, key=lambda x: x["project"]):
        ow_df = d.get("ow_uninit", 0) + d.get("ow_unused", 0)
        print(f"{d['project']:<12} {d['loc']:>6} | {d['ow_overflow']:>11} {d['ow_critical']:>7} {d['ow_fp']:>6} {ow_df:>6} | {d['ct_warnings']:>8} {d['cc_total']:>5} | {d['ow_time_ms']:>7} {d['ct_time_ms']:>8} {d['cc_time_ms']:>7}")
        totals["ow_overflow"] += d["ow_overflow"]
        totals["ow_fp"] += d["ow_fp"]
        totals["ow_uninit"] += d.get("ow_uninit", 0) + d.get("ow_unused", 0)
        totals["ct_warnings"] += d["ct_warnings"]
        totals["cc_total"] += d["cc_total"]

    print("-"*100)
    ow_total = totals["ow_overflow"] + totals["ow_fp"] + totals["ow_uninit"]
    ct_total = totals["ct_warnings"]
    print(f"{'TOTAL':<12} {'':>6} | {totals['ow_overflow']:>11} {'':>7} {totals['ow_fp']:>6} {totals['ow_uninit']:>6} | {ct_total:>8} {totals['cc_total']:>5} |")
    print(f"\nOptiWeave total findings: {ow_total}  |  clang-tidy: {ct_total}  |  cppcheck: {totals['cc_total']}")
    if ct_total > 0:
        print(f"OptiWeave detects {ow_total/ct_total:.1f}x more issues than clang-tidy")

# Save everything
summary = {
    "runtime": {p: {c: {"time": runtime_results[p][c]["times"], "rss": runtime_results[p][c]["rss"]} for c in configs} for p in projects},
}
if os.path.exists(det_file):
    summary["detection"] = det_results

with open(os.path.join(run_dir, "summary.json"), "w") as f:
    json.dump(summary, f, indent=2)

print(f"\nFull results: {run_dir}/summary.json")
print("="*100)
PYEOF
}

# ============================================================
#  Main
# ============================================================
log "============================================================"
log "  OptiWeave Statistical Benchmark Suite"
log "  Runtime iterations: $RUNTIME_ITERS"
log "  Valgrind iterations: $VALGRIND_ITERS"
log "  Results: $RUN_DIR"
log "============================================================"
log ""

# Step 1: Build all drivers
build_drivers

# Step 2: RQ1 - Runtime overhead (40 iterations each)
WORK="$RUN_DIR/binaries"

bench_runtime "tinyexpr" "te"
bench_runtime "lz4" "lz4"
bench_runtime "xxhash" "xxh"
bench_runtime "lua" "lua" "$WORK/bench.lua"

# Step 3: RQ2 - Detection comparison (single run)
run_detection "tinyexpr" "$PROJECT_DIR/tinyexpr"
run_detection "cJSON" "$PROJECT_DIR/cJSON"
run_detection "hashmap_c" "$PROJECT_DIR/hashmap.c"
run_detection "lz4" "$PROJECT_DIR/lz4"
run_detection "xxHash" "$PROJECT_DIR/xxHash"
run_detection "lua" "$PROJECT_DIR/lua"

# Step 4: Generate summary
generate_summary

log "All benchmarks complete."
