#include <optiweave/prelude.hpp>
/**
 * @file test_cache_profiling.cpp
 * @brief Test cache profiling feature for OptiWeave
 *
 * This example demonstrates cache-friendly and cache-unfriendly memory access patterns:
 * - Row-major vs column-major array access
 * - Strided vs contiguous memory access
 * - Large vs small working sets
 */

#include <iostream>
#include <vector>
#include <chrono>
#include <cstring>

constexpr size_t MATRIX_SIZE = 512;
constexpr size_t LARGE_ARRAY_SIZE = 1024 * 1024;  // 1M elements (4MB)

// Cache-friendly: row-major traversal
void row_major_access(double matrix[MATRIX_SIZE][MATRIX_SIZE]) {
    double sum = 0.0;
    for (size_t i = 0; i < MATRIX_SIZE; ++i) {
        for (size_t j = 0; j < MATRIX_SIZE; ++j) {
            sum += optiweave::__ow_subscript_impl(optiweave::__ow_subscript_impl(matrix, i, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/test_cache_profiling.cpp", 24, __FUNCTION__), j, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/test_cache_profiling.cpp", 24, __FUNCTION__);  // Sequential access, cache-friendly
        }
    }
    std::cout << "Row-major sum: " << sum << std::endl;
}

// Cache-unfriendly: column-major traversal
void column_major_access(double matrix[MATRIX_SIZE][MATRIX_SIZE]) {
    double sum = 0.0;
    for (size_t j = 0; j < MATRIX_SIZE; ++j) {
        for (size_t i = 0; i < MATRIX_SIZE; ++i) {
            sum += optiweave::__ow_subscript_impl(optiweave::__ow_subscript_impl(matrix, i, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/test_cache_profiling.cpp", 35, __FUNCTION__), j, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/test_cache_profiling.cpp", 35, __FUNCTION__);  // Strided access, cache-unfriendly
        }
    }
    std::cout << "Column-major sum: " << sum << std::endl;
}

// Cache-friendly: contiguous array access
void contiguous_access(const std::vector<int>& data) {
    long long sum = 0;
    for (size_t i = 0; i < data.size(); ++i) {
        sum += data[i];  // Sequential, cache-friendly
    }
    std::cout << "Contiguous access sum: " << sum << std::endl;
}

// Cache-unfriendly: strided array access
void strided_access(const std::vector<int>& data, size_t stride) {
    long long sum = 0;
    for (size_t i = 0; i < data.size(); i += stride) {
        sum += data[i];  // Strided access, more cache misses
    }
    std::cout << "Strided access (stride=" << stride << ") sum: " << sum << std::endl;
}

// Cache thrashing: accessing more data than cache size
void cache_thrashing_example() {
    constexpr size_t ARRAY_COUNT = 8;
    constexpr size_t ARRAY_SIZE = 256 * 1024;  // 256K elements each (2MB per array)

    std::vector<std::vector<int>> arrays(ARRAY_COUNT);
    for (auto& arr : arrays) {
        arr.resize(ARRAY_SIZE, 42);
    }

    // Access all arrays in round-robin fashion
    // This can cause cache thrashing if total size exceeds L3 cache
    long long sum = 0;
    for (size_t iter = 0; iter < 100; ++iter) {
        for (size_t arr_idx = 0; arr_idx < ARRAY_COUNT; ++arr_idx) {
            for (size_t i = 0; i < 1000; ++i) {
                sum += arrays[arr_idx][i * 256];  // Strided + thrashing
            }
        }
    }
    std::cout << "Cache thrashing sum: " << sum << std::endl;
}

// Simple matrix multiplication (naive implementation)
void naive_matrix_multiply() {
    constexpr size_t N = 128;
    double A[N][N], B[N][N], C[N][N];

    // Initialize
    for (size_t i = 0; i < N; ++i) {
        for (size_t j = 0; j < N; ++j) {
            optiweave::__ow_subscript_impl(optiweave::__ow_subscript_impl(A, i, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/test_cache_profiling.cpp", 90, __FUNCTION__), j, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/test_cache_profiling.cpp", 90, __FUNCTION__) = i + j;
            optiweave::__ow_subscript_impl(optiweave::__ow_subscript_impl(B, i, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/test_cache_profiling.cpp", 91, __FUNCTION__), j, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/test_cache_profiling.cpp", 91, __FUNCTION__) = i * j;
            optiweave::__ow_subscript_impl(optiweave::__ow_subscript_impl(C, i, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/test_cache_profiling.cpp", 92, __FUNCTION__), j, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/test_cache_profiling.cpp", 92, __FUNCTION__) = 0.0;
        }
    }

    // Multiply: C = A * B (naive, cache-unfriendly for B)
    for (size_t i = 0; i < N; ++i) {
        for (size_t j = 0; j < N; ++j) {
            for (size_t k = 0; k < N; ++k) {
                optiweave::__ow_subscript_impl(optiweave::__ow_subscript_impl(C, i, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/test_cache_profiling.cpp", 100, __FUNCTION__), j, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/test_cache_profiling.cpp", 100, __FUNCTION__) += optiweave::__ow_subscript_impl(optiweave::__ow_subscript_impl(A, i, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/test_cache_profiling.cpp", 100, __FUNCTION__), k, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/test_cache_profiling.cpp", 100, __FUNCTION__) * optiweave::__ow_subscript_impl(optiweave::__ow_subscript_impl(B, k, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/test_cache_profiling.cpp", 100, __FUNCTION__), j, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/test_cache_profiling.cpp", 100, __FUNCTION__);  // B[k][j] causes cache misses
            }
        }
    }

    std::cout << "Matrix multiplication result C[0][0]: " << optiweave::__ow_subscript_impl(optiweave::__ow_subscript_impl(C, 0, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/test_cache_profiling.cpp", 105, __FUNCTION__), 0, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/test_cache_profiling.cpp", 105, __FUNCTION__) << std::endl;
}

// Struct of Arrays (cache-friendly)
struct ParticlesSOA {
    std::vector<double> x, y, z;
    std::vector<double> vx, vy, vz;

    ParticlesSOA(size_t n) : x(n), y(n), z(n), vx(n), vy(n), vz(n) {}
};

// Array of Structs (potentially cache-unfriendly)
struct Particle {
    double x, y, z;
    double vx, vy, vz;
};

void update_particles_aos(std::vector<Particle>& particles, double dt) {
    for (size_t i = 0; i < particles.size(); ++i) {
        particles[i].x += particles[i].vx * dt;
        particles[i].y += particles[i].vy * dt;
        particles[i].z += particles[i].vz * dt;
    }
}

void update_particles_soa(ParticlesSOA& particles, double dt) {
    for (size_t i = 0; i < particles.x.size(); ++i) {
        particles.x[i] += particles.vx[i] * dt;
        particles.y[i] += particles.vy[i] * dt;
        particles.z[i] += particles.vz[i] * dt;
    }
}

int main() {
    std::cout << "=== OptiWeave Cache Profiling Test ===" << std::endl;
    std::cout << "\nThis example demonstrates various cache behavior patterns.\n";
    std::cout << "Run with OptiWeave instrumentation to see cache miss statistics.\n\n";

    // Allocate matrix (on heap to avoid stack overflow)
    auto matrix = new double[MATRIX_SIZE][MATRIX_SIZE];

    // Initialize matrix
    for (size_t i = 0; i < MATRIX_SIZE; ++i) {
        for (size_t j = 0; j < MATRIX_SIZE; ++j) {
            optiweave::__ow_subscript_impl(optiweave::__ow_subscript_impl(matrix, i, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/test_cache_profiling.cpp", 149, __FUNCTION__), j, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/test_cache_profiling.cpp", 149, __FUNCTION__) = static_cast<double>(i * MATRIX_SIZE + j);
        }
    }

    // Test 1: Row-major (cache-friendly)
    std::cout << "\n1. Testing row-major access (cache-friendly)..." << std::endl;
    row_major_access(matrix);

    // Test 2: Column-major (cache-unfriendly)
    std::cout << "\n2. Testing column-major access (cache-unfriendly)..." << std::endl;
    column_major_access(matrix);

    delete[] matrix;

    // Test 3: Contiguous access
    std::cout << "\n3. Testing contiguous array access (cache-friendly)..." << std::endl;
    std::vector<int> large_array(LARGE_ARRAY_SIZE);
    for (size_t i = 0; i < large_array.size(); ++i) {
        large_array[i] = static_cast<int>(i);
    }
    contiguous_access(large_array);

    // Test 4: Strided access
    std::cout << "\n4. Testing strided array access (cache-unfriendly)..." << std::endl;
    strided_access(large_array, 64);  // Stride of 64 elements (512 bytes)

    // Test 5: Cache thrashing
    std::cout << "\n5. Testing cache thrashing pattern..." << std::endl;
    cache_thrashing_example();

    // Test 6: Matrix multiplication
    std::cout << "\n6. Testing naive matrix multiplication..." << std::endl;
    naive_matrix_multiply();

    // Test 7: AoS vs SoA
    std::cout << "\n7. Testing Array of Structs (AoS)..." << std::endl;
    constexpr size_t NUM_PARTICLES = 100000;
    std::vector<Particle> particles_aos(NUM_PARTICLES);
    for (auto& p : particles_aos) {
        p.x = p.y = p.z = 0.0;
        p.vx = p.vy = p.vz = 1.0;
    }
    update_particles_aos(particles_aos, 0.01);
    std::cout << "AoS particle update complete" << std::endl;

    std::cout << "\n8. Testing Structure of Arrays (SoA - cache-friendly)..." << std::endl;
    ParticlesSOA particles_soa(NUM_PARTICLES);
    update_particles_soa(particles_soa, 0.01);
    std::cout << "SoA particle update complete" << std::endl;

    std::cout << "\n=== Test Complete ===" << std::endl;
    std::cout << "\nExpected cache profiling results:" << std::endl;
    std::cout << "  • Row-major access: Low cache misses" << std::endl;
    std::cout << "  • Column-major access: High L1/L2 cache misses" << std::endl;
    std::cout << "  • Contiguous access: Low cache misses" << std::endl;
    std::cout << "  • Strided access: High cache misses" << std::endl;
    std::cout << "  • Cache thrashing: High L3 cache misses" << std::endl;
    std::cout << "  • Matrix multiply: High cache misses on B access" << std::endl;
    std::cout << "  • AoS: Moderate cache misses" << std::endl;
    std::cout << "  • SoA: Lower cache misses (better locality)" << std::endl;

    return 0;
}
