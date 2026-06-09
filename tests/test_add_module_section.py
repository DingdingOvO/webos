#!/usr/bin/env python3
"""Tests for add_module_section.py"""

import sys
import os
import tempfile

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'tools'))
from add_module_section import add_custom_section, WASM_MAGIC, WASM_VERSION

def make_minimal_wasm():
    """Create a minimal valid WASM binary with just the header."""
    # Just the WASM header - a valid module with no sections
    return WASM_MAGIC + WASM_VERSION

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

def test_add_custom_section():
    """Test adding a custom section to a WASM file."""
    with tempfile.NamedTemporaryFile(suffix='.wasm', delete=False) as f:
        f.write(make_minimal_wasm())
        path = f.name
    
    try:
        add_custom_section(path, "wex")
        
        with open(path, 'rb') as f:
            data = f.read()
        
        # Verify WASM magic is still present
        assert data[:4] == WASM_MAGIC, "WASM magic corrupted"
        assert data[4:8] == WASM_VERSION, "WASM version corrupted"
        
        # Check for module_type section
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
        
        # Find module_type section and verify it has "Wdll"
        content = find_custom_section(data, "module_type")
        assert content is not None, "module_type custom section not found"
        assert content == b"Wdll", f"Expected b'Wdll', got {content}"
        
        # Verify there's only one module_type section
        count = 0
        offset = 8
        while offset < len(data):
            section_id = data[offset]
            offset += 1
            section_size, offset = read_leb128(data, offset)
            section_end = offset + section_size
            
            if section_id == 0:
                name_len, name_offset = read_leb128(data, offset)
                name = data[name_offset:name_offset + name_len].decode('utf-8')
                if name == "module_type":
                    count += 1
            
            offset = section_end
        
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

if __name__ == "__main__":
    test_add_custom_section()
    test_replace_existing_section()
    test_invalid_module_type()
    print("\nAll add_module_section tests passed!")
