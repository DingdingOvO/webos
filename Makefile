# WebOS Makefile v0.0.b
# Complete build, test, and packaging system

VERSION = 0.0.b

.PHONY: all clean host bootloader kernel drivers services apps tools docs serve \
        test c-tests py-tests ts-tests iso release check-env

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

# Website
site:
	@echo "[Site] Building website..."
	cd site && npm install && npm run build

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
	cd tests && make clean 2>/dev/null || true
	cd host && npm run clean 2>/dev/null || true
	cd desktop && npm run clean 2>/dev/null || true
	rm -rf public/wasm/*.wasm public/wasm/*.wex public/wasm/*.Wdll public/wasm/*.wli
	rm -rf public/webos-host.js public/index.html
	rm -rf build dist
	@echo "Clean complete."

# Docs (just list)
docs:
	@echo "[Docs] Documentation is in docs/ directory"
	@ls -1 docs/*.md
