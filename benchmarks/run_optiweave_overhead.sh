#!/usr/bin/env bash
#
# OptiWeave Instrumented Runtime Overhead Benchmark
# Measures the runtime cost of OptiWeave's own instrumentation,
# comparing against baseline + gprof + ASan + Valgrind from the previous run.
#
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR/projects"
OPTIWEAVE="$SCRIPT_DIR/../build/optiweave"
RUNTIME_LIB="$SCRIPT_DIR/../build/liboptiweave_runtime.a"
RUNTIME_C="$SCRIPT_DIR/../templates/optiweave/optiweave_runtime.c"
TEMPLATES_DIR="$SCRIPT_DIR/../templates"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RUN_DIR="$SCRIPT_DIR/results/optiweave_overhead_$TIMESTAMP"
PERF=$(which perf 2>/dev/null || echo "")

CC="${CC:-gcc}"
RUNTIME_ITERS=40

mkdir -p "$RUN_DIR"

log() { echo "[$(date +%H:%M:%S)] $*" | tee -a "$RUN_DIR/log.txt"; }

# ============================================================
#  Measure wall time + RSS (the fixed version)
# ============================================================
measure_once() {
    local binary="$1"
    shift
    local time_output
    time_output=$( { /usr/bin/time -v "$binary" "$@" > /dev/null; } 2>&1 )
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

run_iters() {
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
#  perf stat for hardware counters
# ============================================================
run_perf() {
    local label="$1"
    local binary="$2"
    shift 2

    if [ -z "$PERF" ]; then return; fi

    log "    Running perf stat for $label..."
    $PERF stat -e cycles,instructions,cache-references,cache-misses,branches,branch-misses \
        "$binary" "$@" > /dev/null 2> "$RUN_DIR/${label}_perf.txt" || true
}

# ============================================================
#  Build all variants of tinyexpr
# ============================================================
build_tinyexpr() {
    log "=== Building tinyexpr (all variants) ==="
    local te="$PROJECT_DIR/tinyexpr"
    local work="$RUN_DIR/binaries"
    mkdir -p "$work"

    # Driver
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

    # Baseline, gprof, ASan
    $CC -O2 -I"$te" -o "$work/te_baseline" "$work/te_driver.c" "$te/tinyexpr.c" -lm
    $CC -O2 -pg -I"$te" -o "$work/te_gprof" "$work/te_driver.c" "$te/tinyexpr.c" -lm
    $CC -O2 -fsanitize=address -I"$te" -o "$work/te_asan" "$work/te_driver.c" "$te/tinyexpr.c" -lm

    # OptiWeave: instrument tinyexpr.c
    log "  Instrumenting tinyexpr.c with OptiWeave..."
    mkdir -p "$work/te_instrumented"
    "$OPTIWEAVE" --array-subscripts --arithmetic-ops --comparison-ops \
        --output-dir="$work/te_instrumented" \
        "$te/tinyexpr.c" -- -std=c11 -w 2>"$work/te_instrument.log" || true

    if [ ! -f "$work/te_instrumented/tinyexpr.c" ]; then
        log "  ERROR: instrumentation failed"
        cat "$work/te_instrument.log" | tail -10
        return 1
    fi

    # Build instrumented (no stats - macro overhead only)
    log "  Building instrumented (no stats)..."
    $CC -O2 -I"$te" -I"$TEMPLATES_DIR" -I"$SCRIPT_DIR/../include" \
        -o "$work/te_optiweave_nostats" "$work/te_driver.c" "$work/te_instrumented/tinyexpr.c" \
        "$RUNTIME_C" -lm -lpthread 2>"$work/te_compile_nostats.log" || {
            log "  WARN: nostats build failed:"
            tail -5 "$work/te_compile_nostats.log"
        }

    # Build instrumented with full stats collection
    log "  Building instrumented (with stats)..."
    $CC -O2 -DOPTIWEAVE_ENABLE_STATS -I"$te" -I"$TEMPLATES_DIR" -I"$SCRIPT_DIR/../include" \
        -o "$work/te_optiweave_stats" "$work/te_driver.c" "$work/te_instrumented/tinyexpr.c" \
        "$RUNTIME_C" "$RUNTIME_LIB" -lm -lpthread 2>"$work/te_compile_stats.log" || {
            log "  WARN: stats build failed:"
            tail -5 "$work/te_compile_stats.log"
        }

    log "  Built: $(ls $work/te_* 2>/dev/null | grep -v '\.log\|\.c\|instrumented' | xargs -n1 basename | tr '\n' ' ')"
}

# ============================================================
#  Build all variants of lz4
# ============================================================
build_lz4() {
    log "=== Building lz4 (all variants) ==="
    local lz="$PROJECT_DIR/lz4"
    local work="$RUN_DIR/binaries"

    cat > "$work/lz4_driver.c" << 'EOF'
#include <stdio.h>
#include <stdlib.h>
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

    # Instrument lz4.c
    log "  Instrumenting lz4.c..."
    mkdir -p "$work/lz4_instrumented"
    "$OPTIWEAVE" --array-subscripts --arithmetic-ops \
        --output-dir="$work/lz4_instrumented" \
        "$lz/lib/lz4.c" -- -std=c11 -w 2>"$work/lz4_instrument.log" || true

    if [ -f "$work/lz4_instrumented/lz4.c" ]; then
        log "  Building lz4 instrumented (no stats)..."
        $CC -O2 -I"$lz/lib" -I"$TEMPLATES_DIR" -I"$SCRIPT_DIR/../include" \
            -o "$work/lz4_optiweave_nostats" "$work/lz4_driver.c" "$work/lz4_instrumented/lz4.c" \
            "$RUNTIME_C" -lpthread 2>"$work/lz4_compile_nostats.log" || {
                log "  WARN: lz4 nostats build failed"; tail -5 "$work/lz4_compile_nostats.log"
            }

        log "  Building lz4 instrumented (with stats)..."
        $CC -O2 -DOPTIWEAVE_ENABLE_STATS -I"$lz/lib" -I"$TEMPLATES_DIR" -I"$SCRIPT_DIR/../include" \
            -o "$work/lz4_optiweave_stats" "$work/lz4_driver.c" "$work/lz4_instrumented/lz4.c" \
            "$RUNTIME_C" "$RUNTIME_LIB" -lpthread 2>"$work/lz4_compile_stats.log" || {
                log "  WARN: lz4 stats build failed"; tail -5 "$work/lz4_compile_stats.log"
            }
    fi
}

# ============================================================
#  Build all variants of xxHash
# ============================================================
build_xxhash() {
    log "=== Building xxHash (all variants) ==="
    local xh="$PROJECT_DIR/xxHash"
    local work="$RUN_DIR/binaries"

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

    # xxHash uses XXH_INLINE_ALL so we instrument the driver itself
    log "  Instrumenting xxhash driver..."
    mkdir -p "$work/xxh_instrumented"
    "$OPTIWEAVE" --array-subscripts --arithmetic-ops \
        --output-dir="$work/xxh_instrumented" \
        "$work/xxh_driver.c" -- -std=c11 -w -I"$xh" 2>"$work/xxh_instrument.log" || true

    if [ -f "$work/xxh_instrumented/xxh_driver.c" ]; then
        log "  Building xxh instrumented (no stats)..."
        $CC -O2 -I"$xh" -I"$TEMPLATES_DIR" -I"$SCRIPT_DIR/../include" \
            -o "$work/xxh_optiweave_nostats" "$work/xxh_instrumented/xxh_driver.c" \
            "$RUNTIME_C" -lpthread 2>"$work/xxh_compile_nostats.log" || {
                log "  WARN: xxh nostats build failed"; tail -5 "$work/xxh_compile_nostats.log"
            }

        log "  Building xxh instrumented (with stats)..."
        $CC -O2 -DOPTIWEAVE_ENABLE_STATS -I"$xh" -I"$TEMPLATES_DIR" -I"$SCRIPT_DIR/../include" \
            -o "$work/xxh_optiweave_stats" "$work/xxh_instrumented/xxh_driver.c" \
            "$RUNTIME_C" "$RUNTIME_LIB" -lpthread 2>"$work/xxh_compile_stats.log" || {
                log "  WARN: xxh stats build failed"; tail -5 "$work/xxh_compile_stats.log"
            }
    fi
}

# ============================================================
#  Build all variants of Lua
# ============================================================
build_lua() {
    log "=== Building Lua (all variants) ==="
    local lua="$PROJECT_DIR/lua"
    local work="$RUN_DIR/binaries"
    local lua_srcs=$(find "$lua" -maxdepth 1 -name "*.c" -not -name "luac.c" -not -name "onelua.c" -not -name "ltests.c" | tr '\n' ' ')

    $CC -O2 -DLUA_USE_LINUX -o "$work/lua_baseline" $lua_srcs -lm -ldl 2>/dev/null
    $CC -O2 -pg -DLUA_USE_LINUX -o "$work/lua_gprof" $lua_srcs -lm -ldl 2>/dev/null
    $CC -O2 -fsanitize=address -DLUA_USE_LINUX -o "$work/lua_asan" $lua_srcs -lm -ldl 2>/dev/null

    # Lua benchmark script
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

    # Instrument the hottest Lua source files (lvm.c is the bytecode interpreter)
    log "  Instrumenting Lua VM core (lvm.c, lapi.c, ltable.c)..."
    mkdir -p "$work/lua_instrumented"
    for src in lvm.c lapi.c ltable.c; do
        "$OPTIWEAVE" --array-subscripts --arithmetic-ops \
            --output-dir="$work/lua_instrumented" \
            "$lua/$src" -- -std=c11 -w -I"$lua" 2>>"$work/lua_instrument.log" || true
    done

    # Build a list of source files: instrumented for ones we transformed, original for rest
    local lua_mixed_srcs=""
    for src_path in $lua_srcs; do
        local base=$(basename "$src_path")
        if [ -f "$work/lua_instrumented/$base" ]; then
            lua_mixed_srcs="$lua_mixed_srcs $work/lua_instrumented/$base"
        else
            lua_mixed_srcs="$lua_mixed_srcs $src_path"
        fi
    done

    log "  Building Lua instrumented (no stats)..."
    $CC -O2 -DLUA_USE_LINUX -I"$lua" -I"$TEMPLATES_DIR" -I"$SCRIPT_DIR/../include" \
        -o "$work/lua_optiweave_nostats" $lua_mixed_srcs "$RUNTIME_C" -lm -ldl -lpthread \
        2>"$work/lua_compile_nostats.log" || {
            log "  WARN: lua nostats build failed"; tail -5 "$work/lua_compile_nostats.log"
        }

    log "  Building Lua instrumented (with stats)..."
    $CC -O2 -DLUA_USE_LINUX -DOPTIWEAVE_ENABLE_STATS -I"$lua" -I"$TEMPLATES_DIR" -I"$SCRIPT_DIR/../include" \
        -o "$work/lua_optiweave_stats" $lua_mixed_srcs "$RUNTIME_C" "$RUNTIME_LIB" -lm -ldl -lpthread \
        2>"$work/lua_compile_stats.log" || {
            log "  WARN: lua stats build failed"; tail -5 "$work/lua_compile_stats.log"
        }
}

# ============================================================
#  Run all measurements for one project
# ============================================================
run_project() {
    local project="$1"
    local prefix="$2"
    local extra="${3:-}"
    local work="$RUN_DIR/binaries"

    log ""
    log "=== Measuring $project ==="

    # Baseline
    if [ -x "$work/${prefix}_baseline" ]; then
        run_iters "${project}_baseline" $RUNTIME_ITERS "$work/${prefix}_baseline" $extra
        run_perf "${project}_baseline" "$work/${prefix}_baseline" $extra
    fi

    # gprof
    if [ -x "$work/${prefix}_gprof" ]; then
        local cwd=$(pwd); cd "$work"
        run_iters "${project}_gprof" $RUNTIME_ITERS "./${prefix}_gprof" $extra
        rm -f gmon.out; cd "$cwd"
    fi

    # ASan
    if [ -x "$work/${prefix}_asan" ]; then
        run_iters "${project}_asan" $RUNTIME_ITERS "$work/${prefix}_asan" $extra
    fi

    # OptiWeave (no stats - just macro overhead)
    if [ -x "$work/${prefix}_optiweave_nostats" ]; then
        run_iters "${project}_ow_nostats" $RUNTIME_ITERS "$work/${prefix}_optiweave_nostats" $extra
        run_perf "${project}_ow_nostats" "$work/${prefix}_optiweave_nostats" $extra
    fi

    # OptiWeave (with stats collection)
    if [ -x "$work/${prefix}_optiweave_stats" ]; then
        run_iters "${project}_ow_stats" $RUNTIME_ITERS "$work/${prefix}_optiweave_stats" $extra
        run_perf "${project}_ow_stats" "$work/${prefix}_optiweave_stats" $extra
    fi
}

# ============================================================
#  Generate summary
# ============================================================
generate_summary() {
    python3 - "$RUN_DIR" << 'PYEOF'
import json, sys, os, statistics

run_dir = sys.argv[1]
projects = ["tinyexpr", "lz4", "xxhash", "lua"]
configs = ["baseline", "gprof", "asan", "ow_nostats", "ow_stats"]

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

def stats(vals):
    if not vals:
        return None
    vals = sorted(vals)
    n = len(vals)
    return {
        "n": n,
        "mean": round(statistics.mean(vals), 1),
        "median": round(statistics.median(vals), 1),
        "stddev": round(statistics.stdev(vals), 1) if n > 1 else 0,
        "min": min(vals), "max": max(vals),
        "p5": vals[max(0, int(n*0.05))],
        "p95": vals[min(n-1, int(n*0.95))],
    }

print("\n" + "="*100)
print("OPTIWEAVE INSTRUMENTED RUNTIME OVERHEAD")
print("="*100)

print(f"\n{'Project':<10} {'Baseline':>10} {'gprof':>10} {'ASan':>10} {'OW(no-stats)':>13} {'OW(stats)':>11}  |  ratios")
print(f"{'':10} {'(ms med)':>10} {'(ms med)':>10} {'(ms med)':>10} {'(ms med)':>13} {'(ms med)':>11}")
print("-"*100)

results = {}
for proj in projects:
    row = {}
    for cfg in configs:
        t, r = load_dat(os.path.join(run_dir, f"{proj}_{cfg}.dat"))
        row[cfg] = {"time": stats(t), "rss": stats(r)}
    results[proj] = row

    base = row["baseline"]["time"]
    if base is None or base["median"] == 0:
        continue

    base_med = base["median"]
    def ratio(cfg):
        s = row[cfg]["time"]
        if s is None or s["median"] == 0:
            return None
        return s["median"] / base_med

    def fmt_med(cfg):
        s = row[cfg]["time"]
        return f"{s['median']:>.0f}" if s else "  N/A"

    def fmt_ratio(cfg):
        r = ratio(cfg)
        return f"{r:.2f}x" if r is not None else "N/A"

    print(f"{proj:<10} {fmt_med('baseline'):>10} {fmt_med('gprof'):>10} {fmt_med('asan'):>10} {fmt_med('ow_nostats'):>13} {fmt_med('ow_stats'):>11}  |  gprof={fmt_ratio('gprof')} asan={fmt_ratio('asan')} ow_ns={fmt_ratio('ow_nostats')} ow={fmt_ratio('ow_stats')}")

# Detail table
print("\n--- Detailed Statistics (median ms ± stddev) ---")
for proj in projects:
    print(f"\n  {proj}:")
    for cfg in configs:
        s = results[proj][cfg]["time"]
        if s:
            print(f"    {cfg:<13} n={s['n']:>3}  mean={s['mean']:>7.1f}  median={s['median']:>7.1f}  stddev={s['stddev']:>6.1f}  min={s['min']:>5}  max={s['max']:>5}")

# Memory
print("\n--- Memory (median RSS KB) ---")
print(f"{'Project':<10} {'baseline':>10} {'gprof':>10} {'asan':>10} {'ow_ns':>10} {'ow_stats':>10}")
for proj in projects:
    row = results[proj]
    def rss(cfg):
        s = row[cfg]["rss"]
        return f"{s['median']:.0f}" if s else "N/A"
    print(f"{proj:<10} {rss('baseline'):>10} {rss('gprof'):>10} {rss('asan'):>10} {rss('ow_nostats'):>10} {rss('ow_stats'):>10}")

# perf stat parsing
print("\n--- perf stat (instructions per cycle, cache miss rate) ---")
print(f"{'Project/config':<25} {'IPC':>8} {'cache miss%':>12} {'branch miss%':>13}")
for proj in projects:
    for cfg in ["baseline", "ow_nostats", "ow_stats"]:
        pf = os.path.join(run_dir, f"{proj}_{cfg}_perf.txt")
        if os.path.exists(pf):
            cycles = insns = cref = cmiss = brnch = bmiss = 0
            with open(pf) as f:
                for line in f:
                    parts = line.strip().split()
                    if len(parts) < 2: continue
                    val = parts[0].replace(",", "")
                    try: v = int(val)
                    except: continue
                    if "cycles" in line and "cache" not in line: cycles = v
                    elif "instructions" in line: insns = v
                    elif "cache-references" in line: cref = v
                    elif "cache-misses" in line: cmiss = v
                    elif "branches" in line and "miss" not in line: brnch = v
                    elif "branch-misses" in line: bmiss = v
            ipc = insns/cycles if cycles else 0
            cmrate = cmiss*100.0/cref if cref else 0
            bmrate = bmiss*100.0/brnch if brnch else 0
            print(f"{proj+'/'+cfg:<25} {ipc:>8.2f} {cmrate:>11.2f}% {bmrate:>12.2f}%")

# Save JSON
with open(os.path.join(run_dir, "summary.json"), "w") as f:
    json.dump(results, f, indent=2)

print(f"\nResults: {run_dir}/summary.json")
print("="*100)
PYEOF
}

# ============================================================
#  Main
# ============================================================
log "============================================"
log "  OptiWeave Instrumented Runtime Overhead"
log "  Iterations: $RUNTIME_ITERS"
log "  Results: $RUN_DIR"
log "============================================"

build_tinyexpr
build_lz4
build_xxhash
build_lua

run_project tinyexpr te
run_project lz4 lz4
run_project xxhash xxh
run_project lua lua "$RUN_DIR/binaries/bench.lua"

generate_summary

log "Done."
