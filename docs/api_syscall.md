# WebOS 系统调用 API 文档

## 1. 系统调用编号范围表

WebOS 的系统调用采用分区间编号方案，每个子系统分配一个 256 个编号的区间。系统调用号的高 8 位标识子系统类别，低 8 位标识该类别内的具体调用。这种设计使得系统调用号具有自描述性，开发者可以根据编号快速判断系统调用所属的功能类别。同时，每个区间预留了充足的编号空间，便于未来扩展新的系统调用。

| 区间名称 | 编号范围 | 起始值 | 描述 |
|---------|---------|-------|------|
| FS | 0x0100 - 0x01FF | 0x0100 | 文件系统操作（读写、目录管理） |
| PROCESS | 0x0200 - 0x02FF | 0x0200 | 进程管理（创建、终止、查询） |
| MEMORY | 0x0300 - 0x03FF | 0x0300 | 内存管理（分配、释放、映射） |
| TIME | 0x0400 - 0x04FF | 0x0400 | 时间服务（系统时钟） |
| IO | 0x0500 - 0x05FF | 0x0500 | 输入输出（设备操作） |
| NET | 0x0600 - 0x06FF | 0x0600 | 网络操作（HTTP 请求、Socket） |
| DEBUG | 0x0700 - 0x07FF | 0x0700 | 调试功能（日志输出） |
| IPC | 0x0800 - 0x08FF | 0x0800 | 进程间通信（消息收发） |
| NOTIFY | 0x0900 - 0x09FF | 0x0900 | 通知机制（系统事件） |
| DYNLINK | 0x0A00 - 0x0AFF | 0x0A00 | 动态链接（模块加载、符号解析） |

### 错误码定义

所有系统调用返回 `int32_t` / `long` 类型的值。返回值为 0 表示操作成功，正值通常表示成功时的附加信息（如读取的字节数），负值表示错误。以下是标准错误码定义：

| 错误码 | 常量 | 值 | 描述 |
|-------|------|-----|------|
| `ERR_OK` | 成功 | 0 | 操作成功完成 |
| `ERR_INVAL` | 无效参数 | -1 | 传入的参数不合法 |
| `ERR_NOENT` | 条目不存在 | -2 | 请求的文件、进程或资源不存在 |
| `ERR_NOMEM` | 内存不足 | -3 | 无法分配足够的内存 |
| `ERR_BUSY` | 资源忙 | -4 | 请求的资源正在被占用 |
| `ERR_PERM` | 权限不足 | -5 | 当前进程没有执行此操作的权限 |
| `ERR_FAULT` | 地址错误 | -6 | 传入的指针指向无效的内存地址 |
| `ERR_EXIST` | 已存在 | -7 | 试图创建已存在的资源 |
| `ERR_NOSYS` | 未实现 | -8 | 系统调用号无效或尚未实现 |

## 2. 系统调用详细说明

### FS 文件系统系统调用 (0x0100 - 0x01FF)

文件系统系统调用提供了对虚拟文件系统的访问能力，包括文件读写和目录操作。WebOS 的文件系统建立在存储驱动之上，使用 IndexedDB 作为底层持久化存储，为应用提供了 POSIX 风格的文件操作接口。

#### SYS_fs_read (0x0100)

从文件中读取数据。应用程序通过路径名指定要读取的文件，内核将文件内容读入用户提供的缓冲区中。

```c
int syscall_fs_read(const char* path, void* buf, int len);
```

| 参数 | 类型 | 说明 |
|------|------|------|
| `path` | `const char*` | 文件路径字符串指针（以 null 结尾） |
| `buf` | `void*` | 接收数据的缓冲区指针 |
| `len` | `int` | 请求读取的最大字节数 |
| **返回值** | `int` | 实际读取的字节数（≥0），或负数错误码 |

**使用示例**：

```c
#include "syscall.h"

char buffer[1024];
int bytes = syscall_fs_read("/config/settings.json", buffer, sizeof(buffer) - 1);
if (bytes > 0) {
    buffer[bytes] = '\0';  // 确保字符串以 null 结尾
    syscall_debug_log(buffer);
} else {
    // 处理错误
}
```

#### SYS_fs_write (0x0101)

向文件写入数据。如果文件不存在，行为取决于文件系统的实现——通常需要先创建文件。写入操作从文件开头开始，覆盖已有内容。

```c
int syscall_fs_write(const char* path, const void* buf, int len);
```

| 参数 | 类型 | 说明 |
|------|------|------|
| `path` | `const char*` | 文件路径字符串指针 |
| `buf` | `const void*` | 要写入的数据缓冲区指针 |
| `len` | `int` | 要写入的字节数 |
| **返回值** | `int` | 实际写入的字节数（≥0），或负数错误码 |

**使用示例**：

```c
const char* config = "{\"theme\":\"dark\",\"fontSize\":14}";
int bytes = syscall_fs_write("/config/settings.json", config, strlen(config));
if (bytes < 0) {
    syscall_debug_log("Failed to write config file");
}
```

#### SYS_fs_mkdir (0x0102)

创建新目录。如果父目录不存在，操作将失败并返回 `ERR_NOENT`。如果目录已存在，返回 `ERR_EXIST`。

```c
int syscall_fs_mkdir(const char* path);
```

| 参数 | 类型 | 说明 |
|------|------|------|
| `path` | `const char*` | 要创建的目录路径 |
| **返回值** | `int` | 0 表示成功，负数表示错误 |

#### SYS_fs_unlink (0x0103)

删除文件或空目录。如果目录不为空，操作将失败并返回 `ERR_BUSY`。如果文件不存在，返回 `ERR_NOENT`。

```c
int syscall_fs_unlink(const char* path);
```

| 参数 | 类型 | 说明 |
|------|------|------|
| `path` | `const char*` | 要删除的文件或目录路径 |
| **返回值** | `int` | 0 表示成功，负数表示错误 |

### PROCESS 进程管理系统调用 (0x0200 - 0x02FF)

进程管理系统调用提供了进程的创建、终止和查询能力。每个进程在内核中都有对应的进程控制块（PCB），记录了进程的运行时状态。

#### SYS_exit (0x0200)

终止当前进程。内核将当前进程的状态设置为 `PROC_STATE_ZOMBIE`，记录退出码，并通知父进程。此系统调用不会返回。

```c
void syscall_exit(int code);
```

| 参数 | 类型 | 说明 |
|------|------|------|
| `code` | `int` | 退出码，0 表示正常退出，非零表示异常 |
| **返回值** | `void` | 此调用不会返回 |

**使用示例**：

```c
if (window_id <= 0) {
    syscall_debug_log("Failed to create window");
    syscall_exit(1);  // 异常退出
}
```

#### SYS_getpid (0x0201)

获取当前进程的进程 ID。PID 是内核分配的唯一标识符，在进程生命周期内保持不变。

```c
int syscall_getpid(void);
```

| 参数 | 类型 | 说明 |
|------|------|------|
| 无 | — | — |
| **返回值** | `int` | 当前进程的 PID（≥1） |

**使用示例**：

```c
int my_pid = syscall_getpid();
char msg[64];
snprintf(msg, sizeof(msg), "My PID is %d", my_pid);
syscall_debug_log(msg);
```

#### SYS_getppid (0x0202)

获取父进程的进程 ID。父进程是创建当前进程的进程。如果父进程已终止，返回 0。

```c
int syscall_getppid(void);
```

| 参数 | 类型 | 说明 |
|------|------|------|
| 无 | — | — |
| **返回值** | `int` | 父进程的 PID，或 0（如果父进程已终止） |

#### SYS_kill (0x0204)

终止指定进程。调用者必须拥有足够的权限才能终止其他进程。通常只有内核和系统服务可以终止应用进程。

```c
int syscall_kill(pid_t pid);
```

| 参数 | 类型 | 说明 |
|------|------|------|
| `pid` | `pid_t` | 要终止的进程 ID |
| **返回值** | `int` | 0 表示成功，`ERR_NOENT` 表示进程不存在，`ERR_PERM` 表示权限不足 |

### MEMORY 内存管理系统调用 (0x0300 - 0x03FF)

内存管理系统调用提供了动态内存分配和释放的能力。在 WebOS 中，所有模块共享同一个 WebAssembly 线性内存空间，内核的内存管理器负责追踪每个内存区域的所有者和使用状态。

#### SYS_malloc (0x0300)

分配指定大小的内存区域。内核在自由链表中查找足够大的空闲区域，将其分配给当前进程并记录所有者信息。分配的内存内容未初始化。

```c
void* syscall_malloc(size_t size);
```

| 参数 | 类型 | 说明 |
|------|------|------|
| `size` | `size_t` | 请求分配的字节数 |
| **返回值** | `void*` | 分配的内存起始地址，或 NULL（内存不足） |

**使用示例**：

```c
// 分配一个像素缓冲区
int width = 400, height = 300;
void* pixels = syscall_malloc(width * height * 4);  // RGBA
if (!pixels) {
    syscall_debug_log("Out of memory");
    syscall_exit(1);
}
// 使用像素缓冲区...
syscall_free(pixels);
```

#### SYS_free (0x0301)

释放之前通过 `SYS_malloc` 分配的内存区域。释放后，内存区域被标记为空闲，并与相邻的空闲区域合并。传入 NULL 指针是安全的，不会有任何效果。

```c
void syscall_free(void* ptr);
```

| 参数 | 类型 | 说明 |
|------|------|------|
| `ptr` | `void*` | 要释放的内存地址（必须是之前 malloc 返回的地址） |
| **返回值** | `void` | 无返回值 |

### TIME 时间系统调用 (0x0400 - 0x04FF)

#### SYS_time_now (0x0400)

获取当前系统时间，以 Unix 时间戳表示（自 1970-01-01 00:00:00 UTC 以来的毫秒数）。底层使用浏览器的 `Date.now()` API 实现。

```c
long syscall_time_now(void);
```

| 参数 | 类型 | 说明 |
|------|------|------|
| 无 | — | — |
| **返回值** | `long` | 当前时间的 Unix 时间戳（毫秒） |

**使用示例**：

```c
long start = syscall_time_now();
// 执行一些耗时操作...
long elapsed = syscall_time_now() - start;
char msg[64];
snprintf(msg, sizeof(msg), "Elapsed: %ld ms", elapsed);
syscall_debug_log(msg);
```

### IO 输入输出系统调用 (0x0500 - 0x05FF)

此区间的系统调用预留用于设备 I/O 操作，当前版本暂未实现。应用程序应通过驱动模块或服务模块间接访问设备功能。

### NET 网络系统调用 (0x0600 - 0x06FF)

此区间的系统调用预留用于网络操作，当前版本暂未实现。应用程序应通过网络服务（`net_service.Wdll`）的 IPC 接口进行 HTTP 请求等网络操作。

### DEBUG 调试系统调用 (0x0700 - 0x07FF)

#### SYS_debug_log (0x0700)

输出调试日志消息到浏览器控制台。消息以 `[print]` 前缀格式显示在浏览器的开发者工具控制台中。此系统调用仅在调试模式下有效，发布版本中可能被优化掉。

```c
void syscall_debug_log(const char* msg);
```

| 参数 | 类型 | 说明 |
|------|------|------|
| `msg` | `const char*` | 日志消息字符串指针（以 null 结尾） |
| **返回值** | `void` | 无返回值 |

**使用示例**：

```c
syscall_debug_log("Application started");
syscall_debug_log("Initializing window manager...");

// 条件日志
if (error_code != 0) {
    char err_msg[128];
    snprintf(err_msg, sizeof(err_msg), "Error: code=%d", error_code);
    syscall_debug_log(err_msg);
}
```

### IPC 进程间通信系统调用 (0x0800 - 0x08FF)

#### SYS_ipc_send (0x0800)

向指定进程发送 IPC 消息。消息数据被复制到目标进程的消息队列中，发送操作是非阻塞的——即使目标进程尚未准备好接收，消息也会被排队等待。

```c
int syscall_ipc_send(int pid, const void* msg, int len);
```

| 参数 | 类型 | 说明 |
|------|------|------|
| `pid` | `int` | 目标进程 ID |
| `msg` | `const void*` | 消息数据指针 |
| `len` | `int` | 消息数据长度（字节） |
| **返回值** | `int` | 0 表示成功，`ERR_NOENT` 表示目标进程不存在，`ERR_BUSY` 表示目标队列已满 |

**使用示例**：

```c
// 向窗口管理器服务发送消息
typedef struct {
    uint32_t type;
    int window_id;
    int x, y;
} wm_request_t;

wm_request_t req = {
    .type = 0x04,  // IPC_MSG_REQUEST
    .window_id = win,
    .x = 100,
    .y = 200
};

int result = syscall_ipc_send(wm_service_pid, &req, sizeof(req));
if (result < 0) {
    syscall_debug_log("IPC send failed");
}
```

#### SYS_ipc_recv (0x0801)

接收 IPC 消息。此调用是非阻塞的——如果当前进程的消息队列为空，立即返回 `ERR_BUSY` 而不等待。应用程序通常在主循环中轮询调用此函数。

```c
int syscall_ipc_recv(void* buf, int len);
```

| 参数 | 类型 | 说明 |
|------|------|------|
| `buf` | `void*` | 接收消息的缓冲区指针 |
| `len` | `int` | 缓冲区大小（字节） |
| **返回值** | `int` | 接收到的消息长度（≥0），`ERR_BUSY` 表示没有消息 |

**使用示例**：

```c
// 在主循环中轮询 IPC 消息
void app_tick(void* arg) {
    char buf[4096];
    int msg_len;

    while ((msg_len = syscall_ipc_recv(buf, sizeof(buf))) > 0) {
        // 处理收到的消息
        handle_ipc_message(buf, msg_len);
    }

    // 继续其他工作...
}
```

### NOTIFY 通知系统调用 (0x0900 - 0x09FF)

此区间的系统调用预留用于系统通知和事件机制，当前版本暂未实现。应用程序应通过 IPC 消息的 `IPC_MSG_EVENT` 类型接收系统事件。

### DYNLINK 动态链接系统调用 (0x0A00 - 0x0AFF)

#### SYS_dlopen (0x0A00)

加载一个动态链接库模块。内核通过宿主运行时获取指定名称的 WASM 模块，编译并实例化后注册到动态链接器的模块表中。如果模块已经加载，增加引用计数并返回现有句柄。

```c
void* syscall_dlopen(const char* name);  // 实际通过 syscall1 封装
// 对应内核函数:
// module_handle_t dynlink_load(const char* name, int32_t name_len);
```

| 参数 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | 模块名称（如 `"wm_service.Wdll"`） |
| **返回值** | `module_handle_t` | 模块句柄，或 `INVALID_MODULE_HANDLE` (0) 表示失败 |

**使用示例**：

```c
// 加载窗口管理器服务
module_handle_t wm_handle = (module_handle_t)syscall1(SYS_dlopen, (long)"wm_service.Wdll");
if (wm_handle == INVALID_MODULE_HANDLE) {
    syscall_debug_log("Failed to load wm_service.Wdll");
    syscall_exit(1);
}
```

#### SYS_dlsym (0x0A01)

从已加载的动态链接库中解析导出符号。返回指定函数的地址，可以通过函数指针直接调用。

```c
int syscall_dlsym(void* handle, const char* sym, void** out);
// 对应内核函数:
// int32_t dynlink_resolve(module_handle_t handle, const char* sym,
//                         int32_t sym_len, void** out);
```

| 参数 | 类型 | 说明 |
|------|------|------|
| `handle` | `void*` | 模块句柄（由 `SYS_dlopen` 返回） |
| `sym` | `const char*` | 符号名称字符串 |
| `out` | `void**` | 输出：函数指针的存放地址 |
| **返回值** | `int` | 0 表示成功，`ERR_NOENT` 表示符号不存在 |

**使用示例**：

```c
// 解析窗口管理器的函数
typedef int (*wm_create_fn)(int, const char*, int, int, int, int);
wm_create_fn wm_create;

// 注意：实际使用中需要通过 syscall3 封装
// 此处展示概念性用法
void* func_ptr;
int result = syscall3(SYS_dlsym, (long)wm_handle, (long)"wm_create_window", (long)&func_ptr);
if (result == 0) {
    wm_create = (wm_create_fn)func_ptr;
    int win = wm_create(getpid(), "My Window", 100, 100, 400, 300);
}
```

## 3. 如何添加新的系统调用

当 WebOS 需要新增功能时，通常需要添加新的系统调用。以下是完整的步骤指南，涵盖了从定义编号到实现处理器的全过程。

### 步骤 1：选择系统调用编号

根据功能类别选择正确的编号区间。例如，要添加一个文件系统的 stat 操作，应使用 FS 区间中未使用的编号：

```c
// libs/syscall.h
#define SYS_fs_stat  0x0104   // 新增：获取文件状态信息
```

确保所选编号在对应区间内尚未被使用。建议在区间末尾追加新的编号，避免与现有编号冲突。

### 步骤 2：在内核中添加处理器函数

在内核的 syscall_dispatch.c 中实现新的系统调用处理器：

```c
// kernel/src/syscall_dispatch.c

static int32_t sys_fs_stat(int32_t a1, int32_t a2,
                            int32_t a3, int32_t a4,
                            int32_t a5) {
    const char* path = (const char*)a1;
    void* stat_buf = (void*)a2;

    // 实现文件状态查询逻辑
    // 通过宿主桥接器访问 IndexedDB 元数据
    // ...

    return 0;  // ERR_OK
}
```

### 步骤 3：注册处理器到分发表

在 `syscall_init()` 函数中注册新的系统调用处理器：

```c
// kernel/src/syscall_dispatch.c

void syscall_init(void) {
    // ... 已有的注册 ...

    // 注册新系统调用
    syscall_register(SYS_fs_stat, sys_fs_stat);
}
```

### 步骤 4：在用户空间添加便捷封装

在 `libs/syscall.h` 中添加用户空间的便捷调用函数：

```c
// libs/syscall.h

#define SYS_fs_stat  0x0104

typedef struct {
    uint32_t size;
    uint32_t mode;
    uint32_t atime;
    uint32_t mtime;
} fs_stat_t;

static inline int syscall_fs_stat(const char* path, fs_stat_t* stat) {
    return (int)syscall2(SYS_fs_stat, (long)path, (long)stat);
}
```

### 步骤 5：在宿主运行时实现浏览器端逻辑

如果新系统调用需要访问浏览器 API，在 `host/src/runtime_bridge.ts` 中注册宿主端处理器：

```typescript
// host/src/runtime_bridge.ts

// SYS_fs_stat (0x0104)
this.registerSyscall(0x0104, (pathPtr, statBufPtr, _a3, _a4, _a5) => {
    const pathBytes = new Uint8Array(this.sharedMemory.buffer, pathPtr);
    const path = new TextDecoder().decode(pathBytes);

    // 使用 IndexedDB 获取文件元数据
    // ...

    // 将结果写入 statBufPtr 指向的内存
    const statView = new Int32Array(this.sharedMemory.buffer, statBufPtr, 4);
    statView[0] = fileSize;
    statView[1] = fileMode;
    statView[2] = atime;
    statView[3] = mtime;

    return 0;  // ERR_OK
});
```

### 步骤 6：更新文档

更新本 API 文档，在对应区间中添加新系统调用的说明，包括编号、参数、返回值和使用示例。

## 4. 使用示例代码

### 完整的应用程序示例

以下示例展示了一个完整的 WebOS 应用程序，演示了多种系统调用的综合使用：

```c
#include <stdint.h>
#include <string.h>

/* 系统调用接口 */
extern "C" {
    long syscall0(long n);
    long syscall1(long n, long a1);
    long syscall2(long n, long a1, long a2);
    long syscall3(long n, long a1, long a2, long a3);

    #define SYS_exit       0x0200
    #define SYS_getpid     0x0201
    #define SYS_malloc     0x0300
    #define SYS_free       0x0301
    #define SYS_time_now   0x0400
    #define SYS_debug_log  0x0700
    #define SYS_ipc_send   0x0800
    #define SYS_ipc_recv   0x0801

    void syscall_exit(int code) { syscall1(SYS_exit, code); }
    int syscall_getpid(void) { return (int)syscall0(SYS_getpid); }
    void* syscall_malloc(size_t size) { return (void*)syscall1(SYS_malloc, (long)size); }
    void syscall_free(void* ptr) { syscall1(SYS_free, (long)ptr); }
    long syscall_time_now(void) { return syscall0(SYS_time_now); }
    void syscall_debug_log(const char* msg) { syscall2(SYS_debug_log, (long)msg, 0); }
    int syscall_ipc_send(int pid, const void* msg, int len) {
        return (int)syscall3(SYS_ipc_send, (long)pid, (long)msg, (long)len);
    }
    int syscall_ipc_recv(void* buf, int len) {
        return (int)syscall2(SYS_ipc_recv, (long)buf, (long)len);
    }
}

#include <emscripten.h>

/* 应用状态 */
static int app_running = 1;

/* 主循环回调 */
void app_tick(void* arg) {
    if (!app_running) return;

    /* 1. 轮询 IPC 消息 */
    char msg_buf[4096];
    int msg_len;
    while ((msg_len = syscall_ipc_recv(msg_buf, sizeof(msg_buf))) > 0) {
        /* 处理 IPC 消息 */
        syscall_debug_log("Received IPC message");
    }

    /* 2. 更新应用逻辑 */
    long now = syscall_time_now();
    /* ... 使用 now 进行时间计算 ... */

    /* 3. 渲染帧 */
    /* ... 调用 WM API 渲染 ... */
}

int main(void) {
    /* 输出启动日志 */
    syscall_debug_log("Sample App starting...");

    /* 获取当前 PID */
    int pid = syscall_getpid();
    char pid_msg[64];
    snprintf(pid_msg, sizeof(pid_msg), "My PID: %d", pid);
    syscall_debug_log(pid_msg);

    /* 分配内存 */
    void* buffer = syscall_malloc(1024);
    if (!buffer) {
        syscall_debug_log("Failed to allocate memory");
        syscall_exit(1);
    }

    /* 使用分配的内存 */
    memset(buffer, 0, 1024);

    /* 读取配置文件 */
    /* int bytes = syscall_fs_read("/config/app.conf", buffer, 1024); */

    /* 注册主循环 (~60fps) */
    emscripten_set_interval(app_tick, 16, NULL);

    /* 释放内存 */
    syscall_free(buffer);

    return 0;
}
```

### IPC 通信示例

以下示例展示了两个进程之间通过 IPC 进行请求-响应通信的完整流程：

```c
/* === 客户端进程 === */

/* 发送请求 */
typedef struct {
    uint32_t msg_type;    /* IPC_MSG_REQUEST = 0x04 */
    uint32_t request_id;
    char payload[256];
} service_request_t;

typedef struct {
    uint32_t msg_type;    /* IPC_MSG_RESPONSE = 0x05 */
    uint32_t request_id;
    int32_t result;
    char payload[256];
} service_response_t;

void call_service(int service_pid, const char* query) {
    service_request_t req;
    req.msg_type = 0x04;  /* IPC_MSG_REQUEST */
    req.request_id = 1;
    strncpy(req.payload, query, sizeof(req.payload) - 1);

    int ret = syscall_ipc_send(service_pid, &req, sizeof(req));
    if (ret < 0) {
        syscall_debug_log("Failed to send IPC request");
        return;
    }

    /* 等待响应（轮询） */
    char buf[4096];
    int max_retries = 100;
    while (max_retries-- > 0) {
        int len = syscall_ipc_recv(buf, sizeof(buf));
        if (len >= sizeof(service_response_t)) {
            service_response_t* resp = (service_response_t*)buf;
            if (resp->msg_type == 0x05 && resp->request_id == 1) {
                /* 收到匹配的响应 */
                char msg[128];
                snprintf(msg, sizeof(msg),
                         "Service returned: %d, %s",
                         resp->result, resp->payload);
                syscall_debug_log(msg);
                return;
            }
        }
    }
    syscall_debug_log("IPC response timeout");
}
```

```c
/* === 服务端进程 === */

void service_tick(void* arg) {
    char buf[4096];
    int len;

    while ((len = syscall_ipc_recv(buf, sizeof(buf))) > 0) {
        service_request_t* req = (service_request_t*)buf;
        if (req->msg_type != 0x04) continue;  /* 只处理请求 */

        /* 处理请求 */
        service_response_t resp;
        resp.msg_type = 0x05;  /* IPC_MSG_RESPONSE */
        resp.request_id = req->request_id;
        resp.result = process_query(req->payload, resp.payload);

        /* 发送响应 */
        int sender_pid = /* 从 IPC 元数据获取发送者 PID */;
        syscall_ipc_send(sender_pid, &resp, sizeof(resp));
    }
}
```
