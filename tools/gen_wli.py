#!/usr/bin/env python3
"""
gen_wli.py - Generate a .wli (WebAssembly Library Interface) file from a .Wdll module.

Usage:
    python gen_wli.py <Wdll_file> [output.wli]

The .wli file is a JSON text file describing the module's exported and imported symbols.
It is used by the dynamic linker for symbol resolution and verification.
"""

import sys
import json
import subprocess
import os

WASM_MAGIC = b'\x00asm'


def read_leb128(data, offset):
    """Read an unsigned LEB128 encoded integer."""
    result = 0
    shift = 0
    while True:
        byte = data[offset]
        offset += 1
        result |= (byte & 0x7F) << shift
        if (byte & 0x80) == 0:
            break
        shift += 7
    return result, offset


def parse_wasm_exports(data):
    """Parse WASM binary to extract export section."""
    offset = 8  # Skip magic + version
    exports = []
    imports = []

    while offset < len(data):
        section_id = data[offset]
        offset += 1
        section_size, offset = read_leb128(data, offset)
        section_end = offset + section_size

        # Import section (id=2)
        if section_id == 2:
            num_imports, offset = read_leb128(data, offset)
            for _ in range(num_imports):
                mod_len, offset = read_leb128(data, offset)
                module_name = data[offset:offset + mod_len].decode('utf-8', errors='replace')
                offset += mod_len
                name_len, offset = read_leb128(data, offset)
                field_name = data[offset:offset + name_len].decode('utf-8', errors='replace')
                offset += name_len
                kind = data[offset]
                offset += 1
                if kind == 0:  # Function
                    type_idx, offset = read_leb128(data, offset)
                    imports.append({"name": field_name, "from": module_name, "kind": "function"})
                elif kind == 1:  # Table
                    offset += 3  # elem_type + limits
                elif kind == 2:  # Memory
                    offset += 2  # limits
                elif kind == 3:  # Global
                    offset += 3  # type + mutability
            offset = section_end

        # Export section (id=7)
        elif section_id == 7:
            num_exports, offset = read_leb128(data, offset)
            for _ in range(num_exports):
                name_len, offset = read_leb128(data, offset)
                name = data[offset:offset + name_len].decode('utf-8', errors='replace')
                offset += name_len
                kind = data[offset]
                offset += 1
                index, offset = read_leb128(data, offset)
                kind_str = {0: "function", 1: "table", 2: "memory", 3: "global"}.get(kind, "unknown")
                exports.append({"name": name, "kind": kind_str})
            offset = section_end

        else:
            offset = section_end

    return exports, imports


def get_module_name(filepath):
    """Extract module name from filename."""
    base = os.path.basename(filepath)
    # Remove extension
    name = base.rsplit('.', 1)[0]
    return name


def generate_wli(wdll_file, output_file=None):
    """Generate a .wli JSON file from a .Wdll module."""
    with open(wdll_file, 'rb') as f:
        data = f.read()

    if data[:4] != WASM_MAGIC:
        print(f"Error: {wdll_file} is not a valid WASM file")
        sys.exit(1)

    module_name = get_module_name(wdll_file)
    exports, imports = parse_wasm_exports(data)

    wli = {
        "module": module_name,
        "exports": [
            {"name": e["name"], "signature": f"{e['kind']}()"} for e in exports
        ],
        "imports": [
            {"name": i["name"], "from": i["from"]} for i in imports
        ]
    }

    if output_file is None:
        output_file = wdll_file.rsplit('.', 1)[0] + '.wli'

    with open(output_file, 'w') as f:
        json.dump(wli, f, indent=2)

    print(f"Generated {output_file}")
    print(f"  Module: {module_name}")
    print(f"  Exports: {len(exports)} symbols")
    print(f"  Imports: {len(imports)} symbols")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <Wdll_file> [output.wli]")
        sys.exit(1)

    wdll_file = sys.argv[1]
    output_file = sys.argv[2] if len(sys.argv) > 2 else None
    generate_wli(wdll_file, output_file)
