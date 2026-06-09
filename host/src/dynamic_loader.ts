/**
 * WebOS Dynamic Loader
 * 
 * Loads and manages WASM modules, reads custom sections to
 * determine module types, and handles dynamic linking.
 */

import { ModuleInfo, ModuleKind, MODULE_TYPE } from './types';

export class DynamicLoader {
  private modules: Map<string, ModuleInfo> = new Map();
  private wasmPath: string;

  constructor(wasmPath: string = '/wasm/') {
    this.wasmPath = wasmPath;
  }

  /** Load a WASM module from the server */
  async loadModule(name: string, imports?: WebAssembly.Imports): Promise<ModuleInfo> {
    // Check cache
    const cached = this.modules.get(name);
    if (cached) {
      return cached;
    }

    // Fetch the WASM file
    const url = `${this.wasmPath}${name}`;
    const response = await fetch(url);
    if (!response.ok) {
      throw new Error(`Failed to fetch module: ${url} (${response.status})`);
    }

    const arrayBuffer = await response.arrayBuffer();
    const bytes = new Uint8Array(arrayBuffer);

    // Read module type from custom section
    const moduleKind = this.readModuleType(bytes);

    // Compile the module
    const module = await WebAssembly.compile(arrayBuffer);

    const info: ModuleInfo = {
      kind: moduleKind.kind,
      typeStr: moduleKind.typeStr,
      module,
    };

    // Cache it
    this.modules.set(name, info);

    return info;
  }

  /** Instantiate a loaded module */
  async instantiateModule(
    name: string,
    imports: WebAssembly.Imports
  ): Promise<WebAssembly.Instance> {
    const info = this.modules.get(name);
    if (!info) {
      throw new Error(`Module not loaded: ${name}`);
    }

    if (info.instance) {
      return info.instance;
    }

    const instance = await WebAssembly.instantiate(info.module, imports);
    info.instance = instance;
    return instance;
  }

  /** Read the module_type custom section from WASM bytes */
  private readModuleType(bytes: Uint8Array): { kind: ModuleKind; typeStr: string } {
    // Skip magic + version (8 bytes)
    let offset = 8;

    while (offset < bytes.length) {
      const sectionId = bytes[offset];
      offset += 1;
      const [sectionSize, newOffset] = this.readLEB128(bytes, offset);
      offset = newOffset;
      const sectionEnd = offset + sectionSize;

      // Custom section (id = 0)
      if (sectionId === 0) {
        const [nameLen, nameOffset] = this.readLEB128(bytes, offset);
        const name = new TextDecoder().decode(bytes.slice(nameOffset, nameOffset + nameLen));

        if (name === 'module_type') {
          // Read the module type string
          const typeStart = nameOffset + nameLen;
          const typeStr = new TextDecoder().decode(bytes.slice(typeStart, sectionEnd));
          const kind = this.moduleKindFromString(typeStr);
          return { kind, typeStr };
        }
      }

      offset = sectionEnd;
    }

    // No module_type section found, try to guess from filename extension
    return { kind: ModuleKind.UNKNOWN, typeStr: 'unknown' };
  }

  /** Read an unsigned LEB128 value */
  private readLEB128(bytes: Uint8Array, offset: number): [number, number] {
    let result = 0;
    let shift = 0;
    while (true) {
      const byte = bytes[offset];
      offset += 1;
      result |= (byte & 0x7F) << shift;
      if ((byte & 0x80) === 0) break;
      shift += 7;
    }
    return [result, offset];
  }

  /** Convert module type string to enum */
  private moduleKindFromString(typeStr: string): ModuleKind {
    switch (typeStr) {
      case MODULE_TYPE.WEX: return ModuleKind.WEX;
      case MODULE_TYPE.WDLL: return ModuleKind.WDLL;
      case MODULE_TYPE.WLI: return ModuleKind.WLI;
      default: return ModuleKind.UNKNOWN;
    }
  }

  /** Get a loaded module's info */
  getModuleInfo(name: string): ModuleInfo | undefined {
    return this.modules.get(name);
  }

  /** List all loaded modules */
  listModules(): string[] {
    return Array.from(this.modules.keys());
  }

  /** Unload a module */
  unloadModule(name: string): boolean {
    return this.modules.delete(name);
  }
}
