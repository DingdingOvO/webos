# WebOS 系统服务 API 文档

## 1. 窗口管理器服务 API

窗口管理器（Window Manager，简称 WM）是 WebOS 中最核心的系统服务之一，负责管理所有应用程序窗口的创建、销毁、移动、调整大小、焦点切换和屏幕合成渲染。窗口管理器服务以 `.Wdll` 动态链接库的形式实现，由内核在启动时加载，应用程序通过 IPC 消息或直接函数调用（动态链接后）与其交互。

**模块名**：`wm_service.Wdll`
**初始化函数**：`wm_service_init()`
**最大窗口数**：64

### API 函数参考

#### wm_service_init

```c
int wm_service_init(void);
```

初始化窗口管理器服务。此函数在系统启动时由内核调用一次，完成窗口管理器的内部数据结构初始化，包括窗口槽位数组、焦点状态、Z-Order 链表等。初始化成功后，窗口管理器进入就绪状态，可以接受来自应用程序的窗口操作请求。此函数不应被应用程序直接调用。

| 参数 | 类型 | 说明 |
|------|------|------|
| 无 | — | — |
| **返回值** | `int` | 0 表示成功，负数表示错误 |

#### wm_create_window

```c
int wm_create_window(int owner_pid, const char* title,
                     int x, int y, int w, int h);
```

创建一个新的应用程序窗口。窗口管理器在内部窗口表中分配一个空闲槽位，记录窗口的标题、位置、大小、所有者进程等信息，并为窗口创建对应的 RGBA 帧缓冲区。新创建的窗口默认不可见，需要调用 `wm_show_window()` 使其显示。创建成功后返回窗口 ID（正整数），失败返回 0 或负值。

| 参数 | 类型 | 说明 |
|------|------|------|
| `owner_pid` | `int` | 创建窗口的进程 ID |
| `title` | `const char*` | 窗口标题字符串 |
| `x` | `int` | 窗口左上角 X 坐标（像素） |
| `y` | `int` | 窗口左上角 Y 坐标（像素） |
| `w` | `int` | 窗口宽度（像素） |
| `h` | `int` | 窗口高度（像素） |
| **返回值** | `int` | 窗口 ID（>0），0 表示创建失败 |

**使用示例**：

```c
/* 通过 WM 客户端库创建窗口 */
int win = wm_create_window("Calculator", 200, 200, 300, 340);
if (win <= 0) {
    syscall_debug_log("Failed to create window");
    syscall_exit(1);
}
wm_show_window(win);
```

#### wm_destroy_window

```c
void wm_destroy_window(int window_id);
```

销毁指定的窗口。窗口管理器释放该窗口占用的所有资源，包括帧缓冲区内存、窗口槽位等。如果被销毁的窗口当前拥有焦点，焦点将自动转移到 Z-Order 中下一个可见窗口。销毁后，窗口 ID 不再有效，后续使用该 ID 的操作将返回错误。

| 参数 | 类型 | 说明 |
|------|------|------|
| `window_id` | `int` | 要销毁的窗口 ID |
| **返回值** | `void` | 无 |

#### wm_show_window

```c
void wm_show_window(int window_id);
```

将指定窗口设置为可见状态。窗口变为可见后，将在下次渲染时被绘制到屏幕上。新创建的窗口默认不可见，必须调用此函数后才能显示。如果窗口已经可见，此调用不会有任何效果。

| 参数 | 类型 | 说明 |
|------|------|------|
| `window_id` | `int` | 要显示的窗口 ID |
| **返回值** | `void` | 无 |

#### wm_hide_window

```c
void wm_hide_window(int window_id);
```

将指定窗口设置为隐藏状态。隐藏的窗口不会被绘制到屏幕上，但其所有状态和资源都被保留，可以通过 `wm_show_window()` 再次显示。这与 `wm_destroy_window()` 不同——隐藏只是暂时不可见，而销毁是永久释放资源。

| 参数 | 类型 | 说明 |
|------|------|------|
| `window_id` | `int` | 要隐藏的窗口 ID |
| **返回值** | `void` | 无 |

#### wm_move_window

```c
void wm_move_window(int window_id, int x, int y);
```

将窗口移动到新的位置。位置坐标是窗口左上角相对于屏幕左上角的像素偏移。移动操作立即生效，下次渲染时窗口将出现在新位置。如果窗口是可见的，移动后屏幕会自动重绘。

| 参数 | 类型 | 说明 |
|------|------|------|
| `window_id` | `int` | 要移动的窗口 ID |
| `x` | `int` | 新的 X 坐标 |
| `y` | `int` | 新的 Y 坐标 |
| **返回值** | `void` | 无 |

#### wm_resize_window

```c
void wm_resize_window(int window_id, int w, int h);
```

调整窗口的大小。调整大小时，窗口管理器会重新分配帧缓冲区以匹配新的尺寸，原有帧缓冲区的内容会被丢弃。应用程序在收到窗口大小变化事件后，需要重新绘制整个窗口内容。建议设置最小窗口尺寸限制，避免窗口过小导致界面不可用。

| 参数 | 类型 | 说明 |
|------|------|------|
| `window_id` | `int` | 要调整大小的窗口 ID |
| `w` | `int` | 新的宽度（像素） |
| `h` | `int` | 新的高度（像素） |
| **返回值** | `void` | 无 |

#### wm_set_focus

```c
void wm_set_focus(int window_id);
```

将输入焦点设置到指定窗口。获得焦点的窗口会出现在所有其他窗口之上（Z-Order 顶部），并且接收所有键盘和鼠标输入事件。之前拥有焦点的窗口会收到 `WM_EVENT_BLUR` 事件，新获得焦点的窗口会收到 `WM_EVENT_FOCUS` 事件。

| 参数 | 类型 | 说明 |
|------|------|------|
| `window_id` | `int` | 要获取焦点的窗口 ID |
| **返回值** | `void` | 无 |

#### wm_get_focused_window

```c
int wm_get_focused_window(void);
```

获取当前拥有输入焦点的窗口 ID。如果没有窗口拥有焦点（例如所有窗口都被隐藏），返回 0。

| 参数 | 类型 | 说明 |
|------|------|------|
| 无 | — | — |
| **返回值** | `int` | 焦点窗口 ID，或 0（无焦点窗口） |

#### wm_get_window_count

```c
int wm_get_window_count(void);
```

获取当前存在的窗口总数，包括可见和隐藏的窗口。此函数主要用于调试和系统状态显示。

| 参数 | 类型 | 说明 |
|------|------|------|
| 无 | — | — |
| **返回值** | `int` | 窗口总数（≥0） |

#### wm_render_all

```c
void wm_render_all(int surface);
```

将所有可见窗口合成渲染到指定的 GPU 表面上。窗口管理器按照 Z-Order 从后到前依次绘制每个可见窗口，实现窗口叠加效果。此函数通常由系统渲染循环在每个帧周期调用一次。

| 参数 | 类型 | 说明 |
|------|------|------|
| `surface` | `int` | 目标 GPU 渲染表面 ID |
| **返回值** | `void` | 无 |

### WM 客户端库

应用程序通常不直接调用 WM 服务的函数，而是通过 `libs/wm_client.h` 提供的客户端库间接访问。客户端库封装了 IPC 通信细节，提供了更友好的 API 接口：

```c
/* 创建窗口（简化版，自动获取当前 PID） */
int wm_create_window(const char* title, int x, int y, int width, int height);

/* 显示/隐藏窗口 */
void wm_show_window(int window_id);
void wm_hide_window(int window_id);

/* 移动和调整窗口 */
void wm_move_window(int window_id, int x, int y);
void wm_resize_window(int window_id, int width, int height);

/* 销毁窗口 */
void wm_destroy_window(int window_id);

/* 事件处理 */
wm_event_t wm_wait_event(int window_id);    /* 阻塞等待事件 */
int wm_poll_event(int window_id, wm_event_t* event); /* 非阻塞轮询事件 */
```

### 窗口事件类型

```c
#define WM_EVENT_CLOSE      1    /* 窗口关闭按钮被点击 */
#define WM_EVENT_RESIZE     2    /* 窗口大小变化 */
#define WM_EVENT_MOVE       3    /* 窗口位置变化 */
#define WM_EVENT_FOCUS      4    /* 窗口获得焦点 */
#define WM_EVENT_BLUR       5    /* 窗口失去焦点 */
#define WM_EVENT_MOUSE      6    /* 鼠标事件 */
#define WM_EVENT_KEY        7    /* 键盘事件 */

/* 鼠标事件子类型 */
#define WM_MOUSE_MOVE       0    /* 鼠标移动 */
#define WM_MOUSE_DOWN       1    /* 鼠标按钮按下 */
#define WM_MOUSE_UP         2    /* 鼠标按钮释放 */
#define WM_MOUSE_WHEEL      3    /* 鼠标滚轮 */

/* 键盘事件子类型 */
#define WM_KEY_DOWN         0    /* 按键按下 */
#define WM_KEY_UP           1    /* 按键释放 */
```

### 事件结构体

```c
typedef struct {
    int type;                          /* 事件类型 */
    int window;                        /* 窗口 ID */
    union {
        struct { int width, height; } resize;        /* WM_EVENT_RESIZE */
        struct { int x, y; } move;                   /* WM_EVENT_MOVE */
        struct {
            unsigned int button;
            int x, y;
            unsigned int subtype;
        } mouse;                                      /* WM_EVENT_MOUSE */
        struct {
            unsigned int keycode;
            unsigned int modifiers;
            unsigned int subtype;
        } key;                                        /* WM_EVENT_KEY */
    } data;
} wm_event_t;
```

## 2. 文件系统服务 API

文件系统服务（Filesystem Service）提供了虚拟文件系统（VFS）的访问能力，建立在存储驱动之上，使用 IndexedDB 作为底层持久化存储。文件系统服务支持文件打开/关闭、读写、目录创建/删除等 POSIX 风格的操作，并通过文件描述符（FD）管理打开的文件。

**模块名**：`fs_service.Wdll`
**初始化函数**：`fs_service_init()`
**最大 FD 数**：64
**最大挂载点数**：16

### API 函数参考

#### fs_service_init

```c
int fs_service_init(void);
```

初始化文件系统服务。此函数完成 FD 表的初始化、挂载点表的创建、以及存储驱动的初始化调用。系统启动时由内核调用一次。初始化过程中，文件系统服务会调用存储驱动的 `storage_driver_init()` 函数建立与 IndexedDB 的连接。

| 参数 | 类型 | 说明 |
|------|------|------|
| 无 | — | — |
| **返回值** | `int` | 0 表示成功，负数表示错误 |

#### fs_open

```c
int fs_open(int owner_pid, const char* path, int flags);
```

打开指定路径的文件，返回文件描述符。文件描述符是一个非负整数，用于后续的读写操作。每个打开的文件在 FD 表中占有一个槽位，记录了文件的路径、打开模式、当前偏移量和所有者进程 ID。当进程退出时，其拥有的所有 FD 会自动关闭。

| 参数 | 类型 | 说明 |
|------|------|------|
| `owner_pid` | `int` | 打开文件的进程 ID |
| `path` | `const char*` | 文件路径 |
| `flags` | `int` | 打开标志的组合（O_RDONLY、O_WRONLY 等） |
| **返回值** | `int` | 文件描述符（≥0），负数表示错误 |

**打开标志**：

| 标志 | 值 | 说明 |
|------|-----|------|
| `O_RDONLY` | 0 | 只读模式 |
| `O_WRONLY` | 1 | 只写模式 |
| `O_RDWR` | 2 | 读写模式 |
| `O_CREAT` | 4 | 文件不存在时创建 |
| `O_TRUNC` | 8 | 截断文件为空 |

**使用示例**：

```c
/* 以读写模式打开配置文件，不存在则创建 */
int fd = fs_open(getpid(), "/config/app.conf", O_RDWR | O_CREAT);
if (fd < 0) {
    syscall_debug_log("Failed to open config file");
    return;
}

/* 读取文件内容 */
char buffer[1024];
int bytes = fs_read(fd, buffer, sizeof(buffer));
if (bytes > 0) {
    buffer[bytes] = '\0';
    /* 处理配置数据... */
}

/* 关闭文件 */
fs_close(fd);
```

#### fs_read

```c
int fs_read(int fd, void* buf, int len);
```

从文件描述符指向的文件中读取数据。读取从当前文件偏移量开始，读取完成后偏移量自动增加实际读取的字节数。如果到达文件末尾，返回 0。

| 参数 | 类型 | 说明 |
|------|------|------|
| `fd` | `int` | 文件描述符 |
| `buf` | `void*` | 接收数据的缓冲区 |
| `len` | `int` | 请求读取的字节数 |
| **返回值** | `int` | 实际读取的字节数（≥0），负数表示错误 |

#### fs_write

```c
int fs_write(int fd, const void* buf, int len);
```

向文件描述符指向的文件写入数据。写入从当前文件偏移量开始，写入完成后偏移量自动增加实际写入的字节数。如果文件以 `O_APPEND` 模式打开，写入总是在文件末尾进行。

| 参数 | 类型 | 说明 |
|------|------|------|
| `fd` | `int` | 文件描述符 |
| `buf` | `const void*` | 要写入的数据 |
| `len` | `int` | 要写入的字节数 |
| **返回值** | `int` | 实际写入的字节数（≥0），负数表示错误 |

#### fs_close

```c
int fs_close(int fd);
```

关闭文件描述符，释放 FD 表中的槽位。关闭后，该文件描述符不再有效。如果有未刷新的写入数据，关闭操作会确保数据被持久化到 IndexedDB 中。

| 参数 | 类型 | 说明 |
|------|------|------|
| `fd` | `int` | 要关闭的文件描述符 |
| **返回值** | `int` | 0 表示成功，负数表示错误 |

#### fs_mkdir

```c
int fs_mkdir(const char* path);
```

创建新目录。如果父目录不存在，操作将失败。目录路径使用 Unix 风格的路径分隔符（`/`），根目录为 `/`。

| 参数 | 类型 | 说明 |
|------|------|------|
| `path` | `const char*` | 要创建的目录路径 |
| **返回值** | `int` | 0 表示成功，负数表示错误 |

#### fs_unlink

```c
int fs_unlink(const char* path);
```

删除指定路径的文件或空目录。如果目标是目录且不为空，操作将失败。

| 参数 | 类型 | 说明 |
|------|------|------|
| `path` | `const char*` | 要删除的路径 |
| **返回值** | `int` | 0 表示成功，负数表示错误 |

#### fs_exists

```c
int fs_exists(const char* path);
```

检查指定路径的文件或目录是否存在。此函数不打开文件，仅检查存在性，因此不会占用 FD 表的槽位。

| 参数 | 类型 | 说明 |
|------|------|------|
| `path` | `const char*` | 要检查的路径 |
| **返回值** | `int` | 1 表示存在，0 表示不存在 |

### 目录结构约定

WebOS 的虚拟文件系统遵循以下目录结构约定：

```
/                        根目录
├── apps/                已安装的应用程序
│   ├── com.os.calculator/
│   │   ├── app.json     应用清单
│   │   └── calculator.wex  应用模块
│   └── ...
├── config/              系统配置
│   ├── settings.json    全局设置
│   └── wm.conf          窗口管理器配置
├── data/                应用数据
│   └── com.os.paint/
│       └── saved.dat    画板保存文件
├── tmp/                 临时文件
└── system/              系统文件
    ├── kernel.wasm      内核模块
    └── bootloader.wasm  引导加载器
```

## 3. 网络服务 API

网络服务（Network Service）提供了 HTTP 请求能力，建立在网络驱动之上，使用浏览器的 Fetch API 作为底层实现。网络服务支持 HTTP GET 和 POST 请求，并提供了连接管理功能，支持最多 16 个并发网络连接。

**模块名**：`net_service.Wdll`
**初始化函数**：`net_service_init()`
**最大并发连接数**：16

### API 函数参考

#### net_service_init

```c
int net_service_init(void);
```

初始化网络服务。此函数完成连接管理表的初始化，并调用网络驱动的初始化函数建立与 Fetch API 的桥接。初始化完成后，网络服务可以接受 HTTP 请求。

| 参数 | 类型 | 说明 |
|------|------|------|
| 无 | — | — |
| **返回值** | `int` | 0 表示成功，负数表示错误 |

#### net_http_get

```c
int net_http_get(const char* url, void* response_buf, int buf_len);
```

发起 HTTP GET 请求。此函数向指定的 URL 发送 GET 请求，并将响应体写入用户提供的缓冲区中。请求是同步执行的——函数会阻塞直到收到响应或超时。由于 WebAssembly 的单线程特性，长时间的 HTTP 请求可能会影响系统的响应性，建议在独立的服务进程中执行网络操作。

| 参数 | 类型 | 说明 |
|------|------|------|
| `url` | `const char*` | 请求的 URL 字符串 |
| `response_buf` | `void*` | 接收响应体的缓冲区 |
| `buf_len` | `int` | 缓冲区大小（字节） |
| **返回值** | `int` | 实际接收的字节数（≥0），负数表示错误 |

**使用示例**：

```c
/* 获取远程 API 数据 */
char response[8192];
int bytes = net_http_get("https://api.example.com/data", response, sizeof(response));
if (bytes > 0) {
    response[bytes] = '\0';
    /* 解析 JSON 响应... */
    syscall_debug_log("API request successful");
} else {
    syscall_debug_log("API request failed");
}
```

#### net_http_post

```c
int net_http_post(const char* url, const void* body, int body_len, void* response_buf);
```

发起 HTTP POST 请求。此函数向指定的 URL 发送 POST 请求，携带请求体数据，并将响应体写入用户提供的缓冲区。请求的 Content-Type 默认为 `application/json`。

| 参数 | 类型 | 说明 |
|------|------|------|
| `url` | `const char*` | 请求的 URL 字符串 |
| `body` | `const void*` | 请求体数据 |
| `body_len` | `int` | 请求体数据长度 |
| `response_buf` | `void*` | 接收响应体的缓冲区 |
| **返回值** | `int` | 响应体字节数（≥0），负数表示错误 |

**使用示例**：

```c
/* 提交数据到远程 API */
const char* json_data = "{\"action\":\"login\",\"user\":\"admin\"}";
char response[4096];
int bytes = net_http_post("https://api.example.com/auth",
                          json_data, strlen(json_data), response);
if (bytes > 0) {
    /* 处理登录响应... */
}
```

#### net_open_connection

```c
int net_open_connection(int owner_pid, const char* url);
```

打开一个持久化的网络连接。与 `net_http_get` 的一次性请求不同，持久化连接可以复用 TCP 连接进行多次数据传输，减少连接建立的开销。此功能主要用于 WebSocket 或长轮询场景。

| 参数 | 类型 | 说明 |
|------|------|------|
| `owner_pid` | `int` | 拥有连接的进程 ID |
| `url` | `const char*` | 连接目标 URL |
| **返回值** | `int` | 连接 ID（≥0），负数表示错误 |

#### net_close_connection

```c
void net_close_connection(int conn_id);
```

关闭指定的网络连接，释放连接管理表中的槽位。关闭后，该连接 ID 不再有效。

| 参数 | 类型 | 说明 |
|------|------|------|
| `conn_id` | `int` | 要关闭的连接 ID |
| **返回值** | `void` | 无 |

## 4. 应用商店服务 API

应用商店服务（App Store Service）提供了应用程序的注册、发现、安装和卸载功能。服务在初始化时会注册系统内置的应用程序（计算器、画板、浏览器、应用商店客户端），用户可以通过服务接口查询已注册的应用、搜索应用、以及安装或卸载第三方应用。

**模块名**：`appstore.Wdll`
**初始化函数**：`appstore_init()`
**最大注册应用数**：128

### API 函数参考

#### appstore_init

```c
int appstore_init(void);
```

初始化应用商店服务。此函数完成应用注册表的初始化，并将系统内置应用（Calculator、Paint、Browser、App Store）注册到表中。每个内置应用包含名称、描述、版本号、入口点、图标标识和分类信息。初始化完成后，应用商店可以接受来自客户端的查询和操作请求。

| 参数 | 类型 | 说明 |
|------|------|------|
| 无 | — | — |
| **返回值** | `int` | 0 表示成功，负数表示错误 |

#### appstore_register

```c
int appstore_register(const char* name, const char* desc, const char* version,
                      const char* entry, const char* icon, const char* category);
```

注册一个新的应用到应用商店。注册时需要提供应用的完整信息，包括名称、描述、版本号、入口模块路径、图标标识和分类。注册成功后，应用会出现在搜索结果和分类列表中，用户可以通过应用商店客户端浏览和安装。

| 参数 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | 应用显示名称 |
| `desc` | `const char*` | 应用描述 |
| `version` | `const char*` | 语义化版本号（如 "1.0.0"） |
| `entry` | `const char*` | 入口模块路径（如 "calculator.wex"） |
| `icon` | `const char*` | 图标标识 |
| `category` | `const char*` | 应用分类 |
| **返回值** | `int` | 应用 ID（≥0），负数表示错误 |

**使用示例**：

```c
/* 注册新的第三方应用 */
int app_id = appstore_register(
    "TextEditor",                          /* 名称 */
    "A simple text editor",                /* 描述 */
    "1.2.0",                               /* 版本 */
    "text_editor.wex",                     /* 入口 */
    "text-editor",                         /* 图标 */
    "Productivity"                         /* 分类 */
);
if (app_id < 0) {
    syscall_debug_log("Failed to register app");
}
```

#### appstore_get_count

```c
int appstore_get_count(void);
```

获取已注册的应用总数，包括内置应用和第三方应用。

| 参数 | 类型 | 说明 |
|------|------|------|
| 无 | — | — |
| **返回值** | `int` | 应用总数（≥0） |

#### appstore_find_by_name

```c
int appstore_find_by_name(const char* name);
```

按名称查找应用。搜索是精确匹配的，区分大小写。如果找到匹配的应用，返回其应用 ID；如果未找到，返回 -1。

| 参数 | 类型 | 说明 |
|------|------|------|
| `name` | `const char*` | 要查找的应用名称 |
| **返回值** | `int` | 应用 ID（≥0），或 -1（未找到） |

#### appstore_install

```c
int appstore_install(int app_id);
```

安装指定应用。安装过程包括：验证应用 ID 的有效性、检查应用模块文件是否存在、将应用标记为已安装状态。已安装的应用会出现在系统启动器和开始菜单中。如果应用已经安装，此调用不会重复安装，直接返回成功。

| 参数 | 类型 | 说明 |
|------|------|------|
| `app_id` | `int` | 要安装的应用 ID |
| **返回值** | `int` | 0 表示成功，负数表示错误 |

**使用示例**：

```c
/* 查找并安装应用 */
int app_id = appstore_find_by_name("Calculator");
if (app_id >= 0) {
    int result = appstore_install(app_id);
    if (result == 0) {
        syscall_debug_log("Calculator installed successfully");
    }
}
```

#### appstore_uninstall

```c
int appstore_uninstall(int app_id);
```

卸载指定应用。卸载过程包括：将应用标记为未安装状态、从启动器和开始菜单中移除。应用的注册信息（名称、描述等）不会被删除，只是标记为未安装，方便日后重新安装。内置应用不允许卸载，尝试卸载内置应用将返回 `ERR_PERM` 错误。

| 参数 | 类型 | 说明 |
|------|------|------|
| `app_id` | `int` | 要卸载的应用 ID |
| **返回值** | `int` | 0 表示成功，负数表示错误 |

#### appstore_is_installed

```c
int appstore_is_installed(int app_id);
```

检查指定应用是否已安装。

| 参数 | 类型 | 说明 |
|------|------|------|
| `app_id` | `int` | 要检查的应用 ID |
| **返回值** | `int` | 1 表示已安装，0 表示未安装，负数表示错误 |

## 5. IPC 协议与消息格式

WebOS 的系统服务通过 IPC（进程间通信）协议与应用程序交互。IPC 协议定义了消息的类型、格式和传输规则，确保服务请求和响应的可靠传递。

### 消息类型定义

```c
#define IPC_MSG_SYSCALL   0x01   /* 系统调用请求 */
#define IPC_MSG_SIGNAL    0x02   /* 信号通知 */
#define IPC_MSG_DATA      0x03   /* 数据传输 */
#define IPC_MSG_REQUEST   0x04   /* 服务请求 */
#define IPC_MSG_RESPONSE  0x05   /* 服务响应 */
#define IPC_MSG_EVENT     0x06   /* 系统事件 */
```

### 服务请求消息格式

当应用程序向服务发送请求时，消息体遵循以下结构：

```c
/* 通用服务请求头 */
typedef struct {
    uint32_t msg_type;       /* 消息类型，固定为 IPC_MSG_REQUEST (0x04) */
    uint32_t request_id;     /* 请求 ID，用于匹配请求和响应 */
    uint32_t service_op;     /* 服务操作码 */
    uint32_t payload_len;    /* 负载数据长度 */
    /* 负载数据紧跟在头部之后 */
} ipc_request_header_t;
```

### 服务响应消息格式

服务处理完请求后，返回的响应消息遵循以下结构：

```c
/* 通用服务响应头 */
typedef struct {
    uint32_t msg_type;       /* 消息类型，固定为 IPC_MSG_RESPONSE (0x05) */
    uint32_t request_id;     /* 对应的请求 ID */
    int32_t  result;         /* 操作结果码，0 表示成功 */
    uint32_t payload_len;    /* 负载数据长度 */
    /* 负载数据紧跟在头部之后 */
} ipc_response_header_t;
```

### 窗口管理器 IPC 操作码

| 操作码 | 名称 | 请求负载 | 响应负载 |
|-------|------|---------|---------|
| 0x01 | WM_OP_CREATE | `{pid, title, x, y, w, h}` | `{window_id}` |
| 0x02 | WM_OP_DESTROY | `{window_id}` | 无 |
| 0x03 | WM_OP_SHOW | `{window_id}` | 无 |
| 0x04 | WM_OP_HIDE | `{window_id}` | 无 |
| 0x05 | WM_OP_MOVE | `{window_id, x, y}` | 无 |
| 0x06 | WM_OP_RESIZE | `{window_id, w, h}` | 无 |
| 0x07 | WM_OP_SET_FOCUS | `{window_id}` | 无 |
| 0x08 | WM_OP_GET_FOCUSED | 无 | `{window_id}` |
| 0x09 | WM_OP_GET_COUNT | 无 | `{count}` |
| 0x0A | WM_OP_RENDER_ALL | `{surface_id}` | 无 |

### 文件系统 IPC 操作码

| 操作码 | 名称 | 请求负载 | 响应负载 |
|-------|------|---------|---------|
| 0x10 | FS_OP_OPEN | `{pid, path, flags}` | `{fd}` |
| 0x11 | FS_OP_READ | `{fd, len}` | `{data}` |
| 0x12 | FS_OP_WRITE | `{fd, data, len}` | `{bytes_written}` |
| 0x13 | FS_OP_CLOSE | `{fd}` | 无 |
| 0x14 | FS_OP_MKDIR | `{path}` | 无 |
| 0x15 | FS_OP_UNLINK | `{path}` | 无 |
| 0x16 | FS_OP_EXISTS | `{path}` | `{exists}` |

## 6. 服务发现机制

服务发现是 WebOS IPC 体系的重要功能，允许应用程序在不知道服务进程 PID 的情况下，通过服务名称找到对应的服务进程并发送消息。这种间接寻址方式解耦了服务的提供者和消费者，使得服务可以在不同的进程中运行，甚至可以在运行时迁移到新的进程而不影响客户端。

### 服务注册

每个服务进程在启动时，通过 IPC 子系统的 `ipc_register_service()` 函数向内核注册自己的服务名称。内核维护一个全局的服务名称到 PID 的映射表：

```
  服务注册表（内核维护）
  ══════════════════════

  ┌───────────────────┬──────────┐
  │    服务名称        │   PID    │
  ├───────────────────┼──────────┤
  │  wm_service       │    2     │
  │  fs_service       │    3     │
  │  net_service      │    4     │
  │  appstore         │    5     │
  └───────────────────┴──────────┘
```

### 服务查找

应用程序通过 `ipc_call_service(service_name, ...)` 发送请求时，内核首先在服务注册表中查找指定名称的服务 PID，然后将请求路由到该 PID 对应的进程。如果服务名称不存在，返回 `ERR_NOENT` 错误。

### 服务启动顺序

内核按照以下顺序启动和注册系统服务：

```
1. 内核启动完成
2. 加载 GPU 驱动 → 加载输入驱动 → 加载存储驱动 → 加载网络驱动
3. 启动窗口管理器服务 → 注册 "wm_service"
4. 启动文件系统服务 → 注册 "fs_service"
5. 启动网络服务 → 注册 "net_service"
6. 启动应用商店服务 → 注册 "appstore"
7. 启动系统应用（Calculator、Paint、Browser 等）
```

所有系统服务在应用程序启动之前完成注册，确保应用程序可以在启动时立即通过服务发现机制访问所需的服务。如果某个服务启动失败，依赖该服务的应用程序将在尝试访问时收到 `ERR_NOENT` 错误，但不会导致系统崩溃。
