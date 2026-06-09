CC = emcc
CFLAGS = -s WASM=1 -s SIDE_MODULE=1 -s EXPORTED_FUNCTIONS='["_appstore_init","_appstore_register","_appstore_get_count","_appstore_find_by_name","_appstore_install","_appstore_uninstall","_appstore_is_installed"]' -O2 -Wall
TARGET = ../../public/wasm/appstore_service.Wdll

all: $(TARGET)

$(TARGET): appstore.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@
	python3 ../../tools/add_module_section.py $@ Wdll
	python3 ../../tools/gen_wli.py $@

clean:
	rm -f $(TARGET) ../../public/wasm/appstore_service.wli
