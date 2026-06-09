# WebOS Top-level Makefile
# Supports recursive building of all modules

.PHONY: all clean host bootloader kernel drivers services apps tools docs serve

# Default target: build everything
all: tools bootloader kernel drivers services apps host

# Phase 0: Tools
tools:
	@echo "[Tools] Building helper tools..."
	chmod +x tools/add_module_section.py tools/gen_wli.py

# Phase 1: TypeScript Host
host:
	@echo "[Host] Building TypeScript host..."
	cd host && npm install && npm run build

# Phase 2: Bootloader
bootloader: tools
	@echo "[Bootloader] Building C bootloader..."
	cd bootloader && make

# Phase 3: Kernel
kernel: tools
	@echo "[Kernel] Building C kernel..."
	cd kernel && make

# Phase 4: Drivers
drivers: tools kernel
	@echo "[Drivers] Building device drivers..."
	cd drivers && make

# Phase 5: Services
services: tools kernel drivers
	@echo "[Services] Building system services..."
	cd services && make

# Phase 6: Apps
apps: tools services
	@echo "[Apps] Building applications..."
	cd apps && make

# Phase 8: Documentation
docs:
	@echo "[Docs] Documentation is in docs/ directory"

# Serve in browser
serve: all
	@echo "[Serve] Starting development server..."
	cd host && npm run serve

# Clean everything
clean:
	cd bootloader && make clean 2>/dev/null || true
	cd kernel && make clean 2>/dev/null || true
	cd drivers && make clean 2>/dev/null || true
	cd services && make clean 2>/dev/null || true
	cd apps && make clean 2>/dev/null || true
	cd host && npm run clean 2>/dev/null || true
	rm -rf public/wasm/*.wasm public/wasm/*.wex public/wasm/*.Wdll public/wasm/*.wli
	@echo "Clean complete."
