#ifndef SYSCALL_H
#define SYSCALL_H

/**
 * WebOS System Call Interface
 * 
 * User-space system call stub functions.
 * Applications include this header to make system calls.
 */

/* Syscall number definitions */
#define SYS_exit            0x0200
#define SYS_getpid          0x0201
#define SYS_getppid         0x0202
#define SYS_kill            0x0204

#define SYS_malloc          0x0300
#define SYS_free            0x0301

#define SYS_fs_read         0x0100
#define SYS_fs_write        0x0101
#define SYS_fs_mkdir        0x0102
#define SYS_fs_unlink       0x0103

#define SYS_time_now        0x0400

#define SYS_debug_log       0x0700

#define SYS_ipc_send        0x0800
#define SYS_ipc_recv        0x0801

#define SYS_dlopen          0x0A00
#define SYS_dlsym           0x0A01

/* Syscall wrapper functions */
static inline long syscall0(long n) {
    /* In WASM, syscalls go through the kernel's exported function */
    extern long kernel_syscall(long, long, long, long);
    return kernel_syscall(n, 0, 0, 0);
}

static inline long syscall1(long n, long a1) {
    extern long kernel_syscall(long, long, long, long);
    return kernel_syscall(n, a1, 0, 0);
}

static inline long syscall2(long n, long a1, long a2) {
    extern long kernel_syscall(long, long, long, long);
    return kernel_syscall(n, a1, a2, 0);
}

static inline long syscall3(long n, long a1, long a2, long a3) {
    extern long kernel_syscall(long, long, long, long);
    return kernel_syscall(n, a1, a2, a3);
}

/* Convenience wrappers */
static inline void syscall_exit(int code) { syscall1(SYS_exit, code); }
static inline int syscall_getpid(void) { return (int)syscall0(SYS_getpid); }
static inline int syscall_getppid(void) { return (int)syscall0(SYS_getppid); }
static inline void* syscall_malloc(size_t size) { return (void*)syscall1(SYS_malloc, (long)size); }
static inline void syscall_free(void* ptr) { syscall1(SYS_free, (long)ptr); }
static inline int syscall_fs_read(const char* path, void* buf, int len) { return (int)syscall3(SYS_fs_read, (long)path, (long)buf, (long)len); }
static inline int syscall_fs_write(const char* path, const void* buf, int len) { return (int)syscall3(SYS_fs_write, (long)path, (long)buf, (long)len); }
static inline long syscall_time_now(void) { return syscall0(SYS_time_now); }
static inline void syscall_debug_log(const char* msg) { syscall2(SYS_debug_log, (long)msg, 0); }
static inline int syscall_ipc_send(int pid, const void* msg, int len) { return (int)syscall3(SYS_ipc_send, (long)pid, (long)msg, (long)len); }
static inline int syscall_ipc_recv(void* buf, int len) { return (int)syscall2(SYS_ipc_recv, (long)buf, (long)len); }

#endif /* SYSCALL_H */
