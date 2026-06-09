CC = emcc
CFLAGS = -s WASM=1 -s SIDE_MODULE=1 -s EXPORTED_FUNCTIONS='["_input_driver_init","_input_driver_poll_event","_input_driver_push_event","_input_driver_has_events"]' -O2 -Wall
TARGET = ../../public/wasm/input_driver.Wdll

all: $(TARGET)

$(TARGET): input_driver.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@
	python3 ../../tools/add_module_section.py $@ Wdll
	python3 ../../tools/gen_wli.py $@

clean:
	rm -f $(TARGET) ../../public/wasm/input_driver.wli
