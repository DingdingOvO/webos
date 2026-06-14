# ISO 与网站

---

## ISO 构建

WebOS ISO 是一个可启动的镜像，包含 Linux + Chromium kiosk 模式，在物理机或虚拟机中运行。

### ISO 架构

```
┌─────────────────────────────────┐
│         GRUB Bootloader         │
├─────────────────────────────────┤
│         Linux Kernel            │
├─────────────────────────────────┤
│         initramfs               │
│  ┌─────────────────────────┐   │
│  │  X11 / Wayland          │   │
│  │  Chromium (kiosk mode)  │   │
│  │  WebOS Host Bundle      │   │
│  └─────────────────────────┘   │
├─────────────────────────────────┤
│  SquashFS (WebOS 文件系统)      │
└─────────────────────────────────┘
```

### 启动流程

```
BIOS/UEFI → GRUB → Linux Kernel → initramfs →
X11 启动 → Chromium --kiosk → 加载 index.html → WebOS 启动链
```

### 构建工具

- `tools/build_iso.sh` — 完整 ISO 构建
- `tools/create_initramfs.sh` — 创建 initramfs
- `tools/installer.sh` — 磁盘安装程序

### 安装程序

ISO 包含 GRUB 菜单选项:
1. **Live Mode** — 直接运行，不修改硬盘
2. **WebOS Installer** — 安装到磁盘

安装程序功能:
- 磁盘分区（自动/手动）
- 格式化（ext4）
- 复制文件系统
- 安装 GRUB
- 创建 swap 分区

---

## 官方网站

### 设计原则

干净的、技术感的设计。参考旧版 DingDing-bbb/webos 的 commit ac607e0c。3 个页面，不要花哨。

### 页面

#### 1. Intro（首页）

- 大标题: **WebOS**
- 副标题: "A real operating system running in your browser"
- 核心特性展示（4 个）:
  - WebGPU Rendered — 所有像素自己画
  - WASM Powered — C/C++ 内核编译为 WebAssembly
  - True Filesystem — WebFS 带真实 inode 和数据块
  - No DOM, No XSS — 画布上没有 DOM 节点
- "Try it now" 按钮 → 链接到 /app/os
- 底部: GitHub 链接 + 版本号

#### 2. Docs（文档）

- 左侧: 文档目录导航
- 右侧: Markdown 渲染的文档内容
- 文档来源: `docs/` 目录

#### 3. Demo（在线体验）

- 全屏嵌入 WebOS 实例
- 通过 iframe 加载 WebOS host 页面

### 技术栈

- Next.js 16 + React 19
- Tailwind CSS 4
- 暗色主题，配色与 WebOS 一致（#0F172A 背景，#3B82F6 强调色）
