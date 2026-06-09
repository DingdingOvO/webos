# WebOS Test Suite Makefile

CC = gcc
CFLAGS = -Wall -Wextra -g -I. -I../kernel/src -Wno-unused-parameter -Wno-pointer-to-int-cast
PYTHON = python3

C_TESTS = test_memory test_process test_ipc test_scheduler test_syscall test_dynlink

.PHONY: all c-tests py-tests ts-tests clean

all: c-tests py-tests

c-tests: $(C_TESTS)
	@echo ""
	@echo "═══════════════════════════════════════════"
	@echo "  Running C unit tests..."
	@echo "═══════════════════════════════════════════"
	@failed=0; \
	for test in $(C_TESTS); do \
		echo ""; \
		./$$test || failed=$$((failed + 1)); \
	done; \
	echo ""; \
	if [ $$failed -eq 0 ]; then \
		echo "✓ All C tests passed!"; \
	else \
		echo "✗ $$failed test suite(s) failed!"; \
		exit 1; \
	fi

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

py-tests:
	@echo ""
	@echo "═══════════════════════════════════════════"
	@echo "  Running Python tool tests..."
	@echo "═══════════════════════════════════════════"
	$(PYTHON) test_add_module_section.py
	$(PYTHON) test_gen_wli.py

ts-tests:
	@echo ""
	@echo "═══════════════════════════════════════════"
	@echo "  Running TypeScript integration tests..."
	@echo "═══════════════════════════════════════════"
	cd integration && npx tsx test_boot_sequence.ts

clean:
	rm -f $(C_TESTS) *.o
