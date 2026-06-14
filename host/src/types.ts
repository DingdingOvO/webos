/**
 * WebOS Host - Type Definitions
 * 
 * Core types for the WebOS TypeScript host runtime.
 * Version: 0.0.1beta
 */

// ─── Module System ───

export enum ModuleKind {
  UNKNOWN = 0,
  WEX = 1,
  WDLL = 2,
  WLI = 3,
}

export const MODULE_TYPE = {
  WEX: 'wex',
  WDLL: 'Wdll',
  WLI: 'wli',
} as const;

export interface ModuleInfo {
  kind: ModuleKind;
  typeStr: string;
  module: WebAssembly.Module;
  instance?: WebAssembly.Instance;
}

// ─── Syscall ───

export type SyscallHandler = (
  a1: number, a2: number, a3: number,
  a4: number, a5: number
) => number;

// ─── Process ───

export enum ProcessState {
  CREATED = 0,
  READY = 1,
  RUNNING = 2,
  BLOCKED = 3,
  TERMINATED = 4,
}

export interface ProcessBlock {
  pid: number;
  ppid: number;
  uid: number;
  state: ProcessState;
  priority: number;
  entryPoint: number;
  memoryOffset: number;
  memorySize: number;
  openFiles: number[];
}

// ─── IPC ───

export interface IpcMessage {
  sender: number;
  receiver: number;
  type: number;
  length: number;
  data: ArrayBuffer;
}

// ─── Window ───

export interface WindowState {
  handle: number;
  title: string;
  x: number;
  y: number;
  width: number;
  height: number;
  focused: boolean;
  minimized: boolean;
  maximized: boolean;
  ownerPid: number;
  // GPU rendering target
  renderTarget?: GPUTexture;
  renderTargetView?: GPUTextureView;
}

// ─── Boot Phase ───

export enum BootPhase {
  INIT = 'init',
  BOOTLOADER = 'bootloader',
  KERNEL = 'kernel',
  DRIVERS = 'drivers',
  SERVICES = 'services',
  LOGIN = 'login',
  DESKTOP = 'desktop',
  COMPLETE = 'complete',
  FAILED = 'failed',
}

// ─── Runtime Config ───

export interface RuntimeConfig {
  wasmPath: string;
  memoryPages: number;
  debugMode: boolean;
  canvasId: string;
}

export const DEFAULT_CONFIG: RuntimeConfig = {
  wasmPath: '/wasm/',
  memoryPages: 256,
  debugMode: true,
  canvasId: 'webos-canvas',
};

// ─── Design Tokens (from 01_设计语言.md) ───

export const DESIGN = {
  color: {
    accentBlue:        '#3B82F6',
    accentBlueHover:   '#2563EB',
    accentBlueActive:  '#1D4ED8',
    accentBlueGlow:    [0.231, 0.510, 0.965, 0.25] as [number, number, number, number],
    surfaceBase:       [0.059, 0.090, 0.165, 1.0] as [number, number, number, number],
    surfaceElevated:   [0.118, 0.161, 0.231, 0.72] as [number, number, number, number],
    surfaceOverlay:    [0.200, 0.255, 0.333, 1.0] as [number, number, number, number],
    textPrimary:       [0.973, 0.980, 0.988, 1.0] as [number, number, number, number],
    textSecondary:     [0.580, 0.639, 0.722, 1.0] as [number, number, number, number],
    border:            [0.580, 0.639, 0.722, 0.12] as [number, number, number, number],
    danger:            [0.937, 0.267, 0.267, 1.0] as [number, number, number, number],
  },
  blur: {
    window:     40,
    taskbar:    40,
    popup:      32,
    dialog:     48,
    toast:      24,
    tooltip:    16,
  },
  noise: {
    opacity: 0.025,
    scale: 6.0,
  },
  radius: {
    window:  12,
    popup:   8,
    button:  6,
    card:    10,
    dialog:  14,
    toast:   10,
    tooltip: 6,
  },
  taskbar: {
    height: 48,
  },
  titlebar: {
    height: 36,
  },
} as const;
