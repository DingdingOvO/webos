/**
 * WebOS Notification System
 *
 * Manages desktop notifications: push, dismiss, auto-expire,
 * and rendering of toast popups in the top-right corner.
 * Uses Catppuccin Mocha palette for theming.
 */

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

const NOTIFICATION_WIDTH = 320;
const NOTIFICATION_MIN_HEIGHT = 72;
const NOTIFICATION_PADDING = 12;
const NOTIFICATION_SPACING = 8;
const NOTIFICATION_MARGIN_RIGHT = 16;
const NOTIFICATION_MARGIN_TOP = 12;
const AUTO_DISMISS_MS = 5000;
const SLIDE_ANIMATION_MS = 300;

export interface Notification {
  id: number;
  title: string;
  message: string;
  icon: string;
  type: 'info' | 'warning' | 'error' | 'success';
  timestamp: number;
  read: boolean;
}

interface NotificationInternal extends Notification {
  createdAt: number;
  slideProgress: number; // 0 = hidden, 1 = fully shown
  dismissing: boolean;
  dismissStart: number;
  height: number;
}

export class NotificationSystem {
  private notifications: NotificationInternal[] = [];
  private nextId: number = 1;
  private timers: Map<number, ReturnType<typeof setTimeout>> = new Map();
  private onCountChange?: (unread: number) => void;

  constructor(onCountChange?: (unread: number) => void) {
    this.onCountChange = onCountChange;
  }

  pushNotification(
    title: string,
    message: string,
    icon: string = 'ℹ️',
    type: 'info' | 'warning' | 'error' | 'success' = 'info',
  ): number {
    const id = this.nextId++;
    const now = Date.now();

    const notification: NotificationInternal = {
      id,
      title,
      message,
      icon,
      type,
      timestamp: now,
      read: false,
      createdAt: now,
      slideProgress: 0,
      dismissing: false,
      dismissStart: 0,
      height: NOTIFICATION_MIN_HEIGHT,
    };

    this.notifications.unshift(notification);

    // Auto-dismiss after 5 seconds
    const timer = setTimeout(() => {
      this.dismissNotification(id);
    }, AUTO_DISMISS_MS);
    this.timers.set(id, timer);

    this.notifyCountChange();
    return id;
  }

  dismissNotification(id: number): void {
    const notif = this.notifications.find(n => n.id === id);
    if (notif && !notif.dismissing) {
      notif.dismissing = true;
      notif.dismissStart = Date.now();
      // Clear auto-dismiss timer
      const timer = this.timers.get(id);
      if (timer) {
        clearTimeout(timer);
        this.timers.delete(id);
      }
    }
  }

  clearAll(): void {
    for (const notif of this.notifications) {
      if (!notif.dismissing) {
        notif.dismissing = true;
        notif.dismissStart = Date.now();
      }
      const timer = this.timers.get(notif.id);
      if (timer) {
        clearTimeout(timer);
        this.timers.delete(notif.id);
      }
    }
    this.notifyCountChange();
  }

  markRead(id: number): void {
    const notif = this.notifications.find(n => n.id === id);
    if (notif) {
      notif.read = true;
      this.notifyCountChange();
    }
  }

  markAllRead(): void {
    for (const notif of this.notifications) {
      notif.read = true;
    }
    this.notifyCountChange();
  }

  getUnread(): Notification[] {
    return this.notifications
      .filter(n => !n.read && !n.dismissing)
      .map(n => this.toPublic(n));
  }

  getAll(): Notification[] {
    return this.notifications
      .filter(n => !n.dismissing)
      .map(n => this.toPublic(n));
  }

  get unreadCount(): number {
    return this.notifications.filter(n => !n.read && !n.dismissing).length;
  }

  handleClick(x: number, y: number, canvasWidth: number): boolean {
    const now = Date.now();
    let currentY = NOTIFICATION_MARGIN_TOP;

    for (const notif of this.notifications) {
      if (notif.dismissing) continue;

      const nx = canvasWidth - NOTIFICATION_WIDTH - NOTIFICATION_MARGIN_RIGHT;
      const nh = notif.height;

      if (x >= nx && x < nx + NOTIFICATION_WIDTH && y >= currentY && y < currentY + nh) {
        notif.read = true;
        this.dismissNotification(notif.id);
        this.notifyCountChange();
        return true;
      }

      currentY += nh + NOTIFICATION_SPACING;
    }

    return false;
  }

  // ── Rendering ─────────────────────────────────────────────────

  renderNotifications(ctx: CanvasRenderingContext2D, canvasWidth: number): void {
    const now = Date.now();
    let currentY = NOTIFICATION_MARGIN_TOP;
    const toRemove: number[] = [];

    for (const notif of this.notifications) {
      // Update slide-in animation
      if (!notif.dismissing) {
        const elapsed = now - notif.createdAt;
        notif.slideProgress = Math.min(1, elapsed / SLIDE_ANIMATION_MS);
      } else {
        // Slide-out animation
        const elapsed = now - notif.dismissStart;
        notif.slideProgress = 1 - Math.min(1, elapsed / SLIDE_ANIMATION_MS);
        if (notif.slideProgress <= 0) {
          toRemove.push(notif.id);
          continue;
        }
      }

      const nx = canvasWidth - NOTIFICATION_WIDTH - NOTIFICATION_MARGIN_RIGHT;
      // Slide from right
      const offsetX = (1 - this.easeOutCubic(notif.slideProgress)) * (NOTIFICATION_WIDTH + NOTIFICATION_MARGIN_RIGHT);
      const drawX = nx + offsetX;
      const drawY = currentY;

      this.renderSingleNotification(ctx, notif, drawX, drawY);
      currentY += notif.height + NOTIFICATION_SPACING;
    }

    // Clean up fully dismissed notifications
    for (const id of toRemove) {
      const idx = this.notifications.findIndex(n => n.id === id);
      if (idx !== -1) this.notifications.splice(idx, 1);
    }
  }

  private renderSingleNotification(
    ctx: CanvasRenderingContext2D,
    notif: NotificationInternal,
    x: number,
    y: number,
  ): void {
    const w = NOTIFICATION_WIDTH;
    const h = notif.height;
    const r = 10;

    // Accent color based on type
    const accentColors: Record<string, string> = {
      info: COLORS.blue,
      warning: COLORS.yellow,
      error: COLORS.red,
      success: COLORS.green,
    };
    const accent = accentColors[notif.type] ?? COLORS.blue;

    // Background
    ctx.save();
    ctx.globalAlpha = 0.92 * notif.slideProgress;
    ctx.fillStyle = COLORS.surface0;
    this.roundRect(ctx, x, y, w, h, r);
    ctx.fill();

    // Accent border on left
    ctx.fillStyle = accent;
    ctx.beginPath();
    ctx.moveTo(x, y + r);
    ctx.lineTo(x, y + h - r);
    ctx.quadraticCurveTo(x, y + h, x + r, y + h);
    ctx.lineTo(x + 4, y + h);
    ctx.lineTo(x + 4, y);
    ctx.lineTo(x + r, y);
    ctx.quadraticCurveTo(x, y, x, y + r);
    ctx.closePath();
    ctx.fill();

    // Border
    ctx.strokeStyle = COLORS.surface1;
    ctx.lineWidth = 1;
    this.roundRect(ctx, x, y, w, h, r);
    ctx.stroke();

    // Icon
    ctx.font = '22px sans-serif';
    ctx.textBaseline = 'middle';
    ctx.fillStyle = COLORS.text;
    ctx.fillText(notif.icon, x + NOTIFICATION_PADDING + 4, y + h / 2 - 8);

    // Title
    ctx.font = 'bold 13px "Segoe UI", sans-serif';
    ctx.fillStyle = COLORS.text;
    ctx.fillText(notif.title, x + NOTIFICATION_PADDING + 34, y + NOTIFICATION_PADDING + 10, w - 80);

    // Timestamp
    const timeStr = this.formatTime(notif.timestamp);
    ctx.font = '11px "Segoe UI", sans-serif';
    ctx.fillStyle = COLORS.overlay0;
    ctx.textAlign = 'right';
    ctx.fillText(timeStr, x + w - NOTIFICATION_PADDING, y + NOTIFICATION_PADDING + 10);
    ctx.textAlign = 'start';

    // Message
    ctx.font = '12px "Segoe UI", sans-serif';
    ctx.fillStyle = COLORS.subtext;
    const maxMsgW = w - NOTIFICATION_PADDING * 2 - 34;
    if (maxMsgW > 0) {
      this.wrapText(ctx, notif.message, x + NOTIFICATION_PADDING + 34, y + NOTIFICATION_PADDING + 30, maxMsgW, 16);
    }

    ctx.restore();
  }

  // ── Helpers ───────────────────────────────────────────────────

  private toPublic(n: NotificationInternal): Notification {
    return {
      id: n.id,
      title: n.title,
      message: n.message,
      icon: n.icon,
      type: n.type,
      timestamp: n.timestamp,
      read: n.read,
    };
  }

  private notifyCountChange(): void {
    this.onCountChange?.(this.unreadCount);
  }

  private easeOutCubic(t: number): number {
    return 1 - Math.pow(1 - t, 3);
  }

  private formatTime(ts: number): string {
    const d = new Date(ts);
    return d.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
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

  private wrapText(
    ctx: CanvasRenderingContext2D,
    text: string,
    x: number,
    y: number,
    maxWidth: number,
    lineHeight: number,
  ): void {
    const words = text.split(' ');
    let line = '';
    let curY = y;

    for (const word of words) {
      const testLine = line + word + ' ';
      const metrics = ctx.measureText(testLine);
      if (metrics.width > maxWidth && line !== '') {
        ctx.fillText(line.trim(), x, curY);
        line = word + ' ';
        curY += lineHeight;
      } else {
        line = testLine;
      }
    }
    ctx.fillText(line.trim(), x, curY);
  }
}
