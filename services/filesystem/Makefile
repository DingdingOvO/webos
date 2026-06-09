CC = emcc
CFLAGS = -s WASM=1 -s SIDE_MODULE=1 -s EXPORTED_FUNCTIONS='["_fs_service_init","_fs_open","_fs_read","_fs_write","_fs_close","_fs_mkdir","_fs_unlink","_fs_exists"]' -O2 -Wall
TARGET = ../../public/wasm/fs_service.Wdll

all: $(TARGET)

$(TARGET): fs_service.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@
	python3 ../../tools/add_module_section.py $@ Wdll
	python3 ../../tools/gen_wli.py $@

clean:
	rm -f $(TARGET) ../../public/wasm/fs_service.wli
