/**
 * WebOS Host - Main Entry Point
 * 
 * Bootstraps the WebOS runtime, loads the bootloader and kernel,
 * and starts the system.
 */

import { RuntimeBridge } from './runtime_bridge';
import { DynamicLoader } from './dynamic_loader';
import { ModuleKind, RuntimeConfig, DEFAULT_CONFIG } from './types';

class WebOSHost {
  private bridge: RuntimeBridge;
  private loader: DynamicLoader;
  private config: RuntimeConfig;
  private bootStatus: HTMLElement | null = null;
  private canvas: HTMLCanvasElement | null = null;

  constructor(config: Partial<RuntimeConfig> = {}) {
    this.config = { ...DEFAULT_CONFIG, ...config };
    this.bridge = new RuntimeBridge(this.config);
    this.loader = new DynamicLoader(this.config.wasmPath);
  }

  /** Initialize and boot the system */
  async boot(): Promise<void> {
    this.bootStatus = document.getElementById('boot-status');
    this.canvas = document.getElementById('webos-canvas') as HTMLCanvasElement;

    this.log('Initializing WebOS runtime...');

    try {
      // Phase 1: Load bootloader
      this.log('Loading bootloader...');
      const bootloader = await this.loader.loadModule('bootloader.wasm');
      this.log(`Bootloader loaded (type: ${bootloader.typeStr})`);

      // Phase 2: Initialize kernel
      this.log('Loading kernel...');
      const kernel = await this.loader.loadModule('kernel.wasm');
      this.log(`Kernel loaded (type: ${kernel.typeStr})`);

      // Phase 3: Instantiate kernel with runtime bridge imports
      this.log('Starting kernel...');
      const imports = this.bridge.getWasmImports();
      const kernelInstance = await this.loader.instantiateModule('kernel.wasm', imports);

      // Call kernel entry point
      if (kernelInstance.exports.main) {
        (kernelInstance.exports.main as Function)();
      } else if (kernelInstance.exports._start) {
        (kernelInstance.exports._start as Function)();
      }

      this.log('Kernel started successfully');

      // Phase 4: Load drivers
      this.log('Loading drivers...');
      await this.loadDrivers();

      // Phase 5: Load services
      this.log('Loading services...');
      await this.loadServices();

      // Phase 6: Boot complete
      this.log('Boot complete');
      this.hideBootStatus();

      // Start render loop
      this.startRenderLoop();

    } catch (error) {
      this.log(`Boot failed: ${error}`);
      console.error('WebOS boot failed:', error);
      if (this.bootStatus) {
        this.bootStatus.innerHTML = `
          <p style="color: #f44; font-size: 16px;">Boot Failed</p>
          <p style="color: #888; font-size: 14px; margin-top: 8px;">${error}</p>
        `;
      }
    }
  }

  /** Load all driver modules */
  private async loadDrivers(): Promise<void> {
    const drivers = [
      'gpu_driver.Wdll',
      'input_driver.Wdll',
      'storage_driver.Wdll',
      'network_driver.Wdll',
    ];

    for (const driver of drivers) {
      try {
        const info = await this.loader.loadModule(driver);
        this.log(`  Driver: ${driver} (${info.typeStr})`);
      } catch (e) {
        this.log(`  Driver ${driver}: not found (skipping)`);
      }
    }
  }

  /** Load all service modules */
  private async loadServices(): Promise<void> {
    const services = [
      'wm.Wdll',
      'fs_service.Wdll',
      'net_service.Wdll',
      'appstore.Wdll',
    ];

    for (const service of services) {
      try {
        const info = await this.loader.loadModule(service);
        this.log(`  Service: ${service} (${info.typeStr})`);
      } catch (e) {
        this.log(`  Service ${service}: not found (skipping)`);
      }
    }
  }

  /** Log a message to the boot status display */
  private log(message: string): void {
    console.log(`[WebOS] ${message}`);
    if (this.bootStatus) {
      const p = this.bootStatus.querySelector('p');
      if (p) {
        p.textContent = message;
      }
    }
  }

  /** Hide the boot status overlay */
  private hideBootStatus(): void {
    if (this.bootStatus) {
      this.bootStatus.style.transition = 'opacity 0.5s';
      this.bootStatus.style.opacity = '0';
      setTimeout(() => {
        this.bootStatus?.remove();
      }, 500);
    }
  }

  /** Start the main render loop */
  private startRenderLoop(): void {
    const render = () => {
      // Render windows to the canvas
      const windows = this.bridge.getAllWindows();
      if (this.canvas && windows.length > 0) {
        const ctx = this.canvas.getContext('2d');
        if (ctx) {
          ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);
          for (const win of windows) {
            const imageData = new ImageData(win.framebuffer, win.width, win.height);
            ctx.putImageData(imageData, win.x, win.y);
          }
        }
      }
      requestAnimationFrame(render);
    };
    requestAnimationFrame(render);
  }
}

// Bootstrap
window.addEventListener('DOMContentLoaded', () => {
  const host = new WebOSHost();
  host.boot();
});
