"""Run against the installed wheel, from outside the source tree; stdlib only."""
import concurrent.futures
import importlib.metadata
import json
import math
import os
from pathlib import Path
import subprocess
import sys
import unittest

import hwspec

SECTIONS = ["cpu", "mem", "os", "gpu", "disk", "net", "battery"]


def command(*args, console=False):
    executable = [str(Path(sys.executable).parent / "hwspec.exe")] if console else [sys.executable, "-m", "hwspec"]
    return subprocess.run(executable + list(args), capture_output=True, encoding="utf-8", timeout=35)


class PythonPackageTests(unittest.TestCase):
    def test_version_and_metadata(self):
        self.assertEqual(hwspec.__version__, importlib.metadata.version("hwspec"))
        result = command("--version")
        self.assertEqual(result.returncode, 0)
        self.assertEqual(result.stdout.strip(), "hwspec " + hwspec.__version__)
        self.assertEqual(result.stderr, "")

    def test_all_normal_sections_and_types(self):
        result = hwspec.collect()
        self.assertEqual(list(result), SECTIONS)
        self.assertIsInstance(result["cpu"], dict)
        self.assertIsInstance(result["gpu"]["adapters"], list)
        self.assertIsInstance(result["disk"]["drives"], list)
        self.assertNotIn("modules", result["mem"])
        self.assertNotIn("max_boost_clock_ghz", result["cpu"])
        for key in ("physical_cores", "logical_cores"):
            value = result["cpu"][key]
            if value is not None:
                self.assertIsInstance(value, int)
                self.assertGreater(value, 0)

    def test_single_selection(self):
        self.assertEqual(list(hwspec.collect("cpu")), ["cpu"])

    def test_combination_duplicates_and_order(self):
        result = hwspec.collect(["battery", "cpu", "cpu"], detail=True)
        self.assertEqual(list(result), ["cpu", "battery"])
        self.assertIn("max_boost_clock_ghz", result["cpu"])
        self.assertIn("cycle_count", result["battery"])

    def test_all_overrides_and_empty_selection(self):
        self.assertEqual(list(hwspec.collect(["cpu", "all"])), SECTIONS)
        self.assertEqual(list(hwspec.collect([])), SECTIONS)

    def test_api_rejects_control_flags_and_unknown_sections(self):
        for name in ("--elevate", "elevate", "help", "no-such-section", "--cpu"):
            with self.subTest(name=name), self.assertRaises(ValueError):
                hwspec.collect(name)

    def test_non_string_names(self):
        with self.assertRaises(TypeError):
            hwspec.collect([None])
        with self.assertRaises(TypeError):
            hwspec.collect(42)

    def test_detailed_skip_admin_and_missing_data(self):
        result = hwspec.collect(["disk", "battery"], detail=True, skip_admin=True)
        self.assertEqual(result["disk"]["disk_health"], {"status": "requires_admin", "devices": []})
        battery = result["battery"]
        self.assertTrue(battery["present"] is None or isinstance(battery["present"], bool))
        design, full = battery["design_capacity_wh"], battery["full_charge_capacity_wh"]
        if design and full is not None:
            self.assertTrue(math.isclose(battery["health_percent"], 100 * full / design, rel_tol=1e-6))
        else:
            self.assertIsNone(battery["health_percent"])

    def test_concurrent_library_calls(self):
        with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:
            results = list(pool.map(lambda section: hwspec.collect(section), ["cpu", "os"]))
        self.assertEqual(list(results[0]), ["cpu"])
        self.assertEqual(list(results[1]), ["os"])

    def test_api_does_not_write_to_process_streams(self):
        result = subprocess.run([sys.executable, "-c", "import hwspec; hwspec.collect('disk', detail=True, skip_admin=True)"],
                                capture_output=True, encoding="utf-8", timeout=35)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout, "")
        self.assertEqual(result.stderr, "")

    def test_module_json_and_pause_are_noninteractive(self):
        result = command("--disk", "-d", "--json", "--pause", "--skip-admin")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stderr, "")
        self.assertEqual(json.loads(result.stdout)["disk"]["disk_health"]["status"], "requires_admin")

    def test_console_entry_point(self):
        result = command("--cpu", "--json", console=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stderr, "")
        self.assertEqual(list(json.loads(result.stdout)), ["cpu"])

    def test_invalid_cli_option(self):
        result = command("--bogus")
        self.assertEqual(result.returncode, 2)
        self.assertEqual(result.stdout, "")
        self.assertIn("unknown option", result.stderr)

    def test_help_and_text(self):
        help_result = command("--help")
        self.assertEqual(help_result.returncode, 0)
        self.assertIn("--elevate", help_result.stdout)
        result = command("--cpu")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertTrue(result.stdout.startswith("CPU\n"))
        self.assertNotIn("Memory\n", result.stdout)

    @unittest.skipUnless(os.environ.get("HWSPEC_NATIVE_EXE"), "standalone comparison is opt-in")
    def test_standalone_static_value_parity(self):
        result = subprocess.run([os.environ["HWSPEC_NATIVE_EXE"], "--cpu", "--os", "--json"],
                                check=True, capture_output=True, encoding="utf-8", timeout=35)
        self.assertEqual(json.loads(result.stdout), hwspec.collect(["cpu", "os"]))


if __name__ == "__main__":
    unittest.main(verbosity=2)
