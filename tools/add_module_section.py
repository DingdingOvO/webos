#!/usr/bin/env python3
"""
add_module_section.py - Add a custom section to a WASM binary file.

Usage:
    python add_module_section.py <wasm_file> <module_type>

Module types: "wex", "Wdll", "wli"

The custom section name is "module_type" and the content is the module type string.
This allows the dynamic loader to identify module types without relying on file extensions.
"""

import sys
import struct

WASM_MAGIC = b'\x00asm'
WASM_VERSION = b'\x01\x00\x00\x00'

# Custom section id = 0
CUSTOM_SECTION_ID = 0


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


def encode_leb128(value):
    """Encode an unsigned integer as LEB128."""
    result = bytearray()
    while True:
        byte = value & 0x7F
        value >>= 7
        if value != 0:
            byte |= 0x80
        result.append(byte)
        if value == 0:
            break
    return bytes(result)


def encode_section(section_id, content):
    """Encode a WASM section with id and content."""
    return bytes([section_id]) + encode_leb128(len(content)) + content


def make_custom_section(name, content_str):
    """Create a custom section with the given name and content string."""
    name_bytes = name.encode('utf-8')
    content_bytes = content_str.encode('utf-8')
    # Custom section payload: name_len + name + content
    payload = encode_leb128(len(name_bytes)) + name_bytes + content_bytes
    return encode_section(CUSTOM_SECTION_ID, payload)


def add_custom_section(wasm_file, module_type):
    """Add or replace the module_type custom section in a WASM file."""
    valid_types = {"wex", "Wdll", "wli"}
    if module_type not in valid_types:
        print(f"Error: Invalid module type '{module_type}'. Must be one of: {', '.join(valid_types)}")
        sys.exit(1)

    with open(wasm_file, 'rb') as f:
        data = bytearray(f.read())

    # Verify WASM magic and version
    if data[:4] != WASM_MAGIC:
        print(f"Error: {wasm_file} is not a valid WASM file (bad magic)")
        sys.exit(1)
    if data[4:8] != WASM_VERSION:
        print(f"Error: {wasm_file} has unsupported WASM version")
        sys.exit(1)

    # Find and remove existing module_type custom section
    offset = 8  # Skip magic + version
    new_data = bytearray(data[:8])  # Keep header

    while offset < len(data):
        section_id = data[offset]
        offset += 1
        section_size, offset = read_leb128(data, offset)
        section_start = offset
        section_end = offset + section_size

        if section_id == CUSTOM_SECTION_ID:
            # Read custom section name
            name_len, name_offset = read_leb128(data, offset)
            name = data[name_offset:name_offset + name_len].decode('utf-8', errors='replace')

            if name == "module_type":
                # Skip this section (remove it)
                print(f"  Replacing existing module_type section")
                offset = section_end
                continue

        # Keep this section
        new_data.extend(data[offset - 1 - len(encode_leb128(section_size)):section_end])
        # Actually, let's just reconstruct properly
        offset = section_end

    # Simpler approach: rebuild the file
    # Re-parse and copy all sections except module_type custom sections
    offset = 8
    sections_to_keep = bytearray()

    while offset < len(data):
        section_start_offset = offset
        section_id = data[offset]
        offset += 1
        section_size, offset = read_leb128(data, offset)
        section_payload_start = offset
        section_end = offset + section_size

        skip = False
        if section_id == CUSTOM_SECTION_ID:
            name_len, name_offset = read_leb128(data, section_payload_start)
            name = data[name_offset:name_offset + name_len].decode('utf-8', errors='replace')
            if name == "module_type":
                skip = True
                print(f"  Removing existing module_type custom section")

        if not skip:
            # Copy the entire section as-is
            sections_to_keep.extend(data[section_start_offset:section_end])

        offset = section_end

    # Build new file: header + custom section + other sections
    custom_section = make_custom_section("module_type", module_type)
    new_file = bytearray(WASM_MAGIC + WASM_VERSION) + bytearray(custom_section) + sections_to_keep

    with open(wasm_file, 'wb') as f:
        f.write(new_file)

    print(f"  Added module_type='{module_type}' to {wasm_file}")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <wasm_file> <module_type>")
        print(f"  module_type: wex | Wdll | wli")
        sys.exit(1)

    wasm_file = sys.argv[1]
    module_type = sys.argv[2]
    add_custom_section(wasm_file, module_type)
