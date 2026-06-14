/**
 * WebOS Desktop Environment
 * 
 * Manages the visual desktop: wallpaper, icons, windows, taskbar, start menu.
 * All 10 pre-installed apps are registered and launchable.
 * Design language: Frosted Glass (#3B82F6 accent, blur+transparency+noise)
 *
 * Version: 0.0.1beta
 */

import { DesktopRenderer } from './renderer';
import { Taskbar } from './taskbar';
import { StartMenu } from './start_menu';
import { NotificationSystem } from './notifications';

export interface DesktopConfig {
  wallpaperColor: string;
  taskbarHeight: number;
  iconSize: number;
  gridSpacing: number;
}

const DEFAULT_CONFIG: DesktopConfig = {
  wallpaperColor: '#0F172A',
  taskbarHeight: 48,
  iconSize: 48,
  gridSpacing: 80,
};

export interface DesktopIcon {
  id: string;
  name: string;
  icon: string;
  x: number;
  y: number;
  appId: string;
  module: string;
}

export class Desktop {
  private renderer: DesktopRenderer;
  private taskbar: Taskbar;
  private startMenu: StartMenu;
  private notifications: NotificationSystem;
  private config: DesktopConfig;
  private icons: DesktopIcon[] = [];
  private running: boolean = false;
  private animFrame: number = 0;

  constructor(canvas: HTMLCanvasElement, config: Partial<DesktopConfig> = {}) {
    this.config = { ...DEFAULT_CONFIG, ...config };
    this.renderer = new DesktopRenderer(canvas);
    this.taskbar = new Taskbar(this.config.taskbarHeight);
    this.startMenu = new StartMenu();
    this.notifications = new NotificationSystem();
  }

  async init(): Promise<void> {
    await this.renderer.init();
    this.setupDefaultIcons();
    this.setupEventHandlers();
    console.log('[Desktop] Initialized with 10 apps');
  }

  /** All 10 pre-installed applications */
  private setupDefaultIcons(): void {
    const apps = [
      { id: 'terminal',      name: 'Terminal',        icon: '💻', appId: 'com.os.terminal',      module: 'terminal.wex' },
      { id: 'filemanager',   name: 'File Manager',    icon: '📁', appId: 'com.os.filemanager',   module: 'file_manager.wex' },
      { id: 'texteditor',    name: 'Text Editor',     icon: '📝', appId: 'com.os.texteditor',    module: 'text_editor.wex' },
      { id: 'settings',      name: 'Settings',        icon: '⚙️', appId: 'com.os.settings',      module: 'settings.wex' },
      { id: 'calculator',    name: 'Calculator',      icon: '🔢', appId: 'com.os.calculator',    module: 'calculator.wex' },
      { id: 'paint',         name: 'Paint',           icon: '🎨', appId: 'com.os.paint',         module: 'paint.wex' },
      { id: 'musicplayer',   name: 'Music Player',    icon: '🎵', appId: 'com.os.musicplayer',   module: 'music_player.wex' },
      { id: 'sysmonitor',    name: 'System Monitor',  icon: '📊', appId: 'com.os.sysmonitor',    module: 'system_monitor.wex' },
      { id: 'browser',       name: 'Browser',         icon: '🌐', appId: 'com.os.browser',       module: 'browser.wex' },
      { id: 'appstore',      name: 'App Store',       icon: '🏪', appId: 'com.os.appstore',      module: 'appstore.wex' },
    ];

    // Layout: 2 columns, left side
    const col1X = 40;
    const col2X = 40 + this.config.iconSize + 20;

    this.icons = apps.map((app, i) => ({
      ...app,
      x: i % 2 === 0 ? col1X : col2X,
      y: 40 + Math.floor(i / 2) * (this.config.iconSize + this.config.gridSpacing),
    }));

    // Register apps in taskbar and start menu
    for (const app of apps) {
      this.startMenu.addApp({
        id: app.id,
        name: app.name,
        icon: app.icon,
        category: this.getCategory(app.id),
        pinned: true,
      });
    }
  }

  private getCategory(appId: string): string {
    const categories: Record<string, string> = {
      terminal: 'System',
      filemanager: 'Utilities',
      texteditor: 'Utilities',
      settings: 'System',
      calculator: 'Utilities',
      paint: 'Graphics',
      musicplayer: 'Multimedia',
      sysmonitor: 'System',
      browser: 'Internet',
      appstore: 'System',
    };
    return categories[appId] || 'Utilities';
  }

  private setupEventHandlers(): void {
    this.renderer.canvas.addEventListener('dblclick', (e) => {
      const rect = this.renderer.canvas.getBoundingClientRect();
      const x = e.clientX - rect.left;
      const y = e.clientY - rect.top;

      // Check if double-clicked on an icon
      for (const icon of this.icons) {
        if (x >= icon.x && x <= icon.x + this.config.iconSize &&
            y >= icon.y && y <= icon.y + this.config.iconSize + 20) {
          this.launchApp(icon.id);
          break;
        }
      }
    });

    this.renderer.canvas.addEventListener('click', (e) => {
      const rect = this.renderer.canvas.getBoundingClientRect();
      const x = e.clientX - rect.left;
      const y = e.clientY - rect.top;

      // Toggle start menu
      if (y >= this.renderer.canvasHeight - this.config.taskbarHeight) {
        if (x < 50) {
          this.startMenu.toggle();
        }
      } else {
        this.startMenu.close();
      }

      // Check notification click
      this.notifications.handleClick(x, y, this.renderer.canvasWidth);
    });

    // Handle resize
    window.addEventListener('resize', () => {
      this.renderer.resize(window.innerWidth, window.innerHeight);
    });
  }

  private launchApp(appId: string): void {
    console.log(`[Desktop] Launching app: ${appId}`);
    const icon = this.icons.find(i => i.id === appId);
    if (!icon) return;

    // Add to taskbar running apps
    this.taskbar.addApp({
      id: appId,
      name: icon.name,
      icon: icon.icon,
      active: true,
    });

    // Show notification
    this.notifications.pushNotification(
      'App Launched',
      `${icon.name} is starting...`,
      icon.icon,
      'info',
    );

    // In production, this sends an IPC message to the kernel
    // to spawn the corresponding .wex process
    console.log(`[Desktop] Spawning: ${icon.module}`);
  }

  /** Render one frame of the desktop */
  private render = (): void => {
    if (!this.running) return;

    const w = this.renderer.canvasWidth;
    const h = this.renderer.canvasHeight;

    // 1. Draw wallpaper (deep blue gradient)
    this.renderer.clear(this.config.wallpaperColor);

    // 2. Draw desktop icons
    for (const icon of this.icons) {
      // Icon background (subtle hover-style)
      this.renderer.drawRoundRect(
        icon.x - 4, icon.y - 4,
        this.config.iconSize + 8, this.config.iconSize + 24,
        8, 'rgba(255,255,255,0.06)',
      );
      // Emoji icon
      this.renderer.drawText(icon.icon, icon.x + 8, icon.y + 35, '#F8FAFC', '28px sans-serif');
      // Label
      this.renderer.drawText(icon.name, icon.x - 8, icon.y + this.config.iconSize + 18, '#F8FAFC', '11px "Segoe UI", sans-serif');
    }

    // 3. Draw start menu (if open)
    if (this.startMenu.isOpen) {
      this.renderStartMenu();
    }

    // 4. Draw taskbar (always on top)
    this.renderTaskbar();

    // 5. Draw notifications
    if (this.renderer.ctx) {
      this.notifications.renderNotifications(this.renderer.ctx, w);
    }

    this.animFrame = requestAnimationFrame(this.render);
  };

  private renderTaskbar(): void {
    const h = this.renderer.canvasHeight;
    const th = this.config.taskbarHeight;
    const w = this.renderer.canvasWidth;

    // Taskbar background — blurred, semi-transparent with noise
    this.renderer.setAlpha(0.78);
    this.renderer.drawRect(0, h - th, w, th, '#1E293B');
    this.renderer.resetAlpha();

    // Top border (1px, replaces shadow)
    this.renderer.drawRect(0, h - th, w, 1, 'rgba(148,163,184,0.12)');

    // Start button (blue accent)
    this.renderer.drawRoundRect(4, h - th + 6, 42, th - 12, 6, '#3B82F6');
    this.renderer.drawText('⊞', 16, h - 14, '#F8FAFC', '20px sans-serif');

    // Running apps area (center)
    const runningApps = this.taskbar.runningApps;
    let appX = 54;
    for (const app of runningApps) {
      const bg = app.active ? 'rgba(59,130,246,0.2)' : 'rgba(255,255,255,0.06)';
      this.renderer.drawRoundRect(appX, h - th + 6, 36, th - 12, 4, bg);
      this.renderer.drawText(app.icon, appX + 8, h - 14, '#F8FAFC', '16px sans-serif');
      appX += 42;
    }

    // System tray (right)
    const trayX = w - 200;
    this.renderer.drawText('📶 🔊', trayX, h - 16, '#94A3B8', '14px sans-serif');
    this.renderer.drawText(
      new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' }),
      w - 60, h - 16, '#F8FAFC', '13px "Segoe UI", sans-serif',
    );
  }

  private renderStartMenu(): void {
    const h = this.renderer.canvasHeight;
    const th = this.config.taskbarHeight;
    const menuH = 440;
    const menuW = 320;
    const menuX = 4;
    const menuY = h - th - menuH;

    // Background — frosted glass
    this.renderer.setAlpha(0.85);
    this.renderer.drawRoundRect(menuX, menuY, menuW, menuH, 16, '#1E293B');
    this.renderer.resetAlpha();

    // Border
    this.renderer.drawRect(menuX, menuY, menuW, 1, 'rgba(148,163,184,0.12)');

    // Search bar
    this.renderer.drawRoundRect(menuX + 16, menuY + 16, menuW - 32, 36, 18, 'rgba(255,255,255,0.08)');
    this.renderer.drawText('🔍 Search...', menuX + 32, menuY + 40, '#64748B', '14px "Segoe UI", sans-serif');

    // Pinned section header
    this.renderer.drawText('Pinned', menuX + 20, menuY + 76, '#94A3B8', '12px "Segoe UI", sans-serif');

    // App grid — 4 columns
    const pinnedApps = this.startMenu.pinnedApps;
    const cols = 4;
    const cellW = (menuW - 40) / cols;
    const cellH = 64;

    for (let i = 0; i < pinnedApps.length; i++) {
      const col = i % cols;
      const row = Math.floor(i / cols);
      const cx = menuX + 20 + col * cellW;
      const cy = menuY + 88 + row * (cellH + 4);

      this.renderer.drawRoundRect(cx, cy, cellW - 4, cellH, 8, 'rgba(255,255,255,0.04)');
      this.renderer.drawText(pinnedApps[i].icon, cx + cellW / 2 - 14, cy + 28, '#F8FAFC', '20px sans-serif');
      this.renderer.drawText(pinnedApps[i].name, cx + 4, cy + 52, '#94A3B8', '10px "Segoe UI", sans-serif');
    }

    // Divider
    const dividerY = menuY + 88 + Math.ceil(pinnedApps.length / cols) * (cellH + 4) + 8;
    this.renderer.drawRect(menuX + 16, dividerY, menuW - 32, 1, 'rgba(148,163,184,0.12)');

    // Power button
    this.renderer.drawText('⏻ Power', menuX + 20, menuY + menuH - 20, '#EF4444', '14px "Segoe UI", sans-serif');

    // Version
    this.renderer.drawText('v0.0.1beta', menuX + menuW - 80, menuY + menuH - 20, '#64748B', '11px "Segoe UI", sans-serif');
  }

  /** Start the desktop render loop */
  start(): void {
    this.running = true;
    this.render();
  }

  /** Stop the desktop */
  stop(): void {
    this.running = false;
    if (this.animFrame) {
      cancelAnimationFrame(this.animFrame);
    }
  }
}
