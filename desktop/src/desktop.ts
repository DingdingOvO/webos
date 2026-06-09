/**
 * WebOS Desktop Environment
 * 
 * Manages the visual desktop: wallpaper, icons, windows, taskbar, start menu.
 * Communicates with the WASM kernel and window manager service.
 */

import { DesktopRenderer } from './renderer';
import { Taskbar } from './taskbar';
import { StartMenu } from './start_menu';

export interface DesktopConfig {
  wallpaperColor: string;
  taskbarHeight: number;
  iconSize: number;
  gridSpacing: number;
}

const DEFAULT_CONFIG: DesktopConfig = {
  wallpaperColor: '#0078d4',
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
  onActivate: () => void;
}

export class Desktop {
  private renderer: DesktopRenderer;
  private taskbar: Taskbar;
  private startMenu: StartMenu;
  private config: DesktopConfig;
  private icons: DesktopIcon[] = [];
  private running: boolean = false;
  private animFrame: number = 0;

  constructor(canvas: HTMLCanvasElement, config: Partial<DesktopConfig> = {}) {
    this.config = { ...DEFAULT_CONFIG, ...config };
    this.renderer = new DesktopRenderer(canvas);
    this.taskbar = new Taskbar(this.config.taskbarHeight);
    this.startMenu = new StartMenu();
  }

  async init(): Promise<void> {
    await this.renderer.init();
    this.setupDefaultIcons();
    this.setupEventHandlers();
    console.log('[Desktop] Initialized');
  }

  private setupDefaultIcons(): void {
    const iconPositions = [
      { x: 40, y: 40 },   // Calculator
      { x: 40, y: 130 },  // Paint
      { x: 40, y: 220 },  // Browser
      { x: 40, y: 310 },  // App Store
      { x: 40, y: 400 },  // Terminal
      { x: 40, y: 490 },  // Settings
    ];

    const apps = [
      { id: 'calculator', name: 'Calculator', icon: '🔢' },
      { id: 'paint', name: 'Paint', icon: '🎨' },
      { id: 'browser', name: 'Browser', icon: '🌐' },
      { id: 'appstore', name: 'App Store', icon: '🏪' },
      { id: 'terminal', name: 'Terminal', icon: '💻' },
      { id: 'settings', name: 'Settings', icon: '⚙️' },
    ];

    this.icons = apps.map((app, i) => ({
      ...app,
      ...iconPositions[i],
      onActivate: () => this.launchApp(app.id),
    }));
  }

  private setupEventHandlers(): void {
    this.renderer.canvas.addEventListener('dblclick', (e) => {
      const rect = this.renderer.canvas.getBoundingClientRect();
      const x = e.clientX - rect.left;
      const y = e.clientY - rect.top;

      // Check if double-clicked on an icon
      for (const icon of this.icons) {
        if (x >= icon.x && x <= icon.x + this.config.iconSize &&
            y >= icon.y && y <= icon.y + this.config.iconSize) {
          icon.onActivate();
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
    });

    // Handle resize
    window.addEventListener('resize', () => {
      this.renderer.resize(window.innerWidth, window.innerHeight);
    });
  }

  private launchApp(appId: string): void {
    console.log(`[Desktop] Launching app: ${appId}`);
    // In production, this sends an IPC message to the kernel
    // to spawn the corresponding .wex process
    const moduleMap: Record<string, string> = {
      calculator: 'calculator.wex',
      paint: 'paint.wex',
      browser: 'browser.wex',
      appstore: 'appstore.wex',
      terminal: 'shell.wex',
      settings: 'settings.wex',
    };
    const module = moduleMap[appId];
    if (module) {
      // kernel_spawn_process(module)
      console.log(`[Desktop] Spawning: ${module}`);
    }
  }

  /** Render one frame of the desktop */
  private render = (): void => {
    if (!this.running) return;

    const w = this.renderer.canvasWidth;
    const h = this.renderer.canvasHeight;

    // 1. Draw wallpaper
    this.renderer.clear(this.config.wallpaperColor);

    // 2. Draw desktop icons
    for (const icon of this.icons) {
      this.renderer.drawRoundRect(
        icon.x, icon.y, this.config.iconSize, this.config.iconSize,
        8, 'rgba(255,255,255,0.15)'
      );
      this.renderer.drawText(icon.icon, icon.x + 10, icon.y + 35, '#ffffff', '28px sans-serif');
      this.renderer.drawText(icon.name, icon.x - 5, icon.y + this.config.iconSize + 18, '#ffffff', '12px "Segoe UI", sans-serif');
    }

    // 3. Draw windows (managed by WM service)
    // This would query the WM service for window positions and render them

    // 4. Draw start menu (if open)
    if (this.startMenu.isOpen) {
      this.renderStartMenu();
    }

    // 5. Draw taskbar (always on top)
    this.renderTaskbar();

    this.animFrame = requestAnimationFrame(this.render);
  };

  private renderTaskbar(): void {
    const h = this.renderer.canvasHeight;
    const th = this.config.taskbarHeight;
    const w = this.renderer.canvasWidth;

    // Taskbar background
    this.renderer.setAlpha(0.85);
    this.renderer.drawRect(0, h - th, w, th, '#1a1a2e');
    this.renderer.resetAlpha();

    // Start button
    this.renderer.drawRoundRect(4, h - th + 6, 42, th - 12, 6, '#0078d4');
    this.renderer.drawText('⊞', 16, h - 14, '#ffffff', '20px sans-serif');

    // Running apps area (center)
    this.renderer.drawRect(54, h - th + 8, 200, th - 16, 'rgba(255,255,255,0.08)');

    // System tray (right)
    const trayX = w - 200;
    this.renderer.drawText('📶 🔊 🕐', trayX, h - 16, '#cccccc', '14px sans-serif');
    this.renderer.drawText(new Date().toLocaleTimeString(), w - 80, h - 16, '#ffffff', '13px "Segoe UI", sans-serif');
  }

  private renderStartMenu(): void {
    const h = this.renderer.canvasHeight;
    const th = this.config.taskbarHeight;
    const menuH = 400;
    const menuW = 300;
    const menuX = 4;
    const menuY = h - th - menuH;

    // Background
    this.renderer.setAlpha(0.92);
    this.renderer.drawRoundRect(menuX, menuY, menuW, menuH, 12, '#2d2d44');
    this.renderer.resetAlpha();

    // Search bar
    this.renderer.drawRoundRect(menuX + 16, menuY + 16, menuW - 32, 36, 18, 'rgba(255,255,255,0.12)');
    this.renderer.drawText('Search...', menuX + 32, menuY + 40, '#888888', '14px "Segoe UI", sans-serif');

    // Pinned apps
    this.renderer.drawText('Pinned', menuX + 20, menuY + 80, '#aaaaaa', '12px "Segoe UI", sans-serif');
    const pinnedApps = ['🔢 Calc', '🎨 Paint', '🌐 Browse', '💻 Term', '⚙️ Config', '🏪 Store'];
    for (let i = 0; i < pinnedApps.length; i++) {
      const col = i % 3;
      const row = Math.floor(i / 3);
      this.renderer.drawRoundRect(
        menuX + 20 + col * 90, menuY + 90 + row * 70,
        78, 58, 8, 'rgba(255,255,255,0.06)'
      );
      this.renderer.drawText(pinnedApps[i], menuX + 30 + col * 90, menuY + 125 + row * 70, '#ffffff', '13px sans-serif');
    }

    // Power button
    this.renderer.drawText('⏻ Power', menuX + 20, menuY + menuH - 20, '#ff6666', '14px "Segoe UI", sans-serif');
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
