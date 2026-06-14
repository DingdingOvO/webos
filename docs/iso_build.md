# WebOS ISO 构建指南

> WebOS v0.0.1beta — 可启动内存操作系统 ISO 镜像构建文档

## 目录

- [概述](#概述)
- [系统要求](#系统要求)
- [构建命令](#构建命令)
- [启动 ISO](#启动-iso)
- [使用安装程序](#使用安装程序)
- [ISO 内部结构](#iso-内部结构)
- [故障排除](#故障排除)

---

## 概述

WebOS ISO 是一个可启动的内存操作系统镜像，包含以下功能：

- **GRUB 引导加载器** — 支持 UEFI 和 BIOS 启动
- **Linux 内核** — 从宿主系统复用
- **自定义 initramfs** — 最小化 Linux 环境，自动启动 Chromium 浏览器
- **WebOS 应用** — 自包含的 HTML/JS 桌面操作系统
- **磁盘安装程序** — 可将 WebOS 安装到硬盘

启动后，系统会在内存中运行，以 Chromium Kiosk 模式自动加载 WebOS 桌面环境。

---

## 系统要求

### 构建环境要求

| 工具 | 用途 | 安装命令 (Debian/Ubuntu) |
|------|------|--------------------------|
| `xorriso` | ISO 镜像创建 | `sudo apt install xorriso` |
| `grub-pc-bin` | BIOS 引导支持 | `sudo apt install grub-pc-bin` |
| `grub-efi-amd64-bin` | UEFI 引导支持 | `sudo apt install grub-efi-amd64-bin` |
| `mtools` | FAT 文件系统工具 | `sudo apt install mtools` |
| `cpio` | initramfs 打包 | `sudo apt install cpio` |
| `gzip` | initramfs 压缩 | 通常已安装 |
| `busybox-static` | initramfs 工具集（推荐） | `sudo apt install busybox-static` |
| `parted` | 磁盘分区（安装程序） | `sudo apt install parted` |
| `Linux 内核` | 启动所需 | 通常已安装 |

一键安装所有依赖：

```bash
# Debian / Ubuntu
sudo apt install xorriso grub-pc-bin grub-efi-amd64-bin mtools cpio busybox-static parted dosfstools e2fsprogs

# Fedora
sudo dnf install xorriso grub2-pc grub2-efi-x64 mtools cpio busybox parted dosfstools e2fsprogs

# Arch Linux
sudo pacman -S libburn grub mtools cpio busybox parted dosfstools e2fsprogs
```

### 运行环境要求（ISO 启动后）

- **内存**: 最低 2GB，推荐 4GB+
- **CPU**: x86_64 架构
- **显卡**: 支持 VESA 或原生 GPU 驱动
- **磁盘**: 安装到硬盘需 2GB+ 空间

---

## 构建命令

### 完整构建

构建所有组件（WASM 模块、TypeScript 运行时、initramfs），然后创建可启动 ISO：

```bash
make iso-full
```

或直接使用脚本：

```bash
chmod +x tools/build_iso.sh
./tools/build_iso.sh dist/webos-0.0.1beta.iso
```

输出文件：`dist/webos-0.0.1beta.iso`

### 快速构建

跳过依赖检查和内核搜索，仅打包数据文件：

```bash
make iso-quick
```

或：

```bash
./tools/build_iso.sh dist/webos.iso --quick
```

### 仅构建 initramfs

```bash
make initramfs
```

或：

```bash
sudo ./tools/create_initramfs.sh build/initrd.img
```

### 仅构建安装程序

```bash
make installer
```

安装程序已包含在 ISO 中，也可单独运行：

```bash
sudo ./tools/installer.sh /dev/sdX
```

### 清理构建产物

```bash
# 清理所有
make clean

# 仅清理 ISO 产物
make clean-iso
```

---

## 启动 ISO

### 使用 QEMU 测试

```bash
# 基本测试（2GB 内存）
qemu-system-x86_64 -cdrom dist/webos-0.0.1beta.iso -m 2G

# 带 UEFI 支持
qemu-system-x86_64 -cdrom dist/webos-0.0.1beta.iso -m 2G \
    -bios /usr/share/OVMF/OVMF_CODE.fd

# 带网络
qemu-system-x86_64 -cdrom dist/webos-0.0.1beta.iso -m 2G \
    -net nic -net user

# 全屏模式
qemu-system-x86_64 -cdrom dist/webos-0.0.1beta.iso -m 2G -full-screen
```

### 使用 VirtualBox

1. 创建新虚拟机 → 类型: Linux → 版本: Linux 2.6/3.x/4.x (64-bit)
2. 内存: 2048 MB+
3. 显示: 启用 3D 加速
4. 存储: 将 ISO 挂载为光驱
5. 启动虚拟机

### 使用真实硬件

1. 将 ISO 写入 USB 启动盘：
   ```bash
   sudo dd if=dist/webos-0.0.1beta.iso of=/dev/sdX bs=4M status=progress
   ```
2. 重启电脑，进入 BIOS/UEFI 设置
3. 选择从 USB 或光驱启动
4. 在 GRUB 菜单选择启动选项

### GRUB 启动菜单选项

| 选项 | 说明 |
|------|------|
| **WebOS v0.0.1beta (Memory OS)** | 默认启动，从内存运行 |
| **WebOS v0.0.1beta (Safe Mode)** | VESA 安全模式，适用于显卡兼容问题 |
| **WebOS v0.0.1beta (Installer)** | 启动安装程序，将 WebOS 安装到硬盘 |
| **WebOS v0.0.1beta (Debug Console)** | 调试模式，开启文本控制台 |
| **Reboot** | 重启电脑 |
| **Power Off** | 关闭电脑 |

---

## 使用安装程序

### 从 ISO 启动安装

1. 启动 ISO，在 GRUB 菜单选择 **WebOS v0.0.1beta (Installer)**
2. 系统会启动浏览器并显示安装界面
3. 选择目标磁盘，点击 **Install WebOS**

### 从命令行安装

```bash
# 查看可用磁盘
sudo ./tools/installer.sh --list-disks

# 交互式安装
sudo ./tools/installer.sh

# 指定目标磁盘
sudo ./tools/installer.sh /dev/sda

# 模拟安装（不实际写入）
sudo ./tools/installer.sh --dry-run /dev/sda

# 自动确认（用于脚本化安装）
sudo ./tools/installer.sh --yes /dev/sda

# 自定义主机名
sudo ./tools/installer.sh --hostname my-webos /dev/sda
```

### 安装程序参数

| 参数 | 说明 |
|------|------|
| `--list-disks` | 列出可用磁盘并退出 |
| `--dry-run` | 模拟安装，不写入磁盘 |
| `--yes`, `-y` | 自动确认磁盘格式化 |
| `--hostname NAME` | 设置主机名（默认: webos） |
| `--username NAME` | 设置用户名（默认: webos） |
| `--efi-size SIZE` | EFI 分区大小 MB（默认: 512） |
| `-h`, `--help` | 显示帮助信息 |

### 安装过程

安装程序会执行以下步骤：

1. **分区磁盘** — 创建 GPT 分区表，EFI (512MB FAT32) + Root (ext4)
2. **格式化分区** — EFI 使用 FAT32，Root 使用 ext4
3. **挂载分区** — 挂载到 `/mnt/webos/`
4. **创建文件系统** — 创建目录结构和设备节点
5. **复制 WebOS 文件** — 内核、initramfs、WebOS 应用
6. **配置系统** — fstab、hostname、os-release
7. **安装 GRUB** — 引导加载器（UEFI 或 BIOS）

安装完成后，移除安装介质并重启即可从硬盘启动 WebOS。

---

## ISO 内部结构

```
webos-0.0.1beta.iso
├── boot/
│   ├── grub/
│   │   └── grub.cfg          # GRUB 引导配置
│   ├── vmlinuz                # Linux 内核
│   └── initrd.img             # 自定义 initramfs
├── webos/
│   ├── index.html             # WebOS 主应用（自包含）
│   ├── wasm/                  # WASM 模块
│   │   ├── kernel.wasm
│   │   ├── bootloader.wasm
│   │   ├── gpu_driver.Wdll
│   │   └── ...
│   ├── wallpapers/            # 壁纸
│   │   └── catgirl-static.png
│   ├── js/                    # JavaScript 运行时
│   │   └── webos-host.js
│   └── css/                   # 样式表
├── installer/
│   └── install.sh             # 磁盘安装脚本
├── EFI/
│   └── BOOT/                  # UEFI 引导文件
└── README.txt                 # 说明文件
```

### initramfs 内部结构

```
initrd.img (cpio+gzip)
├── init                       # 启动脚本 (PID 1)
├── bin/                       # 基础命令 (busybox)
├── sbin/                      # 系统命令
├── etc/                       # 配置文件
│   ├── hostname
│   ├── hosts
│   ├── resolv.conf
│   ├── passwd / group / shadow
│   ├── os-release
│   ├── profile
│   └── X11/
│       └── xorg.conf
├── dev/                       # 设备节点
├── proc/                      # procfs 挂载点
├── sys/                       # sysfs 挂载点
├── webos/                     # WebOS 应用文件
├── installer/                 # 安装脚本
└── root/                      # root 用户主目录
    └── .xinitrc               # X11 kiosk 启动脚本
```

### 启动流程

```
BIOS/UEFI
  → GRUB 引导加载器
    → Linux 内核 (vmlinuz)
      → initramfs (initrd.img)
        → /init 脚本
          → 挂载 proc/sys/dev
          → 加载内核模块 (GPU, 输入, 网络, 存储)
          → 配置网络 (DHCP)
          → 启动 X11 服务器
            → Chromium Kiosk 模式
              → file:///webos/index.html
                → WebOS 桌面环境
```

---

## 故障排除

### ISO 无法启动

**症状**: 插入 USB 后无法从 ISO 启动

**解决方案**:
1. 检查 BIOS/UEFI 启动顺序，确保 USB/光驱优先
2. 确认 UEFI 模式与 ISO 匹配（UEFI ISO 需要 UEFI 启动模式）
3. 使用 `dd` 重新写入 USB：
   ```bash
   sudo dd if=webos-0.0.1beta.iso of=/dev/sdX bs=4M status=progress && sync
   ```
4. 尝试 GRUB 菜单中的 **Safe Mode**

### 黑屏 / 无显示

**症状**: 内核启动后屏幕无输出

**解决方案**:
1. 使用 **Safe Mode (VESA)** 启动
2. 添加内核参数 `nomodeset` 或 `vga=788`
3. 使用 **Debug Console** 启动，查看日志：
   ```
   cat /var/log/xorg.log
   dmesg | grep -i error
   ```

### 浏览器无法启动

**症状**: X 启动了但没有浏览器

**解决方案**:
1. 检查 Chromium 是否已安装：
   ```bash
   which chromium-browser chromium google-chrome
   ```
2. 如果缺少浏览器，使用 HTTP 服务器：
   ```bash
   cd /webos && python3 -m http.server 8080
   ```
3. 从其他设备的浏览器访问 `http://<IP>:8080`

### 网络不可用

**症状**: 无法连接网络

**解决方案**:
1. 检查网络接口：
   ```bash
   ip link show
   ```
2. 手动配置 DHCP：
   ```bash
   udhcpc -i eth0
   # 或
   dhclient eth0
   ```
3. 手动设置 IP：
   ```bash
   ip addr add 192.168.1.100/24 dev eth0
   ip route add default via 192.168.1.1
   echo "nameserver 8.8.8.8" > /etc/resolv.conf
   ```

### 安装程序失败

**症状**: 安装程序报错

**解决方案**:
1. 确保以 root 运行：`sudo ./tools/installer.sh`
2. 检查目标磁盘是否存在：`lsblk`
3. 确保目标磁盘不是当前系统盘
4. 使用 `--dry-run` 模拟安装排查问题
5. 检查分区工具是否安装：`parted`, `mkfs.ext4`, `mkfs.vfat`

### GRUB 安装失败

**症状**: `grub-install` 命令失败

**解决方案**:
1. 确认安装了正确的 GRUB 包：
   ```bash
   # UEFI
   sudo apt install grub-efi-amd64-bin
   # BIOS
   sudo apt install grub-pc-bin
   ```
2. 手动安装 GRUB：
   ```bash
   # UEFI
   grub-install --target=x86_64-efi --efi-directory=/boot/efi --bootloader-id=WebOS
   # BIOS
   grub-install --target=i386-pc /dev/sda
   ```

### initramfs 构建失败

**症状**: `create_initramfs.sh` 报错

**解决方案**:
1. 确保安装了 `busybox-static`：
   ```bash
   sudo apt install busybox-static
   ```
2. 确保 `cpio` 和 `gzip` 已安装
3. 需要 root 权限创建设备节点：
   ```bash
   sudo ./tools/create_initramfs.sh
   ```

### 内存不足

**症状**: 运行缓慢或崩溃

**解决方案**:
1. 增加 QEMU 内存：`-m 4G`
2. 关闭不需要的服务
3. 使用轻量级浏览器（如果 Chromium 太重）

---

## 高级主题

### 自定义 WebOS 应用

修改 `tools/build_iso.sh` 中 Step 3 的 `index.html` 内容，或修改项目源码后重新构建：

```bash
# 构建所有 WASM 模块
make all

# 构建并打包 ISO
make iso-full
```

### 添加自定义内核

将自定义内核放置到以下位置之一：

- `/boot/vmlinuz-<version>`
- `<project>/build/kernel/vmlinuz`

然后重新构建 ISO。

### 创建自定义 initramfs

编辑 `tools/create_initramfs.sh` 中的 init 脚本，添加自定义启动逻辑。

### 在已有 Linux 系统上运行 WebOS

无需 ISO，直接在浏览器中运行：

```bash
# 开发服务器
make serve

# 或使用 Python HTTP 服务器
cd public && python3 -m http.server 8080
```

然后访问 `http://localhost:8080`。

---

*WebOS v0.0.1beta — 构建日期: 2025*
