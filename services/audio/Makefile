CC = emcc
CFLAGS = -s WASM=1 -s SIDE_MODULE=1 -s EXPORTED_FUNCTIONS='["_audio_server_init","_audio_server_shutdown","_audio_stream_open","_audio_stream_close","_audio_stream_write","_audio_stream_set_volume","_audio_stream_get_volume","_audio_stream_pause","_audio_stream_resume","_audio_server_mix_and_output","_audio_server_get_master_volume","_audio_server_set_master_volume","_audio_server_get_stream_info","_audio_server_list_streams"]' -O2 -Wall
TARGET = ../../public/wasm/audio_service.Wdll

all: $(TARGET)

$(TARGET): audio_server.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@
	python3 ../../tools/add_module_section.py $@ Wdll
	python3 ../../tools/gen_wli.py $@

clean:
	rm -f $(TARGET) ../../public/wasm/audio_service.wli
