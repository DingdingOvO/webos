/**
 * WebOS Host - Type Definitions
 * 
 * Core types for the WebOS TypeScript host runtime.
 */

/** Module kind matching the C enum in module_types.h */
export enum ModuleKind {
  UNKNOWN = 0,
  WEX = 1,       // Executable
  WDLL = 2,      // Dynamic library
  WLI = 3,       // Library interface
}

/** Module type string constants */
export const MODULE_TYPE = {
  WEX: 'wex',
  WDLL: 'Wdll',
  WLI: 'wli',
} as const;

/** Information about a loaded WASM module */
export interface ModuleInfo {
  kind: ModuleKind;
  typeStr: string;
  module: WebAssembly.Module;
  instance?: WebAssembly.Instance;
}

/** Syscall number type */
export type SyscallNumber = number;

/** Syscall handler function signature */
export type SyscallHandler = (
  a1: number, a2: number, a3: number,
  a4: number, a5: number
) => number;

/** Process state */
export enum ProcessState {
  READY = 0,
  RUNNING = 1,
  BLOCKED = 2,
  ZOMBIE = 3,
}

/** Process control block */
export interface ProcessBlock {
  pid: number;
  module: ModuleInfo;
  state: ProcessState;
  priority: number;
  entryPoint: number;
}

/** IPC message */
export interface IpcMessage {
  sender: number;
  receiver: number;
  type: number;
  length: number;
  data: ArrayBuffer;
}

/** Window info tracked by the host */
export interface WindowInfo {
  handle: number;
  title: string;
  x: number;
  y: number;
  width: number;
  height: number;
  flags: number;
  framebuffer: Uint8ClampedArray;
  canvas: HTMLCanvasElement;
}

/** Runtime configuration */
export interface RuntimeConfig {
  wasmPath: string;
  memorySize: number;  // in pages (64KB each)
  debugMode: boolean;
  gpuBackend: 'canvas2d' | 'webgl' | 'webgpu';
}

/** Default runtime configuration */
export const DEFAULT_CONFIG: RuntimeConfig = {
  wasmPath: '/wasm/',
  memorySize: 256,  // 16 MB
  debugMode: false,
  gpuBackend: 'canvas2d',
};
