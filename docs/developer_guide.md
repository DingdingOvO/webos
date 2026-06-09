# WebOS 应用开发指南

## 1. 如何编写第一个 C++ 应用程序

本节将引导您从零开始编写一个完整的 WebOS 计算器应用程序。这个计算器应用将展示 WebOS 应用开发的所有核心概念，包括窗口创建、用户输入处理、界面渲染和状态管理。通过这个完整的示例，您将掌握开发 WebOS 应用所需的全部技能。

### 1.1 应用结构概览

一个典型的 WebOS 应用由三个文件组成：C++ 源代码文件（`main.cpp`）、应用清单文件（`app.json`）和构建脚本（`Makefile`）。源代码文件包含应用的全部逻辑，清单文件描述应用的元数据，构建脚本定义编译和打包流程。

```
apps/calculator/
├── main.cpp        # C++ 源代码
├── app.json        # 应用清单
└── Makefile        # 构建脚本
```

### 1.2 完整的计算器示例代码

以下是完整的计算器应用程序源代码，每个部分都附有详细的注释说明：

```cpp
/**
 * WebOS Calculator Application
 *
 * 一个功能完整的计算器应用，演示了 WebOS 应用开发的
 * 所有核心概念：窗口管理、用户输入、界面渲染和状态管理。
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <emscripten.h>

/* ========================================
 * 系统调用接口
 * ======================================== */

extern "C" {
    /* 系统调用桥接函数 - 由内核导出 */
    long syscall0(long n);
    long syscall1(long n, long a1);
    long syscall2(long n, long a1, long a2);
    long syscall3(long n, long a1, long a2, long a3);

    /* 系统调用编号 */
    #define SYS_exit       0x0200
    #define SYS_getpid     0x0201
    #define SYS_debug_log  0x0700

    /* 系统调用便捷封装 */
    void syscall_exit(int code) { syscall1(SYS_exit, code); }
    int syscall_getpid(void) { return (int)syscall0(SYS_getpid); }
    void syscall_debug_log(const char* msg) {
        syscall2(SYS_debug_log, (long)msg, 0);
    }

    /* 窗口管理器客户端函数 */
    int wm_create_window(const char* title, int x, int y, int width, int height);
    void wm_show_window(int window_id);
    void wm_destroy_window(int window_id);
    void wm_move_window(int window_id, int x, int y);
    void wm_resize_window(int window_id, int width, int height);
}

/* ========================================
 * 计算器状态
 * ======================================== */

static int g_window_id = 0;       /* 当前窗口 ID */
static double g_display = 0.0;    /* 当前显示值 */
static double g_stored = 0.0;     /* 存储的操作数 */
static char g_operation = 0;      /* 待执行的运算符 */
static int g_new_number = 1;      /* 是否开始输入新数字 */
static char g_display_text[32] = "0";  /* 显示文本 */

/* ========================================
 * 按钮定义
 * ======================================== */

struct Button {
    const char* label;    /* 按钮标签文本 */
    int x, y, w, h;      /* 按钮位置和尺寸 */
    const char* action;   /* 按钮动作标识 */
};

static Button g_buttons[] = {
    /* 第一行：数字 7-9 和除号 */
    {"7",  10,  80, 65, 45, "digit:7"},
    {"8",  80,  80, 65, 45, "digit:8"},
    {"9",  150, 80, 65, 45, "digit:9"},
    {"/",  220, 80, 65, 45, "op:/"},

    /* 第二行：数字 4-6 和乘号 */
    {"4",  10,  130, 65, 45, "digit:4"},
    {"5",  80,  130, 65, 45, "digit:5"},
    {"6",  150, 130, 65, 45, "digit:6"},
    {"*",  220, 130, 65, 45, "op:*"},

    /* 第三行：数字 1-3 和减号 */
    {"1",  10,  180, 65, 45, "digit:1"},
    {"2",  80,  180, 65, 45, "digit:2"},
    {"3",  150, 180, 65, 45, "digit:3"},
    {"-",  220, 180, 65, 45, "op:-"},

    /* 第四行：数字 0、小数点和加号 */
    {"0",  10,  230, 135, 45, "digit:0"},
    {".",  150, 230, 65, 45, "digit:."},
    {"+",  220, 230, 65, 45, "op:+"},

    /* 第五行：清除和等号 */
    {"C",  10,  280, 100, 45, "clear"},
    {"=",  115, 280, 170, 45, "eq"},
};

static int g_num_buttons = sizeof(g_buttons) / sizeof(g_buttons[0]);

/* ========================================
 * 计算逻辑
 * ======================================== */

/**
 * 处理数字和小数点输入
 * 当用户按下数字键时，将数字追加到当前显示文本中。
 * 如果正在开始一个新数字，则替换当前显示。
 */
void handle_digit(const char* d) {
    if (g_new_number) {
        if (d[0] == '.') {
            strcpy(g_display_text, "0.");
        } else {
            strcpy(g_display_text, d);
        }
        g_new_number = 0;
    } else {
        /* 防止重复输入小数点 */
        if (d[0] == '.' && strchr(g_display_text, '.')) return;
        int len = strlen(g_display_text);
        if (len < 15) {  /* 限制最大位数 */
            g_display_text[len] = d[0];
            g_display_text[len + 1] = '\0';
        }
    }
    g_display = atof(g_display_text);
}

/**
 * 执行待处理的算术运算
 * 如果有之前的运算符和操作数，先执行该运算，
 * 然后保存新的运算符和当前值。
 */
void evaluate_pending(void) {
    if (g_operation && !g_new_number) {
        switch (g_operation) {
            case '+': g_display = g_stored + g_display; break;
            case '-': g_display = g_stored - g_display; break;
            case '*': g_display = g_stored * g_display; break;
            case '/':
                if (g_display != 0) {
                    g_display = g_stored / g_display;
                }
                break;
        }
        snprintf(g_display_text, sizeof(g_display_text),
                 "%.10g", g_display);
    }
}

/**
 * 处理运算符输入
 */
void handle_op(char op) {
    evaluate_pending();
    g_stored = g_display;
    g_operation = op;
    g_new_number = 1;
}

/**
 * 处理等号
 */
void handle_equals(void) {
    evaluate_pending();
    g_operation = 0;
    g_new_number = 1;
}

/**
 * 处理清除键
 */
void handle_clear(void) {
    g_display = 0.0;
    g_stored = 0.0;
    g_operation = 0;
    g_new_number = 1;
    strcpy(g_display_text, "0");
}

/* ========================================
 * 渲染与主循环
 * ======================================== */

/**
 * 主循环回调函数
 * 由 emscripten_set_interval 以约 60fps 的频率调用。
 * 每次回调执行：轮询输入事件 → 更新状态 → 渲染界面
 */
void calc_tick(void* arg) {
    /*
     * 在完整实现中，此函数会：
     *
     * 1. 轮询输入事件
     *    wm_event_t event;
     *    while (wm_poll_event(g_window_id, &event) > 0) {
     *        if (event.type == WM_EVENT_MOUSE) {
     *            // 检查是否点击了某个按钮
     *            for (int i = 0; i < g_num_buttons; i++) {
     *                if (hit_test(&g_buttons[i],
     *                            event.data.mouse.x,
     *                            event.data.mouse.y)) {
     *                    handle_button(&g_buttons[i]);
     *                }
     *            }
     *        }
     *    }
     *
     * 2. 渲染界面
     *    - 绘制显示区域（显示当前数值）
     *    - 绘制所有按钮
     *    - 高亮被按下的按钮
     *    - 调用 GPU 驱动渲染到窗口帧缓冲
     *
     * 3. 让出 CPU
     *    emscripten 的定时回调自动实现协作式让出
     */
}

/* ========================================
 * 程序入口
 * ======================================== */

/**
 * 应用程序主函数
 * 创建窗口、初始化状态、注册主循环
 */
int main(int argc, char** argv) {
    /* 输出启动日志 */
    syscall_debug_log("Calculator starting...");

    /* 创建计算器窗口 */
    g_window_id = wm_create_window("Calculator", 200, 200, 300, 340);
    if (g_window_id <= 0) {
        syscall_debug_log("Failed to create window");
        syscall_exit(1);
    }

    /* 显示窗口 */
    wm_show_window(g_window_id);

    /* 注册定时回调，约 60fps */
    emscripten_set_interval(calc_tick, 16, NULL);

    syscall_debug_log("Calculator initialized");

    return 0;
}
```

### 1.3 代码结构解析

上面的代码可以分为五个逻辑部分：

**系统调用接口部分**：声明了与内核通信所需的桥接函数和系统调用编号。所有 WebOS 应用都需要这些声明来使用内核提供的服务。`syscall0` 到 `syscall3` 是内核导出的 WASM 函数，分别接受 0-3 个参数。便捷封装函数（如 `syscall_exit`、`syscall_debug_log`）隐藏了底层的调用约定细节，使代码更易读。

**状态管理部分**：定义了计算器的核心状态变量。`g_display` 存储当前显示的数值，`g_stored` 存储上一个操作数，`g_operation` 存储待执行的运算符，`g_new_number` 标记是否正在输入新数字，`g_display_text` 存储显示文本的字符串形式。这些变量共同构成了计算器的完整状态。

**按钮定义部分**：使用结构体数组定义了计算器界面的所有按钮。每个按钮记录了标签文本、屏幕坐标、尺寸和动作标识。这种数据驱动的界面设计使得按钮布局的修改不需要改变逻辑代码。

**计算逻辑部分**：实现了计算器的核心算法。`handle_digit()` 处理数字输入，`handle_op()` 处理运算符，`handle_equals()` 计算最终结果，`handle_clear()` 重置所有状态。`evaluate_pending()` 是内部辅助函数，在需要时执行之前暂存的运算。

**渲染与主循环部分**：`calc_tick()` 是应用的主循环回调，由 Emscripten 的 `emscripten_set_interval()` 以约 60fps 的频率调用。`main()` 函数创建窗口、显示窗口、注册回调，然后返回。返回后，控制权交给浏览器事件循环，`calc_tick()` 在每个帧周期被自动调用。

## 2. 编译命令详解

### 2.1 编译器选择

WebOS 应用使用 Emscripten C++ 编译器（`em++`）编译。Emscripten 提供了完整的 C++ 标准库支持，并将 C++ 代码编译为 WebAssembly 模块。选择 `em++` 而非标准 `clang++` 的原因是 Emscripten 自动处理了 WASM 模块的生成、JavaScript 胶水代码的创建、以及 C++ 异常和 RTTI 的 WebAssembly 适配。

### 2.2 编译选项详解

```makefile
CC = em++
CFLAGS = -s WASM=1 \
         -s EXPORTED_FUNCTIONS='["_main"]' \
         -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \
         -s ENVIRONMENT='web' \
         -s ALLOW_MEMORY_GROWTH=1 \
         -O2 \
         -Wall
```

| 选项 | 说明 |
|------|------|
| `-s WASM=1` | 输出 WebAssembly 格式（而非仅 JavaScript） |
| `-s EXPORTED_FUNCTIONS='["_main"]'` | 指定需要导出的函数列表。函数名需要加下划线前缀（C 名称修饰） |
| `-s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]'` | 导出运行时辅助方法，允许 JavaScript 调用 C 函数 |
| `-s ENVIRONMENT='web'` | 目标环境为 Web 浏览器（非 Node.js） |
| `-s ALLOW_MEMORY_GROWTH=1` | 允许 WASM 线性内存动态增长，避免固定内存限制 |
| `-O2` | 优化级别 2，平衡编译速度和运行性能 |
| `-Wall` | 启用所有常见编译警告 |

### 2.3 动态链接库的编译选项

驱动和服务使用 `emcc`（C 编译器）编译，并需要额外的 SIDE_MODULE 选项：

```makefile
CC = emcc
CFLAGS = -s WASM=1 \
         -s SIDE_MODULE=1 \
         -s EXPORTED_FUNCTIONS='["_wm_service_init","_wm_create_window",...]' \
         -O2 \
         -Wall
```

| 选项 | 说明 |
|------|------|
| `-s SIDE_MODULE=1` | 编译为可动态链接的侧边模块，不能独立运行 |
| `-s EXPORTED_FUNCTIONS='[...]'` | 列出所有需要导出的公共 API 函数 |

### 2.4 后处理步骤

编译完成后，需要执行两个后处理步骤：

1. **添加自定义段**：`python3 ../../tools/add_module_section.py $@ wex`
2. **生成接口文件**（仅 .Wdll）：`python3 ../../tools/gen_wli.py $@`

## 3. 添加自定义段标记

自定义段标记是 WebOS 运行时识别模块类型的关键机制。每个编译完成的 WASM 模块都必须通过 `add_module_section.py` 工具添加 `module_type` 自定义段，否则运行时将无法确定模块的加载方式。

### 3.1 为什么需要自定义段

文件后缀名（`.wex`、`.Wdll`）只是人类可读的标识，运行时系统依赖 WASM 二进制内部的 `module_type` 自定义段来确定模块类型。这种设计有以下优势：即使文件被重命名或通过网络传输丢失了原始文件名，模块类型信息仍然保留在二进制数据中；自定义段可以被工具链自动解析，不需要文件系统支持；自定义段可以包含额外的元数据，为未来扩展预留空间。

### 3.2 在 Makefile 中集成

```makefile
# 应用程序 Makefile 示例
TARGET = ../../public/wasm/myapp.wex

all: $(TARGET)

$(TARGET): main.cpp
        @mkdir -p $(dir $@)
        $(CC) $(CFLAGS) $< -o $@
        # 添加自定义段标记
        python3 ../../tools/add_module_section.py $@ wex

clean:
        rm -f $(TARGET)
```

### 3.3 验证自定义段

可以使用 WebAssembly 二进制工具检查自定义段是否正确添加：

```bash
# 使用 wasm-objdump 查看段信息（如果安装了 WABT 工具包）
wasm-objdump -h public/wasm/calculator.wex

# 输出应包含：
# Section 1 (custom "module_type") start=0x0008 end=0x001a
```

也可以使用 `add_module_section.py` 的隐式验证：如果文件已包含 `module_type` 段，工具会输出 "Replacing existing module_type section" 信息，表明检测到了已有段。

## 4. 如何调用系统服务

WebOS 应用程序通过两种方式调用系统服务：直接函数调用（动态链接后）和 IPC 消息传递。直接函数调用适用于已加载到进程地址空间的服务，IPC 消息传递适用于跨进程的服务请求。

### 4.1 通过 WM 客户端库调用窗口管理器

最常见的服务调用方式是使用 WM 客户端库，它封装了与窗口管理器服务的交互细节：

```cpp
#include "wm_client.h"    /* 窗口管理器客户端库头文件 */
#include "syscall.h"      /* 系统调用接口 */

int main(void) {
    /* 创建窗口 - 通过 WM 客户端库 */
    int win = wm_create_window("My App", 100, 100, 400, 300);
    if (win <= 0) {
        syscall_debug_log("Failed to create window");
        syscall_exit(1);
    }

    /* 显示窗口 */
    wm_show_window(win);

    /* 移动窗口 */
    wm_move_window(win, 200, 200);

    /* 调整窗口大小 */
    wm_resize_window(win, 500, 400);

    /* 销毁窗口 */
    wm_destroy_window(win);

    return 0;
}
```

### 4.2 通过 IPC 调用文件系统服务

对于跨进程的服务调用，使用 IPC 消息传递：

```cpp
#include "syscall.h"

/* 文件系统服务操作码 */
#define FS_OP_READ     0x11
#define FS_OP_WRITE    0x12

/* IPC 请求头 */
typedef struct {
    uint32_t msg_type;       /* IPC_MSG_REQUEST = 0x04 */
    uint32_t request_id;     /* 唯一请求 ID */
    uint32_t service_op;     /* 服务操作码 */
    uint32_t payload_len;    /* 负载长度 */
} ipc_request_header_t;

/**
 * 从文件系统读取文件内容
 *
 * @param path     文件路径
 * @param buf      接收数据的缓冲区
 * @param buf_len  缓冲区大小
 * @return         实际读取的字节数，或负数错误码
 */
int read_file(const char* path, void* buf, int buf_len) {
    /* 构造 IPC 请求消息 */
    struct {
        ipc_request_header_t header;
        char path[256];
        int buf_len;
    } request;

    request.header.msg_type = 0x04;   /* IPC_MSG_REQUEST */
    request.header.request_id = 1;
    request.header.service_op = FS_OP_READ;
    request.header.payload_len = sizeof(request) - sizeof(ipc_request_header_t);
    strncpy(request.path, path, sizeof(request.path) - 1);
    request.buf_len = buf_len;

    /* 发送请求到文件系统服务（假设 PID 为 3） */
    int result = syscall_ipc_send(3, &request, sizeof(request));
    if (result < 0) {
        syscall_debug_log("IPC send failed");
        return -1;
    }

    /* 等待响应 */
    char response_buf[8192];
    int retries = 100;
    while (retries-- > 0) {
        int len = syscall_ipc_recv(response_buf, sizeof(response_buf));
        if (len > 0) {
            /* 解析响应 */
            int32_t* resp = (int32_t*)response_buf;
            if (resp[0] == 0x05) {  /* IPC_MSG_RESPONSE */
                int data_len = resp[3];  /* payload_len */
                if (data_len <= buf_len) {
                    memcpy(buf, resp + 4, data_len);
                    return data_len;
                }
            }
        }
    }

    return -1;  /* 超时 */
}
```

### 4.3 通过动态链接调用服务

如果服务模块已通过 `SYS_dlopen` 加载到当前进程，可以直接解析并调用其导出函数：

```cpp
#include "syscall.h"

/* 定义函数指针类型 */
typedef int (*wm_create_fn)(int, const char*, int, int, int, int);

int main(void) {
    /* 动态加载窗口管理器服务 */
    long handle = syscall1(SYS_dlopen, (long)"wm_service.Wdll");
    if (handle == 0) {
        syscall_debug_log("Failed to load wm_service");
        syscall_exit(1);
    }

    /* 解析导出函数 */
    void* func_ptr;
    int ret = syscall3(SYS_dlsym, handle, (long)"wm_create_window", (long)&func_ptr);
    if (ret != 0) {
        syscall_debug_log("Failed to resolve wm_create_window");
        syscall_exit(1);
    }

    /* 调用服务函数 */
    wm_create_fn create_win = (wm_create_fn)func_ptr;
    int win = create_win(syscall_getpid(), "Dynamic App", 100, 100, 400, 300);

    return 0;
}
```

## 5. 如何使用动态库

动态库（`.Wdll`）是 WebOS 模块化架构的核心，驱动和服务都以动态库的形式实现。应用程序可以在运行时加载和调用动态库，实现功能扩展和服务调用。

### 5.1 编写自定义动态库

以下示例展示了如何编写一个简单的数学运算动态库：

```c
/* libs/math_utils/math_utils.h */
#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include <stdint.h>

/* 初始化数学工具库 */
int32_t math_utils_init(void);

/* 计算两个整数的最大公约数 */
int32_t math_gcd(int32_t a, int32_t b);

/* 计算阶乘 */
int64_t math_factorial(int32_t n);

/* 判断素数 */
int32_t math_is_prime(int32_t n);

#endif /* MATH_UTILS_H */
```

```c
/* libs/math_utils/math_utils.c */
#include "math_utils.h"
#include <emscripten.h>

/* 初始化 */
int32_t math_utils_init(void) {
    return 0;  /* 无需特殊初始化 */
}

/* 最大公约数（欧几里得算法） */
int32_t math_gcd(int32_t a, int32_t b) {
    if (a < 0) a = -a;
    if (b < 0) b = -b;
    while (b != 0) {
        int32_t t = b;
        b = a % b;
        a = t;
    }
    return a;
}

/* 阶乘 */
int64_t math_factorial(int32_t n) {
    if (n < 0) return -1;
    if (n <= 1) return 1;
    int64_t result = 1;
    for (int32_t i = 2; i <= n; i++) {
        result *= i;
    }
    return result;
}

/* 素数判断 */
int32_t math_is_prime(int32_t n) {
    if (n < 2) return 0;
    if (n < 4) return 1;
    if (n % 2 == 0) return 0;
    for (int32_t i = 3; i * i <= n; i += 2) {
        if (n % i == 0) return 0;
    }
    return 1;
}
```

### 5.2 动态库的 Makefile

```makefile
CC = emcc
CFLAGS = -s WASM=1 \
         -s SIDE_MODULE=1 \
         -s EXPORTED_FUNCTIONS='["_math_utils_init","_math_gcd","_math_factorial","_math_is_prime"]' \
         -O2 -Wall \
         -I../../libs
TARGET = ../../public/wasm/math_utils.Wdll

all: $(TARGET)

$(TARGET): math_utils.c math_utils.h
        @mkdir -p $(dir $@)
        $(CC) $(CFLAGS) $< -o $@
        python3 ../../tools/add_module_section.py $@ Wdll
        python3 ../../tools/gen_wli.py $@

clean:
        rm -f $(TARGET) ../../public/wasm/math_utils.wli
```

### 5.3 在应用中加载和使用动态库

```cpp
/* apps/math_test/main.cpp */
#include <cstdio>
#include <cstring>
#include <emscripten.h>

extern "C" {
    long syscall0(long n);
    long syscall1(long n, long a1);
    long syscall2(long n, long a1, long a2);
    long syscall3(long n, long a1, long a2, long a3);

    #define SYS_exit       0x0200
    #define SYS_debug_log  0x0700
    #define SYS_dlopen     0x0A00
    #define SYS_dlsym      0x0A01

    void syscall_exit(int code) { syscall1(SYS_exit, code); }
    void syscall_debug_log(const char* msg) {
        syscall2(SYS_debug_log, (long)msg, 0);
    }

    int wm_create_window(const char* title, int x, int y, int w, int h);
    void wm_show_window(int window_id);
    void wm_destroy_window(int window_id);
}

/* 动态库函数指针类型 */
typedef int32_t (*math_init_fn)(void);
typedef int32_t (*math_gcd_fn)(int32_t, int32_t);
typedef int64_t (*math_factorial_fn)(int32_t);
typedef int32_t (*math_is_prime_fn)(int32_t);

/* 加载的函数指针 */
static math_gcd_fn      g_gcd = nullptr;
static math_factorial_fn g_factorial = nullptr;
static math_is_prime_fn  g_is_prime = nullptr;

/**
 * 加载 math_utils 动态库并解析导出函数
 */
int load_math_utils(void) {
    /* 加载动态库 */
    long handle = syscall1(SYS_dlopen, (long)"math_utils.Wdll");
    if (handle == 0) {
        syscall_debug_log("Failed to load math_utils.Wdll");
        return -1;
    }

    /* 解析 init 函数并调用 */
    void* func_ptr;
    int ret = syscall3(SYS_dlsym, handle, (long)"math_utils_init", (long)&func_ptr);
    if (ret == 0 && func_ptr) {
        math_init_fn init = (math_init_fn)func_ptr;
        init();
    }

    /* 解析 gcd 函数 */
    ret = syscall3(SYS_dlsym, handle, (long)"math_gcd", (long)&func_ptr);
    if (ret == 0 && func_ptr) {
        g_gcd = (math_gcd_fn)func_ptr;
    }

    /* 解析 factorial 函数 */
    ret = syscall3(SYS_dlsym, handle, (long)"math_factorial", (long)&func_ptr);
    if (ret == 0 && func_ptr) {
        g_factorial = (math_factorial_fn)func_ptr;
    }

    /* 解析 is_prime 函数 */
    ret = syscall3(SYS_dlsym, handle, (long)"math_is_prime", (long)&func_ptr);
    if (ret == 0 && func_ptr) {
        g_is_prime = (math_is_prime_fn)func_ptr;
    }

    syscall_debug_log("math_utils loaded successfully");
    return 0;
}

int main(int argc, char** argv) {
    syscall_debug_log("Math Test starting...");

    /* 创建窗口 */
    int win = wm_create_window("Math Test", 100, 100, 400, 300);
    wm_show_window(win);

    /* 加载动态库 */
    if (load_math_utils() < 0) {
        syscall_exit(1);
    }

    /* 使用动态库函数 */
    if (g_gcd) {
        int32_t result = g_gcd(48, 36);
        char msg[64];
        snprintf(msg, sizeof(msg), "GCD(48,36) = %d", result);
        syscall_debug_log(msg);
    }

    if (g_factorial) {
        int64_t result = g_factorial(10);
        char msg[64];
        snprintf(msg, sizeof(msg), "10! = %ld", result);
        syscall_debug_log(msg);
    }

    if (g_is_prime) {
        int32_t result = g_is_prime(97);
        char msg[64];
        snprintf(msg, sizeof(msg), "97 is %sprime", result ? "" : "not ");
        syscall_debug_log(msg);
    }

    /* 注册主循环 */
    emscripten_set_interval([](void*) {}, 16, NULL);

    return 0;
}
```

## 6. 如何处理输入事件

WebOS 应用的输入事件（键盘、鼠标）由窗口管理器服务管理和分发。应用程序通过 WM 客户端库的事件轮询接口获取输入事件。

### 6.1 事件轮询模型

WebOS 采用非阻塞的事件轮询模型，与应用主循环紧密结合。在每个帧周期的回调函数中，应用调用 `wm_poll_event()` 检查是否有待处理的事件。如果有多条事件排队，需要循环调用直到队列为空。

```cpp
#include "wm_client.h"

void app_tick(void* arg) {
    wm_event_t event;

    /* 轮询所有待处理的事件 */
    while (wm_poll_event(g_window_id, &event) > 0) {
        switch (event.type) {
            case WM_EVENT_MOUSE:
                handle_mouse_event(&event);
                break;
            case WM_EVENT_KEY:
                handle_key_event(&event);
                break;
            case WM_EVENT_RESIZE:
                handle_resize_event(&event);
                break;
            case WM_EVENT_CLOSE:
                handle_close_event(&event);
                break;
        }
    }

    /* 处理完事件后渲染界面 */
    render_frame();
}
```

### 6.2 鼠标事件处理

```cpp
void handle_mouse_event(const wm_event_t* event) {
    int mx = event->data.mouse.x;
    int my = event->data.mouse.y;

    switch (event->data.mouse.subtype) {
        case WM_MOUSE_DOWN: {
            /* 鼠标按钮按下 - 检查是否点击了某个 UI 元素 */
            unsigned int button = event->data.mouse.button;

            /* 遍历按钮列表，检测命中 */
            for (int i = 0; i < g_num_buttons; i++) {
                Button* btn = &g_buttons[i];
                if (mx >= btn->x && mx < btn->x + btn->w &&
                    my >= btn->y && my < btn->y + btn->h) {
                    /* 命中此按钮 */
                    handle_button_click(btn);
                    break;
                }
            }
            break;
        }

        case WM_MOUSE_UP: {
            /* 鼠标按钮释放 */
            break;
        }

        case WM_MOUSE_MOVE: {
            /* 鼠标移动 - 更新悬停状态 */
            update_hover_state(mx, my);
            break;
        }

        case WM_MOUSE_WHEEL: {
            /* 鼠标滚轮 - 可用于缩放或滚动 */
            break;
        }
    }
}

void handle_button_click(const Button* btn) {
    const char* action = btn->action;

    if (strncmp(action, "digit:", 6) == 0) {
        handle_digit(action + 6);
    } else if (strncmp(action, "op:", 3) == 0) {
        handle_op(action[3]);
    } else if (strcmp(action, "eq") == 0) {
        handle_equals();
    } else if (strcmp(action, "clear") == 0) {
        handle_clear();
    }
}
```

### 6.3 键盘事件处理

```cpp
void handle_key_event(const wm_event_t* event) {
    unsigned int keycode = event->data.key.keycode;
    unsigned int modifiers = event->data.key.modifiers;

    switch (event->data.key.subtype) {
        case WM_KEY_DOWN:
            /* 按键按下 */
            if (keycode >= '0' && keycode <= '9') {
                char digit[2] = { (char)keycode, '\0' };
                handle_digit(digit);
            } else if (keycode == '.') {
                handle_digit(".");
            } else if (keycode == '+' || keycode == '-' ||
                       keycode == '*' || keycode == '/') {
                handle_op((char)keycode);
            } else if (keycode == 13) {  /* Enter */
                handle_equals();
            } else if (keycode == 27) {  /* Escape */
                handle_clear();
            } else if (keycode == 8) {   /* Backspace */
                /* 删除最后一个字符 */
                int len = strlen(g_display_text);
                if (len > 1) {
                    g_display_text[len - 1] = '\0';
                    g_display = atof(g_display_text);
                } else {
                    strcpy(g_display_text, "0");
                    g_display = 0.0;
                }
            }
            break;

        case WM_KEY_UP:
            /* 按键释放 - 通常不需要处理 */
            break;
    }
}
```

### 6.4 窗口事件处理

```cpp
void handle_resize_event(const wm_event_t* event) {
    int new_width = event->data.resize.width;
    int new_height = event->data.resize.height;

    /* 重新计算按钮布局 */
    recalculate_layout(new_width, new_height);

    /* 重新分配帧缓冲区 */
    reallocate_framebuffer(new_width, new_height);

    char msg[64];
    snprintf(msg, sizeof(msg), "Window resized to %dx%d",
             new_width, new_height);
    syscall_debug_log(msg);
}

void handle_close_event(const wm_event_t* event) {
    /* 用户点击了窗口关闭按钮 */
    syscall_debug_log("Window close requested");
    wm_destroy_window(g_window_id);
    g_window_id = 0;
    syscall_exit(0);
}
```

## 7. 如何进行 IPC 通信

IPC 通信是 WebOS 中进程间协作的核心机制。本节通过一个完整的客户端-服务端示例，展示如何使用 IPC 实现跨进程的数据请求和响应。

### 7.1 完整的 IPC 通信示例

以下示例实现了一个简单的"问候服务"——客户端向服务端发送名字，服务端返回问候消息。

**服务端代码**：

```c
/* services/greeter/greeter.c */

#include <stdint.h>
#include <string.h>
#include <emscripten.h>

extern long kernel_syscall(long, long, long, long);

#define SYS_getpid    0x0201
#define SYS_debug_log 0x0700

void debug_log(const char* msg) {
    kernel_syscall(SYS_debug_log, (long)msg, 0, 0);
}

/* 问候消息请求格式 */
typedef struct {
    uint32_t msg_type;    /* IPC_MSG_REQUEST = 0x04 */
    uint32_t request_id;
    char name[128];       /* 用户名字 */
} greeter_request_t;

/* 问候消息响应格式 */
typedef struct {
    uint32_t msg_type;    /* IPC_MSG_RESPONSE = 0x05 */
    uint32_t request_id;
    char greeting[256];   /* 问候消息 */
} greeter_response_t;

/**
 * 服务端主循环
 */
void greeter_tick(void* arg) {
    char buf[4096];
    int len;

    /* 轮询 IPC 消息 */
    extern int syscall_ipc_recv(void*, int);
    while ((len = syscall_ipc_recv(buf, sizeof(buf))) > 0) {
        if (len < sizeof(greeter_request_t)) continue;

        greeter_request_t* req = (greeter_request_t*)buf;
        if (req->msg_type != 0x04) continue;  /* 只处理请求 */

        /* 构造响应 */
        greeter_response_t resp;
        resp.msg_type = 0x05;
        resp.request_id = req->request_id;
        snprintf(resp.greeting, sizeof(resp.greeting),
                 "Hello, %s! Welcome to WebOS!", req->name);

        /* 发送响应（实际需要获取发送者 PID） */
        /* 此处简化处理 */
        debug_log("Greeting sent");
    }
}

/* 初始化函数 - 由内核调用 */
int greeter_init(void) {
    debug_log("Greeter service initialized");

    /* 注册服务名称 */
    /* ipc_register_service("greeter_service"); */

    /* 启动主循环 */
    emscripten_set_interval(greeter_tick, 16, NULL);

    return 0;
}
```

**客户端代码**：

```cpp
/* apps/greeter_client/main.cpp */

#include <cstdio>
#include <cstring>
#include <emscripten.h>

extern "C" {
    long syscall0(long n);
    long syscall1(long n, long a1);
    long syscall2(long n, long a1, long a2);
    long syscall3(long n, long a1, long a2, long a3);

    #define SYS_exit       0x0200
    #define SYS_getpid     0x0201
    #define SYS_debug_log  0x0700
    #define SYS_ipc_send   0x0800
    #define SYS_ipc_recv   0x0801

    void syscall_exit(int code) { syscall1(SYS_exit, code); }
    int syscall_getpid(void) { return (int)syscall0(SYS_getpid); }
    void syscall_debug_log(const char* msg) {
        syscall2(SYS_debug_log, (long)msg, 0);
    }
    int syscall_ipc_send(int pid, const void* msg, int len) {
        return (int)syscall3(SYS_ipc_send, (long)pid, (long)msg, (long)len);
    }
    int syscall_ipc_recv(void* buf, int len) {
        return (int)syscall2(SYS_ipc_recv, (long)buf, (long)len);
    }

    int wm_create_window(const char* title, int x, int y, int w, int h);
    void wm_show_window(int window_id);
}

/* IPC 消息格式（与服务端一致） */
typedef struct {
    uint32_t msg_type;
    uint32_t request_id;
    char name[128];
} greeter_request_t;

typedef struct {
    uint32_t msg_type;
    uint32_t request_id;
    char greeting[256];
} greeter_response_t;

static int g_window_id = 0;
static int g_greeter_pid = 6;  /* 假设问候服务的 PID */
static uint32_t g_next_request_id = 1;

/**
 * 向问候服务发送请求
 */
void send_greeting_request(const char* name) {
    greeter_request_t req;
    req.msg_type = 0x04;  /* IPC_MSG_REQUEST */
    req.request_id = g_next_request_id++;
    strncpy(req.name, name, sizeof(req.name) - 1);
    req.name[sizeof(req.name) - 1] = '\0';

    int result = syscall_ipc_send(g_greeter_pid, &req, sizeof(req));
    if (result < 0) {
        syscall_debug_log("Failed to send greeting request");
    } else {
        char msg[128];
        snprintf(msg, sizeof(msg),
                 "Request sent: greeting for '%s'", name);
        syscall_debug_log(msg);
    }
}

/**
 * 检查问候服务的响应
 */
void check_greeting_response(void) {
    char buf[4096];
    int len;

    while ((len = syscall_ipc_recv(buf, sizeof(buf))) > 0) {
        if (len < sizeof(greeter_response_t)) continue;

        greeter_response_t* resp = (greeter_response_t*)buf;
        if (resp->msg_type != 0x05) continue;  /* 只处理响应 */

        /* 收到问候响应 */
        syscall_debug_log(resp->greeting);
    }
}

/**
 * 主循环回调
 */
void client_tick(void* arg) {
    check_greeting_response();
}

int main(void) {
    syscall_debug_log("Greeter Client starting...");

    /* 创建窗口 */
    g_window_id = wm_create_window("Greeter Client", 150, 150, 400, 200);
    wm_show_window(g_window_id);

    /* 发送第一个问候请求 */
    send_greeting_request("WebOS User");

    /* 注册主循环 */
    emscripten_set_interval(client_tick, 16, NULL);

    return 0;
}
```

### 7.2 IPC 通信的最佳实践

**请求 ID 匹配**：每个请求都应携带唯一的 `request_id`，响应中必须返回相同的 `request_id`。客户端通过匹配 ID 将请求和响应关联起来。如果同时发送多个请求，ID 匹配可以确保每个响应被正确路由到对应的处理逻辑。

**消息大小限制**：单条 IPC 消息的最大大小为 `IPC_MAX_MSG_SIZE`（4096 字节）。如果需要传输更大的数据，应将数据分割为多条消息，或使用共享内存进行零拷贝传输。

**非阻塞接收**：始终使用非阻塞的 `ipc_recv` 接收消息，避免在主循环中阻塞。在协作式调度模型中，阻塞等待会导致整个系统停止响应。

**超时处理**：对于同步 IPC 调用，始终设置合理的超时上限，避免因服务端故障导致客户端无限等待。可以在主循环中计数帧数来检测超时。

**错误恢复**：当 IPC 通信失败时（如目标进程不存在、消息队列满），应用程序应有合理的错误恢复机制，如重试、降级处理或用户提示。
