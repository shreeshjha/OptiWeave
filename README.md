# OptiWeave

Modern C++ source-to-source transformation tool for operator instrumentation.

## Requirements

**LLVM/Clang Version Support:** OptiWeave is designed for LLVM 13.x - 17.x

- **LLVM:** 13.0 - 17.x (**REQUIRED**)
- **Clang:** 13.0 - 17.x (**REQUIRED**)
- **CMake:** 3.20+ 
- **C++ Compiler:** C++20 compatible (GCC 10+ or Clang 13+)

### Supported LLVM Versions

| LLVM Version | Status | Notes |
|--------------|--------|-------|
| 13.x | ✅ Supported | Minimum supported version |
| 14.x | ✅ Supported | Fully tested |
| 15.x | ✅ Supported | Recommended |
| 16.x | ✅ Supported | Fully tested |
| 17.x | ✅ Supported | Latest supported |
| 18.x+ | ❌ Not supported | API changes require code updates |

## Installation

### Installing Compatible LLVM/Clang

#### Ubuntu/Debian
```bash
# Install LLVM 17 (recommended)
sudo apt update
sudo apt install llvm-17-dev clang-17-dev libclang-17-dev

# Or install LLVM 15
sudo apt install llvm-15-dev clang-15-dev libclang-15-dev
```

#### macOS
```bash
# Install LLVM 17 (recommended)
brew install llvm@17

# Or install LLVM 15
brew install llvm@15

# Add to PATH (for LLVM 17)
echo 'export PATH="/opt/homebrew/opt/llvm@17/bin:$PATH"' >> ~/.zshrc
```

#### Arch Linux
```bash
# Install LLVM 17
sudo pacman -S llvm17 clang17

# Or install LLVM 15
sudo pacman -S llvm15 clang15
```

### Building OptiWeave

```bash
# Clone the repository
git clone https://github.com/yourusername/optiweave.git
cd optiweave

# Build with automatic dependency checking
./scripts/build.sh

# Or build manually
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### Troubleshooting Installation

If you encounter version-related errors:

1. **Check your LLVM version:**
   ```bash
   llvm-config --version
   clang --version
   ```

2. **If you have multiple LLVM versions installed:**
   ```bash
   # Specify the LLVM installation directory
   export LLVM_DIR=/usr/lib/llvm-17/lib/cmake/llvm
   ./scripts/build.sh
   ```

3. **If you have LLVM 18+ and can't downgrade:**
   
   OptiWeave currently supports LLVM 13-17. For LLVM 18+ support, please:
   - Check for newer OptiWeave releases
   - Consider using Docker with a compatible LLVM version
   - See the [Compatibility Guide](docs/compatibility.md) for migration information

## Usage

### Basic Usage
```bash
# Transform a single file
optiweave source.cpp -- -std=c++20

# Transform with specific operator types
optiweave --arithmetic-ops --assignment-ops source.cpp -- -std=c++20

# Use custom prelude and output directory
optiweave --prelude=my_prelude.hpp --output-dir=./transformed source.cpp --

# Transform entire project with compilation database
optiweave $(find src -name "*.cpp") --
```

### Advanced Usage
```bash
# Dry run to check what would be transformed
optiweave --dry-run --stats --verbose source.cpp --

# Transform with custom include paths
optiweave source.cpp -- -std=c++20 -I/path/to/headers
```

## Docker Usage (Recommended for LLVM 18+ users)

If you have incompatible LLVM versions, use Docker:

```bash
# Build Docker image with compatible LLVM
docker build -t optiweave .

# Run OptiWeave in container
docker run -v $(pwd):/workspace optiweave source.cpp -- -std=c++20
```

## Documentation

- [Architecture Overview](docs/architecture.md)
- [Transformation Guide](docs/transformations.md)
- [API Reference](docs/api.md)
- [LLVM Compatibility Guide](docs/compatibility.md)

## Examples

See the `examples/` directory for complete usage examples:

- `basic_transformation/` - Simple array subscript transformations
- `template_handling/` - Working with C++ templates
- `custom_operators/` - Custom operator overloads

## Contributing

1. Ensure you have a compatible LLVM version (13-17)
2. Fork the repository
3. Create a feature branch
4. Make your changes
5. Run tests: `./scripts/test.sh`
6. Submit a pull request

## License

MIT License - see [LICENSE](LICENSE) file for details.

## Version History

- **v1.0.0**: Initial release supporting LLVM 13-17
- Support for LLVM 18+ planned for future releases

## Support

- **Issues**: [GitHub Issues](https://github.com/yourusername/optiweave/issues)
- **Discussions**: [GitHub Discussions](https://github.com/yourusername/optiweave/discussions)
- **Documentation**: [Wiki](https://github.com/yourusername/optiweave/wiki)