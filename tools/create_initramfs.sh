#!/bin/bash
# ─────────────────────────────────────────────────────────
# WebOS Initramfs Creator v0.0.1beta
# Creates a minimal initramfs that boots into a Linux
# environment and launches Chromium in kiosk mode
# pointing to the WebOS application
#
# Usage:
#   sudo ./tools/create_initramfs.sh [output_path]
#   sudo ./tools/create_initramfs.sh                       # Default
#   sudo ./tools/create_initramfs.sh build/initrd.img     # Custom path
#
# Requirements: root (for device nodes), cpio, gzip
# Optional: busybox-static or a working /bin/sh
# ─────────────────────────────────────────────────────────

set -e

VERSION="0.0.1beta"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
OUTPUT_PATH="${1:-${PROJECT_DIR}/build/initrd.img}"

# ─── Color helpers ────────────────────────────────────────
RED='\033[0;31m'
GREEN='\033[0;32m'
CYAN='\033[0;36m'
YELLOW='\033[1;33m'
NC='\033[0m'

log()   { echo -e "${CYAN}[initramfs]${NC} $1"; }
ok()    { echo -e "${GREEN}[OK]${NC} $1"; }
warn()  { echo -e "${YELLOW}[WARN]${NC} $1"; }
err()   { echo -e "${RED}[ERROR]${NC} $1"; }

echo ""
echo "═══════════════════════════════════════════════════════════"
echo "  WebOS Initramfs Creator v${VERSION}"
echo "═══════════════════════════════════════════════════════════"
echo ""

# ─── Check dependencies ──────────────────────────────────
for tool in cpio gzip; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        err "Required tool '$tool' not found."
        exit 1
    fi
done

# ─── Create initramfs directory ──────────────────────────
INITRAMFS_DIR=$(mktemp -d /tmp/webos-initramfs.XXXXXX)
trap "rm -rf '$INITRAMFS_DIR'" EXIT

log "Creating initramfs at $INITRAMFS_DIR"

# ─── Create directory structure ──────────────────────────
log "Creating directory structure..."
mkdir -p "${INITRAMFS_DIR}"/{bin,sbin,usr/bin,usr/sbin,usr/lib,lib,lib64}
mkdir -p "${INITRAMFS_DIR}"/{etc,etc/X11,etc/ssl,etc/ssl/certs}
mkdir -p "${INITRAMFS_DIR}"/{proc,sys,dev,dev/pts,dev/shm,dev/mapper}
mkdir -p "${INITRAMFS_DIR}"/{tmp,run,var/log,var/run,var/lib,var/tmp}
mkdir -p "${INITRAMFS_DIR}"/{root,home,mnt,webos,installer,boot}
mkdir -p "${INITRAMFS_DIR}"/{sys/firmware/efi/efivars}
mkdir -p "${INITRAMFS_DIR}"/etc/X11/xorg.conf.d

# ─── Create device nodes ────────────────────────────────
log "Creating device nodes..."
mknod -m 622 "${INITRAMFS_DIR}/dev/console" c 5 1 2>/dev/null || true
mknod -m 666 "${INITRAMFS_DIR}/dev/null" c 1 3 2>/dev/null || true
mknod -m 666 "${INITRAMFS_DIR}/dev/zero" c 1 5 2>/dev/null || true
mknod -m 666 "${INITRAMFS_DIR}/dev/tty" c 5 0 2>/dev/null || true
mknod -m 666 "${INITRAMFS_DIR}/dev/tty0" c 4 0 2>/dev/null || true
mknod -m 666 "${INITRAMFS_DIR}/dev/tty1" c 4 1 2>/dev/null || true
mknod -m 666 "${INITRAMFS_DIR}/dev/tty2" c 4 2 2>/dev/null || true
mknod -m 666 "${INITRAMFS_DIR}/dev/tty3" c 4 3 2>/dev/null || true
mknod -m 666 "${INITRAMFS_DIR}/dev/tty4" c 4 4 2>/dev/null || true
mknod -m 444 "${INITRAMFS_DIR}/dev/random" c 1 8 2>/dev/null || true
mknod -m 444 "${INITRAMFS_DIR}/dev/urandom" c 1 9 2>/dev/null || true
mknod -m 666 "${INITRAMFS_DIR}/dev/input/mice" c 13 63 2>/dev/null || true
mknod -m 666 "${INITRAMFS_DIR}/dev/input/mouse0" c 13 32 2>/dev/null || true
mknod -m 666 "${INITRAMFS_DIR}/dev/agpgart" c 10 175 2>/dev/null || true
mkdir -p "${INITRAMFS_DIR}/dev/input"
mknod -m 660 "${INITRAMFS_DIR}/dev/input/event0" c 13 64 2>/dev/null || true
mknod -m 660 "${INITRAMFS_DIR}/dev/input/event1" c 13 65 2>/dev/null || true
mknod -m 660 "${INITRAMFS_DIR}/dev/input/event2" c 13 66 2>/dev/null || true

ok "Device nodes created"

# ─── Install busybox or shell ────────────────────────────
log "Installing shell and basic utilities..."

SHELL_FOUND=false

# Prefer busybox-static (self-contained, no libs needed)
for bb in busybox busybox-static; do
    BB_PATH=$(command -v "$bb" 2>/dev/null || true)
    if [ -n "$BB_PATH" ]; then
        cp "$BB_PATH" "${INITRAMFS_DIR}/bin/busybox"
        chmod +x "${INITRAMFS_DIR}/bin/busybox"
        
        # Create busybox symlinks for common utilities
        BUSYBOX_CMDS="sh ash ls cat echo mkdir rm cp mv ln mount umount \
                      sleep date pwd id whoami hostname dmesg kill ps \
                      grep sed awk head tail wc cut tr sort uniq tee \
                      test expr printf env clear reset vi ed \
                      ifconfig ip route ping wget curl \
                      tar gzip gunzip zcat bzip2 bunzip2 xz unxz \
                      cpio find xargs dirname basename realpath \
                      mknod chmod chown chgrp \
                      df du free top \
                      losetup blkid fdisk sfdisk parted \
                      mkfs.ext4 mkfs.vfat mkfs.xfs \
                      fsck fsck.ext4 \
                      sync reboot halt poweroff \
                      init switch_root \
                      dd tr hexdump \
                      mdev sysctl \
                      udhcpc dhclient \
                      startx xinit Xorg \
                      chroot pivot_root"
        
        for cmd in $BUSYBOX_CMDS; do
            ln -sf busybox "${INITRAMFS_DIR}/bin/$cmd" 2>/dev/null || true
        done
        
        SHELL_FOUND=true
        ok "Busybox installed from $BB_PATH"
        break
    fi
done

# Fallback: copy system sh and core utils
if [ "$SHELL_FOUND" = false ]; then
    warn "Busybox not found, copying system binaries..."
    
    # Copy essential binaries
    ESSENTIAL_BINS="sh bash ls cat echo mkdir rm cp mv mount umount sleep date \
                    hostname dmesg kill ps grep sed awk chmod chown \
                    mknod sync reboot halt poweroff ifconfig ip \
                    mkfs.ext4 mkfs.vfat parted blkid fdisk \
                    udhcpc dhclient curl wget \
                    startx xinit Xorg chromium chromium-browser \
                    firefox google-chrome"
    
    for bin in $ESSENTIAL_BINS; do
        BIN_PATH=$(command -v "$bin" 2>/dev/null || true)
        if [ -n "$BIN_PATH" ]; then
            cp "$BIN_PATH" "${INITRAMFS_DIR}/bin/" 2>/dev/null || true
            # Copy library dependencies
            if command -v ldd >/dev/null 2>&1; then
                ldd "$BIN_PATH" 2>/dev/null | grep -oP '(/[^\s]+)' | while read lib; do
                    if [ -f "$lib" ]; then
                        LIB_DIR=$(dirname "$lib")
                        mkdir -p "${INITRAMFS_DIR}${LIB_DIR}"
                        cp -f "$lib" "${INITRAMFS_DIR}${LIB_DIR}/" 2>/dev/null || true
                    fi
                done
            fi
        fi
    done
    
    SHELL_FOUND=true
    ok "System binaries copied (with library dependencies)"
fi

# ─── Copy WebOS files ────────────────────────────────────
log "Copying WebOS application files..."

# Check for built ISO webos files
WEBOS_SRC=""
for src in \
    "${PROJECT_DIR}/build/iso/webos" \
    "${PROJECT_DIR}/public"; do
    if [ -d "$src" ]; then
        WEBOS_SRC="$src"
        break
    fi
done

if [ -n "$WEBOS_SRC" ]; then
    # Copy index.html
    if [ -f "$WEBOS_SRC/index.html" ]; then
        cp "$WEBOS_SRC/index.html" "${INITRAMFS_DIR}/webos/"
    fi
    
    # Copy WASM modules
    if [ -d "$WEBOS_SRC/wasm" ]; then
        cp -r "$WEBOS_SRC/wasm/"* "${INITRAMFS_DIR}/webos/wasm/" 2>/dev/null || true
    fi
    
    # Copy wallpapers
    if [ -d "$WEBOS_SRC/wallpapers" ]; then
        cp -r "$WEBOS_SRC/wallpapers/"* "${INITRAMFS_DIR}/webos/wallpapers/" 2>/dev/null || true
    fi
    
    # Copy JS
    if [ -d "$WEBOS_SRC/js" ]; then
        cp -r "$WEBOS_SRC/js/"* "${INITRAMFS_DIR}/webos/js/" 2>/dev/null || true
    fi
    
    # Copy CSS
    if [ -d "$WEBOS_SRC/css" ]; then
        cp -r "$WEBOS_SRC/css/"* "${INITRAMFS_DIR}/webos/css/" 2>/dev/null || true
    fi
    
    WASM_COUNT=$(find "${INITRAMFS_DIR}/webos/wasm" -type f 2>/dev/null | wc -l)
    ok "WebOS files copied (${WASM_COUNT} WASM modules)"
else
    warn "No WebOS source files found. Creating placeholder..."
    cat > "${INITRAMFS_DIR}/webos/index.html" << 'PLACEHOLDER'
<!DOCTYPE html>
<html><head><meta charset="UTF-8"><title>WebOS</title>
<style>*{margin:0;padding:0}body{background:#0a0a1a;color:#fff;display:flex;align-items:center;justify-content:center;height:100vh;font-family:sans-serif}</style>
</head><body><div style="text-align:center"><h1>Web<span style="color:#0078d4">OS</span></h1><p>v0.0.1beta</p></div></body></html>
PLACEHOLDER
fi

# ─── Copy installer ──────────────────────────────────────
if [ -f "${PROJECT_DIR}/tools/installer.sh" ]; then
    cp "${PROJECT_DIR}/tools/installer.sh" "${INITRAMFS_DIR}/installer/install.sh"
    chmod +x "${INITRAMFS_DIR}/installer/install.sh"
    ok "Installer copied"
fi

# ─── Create init script ─────────────────────────────────
log "Creating init script..."

cat > "${INITRAMFS_DIR}/init" << 'INITEOF'
#!/bin/sh
# ═══════════════════════════════════════════════════════════
# WebOS Init v0.0.1beta
# ─────────────────────────────────────────────────────────
# This is PID 1. It runs before anything else and is
# responsible for mounting filesystems, loading drivers,
# configuring networking, and starting the display server
# that runs WebOS in a browser kiosk.
# ═══════════════════════════════════════════════════════════

VERSION="0.0.1beta"

# Parse kernel command line for WebOS mode
WEBOS_MODE="desktop"
for arg in $(cat /proc/cmdline 2>/dev/null); do
    case "$arg" in
        webos_mode=*) WEBOS_MODE="${arg#webos_mode=}" ;;
        debug)        WEBOS_MODE="debug" ;;
    esac
done

# Color helpers
RED='\033[0;31m'; GREEN='\033[0;32m'; CYAN='\033[0;36m'
YELLOW='\033[1;33m'; NC='\033[0m'
log()  { echo -e "${CYAN}[webos]${NC} $1"; }
ok()   { echo -e "${GREEN}[OK]${NC} $1"; }
warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
err()  { echo -e "${RED}[ERR]${NC} $1"; }

# ─── Phase 1: Essential mounts ──────────────────────────
log "Phase 1: Mounting essential filesystems..."
mount -t proc proc /proc
mount -t sysfs sysfs /sys
mount -t devtmpfs devtmpfs /dev
mkdir -p /dev/pts /dev/shm /run /tmp /var/run /var/log
mount -t devpts devpts /dev/pts
mount -t tmpfs tmpfs /dev/shm
mount -t tmpfs tmpfs /tmp
mount -t tmpfs tmpfs /run
mount -t tmpfs tmpfs /var/log

# Mount cgroups if available
if [ -d /sys/fs/cgroup ]; then
    mount -t cgroup2 cgroup2 /sys/fs/cgroup 2>/dev/null || true
fi

# Mount EFI vars if available
if [ -d /sys/firmware/efi ]; then
    mount -t efivarfs efivarfs /sys/firmware/efi/efivars 2>/dev/null || true
fi

ok "Essential filesystems mounted"

# ─── Phase 2: Console setup ─────────────────────────────
log "Phase 2: Console setup..."
echo 0 > /proc/sys/kernel/printk 2>/dev/null
echo "6 4 1 7" > /proc/sys/kernel/printk 2>/dev/null || true
hostname webos
echo "webos" > /etc/hostname
echo "127.0.0.1 localhost webos" > /etc/hosts

# Create /etc/resolv.conf for DNS
echo "nameserver 8.8.8.8" > /etc/resolv.conf
echo "nameserver 8.8.4.4" >> /etc/resolv.conf

ok "Console configured"

# ─── Phase 3: Hardware detection & module loading ────────
log "Phase 3: Loading kernel modules..."

# GPU drivers
for mod in i915 amdgpu nouveau nvidia radeon vmwgfx vesafb efifb; do
    modprobe "$mod" 2>/dev/null && ok "  GPU: $mod" || true
done

# Input drivers
for mod in hid usbhid evdev mousedev joydev atkbd i8042 serio; do
    modprobe "$mod" 2>/dev/null || true
done

# Storage drivers
for mod in ahci nvme sd_mod sr_mod usb-storage ehci-hcd xhci-hcd uhci-hcd; do
    modprobe "$mod" 2>/dev/null || true
done

# Network drivers
for mod in e1000 e1000e r8169 realtek tg3 bnx2 bnx2x sky2 atl1c alx virtio_net; do
    modprobe "$mod" 2>/dev/null || true
done

# Sound drivers
for mod in snd snd-pcm snd-hda-intel snd-hda-codec; do
    modprobe "$mod" 2>/dev/null || true
done

# Filesystem drivers
for mod in ext4 vfat ntfs3 btrfs xfs iso9660 udf nls_cp437 nls_iso8859_1 nls_utf8; do
    modprobe "$mod" 2>/dev/null || true
done

ok "Kernel modules loaded"

# ─── Phase 4: Network configuration ─────────────────────
log "Phase 4: Configuring network..."

# Bring up loopback
ip link set lo up 2>/dev/null || ifconfig lo up 2>/dev/null || true

# Wait for network interfaces to appear
sleep 2

# Configure ethernet via DHCP
for iface in /sys/class/net/e*; do
    [ -e "$iface" ] || continue
    ifname=$(basename "$iface")
    log "  Configuring $ifname via DHCP..."
    ip link set "$ifname" up 2>/dev/null || true
    
    # Try udhcpc first (busybox), then dhclient
    if command -v udhcpc >/dev/null 2>&1; then
        # Create a simple udhcpc script
        mkdir -p /usr/share/udhcpc
        cat > /usr/share/udhcpc/default.script << 'DHCPC'
#!/bin/sh
case "$1" in
    bound|renew)
        ip addr flush dev "$interface"
        ip addr add "$ip/$mask" dev "$interface"
        if [ -n "$router" ]; then
            ip route add default via "$router" dev "$interface"
        fi
        if [ -n "$dns" ]; then
            echo "nameserver $dns" > /etc/resolv.conf
        fi
        ;;
esac
DHCPC
        chmod +x /usr/share/udhcpc/default.script
        udhcpc -i "$ifname" -s /usr/share/udhcpc/default.script -q -n 2>/dev/null && ok "  $ifname: DHCP configured" || true
    elif command -v dhclient >/dev/null 2>&1; then
        dhclient -1 -q "$ifname" 2>/dev/null && ok "  $ifname: DHCP configured" || true
    fi
done

# Configure WiFi if available (basic)
for iface in /sys/class/net/wl*; do
    [ -e "$iface" ] || continue
    ifname=$(basename "$iface")
    ip link set "$ifname" up 2>/dev/null || true
    log "  WiFi $ifname available (manual configuration needed)"
done

ok "Network configured"

# ─── Phase 5: X11 / Wayland setup ───────────────────────
log "Phase 5: Setting up display server..."

# Create X11 configuration
mkdir -p /etc/X11/xorg.conf.d

# Minimal X.org config
cat > /etc/X11/xorg.conf << 'XORGCONF'
Section "ServerFlags"
    Option "DontZap" "true"
    Option "AllowMouseOpenFail" "true"
    Option "AutoAddDevices" "true"
    Option "AutoAddGPU" "true"
    Option "BlankTime" "0"
    Option "StandbyTime" "0"
    Option "SuspendTime" "0"
    Option "OffTime" "0"
EndSection

Section "InputClass"
    Identifier "Keyboard Defaults"
    MatchIsKeyboard "yes"
    Option "XkbLayout" "us"
EndSection

Section "Monitor"
    Identifier "Monitor0"
    Option "DPMS" "off"
EndSection
XORGCONF

# Create xinitrc
INSTALLER_ARG=""
if [ "$WEBOS_MODE" = "install" ]; then
    INSTALLER_ARG="?mode=install"
fi

cat > /root/.xinitrc << XINITRC
#!/bin/sh
# WebOS Kiosk xinitrc

# Disable screen blanking and power management
xset s off
xset -dpms
xset s noblank

# Set background
xsetroot -solid "#0a0a1a"

# Wait for X to be fully ready
sleep 1

# Find a browser
BROWSER=""
for cmd in chromium-browser chromium google-chrome-stable google-chrome firefox midori qemu-web-browser; do
    if command -v \$cmd >/dev/null 2>&1; then
        BROWSER=\$cmd
        break
    fi
done

if [ -n "\$BROWSER" ]; then
    echo "[webos] Launching browser: \$BROWSER"
    case "\$BROWSER" in
        *chromium*|*chrome*)
            exec \$BROWSER \\
                --no-sandbox \\
                --kiosk \\
                --disable-gpu-compositing \\
                --disable-software-rasterizer \\
                --allow-file-access-from-files \\
                --allow-file-access \\
                --enable-features=OverlayScrollbar \\
                --disable-features=TranslateUI \\
                --no-first-run \\
                --no-default-browser-check \\
                --disable-infobars \\
                --disable-session-crashed-bubble \\
                --disable-breakpad \\
                --disable-pinch \\
                --overscroll-history-navigation=0 \\
                --inactivity-gap=0 \\
                "file:///webos/index.html${INSTALLER_ARG}"
            ;;
        *firefox*)
            exec \$BROWSER \\
                --kiosk \\
                "file:///webos/index.html${INSTALLER_ARG}"
            ;;
        *)
            exec \$BROWSER "file:///webos/index.html${INSTALLER_ARG}"
            ;;
    esac
else
    echo "[webos] No browser found. Starting HTTP server instead."
    if command -v python3 >/dev/null 2>&1; then
        cd /webos && python3 -m http.server 8080 &
    fi
    xterm -e "echo 'No browser found. WebOS files at /webos/'; exec /bin/sh"
fi
XINITRC
chmod +x /root/.xinitrc

# ─── Phase 6: Start display ─────────────────────────────
log "Phase 6: Starting display..."

DISPLAY_RUNNING=false

# Method 1: startx (most common)
if command -v startx >/dev/null 2>&1 && [ "$DISPLAY_RUNNING" = false ]; then
    log "  Starting X via startx..."
    startx 2>/var/log/xorg.log &
    X_PID=$!
    sleep 3
    if kill -0 $X_PID 2>/dev/null; then
        DISPLAY_RUNNING=true
        ok "  X server running (PID: $X_PID)"
    else
        warn "  startx failed"
    fi
fi

# Method 2: Direct Xorg
if command -v Xorg >/dev/null 2>&1 && [ "$DISPLAY_RUNNING" = false ]; then
    log "  Starting Xorg directly..."
    Xorg :0 vt7 -noreset 2>/var/log/xorg.log &
    X_PID=$!
    sleep 3
    export DISPLAY=:0
    if kill -0 $X_PID 2>/dev/null; then
        DISPLAY_RUNNING=true
        ok "  Xorg running"
        /bin/sh /root/.xinitrc &
    else
        warn "  Xorg failed"
    fi
fi

# Method 3: Xwayland (if Wayland available)
if command -v Xwayland >/dev/null 2>&1 && [ "$DISPLAY_RUNNING" = false ]; then
    log "  Trying Xwayland..."
    # Start a minimal Wayland compositor if available
    if command -v weston >/dev/null 2>&1; then
        weston --xwayland &
        sleep 3
        export DISPLAY=:0
        export WAYLAND_DISPLAY=wayland-0
        /bin/sh /root/.xinitrc &
        DISPLAY_RUNNING=true
    fi
fi

# ─── Phase 7: Fallback ──────────────────────────────────
if [ "$DISPLAY_RUNNING" = false ]; then
    err "No display server available"
    echo ""
    echo "╔═══════════════════════════════════════════════╗"
    echo "║   WebOS v${VERSION} — Console Mode              ║"
    echo "╠═══════════════════════════════════════════════╣"
    echo "║                                               ║"
    echo "║   WebOS files: /webos/                        ║"
    echo "║   Installer:   /installer/install.sh          ║"
    echo "║                                               ║"
    echo "║   Start browser manually:                     ║"
    echo "║     chromium file:///webos/index.html         ║"
    echo "║     firefox --kiosk file:///webos/index.html  ║"
    echo "║                                               ║"
    echo "║   Start HTTP server:                          ║"
    echo "║     cd /webos && python3 -m http.server 8080  ║"
    echo "║                                               ║"
    echo "║   Install to disk:                            ║"
    echo "║     /installer/install.sh                     ║"
    echo "║                                               ║"
    echo "╚═══════════════════════════════════════════════╝"
    echo ""

    # Start python server if available (useful for other devices)
    if command -v python3 >/dev/null 2>&1; then
        cd /webos && python3 -m http.server 8080 &
        log "HTTP server on http://localhost:8080"
    fi
fi

# ─── Debug mode ─────────────────────────────────────────
if [ "$WEBOS_MODE" = "debug" ]; then
    log "Debug mode — spawning extra shell on tty2"
    (sleep 2; openvt -f -c 2 /bin/sh) &
fi

# ─── Keep init alive ────────────────────────────────────
# Init (PID 1) must never exit. Loop forever.
log "System ready"
while true; do
    sleep 60 &
    wait
done
INITEOF

chmod +x "${INITRAMFS_DIR}/init"
ok "Init script created"

# ─── Create configuration files ──────────────────────────
log "Creating configuration files..."

# /etc/hostname
echo "webos" > "${INITRAMFS_DIR}/etc/hostname"

# /etc/hosts
cat > "${INITRAMFS_DIR}/etc/hosts" << 'HOSTS'
127.0.0.1   localhost webos
::1         localhost webos
HOSTS

# /etc/resolv.conf
cat > "${INITRAMFS_DIR}/etc/resolv.conf" << 'RESOLV'
nameserver 8.8.8.8
nameserver 8.8.4.4
RESOLV

# /etc/passwd
cat > "${INITRAMFS_DIR}/etc/passwd" << 'PASSWD'
root:x:0:0:root:/root:/bin/sh
webos:x:1000:1000:WebOS User:/home/webos:/bin/sh
nobody:x:65534:65534:nobody:/nonexistent:/bin/false
PASSWD

# /etc/group
cat > "${INITRAMFS_DIR}/etc/group" << 'GROUP'
root:x:0:
webos:x:1000:
nogroup:x:65534:
video:x:44:webos
audio:x:29:webos
input:x:104:webos
GROUP

# /etc/shadow (empty passwords — only for live boot!)
cat > "${INITRAMFS_DIR}/etc/shadow" << 'SHADOW'
root::0:0:99999:7:::
webos::0:0:99999:7:::
nobody:*:0:0:99999:7:::
SHADOW
chmod 640 "${INITRAMFS_DIR}/etc/shadow"

# /etc/profile
cat > "${INITRAMFS_DIR}/etc/profile" << 'PROFILE'
export PATH=/bin:/sbin:/usr/bin:/usr/sbin
export HOME=/root
export TERM=xterm-256color
export PS1='\[\033[01;32m\]webos@webos\[\033[00m\]:\[\033[01;34m\]\w\[\033[00m\]\$ '
alias ll='ls -la'
alias la='ls -a'
PROFILE

# /etc/os-release
cat > "${INITRAMFS_DIR}/etc/os-release" << OSRELEASE
NAME="WebOS"
VERSION="0.0.1beta"
ID=webos
ID_LIKE=linux
VERSION_ID="0.0.1beta"
PRETTY_NAME="WebOS 0.0.1beta"
HOME_URL="https://github.com/webos"
BUG_REPORT_URL="https://github.com/webos/issues"
OSRELEASE

# Simple DHCP script for busybox udhcpc
mkdir -p "${INITRAMFS_DIR}/usr/share/udhcpc"
cat > "${INITRAMFS_DIR}/usr/share/udhcpc/default.script" << 'DHCPS'
#!/bin/sh
case "$1" in
    bound|renew)
        [ -n "$ip" ] && [ -n "$mask" ] && ip addr add "$ip/$mask" dev "$interface"
        [ -n "$router" ] && ip route add default via "$router"
        [ -n "$dns" ] && echo "nameserver $dns" > /etc/resolv.conf
        ;;
    deconfig)
        ip addr flush dev "$interface"
        ;;
esac
DHCPS
chmod +x "${INITRAMFS_DIR}/usr/share/udhcpc/default.script"

# /etc/mdev.conf (for busybox mdev)
cat > "${INITRAMFS_DIR}/etc/mdev.conf" << 'MDEV'
snd/*              root:audio  660
input/event[0-9]*   root:input  660
input/mouse[0-9]*   root:input  660
input/mice          root:input  660
MDEV

ok "Configuration files created"

# ─── Build the initramfs archive ─────────────────────────
log "Building initramfs archive..."

mkdir -p "$(dirname "$OUTPUT_PATH")"

(cd "${INITRAMFS_DIR}" && find . | cpio -o -H newc 2>/dev/null | gzip -9) > "$OUTPUT_PATH"

if [ ! -f "$OUTPUT_PATH" ] || [ ! -s "$OUTPUT_PATH" ]; then
    err "Failed to create initramfs archive"
    exit 1
fi

INITRD_SIZE=$(du -h "$OUTPUT_PATH" | cut -f1)
FILE_COUNT=$(find "${INITRAMFS_DIR}" -type f | wc -l)

ok "Initramfs built successfully"
echo ""
echo "═══════════════════════════════════════════════════════════"
echo -e "  ${GREEN}WebOS Initramfs v${VERSION}${NC}"
echo "═══════════════════════════════════════════════════════════"
echo "  Output:    $OUTPUT_PATH"
echo "  Size:      $INITRD_SIZE"
echo "  Files:     $FILE_COUNT"
echo "  Shell:     $([ -f "${INITRAMFS_DIR}/bin/busybox" ] && echo "Busybox" || echo "System binaries")"
echo "  Init:      /init (custom WebOS boot script)"
echo ""
echo "  Boot modes (via kernel cmdline):"
echo "    webos_mode=desktop   Normal kiosk mode (default)"
echo "    webos_mode=install   Launch installer"
echo "    debug                Debug console on tty2"
echo ""
echo "  Usage with kernel:"
echo "    linux /boot/vmlinuz quiet splash boot=live nopersistence"
echo "    initrd ${OUTPUT_PATH}"
echo "═══════════════════════════════════════════════════════════"
