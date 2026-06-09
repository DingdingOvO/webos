#!/usr/bin/env python3
"""Tests for gen_wli.py"""

import sys
import os
import json
import tempfile

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'tools'))

WASM_MAGIC = b'\x00asm'
WASM_VERSION = b'\x01\x00\x00\x00'

def make_wasm_with_exports():
    """Create a minimal WASM with export section."""
    wasm = bytearray(WASM_MAGIC + WASM_VERSION)
    # Type section: one function type () -> ()
    wasm += bytes([0x01, 0x04, 0x01, 0x60, 0x00, 0x00])
    # Function section: one function, type 0
    wasm += bytes([0x03, 0x02, 0x01, 0x00])
    # Export section: export "test_func" as function 0
    wasm += bytes([0x07, 0x0d, 0x01,
                   0x09, 0x74, 0x65, 0x73, 0x74, 0x5f, 0x66, 0x75, 0x6e, 0x63,
                   0x00, 0x00])
    # Code section: one empty function body
    wasm += bytes([0x0a, 0x04, 0x01, 0x02, 0x00, 0x0b])
    return bytes(wasm)

def test_gen_wli():
    """Test WLI generation from a WASM file."""
    from gen_wli import generate_wli
    
    with tempfile.NamedTemporaryFile(suffix='.Wdll', delete=False) as f:
        f.write(make_wasm_with_exports())
        wasm_path = f.name
    
    wli_path = wasm_path.rsplit('.', 1)[0] + '.wli'
    
    try:
        generate_wli(wasm_path, wli_path)
        
        with open(wli_path, 'r') as f:
            wli = json.load(f)
        
        assert 'module' in wli, "Missing 'module' field"
        assert 'exports' in wli, "Missing 'exports' field"
        assert 'imports' in wli, "Missing 'imports' field"
        assert len(wli['exports']) > 0, "No exports found"
        
        # Check that test_func is in exports
        found = any(e['name'] == 'test_func' for e in wli['exports'])
        assert found, "test_func not found in exports"
        
        print("PASS: test_gen_wli")
    finally:
        os.unlink(wasm_path)
        if os.path.exists(wli_path):
            os.unlink(wli_path)

if __name__ == "__main__":
    test_gen_wli()
    print("\nAll gen_wli tests passed!")
