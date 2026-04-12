#!/usr/bin/env bash
#
# OptiWeave Runtime Overhead Benchmark (RQ1)
# Compares: baseline, gprof, ASan, Valgrind, OptiWeave-instrumented
#
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR/projects"
OPTIWEAVE="$SCRIPT_DIR/../build/optiweave"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RUN_DIR="$SCRIPT_DIR/results/overhead_$TIMESTAMP"
WORK_DIR="$RUN_DIR/work"

CC="${CC:-gcc}"
ITERATIONS=5  # Run each config N times and take median

mkdir -p "$RUN_DIR" "$WORK_DIR"

log() { echo "[$(date +%H:%M:%S)] $*" | tee -a "$RUN_DIR/log.txt"; }

# ============================================================
#  Helper: measure runtime (returns milliseconds)
# ============================================================
measure_time() {
    local cmd="$1"
    local label="$2"
    local timeout_sec="${3:-60}"

    # Use /usr/bin/time for wall clock + max RSS
    local time_output
    time_output=$( { /usr/bin/time -v timeout "$timeout_sec" bash -c "$cmd" > /dev/null 2>&1; } 2>&1 ) || true

    local wall_time_str=$(echo "$time_output" | grep "Elapsed (wall clock)" | sed 's/.*): //')
    local wall_ms=$(echo "$wall_time_str" | awk -F'[:.]' '{
        if (NF==4) printf "%.0f\n", ($1*3600+$2*60+$3)*1000+$4*10;
        else if (NF==3) printf "%.0f\n", ($1*60+$2)*1000+$3*10;
        else if (NF==2) printf "%.0f\n", $1*1000+$2*10;
        else printf "%.0f\n", $1*1000
    }')
    local max_rss=$(echo "$time_output" | grep "Maximum resident" | awk '{print $NF}')

    echo "${wall_ms:-0} ${max_rss:-0}"
}

# ============================================================
#  Benchmark: tinyexpr
# ============================================================
bench_tinyexpr() {
    log "=== tinyexpr: Runtime Overhead ==="
    local src="$PROJECT_DIR/tinyexpr"
    local work="$WORK_DIR/tinyexpr"
    mkdir -p "$work"

    # 1. Build baseline
    log "  Building baseline..."
    $CC -O2 -o "$work/bench_baseline" "$src/tinyexpr.c" "$src/benchmark.c" -lm 2>/dev/null
    if [ $? -ne 0 ]; then
        log "  ERROR: baseline build failed, skipping tinyexpr"
        return
    fi

    # 2. Build with gprof
    log "  Building gprof..."
    $CC -O2 -pg -o "$work/bench_gprof" "$src/tinyexpr.c" "$src/benchmark.c" -lm 2>/dev/null

    # 3. Build with ASan
    log "  Building ASan..."
    $CC -O2 -fsanitize=address -o "$work/bench_asan" "$src/tinyexpr.c" "$src/benchmark.c" -lm 2>/dev/null

    # 4. Run baseline
    log "  Running baseline..."
    local base_result=$(measure_time "$work/bench_baseline" "baseline" 30)
    local base_ms=$(echo $base_result | awk '{print $1}')
    local base_rss=$(echo $base_result | awk '{print $2}')

    # 5. Run gprof
    log "  Running gprof..."
    local gprof_result=$(measure_time "cd $work && ./bench_gprof" "gprof" 30)
    local gprof_ms=$(echo $gprof_result | awk '{print $1}')
    local gprof_rss=$(echo $gprof_result | awk '{print $2}')

    # 6. Run ASan
    log "  Running ASan..."
    local asan_result=$(measure_time "$work/bench_asan" "asan" 60)
    local asan_ms=$(echo $asan_result | awk '{print $1}')
    local asan_rss=$(echo $asan_result | awk '{print $2}')

    # 7. Run Valgrind
    log "  Running Valgrind..."
    local valgrind_result=$(measure_time "valgrind --tool=memcheck --leak-check=no $work/bench_baseline" "valgrind" 120)
    local valgrind_ms=$(echo $valgrind_result | awk '{print $1}')
    local valgrind_rss=$(echo $valgrind_result | awk '{print $2}')

    # Calculate overheads
    local gprof_overhead="N/A"
    local asan_overhead="N/A"
    local valgrind_overhead="N/A"
    if [ "${base_ms:-0}" -gt 0 ]; then
        gprof_overhead=$(awk "BEGIN {printf \"%.1f\", ${gprof_ms:-0} / $base_ms}")
        asan_overhead=$(awk "BEGIN {printf \"%.1f\", ${asan_ms:-0} / $base_ms}")
        valgrind_overhead=$(awk "BEGIN {printf \"%.1f\", ${valgrind_ms:-0} / $base_ms}")
    fi

    log "  Results: base=${base_ms}ms gprof=${gprof_ms}ms(${gprof_overhead}x) asan=${asan_ms}ms(${asan_overhead}x) valgrind=${valgrind_ms}ms(${valgrind_overhead}x)"
    log "  RSS(KB): base=${base_rss} gprof=${gprof_rss} asan=${asan_rss} valgrind=${valgrind_rss}"

    echo '{"project":"tinyexpr","baseline_ms":'"${base_ms:-0}"',"baseline_rss":'"${base_rss:-0}"',"gprof_ms":'"${gprof_ms:-0}"',"gprof_rss":'"${gprof_rss:-0}"',"asan_ms":'"${asan_ms:-0}"',"asan_rss":'"${asan_rss:-0}"',"valgrind_ms":'"${valgrind_ms:-0}"',"valgrind_rss":'"${valgrind_rss:-0}"',"gprof_overhead":"'"$gprof_overhead"'","asan_overhead":"'"$asan_overhead"'","valgrind_overhead":"'"$valgrind_overhead"'"}' >> "$RUN_DIR/overhead.jsonl"
}

# ============================================================
#  Benchmark: lz4 (simple_buffer example)
# ============================================================
bench_lz4() {
    log "=== lz4: Runtime Overhead ==="
    local src="$PROJECT_DIR/lz4"
    local work="$WORK_DIR/lz4"
    mkdir -p "$work"

    # Create a test driver that compresses/decompresses repeatedly
    cat > "$work/bench_lz4_driver.c" << 'DRIVER_EOF'
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lz4.h"

int main() {
    const int BLOCK = 65536;
    const int ROUNDS = 1000;
    char *src = malloc(BLOCK);
    char *dst = malloc(LZ4_compressBound(BLOCK));
    char *dec = malloc(BLOCK);

    // Fill with semi-random data
    for (int i = 0; i < BLOCK; i++)
        src[i] = (char)(i * 17 + i / 3);

    for (int r = 0; r < ROUNDS; r++) {
        int compressed = LZ4_compress_default(src, dst, BLOCK, LZ4_compressBound(BLOCK));
        LZ4_decompress_safe(dst, dec, compressed, BLOCK);
    }

    free(src); free(dst); free(dec);
    return 0;
}
DRIVER_EOF

    # 1. Build baseline
    log "  Building baseline..."
    $CC -O2 -I"$src/lib" -o "$work/bench_baseline" "$work/bench_lz4_driver.c" "$src/lib/lz4.c" 2>/dev/null
    if [ $? -ne 0 ]; then
        log "  ERROR: baseline build failed, skipping lz4"
        return
    fi

    # 2. gprof
    log "  Building gprof..."
    $CC -O2 -pg -I"$src/lib" -o "$work/bench_gprof" "$work/bench_lz4_driver.c" "$src/lib/lz4.c" 2>/dev/null

    # 3. ASan
    log "  Building ASan..."
    $CC -O2 -fsanitize=address -I"$src/lib" -o "$work/bench_asan" "$work/bench_lz4_driver.c" "$src/lib/lz4.c" 2>/dev/null

    # Run all
    log "  Running baseline..."
    local base_result=$(measure_time "$work/bench_baseline" "baseline" 60)
    local base_ms=$(echo $base_result | awk '{print $1}')
    local base_rss=$(echo $base_result | awk '{print $2}')

    log "  Running gprof..."
    local gprof_result=$(measure_time "cd $work && ./bench_gprof" "gprof" 60)
    local gprof_ms=$(echo $gprof_result | awk '{print $1}')
    local gprof_rss=$(echo $gprof_result | awk '{print $2}')

    log "  Running ASan..."
    local asan_result=$(measure_time "$work/bench_asan" "asan" 120)
    local asan_ms=$(echo $asan_result | awk '{print $1}')
    local asan_rss=$(echo $asan_result | awk '{print $2}')

    log "  Running Valgrind..."
    local valgrind_result=$(measure_time "valgrind --tool=memcheck --leak-check=no $work/bench_baseline" "valgrind" 300)
    local valgrind_ms=$(echo $valgrind_result | awk '{print $1}')
    local valgrind_rss=$(echo $valgrind_result | awk '{print $2}')

    local gprof_overhead="N/A"
    local asan_overhead="N/A"
    local valgrind_overhead="N/A"
    if [ "${base_ms:-0}" -gt 0 ]; then
        gprof_overhead=$(awk "BEGIN {printf \"%.1f\", ${gprof_ms:-0} / $base_ms}")
        asan_overhead=$(awk "BEGIN {printf \"%.1f\", ${asan_ms:-0} / $base_ms}")
        valgrind_overhead=$(awk "BEGIN {printf \"%.1f\", ${valgrind_ms:-0} / $base_ms}")
    fi

    log "  Results: base=${base_ms}ms gprof=${gprof_ms}ms(${gprof_overhead}x) asan=${asan_ms}ms(${asan_overhead}x) valgrind=${valgrind_ms}ms(${valgrind_overhead}x)"
    log "  RSS(KB): base=${base_rss} gprof=${gprof_rss} asan=${asan_rss} valgrind=${valgrind_rss}"

    echo '{"project":"lz4","baseline_ms":'"${base_ms:-0}"',"baseline_rss":'"${base_rss:-0}"',"gprof_ms":'"${gprof_ms:-0}"',"gprof_rss":'"${gprof_rss:-0}"',"asan_ms":'"${asan_ms:-0}"',"asan_rss":'"${asan_rss:-0}"',"valgrind_ms":'"${valgrind_ms:-0}"',"valgrind_rss":'"${valgrind_rss:-0}"',"gprof_overhead":"'"$gprof_overhead"'","asan_overhead":"'"$asan_overhead"'","valgrind_overhead":"'"$valgrind_overhead"'"}' >> "$RUN_DIR/overhead.jsonl"
}

# ============================================================
#  Benchmark: xxHash
# ============================================================
bench_xxhash() {
    log "=== xxHash: Runtime Overhead ==="
    local src="$PROJECT_DIR/xxHash"
    local work="$WORK_DIR/xxhash"
    mkdir -p "$work"

    # Create a test driver
    cat > "$work/bench_xxhash_driver.c" << 'DRIVER_EOF'
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define XXH_INLINE_ALL
#include "xxhash.h"

int main() {
    const int BLOCK = 65536;
    const int ROUNDS = 50000;
    char *data = malloc(BLOCK);
    for (int i = 0; i < BLOCK; i++)
        data[i] = (char)(i * 31 + i / 7);

    unsigned long long total = 0;
    for (int r = 0; r < ROUNDS; r++) {
        total += XXH64(data, BLOCK, r);
        total += XXH3_64bits_withSeed(data, BLOCK, r);
    }

    printf("Hash: %llu\n", total);
    free(data);
    return 0;
}
DRIVER_EOF

    log "  Building baseline..."
    $CC -O2 -I"$src" -o "$work/bench_baseline" "$work/bench_xxhash_driver.c" 2>/dev/null
    if [ $? -ne 0 ]; then
        log "  ERROR: baseline build failed, skipping xxhash"
        return
    fi

    log "  Building gprof..."
    $CC -O2 -pg -I"$src" -o "$work/bench_gprof" "$work/bench_xxhash_driver.c" 2>/dev/null

    log "  Building ASan..."
    $CC -O2 -fsanitize=address -I"$src" -o "$work/bench_asan" "$work/bench_xxhash_driver.c" 2>/dev/null

    log "  Running baseline..."
    local base_result=$(measure_time "$work/bench_baseline" "baseline" 60)
    local base_ms=$(echo $base_result | awk '{print $1}')
    local base_rss=$(echo $base_result | awk '{print $2}')

    log "  Running gprof..."
    local gprof_result=$(measure_time "cd $work && ./bench_gprof" "gprof" 60)
    local gprof_ms=$(echo $gprof_result | awk '{print $1}')
    local gprof_rss=$(echo $gprof_result | awk '{print $2}')

    log "  Running ASan..."
    local asan_result=$(measure_time "$work/bench_asan" "asan" 120)
    local asan_ms=$(echo $asan_result | awk '{print $1}')
    local asan_rss=$(echo $asan_result | awk '{print $2}')

    log "  Running Valgrind..."
    local valgrind_result=$(measure_time "valgrind --tool=memcheck --leak-check=no $work/bench_baseline" "valgrind" 300)
    local valgrind_ms=$(echo $valgrind_result | awk '{print $1}')
    local valgrind_rss=$(echo $valgrind_result | awk '{print $2}')

    local gprof_overhead="N/A"
    local asan_overhead="N/A"
    local valgrind_overhead="N/A"
    if [ "${base_ms:-0}" -gt 0 ]; then
        gprof_overhead=$(awk "BEGIN {printf \"%.1f\", ${gprof_ms:-0} / $base_ms}")
        asan_overhead=$(awk "BEGIN {printf \"%.1f\", ${asan_ms:-0} / $base_ms}")
        valgrind_overhead=$(awk "BEGIN {printf \"%.1f\", ${valgrind_ms:-0} / $base_ms}")
    fi

    log "  Results: base=${base_ms}ms gprof=${gprof_ms}ms(${gprof_overhead}x) asan=${asan_ms}ms(${asan_overhead}x) valgrind=${valgrind_ms}ms(${valgrind_overhead}x)"
    log "  RSS(KB): base=${base_rss} gprof=${gprof_rss} asan=${asan_rss} valgrind=${valgrind_rss}"

    echo '{"project":"xxhash","baseline_ms":'"${base_ms:-0}"',"baseline_rss":'"${base_rss:-0}"',"gprof_ms":'"${gprof_ms:-0}"',"gprof_rss":'"${gprof_rss:-0}"',"asan_ms":'"${asan_ms:-0}"',"asan_rss":'"${asan_rss:-0}"',"valgrind_ms":'"${valgrind_ms:-0}"',"valgrind_rss":'"${valgrind_rss:-0}"',"gprof_overhead":"'"$gprof_overhead"'","asan_overhead":"'"$asan_overhead"'","valgrind_overhead":"'"$valgrind_overhead"'"}' >> "$RUN_DIR/overhead.jsonl"
}

# ============================================================
#  Benchmark: Lua (run a compute-heavy script)
# ============================================================
bench_lua() {
    log "=== Lua: Runtime Overhead ==="
    local src="$PROJECT_DIR/lua"
    local work="$WORK_DIR/lua"
    mkdir -p "$work"

    # Build Lua - it has its own simple build
    local lua_c_files=$(find "$src" -maxdepth 1 -name "*.c" -not -name "luac.c" -not -name "onelua.c" -not -name "ltests.c" | tr '\n' ' ')

    log "  Building baseline..."
    $CC -O2 -DLUA_USE_LINUX -o "$work/lua_baseline" $lua_c_files -lm -ldl 2>/dev/null
    if [ $? -ne 0 ]; then
        log "  ERROR: baseline build failed, skipping lua"
        return
    fi

    log "  Building gprof..."
    $CC -O2 -pg -DLUA_USE_LINUX -o "$work/lua_gprof" $lua_c_files -lm -ldl 2>/dev/null

    log "  Building ASan..."
    $CC -O2 -fsanitize=address -DLUA_USE_LINUX -o "$work/lua_asan" $lua_c_files -lm -ldl 2>/dev/null

    # Create a compute-heavy Lua script
    cat > "$work/bench.lua" << 'LUA_EOF'
-- Sieve of Eratosthenes + matrix multiply
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
        for j = 1, n do
            A[i][j] = i + j
            B[i][j] = i * j
            C[i][j] = 0
        end
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

for round = 1, 5 do
    io.write(string.format("Round %d: primes=%d mat=%d\n",
        round, sieve(100000), matmul(100)))
end
LUA_EOF

    log "  Running baseline..."
    local base_result=$(measure_time "$work/lua_baseline $work/bench.lua" "baseline" 60)
    local base_ms=$(echo $base_result | awk '{print $1}')
    local base_rss=$(echo $base_result | awk '{print $2}')

    log "  Running gprof..."
    local gprof_result=$(measure_time "cd $work && ./lua_gprof bench.lua" "gprof" 60)
    local gprof_ms=$(echo $gprof_result | awk '{print $1}')
    local gprof_rss=$(echo $gprof_result | awk '{print $2}')

    log "  Running ASan..."
    local asan_result=$(measure_time "$work/lua_asan $work/bench.lua" "asan" 120)
    local asan_ms=$(echo $asan_result | awk '{print $1}')
    local asan_rss=$(echo $asan_result | awk '{print $2}')

    log "  Running Valgrind..."
    local valgrind_result=$(measure_time "valgrind --tool=memcheck --leak-check=no $work/lua_baseline $work/bench.lua" "valgrind" 300)
    local valgrind_ms=$(echo $valgrind_result | awk '{print $1}')
    local valgrind_rss=$(echo $valgrind_result | awk '{print $2}')

    local gprof_overhead="N/A"
    local asan_overhead="N/A"
    local valgrind_overhead="N/A"
    if [ "${base_ms:-0}" -gt 0 ]; then
        gprof_overhead=$(awk "BEGIN {printf \"%.1f\", ${gprof_ms:-0} / $base_ms}")
        asan_overhead=$(awk "BEGIN {printf \"%.1f\", ${asan_ms:-0} / $base_ms}")
        valgrind_overhead=$(awk "BEGIN {printf \"%.1f\", ${valgrind_ms:-0} / $base_ms}")
    fi

    log "  Results: base=${base_ms}ms gprof=${gprof_ms}ms(${gprof_overhead}x) asan=${asan_ms}ms(${asan_overhead}x) valgrind=${valgrind_ms}ms(${valgrind_overhead}x)"
    log "  RSS(KB): base=${base_rss} gprof=${gprof_rss} asan=${asan_rss} valgrind=${valgrind_rss}"

    echo '{"project":"lua","baseline_ms":'"${base_ms:-0}"',"baseline_rss":'"${base_rss:-0}"',"gprof_ms":'"${gprof_ms:-0}"',"gprof_rss":'"${gprof_rss:-0}"',"asan_ms":'"${asan_ms:-0}"',"asan_rss":'"${asan_rss:-0}"',"valgrind_ms":'"${valgrind_ms:-0}"',"valgrind_rss":'"${valgrind_rss:-0}"',"gprof_overhead":"'"$gprof_overhead"'","asan_overhead":"'"$asan_overhead"'","valgrind_overhead":"'"$valgrind_overhead"'"}' >> "$RUN_DIR/overhead.jsonl"
}

# ============================================================
#  Summary
# ============================================================
print_summary() {
    log ""
    log "============================================"
    log "  RQ1: Runtime Overhead Comparison"
    log "============================================"

    python3 - "$RUN_DIR" << 'PYEOF'
import json, sys, os

run_dir = sys.argv[1]
f = os.path.join(run_dir, "overhead.jsonl")
if not os.path.exists(f):
    print("No results found"); sys.exit(1)

results = []
with open(f) as fh:
    for line in fh:
        if line.strip():
            results.append(json.loads(line))

print(f"\n{'Project':<12} {'Baseline':>10} {'gprof':>10} {'ASan':>10} {'Valgrind':>10}")
print(f"{'':12} {'(ms)':>10} {'(ms/x)':>10} {'(ms/x)':>10} {'(ms/x)':>10}")
print("-"*55)

for r in results:
    p = r["project"]
    b = r["baseline_ms"]
    g = f"{r['gprof_ms']}/{r['gprof_overhead']}x"
    a = f"{r['asan_ms']}/{r['asan_overhead']}x"
    v = f"{r['valgrind_ms']}/{r['valgrind_overhead']}x"
    print(f"{p:<12} {b:>10} {g:>10} {a:>10} {v:>10}")

print(f"\n{'Project':<12} {'Base RSS':>10} {'gprof':>10} {'ASan':>10} {'Valgrind':>10}")
print(f"{'':12} {'(KB)':>10} {'(KB)':>10} {'(KB)':>10} {'(KB)':>10}")
print("-"*55)

for r in results:
    p = r["project"]
    print(f"{p:<12} {r['baseline_rss']:>10} {r['gprof_rss']:>10} {r['asan_rss']:>10} {r['valgrind_rss']:>10}")

# Save structured
with open(os.path.join(run_dir, "overhead_summary.json"), "w") as out:
    json.dump({"results": results}, out, indent=2)
print(f"\nSaved to: {run_dir}/overhead_summary.json")
PYEOF
}

# ============================================================
#  Main
# ============================================================
log "============================================"
log "  OptiWeave Runtime Overhead Benchmark (RQ1)"
log "  Comparing: baseline, gprof, ASan, Valgrind"
log "  Results: $RUN_DIR"
log "============================================"
log ""

bench_tinyexpr
bench_lz4
bench_xxhash
bench_lua
print_summary

log "Done."
