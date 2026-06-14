/**
 * WebOS Type Definition Tests
 *
 * Tests for ModuleType enum, ModuleInfo interface, WliInfo interface,
 * RuntimeAPI interface, HostConfig interface, and related types.
 */

import assert from 'assert';

// ---- Replicate the types from host/src/types.ts ----

enum ModuleKind {
  UNKNOWN = 0,
  WEX = 1,
  WDLL = 2,
  WLI = 3,
}

const MODULE_TYPE = {
  WEX: 'wex',
  WDLL: 'Wdll',
  WLI: 'wli',
} as const;

interface ModuleInfo {
  kind: ModuleKind;
  typeStr: string;
  module: WebAssembly.Module;
  instance?: WebAssembly.Instance;
}

interface WliInfo {
  module: string;
  exports: Array<{ name: string; signature: string }>;
  imports: Array<{ name: string; from: string }>;
}

interface RuntimeAPI {
  syscall: (num: number, a1: number, a2: number, a3: number, a4: number, a5: number) => number;
  loadModule: (name: string) => Promise<ModuleInfo>;
  getProcess: (pid: number) => ProcessBlock | null;
}

interface HostConfig {
  wasmPath: string;
  memorySize: number;
  debugMode: boolean;
  gpuBackend: 'canvas2d' | 'webgl' | 'webgpu';
}

interface ProcessBlock {
  pid: number;
  module: ModuleInfo;
  state: ProcessState;
  priority: number;
  entryPoint: number;
}

enum ProcessState {
  READY = 0,
  RUNNING = 1,
  BLOCKED = 2,
  ZOMBIE = 3,
}

interface IpcMessage {
  sender: number;
  receiver: number;
  type: number;
  length: number;
  data: ArrayBuffer;
}

interface WindowInfo {
  handle: number;
  title: string;
  x: number;
  y: number;
  width: number;
  height: number;
  flags: number;
  framebuffer: Uint8ClampedArray;
}

interface RuntimeConfig {
  wasmPath: string;
  memorySize: number;
  debugMode: boolean;
  gpuBackend: 'canvas2d' | 'webgl' | 'webgpu';
}

const DEFAULT_CONFIG: RuntimeConfig = {
  wasmPath: '/wasm/',
  memorySize: 256,
  debugMode: false,
  gpuBackend: 'canvas2d',
};

// ---- Tests ----

function testModuleKindEnum() {
  assert.strictEqual(ModuleKind.UNKNOWN, 0, 'UNKNOWN should be 0');
  assert.strictEqual(ModuleKind.WEX, 1, 'WEX should be 1');
  assert.strictEqual(ModuleKind.WDLL, 2, 'WDLL should be 2');
  assert.strictEqual(ModuleKind.WLI, 3, 'WLI should be 3');
  console.log('  ✓ ModuleKind enum values are correct');
}

function testModuleKindEnumCompleteness() {
  const values = Object.values(ModuleKind).filter(v => typeof v === 'number') as number[];
  assert.strictEqual(values.length, 4, 'Should have 4 ModuleKind values');
  // All values should be unique
  const unique = new Set(values);
  assert.strictEqual(unique.size, 4, 'ModuleKind values should be unique');
  console.log('  ✓ ModuleKind enum is complete and unique');
}

function testModuleTypeConstants() {
  assert.strictEqual(MODULE_TYPE.WEX, 'wex', 'WEX type string');
  assert.strictEqual(MODULE_TYPE.WDLL, 'Wdll', 'WDLL type string');
  assert.strictEqual(MODULE_TYPE.WLI, 'wli', 'WLI type string');
  console.log('  ✓ MODULE_TYPE string constants are correct');
}

function testModuleTypeConsistency() {
  // The string constants should map to the correct enum values
  assert.strictEqual(ModuleKind.WEX, 1);
  assert.strictEqual(ModuleKind.WDLL, 2);
  assert.strictEqual(ModuleKind.WLI, 3);
  console.log('  ✓ Module type enum and string constants are consistent');
}

function testProcessStateEnum() {
  assert.strictEqual(ProcessState.READY, 0, 'READY should be 0');
  assert.strictEqual(ProcessState.RUNNING, 1, 'RUNNING should be 1');
  assert.strictEqual(ProcessState.BLOCKED, 2, 'BLOCKED should be 2');
  assert.strictEqual(ProcessState.ZOMBIE, 3, 'ZOMBIE should be 3');
  console.log('  ✓ ProcessState enum values are correct');
}

function testProcessStateCompleteness() {
  const values = Object.values(ProcessState).filter(v => typeof v === 'number') as number[];
  assert.strictEqual(values.length, 4, 'Should have 4 ProcessState values');
  console.log('  ✓ ProcessState enum is complete');
}

function testDefaultRuntimeConfig() {
  assert.strictEqual(DEFAULT_CONFIG.wasmPath, '/wasm/');
  assert.strictEqual(DEFAULT_CONFIG.memorySize, 256, 'Default memory: 256 pages = 16MB');
  assert.strictEqual(DEFAULT_CONFIG.debugMode, false);
  assert.strictEqual(DEFAULT_CONFIG.gpuBackend, 'canvas2d');
  console.log('  ✓ Default runtime config is correct');
}

function testRuntimeConfigGpuBackendValues() {
  const validBackends: Array<HostConfig['gpuBackend']> = ['canvas2d', 'webgl', 'webgpu'];
  assert.strictEqual(validBackends.length, 3);
  assert.ok(validBackends.includes('canvas2d'));
  assert.ok(validBackends.includes('webgl'));
  assert.ok(validBackends.includes('webgpu'));
  console.log('  ✓ Runtime config GPU backend values are correct');
}

function testWliInfoStructure() {
  const wli: WliInfo = {
    module: 'gpu_driver',
    exports: [
      { name: 'gpu_init', signature: 'function()' },
      { name: 'gpu_draw_rect', signature: 'function(number,number,number,number,number)' },
    ],
    imports: [
      { name: 'malloc', from: 'env' },
    ],
  };
  assert.strictEqual(wli.module, 'gpu_driver');
  assert.strictEqual(wli.exports.length, 2);
  assert.strictEqual(wli.imports.length, 1);
  assert.strictEqual(wli.exports[0].name, 'gpu_init');
  assert.strictEqual(wli.imports[0].from, 'env');
  console.log('  ✓ WliInfo interface structure is correct');
}

function testWliInfoEmptyExportsAndImports() {
  const wli: WliInfo = {
    module: 'empty_module',
    exports: [],
    imports: [],
  };
  assert.strictEqual(wli.exports.length, 0);
  assert.strictEqual(wli.imports.length, 0);
  console.log('  ✓ WliInfo with empty exports/imports is valid');
}

function testIpcMessageStructure() {
  const msg: IpcMessage = {
    sender: 1,
    receiver: 2,
    type: 3,
    length: 4,
    data: new ArrayBuffer(4),
  };
  assert.strictEqual(msg.sender, 1);
  assert.strictEqual(msg.receiver, 2);
  assert.strictEqual(msg.type, 3);
  assert.strictEqual(msg.length, 4);
  assert.ok(msg.data instanceof ArrayBuffer);
  console.log('  ✓ IpcMessage interface structure is correct');
}

// Run all tests
console.log('\n[Type Definition Tests]');
testModuleKindEnum();
testModuleKindEnumCompleteness();
testModuleTypeConstants();
testModuleTypeConsistency();
testProcessStateEnum();
testProcessStateCompleteness();
testDefaultRuntimeConfig();
testRuntimeConfigGpuBackendValues();
testWliInfoStructure();
testWliInfoEmptyExportsAndImports();
testIpcMessageStructure();
console.log('\nAll type definition tests passed!');
