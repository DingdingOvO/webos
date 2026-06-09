CC = emcc
CFLAGS = -s WASM=1 -s SIDE_MODULE=1 -s EXPORTED_FUNCTIONS='["_wm_service_init","_wm_create_window","_wm_destroy_window","_wm_show_window","_wm_hide_window","_wm_move_window","_wm_resize_window","_wm_set_focus","_wm_get_focused_window","_wm_get_window_count","_wm_render_all"]' -O2 -Wall
TARGET = ../../public/wasm/wm_service.Wdll

all: $(TARGET)

$(TARGET): wm.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@
	python3 ../../tools/add_module_section.py $@ Wdll
	python3 ../../tools/gen_wli.py $@

clean:
	rm -f $(TARGET) ../../public/wasm/wm_service.wli
