CC = emcc
CFLAGS = -s WASM=1 -s SIDE_MODULE=1 -s EXPORTED_FUNCTIONS='["_storage_driver_init","_storage_driver_read","_storage_driver_write","_storage_driver_exists","_storage_driver_delete","_storage_driver_mkdir"]' -O2 -Wall
TARGET = ../../public/wasm/storage_driver.Wdll

all: $(TARGET)

$(TARGET): storage_driver.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@
	python3 ../../tools/add_module_section.py $@ Wdll
	python3 ../../tools/gen_wli.py $@

clean:
	rm -f $(TARGET) ../../public/wasm/storage_driver.wli
