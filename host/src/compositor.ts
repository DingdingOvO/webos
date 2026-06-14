/**
 * WebOS Desktop Compositor
 * 
 * Renders the entire desktop using WebGPU. This is the single source of
 * all visual output — wallpaper, windows, taskbar, menus, notifications.
 * 
 * The browser sees only one <canvas>. F12 shows nothing but pixels.
 * 
 * Design language: Frosted Glass
 * - Blue accent (#3B82F6)
 * - Backgrounds are high-blur + micro-transparent
 * - Perlin noise texture on background surfaces (2-3% opacity)
 * - No noise on text, icons, buttons, or content areas
 * 
 * Version: 0.0.1beta
 */

import { DESIGN, WindowState } from './types';

// ─── WGSL Shaders ───

const BLIT_SHADER = /* wgsl */`
struct VSOut {
  @builtin(position) pos: vec4f,
  @location(0) uv: vec2f,
}

@vertex
fn vs(@builtin(vertex_index) idx: u32) -> VSOut {
  // Full-screen triangle
  var pos = array<vec2f, 3>(
    vec2f(-1.0, -1.0),
    vec2f( 3.0, -1.0),
    vec2f(-1.0,  3.0),
  );
  var uv = array<vec2f, 3>(
    vec2f(0.0, 1.0),
    vec2f(2.0, 1.0),
    vec2f(0.0, -1.0),
  );
  var out: VSOut;
  out.pos = vec4f(pos[idx], 0.0, 1.0);
  out.uv = uv[idx];
  return out;
}

@group(0) @binding(0) var<uniform> color: vec4f;
@group(0) @binding(1) var texSampler: sampler;
@group(0) @binding(2) var tex: texture_2d<f32>;
@group(0) @binding(3) var<uniform> hasTexture: u32;

@fragment
fn fs(in: VSOut) -> @location(0) vec4f {
  if (hasTexture == 1u) {
    return textureSample(tex, texSampler, in.uv);
  }
  return color;
}
`;

const BLUR_SHADER = /* wgsl */`
struct VSOut {
  @builtin(position) pos: vec4f,
  @location(0) uv: vec2f,
}

@vertex
fn vs(@builtin(vertex_index) idx: u32) -> VSOut {
  var pos = array<vec2f, 3>(
    vec2f(-1.0, -1.0),
    vec2f( 3.0, -1.0),
    vec2f(-1.0,  3.0),
  );
  var uv = array<vec2f, 3>(
    vec2f(0.0, 1.0),
    vec2f(2.0, 1.0),
    vec2f(0.0, -1.0),
  );
  var out: VSOut;
  out.pos = vec4f(pos[idx], 0.0, 1.0);
  out.uv = uv[idx];
  return out;
}

@group(0) @binding(0) var src: texture_2d<f32>;
@group(0) @binding(1) var srcSampler: sampler;
@group(0) @binding(2) var<uniform> direction: vec2f;

@fragment
fn fs(in: VSOut) -> @location(0) vec4f {
  // Kawase blur — 5 taps per pass, two passes (horizontal + vertical)
  let texSize = vec2f(textureDimensions(src));
  let pixel = 1.0 / texSize;
  let blur = direction * pixel * 2.0;

  var color = textureSample(src, srcSampler, in.uv) * 0.25;
  color += textureSample(src, srcSampler, in.uv + blur) * 0.1875;
  color += textureSample(src, srcSampler, in.uv - blur) * 0.1875;
  color += textureSample(src, srcSampler, in.uv + blur * 2.0) * 0.0625;
  color += textureSample(src, srcSampler, in.uv - blur * 2.0) * 0.0625;

  return color;
}
`;

const NOISE_SHADER = /* wgsl */`
struct VSOut {
  @builtin(position) pos: vec4f,
  @location(0) uv: vec2f,
}

@vertex
fn vs(@builtin(vertex_index) idx: u32) -> VSOut {
  var pos = array<vec2f, 3>(
    vec2f(-1.0, -1.0),
    vec2f( 3.0, -1.0),
    vec2f(-1.0,  3.0),
  );
  var uv = array<vec2f, 3>(
    vec2f(0.0, 1.0),
    vec2f(2.0, 1.0),
    vec2f(0.0, -1.0),
  );
  var out: VSOut;
  out.pos = vec4f(pos[idx], 0.0, 1.0);
  out.uv = uv[idx];
  return out;
}

@group(0) @binding(0) var noiseTex: texture_2d<f32>;
@group(0) @binding(1) var noiseSampler: sampler;
@group(0) @binding(2) var<uniform> params: vec4f; // x=opacity, y=scale, z=0, w=0

@fragment
fn fs(in: VSOut) -> @location(0) vec4f {
  let noise = textureSample(noiseTex, noiseSampler, in.uv * params.y).r;
  let noise_color = mix(0.0, 1.0, noise);
  return vec4f(noise_color, noise_color, noise_color, params.x);
}
`;

// ─── Compositor ───

export class DesktopCompositor {
  private canvas: HTMLCanvasElement;
  private device: GPUDevice;
  private context: GPUCanvasContext;
  private format: GPUTextureFormat;
  private width: number;
  private height: number;

  // Pipelines
  private blitPipeline!: GPURenderPipeline;
  private blurPipeline!: GPURenderPipeline;
  private noisePipeline!: GPURenderPipeline;

  // Bind group layouts
  private blitLayout!: GPUBindGroupLayout;
  private blurLayout!: GPUBindGroupLayout;
  private noiseLayout!: GPUBindGroupLayout;

  // Textures
  private wallpaperTexture!: GPUTexture;
  private noiseTexture!: GPUTexture;
  private blurTempTexture!: GPUTexture;

  // Samplers
  private sampler!: GPUSampler;

  // Bind groups (recreated on resize)
  private wallpaperBindGroup!: GPUBindGroup;
  private blurHBindGroup!: GPUBindGroup;
  private blurVBindGroup!: GPUBindGroup;
  private noiseBindGroup!: GPUBindGroup;

  // Uniform buffers
  private colorBuffer!: GPUBuffer;
  private hasTexBuffer!: GPUBuffer;
  private directionBuffer!: GPUBuffer;
  private noiseParamsBuffer!: GPUBuffer;

  // Windows
  private windows: WindowState[] = [];
  private nextHandle = 1;

  constructor(canvas: HTMLCanvasElement, device: GPUDevice) {
    this.canvas = canvas;
    this.device = device;
    this.width = canvas.width || window.innerWidth;
    this.height = canvas.height || window.innerHeight;

    // Configure canvas context
    this.context = canvas.getContext('webgpu') as GPUCanvasContext;
    this.format = navigator.gpu.getPreferredCanvasFormat();
    this.context.configure({
      device,
      format: this.format,
      alphaMode: 'premultiplied',
    });

    this.initPipelines();
    this.initBuffers();
    this.initTextures();
    this.initBindGroups();

    // Handle resize
    window.addEventListener('resize', () => this.onResize());
  }

  private initPipelines(): void {
    const { device } = this;

    // Blit pipeline — draws a solid color or texture to full screen
    const blitModule = device.createShaderModule({ code: BLIT_SHADER });
    this.blitLayout = device.createBindGroupLayout({
      entries: [
        { binding: 0, visibility: GPUShaderStage.FRAGMENT, buffer: { type: 'uniform' } },
        { binding: 1, visibility: GPUShaderStage.FRAGMENT, sampler: { type: 'filtering' } },
        { binding: 2, visibility: GPUShaderStage.FRAGMENT, texture: { sampleType: 'float' } },
        { binding: 3, visibility: GPUShaderStage.FRAGMENT, buffer: { type: 'uniform' } },
      ],
    });
    this.blitPipeline = device.createRenderPipeline({
      layout: device.createPipelineLayout({ bindGroupLayouts: [this.blitLayout] }),
      vertex: { module: blitModule, entryPoint: 'vs' },
      fragment: {
        module: blitModule,
        entryPoint: 'fs',
        targets: [{ format: this.format, blend: {
          color: { srcFactor: 'src-alpha', dstFactor: 'one-minus-src-alpha' },
          alpha: { srcFactor: 'one', dstFactor: 'one-minus-src-alpha' },
        }}],
      },
    });

    // Blur pipeline — Kawase two-pass blur
    const blurModule = device.createShaderModule({ code: BLUR_SHADER });
    this.blurLayout = device.createBindGroupLayout({
      entries: [
        { binding: 0, visibility: GPUShaderStage.FRAGMENT, texture: { sampleType: 'float' } },
        { binding: 1, visibility: GPUShaderStage.FRAGMENT, sampler: { type: 'filtering' } },
        { binding: 2, visibility: GPUShaderStage.FRAGMENT, buffer: { type: 'uniform' } },
      ],
    });
    this.blurPipeline = device.createRenderPipeline({
      layout: device.createPipelineLayout({ bindGroupLayouts: [this.blurLayout] }),
      vertex: { module: blurModule, entryPoint: 'vs' },
      fragment: {
        module: blurModule,
        entryPoint: 'fs',
        targets: [{ format: this.format }],
      },
    });

    // Noise overlay pipeline
    const noiseModule = device.createShaderModule({ code: NOISE_SHADER });
    this.noiseLayout = device.createBindGroupLayout({
      entries: [
        { binding: 0, visibility: GPUShaderStage.FRAGMENT, texture: { sampleType: 'float' } },
        { binding: 1, visibility: GPUShaderStage.FRAGMENT, sampler: { type: 'filtering' } },
        { binding: 2, visibility: GPUShaderStage.FRAGMENT, buffer: { type: 'uniform' } },
      ],
    });
    this.noisePipeline = device.createRenderPipeline({
      layout: device.createPipelineLayout({ bindGroupLayouts: [this.noiseLayout] }),
      vertex: { module: noiseModule, entryPoint: 'vs' },
      fragment: {
        module: noiseModule,
        entryPoint: 'fs',
        targets: [{ format: this.format, blend: {
          color: { srcFactor: 'src-alpha', dstFactor: 'one-minus-src-alpha' },
          alpha: { srcFactor: 'one', dstFactor: 'one-minus-src-alpha' },
        }}],
      },
    });

    // Sampler
    this.sampler = device.createSampler({
      magFilter: 'linear',
      minFilter: 'linear',
    });
  }

  private initBuffers(): void {
    const { device } = this;

    this.colorBuffer = device.createBuffer({
      size: 16, // vec4f
      usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST,
    });

    this.hasTexBuffer = device.createBuffer({
      size: 4, // u32
      usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST,
    });

    this.directionBuffer = device.createBuffer({
      size: 16, // vec2f padded to vec4f
      usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST,
    });

    this.noiseParamsBuffer = device.createBuffer({
      size: 16, // vec4f
      usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST,
    });
  }

  private initTextures(): void {
    const { device, width, height } = this;

    // Wallpaper texture (gradient for now)
    this.wallpaperTexture = device.createTexture({
      size: [width, height],
      format: 'rgba8unorm',
      usage: GPUTextureUsage.TEXTURE_BINDING | GPUTextureUsage.RENDER_ATTACHMENT | GPUTextureUsage.COPY_DST,
    });
    this.generateWallpaper();

    // Noise texture — 256x256 Perlin noise, generated on CPU
    this.noiseTexture = device.createTexture({
      size: [256, 256],
      format: 'r8unorm',
      usage: GPUTextureUsage.TEXTURE_BINDING | GPUTextureUsage.COPY_DST,
    });
    this.generateNoiseTexture();

    // Blur temporary texture
    this.blurTempTexture = device.createTexture({
      size: [width, height],
      format: this.format,
      usage: GPUTextureUsage.TEXTURE_BINDING | GPUTextureUsage.RENDER_ATTACHMENT,
    });
  }

  private initBindGroups(): void {
    const { device } = this;

    // Wallpaper blit
    this.wallpaperBindGroup = device.createBindGroup({
      layout: this.blitLayout,
      entries: [
        { binding: 0, resource: { buffer: this.colorBuffer } },
        { binding: 1, resource: this.sampler },
        { binding: 2, resource: this.wallpaperTexture.createView() },
        { binding: 3, resource: { buffer: this.hasTexBuffer } },
      ],
    });

    // Blur passes
    this.blurHBindGroup = device.createBindGroup({
      layout: this.blurLayout,
      entries: [
        { binding: 0, resource: this.wallpaperTexture.createView() },
        { binding: 1, resource: this.sampler },
        { binding: 2, resource: { buffer: this.directionBuffer } },
      ],
    });
    this.blurVBindGroup = device.createBindGroup({
      layout: this.blurLayout,
      entries: [
        { binding: 0, resource: this.blurTempTexture.createView() },
        { binding: 1, resource: this.sampler },
        { binding: 2, resource: { buffer: this.directionBuffer } },
      ],
    });

    // Noise overlay
    this.noiseBindGroup = device.createBindGroup({
      layout: this.noiseLayout,
      entries: [
        { binding: 0, resource: this.noiseTexture.createView() },
        { binding: 1, resource: this.sampler },
        { binding: 2, resource: { buffer: this.noiseParamsBuffer } },
      ],
    });
  }

  // ─── Wallpaper Generation ───

  private generateWallpaper(): void {
    const { device, width, height } = this;
    // Deep blue gradient: #0F172A → #1E293B → #0F172A
    const data = new Uint8Array(width * height * 4);
    for (let y = 0; y < height; y++) {
      const t = y / height;
      const t2 = t < 0.5 ? t * 2 : (1 - t) * 2;
      const r = Math.round(15 + t2 * (30 - 15));
      const g = Math.round(23 + t2 * (41 - 23));
      const b = Math.round(42 + t2 * (59 - 42));
      for (let x = 0; x < width; x++) {
        const i = (y * width + x) * 4;
        data[i] = r;
        data[i + 1] = g;
        data[i + 2] = b;
        data[i + 3] = 255;
      }
    }
    device.queue.writeTexture(
      { texture: this.wallpaperTexture },
      data,
      { bytesPerRow: width * 4 },
      { width, height },
    );
  }

  // ─── Noise Texture Generation (Perlin-like) ───

  private generateNoiseTexture(): void {
    const size = 256;
    const data = new Uint8Array(size * size);
    // Simple value noise with interpolation — gives organic texture, not random dots
    const grid = 16;
    const gridValues: number[][] = [];
    for (let gy = 0; gy <= grid; gy++) {
      gridValues[gy] = [];
      for (let gx = 0; gx <= grid; gx++) {
        gridValues[gy][gx] = Math.random();
      }
    }
    for (let y = 0; y < size; y++) {
      for (let x = 0; x < size; x++) {
        const gx = (x / size) * grid;
        const gy = (y / size) * grid;
        const ix = Math.floor(gx);
        const iy = Math.floor(gy);
        const fx = gx - ix;
        const fy = gy - iy;
        // Smooth interpolation
        const sx = fx * fx * (3 - 2 * fx);
        const sy = fy * fy * (3 - 2 * fy);
        const v00 = gridValues[iy][ix];
        const v10 = gridValues[iy][ix + 1];
        const v01 = gridValues[iy + 1][ix];
        const v11 = gridValues[iy + 1][ix + 1];
        const top = v00 + (v10 - v00) * sx;
        const bot = v01 + (v11 - v01) * sx;
        const val = top + (bot - top) * sy;
        data[y * size + x] = Math.round(val * 255);
      }
    }
    this.device.queue.writeTexture(
      { texture: this.noiseTexture },
      data,
      { bytesPerRow: size },
      { width: size, height: size },
    );
  }

  // ─── Window Management ───

  createWindow(title: string, x: number, y: number, w: number, h: number, ownerPid: number): WindowState {
    const win: WindowState = {
      handle: this.nextHandle++,
      title,
      x, y,
      width: w, height: h,
      focused: true,
      minimized: false,
      maximized: false,
      ownerPid,
    };
    // Defocus other windows
    this.windows.forEach(w => w.focused = false);
    this.windows.push(win);
    return win;
  }

  destroyWindow(handle: number): void {
    this.windows = this.windows.filter(w => w.handle !== handle);
  }

  focusWindow(handle: number): void {
    this.windows.forEach(w => w.focused = w.handle === handle);
    // Move to top of Z-order
    const idx = this.windows.findIndex(w => w.handle === handle);
    if (idx >= 0) {
      const [win] = this.windows.splice(idx, 1);
      this.windows.push(win);
    }
  }

  // ─── Main Render ───

  render(time: number): void {
    const { device, context, width, height } = this;
    const commandEncoder = device.createCommandEncoder();

    // ── 1. Draw wallpaper (L0) ──
    const wallpaperPass = commandEncoder.beginRenderPass({
      colorAttachments: [{
        view: context.getCurrentTexture().createView(),
        clearValue: { r: 0.059, g: 0.090, b: 0.165, a: 1.0 },
        loadOp: 'clear',
        storeOp: 'store',
      }],
    });
    device.queue.writeBuffer(this.hasTexBuffer, 0, new Uint32Array([1]));
    wallpaperPass.setPipeline(this.blitPipeline);
    wallpaperPass.setBindGroup(0, this.wallpaperBindGroup);
    wallpaperPass.draw(3);
    wallpaperPass.end();

    // ── 2. Draw windows (L2-L3) ──
    for (const win of this.windows) {
      if (win.minimized) continue;
      this.renderWindow(commandEncoder, win, time);
    }

    // ── 3. Draw taskbar (L4) ──
    this.renderTaskbar(commandEncoder, time);

    device.queue.submit([commandEncoder.finish()]);
  }

  private renderWindow(encoder: GPUCommandEncoder, win: WindowState, time: number): void {
    const { device, context } = this;

    // Window background — blurred, semi-transparent with noise
    const bg = DESIGN.color.surfaceElevated;
    device.queue.writeBuffer(this.colorBuffer, 0, new Float32Array(bg));

    const surfacePass = encoder.beginRenderPass({
      colorAttachments: [{
        view: context.getCurrentTexture().createView(),
        clearValue: { r: 0, g: 0, b: 0, a: 0 },
        loadOp: 'load',
        storeOp: 'store',
      }],
    });

    // Draw window background (semi-transparent)
    device.queue.writeBuffer(this.hasTexBuffer, 0, new Uint32Array([0]));
    surfacePass.setPipeline(this.blitPipeline);

    // Use a simple colored rect for now (blur requires more setup)
    const bgBindGroup = device.createBindGroup({
      layout: this.blitLayout,
      entries: [
        { binding: 0, resource: { buffer: this.colorBuffer } },
        { binding: 1, resource: this.sampler },
        { binding: 2, resource: this.wallpaperTexture.createView() },
        { binding: 3, resource: { buffer: this.hasTexBuffer } },
      ],
    });
    surfacePass.setBindGroup(0, bgBindGroup);
    surfacePass.draw(3);

    // Draw noise overlay on window background
    device.queue.writeBuffer(this.noiseParamsBuffer, 0, new Float32Array([
      DESIGN.noise.opacity, DESIGN.noise.scale, 0, 0
    ]));
    surfacePass.setPipeline(this.noisePipeline);
    surfacePass.setBindGroup(0, this.noiseBindGroup);
    surfacePass.draw(3);

    surfacePass.end();
  }

  private renderTaskbar(encoder: GPUCommandEncoder, time: number): void {
    const { device, context, width, height } = this;
    const taskbarY = height - DESIGN.taskbar.height;

    // Taskbar background — blurred, semi-transparent with noise
    const bg = [DESIGN.color.surfaceElevated[0], DESIGN.color.surfaceElevated[1], DESIGN.color.surfaceElevated[2], 0.78];
    device.queue.writeBuffer(this.colorBuffer, 0, new Float32Array(bg));

    const pass = encoder.beginRenderPass({
      colorAttachments: [{
        view: context.getCurrentTexture().createView(),
        clearValue: { r: 0, g: 0, b: 0, a: 0 },
        loadOp: 'load',
        storeOp: 'store',
      }],
    });

    device.queue.writeBuffer(this.hasTexBuffer, 0, new Uint32Array([0]));
    pass.setPipeline(this.blitPipeline);

    const taskbarBindGroup = device.createBindGroup({
      layout: this.blitLayout,
      entries: [
        { binding: 0, resource: { buffer: this.colorBuffer } },
        { binding: 1, resource: this.sampler },
        { binding: 2, resource: this.wallpaperTexture.createView() },
        { binding: 3, resource: { buffer: this.hasTexBuffer } },
      ],
    });
    pass.setBindGroup(0, taskbarBindGroup);
    pass.draw(3);

    // Noise on taskbar
    pass.setPipeline(this.noisePipeline);
    pass.setBindGroup(0, this.noiseBindGroup);
    pass.draw(3);

    pass.end();
  }

  // ─── Resize ───

  private onResize(): void {
    this.width = window.innerWidth;
    this.height = window.innerHeight;
    this.canvas.width = this.width;
    this.canvas.height = this.height;

    this.context.configure({
      device: this.device,
      format: this.format,
      alphaMode: 'premultiplied',
    });

    // Recreate size-dependent textures
    this.wallpaperTexture.destroy();
    this.wallpaperTexture = this.device.createTexture({
      size: [this.width, this.height],
      format: 'rgba8unorm',
      usage: GPUTextureUsage.TEXTURE_BINDING | GPUTextureUsage.RENDER_ATTACHMENT | GPUTextureUsage.COPY_DST,
    });
    this.generateWallpaper();

    this.blurTempTexture.destroy();
    this.blurTempTexture = this.device.createTexture({
      size: [this.width, this.height],
      format: this.format,
      usage: GPUTextureUsage.TEXTURE_BINDING | GPUTextureUsage.RENDER_ATTACHMENT,
    });

    this.initBindGroups();
  }
}
