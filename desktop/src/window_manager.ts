/**
 * WebOS Window Manager
 *
 * Handles window creation/destruction, dragging, resizing,
 * z-ordering, minimize/maximize/restore, and focus tracking.
 * All rendering is performed via Canvas 2D.
 */

import { DesktopRenderer } from './renderer';

// Catppuccin Mocha palette
const COLORS = {
  base: '#1e1e2e',
  mantle: '#181825',
  crust: '#11111b',
  surface0: '#313244',
  surface1: '#45475a',
  surface2: '#585b70',
  overlay0: '#6c7086',
  overlay1: '#7f849c',
  overlay2: '#9399b2',
  text: '#cdd6f4',
  subtext: '#a6adc8',
  blue: '#89b4fa',
  green: '#a6e3a1',
  red: '#f38ba8',
  yellow: '#f9e2af',
  peach: '#fab387',
};

const TITLEBAR_HEIGHT = 32;
const RESIZE_HANDLE_SIZE = 6;
const BORDER_RADIUS = 8;
const SHADOW_OFFSET = 4;
const SHADOW_BLUR = 12;

export interface WindowState {
  handle: number;
  title: string;
  x: number;
  y: number;
  width: number;
  height: number;
  minWidth: number;
  minHeight: number;
  isMinimized: boolean;
  isMaximized: boolean;
  isActive: boolean;
  zOrder: number;
  canvas: HTMLCanvasElement;
  titlebarColor: string;
  icon: string;
  appId: string;
  preMaxBounds?: { x: number; y: number; width: number; height: number };
}

type ResizeDirection = 'n' | 'ne' | 'e' | 'se' | 's' | 'sw' | 'w' | 'nw' | null;

interface DragState {
  handle: number;
  offsetX: number;
  offsetY: number;
  isResizing: boolean;
  resizeDir: ResizeDirection;
  startX: number;
  startY: number;
  startWinX: number;
  startWinY: number;
  startWinW: number;
  startWinH: number;
}

export class WindowManager {
  private windows: Map<number, WindowState> = new Map();
  private nextHandle: number = 1;
  private nextZOrder: number = 1;
  private renderer: DesktopRenderer;
  private dragState: DragState | null = null;
  private taskbarHeight: number;
  private hoverButton: { handle: number; button: 'min' | 'max' | 'close' } | null = null;

  constructor(renderer: DesktopRenderer, taskbarHeight: number = 48) {
    this.renderer = renderer;
    this.taskbarHeight = taskbarHeight;
  }

  createWindow(
    title: string,
    x: number,
    y: number,
    w: number,
    h: number,
    icon: string = '📱',
    appId: string = '',
    titlebarColor: string = COLORS.blue,
  ): number {
    const handle = this.nextHandle++;
    const canvas = document.createElement('canvas');
    canvas.width = w;
    canvas.height = Math.max(0, h - TITLEBAR_HEIGHT);

    const state: WindowState = {
      handle,
      title,
      x,
      y,
      width: w,
      height: h,
      minWidth: 200,
      minHeight: 120,
      isMinimized: false,
      isMaximized: false,
      isActive: true,
      zOrder: this.nextZOrder++,
      canvas,
      titlebarColor,
      icon,
      appId,
    };

    // Deactivate other windows
    for (const win of this.windows.values()) {
      win.isActive = false;
    }

    this.windows.set(handle, state);
    return handle;
  }

  destroyWindow(handle: number): void {
    this.windows.delete(handle);
    if (this.dragState?.handle === handle) {
      this.dragState = null;
    }
    // Activate topmost remaining window
    this.activateTopmost();
  }

  moveWindow(handle: number, x: number, y: number): void {
    const win = this.windows.get(handle);
    if (win) {
      win.x = x;
      win.y = y;
    }
  }

  resizeWindow(handle: number, w: number, h: number): void {
    const win = this.windows.get(handle);
    if (win) {
      win.width = Math.max(w, win.minWidth);
      win.height = Math.max(h, win.minHeight);
      win.canvas.width = win.width;
      win.canvas.height = Math.max(0, win.height - TITLEBAR_HEIGHT);
    }
  }

  minimizeWindow(handle: number): void {
    const win = this.windows.get(handle);
    if (win && !win.isMinimized) {
      win.isMinimized = true;
      win.isActive = false;
      this.activateTopmost();
    }
  }

  maximizeWindow(handle: number): void {
    const win = this.windows.get(handle);
    if (!win) return;

    if (win.isMaximized) {
      // Restore
      this.restoreWindow(handle);
      return;
    }

    win.preMaxBounds = {
      x: win.x,
      y: win.y,
      width: win.width,
      height: win.height,
    };
    win.x = 0;
    win.y = 0;
    win.width = this.renderer.canvasWidth;
    win.height = this.renderer.canvasHeight - this.taskbarHeight;
    win.isMaximized = true;
    win.canvas.width = win.width;
    win.canvas.height = Math.max(0, win.height - TITLEBAR_HEIGHT);
  }

  restoreWindow(handle: number): void {
    const win = this.windows.get(handle);
    if (!win) return;

    if (win.isMinimized) {
      win.isMinimized = false;
      this.focusWindow(handle);
      return;
    }

    if (win.isMaximized && win.preMaxBounds) {
      win.x = win.preMaxBounds.x;
      win.y = win.preMaxBounds.y;
      win.width = win.preMaxBounds.width;
      win.height = win.preMaxBounds.height;
      win.isMaximized = false;
      win.canvas.width = win.width;
      win.canvas.height = Math.max(0, win.height - TITLEBAR_HEIGHT);
    }
  }

  focusWindow(handle: number): void {
    const win = this.windows.get(handle);
    if (!win) return;

    // Deactivate all
    for (const w of this.windows.values()) {
      w.isActive = false;
    }

    win.isActive = true;
    win.zOrder = this.nextZOrder++;
    if (win.isMinimized) {
      win.isMinimized = false;
    }
  }

  getWindow(handle: number): WindowState | undefined {
    return this.windows.get(handle);
  }

  getWindowCanvas(handle: number): HTMLCanvasElement | undefined {
    return this.windows.get(handle)?.canvas;
  }

  getAllWindows(): WindowState[] {
    return Array.from(this.windows.values());
  }

  getVisibleWindows(): WindowState[] {
    return Array.from(this.windows.values())
      .filter(w => !w.isMinimized)
      .sort((a, b) => a.zOrder - b.zOrder);
  }

  getActiveWindow(): WindowState | null {
    for (const win of this.windows.values()) {
      if (win.isActive && !win.isMinimized) return win;
    }
    return null;
  }

  getMinimizedWindows(): WindowState[] {
    return Array.from(this.windows.values()).filter(w => w.isMinimized);
  }

  setTaskbarHeight(h: number): void {
    this.taskbarHeight = h;
  }

  // ── Mouse handling ────────────────────────────────────────────

  handleMouseDown(x: number, y: number, _button: number): boolean {
    // Check windows from top to bottom (reverse z-order)
    const visible = this.getVisibleWindows().reverse();

    for (const win of visible) {
      // Check titlebar buttons first
      const btnResult = this.hitTestTitlebarButtons(x, y, win);
      if (btnResult) {
        return true; // Will be handled in mouseUp
      }

      // Check if clicking inside window
      if (this.isInsideWindow(x, y, win)) {
        this.focusWindow(win.handle);

        // Check if clicking title bar (for drag)
        if (y >= win.y && y < win.y + TITLEBAR_HEIGHT) {
          // Start dragging
          this.dragState = {
            handle: win.handle,
            offsetX: x - win.x,
            offsetY: y - win.y,
            isResizing: false,
            resizeDir: null,
            startX: x,
            startY: y,
            startWinX: win.x,
            startWinY: win.y,
            startWinW: win.width,
            startWinH: win.height,
          };

          // Double-click titlebar to maximize
          return true;
        }

        // Check resize handles
        const resizeDir = this.hitTestResizeHandle(x, y, win);
        if (resizeDir && !win.isMaximized) {
          this.dragState = {
            handle: win.handle,
            offsetX: 0,
            offsetY: 0,
            isResizing: true,
            resizeDir,
            startX: x,
            startY: y,
            startWinX: win.x,
            startWinY: win.y,
            startWinW: win.width,
            startWinH: win.height,
          };
          return true;
        }

        return true;
      }
    }

    return false;
  }

  handleMouseMove(x: number, y: number): void {
    if (!this.dragState) {
      // Update cursor based on resize handle hover
      this.updateCursorForPosition(x, y);

      // Update titlebar button hover
      this.hoverButton = null;
      const visible = this.getVisibleWindows().reverse();
      for (const win of visible) {
        const btn = this.hitTestTitlebarButtons(x, y, win);
        if (btn) {
          this.hoverButton = { handle: win.handle, button: btn };
          break;
        }
      }
      return;
    }

    const win = this.windows.get(this.dragState.handle);
    if (!win) {
      this.dragState = null;
      return;
    }

    if (this.dragState.isResizing && this.dragState.resizeDir) {
      this.performResize(win, x, y);
    } else {
      // Dragging
      if (!win.isMaximized) {
        win.x = x - this.dragState.offsetX;
        win.y = y - this.dragState.offsetY;

        // Snap to edges
        const snapThreshold = 10;
        if (win.x < snapThreshold) win.x = 0;
        if (win.y < snapThreshold) win.y = 0;
        if (win.x + win.width > this.renderer.canvasWidth - snapThreshold) {
          win.x = this.renderer.canvasWidth - win.width;
        }
        if (win.y + win.height > this.renderer.canvasHeight - this.taskbarHeight - snapThreshold) {
          win.y = this.renderer.canvasHeight - this.taskbarHeight - win.height;
        }
      }
    }
  }

  handleMouseUp(x: number, y: number, _button: number): boolean {
    if (this.dragState) {
      this.dragState = null;
      return true;
    }

    // Check titlebar button clicks
    const visible = this.getVisibleWindows().reverse();
    for (const win of visible) {
      const btn = this.hitTestTitlebarButtons(x, y, win);
      if (btn) {
        switch (btn) {
          case 'close':
            this.destroyWindow(win.handle);
            break;
          case 'min':
            this.minimizeWindow(win.handle);
            break;
          case 'max':
            this.maximizeWindow(win.handle);
            break;
        }
        return true;
      }
    }

    return false;
  }

  handleDoubleClick(x: number, y: number): boolean {
    const visible = this.getVisibleWindows().reverse();
    for (const win of visible) {
      if (
        x >= win.x && x < win.x + win.width &&
        y >= win.y && y < win.y + TITLEBAR_HEIGHT
      ) {
        this.maximizeWindow(win.handle);
        return true;
      }
    }
    return false;
  }

  // ── Rendering ─────────────────────────────────────────────────

  renderAllWindows(ctx: CanvasRenderingContext2D): void {
    const visible = this.getVisibleWindows();

    for (const win of visible) {
      this.renderWindow(ctx, win);
    }
  }

  private renderWindow(ctx: CanvasRenderingContext2D, win: WindowState): void {
    if (win.isMinimized) return;

    const { x, y, width, height, isActive } = win;

    // Shadow
    ctx.save();
    ctx.shadowColor = 'rgba(0, 0, 0, 0.4)';
    ctx.shadowOffsetX = SHADOW_OFFSET;
    ctx.shadowOffsetY = SHADOW_OFFSET;
    ctx.shadowBlur = SHADOW_BLUR;
    ctx.fillStyle = COLORS.mantle;
    this.roundRect(ctx, x, y, width, height, BORDER_RADIUS);
    ctx.fill();
    ctx.restore();

    // Window body
    ctx.save();
    this.roundRect(ctx, x, y, width, height, BORDER_RADIUS);
    ctx.clip();

    // Title bar background
    const titleGrad = ctx.createLinearGradient(x, y, x, y + TITLEBAR_HEIGHT);
    if (isActive) {
      titleGrad.addColorStop(0, COLORS.surface0);
      titleGrad.addColorStop(1, COLORS.mantle);
    } else {
      titleGrad.addColorStop(0, COLORS.surface1);
      titleGrad.addColorStop(1, COLORS.crust);
    }
    ctx.fillStyle = titleGrad;
    ctx.fillRect(x, y, width, TITLEBAR_HEIGHT);

    // Active accent line at top
    if (isActive) {
      ctx.fillStyle = win.titlebarColor;
      ctx.fillRect(x, y, width, 2);
    }

    // Window icon
    ctx.font = '16px sans-serif';
    ctx.fillStyle = COLORS.text;
    ctx.textBaseline = 'middle';
    ctx.fillText(win.icon, x + 10, y + TITLEBAR_HEIGHT / 2);

    // Title text
    ctx.font = `${isActive ? 'bold ' : ''}13px "Segoe UI", sans-serif`;
    ctx.fillStyle = isActive ? COLORS.text : COLORS.subtext;
    const titleX = x + 32;
    const titleMaxW = width - 120;
    if (titleMaxW > 0) {
      ctx.fillText(win.title, titleX, y + TITLEBAR_HEIGHT / 2, titleMaxW);
    }

    // Titlebar buttons
    const btnSize = 14;
    const btnY = y + (TITLEBAR_HEIGHT - btnSize) / 2;
    const btnSpacing = 6;
    let btnX = x + width - 14;

    // Close button
    const closeHover = this.hoverButton?.handle === win.handle && this.hoverButton?.button === 'close';
    ctx.fillStyle = closeHover ? COLORS.red : (isActive ? COLORS.overlay0 : COLORS.surface2);
    ctx.beginPath();
    ctx.arc(btnX, btnY + btnSize / 2, btnSize / 2, 0, Math.PI * 2);
    ctx.fill();
    ctx.fillStyle = isActive ? COLORS.crust : COLORS.mantle;
    ctx.font = 'bold 11px sans-serif';
    ctx.textAlign = 'center';
    ctx.fillText('✕', btnX, btnY + btnSize / 2 + 1);
    ctx.textAlign = 'start';

    btnX -= btnSize + btnSpacing;

    // Maximize button
    const maxHover = this.hoverButton?.handle === win.handle && this.hoverButton?.button === 'max';
    ctx.fillStyle = maxHover ? COLORS.green : (isActive ? COLORS.overlay0 : COLORS.surface2);
    ctx.beginPath();
    ctx.arc(btnX, btnY + btnSize / 2, btnSize / 2, 0, Math.PI * 2);
    ctx.fill();
    ctx.fillStyle = isActive ? COLORS.crust : COLORS.mantle;
    ctx.font = 'bold 11px sans-serif';
    ctx.textAlign = 'center';
    ctx.fillText(win.isMaximized ? '⧉' : '□', btnX, btnY + btnSize / 2 + 1);
    ctx.textAlign = 'start';

    btnX -= btnSize + btnSpacing;

    // Minimize button
    const minHover = this.hoverButton?.handle === win.handle && this.hoverButton?.button === 'min';
    ctx.fillStyle = minHover ? COLORS.yellow : (isActive ? COLORS.overlay0 : COLORS.surface2);
    ctx.beginPath();
    ctx.arc(btnX, btnY + btnSize / 2, btnSize / 2, 0, Math.PI * 2);
    ctx.fill();
    ctx.fillStyle = isActive ? COLORS.crust : COLORS.mantle;
    ctx.font = 'bold 11px sans-serif';
    ctx.textAlign = 'center';
    ctx.fillText('−', btnX, btnY + btnSize / 2 + 1);
    ctx.textAlign = 'start';

    // Content area
    ctx.fillStyle = COLORS.base;
    ctx.fillRect(x, y + TITLEBAR_HEIGHT, width, height - TITLEBAR_HEIGHT);

    // Draw the window's own canvas content
    if (win.canvas.width > 0 && win.canvas.height > 0) {
      ctx.drawImage(win.canvas, x, y + TITLEBAR_HEIGHT, width, height - TITLEBAR_HEIGHT);
    }

    // Border
    ctx.strokeStyle = isActive ? COLORS.surface0 : COLORS.surface1;
    ctx.lineWidth = 1;
    this.roundRect(ctx, x, y, width, height, BORDER_RADIUS);
    ctx.stroke();

    ctx.restore();
  }

  // ── Helpers ───────────────────────────────────────────────────

  private isInsideWindow(x: number, y: number, win: WindowState): boolean {
    return (
      x >= win.x &&
      x < win.x + win.width &&
      y >= win.y &&
      y < win.y + win.height
    );
  }

  private hitTestTitlebarButtons(
    x: number,
    y: number,
    win: WindowState,
  ): 'min' | 'max' | 'close' | null {
    if (y < win.y || y >= win.y + TITLEBAR_HEIGHT) return null;
    if (x < win.x + win.width - 90 || x >= win.x + win.width) return null;

    const btnSize = 14;
    const btnSpacing = 6;
    const btnY = win.y + (TITLEBAR_HEIGHT - btnSize) / 2;
    const btnCenterY = btnY + btnSize / 2;

    let btnX = win.x + win.width - 14;
    if (this.distance(x, y, btnX, btnCenterY) <= btnSize / 2 + 2) return 'close';

    btnX -= btnSize + btnSpacing;
    if (this.distance(x, y, btnX, btnCenterY) <= btnSize / 2 + 2) return 'max';

    btnX -= btnSize + btnSpacing;
    if (this.distance(x, y, btnX, btnCenterY) <= btnSize / 2 + 2) return 'min';

    return null;
  }

  private hitTestResizeHandle(x: number, y: number, win: WindowState): ResizeDirection {
    const { x: wx, y: wy, width: ww, height: wh } = win;
    const s = RESIZE_HANDLE_SIZE;

    // Check corners first (they take priority)
    if (x >= wx - s && x < wx + s && y >= wy - s && y < wy + s) return 'nw';
    if (x >= wx + ww - s && x < wx + ww + s && y >= wy - s && y < wy + s) return 'ne';
    if (x >= wx + ww - s && x < wx + ww + s && y >= wy + wh - s && y < wy + wh + s) return 'se';
    if (x >= wx - s && x < wx + s && y >= wy + wh - s && y < wy + wh + s) return 'sw';

    // Check edges
    if (y >= wy - s && y < wy + s && x >= wx && x < wx + ww) return 'n';
    if (x >= wx + ww - s && x < wx + ww + s && y >= wy && y < wy + wh) return 'e';
    if (y >= wy + wh - s && y < wy + wh + s && x >= wx && x < wx + ww) return 's';
    if (x >= wx - s && x < wx + s && y >= wy && y < wy + wh) return 'w';

    return null;
  }

  private performResize(win: WindowState, x: number, y: number): void {
    if (!this.dragState) return;

    const dx = x - this.dragState.startX;
    const dy = y - this.dragState.startY;
    const dir = this.dragState.resizeDir!;

    let newX = this.dragState.startWinX;
    let newY = this.dragState.startWinY;
    let newW = this.dragState.startWinW;
    let newH = this.dragState.startWinH;

    if (dir.includes('e')) newW = this.dragState.startWinW + dx;
    if (dir.includes('w')) {
      newW = this.dragState.startWinW - dx;
      newX = this.dragState.startWinX + dx;
    }
    if (dir.includes('s')) newH = this.dragState.startWinH + dy;
    if (dir.includes('n')) {
      newH = this.dragState.startWinH - dy;
      newY = this.dragState.startWinY + dy;
    }

    // Enforce minimums
    if (newW < win.minWidth) {
      if (dir.includes('w')) newX = this.dragState.startWinX + this.dragState.startWinW - win.minWidth;
      newW = win.minWidth;
    }
    if (newH < win.minHeight) {
      if (dir.includes('n')) newY = this.dragState.startWinY + this.dragState.startWinH - win.minHeight;
      newH = win.minHeight;
    }

    win.x = newX;
    win.y = newY;
    win.width = newW;
    win.height = newH;
    win.canvas.width = newW;
    win.canvas.height = Math.max(0, newH - TITLEBAR_HEIGHT);
  }

  private updateCursorForPosition(x: number, y: number): void {
    const visible = this.getVisibleWindows().reverse();
    for (const win of visible) {
      const dir = this.hitTestResizeHandle(x, y, win);
      if (dir && !win.isMaximized) {
        const cursorMap: Record<string, string> = {
          n: 'ns-resize',
          ne: 'nesw-resize',
          e: 'ew-resize',
          se: 'nwse-resize',
          s: 'ns-resize',
          sw: 'nesw-resize',
          w: 'ew-resize',
          nw: 'nwse-resize',
        };
        this.renderer.canvas.style.cursor = cursorMap[dir] ?? 'default';
        return;
      }

      // Title bar hover
      if (
        x >= win.x && x < win.x + win.width &&
        y >= win.y && y < win.y + TITLEBAR_HEIGHT &&
        !win.isMaximized
      ) {
        this.renderer.canvas.style.cursor = 'grab';
        return;
      }
    }
    this.renderer.canvas.style.cursor = 'default';
  }

  private activateTopmost(): void {
    const visible = this.getVisibleWindows();
    if (visible.length > 0) {
      const top = visible[visible.length - 1];
      top.isActive = true;
    }
  }

  private distance(x1: number, y1: number, x2: number, y2: number): number {
    return Math.sqrt((x2 - x1) ** 2 + (y2 - y1) ** 2);
  }

  private roundRect(
    ctx: CanvasRenderingContext2D,
    x: number,
    y: number,
    w: number,
    h: number,
    r: number,
  ): void {
    ctx.beginPath();
    ctx.moveTo(x + r, y);
    ctx.lineTo(x + w - r, y);
    ctx.quadraticCurveTo(x + w, y, x + w, y + r);
    ctx.lineTo(x + w, y + h - r);
    ctx.quadraticCurveTo(x + w, y + h, x + w - r, y + h);
    ctx.lineTo(x + r, y + h);
    ctx.quadraticCurveTo(x, y + h, x, y + h - r);
    ctx.lineTo(x, y + r);
    ctx.quadraticCurveTo(x, y, x + r, y);
    ctx.closePath();
  }

  get titlebarHeight(): number {
    return TITLEBAR_HEIGHT;
  }
}
