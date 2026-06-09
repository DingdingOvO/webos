# WebOS 模块格式规范

## 1. 文件后缀名定义表

WebOS 使用特定的文件后缀名来标识不同类型的 WebAssembly 模块。这些后缀名主要用于人类识别和构建系统分类，实际的模块类型由 WASM 二进制文件中的自定义段（Custom Section）决定。这种双标识机制确保了即使文件被错误重命名，运行时系统仍然可以正确识别和处理模块。文件后缀名的设计遵循直觉性原则：`.wex` 暗示"Web Executable"，`.Wdll` 暗示"Web DLL"（注意大写 W 以区别于系统 DLL），`.wli` 暗示"Web Library Interface"。

| 后缀名 | 模块类型 | 自定义段值 | 编译器 | 编译选项 | 描述 |
|--------|---------|-----------|--------|---------|------|
| `.wex` | 可执行模块 | `module_type="wex"` | `em++` (Emscripten C++) | `-s WASM=1 -s EXPORTED_FUNCTIONS='["_main"]'` | 独立应用程序，拥有自己的主循环 |
| `.Wdll` | 动态链接库 | `module_type="Wdll"` | `emcc` (Emscripten C) | `-s WASM=1 -s SIDE_MODULE=1` | 共享库、驱动或服务，不能独立运行 |
| `.wli` | 库接口描述 | `module_type="wli"` | 无（JSON 文本） | N/A | 符号描述文件，辅助动态链接 |
| `.wasm` | 通用 WASM | 无自定义段 | 任意 | 任意 | 未标识类型的 WASM 模块 |

### 各后缀名详解

**.wex（Web Executable）**：这是 WebOS 中可执行应用程序的标准格式。`.wex` 文件是一个完整的 WebAssembly 模块，包含应用程序的全部代码和数据。它必须导出一个 `main()` 函数作为程序入口，运行时系统通过调用该函数启动应用。`.wex` 模块使用 Emscripten C++ 编译器（`em++`）编译，支持 C++ 标准库的子集。典型的编译命令会导出 `_main` 函数，并设置 `ALLOW_MEMORY_GROWTH=1` 以允许动态内存增长。构建完成后，使用 `add_module_section.py` 工具添加 `module_type="wex"` 自定义段。

**.Wdll（Web Dynamic Link Library）**：这是 WebOS 中动态链接库的标准格式，用于驱动程序和系统服务。`.Wdll` 文件使用 Emscripten 的 SIDE_MODULE 模式编译，这意味着它是一个可被主模块动态加载的共享库。SIDE_MODULE 模式下，模块不能独立运行，必须由其他模块链接使用。`.Wdll` 模块必须使用 `EMSCRIPTEN_KEEPALIVE` 宏或 `-s EXPORTED_FUNCTIONS` 编译选项标记需要导出的公共 API 函数。构建完成后，使用 `add_module_section.py` 添加自定义段，并使用 `gen_wli.py` 生成对应的 `.wli` 接口描述文件。

**.wli（Web Library Interface）**：这是一个 JSON 格式的文本文件，描述了对应 `.Wdll` 模块的导出和导入符号信息。`.wli` 文件不包含可执行代码，仅作为动态链接的元数据使用。它由 `gen_wli.py` 工具自动从 `.Wdll` 模块中提取生成，记录了模块导出的函数名称、签名、以及模块导入的外部依赖。动态链接器在加载模块前可以读取 `.wli` 文件进行依赖预检查，确保所有必需的符号都能被满足。

**.wasm（通用 WebAssembly）**：这是标准的 WebAssembly 二进制格式，通常用于引导加载器（Bootloader）和内核（Kernel）等系统级组件。这些模块不添加 `module_type` 自定义段，因为它们的加载和识别由系统启动流程硬编码处理，不需要运行时动态识别。

## 2. 自定义段 module_type 的格式和添加方法

WebAssembly 规范允许在二进制文件中嵌入自定义段（Custom Section），自定义段的段 ID 为 0，可以包含任意内容。WebOS 利用这一机制，在 WASM 文件中嵌入名为 `module_type` 的自定义段，用于存储模块类型标识字符串。自定义段被放置在 WASM 文件头部（magic + version 之后），使得动态加载器无需解析整个文件即可快速识别模块类型。

### 自定义段的二进制格式

```
  WASM 文件结构
  ════════════

  偏移量    内容                          说明
  ──────   ──────────                    ──────
  0x00     00 61 73 6D                   WASM 魔数 (\0asm)
  0x04     01 00 00 00                   WASM 版本号 (1)
  0x08     00                            自定义段 ID = 0
  0x09     0B                            段内容长度 (LEB128)
  0x0A     0B                            名称长度 (LEB128) = 11
  0x0B     6D 6F 64 75 6C 65 5F 74 79 70 65   "module_type"
  0x16     77 65 78                      "wex" (模块类型值)
  0x19     ──────────                    后续标准 WASM 段...
```

自定义段的编码遵循以下规则：

1. **段 ID**：固定为 0（1 字节），表示这是一个自定义段
2. **段长度**：使用 LEB128 编码的段内容总字节数
3. **名称长度**：使用 LEB128 编码的段名称字符串长度
4. **名称**：UTF-8 编码的字符串 "module_type"
5. **内容**：UTF-8 编码的模块类型值（"wex"、"Wdll" 或 "wli"）

### 添加方法

手动添加自定义段是不推荐的，WebOS 提供了 `tools/add_module_section.py` 工具自动完成这一操作。该工具会解析 WASM 二进制文件，移除已存在的 `module_type` 自定义段（如果有），然后在文件头部插入新的自定义段。

```bash
# 为 .wex 可执行模块添加自定义段
python3 tools/add_module_section.py public/wasm/calculator.wex wex

# 为 .Wdll 动态链接库添加自定义段
python3 tools/add_module_section.py public/wasm/wm_service.Wdll Wdll

# 注意：模块类型值区分大小写
# 正确: wex, Wdll, wli
# 错误: WEX, WDLL, WLI, Wex, wDll
```

### 运行时读取过程

宿主运行时的 DynamicLoader 在加载模块时，会自动读取 `module_type` 自定义段：

```typescript
// host/src/dynamic_loader.ts 中的核心逻辑
private readModuleType(bytes: Uint8Array): { kind: ModuleKind; typeStr: string } {
    let offset = 8;  // 跳过 WASM magic + version (8 字节)

    while (offset < bytes.length) {
        const sectionId = bytes[offset];
        offset += 1;
        const [sectionSize, newOffset] = this.readLEB128(bytes, offset);
        offset = newOffset;
        const sectionEnd = offset + sectionSize;

        if (sectionId === 0) {  // 自定义段
            const [nameLen, nameOffset] = this.readLEB128(bytes, offset);
            const name = new TextDecoder().decode(
                bytes.slice(nameOffset, nameOffset + nameLen)
            );

            if (name === 'module_type') {
                const typeStart = nameOffset + nameLen;
                const typeStr = new TextDecoder().decode(
                    bytes.slice(typeStart, sectionEnd)
                );
                return { kind: this.moduleKindFromString(typeStr), typeStr };
            }
        }

        offset = sectionEnd;
    }

    return { kind: ModuleKind.UNKNOWN, typeStr: 'unknown' };
}
```

## 3. .wli 文件的 JSON 格式规范

`.wli` 文件是 WebOS 动态链接体系的重要组成部分，它以 JSON 格式描述了对应 `.Wdll` 模块的接口信息。动态链接器使用 `.wli` 文件进行依赖预检查和符号解析验证，确保模块间的链接关系正确无误。`.wli` 文件由 `gen_wli.py` 工具自动生成，通常不需要手动编辑，但在高级场景下开发者可以修改或扩展其内容。

### 完整格式规范

```json
{
    "module": "wm_service",
    "version": "1.0.0",
    "type": "Wdll",
    "description": "WebOS Window Manager Service",
    "exports": [
        {
            "name": "wm_service_init",
            "kind": "function",
            "signature": "function(): number",
            "description": "Initialize the window manager service"
        },
        {
            "name": "wm_create_window",
            "kind": "function",
            "signature": "function(pid: number, title: string, x: number, y: number, w: number, h: number): number",
            "description": "Create a new window for the specified process"
        },
        {
            "name": "wm_destroy_window",
            "kind": "function",
            "signature": "function(window_id: number): void",
            "description": "Destroy the specified window"
        },
        {
            "name": "wm_show_window",
            "kind": "function",
            "signature": "function(window_id: number): void",
            "description": "Make a window visible"
        },
        {
            "name": "wm_hide_window",
            "kind": "function",
            "signature": "function(window_id: number): void",
            "description": "Hide a window without destroying it"
        },
        {
            "name": "wm_move_window",
            "kind": "function",
            "signature": "function(window_id: number, x: number, y: number): void",
            "description": "Move a window to new position"
        },
        {
            "name": "wm_resize_window",
            "kind": "function",
            "signature": "function(window_id: number, w: number, h: number): void",
            "description": "Resize a window"
        },
        {
            "name": "wm_set_focus",
            "kind": "function",
            "signature": "function(window_id: number): void",
            "description": "Bring a window to front and focus it"
        },
        {
            "name": "wm_get_focused_window",
            "kind": "function",
            "signature": "function(): number",
            "description": "Get the currently focused window ID"
        },
        {
            "name": "wm_get_window_count",
            "kind": "function",
            "signature": "function(): number",
            "description": "Get the total number of windows"
        },
        {
            "name": "wm_render_all",
            "kind": "function",
            "signature": "function(surface: number): void",
            "description": "Render all visible windows to the given surface"
        }
    ],
    "imports": [
        {
            "name": "syscall_invoke",
            "from": "webos",
            "kind": "function",
            "description": "System call entry point"
        },
        {
            "name": "memory",
            "from": "env",
            "kind": "memory",
            "description": "Shared WebAssembly linear memory"
        }
    ],
    "dependencies": []
}
```

### 字段说明

| 字段 | 类型 | 必需 | 描述 |
|------|------|------|------|
| `module` | string | 是 | 模块名称，与文件名一致（不含后缀） |
| `version` | string | 否 | 模块版本号，语义化版本格式 |
| `type` | string | 是 | 模块类型，与自定义段值一致 |
| `description` | string | 否 | 模块功能描述 |
| `exports` | array | 是 | 模块导出的符号列表 |
| `imports` | array | 是 | 模块导入的符号列表 |
| `dependencies` | array | 否 | 模块依赖的其他模块列表 |

### exports 数组元素字段

| 字段 | 类型 | 必需 | 描述 |
|------|------|------|------|
| `name` | string | 是 | 导出符号名称 |
| `kind` | string | 是 | 符号类型：`function`、`table`、`memory`、`global` |
| `signature` | string | 否 | 函数签名描述（TypeScript 风格） |
| `description` | string | 否 | 符号功能描述 |

### imports 数组元素字段

| 字段 | 类型 | 必需 | 描述 |
|------|------|------|------|
| `name` | string | 是 | 导入符号名称 |
| `from` | string | 是 | 来源模块名称（如 `webos`、`env`） |
| `kind` | string | 是 | 符号类型：`function`、`table`、`memory`、`global` |
| `description` | string | 否 | 符号用途描述 |

### 驱动模块的 .wli 示例

以下是 GPU 驱动的 `.wli` 文件示例，展示了驱动模块的接口描述格式：

```json
{
    "module": "gpu_driver",
    "version": "1.0.0",
    "type": "Wdll",
    "description": "WebOS GPU Driver - Surface-based rendering API",
    "exports": [
        {
            "name": "gpu_driver_init",
            "kind": "function",
            "signature": "function(): number",
            "description": "Initialize the GPU driver and create default surface"
        },
        {
            "name": "gpu_create_surface",
            "kind": "function",
            "signature": "function(width: number, height: number): number",
            "description": "Create a new rendering surface with specified dimensions"
        },
        {
            "name": "gpu_destroy_surface",
            "kind": "function",
            "signature": "function(surface_id: number): number",
            "description": "Destroy a rendering surface"
        },
        {
            "name": "gpu_fill_rect",
            "kind": "function",
            "signature": "function(surface_id: number, x: number, y: number, w: number, h: number, color: number): number",
            "description": "Fill a rectangle on the surface with specified RGBA color"
        },
        {
            "name": "gpu_draw_text",
            "kind": "function",
            "signature": "function(surface_id: number, x: number, y: number, text: string, color: number): number",
            "description": "Draw text at the specified position"
        },
        {
            "name": "gpu_present",
            "kind": "function",
            "signature": "function(surface_id: number): number",
            "description": "Present the surface to the screen"
        }
    ],
    "imports": [
        {
            "name": "syscall_invoke",
            "from": "webos",
            "kind": "function"
        },
        {
            "name": "memory",
            "from": "env",
            "kind": "memory"
        }
    ],
    "dependencies": []
}
```

## 4. 不同模块类型对导入/导出的要求

WebOS 对不同类型的模块有明确的导入和导出要求，这些要求确保了模块能够被运行时系统正确加载和执行。违反这些要求的模块将无法正常工作，甚至可能导致系统崩溃。

### .wex 可执行模块的要求

可执行模块是独立运行的应用程序，它的导入导出要求最为严格：

**必需导出**：
- `_main()` — 程序入口函数，无参数或 `(int, char**)` 参数，返回 `int`。调度器通过调用此函数启动应用。如果模块缺少此导出，运行时将无法启动该应用，会在系统日志中记录错误。

**必需导入**：
- `webos.syscall_invoke` — 系统调用桥接函数，签名为 `(number, number, number, number, number, number) => number`。这是所有用户空间模块与内核通信的唯一通道。如果模块缺少此导入，链接器会使用 `-Wl,--allow-undefined` 选项容忍未定义符号，但运行时调用时将导致错误。

**可选导入**：
- `env.memory` — 共享线性内存。大多数模块需要访问共享内存来读取系统调用返回的数据。如果模块不使用动态内存分配，可以不导入此符号。

**禁止行为**：
- 导出除 `_main` 之外的函数是不推荐的，因为其他模块无法直接调用 `.wex` 模块的导出函数。
- 不能使用 `SIDE_MODULE` 编译选项，因为 `.wex` 是主模块，不是可链接的动态库。

### .Wdll 动态链接库的要求

动态链接库是可被其他模块加载的共享库，它的导入导出要求侧重于公共 API 的可发现性：

**必需导出**：
- 至少一个使用 `EMSCRIPTEN_KEEPALIVE` 标记或通过 `-s EXPORTED_FUNCTIONS` 指定的公共 API 函数。没有导出函数的动态库毫无用处，构建系统会在编译时发出警告。
- 建议导出一个 `xxx_init()` 初始化函数，命名格式为 `<模块名>_init`，用于模块的初始化操作。

**必需编译选项**：
- `-s SIDE_MODULE=1` — 告知 Emscripten 此模块将被动态链接，生成合适的重定位信息。没有此选项，模块将无法被动态加载器正确链接。

**必需导入**：
- `webos.syscall_invoke` — 与 `.wex` 相同，系统调用桥接函数。
- `env.memory` — 共享线性内存，动态链接库必须使用与主模块相同的内存实例。

**推荐实践**：
- 使用 `EMSCRIPTEN_KEEPALIVE` 宏标记所有需要导出的函数，确保编译器不会优化掉这些函数。
- 导出函数的命名应遵循 `<模块名>_<操作>` 的格式，如 `wm_create_window`、`fs_open`，避免命名冲突。
- 如果模块依赖其他 `.Wdll` 模块的函数，应在 `.wli` 文件的 `dependencies` 字段中声明。

### .wli 库接口描述文件的要求

`.wli` 文件是纯文本 JSON 文件，没有可执行代码，但必须遵循以下格式要求：

**必需字段**：
- `module` — 模块名称字符串，必须与对应的 `.Wdll` 文件名一致（不含后缀）。
- `exports` — 导出符号数组，至少包含一个元素。
- `imports` — 导入符号数组，可以为空数组。

**格式要求**：
- 必须是合法的 JSON 格式，UTF-8 编码。
- 数组中的符号对象必须包含 `name` 和 `kind` 字段。
- `kind` 值必须是 `function`、`table`、`memory`、`global` 之一。

## 5. 加载器识别优先级流程

当动态加载器收到加载模块的请求时，它会按照以下优先级流程识别模块类型。这个流程确保了即使在文件后缀名缺失或错误的情况下，模块类型仍然可以被正确识别。

```
  加载器识别流程
  ═════════════

  ┌──────────────────────────┐
  │ 收到加载请求              │
  │ 模块名称: "wm_service"   │
  └────────────┬─────────────┘
               │
               ▼
  ┌──────────────────────────┐
  │ 步骤 1: 获取 WASM 二进制  │
  │ 通过 Fetch API 下载文件   │
  └────────────┬─────────────┘
               │
               ▼
  ┌──────────────────────────────────────┐
  │ 步骤 2: 读取 module_type 自定义段     │
  │ 解析 WASM 二进制头部                  │
  │ 查找 id=0 且 name="module_type" 的段 │
  └────────────┬─────────────────────────┘
               │
               ├─ 找到自定义段
               │      │
               │      ▼
               │  ┌──────────────────────┐
               │  │ 步骤 3a: 使用自定义段值 │
               │  │ "wex" → ModuleKind.WEX│
               │  │ "Wdll" → ModuleKind.WDLL│
               │  │ "wli" → ModuleKind.WLI│
               │  └──────────────────────┘
               │
               └─ 未找到自定义段
                      │
                      ▼
              ┌──────────────────────────────┐
              │ 步骤 3b: 根据文件后缀名推断   │
              │ ".wex" → ModuleKind.WEX      │
              │ ".Wdll" → ModuleKind.WDLL    │
              │ ".wli" → ModuleKind.WLI      │
              │ ".wasm" → ModuleKind.UNKNOWN │
              └──────────────┬───────────────┘
                             │
                             ▼
              ┌──────────────────────────────┐
              │ 步骤 4: 根据 ModuleKind 决定  │
              │ 加载和实例化策略              │
              └──────────────────────────────┘
```

**优先级规则**：

1. **自定义段最高优先级**：如果 WASM 二进制文件包含 `module_type` 自定义段，则使用自定义段的值作为模块类型，忽略文件后缀名。这是最可靠的识别方式，因为自定义段是由构建工具在编译后注入的，不会被意外修改。

2. **文件后缀名次优先级**：如果自定义段不存在，加载器根据文件后缀名推断模块类型。这种后备机制确保了未使用 `add_module_section.py` 处理的模块仍然可以被加载，但准确性可能不如自定义段。

3. **未知类型兜底**：如果既没有自定义段，文件后缀名也不在已知列表中，模块被标记为 `ModuleKind.UNKNOWN`。未知类型的模块仍然可以被加载和实例化，但运行时不会为其应用特定类型的初始化逻辑（如为 `.wex` 创建进程）。

**识别结果的处理**：

| 识别结果 | 处理方式 |
|---------|---------|
| `WEX` | 创建进程控制块，加入调度器就绪队列，调用 `main()` 启动 |
| `WDLL` | 编译并实例化，将导出函数注册到全局符号表，增加引用计数 |
| `WLI` | 解析 JSON 内容，注册接口描述到动态链接器，不做实例化 |
| `UNKNOWN` | 按通用 WASM 模块处理，编译并实例化，但不应用特定初始化逻辑 |

## 6. 如何使用 tools/add_module_section.py

`tools/add_module_section.py` 是 WebOS 构建系统中的关键工具，负责在编译后的 WASM 二进制文件中添加或替换 `module_type` 自定义段。该工具使用纯 Python 编写，不依赖任何第三方库，通过直接操作 WASM 二进制格式实现自定义段的注入。

### 命令行用法

```bash
python3 tools/add_module_section.py <wasm_file> <module_type>
```

**参数说明**：

| 参数 | 说明 |
|------|------|
| `wasm_file` | 目标 WASM 文件路径（相对或绝对路径均可） |
| `module_type` | 模块类型标识，必须是 `wex`、`Wdll` 或 `wli` 之一 |

### 使用示例

```bash
# 为计算器应用添加 wex 类型标记
python3 tools/add_module_section.py public/wasm/calculator.wex wex
# 输出: Added module_type='wex' to public/wasm/calculator.wex

# 为窗口管理器服务添加 Wdll 类型标记
python3 tools/add_module_section.py public/wasm/wm_service.Wdll Wdll
# 输出: Added module_type='Wdll' to public/wasm/wm_service.Wdll

# 如果文件已包含 module_type 段，会先移除再添加新的
python3 tools/add_module_section.py public/wasm/calculator.wex wex
# 输出: Replacing existing module_type section
# 输出: Added module_type='wex' to public/wasm/calculator.wex
```

### 在 Makefile 中集成

所有 WebOS 模块的 Makefile 都会在编译完成后自动调用此工具。以下是典型的集成方式：

**应用程序 Makefile**：
```makefile
CC = em++
CFLAGS = -s WASM=1 -s EXPORTED_FUNCTIONS='["_main"]' \
         -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \
         -s ENVIRONMENT='web' -s ALLOW_MEMORY_GROWTH=1 \
         -O2 -Wall
TARGET = ../../public/wasm/calculator.wex

all: $(TARGET)

$(TARGET): main.cpp
        @mkdir -p $(dir $@)
        $(CC) $(CFLAGS) $< -o $@
        python3 ../../tools/add_module_section.py $@ wex

clean:
        rm -f $(TARGET)
```

**动态链接库 Makefile**：
```makefile
CC = emcc
CFLAGS = -s WASM=1 -s SIDE_MODULE=1 \
         -s EXPORTED_FUNCTIONS='["_wm_service_init","_wm_create_window",...]' \
         -O2 -Wall
TARGET = ../../public/wasm/wm_service.Wdll

all: $(TARGET)

$(TARGET): wm.c
        @mkdir -p $(dir $@)
        $(CC) $(CFLAGS) $< -o $@
        python3 ../../tools/add_module_section.py $@ Wdll
        python3 ../../tools/gen_wli.py $@

clean:
        rm -f $(TARGET) ../../public/wasm/wm_service.wli
```

### 工作原理详解

该工具的核心算法分为三个阶段：

**阶段 1：验证**。读取文件头 8 字节，验证 WASM 魔数（`\0asm`）和版本号（1）。如果验证失败，输出错误信息并退出。

**阶段 2：重构**。从偏移量 8 开始逐个解析 WASM 段，将所有非 `module_type` 自定义段的原样保留，跳过已有的 `module_type` 自定义段。这样确保了旧的类型标记被替换而不是重复添加。

**阶段 3：注入**。构建新的 `module_type` 自定义段，按照 WASM 二进制格式规范编码（段 ID + LEB128 长度 + LEB128 名称长度 + 名称 + 内容），将其插入文件头部（magic + version 之后、其他段之前），然后追加保留的原有段。

## 7. 如何使用 tools/gen_wli.py

`tools/gen_wli.py` 是 WebOS 构建系统中的辅助工具，用于从编译后的 `.Wdll` 模块中提取导出和导入符号信息，生成对应的 `.wli` 接口描述文件。该工具解析 WASM 二进制文件的导出段（Export Section，段 ID=7）和导入段（Import Section，段 ID=2），提取符号名称、类型等关键信息，以 JSON 格式输出。

### 命令行用法

```bash
python3 tools/gen_wli.py <Wdll_file> [output.wli]
```

**参数说明**：

| 参数 | 必需 | 说明 |
|------|------|------|
| `Wdll_file` | 是 | 输入的 .Wdll 文件路径 |
| `output.wli` | 否 | 输出的 .wli 文件路径。省略时自动使用输入文件名替换后缀 |

### 使用示例

```bash
# 生成窗口管理器服务的接口文件（自动推断输出路径）
python3 tools/gen_wli.py public/wasm/wm_service.Wdll
# 输出: Generated public/wasm/wm_service.wli
# 输出:   Module: wm_service
# 输出:   Exports: 11 symbols
# 输出:   Imports: 2 symbols

# 指定输出路径
python3 tools/gen_wli.py public/wasm/gpu_driver.Wdll public/wasm/gpu_driver.wli
# 输出: Generated public/wasm/gpu_driver.wli
# 输出:   Module: gpu_driver
# 输出:   Exports: 6 symbols
# 输出:   Imports: 2 symbols

# 批量生成所有动态库的 .wli 文件
for dll in public/wasm/*.Wdll; do
    python3 tools/gen_wli.py "$dll"
done
```

### 生成输出示例

对 `wm_service.Wdll` 运行 `gen_wli.py` 后生成的 `.wli` 文件：

```json
{
  "module": "wm_service",
  "exports": [
    {
      "name": "wm_service_init",
      "signature": "function()"
    },
    {
      "name": "wm_create_window",
      "signature": "function()"
    },
    {
      "name": "wm_destroy_window",
      "signature": "function()"
    },
    {
      "name": "wm_show_window",
      "signature": "function()"
    },
    {
      "name": "wm_hide_window",
      "signature": "function()"
    },
    {
      "name": "wm_move_window",
      "signature": "function()"
    },
    {
      "name": "wm_resize_window",
      "signature": "function()"
    },
    {
      "name": "wm_set_focus",
      "signature": "function()"
    },
    {
      "name": "wm_get_focused_window",
      "signature": "function()"
    },
    {
      "name": "wm_get_window_count",
      "signature": "function()"
    },
    {
      "name": "wm_render_all",
      "signature": "function()"
    }
  ],
  "imports": [
    {
      "name": "syscall_invoke",
      "from": "webos"
    },
    {
      "name": "memory",
      "from": "env"
    }
  ]
}
```

### 工作原理详解

该工具的解析过程分为以下步骤：

**步骤 1：验证文件格式**。读取文件头 4 字节，验证 WASM 魔数（`\0asm`）。如果文件不是有效的 WASM 格式，输出错误并退出。

**步骤 2：解析导入段（ID=2）**。遍历 WASM 段，找到导入段后，依次读取每个导入项的模块名称、字段名称和类型。对于函数类型的导入，记录函数名和来源模块名；对于其他类型（表、内存、全局变量），跳过具体类型信息的详细解析。

**步骤 3：解析导出段（ID=7）**。找到导出段后，依次读取每个导出项的名称、类型和索引。类型包括函数（kind=0）、表（kind=1）、内存（kind=2）、全局变量（kind=3）。将每个导出项转换为对应的描述对象。

**步骤 4：生成 JSON**。将解析结果组装为 JSON 对象，包含 `module`（从文件名推断）、`exports` 数组和 `imports` 数组，写入输出文件。

### 注意事项

- `gen_wli.py` 生成的签名信息是简化的 `function()` 格式，因为 WASM 二进制格式中不包含参数类型名称等高级信息。如果需要更详细的签名描述，开发者需要手动编辑 `.wli` 文件补充参数和返回值类型。
- 工具会跳过 Emscripten 运行时自动导出的符号（如 `__wasm_call_ctors`、`stackAlloc` 等），只保留业务相关的导出函数。但当前版本并未实现过滤逻辑，所有导出符号都会被包含在输出中。
- 如果 `.wli` 文件已存在，`gen_wli.py` 会直接覆盖，不会进行合并或增量更新。因此在手动编辑 `.wli` 文件后，应避免重新运行该工具。
