# WebOS 构建与运行指南

## 1. 环境准备

WebOS 的构建环境需要安装多个工具链和依赖项。由于 WebOS 的核心代码使用 C/C++ 编写并编译为 WebAssembly，您需要安装 Emscripten SDK 作为主要的编译工具链。此外，TypeScript 宿主运行时需要 Node.js 和 npm 环境。本节将详细说明每个依赖项的安装步骤和验证方法。

### Emscripten SDK 安装

Emscripten 是 WebOS 构建系统的核心工具链，负责将 C/C++ 源代码编译为 WebAssembly 模块。WebOS 的所有驱动、服务和应用模块都使用 Emscripten 编译，因此正确安装和配置 Emscripten 是构建 WebOS 的前提条件。

```bash
# 1. 克隆 Emscripten SDK
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk

# 2. 安装最新版本的 Emscripten
./emsdk install latest

# 3. 激活当前版本的 Emscripten
./emsdk activate latest

# 4. 设置环境变量（每次打开新终端都需要执行）
source ./emsdk_env.sh

# 5. 验证安装
emcc --version
# 预期输出: emcc (Emscripten gcc/clang-like replacement) 3.x.x

em++ --version
# 预期输出: em++ (Emscripten gcc/clang-like replacement + linker) 3.x.x
```

**注意事项**：
- `source ./emsdk_env.sh` 会将 `emcc` 和 `em++` 添加到当前终端的 PATH 中。如果您希望在每次打开终端时自动加载，可以将该命令添加到 `~/.bashrc` 或 `~/.zshrc` 文件中。
- Emscripten 需要访问网络以下载预编译的工具链组件。如果您的网络环境需要代理，请先配置好代理设置。
- 如果您在 Windows 上使用 WSL，确保 WSL 环境中安装了必要的构建工具（`make`、`python3` 等）。

### Node.js 和 npm 安装

WebOS 的宿主运行时使用 TypeScript 编写，需要 Node.js 运行时和 npm 包管理器。建议使用 Node.js 18 或更高版本，以确保兼容性和性能。

```bash
# 方法一：使用 NodeSource 仓库安装（Ubuntu/Debian）
curl -fsSL https://deb.nodesource.com/setup_18.x | sudo -E bash -
sudo apt-get install -y nodejs

# 方法二：使用 nvm 安装（推荐，支持多版本管理）
curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.39.0/install.sh | bash
source ~/.bashrc
nvm install 18
nvm use 18

# 验证安装
node --version
# 预期输出: v18.x.x 或更高

npm --version
# 预期输出: 9.x.x 或更高
```

### Python 3 安装

WebOS 的构建工具（`add_module_section.py` 和 `gen_wli.py`）使用 Python 3 编写，需要 Python 3.6 或更高版本。大多数现代 Linux 发行版已预装 Python 3。

```bash
# Ubuntu/Debian
sudo apt-get install python3

# macOS（通常已预装）
brew install python3

# 验证安装
python3 --version
# 预期输出: Python 3.x.x
```

### GNU Make 安装

WebOS 使用 Makefile 组织构建流程，需要 GNU Make 工具。

```bash
# Ubuntu/Debian
sudo apt-get install make

# macOS（通过 Xcode Command Line Tools）
xcode-select --install

# 验证安装
make --version
# 预期输出: GNU Make 4.x
```

### 环境验证清单

在开始构建之前，请确认以下工具均已正确安装：

```bash
# 一键验证所有依赖
echo "=== 环境检查 ===" && \
emcc --version > /dev/null 2>&1 && echo "✓ Emscripten (emcc)" || echo "✗ Emscripten (emcc) 未安装" && \
em++ --version > /dev/null 2>&1 && echo "✓ Emscripten (em++)" || echo "✗ Emscripten (em++) 未安装" && \
node --version > /dev/null 2>&1 && echo "✓ Node.js ($(node --version))" || echo "✗ Node.js 未安装" && \
npm --version > /dev/null 2>&1 && echo "✓ npm ($(npm --version))" || echo "✗ npm 未安装" && \
python3 --version > /dev/null 2>&1 && echo "✓ Python3 ($(python3 --version))" || echo "✗ Python3 未安装" && \
make --version > /dev/null 2>&1 && echo "✓ GNU Make" || echo "✗ GNU Make 未安装"
```

## 2. 一键构建命令

WebOS 提供了顶层 Makefile，支持通过简单的 `make` 命令完成整个项目的构建。构建过程按照严格的依赖顺序执行，确保每个组件在被需要之前已经构建完成。

### 完整构建

```bash
# 克隆仓库
git clone https://github.com/your-org/webos.git
cd webos

# 一键构建所有组件
make all
```

`make all` 会按照以下顺序依次构建各个组件：

```
构建顺序：
════════

1. tools          → 设置构建工具的可执行权限
2. bootloader     → 编译 C 引导加载器 → public/wasm/bootloader.wasm
3. kernel         → 编译 C 内核 → public/wasm/kernel.wasm
4. drivers        → 编译 C 驱动 → public/wasm/*.Wdll + *.wli
5. services       → 编译 C 服务 → public/wasm/*.Wdll + *.wli
6. apps           → 编译 C++ 应用 → public/wasm/*.wex
7. host           → 构建 TypeScript 宿主 → host/dist/webos-host.js
```

构建过程中，每个阶段都会输出进度信息。如果某个阶段失败，构建会立即停止并显示错误信息。修复错误后，可以重新运行 `make all`，Make 会跳过已经成功构建的组件，只重新构建失败的部分。

### 清理构建

```bash
# 清理所有构建产物
make clean

# 清理后重新构建
make clean && make all
```

`make clean` 会递归清理所有子项目的构建产物，包括 `public/wasm/` 目录下的所有 `.wasm`、`.wex`、`.Wdll` 和 `.wli` 文件，以及宿主运行时的 `host/dist/` 目录。

### 增量构建

Make 工具天然支持增量构建——如果某个源文件没有修改，对应的构建步骤会被跳过。这对于日常开发非常高效，只重新编译修改过的模块。

```bash
# 只重新构建修改过的组件
make

# 强制重新构建某个特定组件
cd apps/calculator && make clean && make
```

## 3. 在浏览器中运行

构建完成后，您可以通过启动开发服务器在浏览器中运行 WebOS。WebOS 提供了两种运行方式：使用 Python HTTP 服务器或使用宿主运行时的 webpack 开发服务器。

### 方法一：使用 Python HTTP 服务器

```bash
# 构建并启动开发服务器
make serve
```

`make serve` 会先执行完整的构建流程，然后启动宿主运行时的开发服务器。服务器启动后，在浏览器中访问 `http://localhost:8080` 即可看到 WebOS 的桌面环境。

### 方法二：使用宿主运行时的开发服务器

```bash
# 先确保所有 WASM 模块已构建
make bootloader kernel drivers services apps

# 启动宿主运行时的 webpack-dev-server
cd host && npm run serve
```

webpack-dev-server 提供热模块替换（HMR）功能，当宿主运行时的 TypeScript 代码修改后，浏览器会自动刷新。但请注意，WASM 模块的修改需要手动重新编译。

### 方法三：使用简单的 Python HTTP 服务器

如果只需要静态文件服务（不需要 webpack 的 HMR 功能），可以使用 Python 内置的 HTTP 服务器：

```bash
# 在项目根目录启动
python3 tools/serve.py
# 默认端口: 8080
```

`tools/serve.py` 是一个简单的 HTTP 服务器，设置了正确的 CORS 头和 MIME 类型，支持 `.wasm` 文件的 `application/wasm` Content-Type，以及 `.wex` 和 `.Wdll` 文件的自定义 MIME 类型。

### 浏览器兼容性

WebOS 需要浏览器支持以下 Web 标准：

| 特性 | 最低浏览器版本 | 说明 |
|------|--------------|------|
| WebAssembly | Chrome 57, Firefox 52, Safari 11 | 核心运行时 |
| WebAssembly.Memory | 同上 | 共享线性内存 |
| Fetch API | Chrome 42, Firefox 39, Safari 10.1 | 模块加载 |
| Canvas2D | 所有现代浏览器 | GPU 后端（默认） |
| WebGL 2.0 | Chrome 56, Firefox 51, Safari 15 | GPU 后端（可选） |
| WebGPU | Chrome 113+ | GPU 后端（实验性） |
| IndexedDB | 所有现代浏览器 | 持久化存储 |

**推荐浏览器**：Chrome 113+ 或 Edge 113+（支持完整的 WebGPU 功能）。

## 4. 各模块单独构建

除了顶层的一键构建，您还可以单独构建每个组件。这对于开发特定模块时非常方便，可以大幅缩短构建时间。

### 构建工具

```bash
make tools
# 设置 add_module_section.py 和 gen_wli.py 的可执行权限
# 这个步骤通常由其他构建目标自动依赖，不需要手动执行
```

### 构建引导加载器

```bash
make bootloader
# 或进入目录构建：
cd bootloader && make
# 产物: public/wasm/bootloader.wasm
```

引导加载器使用 `emcc` 编译，编译选项包括 `-s WASM=1 -s EXPORTED_FUNCTIONS='["_main"]'`。由于引导加载器是系统启动的第一个组件，它不需要添加 `module_type` 自定义段。

### 构建内核

```bash
make kernel
# 或进入目录构建：
cd kernel && make
# 产物: public/wasm/kernel.wasm
```

内核编译需要包含所有源文件（`main.c`、`memory.c`、`process.c`、`scheduler.c`、`ipc.c`、`dynlink.c`、`syscall.c`、`syscall_dispatch.c`），并导出 `kernel_syscall` 函数供用户空间模块调用。

### 构建驱动

```bash
make drivers
# 或单独构建某个驱动：
cd drivers/gpu && make          # GPU 驱动
cd drivers/input && make        # 输入驱动
cd drivers/storage && make      # 存储驱动
cd drivers/network && make      # 网络驱动
# 产物: public/wasm/gpu_driver.Wdll, gpu_driver.wli
#       public/wasm/input_driver.Wdll, input_driver.wli
#       public/wasm/storage_driver.Wdll, storage_driver.wli
#       public/wasm/network_driver.Wdll, network_driver.wli
```

每个驱动的构建流程相同：使用 `emcc` 编译为 SIDE_MODULE，然后使用 `add_module_section.py` 添加 `Wdll` 类型标记，最后使用 `gen_wli.py` 生成接口描述文件。

### 构建服务

```bash
make services
# 或单独构建某个服务：
cd services/window_manager && make    # 窗口管理器
cd services/filesystem && make        # 文件系统
cd services/network_service && make   # 网络服务
cd services/appstore && make          # 应用商店
# 产物: public/wasm/wm_service.Wdll, wm_service.wli
#       public/wasm/fs_service.Wdll, fs_service.wli
#       public/wasm/net_service.Wdll, net_service.wli
#       public/wasm/appstore.Wdll, appstore.wli
```

### 构建应用

```bash
make apps
# 或单独构建某个应用：
cd apps/calculator && make     # 计算器
cd apps/paint && make          # 画板
cd apps/browser && make        # 浏览器
cd apps/appstore_client && make # 应用商店客户端
# 产物: public/wasm/calculator.wex
#       public/wasm/paint.wex
#       public/wasm/browser.wex
#       public/wasm/appstore_client.wex
```

应用使用 `em++`（Emscripten C++ 编译器）编译，不需要 SIDE_MODULE 模式，但需要导出 `_main` 函数。编译完成后使用 `add_module_section.py` 添加 `wex` 类型标记。

### 构建宿主运行时

```bash
make host
# 或进入目录构建：
cd host && npm install && npm run build
# 产物: host/dist/webos-host.js
```

宿主运行时的构建使用 webpack 打包 TypeScript 代码为单个 JavaScript 文件。`npm install` 安装项目依赖（如 TypeScript 编译器、webpack、ts-loader 等），`npm run build` 执行编译和打包。

## 5. 常见问题

### 问题：WebGPU 不可用

**症状**：启动 WebOS 时控制台输出 "WebGPU is not available" 警告，GPU 驱动回退到 Canvas2D 渲染。

**原因**：WebGPU 是一项较新的 Web 标准，只有 Chrome 113+ 和 Edge 113+ 版本才支持。Firefox 和 Safari 的 WebGPU 支持仍在实验阶段。

**解决方案**：
1. 升级浏览器到最新版本的 Chrome 或 Edge。
2. 在 Chrome 中启用 WebGPU：访问 `chrome://flags/#enable-unsafe-webgpu`，设置为 Enabled。
3. 如果无法使用 WebGPU，WebOS 会自动回退到 Canvas2D 渲染后端，功能不受影响但性能可能略有下降。
4. 也可以通过运行时配置强制指定渲染后端：
   ```typescript
   const config = { gpuBackend: 'canvas2d' }; // 或 'webgl'
   const bridge = new RuntimeBridge(config);
   ```

### 问题：Emscripten 安装失败

**症状**：执行 `./emsdk install latest` 时报错，通常是网络超时或 Python 版本不兼容。

**解决方案**：
1. 确保网络可以访问 GitHub 和 Amazon S3（Emscripten 的预编译包托管在 S3 上）。
2. 如果使用代理，设置 `HTTP_PROXY` 和 `HTTPS_PROXY` 环境变量。
3. 确保 Python 版本 >= 3.6：`python3 --version`。
4. 尝试安装特定版本而非最新版：`./emsdk install 3.1.44`。
5. 如果 Git 克隆失败，可以手动下载 Emscripten SDK 的压缩包。

### 问题："syscall_invoke 未定义"链接警告

**症状**：编译时出现 `undefined symbol: syscall_invoke` 警告。

**原因**：这是正常现象。`syscall_invoke` 是运行时由宿主环境提供的 WASM 导入函数，编译时不存在定义。对于 `.Wdll` 模块（SIDE_MODULE），Emscripten 会自动处理这种情况；对于 `.wex` 模块，如果使用原始 `clang` 编译器，需要添加 `-Wl,--allow-undefined` 链接选项。使用 Emscripten（`emcc`/`em++`）编译时，未定义的导入函数会被自动处理为外部导入。

**解决方案**：如果使用 Emscripten 编译，可以忽略此警告。如果使用原始 clang 编译，添加链接选项：
```bash
clang -target wasm32 -nostdlib -Wl,--allow-undefined -Wl,--no-entry ...
```

### 问题：构建产物不在 public/wasm/ 目录中

**症状**：`make all` 执行成功但 `public/wasm/` 目录为空或缺少某些文件。

**原因**：可能是子项目的 Makefile 中的输出目录配置不正确，或者目录创建失败。

**解决方案**：
1. 手动创建输出目录：`mkdir -p public/wasm`
2. 检查子项目 Makefile 中的 `OUTPUT_DIR` 变量是否正确指向 `../../public/wasm/` 或 `$(dir $@)` 路径。
3. 确保文件系统有足够的磁盘空间。

### 问题：浏览器控制台显示 "Failed to fetch module"

**症状**：WebOS 启动时浏览器控制台出现 "Failed to fetch module: /wasm/kernel.wasm (404)" 错误。

**原因**：开发服务器没有正确配置，或者 WASM 文件不在服务器的静态文件目录中。

**解决方案**：
1. 确认 `public/wasm/` 目录中有对应的 `.wasm` 文件。
2. 确认开发服务器以 `public/` 目录为根目录提供静态文件服务。
3. 如果使用自定义开发服务器，确保正确配置了 `.wasm` 文件的 MIME 类型为 `application/wasm`。

### 问题：Emscripten 编译报错 "SIDE_MODULE requires MAIN_MODULE"

**症状**：编译 `.Wdll` 模块时出现 "SIDE_MODULE requires MAIN_MODULE or standalone mode" 错误。

**原因**：Emscripten 的 SIDE_MODULE 模式需要与 MAIN_MODULE 配合使用，或者明确指定为独立模式。

**解决方案**：确保 Makefile 中的编译选项正确：
```makefile
# 正确的 .Wdll 编译选项
CC = emcc
CFLAGS = -s WASM=1 -s SIDE_MODULE=1 -O2
```

## 6. 开发工作流

### 日常开发流程

```
  开发工作流
  ════════

  ┌──────────────┐     ┌──────────────┐     ┌──────────────┐
  │  修改源代码   │────▶│  增量构建     │────▶│  浏览器测试   │
  │  (编辑器)     │     │  (make)      │     │  (刷新页面)   │
  └──────────────┘     └──────────────┘     └──────┬───────┘
                                                   │
                                          ┌────────▼────────┐
                                          │  通过/失败？     │
                                          └────────┬────────┘
                                              │           │
                                          通过 │           │ 失败
                                              ▼           ▼
                                        ┌─────────┐  ┌──────────┐
                                        │ 继续开发 │  │ 修复代码 │
                                        └─────────┘  └──────────┘
```

### 推荐的开发环境配置

**VS Code 配置**：推荐安装以下扩展以获得最佳开发体验：

1. **C/C++** (Microsoft) — C/C++ 智能提示、调试支持
2. **Emscripten** — Emscripten 项目支持
3. **WebAssembly** — WASM 文件查看和调试
4. **TSDK** — TypeScript 开发工具包

**c_cpp_properties.json** 配置示例：

```json
{
    "configurations": [
        {
            "name": "WebOS",
            "includePath": [
                "${workspaceFolder}/libs",
                "${workspaceFolder}/kernel/src",
                "${workspaceFolder}/bootloader/src"
            ],
            "defines": [
                "__EMSCRIPTEN__"
            ],
            "compilerPath": "/path/to/emsdk/upstream/emscripten/emcc",
            "cStandard": "c11",
            "cppStandard": "c++17"
        }
    ],
    "version": 4
}
```

### 模块开发检查清单

开发新模块时，请按照以下检查清单逐项确认：

- [ ] 源代码文件放置在正确的目录（`apps/`、`drivers/`、`services/`）
- [ ] Makefile 配置了正确的编译器和编译选项
- [ ] 导出函数使用 `EMSCRIPTEN_KEEPALIVE` 标记或列在 `EXPORTED_FUNCTIONS` 中
- [ ] 构建后执行了 `add_module_section.py` 添加自定义段
- [ ] 如果是 `.Wdll` 模块，执行了 `gen_wli.py` 生成接口描述
- [ ] 如果是应用，创建了 `app.json` 清单文件
- [ ] 顶层 Makefile 中已添加了新模块的构建目标
- [ ] 浏览器测试通过，控制台无错误输出

### 调试技巧

**启用调试模式**：在宿主运行时的配置中启用 `debugMode`，可以看到详细的系统调用日志、IPC 消息流转和模块加载信息：

```typescript
const config = { debugMode: true };
const bridge = new RuntimeBridge(config);
```

**使用浏览器 DevTools**：
1. 打开 Chrome DevTools (F12)
2. 切换到 Sources 面板，可以查看和调试 WASM 模块
3. 切换到 Console 面板，查看 `[print]` 前缀的内核日志
4. 切换到 Network 面板，查看 WASM 模块的加载情况

**使用 sys_debug_log 输出调试信息**：在 C/C++ 代码中使用 `syscall_debug_log()` 函数输出调试信息到浏览器控制台：

```c
syscall_debug_log("Entering function foo");
char buf[128];
snprintf(buf, sizeof(buf), "Value of x: %d", x);
syscall_debug_log(buf);
```

**优化构建**：开发时使用 `-O0` 或 `-O1` 优化级别，便于调试；发布时使用 `-O2` 或 `-Os` 优化级别，减小文件体积并提升性能。调试时可以添加 `-g` 选项保留调试信息：

```makefile
# 开发配置
CFLAGS = -s WASM=1 -O0 -g -Wall

# 发布配置
CFLAGS = -s WASM=1 -O2 -Wall
```
