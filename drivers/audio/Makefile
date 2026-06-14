CC = emcc
CFLAGS = -s WASM=1 -s SIDE_MODULE=1 -s EXPORTED_FUNCTIONS='["_audio_driver_init","_audio_driver_shutdown","_audio_driver_write","_audio_driver_position","_audio_driver_set_master_volume","_audio_driver_get_master_volume","_audio_driver_is_ready"]' -O2 -Wall
TARGET = ../../public/wasm/audio_driver.Wdll

all: $(TARGET)

$(TARGET): audio_driver.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@
	python3 ../../tools/add_module_section.py $@ Wdll
	python3 ../../tools/gen_wli.py $@

clean:
	rm -f $(TARGET) ../../public/wasm/audio_driver.wli
