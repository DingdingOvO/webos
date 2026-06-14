#!/bin/bash
# WebOS ISO Builder v0.0.1beta
# Creates a bootable ISO image with GRUB + Linux + Chromium kiosk
# Includes an installer for disk installation
#
# Usage:
#   ./tools/build_iso.sh [output_path] [--quick]
#   ./tools/build_iso.sh                     # Full build
#   ./tools/build_iso.sh dist/webos.iso      # Custom output path
#   ./tools/build_iso.sh --quick             # Quick (skip kernel/initramfs checks)
#
# Requirements: xorriso, grub-pc-bin, grub-efi-amd64-bin, mtools,
#               cpio, gzip, a Linux kernel (vmlinuz)

set -e

VERSION="0.0.1beta"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_DIR}/build/iso"
ISO_NAME="webos-${VERSION}.iso"
OUTPUT_PATH=""
QUICK_MODE=false

# ─── Parse arguments ──────────────────────────────────────
for arg in "$@"; do
    case "$arg" in
        --quick)  QUICK_MODE=true ;;
        *)        OUTPUT_PATH="$arg" ;;
    esac
done
OUTPUT_PATH="${OUTPUT_PATH:-${PROJECT_DIR}/dist/${ISO_NAME}}"

# ─── Color helpers ────────────────────────────────────────
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

log_step()   { echo -e "${CYAN}[$1]${NC} $2"; }
log_ok()     { echo -e "${GREEN}[OK]${NC} $1"; }
log_warn()   { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_err()    { echo -e "${RED}[ERROR]${NC} $1"; }

# ─── Banner ───────────────────────────────────────────────
echo ""
echo "═══════════════════════════════════════════════════════════"
echo "  WebOS ISO Builder v${VERSION}"
echo "  Mode: $([ "$QUICK_MODE" = true ] && echo "Quick" || echo "Full")"
echo "═══════════════════════════════════════════════════════════"
echo ""

# ─── Dependency check ─────────────────────────────────────
check_dependency() {
    if ! command -v "$1" >/dev/null 2>&1; then
        log_err "Required tool '$1' not found. Please install $2."
        DEPS_MISSING=true
    fi
}

DEPS_MISSING=false
if [ "$QUICK_MODE" = false ]; then
    check_dependency "xorriso"     "xorriso (libisoburn)"
    check_dependency "mformat"     "mtools"
    check_dependency "cpio"        "cpio"
fi

if [ "$DEPS_MISSING" = true ]; then
    log_err "Missing dependencies. Install them and re-run."
    echo ""
    echo "  Debian/Ubuntu: sudo apt install xorriso grub-pc-bin grub-efi-amd64-bin mtools cpio"
    echo "  Fedora:        sudo dnf install xorriso grub2-pc grub2-efi mtools cpio"
    echo "  Arch:          sudo pacman -S libburn grub mtools cpio"
    exit 1
fi

# ─── Step 1: Create ISO directory structure ────────────────
log_step "1/7" "Creating ISO directory structure..."
rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}/boot/grub"
mkdir -p "${BUILD_DIR}/boot/grub/x86_64-efi"
mkdir -p "${BUILD_DIR}/webos/wasm"
mkdir -p "${BUILD_DIR}/webos/wallpapers"
mkdir -p "${BUILD_DIR}/webos/js"
mkdir -p "${BUILD_DIR}/webos/css"
mkdir -p "${BUILD_DIR}/installer"
mkdir -p "${BUILD_DIR}/EFI/BOOT"

log_ok "ISO directory structure created"

# ─── Step 2: Copy all WebOS files ─────────────────────────
log_step "2/7" "Copying WebOS files..."

# Copy WASM modules
if [ -d "${PROJECT_DIR}/public/wasm" ]; then
    cp -r "${PROJECT_DIR}/public/wasm/"* "${BUILD_DIR}/webos/wasm/" 2>/dev/null || true
    WASM_COUNT=$(find "${BUILD_DIR}/webos/wasm" -type f 2>/dev/null | wc -l)
    log_ok "Copied ${WASM_COUNT} WASM module(s)"
else
    log_warn "No public/wasm/ directory found"
fi

# Copy wallpapers
if [ -d "${PROJECT_DIR}/public/wallpapers" ]; then
    cp -r "${PROJECT_DIR}/public/wallpapers/"* "${BUILD_DIR}/webos/wallpapers/" 2>/dev/null || true
    WP_COUNT=$(find "${BUILD_DIR}/webos/wallpapers" -type f 2>/dev/null | wc -l)
    log_ok "Copied ${WP_COUNT} wallpaper(s)"
else
    log_warn "No public/wallpapers/ directory found"
fi

# Copy built host JS if it exists
if [ -f "${PROJECT_DIR}/host/dist/webos-host.js" ]; then
    cp "${PROJECT_DIR}/host/dist/webos-host.js" "${BUILD_DIR}/webos/js/"
    log_ok "Copied webos-host.js"
elif [ -f "${PROJECT_DIR}/public/webos-host.js" ]; then
    cp "${PROJECT_DIR}/public/webos-host.js" "${BUILD_DIR}/webos/js/"
    log_ok "Copied webos-host.js (from public/)"
else
    log_warn "No webos-host.js found (host must be built first)"
fi

# Copy any additional public assets
if [ -f "${PROJECT_DIR}/public/favicon.ico" ]; then
    cp "${PROJECT_DIR}/public/favicon.ico" "${BUILD_DIR}/webos/" 2>/dev/null || true
fi
if [ -f "${PROJECT_DIR}/public/favicon.svg" ]; then
    cp "${PROJECT_DIR}/public/favicon.svg" "${BUILD_DIR}/webos/" 2>/dev/null || true
fi

log_ok "WebOS file copy complete"

# ─── Step 3: Create self-contained HTML entry point ───────
log_step "3/7" "Creating self-contained WebOS HTML/JS application..."

cat > "${BUILD_DIR}/webos/index.html" << 'HTMLEOF'
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>WebOS v0.0.1beta</title>
<style>
/* ═══ Reset & Base ═══ */
*{margin:0;padding:0;box-sizing:border-box}
html,body{width:100%;height:100%;overflow:hidden;background:#000;font-family:'Segoe UI',system-ui,-apple-system,sans-serif;user-select:none}

/* ═══ Boot Screen ═══ */
#boot-screen{position:fixed;inset:0;background:#0a0a1a;display:flex;flex-direction:column;align-items:center;justify-content:center;z-index:9999;transition:opacity .6s}
#boot-screen.fade-out{opacity:0;pointer-events:none}
.boot-logo{font-size:48px;font-weight:700;color:#fff;margin-bottom:8px;letter-spacing:2px}
.boot-logo span{color:#0078d4}
.boot-version{color:#666;font-size:14px;margin-bottom:32px}
.boot-spinner{width:36px;height:36px;border:3px solid #222;border-top-color:#0078d4;border-radius:50%;animation:spin .8s linear infinite}
.boot-msg{color:#555;font-size:13px;margin-top:16px;font-family:'Cascadia Code','Consolas',monospace;min-height:20px}
@keyframes spin{to{transform:rotate(360deg)}}

/* ═══ Desktop ═══ */
#desktop{position:fixed;inset:0;display:none;flex-direction:column}
#desktop.visible{display:flex}

/* Wallpaper */
.wallpaper{position:absolute;inset:0;background:linear-gradient(135deg,#0a1628 0%,#0d2137 40%,#0f3062 70%,#0a1a3a 100%);z-index:0}
.wallpaper-img{position:absolute;inset:0;background-size:cover;background-position:center;opacity:.3}

/* Desktop Icons */
.desktop-icons{position:absolute;top:16px;left:16px;display:flex;flex-direction:column;gap:8px;z-index:1}
.desktop-icon{width:80px;display:flex;flex-direction:column;align-items:center;gap:4px;padding:8px 4px;border-radius:8px;cursor:pointer;transition:background .15s}
.desktop-icon:hover{background:rgba(255,255,255,.1)}
.desktop-icon:active{background:rgba(255,255,255,.15)}
.desktop-icon .icon{font-size:32px;line-height:1}
.desktop-icon .label{color:#fff;font-size:11px;text-align:center;text-shadow:0 1px 3px rgba(0,0,0,.8);line-height:1.2;max-width:76px;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}

/* Window Manager */
.window{position:absolute;min-width:320px;min-height:200px;background:#1e1e2e;border-radius:10px;box-shadow:0 8px 32px rgba(0,0,0,.5);display:flex;flex-direction:column;overflow:hidden;z-index:10}
.window.active{box-shadow:0 12px 48px rgba(0,0,0,.7);z-index:11}
.window-titlebar{height:36px;background:#2d2d44;display:flex;align-items:center;padding:0 12px;cursor:grab;flex-shrink:0}
.window-titlebar:active{cursor:grabbing}
.window-title{flex:1;color:#ddd;font-size:13px;font-weight:500}
.window-controls{display:flex;gap:8px}
.window-btn{width:14px;height:14px;border-radius:50%;border:none;cursor:pointer;font-size:0}
.window-btn.close{background:#ff5f57}
.window-btn.minimize{background:#ffbd2e}
.window-btn.maximize{background:#28c840}
.window-btn:hover{filter:brightness(1.2)}
.window-body{flex:1;overflow:auto;background:#181825;position:relative}

/* Taskbar */
.taskbar{height:48px;background:rgba(20,20,35,.92);backdrop-filter:blur(20px);display:flex;align-items:center;padding:0 8px;z-index:100;flex-shrink:0;border-top:1px solid rgba(255,255,255,.06)}
.start-btn{width:44px;height:36px;display:flex;align-items:center;justify-content:center;border-radius:6px;cursor:pointer;color:#fff;font-size:20px;transition:background .15s}
.start-btn:hover{background:rgba(255,255,255,.1)}
.taskbar-apps{flex:1;display:flex;align-items:center;padding:0 8px;gap:4px}
.taskbar-app{height:36px;padding:0 14px;display:flex;align-items:center;gap:6px;border-radius:6px;color:#ccc;font-size:13px;cursor:pointer;transition:background .15s}
.taskbar-app:hover{background:rgba(255,255,255,.08)}
.taskbar-app.active{background:rgba(0,120,212,.3);color:#fff}
.taskbar-app .app-icon{font-size:16px}
.system-tray{display:flex;align-items:center;gap:12px;color:#aaa;font-size:13px;padding:0 8px}
.tray-icon{cursor:pointer;transition:color .15s}
.tray-icon:hover{color:#fff}
.clock{color:#fff;font-size:13px;font-weight:500;min-width:70px;text-align:right}

/* Start Menu */
.start-menu{position:absolute;bottom:52px;left:4px;width:340px;max-height:480px;background:rgba(30,30,50,.96);backdrop-filter:blur(24px);border-radius:12px;border:1px solid rgba(255,255,255,.08);z-index:200;display:none;flex-direction:column;overflow:hidden;box-shadow:0 16px 48px rgba(0,0,0,.5)}
.start-menu.visible{display:flex}
.start-search{padding:16px 16px 12px;border-bottom:1px solid rgba(255,255,255,.06)}
.start-search input{width:100%;height:36px;background:rgba(255,255,255,.08);border:1px solid rgba(255,255,255,.1);border-radius:18px;padding:0 16px;color:#fff;font-size:14px;outline:none}
.start-search input::placeholder{color:#666}
.start-search input:focus{border-color:#0078d4}
.start-section{padding:12px 16px 8px;color:#888;font-size:12px;font-weight:600;text-transform:uppercase;letter-spacing:.5px}
.start-grid{display:grid;grid-template-columns:repeat(3,1fr);gap:4px;padding:4px 12px 12px}
.start-app{display:flex;flex-direction:column;align-items:center;gap:4px;padding:12px 8px;border-radius:8px;cursor:pointer;transition:background .15s}
.start-app:hover{background:rgba(255,255,255,.08)}
.start-app .icon{font-size:24px}
.start-app .name{color:#ddd;font-size:12px;text-align:center}
.start-footer{margin-top:auto;padding:12px 16px;border-top:1px solid rgba(255,255,255,.06);display:flex;justify-content:space-between;align-items:center}
.power-btn{color:#ff6b6b;cursor:pointer;font-size:14px;display:flex;align-items:center;gap:6px;padding:6px 10px;border-radius:6px;transition:background .15s}
.power-btn:hover{background:rgba(255,107,107,.1)}
.settings-btn{color:#aaa;cursor:pointer;font-size:14px;display:flex;align-items:center;gap:6px;padding:6px 10px;border-radius:6px;transition:background .15s}
.settings-btn:hover{background:rgba(255,255,255,.08)}

/* Context Menu */
.context-menu{position:absolute;min-width:200px;background:rgba(30,30,50,.96);backdrop-filter:blur(20px);border-radius:8px;border:1px solid rgba(255,255,255,.08);z-index:300;padding:4px;box-shadow:0 8px 24px rgba(0,0,0,.4);display:none}
.context-menu.visible{display:block}
.ctx-item{padding:8px 16px;color:#ddd;font-size:13px;border-radius:4px;cursor:pointer;display:flex;align-items:center;gap:10px}
.ctx-item:hover{background:rgba(255,255,255,.08)}
.ctx-sep{height:1px;background:rgba(255,255,255,.06);margin:4px 8px}

/* Notification */
.notification{position:absolute;top:16px;right:16px;background:rgba(30,30,50,.95);backdrop-filter:blur(16px);border-radius:10px;border:1px solid rgba(255,255,255,.08);padding:14px 20px;color:#fff;font-size:13px;z-index:400;box-shadow:0 8px 24px rgba(0,0,0,.4);transform:translateX(120%);transition:transform .3s}
.notification.show{transform:translateX(0)}

/* Installer Screen (shown from GRUB installer entry) */
#installer-screen{position:fixed;inset:0;background:#0a0a1a;display:none;flex-direction:column;align-items:center;justify-content:center;z-index:9999;color:#fff}
#installer-screen.visible{display:flex}
.installer-box{width:600px;max-width:90vw;background:rgba(30,30,50,.95);border-radius:16px;border:1px solid rgba(255,255,255,.08);padding:32px;box-shadow:0 16px 48px rgba(0,0,0,.5)}
.installer-box h2{font-size:22px;margin-bottom:8px}
.installer-box .sub{color:#888;font-size:14px;margin-bottom:24px}
.installer-step{margin-bottom:20px}
.installer-step label{display:block;color:#aaa;font-size:13px;margin-bottom:6px}
.installer-step select,.installer-step input{width:100%;height:40px;background:rgba(255,255,255,.06);border:1px solid rgba(255,255,255,.12);border-radius:8px;padding:0 12px;color:#fff;font-size:14px;outline:none}
.installer-step select:focus,.installer-step input:focus{border-color:#0078d4}
.installer-btn{width:100%;height:44px;background:#0078d4;border:none;border-radius:8px;color:#fff;font-size:15px;font-weight:600;cursor:pointer;transition:background .15s;margin-top:8px}
.installer-btn:hover{background:#106ebe}
.installer-progress{width:100%;height:6px;background:rgba(255,255,255,.08);border-radius:3px;margin-top:16px;overflow:hidden;display:none}
.installer-progress.visible{display:block}
.installer-progress-bar{height:100%;background:#0078d4;border-radius:3px;transition:width .3s;width:0}
</style>
</head>
<body>

<!-- Boot Screen -->
<div id="boot-screen">
  <div class="boot-logo">Web<span>OS</span></div>
  <div class="boot-version">v0.0.1beta</div>
  <div class="boot-spinner"></div>
  <div class="boot-msg" id="boot-msg">Initializing system...</div>
</div>

<!-- Desktop -->
<div id="desktop">
  <div class="wallpaper">
    <div class="wallpaper-img" id="wallpaper-img"></div>
  </div>

  <!-- Desktop Icons -->
  <div class="desktop-icons" id="desktop-icons"></div>

  <!-- Windows Container -->
  <div id="windows-container"></div>

  <!-- Start Menu -->
  <div class="start-menu" id="start-menu">
    <div class="start-search">
      <input type="text" placeholder="Search apps..." id="start-search-input">
    </div>
    <div class="start-section">Pinned</div>
    <div class="start-grid" id="start-grid"></div>
    <div class="start-footer">
      <div class="settings-btn" onclick="WebOS.openApp('settings')">&#9881; Settings</div>
      <div class="power-btn" onclick="WebOS.shutdown()">&#9211; Power</div>
    </div>
  </div>

  <!-- Context Menu -->
  <div class="context-menu" id="context-menu">
    <div class="ctx-item" onclick="WebOS.openApp('terminal')">&#128187; Open Terminal</div>
    <div class="ctx-item" onclick="WebOS.openApp('settings')">&#9881; Settings</div>
    <div class="ctx-sep"></div>
    <div class="ctx-item" onclick="WebOS.refresh()">&#128260; Refresh</div>
    <div class="ctx-item" onclick="WebOS.showAbout()">&#8505; About WebOS</div>
  </div>

  <!-- Notification -->
  <div class="notification" id="notification"></div>

  <!-- Taskbar -->
  <div class="taskbar">
    <div class="start-btn" onclick="WebOS.toggleStartMenu()" title="Start">&#8862;</div>
    <div class="taskbar-apps" id="taskbar-apps"></div>
    <div class="system-tray">
      <span class="tray-icon" title="Network">&#128246;</span>
      <span class="tray-icon" title="Volume">&#128266;</span>
      <span class="tray-icon" title="Battery">&#128267;</span>
      <span class="clock" id="clock"></span>
    </div>
  </div>
</div>

<!-- Installer Screen -->
<div id="installer-screen">
  <div class="installer-box">
    <h2>WebOS Installer</h2>
    <div class="sub">v0.0.1beta &mdash; Install WebOS to disk</div>
    <div class="installer-step">
      <label>Select target disk:</label>
      <select id="installer-disk">
        <option>/dev/sda</option>
        <option>/dev/nvme0n1</option>
      </select>
    </div>
    <div class="installer-step">
      <label>Hostname:</label>
      <input type="text" id="installer-hostname" value="webos">
    </div>
    <button class="installer-btn" onclick="WebOS.runInstaller()">Install WebOS</button>
    <div class="installer-progress" id="installer-progress">
      <div class="installer-progress-bar" id="installer-progress-bar"></div>
    </div>
    <div id="installer-status" style="color:#888;font-size:13px;margin-top:12px;text-align:center"></div>
  </div>
</div>

<script>
// ═══════════════════════════════════════════════════════════
// WebOS v0.0.1beta — Complete Self-Contained Runtime
// ═══════════════════════════════════════════════════════════

const WebOS = (() => {
  // ── State ──────────────────────────────────────────────
  const state = {
    windows: [],
    nextWinId: 1,
    activeWinId: null,
    zIndex: 10,
    startMenuOpen: false,
    contextMenuOpen: false,
    dragState: null,
    booted: false,
  };

  const apps = [
    { id: 'calculator', name: 'Calculator', icon: '\u{1F522}', pinned: true, category: 'Utilities' },
    { id: 'paint',      name: 'Paint',      icon: '\u{1F3A8}', pinned: true, category: 'Graphics' },
    { id: 'browser',    name: 'Browser',    icon: '\u{1F310}', pinned: true, category: 'Internet' },
    { id: 'appstore',   name: 'App Store',  icon: '\u{1F3EA}', pinned: true, category: 'System' },
    { id: 'terminal',   name: 'Terminal',   icon: '\u{1F4BB}', pinned: true, category: 'System' },
    { id: 'settings',   name: 'Settings',   icon: '\u{2699}',  pinned: true, category: 'System' },
    { id: 'files',      name: 'Files',      icon: '\u{1F4C1}', pinned: true, category: 'System' },
    { id: 'notepad',    name: 'Notepad',    icon: '\u{1F4DD}', pinned: false, category: 'Utilities' },
    { id: 'music',      name: 'Music',      icon: '\u{1F3B5}', pinned: false, category: 'Media' },
  ];

  // ── Boot sequence ──────────────────────────────────────
  async function boot() {
    const msg = document.getElementById('boot-msg');
    const steps = [
      'Loading kernel...',
      'Initializing memory manager...',
      'Starting GPU driver...',
      'Loading input driver...',
      'Starting window manager...',
      'Loading filesystem service...',
      'Initializing network service...',
      'Loading desktop environment...',
      'Starting applications...',
      'Boot complete!',
    ];
    for (let i = 0; i < steps.length; i++) {
      if (msg) msg.textContent = steps[i];
      await sleep(200 + Math.random() * 300);
    }

    // Check if we should show the installer
    const params = new URLSearchParams(window.location.search);
    if (params.get('mode') === 'install') {
      document.getElementById('boot-screen').classList.add('fade-out');
      setTimeout(() => {
        document.getElementById('boot-screen').style.display = 'none';
        document.getElementById('installer-screen').classList.add('visible');
      }, 600);
      return;
    }

    // Try loading WASM kernel if available
    try {
      await loadWASMRuntime();
    } catch(e) {
      console.log('[WebOS] WASM runtime not available, using native JS runtime');
    }

    // Show desktop
    const bootScreen = document.getElementById('boot-screen');
    bootScreen.classList.add('fade-out');
    setTimeout(() => {
      bootScreen.style.display = 'none';
      document.getElementById('desktop').classList.add('visible');
      state.booted = true;
    }, 600);

    initDesktop();
  }

  async function loadWASMRuntime() {
    // Try to load the WASM kernel if wasm files exist
    try {
      const resp = await fetch('/wasm/kernel.wasm');
      if (!resp.ok) throw new Error('No kernel.wasm');
      const bytes = await resp.arrayBuffer();
      const module = await WebAssembly.compile(bytes);
      console.log('[WebOS] WASM kernel loaded');
      // In a full implementation, we'd instantiate with syscall imports
    } catch(e) {
      throw e;
    }
  }

  // ── Desktop initialization ─────────────────────────────
  function initDesktop() {
    renderDesktopIcons();
    renderStartMenu();
    startClock();
    setupEventListeners();
    loadWallpaper();
    notify('Welcome to WebOS v0.0.1beta');
  }

  function renderDesktopIcons() {
    const container = document.getElementById('desktop-icons');
    if (!container) return;
    container.innerHTML = '';
    const desktopApps = apps.filter(a => a.pinned);
    desktopApps.forEach(app => {
      const el = document.createElement('div');
      el.className = 'desktop-icon';
      el.innerHTML = `<div class="icon">${app.icon}</div><div class="label">${app.name}</div>`;
      el.addEventListener('dblclick', () => openApp(app.id));
      container.appendChild(el);
    });
  }

  function renderStartMenu() {
    const grid = document.getElementById('start-grid');
    if (!grid) return;
    grid.innerHTML = '';
    apps.forEach(app => {
      const el = document.createElement('div');
      el.className = 'start-app';
      el.innerHTML = `<div class="icon">${app.icon}</div><div class="name">${app.name}</div>`;
      el.addEventListener('click', () => { openApp(app.id); toggleStartMenu(false); });
      grid.appendChild(el);
    });
  }

  function startClock() {
    function update() {
      const el = document.getElementById('clock');
      if (el) {
        const now = new Date();
        el.textContent = now.toLocaleTimeString([], {hour:'2-digit',minute:'2-digit'});
      }
    }
    update();
    setInterval(update, 1000);
  }

  function loadWallpaper() {
    const img = document.getElementById('wallpaper-img');
    if (img) {
      img.style.backgroundImage = "url('wallpapers/catgirl-static.png')";
    }
  }

  // ── Event listeners ────────────────────────────────────
  function setupEventListeners() {
    // Right-click context menu
    document.addEventListener('contextmenu', (e) => {
      e.preventDefault();
      const menu = document.getElementById('context-menu');
      menu.style.left = e.clientX + 'px';
      menu.style.top = e.clientY + 'px';
      menu.classList.add('visible');
      state.contextMenuOpen = true;
    });

    // Close menus on left click
    document.addEventListener('click', (e) => {
      if (state.contextMenuOpen) {
        document.getElementById('context-menu').classList.remove('visible');
        state.contextMenuOpen = false;
      }
      if (state.startMenuOpen && !e.target.closest('.start-menu') && !e.target.closest('.start-btn')) {
        toggleStartMenu(false);
      }
    });

    // Window dragging
    document.addEventListener('mousemove', (e) => {
      if (state.dragState) {
        const win = state.windows.find(w => w.id === state.dragState.winId);
        if (win) {
          const dx = e.clientX - state.dragState.startX;
          const dy = e.clientY - state.dragState.startY;
          win.x = state.dragState.origX + dx;
          win.y = state.dragState.origY + dy;
          win.element.style.left = win.x + 'px';
          win.element.style.top = win.y + 'px';
        }
      }
    });

    document.addEventListener('mouseup', () => {
      state.dragState = null;
    });

    // Search filter in start menu
    const searchInput = document.getElementById('start-search-input');
    if (searchInput) {
      searchInput.addEventListener('input', (e) => {
        const q = e.target.value.toLowerCase();
        const items = document.querySelectorAll('.start-app');
        items.forEach(item => {
          const name = item.querySelector('.name').textContent.toLowerCase();
          item.style.display = name.includes(q) ? '' : 'none';
        });
      });
    }

    // Keyboard shortcuts
    document.addEventListener('keydown', (e) => {
      if (e.key === 'Escape') {
        toggleStartMenu(false);
        document.getElementById('context-menu').classList.remove('visible');
      }
    });
  }

  // ── Window management ──────────────────────────────────
  function createWindow(title, content, opts = {}) {
    const id = state.nextWinId++;
    const w = opts.width || 640;
    const h = opts.height || 440;
    const x = 120 + (state.windows.length * 30) % 200;
    const y = 60 + (state.windows.length * 30) % 150;

    const el = document.createElement('div');
    el.className = 'window';
    el.style.cssText = `left:${x}px;top:${y}px;width:${w}px;height:${h}px;z-index:${++state.zIndex}`;
    el.innerHTML = `
      <div class="window-titlebar" data-win-id="${id}">
        <span class="window-title">${title}</span>
        <div class="window-controls">
          <button class="window-btn minimize" title="Minimize"></button>
          <button class="window-btn maximize" title="Maximize"></button>
          <button class="window-btn close" title="Close"></button>
        </div>
      </div>
      <div class="window-body">${content}</div>
    `;

    document.getElementById('windows-container').appendChild(el);

    const win = { id, title, element: el, x, y, width: w, height: h, minimized: false };
    state.windows.push(win);
    focusWindow(id);
    updateTaskbar();

    // Title bar drag
    const titlebar = el.querySelector('.window-titlebar');
    titlebar.addEventListener('mousedown', (e) => {
      if (e.target.classList.contains('window-btn')) return;
      state.dragState = { winId: id, startX: e.clientX, startY: e.clientY, origX: win.x, origY: win.y };
      focusWindow(id);
    });

    // Click to focus
    el.addEventListener('mousedown', () => focusWindow(id));

    // Window buttons
    el.querySelector('.window-btn.close').addEventListener('click', (e) => { e.stopPropagation(); closeWindow(id); });
    el.querySelector('.window-btn.minimize').addEventListener('click', (e) => { e.stopPropagation(); minimizeWindow(id); });
    el.querySelector('.window-btn.maximize').addEventListener('click', (e) => { e.stopPropagation(); maximizeWindow(id); });

    return id;
  }

  function focusWindow(id) {
    state.windows.forEach(w => w.element.classList.remove('active'));
    const win = state.windows.find(w => w.id === id);
    if (win) {
      win.element.classList.add('active');
      win.element.style.zIndex = ++state.zIndex;
      state.activeWinId = id;
    }
    updateTaskbar();
  }

  function closeWindow(id) {
    const idx = state.windows.findIndex(w => w.id === id);
    if (idx >= 0) {
      state.windows[idx].element.remove();
      state.windows.splice(idx, 1);
      if (state.activeWinId === id) {
        state.activeWinId = state.windows.length > 0 ? state.windows[state.windows.length - 1].id : null;
      }
    }
    updateTaskbar();
  }

  function minimizeWindow(id) {
    const win = state.windows.find(w => w.id === id);
    if (win) {
      win.minimized = !win.minimized;
      win.element.style.display = win.minimized ? 'none' : 'flex';
    }
    updateTaskbar();
  }

  function maximizeWindow(id) {
    const win = state.windows.find(w => w.id === id);
    if (win) {
      if (win.maximized) {
        win.element.style.cssText = `left:${win.x}px;top:${win.y}px;width:${win.width}px;height:${win.height}px;z-index:${win.element.style.zIndex}`;
        win.maximized = false;
      } else {
        win.element.style.cssText = `left:0;top:0;width:100%;height:calc(100% - 48px);z-index:${++state.zIndex}`;
        win.maximized = true;
      }
    }
  }

  function updateTaskbar() {
    const container = document.getElementById('taskbar-apps');
    if (!container) return;
    container.innerHTML = '';
    state.windows.forEach(win => {
      const el = document.createElement('div');
      el.className = 'taskbar-app' + (win.id === state.activeWinId ? ' active' : '');
      const app = apps.find(a => a.name === win.title);
      el.innerHTML = `<span class="app-icon">${app ? app.icon : '\u{1F5B5}'}</span> ${win.title}`;
      el.addEventListener('click', () => {
        if (win.minimized) minimizeWindow(win.id);
        focusWindow(win.id);
      });
      container.appendChild(el);
    });
  }

  // ── App implementations ────────────────────────────────
  function openApp(appId) {
    const app = apps.find(a => a.id === appId);
    const title = app ? app.name : appId;

    const appContent = {
      calculator: () => {
        let expr = '';
        return `<div style="padding:16px;height:100%;display:flex;flex-direction:column">
          <div id="calc-display" style="background:#11111b;color:#cdd6f4;font-size:28px;text-align:right;padding:12px 16px;border-radius:8px;margin-bottom:12px;min-height:50px;font-family:monospace">0</div>
          <div style="display:grid;grid-template-columns:repeat(4,1fr);gap:8px;flex:1">
            ${['C','\u00B1','%','\u00F7','7','8','9','\u00D7','4','5','6','\u2212','1','2','3','+','0','.','\u232B','='].map(b => {
              const isOp = ['\u00F7','\u00D7','\u2212','+','='].includes(b);
              const isFunc = ['C','\u00B1','%'].includes(b);
              const bg = isOp ? '#0078d4' : isFunc ? '#2a2a3e' : '#181825';
              return `<button onclick="WebOS._calcPress('${b}')" style="background:${bg};color:#fff;border:none;border-radius:8px;font-size:20px;cursor:pointer;padding:8px">${b}</button>`;
            }).join('')}
          </div>
        </div>`;
      },

      terminal: () => `<div style="background:#11111b;height:100%;padding:12px;font-family:'Cascadia Code','Consolas',monospace;font-size:13px;color:#a6e3a1;overflow-y:auto">
        <div>WebOS Terminal v0.0.1beta</div>
        <div>Type 'help' for available commands.</div>
        <div style="margin-top:8px;display:flex"><span style="color:#89b4fa">webos@webos:~$&nbsp;</span><input id="term-input" style="flex:1;background:transparent;border:none;color:#a6e3a1;outline:none;font-family:inherit;font-size:inherit" autofocus></div>
        <div id="term-output"></div>
      </div>`,

      browser: () => `<div style="display:flex;flex-direction:column;height:100%">
        <div style="display:flex;gap:4px;padding:8px;background:#2d2d44;align-items:center">
          <button style="background:transparent;border:none;color:#aaa;cursor:pointer;font-size:16px">\u2190</button>
          <button style="background:transparent;border:none;color:#aaa;cursor:pointer;font-size:16px">\u2192</button>
          <button style="background:transparent;border:none;color:#aaa;cursor:pointer;font-size:16px">\u21BB</button>
          <input id="browser-url" value="https://webos.local" style="flex:1;background:rgba(255,255,255,.08);border:1px solid rgba(255,255,255,.1);border-radius:16px;padding:6px 14px;color:#fff;font-size:13px;outline:none">
        </div>
        <div style="flex:1;background:#181825;display:flex;align-items:center;justify-content:center;color:#666;font-size:16px">
          <div style="text-align:center">
            <div style="font-size:48px;margin-bottom:16px">\u{1F310}</div>
            <div>WebOS Browser</div>
            <div style="font-size:13px;margin-top:8px;color:#444">v0.0.1beta</div>
          </div>
        </div>
      </div>`,

      paint: () => `<div style="display:flex;flex-direction:column;height:100%">
        <div style="display:flex;gap:4px;padding:6px;background:#2d2d44;align-items:center">
          <input type="color" id="paint-color" value="#0078d4" style="width:32px;height:28px;border:none;cursor:pointer">
          <input type="range" id="paint-size" min="1" max="20" value="3" style="width:80px">
          <button onclick="WebOS._paintClear()" style="background:rgba(255,255,255,.08);border:1px solid rgba(255,255,255,.1);color:#ccc;border-radius:4px;padding:4px 10px;cursor:pointer;font-size:12px">Clear</button>
        </div>
        <canvas id="paint-canvas" style="flex:1;cursor:crosshair;background:#fff"></canvas>
      </div>`,

      settings: () => `<div style="padding:20px;color:#cdd6f4">
        <h3 style="margin-bottom:16px;color:#fff">Settings</h3>
        <div style="margin-bottom:16px">
          <div style="color:#888;font-size:12px;margin-bottom:6px">Wallpaper</div>
          <div style="display:flex;gap:8px">
            <div onclick="WebOS._setWallpaper('#0a1628')" style="width:60px;height:40px;background:linear-gradient(135deg,#0a1628,#0d2137);border-radius:6px;cursor:pointer;border:2px solid transparent"></div>
            <div onclick="WebOS._setWallpaper('#1a0a28')" style="width:60px;height:40px;background:linear-gradient(135deg,#1a0a28,#2d1040);border-radius:6px;cursor:pointer;border:2px solid transparent"></div>
            <div onclick="WebOS._setWallpaper('#0a281a')" style="width:60px;height:40px;background:linear-gradient(135deg,#0a281a,#0d4030);border-radius:6px;cursor:pointer;border:2px solid transparent"></div>
            <div onclick="WebOS._setWallpaper('#28280a')" style="width:60px;height:40px;background:linear-gradient(135deg,#28280a,#404010);border-radius:6px;cursor:pointer;border:2px solid transparent"></div>
          </div>
        </div>
        <div style="margin-bottom:16px">
          <div style="color:#888;font-size:12px;margin-bottom:6px">System</div>
          <div style="background:rgba(255,255,255,.04);padding:12px;border-radius:8px;font-size:13px">
            <div>Version: 0.0.1beta</div>
            <div>Kernel: WASM Microkernel</div>
            <div>Runtime: Browser</div>
            <div>Memory: ${navigator.deviceMemory || '?'} GB</div>
            <div>Platform: ${navigator.platform}</div>
          </div>
        </div>
      </div>`,

      appstore: () => `<div style="padding:20px;color:#cdd6f4">
        <h3 style="margin-bottom:16px;color:#fff">App Store</h3>
        <div style="display:grid;grid-template-columns:repeat(2,1fr);gap:12px">
          ${[
            {name:'Text Editor',icon:'\u{1F4DD}',cat:'Utilities'},
            {name:'Video Player',icon:'\u{1F3AC}',cat:'Media'},
            {name:'Code Editor',icon:'\u{1F4BB}',cat:'Development'},
            {name:'Chat',icon:'\u{1F4AC}',cat:'Communication'},
          ].map(a => `<div style="background:rgba(255,255,255,.04);border-radius:10px;padding:16px;cursor:pointer">
            <div style="font-size:28px;margin-bottom:8px">${a.icon}</div>
            <div style="color:#fff;font-size:14px">${a.name}</div>
            <div style="color:#666;font-size:12px">${a.cat}</div>
            <button style="margin-top:8px;background:#0078d4;border:none;border-radius:4px;color:#fff;padding:4px 12px;font-size:12px;cursor:pointer">Install</button>
          </div>`).join('')}
        </div>
      </div>`,

      files: () => `<div style="display:flex;flex-direction:column;height:100%">
        <div style="padding:8px;background:#2d2d44;color:#aaa;font-size:13px">/home/webos/</div>
        <div style="flex:1;padding:12px;color:#cdd6f4;font-size:13px">
          ${['\u{1F4C1} Documents','\u{1F4C1} Downloads','\u{1F4C1} Pictures','\u{1F4C1} Music','\u{1F4C4} readme.txt','\u{1F4C4} notes.md'].map(f => 
            `<div style="padding:6px 8px;border-radius:4px;cursor:pointer" onmouseover="this.style.background='rgba(255,255,255,.06)'" onmouseout="this.style.background='transparent'">${f}</div>`
          ).join('')}
        </div>
      </div>`,

      notepad: () => `<div style="height:100%;display:flex;flex-direction:column">
        <textarea style="flex:1;background:#181825;color:#cdd6f4;border:none;padding:16px;font-family:'Cascadia Code','Consolas',monospace;font-size:14px;resize:none;outline:none" placeholder="Start typing..."></textarea>
      </div>`,

      music: () => `<div style="padding:20px;color:#cdd6f4;text-align:center">
        <div style="font-size:64px;margin:24px 0">\u{1F3B5}</div>
        <div style="color:#fff;font-size:18px">No track playing</div>
        <div style="color:#666;font-size:14px;margin-top:8px">Music Player v0.0.1beta</div>
        <div style="margin-top:24px;display:flex;justify-content:center;gap:16px;font-size:24px">
          <span style="cursor:pointer">\u23EE</span>
          <span style="cursor:pointer">\u23EF</span>
          <span style="cursor:pointer">\u23ED</span>
        </div>
      </div>`,
    };

    const contentFn = appContent[appId];
    const content = contentFn ? contentFn() : `<div style="display:flex;align-items:center;justify-content:center;height:100%;color:#888">${title}</div>`;
    const winId = createWindow(title, content, { width: appId === 'calculator' ? 320 : 640, height: appId === 'calculator' ? 480 : 440 });

    // Post-creation setup
    setTimeout(() => {
      if (appId === 'terminal') setupTerminal(winId);
      if (appId === 'paint') setupPaint(winId);
    }, 50);
  }

  // ── Terminal setup ─────────────────────────────────────
  function setupTerminal(winId) {
    const input = document.getElementById('term-input');
    const output = document.getElementById('term-output');
    if (!input || !output) return;

    input.addEventListener('keydown', (e) => {
      if (e.key === 'Enter') {
        const cmd = input.value.trim();
        input.value = '';
        const result = processCommand(cmd);
        output.innerHTML += `<div style="color:#89b4fa">webos@webos:~$ ${cmd}</div>`;
        if (result) output.innerHTML += `<div style="color:#cdd6f4">${result}</div>`;
      }
    });
    input.focus();
  }

  function processCommand(cmd) {
    const commands = {
      help: 'Available commands: help, clear, echo, date, uname, whoami, ls, cat, neofetch, shutdown, reboot',
      clear: '__CLEAR__',
      date: new Date().toString(),
      uname: 'WebOS 0.0.1beta WASM-microkernel x86_64',
      whoami: 'webos',
      ls: 'Documents  Downloads  Pictures  Music  readme.txt  notes.md',
      cat: 'Usage: cat <filename>',
      neofetch: `<pre style="color:#89b4fa">
  ╦ ╦┌─┐┌┐ ╔═╗╔═╗</pre>
<pre style="color:#a6e3a1">  ║║║├┤ ├┴┐║ ║╚═╗</pre>
<pre style="color:#f9e2af">  ╚╩╝└─┘└─┘╚═╝╚═╝</pre>
<span style="color:#cdd6f4">  OS: WebOS 0.0.1beta
  Kernel: WASM Microkernel
  Shell: webos-sh
  Resolution: ${window.innerWidth}x${window.innerHeight}
  DE: WebOS Desktop
  WM: webos-wm
  Terminal: webos-term
  CPU: ${navigator.hardwareConcurrency || '?'} cores
  Memory: ${navigator.deviceMemory || '?'} GB</span>`,
      shutdown: '__SHUTDOWN__',
      reboot: '__REBOOT__',
    };

    if (cmd === '') return '';
    const parts = cmd.split(' ');
    const base = parts[0].toLowerCase();

    if (base === 'echo') return parts.slice(1).join(' ');
    if (base === 'cat' && parts[1] === 'readme.txt') return 'Welcome to WebOS v0.0.1beta!';
    if (base === 'cat' && parts[1] === 'notes.md') return '# WebOS Notes\n- Built with WASM + TypeScript\n- Runs in any modern browser';

    const result = commands[base];
    if (result === '__CLEAR__') {
      const output = document.getElementById('term-output');
      if (output) output.innerHTML = '';
      return '';
    }
    if (result === '__SHUTDOWN__') { shutdown(); return 'Shutting down...'; }
    if (result === '__REBOOT__') { location.reload(); return ''; }
    return result || `webos-sh: command not found: ${base}`;
  }

  // ── Paint setup ────────────────────────────────────────
  function setupPaint(winId) {
    const canvas = document.getElementById('paint-canvas');
    if (!canvas) return;
    const rect = canvas.parentElement.getBoundingClientRect();
    canvas.width = rect.width;
    canvas.height = rect.height - 44;
    const ctx = canvas.getContext('2d');
    ctx.fillStyle = '#fff';
    ctx.fillRect(0, 0, canvas.width, canvas.height);
    let drawing = false;

    canvas.addEventListener('mousedown', (e) => {
      drawing = true;
      const r = canvas.getBoundingClientRect();
      ctx.beginPath();
      ctx.moveTo(e.clientX - r.left, e.clientY - r.top);
    });
    canvas.addEventListener('mousemove', (e) => {
      if (!drawing) return;
      const r = canvas.getBoundingClientRect();
      const color = document.getElementById('paint-color');
      const size = document.getElementById('paint-size');
      ctx.strokeStyle = color ? color.value : '#0078d4';
      ctx.lineWidth = size ? parseInt(size.value) : 3;
      ctx.lineCap = 'round';
      ctx.lineTo(e.clientX - r.left, e.clientY - r.top);
      ctx.stroke();
    });
    canvas.addEventListener('mouseup', () => { drawing = false; });
    canvas.addEventListener('mouseleave', () => { drawing = false; });
  }

  // ── Utility functions ──────────────────────────────────
  function toggleStartMenu(force) {
    const menu = document.getElementById('start-menu');
    state.startMenuOpen = force !== undefined ? force : !state.startMenuOpen;
    menu.classList.toggle('visible', state.startMenuOpen);
    if (state.startMenuOpen) {
      const input = document.getElementById('start-search-input');
      if (input) { input.value = ''; input.focus(); }
    }
  }

  function notify(msg, duration = 3000) {
    const el = document.getElementById('notification');
    if (!el) return;
    el.textContent = msg;
    el.classList.add('show');
    setTimeout(() => el.classList.remove('show'), duration);
  }

  function shutdown() {
    document.getElementById('desktop').classList.remove('visible');
    const bootScreen = document.getElementById('boot-screen');
    bootScreen.style.display = 'flex';
    bootScreen.classList.remove('fade-out');
    const msg = document.getElementById('boot-msg');
    if (msg) msg.textContent = 'Shutting down...';
    setTimeout(() => {
      if (msg) msg.textContent = 'System halted.';
      const spinner = bootScreen.querySelector('.boot-spinner');
      if (spinner) spinner.style.display = 'none';
    }, 2000);
  }

  function refresh() {
    location.reload();
  }

  function showAbout() {
    createWindow('About WebOS', `<div style="padding:24px;text-align:center;color:#cdd6f4">
      <div style="font-size:48px;font-weight:700;color:#fff">Web<span style="color:#0078d4">OS</span></div>
      <div style="color:#888;margin-top:8px">v0.0.1beta</div>
      <div style="margin-top:16px;font-size:14px;line-height:1.8;color:#aaa">
        A web-based operating system<br>
        built with WebAssembly & TypeScript<br><br>
        Kernel: WASM Microkernel<br>
        Desktop: Canvas2D / WebGPU<br>
        Runtime: Browser Native
      </div>
    </div>`, { width: 400, height: 340 });
  }

  // ── Internal helpers (exposed for inline onclick) ──────
  function _calcPress(key) {
    const display = document.getElementById('calc-display');
    if (!display) return;
    if (key === 'C') { display.textContent = '0'; state._calcExpr = ''; return; }
    if (key === '\u232B') { state._calcExpr = (state._calcExpr || '').slice(0, -1); display.textContent = state._calcExpr || '0'; return; }
    if (key === '=') {
      try {
        const expr = (state._calcExpr || '').replace(/\u00F7/g,'/').replace(/\u00D7/g,'*').replace(/\u2212/g,'-');
        const result = Function('"use strict";return (' + expr + ')')();
        display.textContent = Number.isFinite(result) ? parseFloat(result.toFixed(10)) : 'Error';
        state._calcExpr = String(display.textContent);
      } catch { display.textContent = 'Error'; state._calcExpr = ''; }
      return;
    }
    if (key === '\u00B1') {
      if (state._calcExpr && state._calcExpr[0] === '-') state._calcExpr = state._calcExpr.slice(1);
      else if (state._calcExpr) state._calcExpr = '-' + state._calcExpr;
      display.textContent = state._calcExpr || '0';
      return;
    }
    if (key === '%') {
      try { state._calcExpr = String(parseFloat(state._calcExpr) / 100); } catch {}
      display.textContent = state._calcExpr || '0';
      return;
    }
    state._calcExpr = (state._calcExpr || '') + key;
    display.textContent = state._calcExpr;
  }

  function _paintClear() {
    const canvas = document.getElementById('paint-canvas');
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    ctx.fillStyle = '#fff';
    ctx.fillRect(0, 0, canvas.width, canvas.height);
  }

  function _setWallpaper(color) {
    const wp = document.querySelector('.wallpaper');
    if (wp) wp.style.background = color;
  }

  // ── Installer (browser-side simulation for live mode) ──
  function runInstaller() {
    const progress = document.getElementById('installer-progress');
    const bar = document.getElementById('installer-progress-bar');
    const status = document.getElementById('installer-status');
    progress.classList.add('visible');
    status.textContent = 'Starting installation...';
    const steps = [
      {pct: 5,   msg: 'Partitioning disk...'},
      {pct: 15,  msg: 'Formatting EFI partition...'},
      {pct: 25,  msg: 'Formatting root partition...'},
      {pct: 35,  msg: 'Copying WebOS kernel...'},
      {pct: 50,  msg: 'Copying WebOS runtime...'},
      {pct: 65,  msg: 'Installing desktop environment...'},
      {pct: 75,  msg: 'Installing bootloader (GRUB)...'},
      {pct: 85,  msg: 'Configuring initramfs...'},
      {pct: 95,  msg: 'Setting up auto-boot...'},
      {pct: 100, msg: 'Installation complete!'},
    ];
    let i = 0;
    const iv = setInterval(() => {
      if (i >= steps.length) { clearInterval(iv); status.textContent = 'Installation complete! You can now reboot.'; status.style.color = '#28c840'; return; }
      bar.style.width = steps[i].pct + '%';
      status.textContent = steps[i].msg;
      i++;
    }, 800);
  }

  function sleep(ms) { return new Promise(r => setTimeout(r, ms)); }

  // ── Public API ─────────────────────────────────────────
  return {
    boot,
    openApp,
    toggleStartMenu,
    notify,
    shutdown,
    refresh,
    showAbout,
    runInstaller,
    _calcPress,
    _paintClear,
    _setWallpaper,
  };
})();

// Start the system
window.addEventListener('DOMContentLoaded', () => WebOS.boot());
</script>
</body>
</html>
HTMLEOF

# Calculate size of the HTML
HTML_SIZE=$(wc -c < "${BUILD_DIR}/webos/index.html")
log_ok "Created self-contained HTML (${HTML_SIZE} bytes)"

# ─── Step 4: Create GRUB configuration ────────────────────
log_step "4/7" "Creating GRUB bootloader configuration..."

cat > "${BUILD_DIR}/boot/grub/grub.cfg" << 'GRUBEOF'
# WebOS GRUB Configuration v0.0.1beta
# ─────────────────────────────────────

set default=0
set timeout=5
set gfxpayload=keep

# Theme colors
set color_normal=white/black
set color_highlight=cyan/black

menuentry "WebOS v0.0.1beta (Memory OS)" {
    echo "Loading WebOS kernel..."
    linux /boot/vmlinuz quiet splash boot=live nopersistence nomodeset vt.global_cursor_default=0
    echo "Loading initramfs..."
    initrd /boot/initrd.img
}

menuentry "WebOS v0.0.1beta (Safe Mode - VESA)" {
    echo "Loading WebOS kernel (safe mode)..."
    linux /boot/vmlinuz quiet boot=live nopersistence vga=788 nomodeset
    echo "Loading initramfs..."
    initrd /boot/initrd.img
}

menuentry "WebOS v0.0.1beta (Installer)" {
    echo "Loading WebOS installer..."
    linux /boot/vmlinuz quiet boot=live nopersistence webos_mode=install
    echo "Loading initramfs..."
    initrd /boot/initrd.img
}

menuentry "WebOS v0.0.1beta (Debug Console)" {
    echo "Loading WebOS with debug console..."
    linux /boot/vmlinuz boot=live nopersistence debug
    initrd /boot/initrd.img
}

menuentry "Reboot" {
    reboot
}

menuentry "Power Off" {
    halt
}
GRUBEOF

log_ok "GRUB configuration created"

# ─── Step 5: Create init script for initramfs ─────────────
log_step "5/7" "Creating initramfs init script..."

mkdir -p "${BUILD_DIR}/initramfs"
cat > "${BUILD_DIR}/initramfs/init" << 'INITEOF'
#!/bin/sh
# ─────────────────────────────────────────────────────────
# WebOS Init Script v0.0.1beta
# Boots into a minimal Linux environment that launches
# Chromium in kiosk mode pointing to the WebOS application
# ─────────────────────────────────────────────────────────

VERSION="0.0.1beta"
WEBOS_MODE="${webos_mode:-desktop}"

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
CYAN='\033[0;36m'
YELLOW='\033[1;33m'
NC='\033[0m'

log()  { echo -e "${CYAN}[webos-init]${NC} $1"; }
ok()   { echo -e "${GREEN}[OK]${NC} $1"; }
warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
err()  { echo -e "${RED}[ERROR]${NC} $1"; }

log "WebOS v${VERSION} init starting (mode: ${WEBOS_MODE})"

# ─── Phase 1: Mount essential filesystems ────────────────
log "Mounting essential filesystems..."
mount -t proc proc /proc
mount -t sysfs sysfs /sys
mount -t devtmpfs devtmpfs /dev
mkdir -p /dev/pts /dev/shm /run /tmp /var/run /var/log
mount -t devpts devpts /dev/pts
mount -t tmpfs tmpfs /dev/shm
mount -t tmpfs tmpfs /tmp
mount -t tmpfs tmpfs /run
ok "Essential filesystems mounted"

# ─── Phase 2: Set up console ────────────────────────────
# Disable kernel messages on console
echo 0 > /proc/sys/kernel/printk 2>/dev/null || true

# Set hostname
hostname webos 2>/dev/null || true
echo "webos" > /etc/hostname 2>/dev/null || true

# ─── Phase 3: Hardware detection ────────────────────────
log "Detecting hardware..."
# Load essential kernel modules
for mod in ext4 vfat ntfs nls_cp437 nls_iso8859_1 efivarfs \
           i915 amdgpu nouveau fbcon vesafb \
           hid usbhid evdev \
           snd snd-pcm snd-hda-intel \
           e1000 e1000e r8169 realtek \
           ahci nvme sd_mod sr_mod; do
    modprobe "$mod" 2>/dev/null && ok "Module: $mod" || true
done

# Mount EFI vars if available
modprobe efivarfs 2>/dev/null && mount -t efivarfs efivarfs /sys/firmware/efi/efivars 2>/dev/null || true

# ─── Phase 4: Networking ────────────────────────────────
log "Setting up networking..."
# Bring up loopback
ip link set lo up 2>/dev/null || ifconfig lo up 2>/dev/null || true

# Try DHCP on all ethernet interfaces
for iface in /sys/class/net/e*; do
    [ -e "$iface" ] || continue
    ifname=$(basename "$iface")
    log "Configuring $ifname via DHCP..."
    ip link set "$ifname" up 2>/dev/null || true
    udhcpc -i "$ifname" -s /bin/simple-dhcpc -q -n 2>/dev/null || \
    dhclient "$ifname" 2>/dev/null || true
done

# ─── Phase 5: Setup X11 / Wayland ───────────────────────
log "Setting up display server..."

# Create X11 config directory
mkdir -p /etc/X11/xorg.conf.d

# Create a minimal xorg.conf for autodetection
cat > /etc/X11/xorg.conf << 'XORGEOF'
Section "ServerFlags"
    Option "DontZap" "true"
    Option "AllowMouseOpenFail" "true"
    Option "AutoAddDevices" "true"
    Option "AutoAddGPU" "true"
EndSection

Section "InputClass"
    Identifier "Keyboard Defaults"
    MatchIsKeyboard "yes"
    Option "XkbLayout" "us"
EndSection
XORGEOF

# Create xinitrc for kiosk mode
INSTALLER_ARG=""
if [ "$WEBOS_MODE" = "install" ]; then
    INSTALLER_ARG="?mode=install"
fi

cat > /root/.xinitrc << XINITRC
#!/bin/sh
# WebOS kiosk xinitrc

# Disable screen blanking
xset s off
xset -dpms
xset s noblank

# Set background
xsetroot -solid "#0a0a1a"

# Hide cursor (optional — uncomment if you want no cursor)
# xdotool mousemove 9999 9999

# Launch Chromium in kiosk mode
BROWSER=""
for cmd in chromium-browser chromium google-chrome google-chrome-stable firefox; do
    if command -v \$cmd >/dev/null 2>&1; then
        BROWSER=\$cmd
        break
    fi
done

if [ -n "\$BROWSER" ]; then
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
                --inactivity-gap=0 \\
                "file:///webos/index.html${INSTALLER_ARG}"
            ;;
        *firefox*)
            exec \$BROWSER \\
                --kiosk \\
                "file:///webos/index.html${INSTALLER_ARG}"
            ;;
    esac
else
    # No browser found - try to start a simple server
    warn "No browser found!"
    if command -v python3 >/dev/null 2>&1; then
        cd /webos && python3 -m http.server 8080 &
        log "WebOS available at http://localhost:8080"
    fi
    exec /bin/sh
fi
XINITRC
chmod +x /root/.xinitrc

# ─── Phase 6: Launch the display ────────────────────────
log "Starting display server..."

# Check if we have a display
DISPLAY_AVAILABLE=false

# Try to start X
if command -v startx >/dev/null 2>&1; then
    log "Starting X server..."
    startx 2>/var/log/xorg.log &
    X_PID=$!
    sleep 3

    if kill -0 $X_PID 2>/dev/null; then
        DISPLAY_AVAILABLE=true
        ok "X server started (PID: $X_PID)"
    else
        warn "X server failed to start"
    fi
elif command -v Xorg >/dev/null 2>&1; then
    log "Starting Xorg directly..."
    Xorg :0 vt7 2>/var/log/xorg.log &
    X_PID=$!
    sleep 3
    export DISPLAY=:0

    if kill -0 $X_PID 2>/dev/null; then
        DISPLAY_AVAILABLE=true
        ok "Xorg started"
        # Run xinitrc manually
        /bin/sh /root/.xinitrc &
    else
        warn "Xorg failed to start"
    fi
fi

# ─── Phase 7: Fallback console ──────────────────────────
if [ "$DISPLAY_AVAILABLE" = false ]; then
    err "No display server available"
    log "Falling back to text console..."
    log ""
    log "╔═══════════════════════════════════════╗"
    log "║     WebOS v${VERSION} — Console Mode     ║"
    log "╠═══════════════════════════════════════╣"
    log "║                                       ║"
    log "║  WebOS files are at /webos/           ║"
    log "║                                       ║"
    log "║  To start a browser manually:         ║"
    log "║    chromium file:///webos/index.html  ║"
    log "║    firefox file:///webos/index.html   ║"
    log "║                                       ║"
    log "║  To start an HTTP server:             ║"
    log "║    cd /webos && python3 -m http.server║"
    log "║                                       ║"
    log "║  To run the installer:                ║"
    log "║    /installer/install.sh              ║"
    log "║                                       ║"
    log "╚═══════════════════════════════════════╝"
    log ""

    # Start python server if available
    if command -v python3 >/dev/null 2>&1; then
        cd /webos && python3 -m http.server 8080 &
        log "HTTP server started on port 8080"
    fi
fi

# Keep init alive
while true; do
    sleep 60 &
    wait
done
INITEOF

chmod +x "${BUILD_DIR}/initramfs/init"
log_ok "Init script created"

# ─── Step 6: Create installer script ─────────────────────
log_step "6/7" "Creating installer..."

cat > "${BUILD_DIR}/installer/install.sh" << 'INSTEOF'
#!/bin/sh
# ─────────────────────────────────────────────────────────
# WebOS Installer v0.0.1beta
# Installs WebOS to a target disk with EFI + root partitions
# ─────────────────────────────────────────────────────────

set -e

VERSION="0.0.1beta"
RED='\033[0;31m'
GREEN='\033[0;32m'
CYAN='\033[0;36m'
YELLOW='\033[1;33m'
NC='\033[0m'

log()  { echo -e "${CYAN}[install]${NC} $1"; }
ok()   { echo -e "${GREEN}[OK]${NC} $1"; }
warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
err()  { echo -e "${RED}[ERROR]${NC} $1"; }

echo ""
echo "═══════════════════════════════════════════════════════════"
echo "  WebOS Installer v${VERSION}"
echo "═══════════════════════════════════════════════════════════"
echo ""

# ─── Check root ─────────────────────────────────────────
if [ "$(id -u)" -ne 0 ]; then
    err "This installer must be run as root."
    err "Run: sudo /installer/install.sh"
    exit 1
fi

# ─── Detect disks ───────────────────────────────────────
log "Detecting available disks..."
echo ""
echo "Available disks:"
echo "──────────────────────────────────────"

DISKS=""
if command -v lsblk >/dev/null 2>&1; then
    DISKS=$(lsblk -dno NAME,SIZE,MODEL,TYPE | grep -E 'disk|loop' | head -20)
elif [ -d /sys/block ]; then
    for d in /sys/block/sd* /sys/block/nvme* /sys/block/vd*; do
        [ -e "$d" ] || continue
        name=$(basename "$d")
        size=$(cat "$d/size" 2>/dev/null)
        model=$(cat "$d/device/model" 2>/dev/null || echo "Unknown")
        echo "$name  ${size}  $model"
    done
fi

echo "$DISKS"
echo ""

# ─── Select target disk ─────────────────────────────────
if [ -n "$1" ]; then
    TARGET_DISK="$1"
else
    echo -n "Enter target disk (e.g., /dev/sda): "
    read -r TARGET_DISK
fi

if [ ! -b "$TARGET_DISK" ]; then
    err "Device $TARGET_DISK not found or not a block device"
    exit 1
fi

echo ""
warn "ALL DATA ON $TARGET_DISK WILL BE DESTROYED!"
echo -n "Type 'YES' to continue: "
read -r CONFIRM
if [ "$CONFIRM" != "YES" ]; then
    log "Installation cancelled."
    exit 0
fi

# ─── Partition the disk ─────────────────────────────────
log "Partitioning $TARGET_DISK..."

# Wipe existing partition tables
wipefs -a "$TARGET_DISK" 2>/dev/null || true
sgdisk -Z "$TARGET_DISK" 2>/dev/null || true

# Create GPT partition table
parted -s "$TARGET_DISK" mklabel gpt

# EFI System Partition (512 MB)
parted -s "$TARGET_DISK" mkpart primary fat32 1MiB 513MiB
parted -s "$TARGET_DISK" set 1 esp on
parted -s "$TARGET_DISK" set 1 boot on

# Root partition (rest of disk)
parted -s "$TARGET_DISK" mkpart primary ext4 513MiB 100%

ok "Partition table created"

# ─── Determine partition names ──────────────────────────
if echo "$TARGET_DISK" | grep -q "nvme"; then
    EFI_PART="${TARGET_DISK}p1"
    ROOT_PART="${TARGET_DISK}p2"
else
    EFI_PART="${TARGET_DISK}1"
    ROOT_PART="${TARGET_DISK}2"
fi

# Wait for partitions to appear
sleep 2
partprobe "$TARGET_DISK" 2>/dev/null || true
sleep 1

# ─── Format partitions ─────────────────────────────────
log "Formatting EFI partition ($EFI_PART)..."
mkfs.vfat -F 32 -n WEBOS_EFI "$EFI_PART"

log "Formatting root partition ($ROOT_PART)..."
mkfs.ext4 -L webos "$ROOT_PART"

ok "Partitions formatted"

# ─── Mount partitions ──────────────────────────────────
log "Mounting partitions..."
MOUNT_DIR="/mnt/webos"
mkdir -p "$MOUNT_DIR"
mount "$ROOT_PART" "$MOUNT_DIR"
mkdir -p "$MOUNT_DIR/boot/efi"
mount "$EFI_PART" "$MOUNT_DIR/boot/efi"

ok "Partitions mounted at $MOUNT_DIR"

# ─── Install base system ────────────────────────────────
log "Installing WebOS system files..."

# Create directory structure
mkdir -p "$MOUNT_DIR"/{bin,sbin,lib,lib64,etc,proc,sys,dev,tmp,run,var/log}
mkdir -p "$MOUNT_DIR"/boot/grub
mkdir -p "$MOUNT_DIR"/boot/efi
mkdir -p "$MOUNT_DIR"/webos/{wasm,wallpapers,js,css}
mkdir -p "$MOUNT_DIR"/home/webos
mkdir -p "$MOUNT_DIR"/root

# Copy the kernel and initramfs
if [ -f /boot/vmlinuz ]; then
    cp /boot/vmlinuz "$MOUNT_DIR/boot/vmlinuz"
    ok "Kernel installed"
fi

if [ -f /boot/initrd.img ]; then
    cp /boot/initrd.img "$MOUNT_DIR/boot/initrd.img"
    ok "Initramfs installed"
fi

# Copy WebOS application files
if [ -d /webos ]; then
    cp -r /webos/* "$MOUNT_DIR/webos/"
    ok "WebOS files installed"
fi

# Copy installer
if [ -d /installer ]; then
    mkdir -p "$MOUNT_DIR/installer"
    cp /installer/install.sh "$MOUNT_DIR/installer/"
fi

# ─── Create fstab ───────────────────────────────────────
log "Creating /etc/fstab..."
ROOT_UUID=$(blkid -s UUID -o value "$ROOT_PART")
EFI_UUID=$(blkid -s UUID -o value "$EFI_PART")

cat > "$MOUNT_DIR/etc/fstab" << FSTAB
# WebOS v${VERSION} fstab
UUID=${ROOT_UUID}  /          ext4  defaults,noatime  0  1
UUID=${EFI_UUID}   /boot/efi  vfat  defaults          0  2
tmpfs              /tmp       tmpfs defaults          0  0
tmpfs              /run       tmpfs defaults          0  0
FSTAB

ok "fstab created"

# ─── Create init for installed system ───────────────────
cat > "$MOUNT_DIR/sbin/webos-init" << 'WEBOSEOF'
#!/bin/sh
# WebOS installed system init
exec /init
WEBOSEOF
chmod +x "$MOUNT_DIR/sbin/webos-init"

# ─── Install GRUB bootloader ────────────────────────────
log "Installing GRUB bootloader..."

if [ -d /sys/firmware/efi ]; then
    # UEFI install
    grub-install --target=x86_64-efi \
        --efi-directory="$MOUNT_DIR/boot/efi" \
        --bootloader-id=WebOS \
        --removable \
        --no-nvram \
        2>/dev/null && ok "GRUB installed (UEFI)" || \
    warn "GRUB UEFI install failed (may need manual setup)"
else
    # BIOS install
    grub-install --target=i386-pc \
        --boot-directory="$MOUNT_DIR/boot" \
        "$TARGET_DISK" \
        2>/dev/null && ok "GRUB installed (BIOS)" || \
    warn "GRUB BIOS install failed (may need manual setup)"
fi

# Create GRUB config for installed system
cat > "$MOUNT_DIR/boot/grub/grub.cfg" << 'GRUBCFG'
set default=0
set timeout=3

menuentry "WebOS v0.0.1beta" {
    linux /boot/vmlinuz quiet splash root=UUID=ROOTUUID boot=live nopersistence
    initrd /boot/initrd.img
}

menuentry "WebOS v0.0.1beta (Safe Mode)" {
    linux /boot/vmlinuz quiet boot=live nopersistence vga=788 nomodeset
    initrd /boot/initrd.img
}
GRUBCFG

# Replace ROOTUUID placeholder
sed -i "s/ROOTUUID/${ROOT_UUID}/g" "$MOUNT_DIR/boot/grub/grub.cfg"

ok "GRUB configuration created"

# ─── Unmount ────────────────────────────────────────────
log "Unmounting partitions..."
sync
umount "$MOUNT_DIR/boot/efi"
umount "$MOUNT_DIR"

ok "Installation complete!"
echo ""
echo "═══════════════════════════════════════════════════════════"
echo -e "  ${GREEN}WebOS v${VERSION} installed successfully!${NC}"
echo "═══════════════════════════════════════════════════════════"
echo ""
echo "  Installed to: $TARGET_DISK"
echo "  Root: $ROOT_PART (ext4)"
echo "  EFI:  $EFI_PART (vfat)"
echo ""
echo "  Remove the installation media and reboot to start WebOS."
echo ""
INSTEOF

chmod +x "${BUILD_DIR}/installer/install.sh"
log_ok "Installer script created"

# ─── Step 7: Build the ISO ───────────────────────────────
log_step "7/7" "Building ISO image..."
mkdir -p "$(dirname "$OUTPUT_PATH")"

# Locate kernel
VMLINUZ=""
INITRD=""

if [ "$QUICK_MODE" = false ]; then
    # Try multiple kernel locations
    for kpath in \
        "/boot/vmlinuz-$(uname -r)" \
        "/boot/vmlinuz" \
        "/vmlinuz" \
        "/usr/lib/syslinux/vmlinuz" \
        "${PROJECT_DIR}/build/kernel/vmlinuz"; do
        if [ -f "$kpath" ]; then
            VMLINUZ="$kpath"
            break
        fi
    done

    # Try multiple initrd locations
    for ipath in \
        "/boot/initrd.img-$(uname -r)" \
        "/boot/initrd.img" \
        "/initrd.img" \
        "${PROJECT_DIR}/build/kernel/initrd.img"; do
        if [ -f "$ipath" ]; then
            INITRD="$ipath"
            break
        fi
    done
fi

if [ -n "$VMLINUZ" ]; then
    log_ok "Found kernel: $VMLINUZ"
    cp "$VMLINUZ" "${BUILD_DIR}/boot/vmlinuz"

    # Build custom initramfs with our init script + webos files
    log "Building custom initramfs..."
    INITRD_DIR="${BUILD_DIR}/initrd_staging"
    rm -rf "${INITRD_DIR}"
    mkdir -p "${INITRD_DIR}"/{bin,sbin,etc,proc,sys,dev,tmp,run,var/log,var/run,root,home,webos,installer,boot,usr/bin,usr/sbin,usr/lib,lib64,mnt}

    # Copy the init script
    cp "${BUILD_DIR}/initramfs/init" "${INITRD_DIR}/init"
    chmod +x "${INITRD_DIR}/init"

    # Copy WebOS files into initramfs
    cp -r "${BUILD_DIR}/webos"/* "${INITRD_DIR}/webos/"

    # Copy installer into initramfs
    cp -r "${BUILD_DIR}/installer"/* "${INITRD_DIR}/installer/"

    # Copy basic utilities if available (for installer)
    for cmd in sh bash ls cp mv rm cat echo mkdir mount umount parted mkfs.ext4 mkfs.vfat grub-install wipefs sgdisk blkid ip udhcpc dhclient modprobe sleep sync chmod hostname; do
        CMD_PATH=$(command -v "$cmd" 2>/dev/null || true)
        if [ -n "$CMD_PATH" ]; then
            # Copy the binary and its library dependencies
            cp "$CMD_PATH" "${INITRD_DIR}/bin/" 2>/dev/null || true
            # Try to copy shared libraries
            ldd "$CMD_PATH" 2>/dev/null | grep -o '/[^ ]*' | while read lib; do
                [ -f "$lib" ] && cp -f "$lib" "${INITRD_DIR}${lib}" 2>/dev/null || true
            done
        fi
    done

    # Create etc files
    echo "webos" > "${INITRD_DIR}/etc/hostname"
    echo "127.0.0.1 localhost webos" > "${INITRD_DIR}/etc/hosts"

    # Build the initramfs cpio archive
    (cd "${INITRD_DIR}" && find . | cpio -o -H newc 2>/dev/null | gzip -9) > "${BUILD_DIR}/boot/initrd.img"
    INITRD_SIZE=$(du -h "${BUILD_DIR}/boot/initrd.img" | cut -f1)
    log_ok "Custom initramfs built (${INITRD_SIZE})"

    # Cleanup staging
    rm -rf "${INITRD_DIR}"

    # Build ISO with GRUB (bootable)
    log "Creating bootable ISO with GRUB..."
    if command -v grub-mkrescue >/dev/null 2>&1; then
        grub-mkrescue -o "$OUTPUT_PATH" "${BUILD_DIR}" 2>/dev/null
        if [ $? -eq 0 ]; then
            ISO_SIZE=$(du -h "$OUTPUT_PATH" | cut -f1)
            echo ""
            echo "═══════════════════════════════════════════════════════════"
            echo -e "  ${GREEN}Bootable ISO built successfully!${NC}"
            echo "═══════════════════════════════════════════════════════════"
            echo "  Output: $OUTPUT_PATH"
            echo "  Size:   $ISO_SIZE"
            echo ""
            echo "  Boot options:"
            echo "    1. WebOS (Memory OS)  — Run from RAM"
            echo "    2. Safe Mode          — VESA fallback"
            echo "    3. Installer          — Install to disk"
            echo "    4. Debug Console      — Text mode"
            echo ""
            echo "  To test with QEMU:"
            echo "    qemu-system-x86_64 -cdrom $OUTPUT_PATH -m 2G"
            echo "═══════════════════════════════════════════════════════════"
        else
            log_warn "grub-mkrescue failed, trying xorriso..."
            build_data_iso
        fi
    else
        log_warn "grub-mkrescue not found, trying xorriso..."
        build_data_iso
    fi
else
    # No kernel found
    if [ "$QUICK_MODE" = false ]; then
        log_warn "No Linux kernel found. Creating data-only ISO."
    fi

    # Create README
    cat > "${BUILD_DIR}/README.txt" << 'READMEEOF'
WebOS v0.0.1beta — Memory Operating System
===========================================

This ISO contains the WebOS application files.

To create a fully bootable ISO:
1. Ensure a Linux kernel is installed (vmlinuz)
2. Install required tools: xorriso, grub-pc-bin, mtools, cpio
3. Run: ./tools/build_iso.sh

The files in /webos/ can also be used by:
- Opening webos/index.html in any modern browser
- Serving via: python3 -m http.server 8080

To install WebOS to disk:
- Boot the ISO and select "Installer" from the menu
- Or run: sudo /installer/install.sh /dev/sdX
READMEEOF

    build_data_iso
fi

# ─── Helper: build data-only ISO ─────────────────────────
build_data_iso() {
    if command -v xorriso >/dev/null 2>&1; then
        xorriso -as mkisofs \
            -o "$OUTPUT_PATH" \
            -J -r -V "WebOS-${VERSION}" \
            "${BUILD_DIR}" 2>/dev/null
        if [ $? -eq 0 ]; then
            ISO_SIZE=$(du -h "$OUTPUT_PATH" | cut -f1)
            echo ""
            echo "═══════════════════════════════════════════════════════════"
            echo -e "  ${YELLOW}Data ISO built (not bootable)${NC}"
            echo "═══════════════════════════════════════════════════════════"
            echo "  Output: $OUTPUT_PATH"
            echo "  Size:   $ISO_SIZE"
            echo ""
            echo "  Note: This ISO is not bootable. Install a Linux kernel"
            echo "  and the required tools to create a bootable ISO."
            echo "═══════════════════════════════════════════════════════════"
        else
            log_err "xorriso failed"
            exit 1
        fi
    elif command -v mkisofs >/dev/null 2>&1; then
        mkisofs -o "$OUTPUT_PATH" -J -r -V "WebOS-${VERSION}" "${BUILD_DIR}" 2>/dev/null
        ISO_SIZE=$(du -h "$OUTPUT_PATH" | cut -f1)
        echo "  Data ISO: $OUTPUT_PATH ($ISO_SIZE)"
    else
        log_err "No ISO creation tool found. Install xorriso or genisoimage."
        exit 1
    fi
}

# ─── Cleanup ─────────────────────────────────────────────
rm -rf "${BUILD_DIR}/initramfs"

echo ""
echo "Done! ISO image: $OUTPUT_PATH"
