#!/usr/bin/env python3
"""Tests for gen_wli.py - Extended"""

import sys
import os
import json
import tempfile

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'tools'))

WASM_MAGIC = b'\x00asm'
WASM_VERSION = b'\x01\x00\x00\x00'


def make_wasm_with_exports(exports_list=None):
    """Create a minimal WASM with export section.
    exports_list: list of (name, kind) tuples. kind: 0=func, 1=table, 2=memory, 3=global
    """
    if exports_list is None:
        exports_list = [("test_func", 0)]

    wasm = bytearray(WASM_MAGIC + WASM_VERSION)
    # Type section: one function type () -> ()
    wasm += bytes([0x01, 0x04, 0x01, 0x60, 0x00, 0x00])
    # Function section: one function, type 0
    wasm += bytes([0x03, 0x02, 0x01, 0x00])

    # Build export section
    export_payload = bytearray()
    export_payload.append(len(exports_list))
    for name, kind in exports_list:
        name_bytes = name.encode('utf-8')
        export_payload.append(len(name_bytes))
        export_payload.extend(name_bytes)
        export_payload.append(kind)
        export_payload.append(0)  # index

    wasm.append(0x07)  # Export section id
    export_bytes = bytes(export_payload)
    wasm.append(len(export_bytes))
    wasm.extend(export_bytes)

    # Code section: one empty function body
    wasm += bytes([0x0a, 0x04, 0x01, 0x02, 0x00, 0x0b])
    return bytes(wasm)


def make_wasm_with_imports(imports_list=None):
    """Create a WASM with import section.
    imports_list: list of (module, name, kind) tuples. kind: 0=func, 1=table, 2=memory, 3=global
    """
    if imports_list is None:
        imports_list = [("env", "malloc", 0)]

    wasm = bytearray(WASM_MAGIC + WASM_VERSION)
    # Type section: one function type () -> ()
    wasm += bytes([0x01, 0x04, 0x01, 0x60, 0x00, 0x00])

    # Build import section
    import_payload = bytearray()
    import_payload.append(len(imports_list))
    for module, name, kind in imports_list:
        mod_bytes = module.encode('utf-8')
        import_payload.append(len(mod_bytes))
        import_payload.extend(mod_bytes)
        name_bytes = name.encode('utf-8')
        import_payload.append(len(name_bytes))
        import_payload.extend(name_bytes)
        import_payload.append(kind)
        if kind == 0:  # Function - add type index
            import_payload.append(0)

    wasm.append(0x02)  # Import section id
    import_bytes = bytes(import_payload)
    wasm.append(len(import_bytes))
    wasm.extend(import_bytes)

    # Function section: one function, type 0
    wasm += bytes([0x03, 0x02, 0x01, 0x00])
    # Code section: one empty function body
    wasm += bytes([0x0a, 0x04, 0x01, 0x02, 0x00, 0x0b])
    return bytes(wasm)


def make_wasm_with_memory_export():
    """Create WASM with a memory export."""
    wasm = bytearray(WASM_MAGIC + WASM_VERSION)
    # Type section: one function type () -> ()
    wasm += bytes([0x01, 0x04, 0x01, 0x60, 0x00, 0x00])
    # Function section: one function, type 0
    wasm += bytes([0x03, 0x02, 0x01, 0x00])
    # Memory section: one memory with min=1 page
    wasm += bytes([0x05, 0x03, 0x01, 0x00, 0x01])
    # Export section: export "memory" as memory 0
    wasm += bytes([0x07, 0x0a, 0x01,
                   0x06, 0x6d, 0x65, 0x6d, 0x6f, 0x72, 0x79,  # "memory"
                   0x02, 0x00])  # kind=memory, index=0
    # Code section: one empty function body
    wasm += bytes([0x0a, 0x04, 0x01, 0x02, 0x00, 0x0b])
    return bytes(wasm)


def make_wasm_with_table_export():
    """Create WASM with a table export."""
    wasm = bytearray(WASM_MAGIC + WASM_VERSION)
    # Type section: one function type () -> ()
    wasm += bytes([0x01, 0x04, 0x01, 0x60, 0x00, 0x00])
    # Function section: one function, type 0
    wasm += bytes([0x03, 0x02, 0x01, 0x00])
    # Table section: one table with funcref, min=1
    wasm += bytes([0x04, 0x04, 0x01, 0x70, 0x00, 0x01])
    # Export section: export "table" as table 0
    wasm += bytes([0x07, 0x08, 0x01,
                   0x05, 0x74, 0x61, 0x62, 0x6c, 0x65,  # "table"
                   0x01, 0x00])  # kind=table, index=0
    # Code section: one empty function body
    wasm += bytes([0x0a, 0x04, 0x01, 0x02, 0x00, 0x0b])
    return bytes(wasm)


def make_wasm_no_exports():
    """Create WASM with no exports."""
    wasm = bytearray(WASM_MAGIC + WASM_VERSION)
    # Type section
    wasm += bytes([0x01, 0x04, 0x01, 0x60, 0x00, 0x00])
    # Function section
    wasm += bytes([0x03, 0x02, 0x01, 0x00])
    # Code section
    wasm += bytes([0x0a, 0x04, 0x01, 0x02, 0x00, 0x0b])
    return bytes(wasm)


# ---- Tests ----

def test_gen_wli_basic():
    """Test basic WLI generation from a WASM file with one export."""
    from gen_wli import generate_wli

    with tempfile.NamedTemporaryFile(suffix='.Wdll', delete=False, prefix='gpu_driver') as f:
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

        found = any(e['name'] == 'test_func' for e in wli['exports'])
        assert found, "test_func not found in exports"

        print("PASS: test_gen_wli_basic")
    finally:
        os.unlink(wasm_path)
        if os.path.exists(wli_path):
            os.unlink(wli_path)


def test_gen_wli_with_imports():
    """Test WLI generation with WASM that has imports."""
    from gen_wli import generate_wli

    with tempfile.NamedTemporaryFile(suffix='.Wdll', delete=False, prefix='test_mod') as f:
        f.write(make_wasm_with_imports([("env", "malloc", 0), ("webos", "syscall", 0)]))
        wasm_path = f.name

    wli_path = wasm_path.rsplit('.', 1)[0] + '.wli'

    try:
        generate_wli(wasm_path, wli_path)

        with open(wli_path, 'r') as f:
            wli = json.load(f)

        assert len(wli['imports']) >= 2, f"Expected at least 2 imports, got {len(wli['imports'])}"

        import_names = [i['name'] for i in wli['imports']]
        assert 'malloc' in import_names, f"malloc not in imports: {import_names}"
        assert 'syscall' in import_names, f"syscall not in imports: {import_names}"

        # Verify 'from' field
        malloc_imp = next(i for i in wli['imports'] if i['name'] == 'malloc')
        assert malloc_imp['from'] == 'env', f"Expected 'env', got '{malloc_imp['from']}'"

        print("PASS: test_gen_wli_with_imports")
    finally:
        os.unlink(wasm_path)
        if os.path.exists(wli_path):
            os.unlink(wli_path)


def test_gen_wli_multiple_exports():
    """Test WLI generation with multiple exports."""
    from gen_wli import generate_wli

    exports = [("func_a", 0), ("func_b", 0), ("func_c", 0)]
    with tempfile.NamedTemporaryFile(suffix='.Wdll', delete=False, prefix='multi') as f:
        f.write(make_wasm_with_exports(exports))
        wasm_path = f.name

    wli_path = wasm_path.rsplit('.', 1)[0] + '.wli'

    try:
        generate_wli(wasm_path, wli_path)

        with open(wli_path, 'r') as f:
            wli = json.load(f)

        assert len(wli['exports']) == 3, f"Expected 3 exports, got {len(wli['exports'])}"

        export_names = [e['name'] for e in wli['exports']]
        assert 'func_a' in export_names
        assert 'func_b' in export_names
        assert 'func_c' in export_names

        print("PASS: test_gen_wli_multiple_exports")
    finally:
        os.unlink(wasm_path)
        if os.path.exists(wli_path):
            os.unlink(wli_path)


def test_gen_wli_no_exports():
    """Test WLI generation with WASM that has no exports."""
    from gen_wli import generate_wli

    with tempfile.NamedTemporaryFile(suffix='.Wdll', delete=False, prefix='noexp') as f:
        f.write(make_wasm_no_exports())
        wasm_path = f.name

    wli_path = wasm_path.rsplit('.', 1)[0] + '.wli'

    try:
        generate_wli(wasm_path, wli_path)

        with open(wli_path, 'r') as f:
            wli = json.load(f)

        assert len(wli['exports']) == 0, f"Expected 0 exports, got {len(wli['exports'])}"

        print("PASS: test_gen_wli_no_exports")
    finally:
        os.unlink(wasm_path)
        if os.path.exists(wli_path):
            os.unlink(wli_path)


def test_gen_wli_table_export():
    """Test WLI generation with table exports."""
    from gen_wli import generate_wli

    with tempfile.NamedTemporaryFile(suffix='.Wdll', delete=False, prefix='tblmod') as f:
        f.write(make_wasm_with_table_export())
        wasm_path = f.name

    wli_path = wasm_path.rsplit('.', 1)[0] + '.wli'

    try:
        generate_wli(wasm_path, wli_path)

        with open(wli_path, 'r') as f:
            wli = json.load(f)

        table_exports = [e for e in wli['exports'] if 'table' in e.get('signature', '')]
        assert len(table_exports) > 0, "No table exports found"

        print("PASS: test_gen_wli_table_export")
    finally:
        os.unlink(wasm_path)
        if os.path.exists(wli_path):
            os.unlink(wli_path)


def test_gen_wli_memory_export():
    """Test WLI generation with memory exports."""
    from gen_wli import generate_wli

    with tempfile.NamedTemporaryFile(suffix='.Wdll', delete=False, prefix='memmod') as f:
        f.write(make_wasm_with_memory_export())
        wasm_path = f.name

    wli_path = wasm_path.rsplit('.', 1)[0] + '.wli'

    try:
        generate_wli(wasm_path, wli_path)

        with open(wli_path, 'r') as f:
            wli = json.load(f)

        memory_exports = [e for e in wli['exports'] if 'memory' in e.get('signature', '')]
        assert len(memory_exports) > 0, "No memory exports found"

        print("PASS: test_gen_wli_memory_export")
    finally:
        os.unlink(wasm_path)
        if os.path.exists(wli_path):
            os.unlink(wli_path)


def test_gen_wli_invalid_wasm():
    """Test WLI generation with an invalid WASM file."""
    from gen_wli import generate_wli

    with tempfile.NamedTemporaryFile(suffix='.Wdll', delete=False) as f:
        f.write(b'not a wasm file at all')
        wasm_path = f.name

    try:
        try:
            generate_wli(wasm_path)
            assert False, "Should have raised SystemExit for invalid WASM"
        except SystemExit:
            pass
        print("PASS: test_gen_wli_invalid_wasm")
    finally:
        os.unlink(wasm_path)


def test_gen_wli_empty_file():
    """Test WLI generation with an empty file."""
    from gen_wli import generate_wli

    with tempfile.NamedTemporaryFile(suffix='.Wdll', delete=False) as f:
        f.write(b'')
        wasm_path = f.name

    try:
        try:
            generate_wli(wasm_path)
            assert False, "Should have raised SystemExit for empty file"
        except (SystemExit, IndexError):
            pass
        print("PASS: test_gen_wli_empty_file")
    finally:
        os.unlink(wasm_path)


def test_gen_wli_custom_output_path():
    """Test WLI generation with a custom output path."""
    from gen_wli import generate_wli

    with tempfile.NamedTemporaryFile(suffix='.Wdll', delete=False, prefix='custom') as f:
        f.write(make_wasm_with_exports())
        wasm_path = f.name

    custom_output = wasm_path + '.custom.wli'

    try:
        generate_wli(wasm_path, custom_output)

        assert os.path.exists(custom_output), "Custom output file not created"

        with open(custom_output, 'r') as f:
            wli = json.load(f)

        assert 'module' in wli
        print("PASS: test_gen_wli_custom_output_path")
    finally:
        os.unlink(wasm_path)
        if os.path.exists(custom_output):
            os.unlink(custom_output)


def test_gen_wli_module_name_extraction():
    """Test that module name is extracted correctly from filename."""
    from gen_wli import generate_wli, get_module_name

    # Test various filenames
    assert get_module_name("/path/to/gpu_driver.Wdll") == "gpu_driver"
    assert get_module_name("simple.wli") == "simple"
    assert get_module_name("/apps/calculator.wex") == "calculator"
    assert get_module_name("my.module.Wdll") == "my.module"

    # Now test through generate_wli
    with tempfile.NamedTemporaryFile(suffix='.Wdll', delete=False, prefix='gpu_driver') as f:
        f.write(make_wasm_with_exports())
        wasm_path = f.name

    wli_path = wasm_path.rsplit('.', 1)[0] + '.wli'

    try:
        generate_wli(wasm_path, wli_path)
        with open(wli_path, 'r') as f:
            wli = json.load(f)
        # Module name should be extracted from the filename
        assert 'module' in wli
        assert len(wli['module']) > 0
        print("PASS: test_gen_wli_module_name_extraction")
    finally:
        os.unlink(wasm_path)
        if os.path.exists(wli_path):
            os.unlink(wli_path)


def test_gen_wli_json_valid():
    """Test that the generated WLI file is valid JSON."""
    from gen_wli import generate_wli

    with tempfile.NamedTemporaryFile(suffix='.Wdll', delete=False, prefix='jsonval') as f:
        f.write(make_wasm_with_exports())
        wasm_path = f.name

    wli_path = wasm_path.rsplit('.', 1)[0] + '.wli'

    try:
        generate_wli(wasm_path, wli_path)

        with open(wli_path, 'r') as f:
            wli = json.load(f)

        # Verify structure
        assert isinstance(wli['module'], str)
        assert isinstance(wli['exports'], list)
        assert isinstance(wli['imports'], list)

        for exp in wli['exports']:
            assert isinstance(exp['name'], str)
            assert isinstance(exp['signature'], str)

        for imp in wli['imports']:
            assert isinstance(imp['name'], str)
            assert isinstance(imp['from'], str)

        print("PASS: test_gen_wli_json_valid")
    finally:
        os.unlink(wasm_path)
        if os.path.exists(wli_path):
            os.unlink(wli_path)


if __name__ == "__main__":
    test_gen_wli_basic()
    test_gen_wli_with_imports()
    test_gen_wli_multiple_exports()
    test_gen_wli_no_exports()
    test_gen_wli_table_export()
    test_gen_wli_memory_export()
    test_gen_wli_invalid_wasm()
    test_gen_wli_empty_file()
    test_gen_wli_custom_output_path()
    test_gen_wli_module_name_extraction()
    test_gen_wli_json_valid()
    print("\nAll gen_wli tests passed!")
