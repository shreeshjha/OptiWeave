# OptiWeave Commands Cheat Sheet

This guide shows how to build, run, and test OptiWeave locally.

## Prerequisites

- LLVM/Clang 13–17, CMake 3.20+, a C++20 compiler
- macOS (Homebrew):
  - `brew install llvm@17`
  - Optionally set: `export LLVM_DIR=/opt/homebrew/opt/llvm@17/lib/cmake/llvm` and `export Clang_DIR=/opt/homebrew/opt/llvm@17/lib/cmake/clang`
- Ubuntu/Debian:
  - `sudo apt update && sudo apt install llvm-17-dev clang-17-dev libclang-17-dev`
  - Optionally set: `export LLVM_DIR=/usr/lib/llvm-17/lib/cmake/llvm` and `export Clang_DIR=/usr/lib/llvm-17/lib/cmake/clang`

## Build

- Default build:
  - `./scripts/build.sh`
- With tests and verbose logs:
  - `./scripts/build.sh --tests --verbose`
- Manual CMake:
  - `mkdir build && cd build`
  - `cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON`
  - `make -j$(nproc)` or `ninja`

Artifacts: the CLI is typically at `./build/bin/optiweave` (verify with `ls build/bin`).

## Quick Test

1) Create a small program:

```
echo 'int main(){ int a[10]; return a[5]; }' > test.cpp
```

2) Transform and compile in one step:

```
./build/bin/optiweave test.cpp --compile -o instrumented
```

3) Run the instrumented binary:

```
./instrumented
```

## Common Workflows

- Single file → executable:
  - `./build/bin/optiweave source.cpp --compile -o app`

- Multiple files → executable:
  - `./build/bin/optiweave f1.cpp f2.cpp --compile -o app`

- Enable more operator types:
  - `./build/bin/optiweave source.cpp --arithmetic-ops --assignment-ops --comparison-ops --compile -o app`

- Transform only to a directory (no compilation):
  - `./build/bin/optiweave source.cpp --output-dir=./transformed --verbose`

- Manual compile after transform:
  - `clang++ -std=c++20 -I./templates ./transformed/source.cpp -L./build -loptiweave_runtime -o app`

- Preview without writing (dry run):
  - `./build/bin/optiweave source.cpp --dry-run --verbose`

## Examples

- Build examples (enabled by default):
  - `./scripts/build.sh`

- Instrument and build the example:
  - `./build/bin/optiweave examples/basic_transformation/example.cpp --compile -o example_instrumented`

- Run:
  - `./example_instrumented`

## Tests

- Build with tests:
  - `./scripts/build.sh --tests`

- Run all tests:
  - `cd build && ctest`
  - or `./scripts/test.sh`

- Filter by label:
  - `ctest -L unit`
  - `ctest -L integration`

- Verbose:
  - `ctest --verbose`

## Troubleshooting

- LLVM/Clang not found:
  - Set `LLVM_DIR` and `Clang_DIR` to the correct CMake package paths (see Prerequisites) and rerun `./scripts/build.sh`.

- Missing compile database:
  - The tool auto-generates `compile_commands.json` if absent, or use `--generate-compile-commands`.

- Templates/runtime not found during `--compile`:
  - Ensure you built the project (`optiweave_runtime` static lib and `templates` are used automatically by the tool in compile mode).

- Binary path differs:
  - Check `./build/bin/optiweave` and `./build/optiweave` with `ls` to locate the executable.

