# WebOS Makefile v0.0.1beta
# Complete build, test, and packaging system

VERSION = 0.0.1beta

.PHONY: all clean host bootloader kernel drivers services apps tools docs serve \
        test c-tests py-tests ts-tests iso release check-env \
        site-build site-dev lint test-report help

# ═══════════════════════════════════════════════════════
#  Build targets
# ═══════════════════════════════════════════════════════

# Default: build everything
all: check-env tools bootloader kernel drivers services apps host

# Verify build environment
check-env:
	@echo "═══════════════════════════════════════════"
	@echo "  WebOS v$(VERSION) Build System"
	@echo "═══════════════════════════════════════════"
	@echo -n "  Emscripten: "
	@which emcc > /dev/null 2>&1 && emcc --version | head -1 || echo "NOT FOUND (required for WASM build)"
	@echo -n "  GCC: "
	@which gcc > /dev/null 2>&1 && gcc --version | head -1 || echo "NOT FOUND (required for C tests)"
	@echo -n "  Node.js: "
	@which node > /dev/null 2>&1 && node --version || echo "NOT FOUND (required for host build)"
	@echo -n "  Python 3: "
	@which python3 > /dev/null 2>&1 && python3 --version || echo "NOT FOUND (required for tools)"
	@echo ""

# Tools
tools:
	@echo "[Tools] Preparing helper tools..."
	@chmod +x tools/add_module_section.py tools/gen_wli.py tools/serve.py tools/build_iso.sh 2>/dev/null || true

# TypeScript Host
host:
	@echo "[Host] Building TypeScript host..."
	cd host && npm install && npm run build

# Desktop Environment
desktop:
	@echo "[Desktop] Building desktop environment..."
	cd desktop && npm install && npm run build

# Bootloader
bootloader: tools
	@echo "[Bootloader] Building C bootloader..."
	cd bootloader && make

# Kernel
kernel: tools
	@echo "[Kernel] Building C kernel..."
	cd kernel && make

# Drivers
drivers: tools kernel
	@echo "[Drivers] Building device drivers..."
	cd drivers && make

# Services
services: tools kernel drivers
	@echo "[Services] Building system services..."
	cd services && make

# Apps
apps: tools services
	@echo "[Apps] Building applications..."
	cd apps && make

# Website build (production)
site-build:
	@echo "[Site] Building website for production..."
	cd site && npm install && npm run build

# Website development server
site-dev:
	@echo "[Site] Starting website development server..."
	cd site && npm install && npm run dev

# ═══════════════════════════════════════════════════════
#  Lint targets
# ═══════════════════════════════════════════════════════

lint:
	@echo "[Lint] Checking C/C++ formatting..."
	@find kernel bootloader drivers services apps libs -name '*.c' -o -name '*.h' -o -name '*.cpp' 2>/dev/null | \
		xargs clang-format --dry-run --Werror 2>&1 || echo "[Lint] C/C++ formatting issues found (non-blocking)"
	@echo "[Lint] Checking TypeScript formatting..."
	@find host desktop site -name '*.ts' -o -name '*.tsx' 2>/dev/null | \
		xargs prettier --check 2>&1 || echo "[Lint] TypeScript formatting issues found (non-blocking)"
	@echo "[Lint] Checking Python formatting..."
	@pip install black -q 2>/dev/null
	@find tools tests -name '*.py' 2>/dev/null | xargs black --check 2>&1 || echo "[Lint] Python formatting issues found (non-blocking)"
	@echo "[Lint] Lint check complete."

# ═══════════════════════════════════════════════════════
#  Test targets
# ═══════════════════════════════════════════════════════

test: c-tests py-tests ts-tests
	@echo ""
	@echo "═══════════════════════════════════════════"
	@echo "  All tests completed!"
	@echo "═══════════════════════════════════════════"

c-tests:
	@echo "[Test] Running C unit tests..."
	cd tests && make c-tests

py-tests:
	@echo "[Test] Running Python tool tests..."
	cd tests && make py-tests

ts-tests:
	@echo "[Test] Running TypeScript integration tests..."
	cd tests && make ts-tests

# Generate test report
test-report: test
	@echo ""
	@echo "═══════════════════════════════════════════"
	@echo "  WebOS v$(VERSION) Test Report"
	@echo "═══════════════════════════════════════════"
	@echo "  Date: $$(date -u)"
	@echo "  Version: $(VERSION)"
	@echo "  C Tests: Completed"
	@echo "  Python Tests: Completed"
	@echo "  TypeScript Tests: Completed"
	@echo "═══════════════════════════════════════════"
	@mkdir -p dist
	@echo "WebOS v$(VERSION) Test Report" > dist/test-report.txt
	@echo "Generated: $$(date -u)" >> dist/test-report.txt
	@echo "" >> dist/test-report.txt
	@echo "C Unit Tests: Passed" >> dist/test-report.txt
	@echo "Python Tool Tests: Passed" >> dist/test-report.txt
	@echo "TypeScript Integration Tests: Passed" >> dist/test-report.txt
	@echo "[Report] Test report saved to dist/test-report.txt"

# ═══════════════════════════════════════════════════════
#  Packaging targets
# ═══════════════════════════════════════════════════════

# ISO build (bootable memory OS)
iso: all
	@echo "[ISO] Building bootable ISO image..."
	chmod +x tools/build_iso.sh 2>/dev/null || true
	./tools/build_iso.sh dist/webos-$(VERSION).iso

# Release package
release: all test iso
	@echo "[Release] Creating release package..."
	mkdir -p dist
	@echo "  WebOS v$(VERSION)" > dist/RELEASE.txt
	@echo "  Built: $$(date)" >> dist/RELEASE.txt
	tar czf dist/webos-$(VERSION).tar.gz \
		--exclude='node_modules' \
		--exclude='.git' \
		--exclude='dist' \
		.
	@echo "  Release: dist/webos-$(VERSION).tar.gz"

# ═══════════════════════════════════════════════════════
#  Development targets
# ═══════════════════════════════════════════════════════

# Serve in browser
serve:
	@echo "[Serve] Starting development server on http://localhost:8080"
	python3 tools/serve.py 8080

# Clean everything
clean:
	cd bootloader && make clean 2>/dev/null || true
	cd kernel && make clean 2>/dev/null || true
	cd drivers && make clean 2>/dev/null || true
	cd services && make clean 2>/dev/null || true
	cd apps && make clean 2>/dev/null || true
	cd libs && make clean 2>/dev/null || true
	cd tests && make clean 2>/dev/null || true
	cd host && npm run clean 2>/dev/null || true
	cd desktop && npm run clean 2>/dev/null || true
	cd site && npm run clean 2>/dev/null || true
	rm -rf public/wasm/*.wasm public/wasm/*.wex public/wasm/*.Wdll public/wasm/*.wli
	rm -rf public/webos-host.js public/index.html
	rm -rf build dist
	@echo "Clean complete."

# Docs (just list)
docs:
	@echo "[Docs] Documentation is in docs/ directory"
	@ls -1 docs/*.md

# ═══════════════════════════════════════════════════════
#  Help
# ═══════════════════════════════════════════════════════

help:
	@echo "═══════════════════════════════════════════"
	@echo "  WebOS v$(VERSION) Build System"
	@echo "═══════════════════════════════════════════"
	@echo ""
	@echo "  Build Targets:"
	@echo "    all          Build everything (default)"
	@echo "    bootloader   Build C bootloader (WASM)"
	@echo "    kernel       Build C kernel (WASM)"
	@echo "    drivers      Build device drivers (WASM)"
	@echo "    services     Build system services (WASM)"
	@echo "    apps         Build applications (WASM)"
	@echo "    host         Build TypeScript host"
	@echo "    desktop      Build desktop environment"
	@echo "    site-build   Build website for production"
	@echo "    site-dev     Start website development server"
	@echo ""
	@echo "  Test Targets:"
	@echo "    test         Run all tests"
	@echo "    c-tests      Run C unit tests"
	@echo "    py-tests     Run Python tool tests"
	@echo "    ts-tests     Run TypeScript integration tests"
	@echo "    test-report  Run tests and generate report"
	@echo ""
	@echo "  Quality Targets:"
	@echo "    lint         Check code formatting (C, TS, Python)"
	@echo ""
	@echo "  Packaging Targets:"
	@echo "    iso          Build bootable ISO image"
	@echo "    release      Create release package (build + test + ISO)"
	@echo ""
	@echo "  Utility Targets:"
	@echo "    check-env    Verify build environment"
	@echo "    serve        Start development server (port 8080)"
	@echo "    clean        Remove all build artifacts"
	@echo "    docs         List documentation files"
	@echo "    help         Show this help message"
	@echo ""
	@echo "═══════════════════════════════════════════"
