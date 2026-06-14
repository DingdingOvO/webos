#!/bin/bash
# ─────────────────────────────────────────────────────────
# WebOS Installer v0.0.1beta
# Standalone disk installer for WebOS
#
# This script can be run:
#   1. From a booted WebOS live ISO
#   2. Directly from a Linux system with WebOS files
#
# Usage:
#   sudo ./tools/installer.sh                     # Interactive
#   sudo ./tools/installer.sh /dev/sda            # Non-interactive
#   sudo ./tools/installer.sh --list-disks        # List disks only
#   sudo ./tools/installer.sh --dry-run /dev/sda  # Simulate install
# ─────────────────────────────────────────────────────────

set -e

VERSION="0.0.1beta"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

# ─── Color helpers ────────────────────────────────────────
RED='\033[0;31m'
GREEN='\033[0;32m'
CYAN='\033[0;36m'
YELLOW='\033[1;33m'
BOLD='\033[1m'
NC='\033[0m'

log()   { echo -e "${CYAN}[install]${NC} $1"; }
ok()    { echo -e "${GREEN}[OK]${NC} $1"; }
warn()  { echo -e "${YELLOW}[WARN]${NC} $1"; }
err()   { echo -e "${RED}[ERROR]${NC} $1"; }
header(){ echo -e "${BOLD}${CYAN}$1${NC}"; }

# ─── Parse arguments ─────────────────────────────────────
TARGET_DISK=""
LIST_DISKS=false
DRY_RUN=false
AUTO_CONFIRM=false
EFI_SIZE="512"        # MB
HOSTNAME="webos"
USERNAME="webos"

while [ $# -gt 0 ]; do
    case "$1" in
        --list-disks)  LIST_DISKS=true; shift ;;
        --dry-run)     DRY_RUN=true; shift ;;
        --yes|-y)      AUTO_CONFIRM=true; shift ;;
        --hostname)    HOSTNAME="$2"; shift 2 ;;
        --username)    USERNAME="$2"; shift 2 ;;
        --efi-size)    EFI_SIZE="$2"; shift 2 ;;
        -h|--help)     
            echo "WebOS Installer v${VERSION}"
            echo ""
            echo "Usage: sudo $0 [options] [target_disk]"
            echo ""
            echo "Options:"
            echo "  --list-disks       List available disks and exit"
            echo "  --dry-run          Simulate installation without writing"
            echo "  --yes, -y          Auto-confirm disk formatting"
            echo "  --hostname NAME    Set hostname (default: webos)"
            echo "  --username NAME    Set username (default: webos)"
            echo "  --efi-size SIZE    EFI partition size in MB (default: 512)"
            echo "  -h, --help         Show this help"
            exit 0
            ;;
        /dev/*)        TARGET_DISK="$1"; shift ;;
        *)             err "Unknown option: $1"; exit 1 ;;
    esac
done

# ─── Banner ───────────────────────────────────────────────
echo ""
echo "═══════════════════════════════════════════════════════════"
echo "  WebOS Installer v${VERSION}"
echo "═══════════════════════════════════════════════════════════"
echo ""

# ─── Pre-flight checks ───────────────────────────────────

# Check root
if [ "$(id -u)" -ne 0 ]; then
    err "This installer must be run as root."
    err "Run: sudo $0"
    exit 1
fi

# Check required tools
check_tool() {
    if ! command -v "$1" >/dev/null 2>&1; then
        err "Required tool '$1' not found. Please install $2."
        MISSING_TOOLS=true
    fi
}

MISSING_TOOLS=false
check_tool "parted"    "parted (GNU Parted)"
check_tool "mkfs.ext4" "e2fsprogs"
check_tool "mkfs.vfat" "dosfstools"
check_tool "grub-install" "grub2 or grub-pc"

if [ "$MISSING_TOOLS" = true ]; then
    echo ""
    echo "  Debian/Ubuntu: sudo apt install parted e2fsprogs dosfstools grub-pc-bin grub-efi-amd64-bin"
    echo "  Fedora:        sudo dnf install parted e2fsprogs dosfstools grub2-pc grub2-efi-x64"
    echo "  Arch:          sudo pacman -S parted e2fsprogs dosfstools grub"
    exit 1
fi

# ─── List disks mode ─────────────────────────────────────
if [ "$LIST_DISKS" = true ]; then
    header "Available Disks"
    echo "──────────────────────────────────────────────────────"
    if command -v lsblk >/dev/null 2>&1; then
        lsblk -dno NAME,SIZE,MODEL,TYPE,TRAN | grep -E 'disk' | head -30
    else
        for d in /sys/block/sd* /sys/block/nvme* /sys/block/vd* /sys/block/mmcblk*; do
            [ -e "$d" ] || continue
            name=$(basename "$d")
            size_sector=$(cat "$d/size" 2>/dev/null || echo 0)
            size_mb=$((size_sector * 512 / 1024 / 1024))
            model=$(cat "$d/device/model" 2>/dev/null || echo "Unknown")
            echo "/dev/$name  ${size_mb}MB  $model"
        done
    fi
    echo ""
    exit 0
fi

# ─── Select target disk ──────────────────────────────────
if [ -z "$TARGET_DISK" ]; then
    header "Available Disks"
    echo "──────────────────────────────────────────────────────"
    
    DISK_LIST=""
    if command -v lsblk >/dev/null 2>&1; then
        DISK_LIST=$(lsblk -dno NAME,SIZE,MODEL,TYPE | grep -E 'disk')
    else
        for d in /sys/block/sd* /sys/block/nvme* /sys/block/vd*; do
            [ -e "$d" ] || continue
            name=$(basename "$d")
            size_sector=$(cat "$d/size" 2>/dev/null || echo 0)
            size_mb=$((size_sector * 512 / 1024 / 1024))
            model=$(cat "$d/device/model" 2>/dev/null || echo "Unknown")
            echo "$name  ${size_mb}MB  $model"
        done
    fi
    
    echo "$DISK_LIST"
    echo ""
    echo -n "Enter target disk (e.g., /dev/sda): "
    read -r TARGET_DISK
fi

# Validate target disk
if [ -z "$TARGET_DISK" ]; then
    err "No target disk specified."
    exit 1
fi

if [ ! -b "$TARGET_DISK" ]; then
    err "Device $TARGET_DISK not found or not a block device."
    exit 1
fi

# Safety check: don't install to the running disk
ROOT_DISK=$(lsblk -no PKNAME "$(findmnt -n -o SOURCE /)" 2>/dev/null | head -1)
if [ -n "$ROOT_DISK" ] && [ "/dev/$ROOT_DISK" = "$TARGET_DISK" ]; then
    err "Cannot install to the running system disk ($TARGET_DISK)!"
    err "Please use a different disk."
    exit 1
fi

# Get disk info
DISK_SIZE_BYTES=$(blockdev --getsize64 "$TARGET_DISK" 2>/dev/null || echo 0)
DISK_SIZE_GB=$((DISK_SIZE_BYTES / 1024 / 1024 / 1024))

if [ "$DISK_SIZE_GB" -lt 2 ]; then
    err "Target disk too small (${DISK_SIZE_GB}GB). Minimum 2GB required."
    exit 1
fi

# ─── Show install plan ───────────────────────────────────
header "Installation Plan"
echo "──────────────────────────────────────────────────────"
echo "  Target:     $TARGET_DISK (${DISK_SIZE_GB} GB)"
echo "  Hostname:   $HOSTNAME"
echo "  EFI Size:   ${EFI_SIZE} MB"
echo "  Boot Mode:  $([ -d /sys/firmware/efi ] && echo "UEFI" || echo "BIOS")"
echo ""
echo "  Partition Layout:"
echo "    1. EFI System: 1MiB - ${EFI_SIZE}MiB (FAT32, bootable)"
echo "    2. Root:       ${EFI_SIZE}MiB - 100% (ext4)"
echo ""

if [ "$DRY_RUN" = true ]; then
    warn "DRY RUN — no changes will be made."
    echo ""
fi

# ─── Confirmation ─────────────────────────────────────────
if [ "$AUTO_CONFIRM" = false ]; then
    echo -e "  ${RED}${BOLD}WARNING: ALL DATA ON $TARGET_DISK WILL BE DESTROYED!${NC}"
    echo ""
    echo -n "  Type 'YES' to proceed: "
    read -r CONFIRM
    if [ "$CONFIRM" != "YES" ]; then
        log "Installation cancelled."
        exit 0
    fi
fi

echo ""

# ─── Phase 1: Partition the disk ─────────────────────────
header "[1/7] Partitioning $TARGET_DISK..."

if [ "$DRY_RUN" = false ]; then
    # Wipe existing signatures
    wipefs -a "$TARGET_DISK" 2>/dev/null || true
    
    # Erase GPT/MBR
    sgdisk -Z "$TARGET_DISK" 2>/dev/null || true
    
    # Create new GPT partition table
    parted -s "$TARGET_DISK" mklabel gpt
    
    # EFI System Partition
    parted -s "$TARGET_DISK" mkpart primary fat32 1MiB "${EFI_SIZE}MiB"
    parted -s "$TARGET_DISK" set 1 esp on
    parted -s "$TARGET_DISK" set 1 boot on
    
    # Root partition
    parted -s "$TARGET_DISK" mkpart primary ext4 "${EFI_SIZE}MiB" 100%
    
    # Re-read partition table
    partprobe "$TARGET_DISK" 2>/dev/null || true
    sleep 2
fi

ok "Partition table created"

# ─── Determine partition device names ────────────────────
if echo "$TARGET_DISK" | grep -qE "nvme|mmcblk"; then
    EFI_PART="${TARGET_DISK}p1"
    ROOT_PART="${TARGET_DISK}p2"
else
    EFI_PART="${TARGET_DISK}1"
    ROOT_PART="${TARGET_DISK}2"
fi

log "EFI partition:  $EFI_PART"
log "Root partition: $ROOT_PART"

# ─── Phase 2: Format partitions ─────────────────────────
header "[2/7] Formatting partitions..."

if [ "$DRY_RUN" = false ]; then
    mkfs.vfat -F 32 -n WEBOS_EFI "$EFI_PART"
    mkfs.ext4 -q -L webos "$ROOT_PART"
fi

ok "Partitions formatted"

# ─── Phase 3: Mount partitions ───────────────────────────
header "[3/7] Mounting partitions..."

MOUNT_DIR="/mnt/webos"

if [ "$DRY_RUN" = false ]; then
    mkdir -p "$MOUNT_DIR"
    mount "$ROOT_PART" "$MOUNT_DIR"
    mkdir -p "$MOUNT_DIR/boot/efi"
    mount "$EFI_PART" "$MOUNT_DIR/boot/efi"
fi

ok "Partitions mounted at $MOUNT_DIR"

# ─── Phase 4: Create filesystem structure ────────────────
header "[4/7] Creating filesystem..."

if [ "$DRY_RUN" = false ]; then
    # Base directories
    mkdir -p "$MOUNT_DIR"/{bin,sbin,lib,lib64,etc,proc,sys,dev,tmp,run}
    mkdir -p "$MOUNT_DIR"/var/{log,run,tmp,lib}
    mkdir -p "$MOUNT_DIR"/boot/grub
    mkdir -p "$MOUNT_DIR"/boot/efi
    mkdir -p "$MOUNT_DIR"/usr/{bin,sbin,lib,share}
    mkdir -p "$MOUNT_DIR"/webos/{wasm,wallpapers,js,css}
    mkdir -p "$MOUNT_DIR"/home/"$USERNAME"
    mkdir -p "$MOUNT_DIR"/root
    mkdir -p "$MOUNT_DIR"/installer

    # Device nodes (minimal)
    mknod -m 622 "$MOUNT_DIR/dev/console" c 5 1 2>/dev/null || true
    mknod -m 666 "$MOUNT_DIR/dev/null" c 1 3 2>/dev/null || true
    mknod -m 666 "$MOUNT_DIR/dev/zero" c 1 5 2>/dev/null || true
    mknod -m 444 "$MOUNT_DIR/dev/random" c 1 8 2>/dev/null || true
    mknod -m 444 "$MOUNT_DIR/dev/urandom" c 1 9 2>/dev/null || true
fi

ok "Filesystem structure created"

# ─── Phase 5: Copy WebOS files ──────────────────────────
header "[5/7] Installing WebOS files..."

# Locate WebOS source files
WEBOS_SRC=""
for src in \
    "${PROJECT_DIR}/build/iso/webos" \
    "${PROJECT_DIR}/public" \
    "/webos" \
    "/run/webos" \
    "${SCRIPT_DIR}/../build/iso/webos"; do
    if [ -d "$src" ] && [ -f "$src/index.html" -o -f "$src/wasm/kernel.wasm" 2>/dev/null ]; then
        WEBOS_SRC="$src"
        break
    fi
done

# If we have a built ISO, extract from it
if [ -z "$WEBOS_SRC" ] && [ -f "${PROJECT_DIR}/dist/webos-${VERSION}.iso" ]; then
    log "Extracting files from ISO..."
    ISO_MOUNT=$(mktemp -d)
    mount -o ro,loop "${PROJECT_DIR}/dist/webos-${VERSION}.iso" "$ISO_MOUNT" 2>/dev/null
    if [ -d "$ISO_MOUNT/webos" ]; then
        WEBOS_SRC="$ISO_MOUNT/webos"
    fi
fi

if [ -n "$WEBOS_SRC" ]; then
    if [ "$DRY_RUN" = false ]; then
        # Copy all WebOS files
        if [ -f "$WEBOS_SRC/index.html" ]; then
            cp "$WEBOS_SRC/index.html" "$MOUNT_DIR/webos/"
        fi
        
        # Copy WASM modules
        if [ -d "$WEBOS_SRC/wasm" ]; then
            cp -r "$WEBOS_SRC/wasm/"* "$MOUNT_DIR/webos/wasm/" 2>/dev/null || true
        elif [ -d "$WEBOS_SRC/../wasm" ]; then
            cp -r "$WEBOS_SRC/../wasm/"* "$MOUNT_DIR/webos/wasm/" 2>/dev/null || true
        fi
        
        # Copy wallpapers
        if [ -d "$WEBOS_SRC/wallpapers" ]; then
            cp -r "$WEBOS_SRC/wallpapers/"* "$MOUNT_DIR/webos/wallpapers/" 2>/dev/null || true
        elif [ -d "$WEBOS_SRC/../wallpapers" ]; then
            cp -r "$WEBOS_SRC/../wallpapers/"* "$MOUNT_DIR/webos/wallpapers/" 2>/dev/null || true
        fi
        
        # Copy JS
        if [ -d "$WEBOS_SRC/js" ]; then
            cp -r "$WEBOS_SRC/js/"* "$MOUNT_DIR/webos/js/" 2>/dev/null || true
        fi
        
        # Copy CSS
        if [ -d "$WEBOS_SRC/css" ]; then
            cp -r "$WEBOS_SRC/css/"* "$MOUNT_DIR/webos/css/" 2>/dev/null || true
        fi
    fi
    
    ok "WebOS files installed from $WEBOS_SRC"
else
    warn "WebOS source files not found. Creating minimal installation."
    if [ "$DRY_RUN" = false ]; then
        # Create a minimal index.html
        cat > "$MOUNT_DIR/webos/index.html" << 'MINHTML'
<!DOCTYPE html>
<html lang="en">
<head><meta charset="UTF-8"><title>WebOS</title>
<style>*{margin:0;padding:0}body{background:#0a0a1a;color:#fff;display:flex;align-items:center;justify-content:center;height:100vh;font-family:sans-serif}</style>
</head>
<body><div style="text-align:center"><h1>Web<span style="color:#0078d4">OS</span></h1><p>v0.0.1beta</p><p style="color:#888;margin-top:16px">System installed. Place WebOS files in /webos/</p></div></body>
</html>
MINHTML
    fi
fi

# ─── Copy kernel and initramfs ───────────────────────────
if [ "$DRY_RUN" = false ]; then
    # Try to find the kernel
    VMLINUZ_SRC=""
    for kpath in \
        "/boot/vmlinuz-$(uname -r)" \
        "/boot/vmlinuz" \
        "$ISO_MOUNT/boot/vmlinuz" \
        "${PROJECT_DIR}/build/iso/boot/vmlinuz"; do
        if [ -f "$kpath" ]; then
            VMLINUZ_SRC="$kpath"
            break
        fi
    done

    if [ -n "$VMLINUZ_SRC" ]; then
        cp "$VMLINUZ_SRC" "$MOUNT_DIR/boot/vmlinuz"
        ok "Kernel installed from $VMLINUZ_SRC"
    else
        warn "No kernel found. You'll need to install one manually."
    fi

    # Try to find/create initramfs
    INITRD_SRC=""
    for ipath in \
        "/boot/initrd.img-$(uname -r)" \
        "/boot/initrd.img" \
        "$ISO_MOUNT/boot/initrd.img" \
        "${PROJECT_DIR}/build/iso/boot/initrd.img"; do
        if [ -f "$ipath" ]; then
            INITRD_SRC="$ipath"
            break
        fi
    done

    if [ -n "$INITRD_SRC" ]; then
        cp "$INITRD_SRC" "$MOUNT_DIR/boot/initrd.img"
        ok "Initramfs installed"
    else
        warn "No initramfs found. Attempting to generate..."
        if command -v mkinitramfs >/dev/null 2>&1; then
            mkinitramfs -o "$MOUNT_DIR/boot/initrd.img" "$(uname -r)" 2>/dev/null && ok "Initramfs generated"
        elif command -v dracut >/dev/null 2>&1; then
            dracut --force "$MOUNT_DIR/boot/initrd.img" "$(uname -r)" 2>/dev/null && ok "Initramfs generated (dracut)"
        else
            warn "Could not generate initramfs automatically."
        fi
    fi

    # Copy installer
    cp "$0" "$MOUNT_DIR/installer/install.sh" 2>/dev/null || true
    chmod +x "$MOUNT_DIR/installer/install.sh" 2>/dev/null || true
fi

# ─── Phase 6: Configure system ───────────────────────────
header "[6/7] Configuring system..."

if [ "$DRY_RUN" = false ]; then
    ROOT_UUID=$(blkid -s UUID -o value "$ROOT_PART" 2>/dev/null || echo "UNKNOWN")
    EFI_UUID=$(blkid -s UUID -o value "$EFI_PART" 2>/dev/null || echo "UNKNOWN")

    # fstab
    cat > "$MOUNT_DIR/etc/fstab" << FSTAB
# WebOS v${VERSION} — /etc/fstab
# <fs>      <mount>      <type>  <options>          <dump> <pass>
UUID=${ROOT_UUID}  /            ext4   defaults,noatime   0      1
UUID=${EFI_UUID}   /boot/efi    vfat   defaults           0      2
tmpfs              /tmp         tmpfs  defaults,nosuid    0      0
tmpfs              /run         tmpfs  defaults           0      0
proc               /proc        proc   defaults           0      0
sysfs              /sys         sysfs  defaults           0      0
devtmpfs           /dev         devtmpfs defaults         0      0
FSTAB

    # hostname
    echo "$HOSTNAME" > "$MOUNT_DIR/etc/hostname"
    echo "127.0.0.1 localhost $HOSTNAME" > "$MOUNT_DIR/etc/hosts"

    # os-release
    cat > "$MOUNT_DIR/etc/os-release" << OSRELEASE
NAME="WebOS"
VERSION="0.0.1beta"
ID=webos
ID_LIKE=linux
VERSION_ID="0.0.1beta"
PRETTY_NAME="WebOS 0.0.1beta"
HOME_URL="https://github.com/webos"
BUG_REPORT_URL="https://github.com/webos/issues"
OSRELEASE

    # Init script for installed system
    cat > "$MOUNT_DIR/init" << 'SYSEOF'
#!/bin/sh
# WebOS v0.0.1beta — System Init
# Mounts filesystems, starts display, launches WebOS kiosk

mount -t proc proc /proc
mount -t sysfs sysfs /sys
mount -t devtmpfs devtmpfs /dev
mkdir -p /dev/pts /dev/shm /run /tmp
mount -t devpts devpts /dev/pts
mount -t tmpfs tmpfs /tmp
mount -t tmpfs tmpfs /run

hostname webos
echo 0 > /proc/sys/kernel/printk 2>/dev/null

# Load GPU drivers
for mod in i915 amdgpu nouveau fbcon vesafb; do
    modprobe "$mod" 2>/dev/null || true
done

# Networking
ip link set lo up 2>/dev/null || true
for iface in /sys/class/net/e*; do
    [ -e "$iface" ] || continue
    ifname=$(basename "$iface")
    ip link set "$ifname" up 2>/dev/null || true
    udhcpc -i "$ifname" -q -n 2>/dev/null || true
done

# Start X + browser
if command -v startx >/dev/null 2>&1; then
    xset s off 2>/dev/null; xset -dpms 2>/dev/null; xset s noblank 2>/dev/null
    for cmd in chromium-browser chromium google-chrome firefox; do
        if command -v "$cmd" >/dev/null 2>&1; then
            case "$cmd" in
                *chrom*|*chrome*) startx "$cmd" --no-sandbox --kiosk --allow-file-access-from-files file:///webos/index.html & ;;
                *firefox*) startx "$cmd" --kiosk file:///webos/index.html & ;;
            esac
            break
        fi
    done
elif command -v python3 >/dev/null 2>&1; then
    cd /webos && python3 -m http.server 8080 &
fi

while true; do sleep 60; done
SYSEOF
    chmod +x "$MOUNT_DIR/init"
fi

ok "System configured"

# ─── Phase 7: Install GRUB bootloader ────────────────────
header "[7/7] Installing GRUB bootloader..."

if [ "$DRY_RUN" = false ]; then
    BOOT_MODE="bios"
    if [ -d /sys/firmware/efi ]; then
        BOOT_MODE="efi"
    fi

    GRUB_INSTALL_OK=false

    if [ "$BOOT_MODE" = "efi" ]; then
        log "Installing GRUB for UEFI..."
        # Try UEFI install
        if command -v grub-install >/dev/null 2>&1; then
            grub-install --target=x86_64-efi \
                --efi-directory="$MOUNT_DIR/boot/efi" \
                --bootloader-id=WebOS \
                --removable \
                --no-nvram \
                --boot-directory="$MOUNT_DIR/boot" \
                2>/dev/null && GRUB_INSTALL_OK=true
        fi

        if [ "$GRUB_INSTALL_OK" = false ]; then
            warn "UEFI GRUB install failed. Trying alternative method..."
            # Try with different options
            grub-install --target=x86_64-efi \
                --efi-directory="$MOUNT_DIR/boot/efi" \
                --bootloader-id=WebOS \
                --removable \
                --boot-directory="$MOUNT_DIR/boot" \
                2>/dev/null && GRUB_INSTALL_OK=true
        fi
    fi

    if [ "$GRUB_INSTALL_OK" = false ]; then
        log "Installing GRUB for BIOS..."
        grub-install --target=i386-pc \
            --boot-directory="$MOUNT_DIR/boot" \
            "$TARGET_DISK" \
            2>/dev/null && GRUB_INSTALL_OK=true
    fi

    if [ "$GRUB_INSTALL_OK" = true ]; then
        ok "GRUB bootloader installed"
    else
        warn "GRUB install failed. You may need to install GRUB manually:"
        warn "  grub-install --target=x86_64-efi --efi-directory=$MOUNT_DIR/boot/efi --bootloader-id=WebOS"
        warn "  grub-install --target=i386-pc --boot-directory=$MOUNT_DIR/boot $TARGET_DISK"
    fi

    # Create GRUB config
    mkdir -p "$MOUNT_DIR/boot/grub"
    cat > "$MOUNT_DIR/boot/grub/grub.cfg" << GRUBCFG
# WebOS v${VERSION} — GRUB Configuration
set default=0
set timeout=3
set gfxpayload=keep

menuentry "WebOS v${VERSION}" {
    linux /boot/vmlinuz quiet splash root=UUID=${ROOT_UUID} boot=live nopersistence
    initrd /boot/initrd.img
}

menuentry "WebOS v${VERSION} (Safe Mode)" {
    linux /boot/vmlinuz quiet boot=live nopersistence vga=788 nomodeset
    initrd /boot/initrd.img
}

menuentry "WebOS v${VERSION} (Debug)" {
    linux /boot/vmlinuz boot=live nopersistence debug
    initrd /boot/initrd.img
}
GRUBCFG

    ok "GRUB configuration written"
fi

# ─── Finalize ────────────────────────────────────────────
if [ "$DRY_RUN" = false ]; then
    sync
    
    # Unmount
    log "Unmounting partitions..."
    umount "$MOUNT_DIR/boot/efi" 2>/dev/null || true
    umount "$MOUNT_DIR" 2>/dev/null || true
    
    # Clean up ISO mount
    if [ -n "$ISO_MOUNT" ] && mountpoint -q "$ISO_MOUNT" 2>/dev/null; then
        umount "$ISO_MOUNT"
        rmdir "$ISO_MOUNT"
    fi
fi

echo ""
echo "═══════════════════════════════════════════════════════════"
if [ "$DRY_RUN" = true ]; then
    echo -e "  ${YELLOW}DRY RUN COMPLETE — No changes were made${NC}"
else
    echo -e "  ${GREEN}${BOLD}WebOS v${VERSION} installed successfully!${NC}"
fi
echo "═══════════════════════════════════════════════════════════"
echo ""
echo "  Target:     $TARGET_DISK (${DISK_SIZE_GB} GB)"
echo "  EFI:        $EFI_PART (${EFI_SIZE} MB, FAT32)"
echo "  Root:       $ROOT_PART (ext4)"
echo "  Hostname:   $HOSTNAME"
echo "  Boot Mode:  $([ -d /sys/firmware/efi ] && echo "UEFI" || echo "BIOS")"
echo ""
if [ "$DRY_RUN" = false ]; then
    echo "  Next steps:"
    echo "    1. Remove the installation media"
    echo "    2. Reboot the system"
    echo "    3. WebOS will auto-boot from disk"
    echo ""
    echo "  If the system doesn't boot:"
    echo "    - Check BIOS/UEFI boot order"
    echo "    - Reinstall GRUB: grub-install --target=x86_64-efi --efi-directory=/boot/efi"
    echo ""
fi
echo "═══════════════════════════════════════════════════════════"
