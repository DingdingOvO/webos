CC = emcc
CFLAGS = -s WASM=1 -s SIDE_MODULE=1 -s EXPORTED_FUNCTIONS='["_network_driver_init","_network_driver_http_get","_network_driver_http_post"]' -O2 -Wall
TARGET = ../../public/wasm/network_driver.Wdll

all: $(TARGET)

$(TARGET): network_driver.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@
	python3 ../../tools/add_module_section.py $@ Wdll
	python3 ../../tools/gen_wli.py $@

clean:
	rm -f $(TARGET) ../../public/wasm/network_driver.wli
