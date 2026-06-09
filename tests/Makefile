# WebOS Test Suite Makefile

CC = gcc
CFLAGS = -Wall -Wextra -g -I. -I../kernel/src -Wno-unused-parameter -Wno-pointer-to-int-cast
CFLAGS_STUBS = -Wall -Wextra -g -I. -I../kernel/src -Istubs -I../drivers/gpu -I../drivers/input \
	       -I../drivers/storage -I../drivers/network -I../services/window_manager \
	       -I../services/filesystem -I../services/network_service -I../services/appstore \
	       -Wno-unused-parameter -Wno-pointer-to-int-cast
PYTHON = python3

# Original C tests
C_TESTS = test_memory test_process test_ipc test_scheduler test_syscall test_dynlink

# Extended C tests
C_EXTENDED = test_memory_extended test_process_extended test_ipc_extended \
	     test_syscall_extended test_scheduler_extended test_dynlink_extended

# Integration kernel test
C_INTEGRATION = test_integration_kernel

# Driver and service stub tests
C_STUBS = test_driver_stubs test_services

# All C tests
ALL_C_TESTS = $(C_TESTS) $(C_EXTENDED) $(C_INTEGRATION) $(C_STUBS)

# Python tests
PY_TESTS = test_add_module_section.py test_gen_wli.py

# TypeScript tests
TS_TESTS = integration/test_boot_sequence.ts integration/test_dynamic_loader.ts \
	   integration/test_runtime_bridge.ts integration/test_types.ts

.PHONY: all c-tests py-tests ts-tests clean original extended stubs

all: c-tests py-tests

c-tests: original extended integration-kernel stubs

original: $(C_TESTS)
	@echo ""
	@echo "═══════════════════════════════════════════"
	@echo "  Running original C unit tests..."
	@echo "═══════════════════════════════════════════"
	@failed=0; \
	for test in $(C_TESTS); do \
	        echo ""; \
	        ./$$test || failed=$$((failed + 1)); \
	done; \
	echo ""; \
	if [ $$failed -eq 0 ]; then \
	        echo "✓ All original C tests passed!"; \
	else \
	        echo "✗ $$failed test suite(s) failed!"; \
	        exit 1; \
	fi

extended: $(C_EXTENDED)
	@echo ""
	@echo "═══════════════════════════════════════════"
	@echo "  Running extended C unit tests..."
	@echo "═══════════════════════════════════════════"
	@failed=0; \
	for test in $(C_EXTENDED); do \
	        echo ""; \
	        ./$$test || failed=$$((failed + 1)); \
	done; \
	echo ""; \
	if [ $$failed -eq 0 ]; then \
	        echo "✓ All extended C tests passed!"; \
	else \
	        echo "✗ $$failed test suite(s) failed!"; \
	        exit 1; \
	fi

integration-kernel: $(C_INTEGRATION)
	@echo ""
	@echo "═══════════════════════════════════════════"
	@echo "  Running C integration tests..."
	@echo "═══════════════════════════════════════════"
	@failed=0; \
	for test in $(C_INTEGRATION); do \
	        echo ""; \
	        ./$$test || failed=$$((failed + 1)); \
	done; \
	echo ""; \
	if [ $$failed -eq 0 ]; then \
	        echo "✓ All C integration tests passed!"; \
	else \
	        echo "✗ $$failed test suite(s) failed!"; \
	        exit 1; \
	fi

stubs: $(C_STUBS)
	@echo ""
	@echo "═══════════════════════════════════════════"
	@echo "  Running driver/service stub tests..."
	@echo "═══════════════════════════════════════════"
	@failed=0; \
	for test in $(C_STUBS); do \
	        echo ""; \
	        ./$$test || failed=$$((failed + 1)); \
	done; \
	echo ""; \
	if [ $$failed -eq 0 ]; then \
	        echo "✓ All driver/service stub tests passed!"; \
	else \
	        echo "✗ $$failed test suite(s) failed!"; \
	        exit 1; \
	fi

# ---- Original test build rules ----

test_memory: test_memory.c ../kernel/src/memory.c ../kernel/src/memory.h test_framework.h
	$(CC) $(CFLAGS) -o $@ test_memory.c

test_process: test_process.c ../kernel/src/process.c ../kernel/src/process.h test_framework.h
	$(CC) $(CFLAGS) -o $@ test_process.c

test_ipc: test_ipc.c ../kernel/src/process.c ../kernel/src/ipc.c ../kernel/src/ipc.h ../kernel/src/scheduler.h test_framework.h
	$(CC) $(CFLAGS) -o $@ test_ipc.c

test_scheduler: test_scheduler.c ../kernel/src/process.c ../kernel/src/scheduler.c ../kernel/src/scheduler.h ../kernel/src/process.h test_framework.h
	$(CC) $(CFLAGS) -o $@ test_scheduler.c

test_syscall: test_syscall.c ../kernel/src/process.c ../kernel/src/ipc.c ../kernel/src/syscall.c ../kernel/src/scheduler.h test_framework.h
	$(CC) $(CFLAGS) -o $@ test_syscall.c

test_dynlink: test_dynlink.c ../kernel/src/dynlink.c ../kernel/src/dynlink.h test_framework.h
	$(CC) $(CFLAGS) -o $@ test_dynlink.c

# ---- Extended test build rules ----

test_memory_extended: test_memory_extended.c ../kernel/src/memory.c ../kernel/src/memory.h test_framework.h
	$(CC) $(CFLAGS) -o $@ test_memory_extended.c

test_process_extended: test_process_extended.c ../kernel/src/process.c ../kernel/src/process.h test_framework.h
	$(CC) $(CFLAGS) -o $@ test_process_extended.c

test_ipc_extended: test_ipc_extended.c ../kernel/src/process.c ../kernel/src/ipc.c ../kernel/src/ipc.h ../kernel/src/scheduler.h test_framework.h
	$(CC) $(CFLAGS) -o $@ test_ipc_extended.c

test_syscall_extended: test_syscall_extended.c ../kernel/src/process.c ../kernel/src/ipc.c ../kernel/src/syscall.c ../kernel/src/scheduler.h test_framework.h
	$(CC) $(CFLAGS) -o $@ test_syscall_extended.c

test_scheduler_extended: test_scheduler_extended.c ../kernel/src/process.c ../kernel/src/scheduler.c ../kernel/src/scheduler.h ../kernel/src/process.h test_framework.h
	$(CC) $(CFLAGS) -o $@ test_scheduler_extended.c

test_dynlink_extended: test_dynlink_extended.c ../kernel/src/dynlink.c ../kernel/src/dynlink.h test_framework.h
	$(CC) $(CFLAGS) -o $@ test_dynlink_extended.c

# ---- Integration kernel test build rule ----

test_integration_kernel: test_integration_kernel.c ../kernel/src/memory.c ../kernel/src/memory.h \
	                 ../kernel/src/process.c ../kernel/src/process.h \
	                 ../kernel/src/ipc.c ../kernel/src/ipc.h ../kernel/src/scheduler.h \
	                 ../kernel/src/scheduler.c ../kernel/src/scheduler.h \
	                 ../kernel/src/syscall.c ../kernel/src/syscall.h \
	                 ../kernel/src/dynlink.c ../kernel/src/dynlink.h test_framework.h
	$(CC) $(CFLAGS) -o $@ test_integration_kernel.c

# ---- Driver/Service stub test build rules ----

test_driver_stubs: test_driver_stubs.c \
	           ../drivers/gpu/gpu_driver.c ../drivers/gpu/gpu_driver.h \
	           ../drivers/input/input_driver.c ../drivers/input/input_driver.h \
	           ../drivers/storage/storage_driver.c ../drivers/storage/storage_driver.h \
	           ../drivers/network/network_driver.c ../drivers/network/network_driver.h \
	           stubs/emscripten.h test_framework.h
	$(CC) $(CFLAGS_STUBS) -o $@ test_driver_stubs.c

test_services: test_services.c \
	       ../services/window_manager/wm.c ../services/window_manager/wm.h \
	       ../services/filesystem/fs_service.c ../services/filesystem/fs_service.h \
	       ../services/network_service/net_service.c ../services/network_service/net_service.h \
	       ../services/appstore/appstore.c ../services/appstore/appstore.h \
	       stubs/emscripten.h test_framework.h
	$(CC) $(CFLAGS_STUBS) -o $@ test_services.c

# ---- Python tests ----

py-tests:
	@echo ""
	@echo "═══════════════════════════════════════════"
	@echo "  Running Python tool tests..."
	@echo "═══════════════════════════════════════════"
	@failed=0; \
	for test in $(PY_TESTS); do \
	        echo ""; \
	        $(PYTHON) $$test || failed=$$((failed + 1)); \
	done; \
	if [ $$failed -ne 0 ]; then \
	        echo "✗ $$failed Python test(s) failed!"; \
	        exit 1; \
	fi

# ---- TypeScript tests ----

ts-tests:
	@echo ""
	@echo "═══════════════════════════════════════════"
	@echo "  Running TypeScript integration tests..."
	@echo "═══════════════════════════════════════════"
	@failed=0; \
	for test in $(TS_TESTS); do \
	        echo ""; \
	        cd integration && npx tsx $$(basename $$test) || failed=$$((failed + 1)); \
	        cd ..; \
	done; \
	if [ $$failed -ne 0 ]; then \
	        echo "✗ $$failed TypeScript test(s) failed!"; \
	        exit 1; \
	fi

# ---- Clean ----

clean:
	rm -f $(ALL_C_TESTS) *.o
