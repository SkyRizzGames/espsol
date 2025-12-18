# Contributing to ESPSOL

Thank you for your interest in contributing to ESPSOL! This document provides guidelines and instructions for contributing.

## Code of Conduct

By participating in this project, you agree to maintain a respectful and inclusive environment for everyone.

## How to Contribute

### Reporting Issues

- Check if the issue already exists
- Use the issue template if available
- Include ESP-IDF version, ESP32 variant, and steps to reproduce
- Attach relevant logs (use `idf.py monitor`)

### Submitting Pull Requests

1. **Fork the repository**
   ```bash
   git clone https://github.com/SkyRizzGames/espsol.git
   cd espsol
   ```

2. **Create a feature branch**
   ```bash
   git checkout -b feature/your-feature-name
   ```

3. **Make your changes**
   - Follow the coding style (see below)
   - Add tests for new functionality
   - Update documentation if needed

4. **Run tests**
   ```bash
   cd test/host
   ./run_tests.sh
   ```

5. **Commit with clear messages**
   ```bash
   git commit -m "feat: add new feature description"
   ```

6. **Push and create PR**
   ```bash
   git push origin feature/your-feature-name
   ```

## Coding Style

### General Rules

- Pure C (C99), no C++ features
- 4-space indentation
- 100-character line limit
- Use `esp_err_t` return types for all public functions

### Naming Conventions

| Type | Convention | Example |
|------|------------|---------|
| Functions | `espsol_module_action` | `espsol_tx_create()` |
| Types | `espsol_name_t` | `espsol_keypair_t` |
| Constants | `ESPSOL_CONSTANT_NAME` | `ESPSOL_PUBKEY_SIZE` |
| Enums | `ESPSOL_ENUM_VALUE` | `ESPSOL_COMMITMENT_CONFIRMED` |

### Error Handling

```c
esp_err_t espsol_function(args) {
    if (args == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_err_t ret = internal_operation();
    if (ret != ESP_OK) {
        return ret;
    }
    
    return ESP_OK;
}
```

### Documentation

- Use Doxygen-style comments for public APIs
- Document all parameters and return values

```c
/**
 * @brief Brief description
 *
 * @param[in]  input   Description of input parameter
 * @param[out] output  Description of output parameter
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if parameters are invalid
 */
esp_err_t espsol_function(const type_t *input, type_t *output);
```

## Project Structure

```
espsol/
├── components/espsol/
│   ├── include/          # Public headers
│   ├── priv_include/     # Internal headers
│   ├── src/              # Implementation
│   ├── CMakeLists.txt
│   └── Kconfig
├── examples/             # Example projects
├── test/
│   ├── host/             # Host-based unit tests
│   └── main/             # ESP32 integration tests
└── docs/                 # Documentation
```

## Testing

### Host Tests (no hardware required)

```bash
cd test/host
./run_tests.sh
```

### ESP32 Tests

```bash
cd test
idf.py set-target esp32c3
idf.py build flash monitor
```

### Adding Tests

- Add test functions to `test/host/test_*.c`
- Follow existing patterns
- Test both success and error cases

## Commit Message Format

Use conventional commits:

- `feat:` New feature
- `fix:` Bug fix
- `docs:` Documentation changes
- `test:` Test additions/changes
- `refactor:` Code refactoring
- `chore:` Maintenance tasks

Examples:
```
feat: add Token-2022 extension support
fix: handle RPC timeout correctly
docs: update API reference for token module
test: add tests for ATA derivation
```

## Development Setup

### Prerequisites

- ESP-IDF v5.0 or later
- GCC for host tests
- Git

### Building

```bash
# Source ESP-IDF
source ~/esp/v5.5.1/esp-idf/export.sh

# Build test project
cd test
idf.py set-target esp32c3
idf.py build
```

## Areas for Contribution

### Good First Issues

- Documentation improvements
- Additional examples
- Test coverage improvements
- Typo fixes

### Feature Ideas

- WebSocket subscription support
- Token-2022 extensions
- Versioned transactions (v0)
- Additional RPC methods

## License

By contributing, you agree that your contributions will be licensed under the Apache 2.0 license.

## Questions?

- Open a GitHub issue

Thank you for contributing! 🚀
