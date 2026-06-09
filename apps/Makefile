.PHONY: all clean calculator paint browser appstore

all: calculator paint browser appstore

calculator:
	cd calculator && make

paint:
	cd paint && make

browser:
	cd browser && make

appstore:
	cd appstore_client && make

clean:
	cd calculator && make clean 2>/dev/null || true
	cd paint && make clean 2>/dev/null || true
	cd browser && make clean 2>/dev/null || true
	cd appstore_client && make clean 2>/dev/null || true
