# Contributing to WebOS

Thank you for your interest in contributing to WebOS! This guide will help you get started with development, testing, and submitting your changes.

## Table of Contents

- [Development Setup](#development-setup)
- [Build Instructions](#build-instructions)
- [Test Instructions](#test-instructions)
- [Code Style Guidelines](#code-style-guidelines)
- [PR Process](#pr-process)
- [Commit Message Format](#commit-message-format)

---

## Development Setup

### Prerequisites

| Tool | Version | Purpose |
|------|---------|---------|
| Emscripten (emcc) | 3.1.56 | Compiling C/C++ to WebAssembly |
| GCC | 10+ | Compiling C tests natively |
| Node.js | 22 | TypeScript host & website builds |
| Python | 3.12+ | Build tools and scripts |
| Make | GNU | Build orchestration |

### Installing Emscripten

```bash
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install 3.1.56
./emsdk activate 3.1.56
source ./emsdk_env.sh
```

### Cloning the Repository

```bash
git clone https://github.com/your-org/webos.git
cd webos
```

### Initial Setup

```bash
# Verify your environment
make check-env

# Build everything
make all
```

---

## Build Instructions

### Full Build

```bash
make all
```

This builds all WASM modules (bootloader, kernel, drivers, services, apps) and the TypeScript host.

### Individual Components

```bash
# WASM modules
make bootloader
make kernel
make drivers
make services
make apps

# TypeScript
make host
make desktop

# Website
make site-build
```

### Development Server

```bash
# Start the WebOS dev server (serves public/ on port 8080)
make serve

# Start the website dev server (Next.js with Turbopack)
make site-dev
```

### ISO Image

```bash
make iso
```

### Clean Build Artifacts

```bash
make clean
```

---

## Test Instructions

### Run All Tests

```bash
make test
```

### Run Individual Test Suites

```bash
# C unit tests (kernel, IPC, scheduler, etc.)
make c-tests

# Python tool tests (gen_wli, add_module_section)
make py-tests

# TypeScript integration tests
make ts-tests
```

### Generate Test Report

```bash
make test-report
```

This runs all tests and writes a summary to `dist/test-report.txt`.

### Running Tests Manually

```bash
# C tests
cd tests
make c-tests

# Python tests
cd tests
make py-tests

# TypeScript integration tests
cd tests
npm install
make ts-tests
```

---

## Code Style Guidelines

### C/C++ Code

We use `clang-format` with a project-wide `.clang-format` configuration:

- **Style base**: LLVM
- **Indent width**: 4 spaces (no tabs)
- **Brace style**: Allman
- **Column limit**: 100 characters
- **Short functions**: Not on a single line
- **Short if statements**: Not on a single line

Check formatting:

```bash
# Dry-run check (errors on formatting issues)
find kernel bootloader drivers services apps libs -name '*.c' -o -name '*.h' -o -name '*.cpp' | \
  xargs clang-format --dry-run --Werror

# Auto-format
find kernel bootloader drivers services apps libs -name '*.c' -o -name '*.h' -o -name '*.cpp' | \
  xargs clang-format -i
```

### TypeScript/TSX Code

We use Prettier with the following settings (`.prettierrc`):

- **Semicolons**: Always
- **Quotes**: Single
- **Trailing commas**: ES5
- **Print width**: 100 characters
- **Tab width**: 2 spaces

Check formatting:

```bash
# Check
find host desktop site -name '*.ts' -o -name '*.tsx' | xargs prettier --check

# Auto-format
find host desktop site -name '*.ts' -o -name '*.tsx' | xargs prettier --write
```

### Python Code

We use `black` for Python formatting:

```bash
# Check
find tools tests -name '*.py' | xargs black --check

# Auto-format
find tools tests -name '*.py' | xargs black
```

### Lint All Code

```bash
make lint
```

---

## PR Process

### Before Submitting a PR

1. **Create a branch** from `main`:
   ```bash
   git checkout -b feature/my-feature
   ```

2. **Make your changes** and ensure they build:
   ```bash
   make all
   ```

3. **Run all tests**:
   ```bash
   make test
   ```

4. **Check code formatting**:
   ```bash
   make lint
   ```

5. **Commit your changes** using the commit message format below.

6. **Push and open a PR** against the `main` branch.

### PR Requirements

- All CI checks must pass (lint, C tests, Python tests, WASM builds, TypeScript builds, integration tests)
- Code must be properly formatted
- New features should include tests
- Breaking changes must be documented

### PR Review

- At least one approval is required
- CI must pass on all platforms
- Reviewers will check for code quality, test coverage, and documentation

---

## Commit Message Format

We follow the [Conventional Commits](https://www.conventionalcommits.org/) specification:

```
<type>(<scope>): <description>

[optional body]

[optional footer(s)]
```

### Types

| Type | Description |
|------|-------------|
| `feat` | New feature |
| `fix` | Bug fix |
| `docs` | Documentation changes |
| `style` | Code style changes (formatting, no logic change) |
| `refactor` | Code refactoring (no feature or fix) |
| `test` | Adding or updating tests |
| `build` | Build system or dependency changes |
| `ci` | CI/CD configuration changes |
| `chore` | Other changes that don't fit above |

### Scopes

| Scope | Component |
|-------|-----------|
| `kernel` | Kernel module |
| `bootloader` | Bootloader module |
| `drivers` | Device drivers |
| `services` | System services |
| `apps` | Applications |
| `host` | TypeScript host |
| `desktop` | Desktop environment |
| `site` | Website |
| `tools` | Build tools |
| `ci` | CI/CD pipeline |

### Examples

```
feat(kernel): add virtual memory mapping
fix(drivers): resolve GPU driver crash on init
docs(site): update API documentation for syscalls
test(kernel): add unit tests for IPC channel
build(ci): update Emscripten version to 3.1.56
ci(deploy): add GitHub Pages deployment workflow
refactor(services): extract common service interface
chore(deps): update Next.js to latest version
```

---

## Questions?

If you have questions about contributing, please open an issue with the `question` label and we'll be happy to help.

Thank you for contributing to WebOS!
