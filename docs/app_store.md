# WebOS 应用商店规范

## 1. 应用清单 app.json 格式

应用清单（`app.json`）是每个 WebOS 应用的核心元数据文件，描述了应用的标识、版本、权限、依赖等关键信息。应用商店服务使用清单文件注册和展示应用，安装程序使用清单文件验证应用完整性，运行时系统使用清单文件检查权限和加载依赖。一个格式正确、信息完整的清单文件是应用发布的前提条件。

### 完整字段说明

```json
{
    "id": "com.os.calculator",
    "name": "Calculator",
    "version": "1.0.0",
    "description": "A simple calculator with basic arithmetic operations",
    "long_description": "WebOS Calculator provides standard arithmetic operations including addition, subtraction, multiplication, and division. Features include decimal point support, clear function, and a clean visual interface optimized for both mouse and keyboard input.",
    "author": "WebOS Team",
    "author_email": "team@webos.dev",
    "icon": "/icons/calc.png",
    "icon_svg": "/icons/calc.svg",
    "category": "Utilities",
    "type": "wex",
    "entry": "calculator.wex",
    "permissions": ["wm", "input"],
    "dependencies": [],
    "minWidth": 300,
    "minHeight": 340,
    "maxWidth": 600,
    "maxHeight": 680,
    "defaultWidth": 300,
    "defaultHeight": 340,
    "min_os_version": "0.1.0",
    "homepage": "https://webos.dev/apps/calculator",
    "repository": "https://github.com/webos-apps/calculator",
    "license": "MIT",
    "keywords": ["calculator", "math", "arithmetic", "utilities"],
    "locales": {
        "zh-CN": {
            "name": "计算器",
            "description": "一个提供基本算术运算的简单计算器"
        },
        "zh-TW": {
            "name": "計算器",
            "description": "一個提供基本算術運算的簡單計算器"
        }
    },
    "release_notes": {
        "1.0.0": "Initial release with basic arithmetic operations"
    }
}
```

### 字段详细说明

| 字段 | 类型 | 必需 | 默认值 | 说明 |
|------|------|------|--------|------|
| `id` | string | 是 | — | 应用唯一标识符，采用反向域名格式（如 `com.os.calculator`）。必须以字母开头，只允许小写字母、数字、点号和连字符。全局唯一，不可与已注册的应用冲突。 |
| `name` | string | 是 | — | 应用显示名称，用于应用商店和启动器中的展示。长度建议不超过 30 个字符。支持 Unicode 字符（如中文）。 |
| `version` | string | 是 | — | 语义化版本号（SemVer 格式：`MAJOR.MINOR.PATCH`）。每次更新必须递增版本号。预发布版本可附加后缀（如 `1.0.0-beta.1`）。 |
| `description` | string | 是 | — | 应用简短描述，用于应用商店列表页展示。建议不超过 100 个字符。 |
| `long_description` | string | 否 | — | 应用详细描述，用于应用商店详情页展示。支持纯文本格式，无长度限制。 |
| `author` | string | 是 | — | 应用作者名称，可以是个人姓名或组织名称。 |
| `author_email` | string | 否 | — | 作者联系邮箱，用于用户反馈和安全通知。 |
| `icon` | string | 是 | — | 应用图标文件路径（相对于应用根目录），PNG 格式，推荐 128x128 像素。 |
| `icon_svg` | string | 否 | — | 应用图标的 SVG 格式版本，用于高 DPI 显示。如果提供，系统优先使用 SVG 图标。 |
| `category` | string | 是 | — | 应用分类，必须是预定义分类之一（见分类表）。 |
| `type` | string | 是 | — | 模块类型，目前仅支持 `"wex"`。 |
| `entry` | string | 是 | — | 入口模块文件名（如 `calculator.wex`），必须是 `.wex` 后缀的 WASM 模块。 |
| `permissions` | array | 是 | `[]` | 应用请求的权限列表。未声明的权限在运行时将被拒绝。 |
| `dependencies` | array | 是 | `[]` | 应用依赖的动态库列表，每个元素为 `.Wdll` 文件名（如 `"net_service.Wdll"`）。 |
| `minWidth` | number | 否 | `200` | 窗口最小宽度（像素），用户不能将窗口缩小到此值以下。 |
| `minHeight` | number | 否 | `200` | 窗口最小高度（像素）。 |
| `maxWidth` | number | 否 | `4096` | 窗口最大宽度（像素）。 |
| `maxHeight` | number | 否 | `4096` | 窗口最大高度（像素）。 |
| `defaultWidth` | number | 否 | `minWidth` | 窗口默认宽度（首次打开时）。 |
| `defaultHeight` | number | 否 | `minHeight` | 窗口默认高度（首次打开时）。 |
| `min_os_version` | string | 否 | `"0.1.0"` | 最低兼容的 WebOS 版本号。低于此版本的系统将阻止安装。 |
| `homepage` | string | 否 | — | 应用主页 URL。 |
| `repository` | string | 否 | — | 源代码仓库 URL。 |
| `license` | string | 否 | — | 开源许可证标识符（如 `"MIT"`、`"Apache-2.0"`、`"GPL-3.0"`）。 |
| `keywords` | array | 否 | `[]` | 搜索关键词列表，用于应用商店的搜索匹配。 |
| `locales` | object | 否 | `{}` | 多语言本地化信息，键为语言代码（如 `"zh-CN"`），值为包含 `name` 和 `description` 的对象。 |
| `release_notes` | object | 否 | `{}` | 版本发布说明，键为版本号，值为该版本的更新内容描述。 |

### 预定义应用分类

| 分类标识 | 显示名称 | 描述 |
|---------|---------|------|
| `Utilities` | 工具 | 系统工具和实用程序（计算器、时钟等） |
| `Internet` | 网络 | 网络浏览和通信（浏览器、邮件客户端等） |
| `Productivity` | 效率 | 办公和效率工具（文本编辑器、笔记等） |
| `Graphics` | 图形 | 图形创作和编辑（画板、图片查看器等） |
| `Media` | 媒体 | 媒体播放和创作（音乐播放器、视频播放器等） |
| `Games` | 游戏 | 游戏和娱乐 |
| `System` | 系统 | 系统管理工具（设置、文件管理器等） |
| `Development` | 开发 | 开发者工具（终端、代码编辑器等） |
| `Education` | 教育 | 教育和学习工具 |
| `Finance` | 财务 | 财务管理工具 |

### 最小有效清单

以下是一个最小有效清单，只包含必需字段：

```json
{
    "id": "com.example.hello",
    "name": "Hello World",
    "version": "0.1.0",
    "description": "A minimal hello world application",
    "author": "Example Developer",
    "icon": "/icons/hello.png",
    "category": "Utilities",
    "type": "wex",
    "permissions": [],
    "entry": "hello.wex",
    "dependencies": []
}
```

## 2. 图标和资源要求

应用图标是用户识别和区分应用的重要视觉元素。WebOS 对应用图标和配套资源有明确的格式和尺寸要求，以确保在不同显示环境下的一致性和美观性。

### 图标规格

| 资源 | 格式 | 尺寸 | 必需 | 说明 |
|------|------|------|------|------|
| 主图标 | PNG | 128x128 px | 是 | 用于应用商店、启动器和任务栏显示。背景必须为透明。 |
| 大图标 | PNG | 256x256 px | 否 | 用于应用详情页和通知中心的高分辨率显示。 |
| SVG 图标 | SVG | 矢量 | 否 | 推荐提供，用于任意缩放的高 DPI 显示。系统优先使用 SVG。 |
| 启动画面 | PNG | 与窗口默认尺寸相同 | 否 | 应用加载时显示的占位画面，避免窗口内容闪烁。 |

### 图标设计规范

**视觉风格**：图标应采用扁平化或轻拟物风格，与 WebOS 的整体视觉语言保持一致。避免过于复杂的细节，因为在 128x128 像素尺寸下，过于细致的图案会变得模糊不清。

**颜色**：建议使用鲜明、饱和度适中的颜色作为图标的主体色调，使其在深色和浅色桌面背景下都清晰可辨。避免使用纯黑或纯白作为主色调，因为它们可能融入桌面背景。

**安全区域**：图标内容的 80% 区域应包含核心视觉元素，外围 10% 的边距保持空白，确保图标在圆形或圆角矩形裁切下仍然美观。

**透明背景**：PNG 图标必须使用透明背景（Alpha 通道），不要包含固定的背景色或边框。系统会根据当前桌面主题自动添加适当的背景或边框。

### 资源文件组织

```
apps/calculator/
├── main.cpp
├── app.json
├── Makefile
└── icons/
    ├── calc.png          # 128x128 主图标
    ├── calc_256.png      # 256x256 大图标
    └── calc.svg          # SVG 矢量图标
```

图标文件放在应用目录的 `icons/` 子目录中，`app.json` 中的 `icon` 字段使用相对路径引用。安装时，图标文件会被复制到系统的应用资源目录中。

### 其他资源文件

除图标外，应用还可以包含以下资源文件：

| 资源类型 | 文件格式 | 说明 |
|---------|---------|------|
| 配置文件 | JSON | 应用默认配置 |
| 字体文件 | WOFF2 | 自定义字体 |
| 音频文件 | OGG, MP3 | 音效和背景音乐 |
| 数据文件 | 任意 | 应用所需的只读数据 |

资源文件在 `app.json` 中不需要显式声明，只需放在应用目录中即可。安装时，整个应用目录（包括所有资源文件）会被一起打包和部署。

## 3. 如何提交应用到商店

应用提交到 WebOS 应用商店需要经过准备、验证、打包和上传四个步骤。每个步骤都有明确的要求和检查点，确保只有高质量、安全合规的应用才能上架。

### 3.1 准备提交

在提交应用之前，开发者需要确保以下条件已满足：

**功能完整性**：应用的核心功能已经实现并通过手动测试。应用不应在正常操作下崩溃或卡死。所有用户可见的功能都应该正常工作，没有明显的 bug。

**清单完整性**：`app.json` 文件包含所有必需字段，字段值符合规范要求。`version` 字段反映当前版本号，`permissions` 字段列出了所有需要的权限，`dependencies` 字段列出了所有依赖的动态库。

**图标合规**：主图标文件存在且符合尺寸要求（128x128 PNG，透明背景）。图标在浅色和深色背景下都清晰可辨。

### 3.2 验证应用

WebOS 提供了验证工具检查应用的完整性和合规性：

```bash
# 步骤 1：验证 WASM 模块格式
# 检查 .wex 文件是否为有效的 WASM 二进制格式
python3 -c "
import sys
data = open(sys.argv[1], 'rb').read()
assert data[:4] == b'\\x00asm', 'Not a valid WASM file'
assert data[4:8] == b'\\x01\\x00\\x00\\x00', 'Unsupported WASM version'
print('WASM format: OK')
" public/wasm/calculator.wex

# 步骤 2：验证自定义段
# 检查 module_type 自定义段是否存在且值正确
python3 ../../tools/add_module_section.py --verify public/wasm/calculator.wex

# 步骤 3：验证应用清单
# 检查 app.json 格式和必需字段
python3 -c "
import json, sys
with open(sys.argv[1]) as f:
    app = json.load(f)
required = ['id', 'name', 'version', 'description', 'author',
            'category', 'type', 'permissions', 'entry', 'dependencies']
missing = [k for k in required if k not in app]
if missing:
    print(f'Missing required fields: {missing}')
    sys.exit(1)
print('App manifest: OK')
" apps/calculator/app.json

# 步骤 4：生成 .wli 接口文件
python3 ../../tools/gen_wli.py public/wasm/calculator.wex
```

### 3.3 打包应用

将应用的所有文件打包为一个发布包：

```
com.os.calculator-1.0.0.webospkg/
├── app.json                    # 应用清单
├── calculator.wex              # 主模块
├── calculator.wli              # 接口描述（可选）
├── icons/
│   ├── calc.png               # 主图标
│   ├── calc_256.png           # 大图标
│   └── calc.svg               # SVG 图标
└── resources/                  # 其他资源文件（如有）
```

### 3.4 提交到应用商店

将发布包提交到 WebOS 应用商店的远程注册表。提交方式包括：

**方式一：通过应用商店客户端提交**。在 WebOS 桌面环境中打开应用商店客户端，切换到"开发者"选项卡，上传发布包。系统会自动执行验证流程，验证通过后应用进入审核队列。

**方式二：通过命令行工具提交**。使用 WebOS SDK 提供的 CLI 工具：

```bash
# 安装 WebOS SDK CLI
npm install -g @webos/sdk

# 登录开发者账号
webos login --email dev@example.com

# 提交应用
webos publish --package com.os.calculator-1.0.0.webospkg

# 查看审核状态
webos status --app com.os.calculator
```

**方式三：通过 API 提交**。直接调用应用商店的 REST API：

```bash
curl -X POST https://appstore.webos.dev/api/v1/apps \
     -H "Authorization: Bearer <dev_token>" \
     -F "package=@com.os.calculator-1.0.0.webospkg.zip"
```

### 3.5 审核流程

应用提交后进入审核流程，审核内容包括：

1. **格式验证**：WASM 模块格式正确，自定义段完整
2. **安全检查**：没有禁止的导入函数，权限声明合理
3. **功能测试**：应用可以正常启动和运行，不会导致系统崩溃
4. **内容审核**：应用内容符合社区准则

审核通过后，应用自动上架到应用商店，用户可以搜索和安装。

## 4. 安装流程

WebOS 的应用安装流程设计为全自动的，用户只需点击"安装"按钮，系统会自动处理下载、验证、部署和注册的全部步骤。以下是安装流程的详细说明。

### 4.1 安装流程图

```
  用户操作             应用商店客户端              应用商店服务              文件系统服务
  ════════            ══════════════             ══════════════            ═════════════

  点击"安装" ────────▶ 查询应用信息
                      获取 app.json
                      检查磁盘空间
                            │
                            ▼
                      发送安装请求 ──────────▶ 验证应用 ID
                                            检查是否已安装
                                            获取下载 URL
                                                  │
                                                  ▼
                                            下载 .wex 模块 ─────────▶ 创建应用目录
                                            下载 .Wdll 依赖          /apps/com.os.calculator/
                                            下载图标资源             写入 calculator.wex
                                            下载 app.json           写入 app.json
                                                                     写入图标文件
                                                  │                        │
                                                  ▼                        ▼
                                            验证 WASM 格式 ◀──────── 读取 .wex 文件
                                            验证自定义段
                                            注册到应用表
                                                  │
                                                  ▼
                      安装完成通知 ◀──────── 返回成功
  显示已安装标识
```

### 4.2 下载与存储

安装过程首先将应用的所有文件下载到临时目录，验证完整性后再移动到最终位置：

1. **创建临时目录**：在 `/tmp/install/` 下创建以应用 ID 命名的临时目录
2. **下载主模块**：从远程注册表下载 `.wex` 文件到临时目录
3. **下载依赖库**：如果 `dependencies` 列表中有 `.Wdll` 文件，逐一下载
4. **下载图标和资源**：下载图标文件和其他资源文件
5. **下载清单文件**：下载 `app.json` 到临时目录

### 4.3 验证与部署

下载完成后，应用商店服务执行以下验证步骤：

1. **WASM 格式验证**：检查 `.wex` 文件的 WASM 魔数和版本号
2. **自定义段验证**：确认 `module_type` 自定义段的值为 `wex`
3. **清单验证**：解析 `app.json`，检查必需字段是否完整
4. **依赖检查**：确认所有 `dependencies` 中列出的 `.Wdll` 文件都已下载且格式正确

验证通过后，将临时目录中的文件移动到最终位置：

```
/apps/com.os.calculator/
├── app.json
├── calculator.wex
├── icons/
│   └── calc.png
└── (其他资源文件)
```

### 4.4 注册与启动

文件部署完成后，应用商店服务将应用注册到系统的应用注册表中。注册信息包括应用 ID、名称、图标路径、入口模块路径和分类。注册完成后，应用出现在系统启动器和开始菜单中，用户可以点击图标启动应用。

启动流程：
1. 用户点击应用图标
2. 系统读取 `app.json` 中的 `entry` 字段，获取入口模块路径
3. 内核通过动态加载器加载入口模块
4. 为模块创建新进程，设置进程状态为 `PROC_STATE_READY`
5. 调度器选中该进程，调用 `main()` 函数启动应用

## 5. 应用权限模型

WebOS 采用了声明式权限模型，应用必须在 `app.json` 的 `permissions` 字段中声明所需的权限。运行时系统在应用尝试使用受保护的 API 时检查权限声明，未声明的权限将被拒绝。这种设计确保了用户对应用能力的知情权和控制权。

### 5.1 权限列表

| 权限 | 标识 | 对应系统调用/服务 | 描述 |
|------|------|-----------------|------|
| 窗口管理 | `wm` | WM 服务 API (0x01-0x0A) | 创建和管理应用窗口。几乎所有图形应用都需要此权限。 |
| 输入设备 | `input` | 输入驱动 API | 接收键盘和鼠标输入事件。需要用户交互的应用需要此权限。 |
| 网络访问 | `network` | 网络服务 API (0x06xx) | 发起 HTTP 请求和 WebSocket 连接。浏览器、邮件客户端等需要此权限。 |
| 文件系统 | `storage` | 文件系统服务 API (0x01xx) | 读写文件和目录。需要持久化数据的应用需要此权限。 |
| 进程间通信 | `ipc` | IPC 系统调用 (0x08xx) | 与其他进程通信。需要与其他服务交互的应用需要此权限。 |
| 系统管理 | `system` | 进程管理 (0x02xx) | 创建和终止进程、修改系统设置。仅系统管理工具需要此权限。 |

### 5.2 权限检查机制

权限检查发生在两个层面：

**安装时检查**：应用商店服务在安装应用时，会分析 `app.json` 中的权限声明。对于高风险权限（如 `system`），应用商店会向用户显示权限请求对话框，要求用户确认。用户拒绝的权限不会被授予，但应用仍然可以安装——只是相关功能将不可用。

**运行时检查**：当应用通过系统调用请求受保护的服务时，内核的 syscall_dispatch 函数会检查当前进程的权限列表。如果权限未声明，系统调用返回 `ERR_PERM` (-5) 错误。应用程序应正确处理权限被拒绝的情况，向用户显示友好的提示信息。

```c
/* 权限检查示例 */
int create_window(void) {
    int win = wm_create_window("My App", 100, 100, 400, 300);
    if (win <= 0) {
        /* 可能是权限不足 */
        syscall_debug_log("Window creation failed - check 'wm' permission");
        return -1;
    }
    return win;
}
```

### 5.3 最小权限原则

应用应遵循最小权限原则，只声明实际需要的权限。不必要的权限声明会增加用户的安全顾虑，降低安装意愿。以下是一些常见应用类型的推荐权限配置：

| 应用类型 | 推荐权限 | 说明 |
|---------|---------|------|
| 计算器 | `wm`, `input` | 仅需窗口和输入，无需存储或网络 |
| 画板 | `wm`, `input`, `storage` | 需要存储保存画作 |
| 浏览器 | `wm`, `input`, `network`, `storage` | 需要网络和缓存 |
| 文本编辑器 | `wm`, `input`, `storage` | 需要存储读写文件 |
| 终端 | `wm`, `input`, `ipc`, `system` | 需要执行命令和进程管理 |
| 设置 | `system`, `storage` | 需要系统管理权限 |

## 6. 版本管理

WebOS 采用语义化版本（Semantic Versioning，SemVer）规范管理应用版本。版本号格式为 `MAJOR.MINOR.PATCH`，每个部分都有明确的含义和递增规则。

### 6.1 版本号规则

| 部分 | 名称 | 递增条件 | 示例 |
|------|------|---------|------|
| MAJOR | 主版本号 | 不兼容的 API 变更 | 1.0.0 → 2.0.0 |
| MINOR | 次版本号 | 向后兼容的功能新增 | 1.0.0 → 1.1.0 |
| PATCH | 修订号 | 向后兼容的问题修复 | 1.0.0 → 1.0.1 |

**主版本号（MAJOR）**：当应用进行了不兼容的 API 变更时递增。例如，应用的 IPC 接口发生了破坏性变更、删除了某个公开的导出函数、或者改变了配置文件格式导致旧版本不兼容时，必须递增主版本号。主版本号变更意味着用户可能需要迁移数据或调整使用方式。

**次版本号（MINOR）**：当应用新增了向后兼容的功能时递增。例如，添加了新的 UI 功能、增加了新的导出函数（但保留了旧函数）、或者扩展了配置文件的字段（但旧字段仍然有效）时，递增次版本号。次版本号变更不会影响已有功能的正常使用。

**修订号（PATCH）**：当应用进行了向后兼容的问题修复时递增。例如，修复了一个 UI 显示 bug、修正了一个计算错误、或者优化了性能，不影响应用的外部行为。修订号变更是最安全的更新，用户可以放心升级。

### 6.2 版本比较规则

版本号按照以下规则进行比较：

1. 从左到右依次比较 MAJOR、MINOR、PATCH
2. 数值大的版本更高（如 `1.2.0` > `1.1.9`）
3. 缺失的部分视为 0（如 `1.2` 等同于 `1.2.0`）
4. 预发布版本低于正式版本（如 `1.0.0-alpha` < `1.0.0`）

### 6.3 更新流程

```
  版本更新流程
  ════════════

  ┌──────────────┐     ┌──────────────┐     ┌──────────────┐
  │ 开发者发布    │────▶│ 应用商店检查  │────▶│ 通知用户更新 │
  │ 新版本        │     │ 版本差异      │     │ 显示更新日志 │
  └──────────────┘     └──────────────┘     └──────┬───────┘
                                                   │
                                          ┌────────▼────────┐
                                          │ 用户确认更新？   │
                                          └────────┬────────┘
                                             │           │
                                          确认 │           │ 取消
                                             ▼           ▼
                                      ┌──────────┐  ┌──────────┐
                                      │ 下载更新  │  │ 保持当前 │
                                      │ 替换文件  │  │ 版本     │
                                      │ 更新注册  │  └──────────┘
                                      └──────────┘
```

开发者发布新版本后，应用商店服务通过 `appstore_check_updates()` API 检查已安装应用是否有更新版本。如果发现新版本，系统会在通知中心显示更新提醒，附带版本号和更新日志。用户确认更新后，系统下载新版本的文件并替换旧文件。更新过程中，应用商店服务会保留用户的个人数据（位于 `/data/<app_id>/` 目录），只替换应用本身和系统配置。

### 6.4 回滚机制

如果更新后的应用出现严重问题，用户可以回滚到上一个版本。应用商店服务保留每个应用的最近三个版本文件，支持一键回滚：

```c
/* 回滚到上一版本 */
int appstore_rollback(const char* app_id);
```

回滚操作将当前版本的应用文件替换为上一版本的文件，并更新注册表中的版本号。用户数据不会被回滚，因为数据目录与应用文件是分开存储的。如果回滚后问题仍然存在，用户可以继续回滚到更早的版本，直到找到可用的版本。

### 6.5 版本兼容性

应用的 `min_os_version` 字段声明了最低兼容的 WebOS 系统版本。当用户尝试在不兼容的系统版本上安装应用时，安装程序会显示不兼容提示并阻止安装。开发者在发布新版本时，应注意以下兼容性规则：

- 如果新版本使用了新引入的系统调用或服务 API，必须在 `min_os_version` 中指定引入这些 API 的最低系统版本。
- 如果新版本只是修复 bug 或新增不需要新 API 的功能，不应修改 `min_os_version`，以保持对旧系统版本的支持。
- 主版本号升级时（如 1.x → 2.x），建议同时提高 `min_os_version`，因为破坏性变更可能依赖较新的系统功能。
