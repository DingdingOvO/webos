#!/usr/bin/env python3
"""
WebOS Test Report Aggregator

Runs all C and Python tests, captures output, and generates a summary report.
Exit code: 0 if all pass, 1 if any fail.
"""

import subprocess
import sys
import os
import time
from dataclasses import dataclass, field


@dataclass
class TestResult:
    name: str
    passed: bool
    output: str = ""
    duration: float = 0.0
    error: str = ""


@dataclass
class TestSuite:
    name: str
    results: list = field(default_factory=list)

    @property
    def passed(self) -> int:
        return sum(1 for r in self.results if r.passed)

    @property
    def failed(self) -> int:
        return sum(1 for r in self.results if not r.passed)

    @property
    def total(self) -> int:
        return len(self.results)


def run_command(cmd, cwd=None, timeout=120):
    """Run a command and return the result."""
    try:
        result = subprocess.run(
            cmd,
            cwd=cwd,
            capture_output=True,
            text=True,
            timeout=timeout,
        )
        return result.returncode, result.stdout + result.stderr
    except subprocess.TimeoutExpired:
        return -1, "TIMEOUT"
    except Exception as e:
        return -1, str(e)


def run_c_tests(test_dir):
    """Build and run all C tests."""
    suite = TestSuite(name="C Unit Tests")

    # Build all C tests first
    print("  Building C tests...")
    rc, output = run_command(["make", "c-tests"], cwd=test_dir, timeout=300)
    if rc != 0:
        result = TestResult("C Build", False, output, 0, "Build failed")
        suite.results.append(result)
        return suite

    # Run individual C test binaries
    c_binaries = [
        "test_memory", "test_process", "test_ipc", "test_scheduler",
        "test_syscall", "test_dynlink",
        "test_memory_extended", "test_process_extended", "test_ipc_extended",
        "test_syscall_extended", "test_scheduler_extended", "test_dynlink_extended",
        "test_integration_kernel",
        "test_driver_stubs", "test_services",
    ]

    for binary in c_binaries:
        binary_path = os.path.join(test_dir, binary)
        if not os.path.exists(binary_path):
            # Skip if not built
            continue

        start = time.time()
        rc, output = run_command([binary_path], cwd=test_dir)
        duration = time.time() - start

        result = TestResult(
            name=binary,
            passed=(rc == 0),
            output=output,
            duration=duration,
        )
        suite.results.append(result)

    return suite


def run_python_tests(test_dir):
    """Run all Python tests."""
    suite = TestSuite(name="Python Tool Tests")

    py_tests = ["test_add_module_section.py", "test_gen_wli.py"]

    for test_file in py_tests:
        test_path = os.path.join(test_dir, test_file)
        if not os.path.exists(test_path):
            continue

        start = time.time()
        rc, output = run_command([sys.executable, test_path], cwd=test_dir)
        duration = time.time() - start

        result = TestResult(
            name=test_file,
            passed=(rc == 0),
            output=output,
            duration=duration,
        )
        suite.results.append(result)

    return suite


def run_typescript_tests(test_dir):
    """Run all TypeScript tests (if tsx is available)."""
    suite = TestSuite(name="TypeScript Integration Tests")

    ts_tests = [
        "test_boot_sequence.ts",
        "test_dynamic_loader.ts",
        "test_runtime_bridge.ts",
        "test_types.ts",
    ]

    integration_dir = os.path.join(test_dir, "integration")

    for test_file in ts_tests:
        test_path = os.path.join(integration_dir, test_file)
        if not os.path.exists(test_path):
            continue

        start = time.time()
        rc, output = run_command(
            ["npx", "tsx", test_file],
            cwd=integration_dir,
            timeout=60,
        )
        duration = time.time() - start

        result = TestResult(
            name=test_file,
            passed=(rc == 0),
            output=output,
            duration=duration,
        )
        suite.results.append(result)

    return suite


def print_report(suites):
    """Print the summary report."""
    total_passed = 0
    total_failed = 0
    total_time = 0.0

    print("\n")
    print("╔══════════════════════════════════════════════════════════════╗")
    print("║              WebOS Test Suite - Summary Report              ║")
    print("║                    v0.0.1beta                               ║")
    print("╚══════════════════════════════════════════════════════════════╝")
    print()

    for suite in suites:
        if suite.total == 0:
            continue

        status = "✓" if suite.failed == 0 else "✗"
        print(f"  {status} {suite.name}")
        print(f"    Passed: {suite.passed}/{suite.total}")

        for result in suite.results:
            icon = "✓" if result.passed else "✗"
            duration_str = f"{result.duration:.2f}s" if result.duration > 0 else ""
            print(f"      {icon} {result.name} {duration_str}")
            if not result.passed and result.error:
                print(f"         ERROR: {result.error}")

        total_passed += suite.passed
        total_failed += suite.failed
        total_time += sum(r.duration for r in suite.results)
        print()

    print("══════════════════════════════════════════════════════════════")
    print(f"  Total: {total_passed + total_failed} tests")
    print(f"  Passed: {total_passed}")
    print(f"  Failed: {total_failed}")
    print(f"  Duration: {total_time:.2f}s")

    if total_failed == 0:
        print("\n  🎉 ALL TESTS PASSED! 🎉\n")
    else:
        print(f"\n  ⚠️  {total_failed} TEST(S) FAILED ⚠️\n")

    print("══════════════════════════════════════════════════════════════")

    return total_failed


def main():
    test_dir = os.path.dirname(os.path.abspath(__file__))

    print("WebOS Test Report Generator")
    print("=" * 50)

    suites = []

    # Run C tests
    print("\n[1/3] Running C tests...")
    c_suite = run_c_tests(test_dir)
    suites.append(c_suite)
    print(f"  C tests: {c_suite.passed} passed, {c_suite.failed} failed")

    # Run Python tests
    print("\n[2/3] Running Python tests...")
    py_suite = run_python_tests(test_dir)
    suites.append(py_suite)
    print(f"  Python tests: {py_suite.passed} passed, {py_suite.failed} failed")

    # Run TypeScript tests
    print("\n[3/3] Running TypeScript tests...")
    ts_suite = run_typescript_tests(test_dir)
    suites.append(ts_suite)
    print(f"  TypeScript tests: {ts_suite.passed} passed, {ts_suite.failed} failed")

    # Print report
    total_failed = print_report(suites)

    sys.exit(1 if total_failed > 0 else 0)


if __name__ == "__main__":
    main()
