# WebOS Shared Libraries Makefile

CC = clang
CFLAGS = -target wasm32 -nostdlib -Wall -Wextra -O2
AR = llvm-ar

LIB_OBJS = syscall.o wm_client.o

all: libwebos.a

libwebos.a: $(LIB_OBJS)
	$(AR) rcs $@ $^

syscall.o: syscall.c syscall.h module_types.h
	$(CC) $(CFLAGS) -c -o $@ $<

wm_client.o: wm_client.c wm_client.h syscall.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(LIB_OBJS) libwebos.a
