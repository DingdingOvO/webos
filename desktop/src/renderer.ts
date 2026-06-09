/**
 * WebOS Desktop Renderer
 * 
 * Uses WebGPU to render the desktop environment including:
 * - Wallpaper background
 * - Window frames and content
 * - Taskbar and start menu
 * - Context menus and notifications
 * 
 * Falls back to Canvas 2D when WebGPU is unavailable.
 */

export class DesktopRenderer {
  private canvas: HTMLCanvasElement;
  private ctx: CanvasRenderingContext2D | null = null;
  private gpuDevice: GPUDevice | null = null;
  private gpuContext: GPUCanvasContext | null = null;
  private useWebGPU: boolean = false;
  private width: number = 0;
  private height: number = 0;

  constructor(canvas: HTMLCanvasElement) {
    this.canvas = canvas;
    this.width = canvas.width;
    this.height = canvas.height;
  }

  async init(): Promise<void> {
    // Try WebGPU first
    if (navigator.gpu) {
      try {
        const adapter = await navigator.gpu.requestAdapter();
        if (adapter) {
          this.gpuDevice = await adapter.requestDevice();
          this.gpuContext = this.canvas.getContext('webgpu') as GPUCanvasContext;
          if (this.gpuContext) {
            const format = navigator.gpu.getPreferredCanvasFormat();
            this.gpuContext.configure({
              device: this.gpuDevice,
              format,
              alphaMode: 'premultiplied',
            });
            this.useWebGPU = true;
            console.log('[DesktopRenderer] WebGPU initialized');
            return;
          }
        }
      } catch (e) {
        console.warn('[DesktopRenderer] WebGPU not available, falling back to Canvas 2D:', e);
      }
    }

    // Fallback to Canvas 2D
    this.ctx = this.canvas.getContext('2d');
    if (!this.ctx) {
      throw new Error('Cannot get canvas 2D context');
    }
    console.log('[DesktopRenderer] Canvas 2D initialized');
  }

  /** Resize the renderer */
  resize(width: number, height: number): void {
    this.width = width;
    this.height = height;
    this.canvas.width = width;
    this.canvas.height = height;
    
    if (this.useWebGPU && this.gpuContext) {
      this.gpuContext.configure({
        device: this.gpuDevice!,
        format: navigator.gpu.getPreferredCanvasFormat(),
        alphaMode: 'premultiplied',
      });
    }
  }

  /** Clear the entire canvas with a color */
  clear(color: string): void {
    if (this.ctx) {
      this.ctx.fillStyle = color;
      this.ctx.fillRect(0, 0, this.width, this.height);
    }
  }

  /** Draw a filled rectangle */
  drawRect(x: number, y: number, w: number, h: number, color: string): void {
    if (this.ctx) {
      this.ctx.fillStyle = color;
      this.ctx.fillRect(x, y, w, h);
    }
  }

  /** Draw a rectangle outline */
  drawRectOutline(x: number, y: number, w: number, h: number, color: string, lineWidth: number = 1): void {
    if (this.ctx) {
      this.ctx.strokeStyle = color;
      this.ctx.lineWidth = lineWidth;
      this.ctx.strokeRect(x, y, w, h);
    }
  }

  /** Draw text */
  drawText(text: string, x: number, y: number, color: string, font: string = '14px "Segoe UI", sans-serif'): void {
    if (this.ctx) {
      this.ctx.font = font;
      this.ctx.fillStyle = color;
      this.ctx.fillText(text, x, y);
    }
  }

  /** Draw an image */
  drawImage(img: HTMLImageElement, x: number, y: number, w: number, h: number): void {
    if (this.ctx) {
      this.ctx.drawImage(img, x, y, w, h);
    }
  }

  /** Draw a rounded rectangle */
  drawRoundRect(x: number, y: number, w: number, h: number, r: number, color: string): void {
    if (this.ctx) {
      this.ctx.beginPath();
      this.ctx.moveTo(x + r, y);
      this.ctx.lineTo(x + w - r, y);
      this.ctx.quadraticCurveTo(x + w, y, x + w, y + r);
      this.ctx.lineTo(x + w, y + h - r);
      this.ctx.quadraticCurveTo(x + w, y + h, x + w - r, y + h);
      this.ctx.lineTo(x + r, y + h);
      this.ctx.quadraticCurveTo(x, y + h, x, y + h - r);
      this.ctx.lineTo(x, y + r);
      this.ctx.quadraticCurveTo(x, y, x + r, y);
      this.ctx.closePath();
      this.ctx.fillStyle = color;
      this.ctx.fill();
    }
  }

  /** Set global alpha for transparency */
  setAlpha(alpha: number): void {
    if (this.ctx) {
      this.ctx.globalAlpha = alpha;
    }
  }

  /** Reset global alpha */
  resetAlpha(): void {
    if (this.ctx) {
      this.ctx.globalAlpha = 1.0;
    }
  }

  get isWebGPU(): boolean { return this.useWebGPU; }
  get canvasWidth(): number { return this.width; }
  get canvasHeight(): number { return this.height; }
}
