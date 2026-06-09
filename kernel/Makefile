# WebOS Kernel Makefile

CC = clang
CFLAGS = -target wasm32 -nostdlib -Wall -Wextra -O2 \
         -I../libs

LDFLAGS = -Wl,--no-entry -Wl,--export=main \
          -Wl,--export=syscall_dispatch

OUTPUT_DIR = ../public/wasm

KERNEL_SRCS = src/main.c src/memory.c src/process.c src/scheduler.c \
              src/ipc.c src/syscall.c src/syscall_dispatch.c src/dynlink.c

all: $(OUTPUT_DIR)/kernel.wasm

$(OUTPUT_DIR)/kernel.wasm: $(KERNEL_SRCS)
	@mkdir -p $(OUTPUT_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(KERNEL_SRCS)
	python3 ../tools/add_module_section.py $@ wli

clean:
	rm -f $(OUTPUT_DIR)/kernel.wasm
