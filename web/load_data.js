// OptiWeave Dashboard Data Loader
// This script loads either real data from dashboard_data.json or uses mock data

async function loadDashboardData() {
    try {
        // Try to load real data from JSON file
        const response = await fetch('../examples/dashboard_data.json');

        if (!response.ok) {
            return getMockData();
        }

        const data = await response.json();

        // Check if data is empty or missing critical fields
        if (!data.summary || data.summary.total_operations === 0) {
            return getMockData();
        }

        // Transform real data to match dashboard format
        return {
            summary: {
                total_operations: data.summary.total_operations.toLocaleString(),
                total_runtime_ms: data.summary.total_runtime_ms.toFixed(1),
                issues_found: data.summary.issues_found,
                potential_speedup: data.summary.potential_speedup
            },
            hotspots: data.hotspots.map(h => ({
                name: h.name,
                file: h.file.split('/').pop(), // Get filename only
                time: h.time_ms.toFixed(3) + 'ms',
                percent: h.percent.toFixed(1)
            })),
            operations: data.operations.map(op => ({
                type: op.type,
                count: op.count,
                percent: op.percent.toFixed(1)
            })),
            suggestions: data.suggestions.map(s => ({
                severity: s.severity.toLowerCase(),
                title: s.title,
                description: s.description,
                speedup: s.speedup,
                location: `${s.location.file.split('/').pop()}:${s.location.line}`,
                current_code: s.current_code,
                optimized_code: s.optimized_code
            }))
        };
    } catch (error) {
        // Fallback to mock data if loading fails
        return getMockData();
    }
}

function getMockData() {
    return {
        summary: {
            total_operations: "1,250,000",
            total_runtime_ms: "245.0",
            issues_found: 12,
            potential_speedup: "3.2x"
        },
        hotspots: [
            { name: "matrix_multiply", file: "compute.cpp", time: "89.5ms", percent: "36.5" },
            { name: "vector_add", file: "compute.cpp", time: "45.2ms", percent: "18.4" },
            { name: "fft_transform", file: "signal.cpp", time: "32.1ms", percent: "13.1" },
            { name: "data_copy", file: "memory.cpp", time: "28.7ms", percent: "11.7" },
            { name: "allocate_buffer", file: "memory.cpp", time: "15.3ms", percent: "6.2" }
        ],
        operations: [
            { type: "Array Access", count: 450000, percent: "36.0" },
            { type: "Multiplication", count: 320000, percent: "25.6" },
            { type: "Addition", count: 280000, percent: "22.4" },
            { type: "Division", count: 120000, percent: "9.6" },
            { type: "Comparison", count: 80000, percent: "6.4" }
        ],
        suggestions: [
            {
                severity: "high",
                title: "Division in Hot Loop",
                description: "Matrix multiplication loop contains expensive division operations",
                speedup: "12-15x",
                location: "compute.cpp:45",
                current_code: "for (int i = 0; i < n; i++) {\n    result[i] = data[i] / divisor;\n}",
                optimized_code: "double inv = 1.0 / divisor;\nfor (int i = 0; i < n; i++) {\n    result[i] = data[i] * inv;\n}"
            },
            {
                severity: "high",
                title: "Cache-Unfriendly Memory Access",
                description: "Column-major access pattern causes cache misses",
                speedup: "8-10x",
                location: "compute.cpp:89",
                current_code: "for (int i = 0; i < rows; i++)\n    for (int j = 0; j < cols; j++)\n        sum += matrix[j][i];",
                optimized_code: "for (int j = 0; j < cols; j++)\n    for (int i = 0; i < rows; i++)\n        sum += matrix[j][i];"
            },
            {
                severity: "medium",
                title: "SIMD Vectorization Opportunity",
                description: "Simple arithmetic loop can benefit from SIMD",
                speedup: "3-4x",
                location: "compute.cpp:112",
                current_code: "for (int i = 0; i < n; i++) {\n    c[i] = a[i] + b[i];\n}",
                optimized_code: "#pragma omp simd\nfor (int i = 0; i < n; i++) {\n    c[i] = a[i] + b[i];\n}"
            },
            {
                severity: "medium",
                title: "Repeated sqrt() Calls",
                description: "Computing same square root multiple times",
                speedup: "2-3x",
                location: "signal.cpp:67",
                current_code: "for (int i = 0; i < n; i++) {\n    output[i] = input[i] / sqrt(n);\n}",
                optimized_code: "double inv_sqrt_n = 1.0 / sqrt(n);\nfor (int i = 0; i < n; i++) {\n    output[i] = input[i] * inv_sqrt_n;\n}"
            },
            {
                severity: "low",
                title: "Branch Prediction Issue",
                description: "Unpredictable branching in tight loop",
                speedup: "1.5-2x",
                location: "compute.cpp:145",
                current_code: "for (int i = 0; i < n; i++) {\n    if (data[i] > threshold)\n        result++;\n}",
                optimized_code: "for (int i = 0; i < n; i++) {\n    result += (data[i] > threshold);\n}"
            }
        ]
    };
}
