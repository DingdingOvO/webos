CC = emcc
CFLAGS = -s WASM=1 -s SIDE_MODULE=1 -s EXPORTED_FUNCTIONS='["_gpu_driver_init","_gpu_driver_create_surface","_gpu_driver_draw_rect","_gpu_driver_draw_text","_gpu_driver_clear","_gpu_driver_present","_gpu_driver_destroy_surface","_gpu_driver_get_screen_width","_gpu_driver_get_screen_height","_gpu_driver_get_default_surface"]' -O2 -Wall
TARGET = ../../public/wasm/gpu_driver.Wdll

all: $(TARGET)

$(TARGET): gpu_driver.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@
	python3 ../../tools/add_module_section.py $@ Wdll
	python3 ../../tools/gen_wli.py $@

clean:
	rm -f $(TARGET) ../../public/wasm/gpu_driver.wli
