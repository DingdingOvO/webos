#!/usr/bin/env python3
"""Tests for add_module_section.py - Extended"""

import sys
import os
import tempfile

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'tools'))
from add_module_section import add_custom_section, WASM_MAGIC, WASM_VERSION

def make_minimal_wasm():
    """Create a minimal valid WASM binary with just the header."""
    return WASM_MAGIC + WASM_VERSION

def make_wasm_with_type_section():
    """Create a WASM binary that already has a type section."""
    wasm = bytearray(WASM_MAGIC + WASM_VERSION)
    # Type section: one function type () -> ()
    wasm += bytes([0x01, 0x04, 0x01, 0x60, 0x00, 0x00])
    return bytes(wasm)

def read_leb128(data, offset):
    """Read an unsigned LEB128 encoded integer."""
    result = 0
    shift = 0
    while offset < len(data):
        byte = data[offset]
        offset += 1
        result |= (byte & 0x7F) << shift
        if (byte & 0x80) == 0:
            break
        shift += 7
    return result, offset

def find_custom_section(data, name):
    """Find a custom section by name in a WASM binary."""
    offset = 8  # Skip magic + version
    while offset < len(data):
        if offset >= len(data):
            break
        section_id = data[offset]
        offset += 1
        if offset >= len(data):
            break
        section_size, offset = read_leb128(data, offset)
        section_end = offset + section_size

        if section_id == 0:  # Custom section
            name_len, name_offset = read_leb128(data, offset)
            section_name = data[name_offset:name_offset + name_len].decode('utf-8')
            if section_name == name:
                content = data[name_offset + name_len:section_end]
                return content

        offset = section_end
    return None

def count_custom_sections(data, name):
    """Count how many custom sections with a given name exist."""
    count = 0
    offset = 8
    while offset < len(data):
        if offset >= len(data):
            break
        section_id = data[offset]
        offset += 1
        if offset >= len(data):
            break
        section_size, offset = read_leb128(data, offset)
        section_end = offset + section_size
        if section_id == 0:
            name_len, name_offset = read_leb128(data, offset)
            section_name = data[name_offset:name_offset + name_len].decode('utf-8')
            if section_name == name:
                count += 1
        offset = section_end
    return count

def test_add_custom_section():
    """Test adding a custom section to a WASM file."""
    with tempfile.NamedTemporaryFile(suffix='.wasm', delete=False) as f:
        f.write(make_minimal_wasm())
        path = f.name

    try:
        add_custom_section(path, "wex")

        with open(path, 'rb') as f:
            data = f.read()

        assert data[:4] == WASM_MAGIC, "WASM magic corrupted"
        assert data[4:8] == WASM_VERSION, "WASM version corrupted"

        content = find_custom_section(data, "module_type")
        assert content is not None, "module_type custom section not found"
        assert content == b"wex", f"Expected b'wex', got {content}"

        print("PASS: test_add_custom_section")
    finally:
        os.unlink(path)

def test_replace_existing_section():
    """Test that adding a section replaces an existing one."""
    with tempfile.NamedTemporaryFile(suffix='.wasm', delete=False) as f:
        f.write(make_minimal_wasm())
        path = f.name

    try:
        add_custom_section(path, "wex")
        add_custom_section(path, "Wdll")

        with open(path, 'rb') as f:
            data = f.read()

        content = find_custom_section(data, "module_type")
        assert content is not None, "module_type custom section not found"
        assert content == b"Wdll", f"Expected b'Wdll', got {content}"

        count = count_custom_sections(data, "module_type")
        assert count == 1, f"Expected 1 module_type section, found {count}"

        print("PASS: test_replace_existing_section")
    finally:
        os.unlink(path)

def test_invalid_module_type():
    """Test that invalid module types are rejected."""
    with tempfile.NamedTemporaryFile(suffix='.wasm', delete=False) as f:
        f.write(make_minimal_wasm())
        path = f.name

    try:
        try:
            add_custom_section(path, "invalid")
            assert False, "Should have raised SystemExit"
        except SystemExit:
            pass
        print("PASS: test_invalid_module_type")
    finally:
        os.unlink(path)

def test_add_type_wex():
    """Test adding module_type='wex'."""
    with tempfile.NamedTemporaryFile(suffix='.wasm', delete=False) as f:
        f.write(make_minimal_wasm())
        path = f.name

    try:
        add_custom_section(path, "wex")
        with open(path, 'rb') as f:
            data = f.read()
        content = find_custom_section(data, "module_type")
        assert content == b"wex", f"Expected b'wex', got {content}"
        print("PASS: test_add_type_wex")
    finally:
        os.unlink(path)

def test_add_type_Wdll():
    """Test adding module_type='Wdll'."""
    with tempfile.NamedTemporaryFile(suffix='.wasm', delete=False) as f:
        f.write(make_minimal_wasm())
        path = f.name

    try:
        add_custom_section(path, "Wdll")
        with open(path, 'rb') as f:
            data = f.read()
        content = find_custom_section(data, "module_type")
        assert content == b"Wdll", f"Expected b'Wdll', got {content}"
        print("PASS: test_add_type_Wdll")
    finally:
        os.unlink(path)

def test_add_type_wli():
    """Test adding module_type='wli'."""
    with tempfile.NamedTemporaryFile(suffix='.wasm', delete=False) as f:
        f.write(make_minimal_wasm())
        path = f.name

    try:
        add_custom_section(path, "wli")
        with open(path, 'rb') as f:
            data = f.read()
        content = find_custom_section(data, "module_type")
        assert content == b"wli", f"Expected b'wli', got {content}"
        print("PASS: test_add_type_wli")
    finally:
        os.unlink(path)

def test_multiple_section_additions():
    """Test adding sections multiple times (wex -> Wdll -> wli)."""
    with tempfile.NamedTemporaryFile(suffix='.wasm', delete=False) as f:
        f.write(make_minimal_wasm())
        path = f.name

    try:
        add_custom_section(path, "wex")
        add_custom_section(path, "Wdll")
        add_custom_section(path, "wli")

        with open(path, 'rb') as f:
            data = f.read()
        content = find_custom_section(data, "module_type")
        assert content == b"wli", f"Expected b'wli' after final addition, got {content}"
        count = count_custom_sections(data, "module_type")
        assert count == 1, f"Expected 1 module_type section, found {count}"
        print("PASS: test_multiple_section_additions")
    finally:
        os.unlink(path)

def test_non_wasm_file_error():
    """Test that a non-WASM file is rejected."""
    with tempfile.NamedTemporaryFile(suffix='.wasm', delete=False) as f:
        f.write(b'ELF this is not WASM')
        path = f.name

    try:
        try:
            add_custom_section(path, "wex")
            assert False, "Should have raised SystemExit for non-WASM file"
        except SystemExit:
            pass
        print("PASS: test_non_wasm_file_error")
    finally:
        os.unlink(path)

def test_empty_file_error():
    """Test that an empty file is rejected."""
    with tempfile.NamedTemporaryFile(suffix='.wasm', delete=False) as f:
        f.write(b'')
        path = f.name

    try:
        try:
            add_custom_section(path, "wex")
            assert False, "Should have raised SystemExit for empty file"
        except (SystemExit, IndexError):
            pass
        print("PASS: test_empty_file_error")
    finally:
        os.unlink(path)

def test_wasm_magic_preserved():
    """Test that WASM magic bytes are preserved after adding section."""
    with tempfile.NamedTemporaryFile(suffix='.wasm', delete=False) as f:
        f.write(make_minimal_wasm())
        path = f.name

    try:
        add_custom_section(path, "wex")
        with open(path, 'rb') as f:
            data = f.read()
        assert data[:4] == b'\x00asm', f"WASM magic not preserved: {data[:4]}"
        print("PASS: test_wasm_magic_preserved")
    finally:
        os.unlink(path)

def test_wasm_version_preserved():
    """Test that WASM version bytes are preserved after adding section."""
    with tempfile.NamedTemporaryFile(suffix='.wasm', delete=False) as f:
        f.write(make_minimal_wasm())
        path = f.name

    try:
        add_custom_section(path, "Wdll")
        with open(path, 'rb') as f:
            data = f.read()
        assert data[4:8] == b'\x01\x00\x00\x00', f"WASM version not preserved: {data[4:8]}"
        print("PASS: test_wasm_version_preserved")
    finally:
        os.unlink(path)

def test_section_ordering_after_replacement():
    """Test that custom section is placed before other sections after replacement."""
    with tempfile.NamedTemporaryFile(suffix='.wasm', delete=False) as f:
        f.write(make_wasm_with_type_section())
        path = f.name

    try:
        add_custom_section(path, "wex")
        with open(path, 'rb') as f:
            data = f.read()

        # The module_type custom section (id=0) should come before
        # the type section (id=1)
        offset = 8
        first_section_id = data[offset]
        assert first_section_id == 0, f"First section should be custom (0), got {first_section_id}"

        # Find module_type content
        content = find_custom_section(data, "module_type")
        assert content == b"wex", f"Expected b'wex', got {content}"
        print("PASS: test_section_ordering_after_replacement")
    finally:
        os.unlink(path)

def test_large_wasm_file():
    """Test adding section to a WASM file with multiple existing sections."""
    with tempfile.NamedTemporaryFile(suffix='.wasm', delete=False) as f:
        wasm = bytearray(WASM_MAGIC + WASM_VERSION)
        # Type section
        wasm += bytes([0x01, 0x04, 0x01, 0x60, 0x00, 0x00])
        # Function section
        wasm += bytes([0x03, 0x02, 0x01, 0x00])
        # Export section
        wasm += bytes([0x07, 0x08, 0x01, 0x04, 0x6d, 0x61, 0x69, 0x6e, 0x00, 0x00])
        # Code section
        wasm += bytes([0x0a, 0x04, 0x01, 0x02, 0x00, 0x0b])
        f.write(bytes(wasm))
        path = f.name

    try:
        add_custom_section(path, "Wdll")
        with open(path, 'rb') as f:
            data = f.read()
        content = find_custom_section(data, "module_type")
        assert content == b"Wdll", f"Expected b'Wdll', got {content}"
        # WASM header should still be intact
        assert data[:4] == WASM_MAGIC
        assert data[4:8] == WASM_VERSION
        print("PASS: test_large_wasm_file")
    finally:
        os.unlink(path)

if __name__ == "__main__":
    test_add_custom_section()
    test_replace_existing_section()
    test_invalid_module_type()
    test_add_type_wex()
    test_add_type_Wdll()
    test_add_type_wli()
    test_multiple_section_additions()
    test_non_wasm_file_error()
    test_empty_file_error()
    test_wasm_magic_preserved()
    test_wasm_version_preserved()
    test_section_ordering_after_replacement()
    test_large_wasm_file()
    print("\nAll add_module_section tests passed!")
