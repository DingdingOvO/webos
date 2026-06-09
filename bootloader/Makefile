# WebOS Bootloader Makefile

CC = clang
CFLAGS = -target wasm32 -nostdlib -Wall -Wextra -O2 \
         -I../libs

LDFLAGS = -Wl,--no-entry -Wl,--export=main \
          -Wl,--export=get_boot_stage

OUTPUT_DIR = ../public/wasm

BOOTLOADER_SRCS = src/main.c src/loader.c src/gpu_init.c

all: $(OUTPUT_DIR)/bootloader.wasm

$(OUTPUT_DIR)/bootloader.wasm: $(BOOTLOADER_SRCS)
	@mkdir -p $(OUTPUT_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(BOOTLOADER_SRCS)
	python3 ../tools/add_module_section.py $@ wli

clean:
	rm -f $(OUTPUT_DIR)/bootloader.wasm
