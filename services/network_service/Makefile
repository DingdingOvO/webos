CC = emcc
CFLAGS = -s WASM=1 -s SIDE_MODULE=1 -s EXPORTED_FUNCTIONS='["_net_service_init","_net_http_get","_net_http_post","_net_open_connection","_net_close_connection"]' -O2 -Wall
TARGET = ../../public/wasm/net_service.Wdll

all: $(TARGET)

$(TARGET): net_service.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@
	python3 ../../tools/add_module_section.py $@ Wdll
	python3 ../../tools/gen_wli.py $@

clean:
	rm -f $(TARGET) ../../public/wasm/net_service.wli
