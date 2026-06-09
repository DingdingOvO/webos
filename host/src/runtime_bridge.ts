/**
 * WebOS Runtime Bridge
 * 
 * Provides the host-side implementation of syscalls and other
 * imported WASM functions. This is the interface between
 * WASM modules and the browser environment.
 */

import { SyscallHandler, ProcessBlock, ProcessState, WindowInfo, IpcMessage, RuntimeConfig, DEFAULT_CONFIG } from './types';

export class RuntimeBridge {
  private config: RuntimeConfig;
  private processes: Map<number, ProcessBlock> = new Map();
  private windows: Map<number, WindowInfo> = new Map();
  private nextPid = 1;
  private nextWindowHandle = 1;
  private syscallHandlers: Map<number, SyscallHandler> = new Map();
  private ipcQueue: IpcMessage[] = [];
  private sharedMemory: WebAssembly.Memory;

  constructor(config: Partial<RuntimeConfig> = {}) {
    this.config = { ...DEFAULT_CONFIG, ...config };
    this.sharedMemory = new WebAssembly.Memory({
      initial: this.config.memorySize,
      maximum: 1024,
      shared: false,
    });
    this.registerDefaultSyscalls();
  }

  /** Get the WebAssembly imports object for module instantiation */
  getWasmImports(): WebAssembly.Imports {
    return {
      webos: {
        syscall_invoke: (num: number, a1: number, a2: number,
                         a3: number, a4: number, a5: number): number => {
          return this.handleSyscall(num, a1, a2, a3, a4, a5);
        },
      },
      env: {
        memory: this.sharedMemory,
      },
    };
  }

  /** Register a syscall handler */
  registerSyscall(num: number, handler: SyscallHandler): void {
    this.syscallHandlers.set(num, handler);
  }

  /** Handle a syscall invocation from a WASM module */
  private handleSyscall(num: number, a1: number, a2: number,
                         a3: number, a4: number, a5: number): number {
    const handler = this.syscallHandlers.get(num);
    if (handler) {
      try {
        return handler(a1, a2, a3, a4, a5);
      } catch (e) {
        console.error(`Syscall ${num} error:`, e);
        return -6; // ERR_FAULT
      }
    }
    if (this.config.debugMode) {
      console.warn(`Unimplemented syscall: ${num}`);
    }
    return -8; // ERR_NOSYS
  }

  /** Register default syscall implementations */
  private registerDefaultSyscalls(): void {
    // SYS_PRINT (1)
    this.registerSyscall(1, (msgPtr, len, _a3, _a4, _a5) => {
      const bytes = new Uint8Array(this.sharedMemory.buffer, msgPtr, len);
      const text = new TextDecoder().decode(bytes);
      console.log('[print]', text);
      return 0;
    });

    // SYS_GETPID (22)
    this.registerSyscall(22, () => {
      // Return current process PID (simplified)
      return this.nextPid - 1 || 1;
    });

    // SYS_MALLOC (10)
    this.registerSyscall(10, (size, _a2, _a3, _a4, _a5) => {
      // Simplified malloc - in real impl would use a heap allocator
      return 0; // Stub
    });

    // SYS_FREE (11)
    this.registerSyscall(11, (_a1, _a2, _a3, _a4, _a5) => {
      return 0; // Stub
    });

    // SYS_WM_CREATE (50)
    this.registerSyscall(50, (titlePtr, titleLen, posPack, dimPack, flags) => {
      const x = posPack & 0xFFFF;
      const y = (posPack >> 16) & 0xFFFF;
      const w = dimPack & 0xFFFF;
      const h = (dimPack >> 16) & 0xFFFF;
      const titleBytes = new Uint8Array(this.sharedMemory.buffer, titlePtr, titleLen);
      const title = new TextDecoder().decode(titleBytes);

      const handle = this.nextWindowHandle++;
      const canvas = document.createElement('canvas');
      canvas.width = w;
      canvas.height = h;
      const framebuffer = new Uint8ClampedArray(w * h * 4);

      this.windows.set(handle, { handle, title, x, y, width: w, height: h, flags, framebuffer, canvas });
      return handle;
    });

    // SYS_WM_DESTROY (51)
    this.registerSyscall(51, (handle, _a2, _a3, _a4, _a5) => {
      return this.windows.delete(handle) ? 0 : -2;
    });

    // SYS_IPC_SEND (30)
    this.registerSyscall(30, (target, type, dataPtr, len, _a5) => {
      const data = new ArrayBuffer(len);
      const src = new Uint8Array(this.sharedMemory.buffer, dataPtr, len);
      new Uint8Array(data).set(src);
      this.ipcQueue.push({
        sender: this.nextPid - 1 || 1,
        receiver: target,
        type,
        length: len,
        data,
      });
      return 0;
    });
  }

  /** Get a window by handle */
  getWindow(handle: number): WindowInfo | undefined {
    return this.windows.get(handle);
  }

  /** Get all windows */
  getAllWindows(): WindowInfo[] {
    return Array.from(this.windows.values());
  }

  /** Get the shared memory */
  getMemory(): WebAssembly.Memory {
    return this.sharedMemory;
  }
}
