/**
 * WebOS Host - Main Entry Point
 * 
 * The launcher initializes WebGPU, loads C bootloader/kernel as WASM,
 * and starts the desktop environment. Everything renders to a single
 * full-screen canvas — no DOM elements for UI.
 * 
 * Boot chain: TS Launcher → C Bootloader → C Kernel → Login → Desktop
 * 
 * Version: 0.0.1beta
 */

import { RuntimeBridge } from './runtime_bridge';
import { DynamicLoader } from './dynamic_loader';
import { DesktopCompositor } from './compositor';
import { DEFAULT_CONFIG, RuntimeConfig, BootPhase, DESIGN } from './types';

class WebOSHost {
  private config: RuntimeConfig;
  private bridge: RuntimeBridge;
  private loader: DynamicLoader;
  private compositor: DesktopCompositor | null = null;
  private canvas: HTMLCanvasElement | null = null;
  private bootOverlay: HTMLDivElement | null = null;
  private phase: BootPhase = BootPhase.INIT;

  constructor(config: Partial<RuntimeConfig> = {}) {
    this.config = { ...DEFAULT_CONFIG, ...config };
    this.bridge = new RuntimeBridge(this.config);
    this.loader = new DynamicLoader(this.config.wasmPath);
  }

  async boot(): Promise<void> {
    this.canvas = document.getElementById(this.config.canvasId) as HTMLCanvasElement;
    if (!this.canvas) {
      throw new Error('Canvas element not found');
    }
    this.bootOverlay = document.getElementById('boot-overlay') as HTMLDivElement;

    try {
      // Phase 1: Initialize WebGPU
      this.setPhase(BootPhase.INIT);
      this.log('Initializing WebGPU...');
      await this.initGPU();

      // Phase 2: Load bootloader
      this.setPhase(BootPhase.BOOTLOADER);
      this.log('Loading bootloader...');
      await this.loader.loadModule('bootloader.wasm');

      // Phase 3: Load kernel
      this.setPhase(BootPhase.KERNEL);
      this.log('Loading kernel...');
      const kernelModule = await this.loader.loadModule('kernel.wasm');
      const imports = this.bridge.getWasmImports();
      const kernelInstance = await this.loader.instantiateModule('kernel.wasm', imports);

      // Call kernel entry
      const entry = kernelInstance.exports.main || kernelInstance.exports._start;
      if (entry) {
        (entry as Function)();
      }
      this.log('Kernel started');

      // Phase 4: Load drivers
      this.setPhase(BootPhase.DRIVERS);
      this.log('Loading drivers...');
      await this.loadDrivers();

      // Phase 5: Load services
      this.setPhase(BootPhase.SERVICES);
      this.log('Loading services...');
      await this.loadServices();

      // Phase 6: Login (skip for now, auto-login)
      this.setPhase(BootPhase.LOGIN);
      this.log('Starting login...');

      // Phase 7: Desktop
      this.setPhase(BootPhase.DESKTOP);
      this.log('Starting desktop...');

      // Hide boot overlay
      this.hideBootOverlay();

      // Start render loop
      this.startRenderLoop();

      this.setPhase(BootPhase.COMPLETE);
      this.log('Boot complete');

    } catch (error) {
      this.setPhase(BootPhase.FAILED);
      this.log(`Boot failed: ${error}`);
      console.error('[WebOS] Boot failed:', error);
      this.showBootError(String(error));
    }
  }

  private async initGPU(): Promise<void> {
    if (!navigator.gpu) {
      throw new Error('WebGPU not supported. Please use a WebGPU-compatible browser.');
    }

    const adapter = await navigator.gpu.requestAdapter();
    if (!adapter) {
      throw new Error('No GPU adapter found.');
    }

    const device = await adapter.requestDevice();
    device.lost.then((info) => {
      console.error('[WebOS] GPU device lost:', info.message);
    });

    // Create compositor — it handles all rendering
    this.compositor = new DesktopCompositor(this.canvas!, device);

    this.log('WebGPU initialized');
  }

  private async loadDrivers(): Promise<void> {
    const drivers = [
      'gpu_driver.Wdll',
      'audio_driver.Wdll',
      'input_driver.Wdll',
      'storage_driver.Wdll',
      'network_driver.Wdll',
    ];
    for (const name of drivers) {
      try {
        await this.loader.loadModule(name);
        this.log(`  ${name}: loaded`);
      } catch {
        this.log(`  ${name}: not found (skip)`);
      }
    }
  }

  private async loadServices(): Promise<void> {
    const services = [
      'fs_service.Wdll',
      'audio_server.Wdll',
      'wm.Wdll',
      'net_service.Wdll',
      'appstore.Wdll',
    ];
    for (const name of services) {
      try {
        await this.loader.loadModule(name);
        this.log(`  ${name}: loaded`);
      } catch {
        this.log(`  ${name}: not found (skip)`);
      }
    }
  }

  // ─── Render Loop ───

  private startRenderLoop(): void {
    if (!this.compositor) return;

    const frame = (time: number) => {
      this.compositor!.render(time);
      requestAnimationFrame(frame);
    };
    requestAnimationFrame(frame);
  }

  // ─── Boot UI ───

  private setPhase(phase: BootPhase): void {
    this.phase = phase;
  }

  private log(msg: string): void {
    const timestamp = performance.now().toFixed(0).padStart(6, ' ');
    console.log(`[WebOS ${timestamp}ms] ${msg}`);

    if (this.bootOverlay) {
      const logEl = this.bootOverlay.querySelector('.boot-log');
      if (logEl) {
        logEl.textContent = msg;
      }
    }
  }

  private hideBootOverlay(): void {
    if (this.bootOverlay) {
      this.bootOverlay.style.transition = 'opacity 0.6s ease-out';
      this.bootOverlay.style.opacity = '0';
      setTimeout(() => this.bootOverlay?.remove(), 600);
    }
  }

  private showBootError(error: string): void {
    if (this.bootOverlay) {
      const spinner = this.bootOverlay.querySelector('.boot-spinner');
      if (spinner) (spinner as HTMLElement).style.display = 'none';
      const logEl = this.bootOverlay.querySelector('.boot-log');
      if (logEl) {
        logEl.innerHTML = `<span style="color:#EF4444">Boot Failed</span><br><span style="color:#94A3B8;font-size:13px">${error}</span>`;
      }
    }
  }
}

// ─── Bootstrap ───

window.addEventListener('DOMContentLoaded', () => {
  const host = new WebOSHost();
  host.boot();
});
