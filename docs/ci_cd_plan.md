# OptiWeave CI/CD Integration Plan

This document outlines the plan for integrating OptiWeave into CI/CD pipelines using GitHub Actions and GitLab CI.

## Overview

OptiWeave can be integrated into CI/CD pipelines to:
1. **Automated Testing**: Run unit and integration tests on every commit
2. **Performance Regression Detection**: Track overhead metrics over time
3. **Static Analysis Gates**: Block merges with potential issues
4. **Benchmark Comparisons**: Compare profiling overhead automatically
5. **Documentation Generation**: Auto-generate API docs

---

## Phase 1: Basic CI Pipeline (GitHub Actions)

### Priority: High
### Timeline: 1-2 weeks

### Goals
- Build OptiWeave on multiple platforms
- Run unit and integration tests
- Check code formatting

### Proposed Workflow: `.github/workflows/ci.yml`

```yaml
name: CI

on:
  push:
    branches: [main, develop]
  pull_request:
    branches: [main, develop]

jobs:
  build-linux:
    runs-on: ubuntu-22.04
    strategy:
      matrix:
        llvm: [14, 15, 16, 17]
    steps:
      - uses: actions/checkout@v4
      
      - name: Install LLVM ${{ matrix.llvm }}
        run: |
          wget https://apt.llvm.org/llvm.sh
          chmod +x llvm.sh
          sudo ./llvm.sh ${{ matrix.llvm }}
          
      - name: Configure
        run: |
          cmake -B build \
            -DCMAKE_BUILD_TYPE=Release \
            -DLLVM_DIR=/usr/lib/llvm-${{ matrix.llvm }}/lib/cmake/llvm \
            -DClang_DIR=/usr/lib/llvm-${{ matrix.llvm }}/lib/cmake/clang
            
      - name: Build
        run: cmake --build build -j$(nproc)
        
      - name: Test
        run: ctest --test-dir build --output-on-failure

  build-macos:
    runs-on: macos-13
    steps:
      - uses: actions/checkout@v4
      
      - name: Install LLVM
        run: brew install llvm@17
        
      - name: Configure
        run: |
          export PATH="/opt/homebrew/opt/llvm@17/bin:$PATH"
          cmake -B build -DCMAKE_BUILD_TYPE=Release
          
      - name: Build
        run: cmake --build build -j$(sysctl -n hw.ncpu)
        
      - name: Test
        run: ctest --test-dir build --output-on-failure

  lint:
    runs-on: ubuntu-22.04
    steps:
      - uses: actions/checkout@v4
      
      - name: Check formatting
        run: |
          find src include -name '*.cpp' -o -name '*.hpp' | \
            xargs clang-format --dry-run --Werror
```

---

## Phase 2: Performance Benchmarking (GitHub Actions)

### Priority: Medium
### Timeline: 2-3 weeks (after Phase 1)

### Goals
- Run overhead benchmarks on every PR
- Detect performance regressions
- Track metrics over time

### Proposed Workflow: `.github/workflows/benchmark.yml`

```yaml
name: Benchmark

on:
  pull_request:
    branches: [main]
  workflow_dispatch:

jobs:
  benchmark:
    runs-on: ubuntu-22.04
    steps:
      - uses: actions/checkout@v4
        with:
          fetch-depth: 0
          
      - name: Install dependencies
        run: sudo apt-get install -y llvm-17-dev libclang-17-dev valgrind
        
      - name: Build OptiWeave
        run: |
          cmake -B build -DCMAKE_BUILD_TYPE=Release
          cmake --build build -j$(nproc)
          
      - name: Run baseline comparison
        run: ./evaluation/baseline_comparison/run_baseline_comparison.sh
        
      - name: Upload results
        uses: actions/upload-artifact@v4
        with:
          name: benchmark-results
          path: evaluation/baseline_comparison/results/
          
      - name: Comment PR with results
        if: github.event_name == 'pull_request'
        uses: actions/github-script@v7
        with:
          script: |
            const fs = require('fs');
            const report = fs.readFileSync(
              'evaluation/baseline_comparison/results/comparison_report.md',
              'utf8'
            );
            github.rest.issues.createComment({
              owner: context.repo.owner,
              repo: context.repo.repo,
              issue_number: context.issue.number,
              body: '## Benchmark Results\n\n' + report
            });
```

---

## Phase 3: Static Analysis Integration

### Priority: Medium
### Timeline: 2-3 weeks (after Phase 2)

### Goals
- Run OptiWeave's static analysis on sample projects
- Compare with cppcheck, clang-tidy
- Generate quality gates

### Proposed Workflow: `.github/workflows/static-analysis.yml`

```yaml
name: Static Analysis

on:
  push:
    branches: [main, develop]

jobs:
  analyze:
    runs-on: ubuntu-22.04
    steps:
      - uses: actions/checkout@v4
      
      - name: Build OptiWeave
        run: |
          cmake -B build -DCMAKE_BUILD_TYPE=Release
          cmake --build build -j$(nproc)
          
      - name: Run OptiWeave analysis on examples
        run: |
          for file in examples/basic_transformation/*.cpp; do
            ./build/optiweave "$file" --analyze-only \
              --overflow-detection \
              --complexity-analysis \
              --pattern-detection
          done
          
      - name: Run cppcheck
        run: |
          sudo apt-get install -y cppcheck
          cppcheck --enable=all --error-exitcode=1 src/
          
      - name: Run clang-tidy
        run: |
          clang-tidy -p build src/*.cpp -- -std=c++20
```

---

## Phase 4: GitLab CI Integration

### Priority: Low
### Timeline: 1-2 weeks (after Phase 3)

### Proposed Configuration: `.gitlab-ci.yml`

```yaml
stages:
  - build
  - test
  - benchmark
  - deploy

variables:
  LLVM_VERSION: "17"

.llvm-setup: &llvm-setup
  before_script:
    - apt-get update && apt-get install -y wget lsb-release software-properties-common
    - wget https://apt.llvm.org/llvm.sh && chmod +x llvm.sh
    - ./llvm.sh ${LLVM_VERSION}

build:linux:
  stage: build
  image: ubuntu:22.04
  <<: *llvm-setup
  script:
    - cmake -B build -DCMAKE_BUILD_TYPE=Release
    - cmake --build build -j$(nproc)
  artifacts:
    paths:
      - build/
    expire_in: 1 hour

test:unit:
  stage: test
  image: ubuntu:22.04
  needs: [build:linux]
  <<: *llvm-setup
  script:
    - ctest --test-dir build --output-on-failure

test:integration:
  stage: test
  image: ubuntu:22.04
  needs: [build:linux]
  <<: *llvm-setup
  script:
    - ./scripts/test.sh

benchmark:overhead:
  stage: benchmark
  image: ubuntu:22.04
  needs: [build:linux]
  <<: *llvm-setup
  script:
    - apt-get install -y valgrind linux-tools-generic
    - ./evaluation/baseline_comparison/run_baseline_comparison.sh
  artifacts:
    paths:
      - evaluation/baseline_comparison/results/
    reports:
      metrics: evaluation/baseline_comparison/results/comparison_results.csv
  only:
    - main
    - merge_requests

pages:
  stage: deploy
  image: alpine:latest
  needs: [test:unit]
  script:
    - mkdir -p public
    - cp -r docs/* public/
    - cp -r web/* public/
  artifacts:
    paths:
      - public
  only:
    - main
```

---

## Phase 5: Advanced Features

### Priority: Low
### Timeline: Ongoing

### 5.1 Performance Trend Tracking

Store benchmark results and visualize trends over time:

```yaml
# Add to benchmark job
- name: Store metrics
  run: |
    mkdir -p metrics
    echo "$(date +%s),$(git rev-parse HEAD),$(cat results/overhead.txt)" \
      >> metrics/history.csv
      
- name: Generate trend chart
  uses: benchmark-action/github-action-benchmark@v1
  with:
    tool: 'customSmallerIsBetter'
    output-file-path: metrics/benchmark.json
```

### 5.2 Release Automation

```yaml
name: Release

on:
  push:
    tags:
      - 'v*'

jobs:
  release:
    runs-on: ubuntu-22.04
    steps:
      - uses: actions/checkout@v4
      
      - name: Build
        run: |
          cmake -B build -DCMAKE_BUILD_TYPE=Release
          cmake --build build
          
      - name: Package
        run: |
          cpack -G TGZ -B packages
          cpack -G DEB -B packages
          
      - name: Create Release
        uses: softprops/action-gh-release@v1
        with:
          files: packages/*
```

### 5.3 Docker Image Publishing

```yaml
name: Docker

on:
  push:
    branches: [main]
    tags: ['v*']

jobs:
  docker:
    runs-on: ubuntu-22.04
    steps:
      - uses: actions/checkout@v4
      
      - uses: docker/login-action@v3
        with:
          username: ${{ secrets.DOCKERHUB_USERNAME }}
          password: ${{ secrets.DOCKERHUB_TOKEN }}
          
      - uses: docker/build-push-action@v5
        with:
          push: true
          tags: optiweave/optiweave:latest
```

---

## Implementation Checklist

### Phase 1: Basic CI
- [ ] Create `.github/workflows/ci.yml`
- [ ] Test on Linux with LLVM 14-17
- [ ] Test on macOS
- [ ] Add code formatting check
- [ ] Set up branch protection rules

### Phase 2: Benchmarking
- [ ] Create `.github/workflows/benchmark.yml`
- [ ] Integrate baseline comparison script
- [ ] Add PR commenting with results
- [ ] Store artifacts

### Phase 3: Static Analysis
- [ ] Create `.github/workflows/static-analysis.yml`
- [ ] Run OptiWeave's own analysis
- [ ] Integrate cppcheck
- [ ] Add quality gates

### Phase 4: GitLab CI
- [ ] Create `.gitlab-ci.yml`
- [ ] Mirror pipeline structure
- [ ] Test with GitLab runners

### Phase 5: Advanced
- [ ] Performance trend tracking
- [ ] Release automation
- [ ] Docker image publishing
- [ ] Documentation deployment

---

## Required Secrets

| Secret | Platform | Description |
|--------|----------|-------------|
| `DOCKERHUB_USERNAME` | GitHub/GitLab | Docker Hub username |
| `DOCKERHUB_TOKEN` | GitHub/GitLab | Docker Hub access token |
| `CODECOV_TOKEN` | GitHub | Code coverage upload token |

---

## Estimated Timeline

| Phase | Duration | Dependencies |
|-------|----------|--------------|
| Phase 1 | 1-2 weeks | None |
| Phase 2 | 2-3 weeks | Phase 1 |
| Phase 3 | 2-3 weeks | Phase 1 |
| Phase 4 | 1-2 weeks | Phase 1-3 |
| Phase 5 | Ongoing | Phase 1-4 |

**Total estimated time**: 6-10 weeks for full implementation

---

## Success Metrics

1. **Build Success Rate**: > 95% on all platforms
2. **Test Coverage**: > 80% code coverage
3. **Performance Regression**: < 5% deviation from baseline
4. **CI Duration**: < 15 minutes for full pipeline
5. **False Positive Rate**: < 10% for static analysis

---

## Notes

- All workflow files should be tested in a feature branch before merging
- Consider using self-hosted runners for performance-sensitive benchmarks
- Cache LLVM installations to speed up builds
- Use matrix builds to test multiple LLVM versions efficiently

---

*Document created: $(date '+%Y-%m-%d')*
*Last updated: $(date '+%Y-%m-%d')*
