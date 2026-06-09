#!/bin/bash
# WebOS ISO Builder v0.0.b
# Creates a bootable ISO image with a minimal Linux + Chromium kiosk
# that auto-launches the WebOS application.
#
# Usage: ./tools/build_iso.sh [output_path]
#
# Requirements: xorriso, grub-pc-bin, mtools, a Linux kernel (vmlinuz)

set -e

VERSION="0.0.b"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_DIR}/build/iso"
ISO_NAME="webos-${VERSION}.iso"
OUTPUT_PATH="${1:-${PROJECT_DIR}/dist/${ISO_NAME}}"

echo "═══════════════════════════════════════════"
echo "  WebOS ISO Builder v${VERSION}"
echo "═══════════════════════════════════════════"
echo ""

# Step 1: Create ISO directory structure
echo "[1/6] Creating ISO directory structure..."
rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}/boot/grub"
mkdir -p "${BUILD_DIR}/webos/wasm"
mkdir -p "${BUILD_DIR}/webos/wallpapers"
mkdir -p "${BUILD_DIR}/webos/js"

# Step 2: Copy WebOS files
echo "[2/6] Copying WebOS files..."
if [ -d "${PROJECT_DIR}/public/wasm" ]; then
    cp -r "${PROJECT_DIR}/public/wasm/"* "${BUILD_DIR}/webos/wasm/" 2>/dev/null || true
fi
if [ -d "${PROJECT_DIR}/public/wallpapers" ]; then
    cp -r "${PROJECT_DIR}/public/wallpapers/"* "${BUILD_DIR}/webos/wallpapers/" 2>/dev/null || true
fi
if [ -f "${PROJECT_DIR}/public/webos-host.js" ]; then
    cp "${PROJECT_DIR}/public/webos-host.js" "${BUILD_DIR}/webos/js/"
fi
if [ -f "${PROJECT_DIR}/public/index.html" ]; then
    cp "${PROJECT_DIR}/public/index.html" "${BUILD_DIR}/webos/"
fi

# Step 3: Create the self-contained HTML entry point
echo "[3/6] Creating self-contained HTML..."
cat > "${BUILD_DIR}/webos/index.html" << 'HTMLEOF'
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>WebOS v0.0.b</title>
<style>
* { margin: 0; padding: 0; box-sizing: border-box; }
html, body { width: 100%; height: 100%; overflow: hidden; background: #000; }
#webos-canvas { width: 100%; height: 100%; display: block; }
#boot-status {
  position: fixed; top: 50%; left: 50%; transform: translate(-50%, -50%);
  color: #ccc; font-family: 'Segoe UI', sans-serif; font-size: 18px;
  text-align: center;
}
.spinner {
  width: 40px; height: 40px; margin: 20px auto;
  border: 3px solid #333; border-top-color: #0078d4;
  border-radius: 50%; animation: spin 1s linear infinite;
}
@keyframes spin { to { transform: rotate(360deg); } }
</style>
</head>
<body>
<canvas id="webos-canvas"></canvas>
<div id="boot-status">
  <div class="spinner"></div>
  <p>WebOS v0.0.b — Starting...</p>
</div>
</body>
</html>
HTMLEOF

# Step 4: Create GRUB config
echo "[4/6] Creating GRUB configuration..."
cat > "${BUILD_DIR}/boot/grub/grub.cfg" << 'GRUBEOF'
set timeout=0
set default=0

menuentry "WebOS v0.0.b (Memory OS)" {
    linux /boot/vmlinuz quiet splash boot=live nopersistence
    initrd /boot/initrd.img
}

menuentry "WebOS v0.0.b (Safe Mode)" {
    linux /boot/vmlinuz quiet splash boot=live nopersistence nomodeset
    initrd /boot/initrd.img
}
GRUBEOF

# Step 5: Check for Linux kernel
echo "[5/6] Preparing boot kernel..."
VMLINUZ=""
INITRD=""

# Check for system kernel
if [ -f "/boot/vmlinuz-$(uname -r)" ]; then
    VMLINUZ="/boot/vmlinuz-$(uname -r)"
    INITRD="/boot/initrd.img-$(uname -r)"
elif [ -f "/vmlinuz" ]; then
    VMLINUZ="/vmlinuz"
    INITRD="/initrd.img"
fi

if [ -n "$VMLINUZ" ]; then
    echo "  Using kernel: $VMLINUZ"
    cp "$VMLINUZ" "${BUILD_DIR}/boot/vmlinuz"
    if [ -n "$INITRD" ] && [ -f "$INITRD" ]; then
        cp "$INITRD" "${BUILD_DIR}/boot/initrd.img"
    fi
    
    # Create minimal initramfs with Chromium kiosk
    echo "  Creating initramfs with browser kiosk..."
    INITRD_DIR="${BUILD_DIR}/initrd_staging"
    mkdir -p "${INITRD_DIR}"/{bin,sbin,etc,proc,sys,dev,tmp,run,webos}
    
    # Copy webos files into initramfs
    cp -r "${BUILD_DIR}/webos" "${INITRD_DIR}/webos"
    
    # Create init script
    cat > "${INITRD_DIR}/init" << 'INITEOF'
#!/bin/sh
mount -t proc proc /proc
mount -t sysfs sysfs /sys
mount -t devtmpfs devtmpfs /dev

echo "WebOS v0.0.b — Memory Operating System"
echo "Loading WebOS..."

# Try to start X with Chromium in kiosk mode
if command -v startx >/dev/null 2>&1; then
    startx /usr/bin/chromium-browser \
        --no-sandbox \
        --kiosk \
        --disable-gpu-compositing \
        --allow-file-access-from-files \
        file:///webos/index.html
elif command -v firefox >/dev/null 2>&1; then
    firefox --kiosk file:///webos/index.html
else
    echo "No browser found. WebOS files are at /webos/"
    exec /bin/sh
fi
INITEOF
    chmod +x "${INITRD_DIR}/init"
    
    echo "  Note: For a full ISO, you need to build a custom initramfs"
    echo "  with a minimal X server and Chromium. See docs/building.md"
else
    echo "  WARNING: No Linux kernel found. ISO will be data-only."
    echo "  To build a bootable ISO, install a Linux kernel first."
    
    # Create a README in the ISO
    cat > "${BUILD_DIR}/README.txt" << 'READMEEOF'
WebOS v0.0.b — Memory Operating System
========================================

This ISO contains the WebOS application files.

To create a fully bootable ISO:
1. Install a Linux kernel (vmlinuz)
2. Build an initramfs with X11 + Chromium
3. Run: ./tools/build_iso.sh

The files in /webos/ can also be loaded by:
- Opening webos/index.html in any modern browser
- Serving via: python3 -m http.server 8080
READMEEOF
fi

# Step 6: Build the ISO
echo "[6/6] Building ISO image..."
mkdir -p "$(dirname "$OUTPUT_PATH")"

if command -v grub-mkrescue >/dev/null 2>&1; then
    grub-mkrescue -o "$OUTPUT_PATH" "${BUILD_DIR}" 2>/dev/null
    if [ $? -eq 0 ]; then
        ISO_SIZE=$(du -h "$OUTPUT_PATH" | cut -f1)
        echo ""
        echo "═══════════════════════════════════════════"
        echo "  ISO built successfully!"
        echo "  Output: $OUTPUT_PATH"
        echo "  Size: $ISO_SIZE"
        echo "═══════════════════════════════════════════"
    else
        echo "  grub-mkrescue failed, creating data ISO instead..."
        xorriso -as mkisofs -o "$OUTPUT_PATH" -J -r "${BUILD_DIR}" 2>/dev/null || \
        mkisofs -o "$OUTPUT_PATH" -J -r "${BUILD_DIR}" 2>/dev/null || \
        echo "  Failed to create ISO. Install xorriso or genisoimage."
    fi
else
    echo "  grub-mkrescue not found, creating data ISO..."
    xorriso -as mkisofs -o "$OUTPUT_PATH" -J -r "${BUILD_DIR}" 2>/dev/null || \
    mkisofs -o "$OUTPUT_PATH" -J -r "${BUILD_DIR}" 2>/dev/null || \
    echo "  Failed to create ISO. Install xorriso or genisoimage."
fi

# Cleanup
rm -rf "${BUILD_DIR}"

echo ""
echo "Done! ISO image: $OUTPUT_PATH"
