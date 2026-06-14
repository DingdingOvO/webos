CC = em++
CFLAGS = -s WASM=1 -s EXPORTED_FUNCTIONS='["_main"]' \
         -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \
         -s ENVIRONMENT='web' -s ALLOW_MEMORY_GROWTH=1 \
         -O2 -Wall
TARGET = ../../public/wasm/file_manager.wex

all: $(TARGET)

$(TARGET): main.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@
	python3 ../../tools/add_module_section.py $@ wex

clean:
	rm -f $(TARGET)
