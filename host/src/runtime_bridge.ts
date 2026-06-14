/**
 * WebOS Runtime Bridge
 * 
 * Provides the host-side implementation of syscalls and browser API
 * bridges. This is the interface between WASM modules and the browser.
 * 
 * Key principle: WASM modules NEVER call browser APIs directly.
 * They go through this bridge via imported functions.
 * 
 * Version: 0.0.1beta
 */

import { SyscallHandler, RuntimeConfig, DEFAULT_CONFIG } from './types';

export class RuntimeBridge {
  private config: RuntimeConfig;
  private memory: WebAssembly.Memory;
  private syscallHandlers: Map<number, SyscallHandler> = new Map();

  constructor(config: Partial<RuntimeConfig> = {}) {
    this.config = { ...DEFAULT_CONFIG, ...config };
    this.memory = new WebAssembly.Memory({
      initial: this.config.memoryPages,
      maximum: 1024,
    });
    this.registerSyscalls();
  }

  /** Get the WASM imports object for module instantiation */
  getWasmImports(): WebAssembly.Imports {
    return {
      env: {
        memory: this.memory,
      },
      webos: {
        // Syscall dispatcher — all kernel-to-host calls go through this
        syscall_invoke: (num: number, a1: number, a2: number,
                         a3: number, a4: number, a5: number): number => {
          return this.dispatch(num, a1, a2, a3, a4, a5);
        },

        // Console logging
        js_console_log: (ptr: number, len: number): void => {
          const bytes = new Uint8Array(this.memory.buffer, ptr, len);
          const text = new TextDecoder().decode(bytes);
          console.log(`[kernel] ${text}`);
        },

        // Time
        js_time_now: (): number => {
          return performance.now();
        },

        // Storage (IndexedDB block interface)
        js_storage_read: (blockNum: number, bufPtr: number): number => {
          // Stub — will be implemented with IndexedDB
          return -1;
        },
        js_storage_write: (blockNum: number, bufPtr: number): number => {
          // Stub
          return -1;
        },

        // Input events
        js_input_get_events: (bufPtr: number, maxCount: number): number => {
          // Stub — will poll from input event queue
          return 0;
        },

        // GPU — these are called by the GPU driver from WASM
        js_gpu_create_context: (): number => {
          // Handled by compositor, not by WASM directly
          return 0;
        },
        js_gpu_draw: (pipelineId: number, vertexCount: number): void => {
          // Stub
        },

        // Audio
        js_audio_create_context: (): number => {
          // Stub — Web Audio API bridge
          return 0;
        },
        js_audio_play_pcm: (dataPtr: number, len: number, sampleRate: number): void => {
          // Stub
        },
        js_audio_set_volume: (ctxId: number, volume: number): void => {
          // Stub
        },
      },
    };
  }

  /** Get the shared memory */
  getMemory(): WebAssembly.Memory {
    return this.memory;
  }

  /** Dispatch a syscall */
  private dispatch(num: number, a1: number, a2: number,
                   a3: number, a4: number, a5: number): number {
    const handler = this.syscallHandlers.get(num);
    if (handler) {
      try {
        return handler(a1, a2, a3, a4, a5);
      } catch (e) {
        console.error(`[WebOS] Syscall ${num.toString(16)} fault:`, e);
        return -6; // ERR_FAULT
      }
    }
    if (this.config.debugMode) {
      console.warn(`[WebOS] Unimplemented syscall: 0x${num.toString(16)}`);
    }
    return -8; // ERR_NOSYS
  }

  /** Register default syscall implementations */
  private registerSyscalls(): void {
    // SYS_PRINT (0x01)
    this.registerSyscall(0x01, (msgPtr, len) => {
      const bytes = new Uint8Array(this.memory.buffer, msgPtr, len);
      const text = new TextDecoder().decode(bytes);
      console.log(`[print] ${text}`);
      return 0;
    });

    // SYS_GETPID (0x0D)
    this.registerSyscall(0x0D, () => {
      return 1; // Simplified: return init PID
    });

    // SYS_YIELD (0x10)
    this.registerSyscall(0x10, () => {
      return 0;
    });

    // SYS_IPC_SEND (0x09)
    this.registerSyscall(0x09, (target, type, dataPtr, len) => {
      // Stub
      return 0;
    });

    // SYS_IPC_RECV (0x0A)
    this.registerSyscall(0x0A, (bufPtr, timeout) => {
      // Stub
      return -1; // Would block
    });
  }

  /** Register a custom syscall handler */
  registerSyscall(num: number, handler: SyscallHandler): void {
    this.syscallHandlers.set(num, handler);
  }
}
