.PHONY: all clean wm filesystem network appstore

all: wm filesystem network appstore

wm:
	cd window_manager && make

filesystem:
	cd filesystem && make

network:
	cd network_service && make

appstore:
	cd appstore && make

clean:
	cd window_manager && make clean 2>/dev/null || true
	cd filesystem && make clean 2>/dev/null || true
	cd network_service && make clean 2>/dev/null || true
	cd appstore && make clean 2>/dev/null || true
