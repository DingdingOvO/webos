.PHONY: all clean gpu input storage network

all: gpu input storage network

gpu:
	cd gpu && make

input:
	cd input && make

storage:
	cd storage && make

network:
	cd network && make

clean:
	cd gpu && make clean 2>/dev/null || true
	cd input && make clean 2>/dev/null || true
	cd storage && make clean 2>/dev/null || true
	cd network && make clean 2>/dev/null || true
