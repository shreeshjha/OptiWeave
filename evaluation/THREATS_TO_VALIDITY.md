# Threats to Validity

This document discusses potential threats to the validity of OptiWeave's evaluation results and the mitigations applied.

---

## Overview

| Validity Type | Key Threats | Mitigations |
|---------------|-------------|-------------|
| **Internal** | Measurement noise, instrumentation bias | Warmup, 10 iterations, 95% CI |
| **External** | Benchmark representativeness | Multiple benchmark suites + real-world projects |
| **Construct** | Synthetic test cases | CWE-based patterns, real library testing |
| **Conclusion** | Multiple comparisons | Report raw p-values, effect sizes |

---

## Internal Validity

Internal validity concerns whether the observed effects are actually caused by the treatment (OptiWeave instrumentation) rather than confounding factors.

### Threat 1: Measurement Noise

**Description**: Timing measurements may be affected by system load, caching effects, CPU frequency scaling, and other environmental factors.

**Mitigation**:
- **10 iterations per benchmark**: Reduces random variation impact
- **First iteration discarded**: Warmup eliminates cold-cache effects
- **95% confidence intervals**: Quantifies measurement uncertainty
- **IQR-based outlier removal**: Removes anomalous measurements
- **Controlled environment**: Same machine for all measurements

**Evidence**:
```
Mean overhead: 4.80%
95% CI: Reported for each benchmark
Outliers removed: IQR method applied
```

### Threat 2: Instrumentation Overhead Bias

**Description**: The act of measuring may affect what is being measured (observer effect). OptiWeave instrumentation could affect cache behavior or branch prediction.

**Mitigation**:
- **Comparison with gprof**: Similar instrumentation approach
- **Selective instrumentation**: Only instrument what's needed
- **Separate timing code**: Timing measurements outside hot loops
- **Multiple instrumentation levels tested**: Array-only vs full

**Evidence**:
```
Array-only overhead: 4.80%
Full instrumentation: 69.3%
gprof overhead: 10.6%
```

### Threat 3: Implementation Bugs

**Description**: Bugs in OptiWeave could cause incorrect statistics or missed bugs.

**Mitigation**:
- **Unit tests**: Test suite for core functionality
- **Manual verification**: Sample outputs manually verified
- **Ground truth comparison**: Known bugs in test cases

---

## External Validity

External validity concerns whether results generalize beyond the specific benchmarks and conditions tested.

### Threat 1: Benchmark Representativeness

**Description**: Polybench kernels are synthetic scientific computing benchmarks that may not represent real-world applications.

**Mitigation**:
- **Multiple benchmark sources**: Polybench + custom use cases
- **Real-world libraries tested**: json-c, cJSON
- **Diverse categories**: 6 Polybench categories covering different patterns
- **Industry acceptance**: Polybench widely used in compiler research

**Evidence**:
| Benchmark Source | Benchmarks | Overhead |
|------------------|------------|----------|
| Polybench | 27 kernels | 4.80% mean |
| Use cases | 6 demos | Variable |
| Real libraries | json-c, cJSON | <20% |

### Threat 2: Language Scope

**Description**: Only C/C++ tested. Results may not generalize to other languages.

**Mitigation**:
- **Scope limitation acknowledged**: OptiWeave is designed for C/C++
- **LLVM-based**: Could extend to other LLVM-supported languages

### Threat 3: Hardware/Platform Dependence

**Description**: Results obtained on specific hardware may differ on other platforms.

**Mitigation**:
- **Standard x86-64 platform**: Most common deployment target
- **Linux environment**: Widely used for development
- **Relative comparisons**: Overhead % is relative, not absolute

**Test Environment**:
```
Platform: Linux 6.8.0-59-generic
CPU: Intel/AMD x86_64
Compiler: g++ 13+ / clang 17+
```

---

## Construct Validity

Construct validity concerns whether we are measuring what we claim to measure.

### Threat 1: Synthetic Test Cases

**Description**: Bug detection test cases are manually created, not real-world CVEs.

**Mitigation**:
- **CWE-based patterns**: Test cases based on CWE-190 (Integer Overflow)
- **Ground truth verification**: Each test case manually analyzed
- **Real library testing**: json-c case study with 57 real warnings
- **Multiple categories**: 8 bug categories tested

**Evidence**:
| Source | Test Cases | Type |
|--------|------------|------|
| Synthetic suite | 51 | Controlled experiments |
| json-c | 57 warnings | Real-world validation |

### Threat 2: Ground Truth Reliability

**Description**: Ground truth labels created by authors may contain bias.

**Mitigation**:
- **Conservative labeling**: When uncertain, labeled as negative
- **Clear criteria**: Documented labeling rules
- **Reproducible**: Test cases and labels available for review

**Limitation Acknowledged**: No inter-rater reliability calculated. Single expert labeling.

### Threat 3: Overhead Percentage Interpretation

**Description**: Percentage overhead may hide absolute timing differences that matter to users.

**Mitigation**:
- **Absolute times reported**: CSV contains raw timings
- **Short benchmarks noted**: <10ms benchmarks identified
- **Multiple overhead levels**: Array-only vs full comparison

---

## Conclusion Validity

Conclusion validity concerns whether the statistical conclusions are correct.

### Threat 1: Multiple Comparisons

**Description**: Testing 27 benchmarks increases risk of Type I errors (false positives).

**Mitigation**:
- **Raw p-values reported**: Transparent about significance levels
- **Bonferroni correction noted**: α_corrected = 0.05/27 = 0.00185
- **Effect sizes reported**: Cohen's d for practical significance
- **Conservative interpretation**: Focus on overall trends, not individual p-values

**Statistical Approach**:
```
Significance level: α = 0.05
Bonferroni-corrected: α = 0.00185
Tests used: Paired t-test
Effect size: Cohen's d
```

### Threat 2: Sample Sizes

**Description**: 10 iterations per benchmark may be insufficient for high-variance benchmarks.

**Mitigation**:
- **Confidence intervals reported**: Width indicates uncertainty
- **Significance test**: Distinguishes real effects from noise
- **Conservative claims**: Only statistically significant results highlighted

**Evidence**:
```
12/27 benchmarks show statistically significant overhead
Remaining 15/27 show no significant difference
```

### Threat 3: Tool Comparison Fairness

**Description**: Comparison with Cppcheck may not be fair if tools have different goals.

**Mitigation**:
- **Similar scope**: Both detect integer overflow issues
- **Same test inputs**: Identical test cases for both
- **Multiple metrics**: Precision, recall, F1 all reported
- **Acknowledged differences**: Cppcheck has zero FP, OptiWeave has higher recall

---

## Summary of Limitations

### Acknowledged Limitations

1. **Synthetic test suite**: Not validated against real CVE database
2. **Single baseline tool**: Only compared against Cppcheck, not Clang Static Analyzer or Infer
3. **Self-authored ground truth**: No external validation
4. **Limited platforms**: Only Linux x86-64 tested
5. **C++ edge cases**: Some template patterns cause transformation issues

### Strengths

1. **Multiple benchmark suites**: Polybench + use cases + real libraries
2. **Statistical rigor**: CIs, p-values, effect sizes
3. **Transparent methodology**: All scripts and data available
4. **Real-world validation**: json-c and cJSON case studies
5. **Multiple metrics**: Overhead, precision, recall, speedup

---

## Recommendations for Future Work

1. **External replication**: Independent reproduction of results
2. **NIST Juliet evaluation**: Standard bug detection benchmark
3. **User study**: Developer experience evaluation
4. **Cross-platform testing**: macOS, Windows, ARM
5. **Comparison with more tools**: Clang Static Analyzer, Infer, Coverity

---

*Generated: 2026-01-31*
*OptiWeave Evaluation v1.0*
