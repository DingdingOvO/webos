# WebOS

**基于 C/WebAssembly 的浏览器操作系统**

WebOS 是一个运行在浏览器中的微型操作系统，完全基于 WebAssembly 构建。系统采用纯 C 语言编写内核、驱动和系统服务，用户态应用使用 C++ 开发，TypeScript 仅作为浏览器宿主入口层。

## 核心设计原则

- **完全动态化**：无静态链接，所有模块运行时动态加载
- **模块类型自描述**：通过 WebAssembly 自定义段 `module_type` 标识模块类型
- **纯 C 系统**：Bootloader、内核、驱动、系统服务全部用 C 语言
- **C++ 应用**：用户态应用使用 C++ 开发
- **TypeScript 宿主**：仅作为浏览器入口，负责加载 bootloader.wex 并提供基础运行时桥接

## 架构概览

```
┌─────────────────────────────────────┐
│          用户应用 (C++ .wex)         │
│   Calculator │ Paint │ Browser │ ... │
├─────────────────────────────────────┤
│         系统服务 (C .Wdll)           │
│  窗口管理器 │ 文件系统 │ 网络 │ 商店   │
├─────────────────────────────────────┤
│         设备驱动 (C .Wdll)           │
│   GPU │ 输入 │ 存储 │ 网络           │
├─────────────────────────────────────┤
│          内核 (C .wex)              │
│  进程管理 │ 内存 │ IPC │ 系统调用     │
├─────────────────────────────────────┤
│         Bootloader (C .wex)         │
│    GPU初始化 │ 加载内核 │ 传递控制     │
├─────────────────────────────────────┤
│       TypeScript 宿主 (浏览器入口)    │
│  动态加载器 │ 运行时桥接 │ WebGPU      │
├─────────────────────────────────────┤
│          浏览器 (WebAssembly)        │
└─────────────────────────────────────┘
```

## 文件类型规范

| 后缀 | 含义 | 格式 | 加载行为 |
|------|------|------|---------|
| `.wex` | 可执行程序 | WASM + module_type="wex" | 实例化后调用 `_start` 或 `main` |
| `.Wdll` | 动态链接库 | WASM + module_type="Wdll" | 实例化，缓存 exports |
| `.wli` | 接口描述文件 | JSON | 不实例化，解析符号表 |
| `.wasm` | 通用 WASM | 标准 WASM | 默认按 .Wdll 处理 |

加载器识别优先级：读取文件 → 检查 WASM 魔数 → 解析自定义段 `module_type` → 若不存在，按 `.Wdll` 处理。

## 项目结构

```
webos/
├── host/                 # TypeScript 宿主（浏览器入口）
│   └── src/
│       ├── main.ts           # 入口：加载 bootloader.wex
│       ├── dynamic_loader.ts # 动态模块加载器
│       ├── runtime_bridge.ts # JS 运行时桥接
│       └── types.ts          # 类型定义
├── bootloader/           # C 引导程序
│   └── src/
│       ├── main.c            # 入口：初始化 GPU，加载内核
│       ├── loader.c/h        # 模块加载接口
│       └── gpu_init.c/h      # GPU 初始化
├── kernel/               # C 内核
│   └── src/
│       ├── main.c            # 内核入口，初始化所有子系统
│       ├── memory.c/h        # 页分配器 + 堆分配器
│       ├── process.c/h       # 进程控制块与进程表
│       ├── scheduler.c/h     # 轮转调度器
│       ├── ipc.c/h           # 进程间通信
│       ├── syscall.c/h       # 系统调用表与分发
│       ├── dynlink.c/h       # 动态链接器
│       └── host_func.h       # JS 宿主函数声明
├── drivers/              # C 设备驱动 (.Wdll)
│   ├── gpu/                  # GPU 驱动（WebGPU 封装）
│   ├── input/                # 输入驱动（键盘/鼠标）
│   ├── storage/              # 存储驱动（IndexedDB）
│   └── network/              # 网络驱动（Fetch API）
├── services/             # C 系统服务 (.Wdll)
│   ├── window_manager/       # 窗口管理器
│   ├── filesystem/           # 文件系统服务
│   ├── network_service/      # 网络服务
│   └── appstore/             # 应用商店服务
├── apps/                 # C++ 应用程序 (.wex)
│   ├── calculator/           # 计算器
│   ├── paint/                # 画图
│   ├── browser/              # 浏览器
│   └── appstore_client/      # 应用商店客户端
├── desktop/              # TypeScript 桌面 UI
│   └── src/
│       ├── renderer.ts       # WebGPU/Canvas2D 渲染器
│       ├── desktop.ts        # 桌面环境
│       ├── taskbar.ts        # 任务栏
│       └── start_menu.ts     # 开始菜单
├── libs/                 # 共享库与头文件
│   ├── syscall.h             # 系统调用桩函数
│   ├── wm_client.h/c         # 窗口管理器客户端库
│   └── module_types.h        # 模块类型定义
├── tools/                # 构建工具
│   ├── add_module_section.py # 添加 WASM 自定义段
│   ├── gen_wli.py            # 生成 .wli 接口文件
│   └── serve.py              # 开发服务器
├── docs/                 # 文档
│   ├── architecture.md       # 系统架构
│   ├── module_spec.md        # 模块格式规范
│   ├── building.md           # 构建与运行
│   ├── api_syscall.md        # 系统调用 API
│   ├── api_services.md       # 系统服务 API
│   ├── developer_guide.md    # 应用开发指南
│   └── app_store.md          # 应用商店规范
├── public/               # 静态资源
│   ├── wasm/                 # 编译后的 WASM 模块
│   └── wallpapers/           # 壁纸
├── Makefile              # 顶层构建文件
└── README.md
```

## 快速开始

### 环境要求

- [Emscripten SDK](https://emscripten.org/) (emcc, em++)
- Python 3.8+
- Node.js 18+

### 构建与运行

```bash
# 安装 Emscripten
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk && ./emsdk install latest && ./emsdk activate latest
source ./emsdk_env.sh

# 克隆项目
git clone https://github.com/DingdingOvO/webos.git
cd webos

# 一键构建
make all

# 在浏览器中运行
make serve
# 打开 http://localhost:8080
```

### 单独构建模块

```bash
make bootloader    # 构建引导程序
make kernel        # 构建内核
make drivers       # 构建驱动
make services      # 构建服务
make apps          # 构建应用
make host          # 构建 TypeScript 宿主
```

## 系统调用

| 子系统 | 编号范围 | 示例 |
|--------|---------|------|
| 文件系统 | 0x0100 | fs_read, fs_write, fs_mkdir |
| 进程管理 | 0x0200 | exit, getpid, fork, kill |
| 内存管理 | 0x0300 | malloc, free |
| 时间 | 0x0400 | time_now |
| I/O | 0x0500 | (预留) |
| 网络 | 0x0600 | (预留) |
| 调试 | 0x0700 | debug_log |
| IPC | 0x0800 | ipc_send, ipc_recv |
| 通知 | 0x0900 | notify_show |
| 动态链接 | 0x0A00 | dlopen, dlsym |

## 开发文档

- [系统架构](docs/architecture.md)
- [模块格式规范](docs/module_spec.md)
- [构建与运行](docs/building.md)
- [系统调用 API](docs/api_syscall.md)
- [系统服务 API](docs/api_services.md)
- [应用开发指南](docs/developer_guide.md)
- [应用商店规范](docs/app_store.md)

## 许可证

MIT License
