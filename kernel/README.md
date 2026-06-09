# WebOS Kernel

The kernel is the core of WebOS, providing essential operating system services:

## Subsystems

| Subsystem | Files | Description |
|-----------|-------|-------------|
| Memory | `memory.c/h` | Memory allocation and management |
| Process | `process.c/h` | Process creation, scheduling, termination |
| Scheduler | `scheduler.c/h` | Round-robin process scheduler |
| IPC | `ipc.c/h` | Inter-process communication |
| Syscall | `syscall.c/h`, `syscall_dispatch.c` | System call dispatch |
| DynLink | `dynlink.c/h` | Dynamic module loading and linking |
| Host | `host_func.h` | Host bridge interface |

## Architecture

The kernel runs as a WASM module imported by the host runtime. User-space modules
communicate with the kernel through syscalls, which are dispatched via the host
bridge (`webos.syscall_invoke`).

```
User Module (.wex/.Wdll)
  └─> syscall_invoke() [WASM import]
        └─> RuntimeBridge [TypeScript host]
              └─> kernel.syscall_dispatch() [WASM export]
                    └─> syscall handler
```

## Building

```bash
make
```

Output: `public/wasm/kernel.wasm`
