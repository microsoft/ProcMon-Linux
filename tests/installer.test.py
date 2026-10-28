"""Run on Linux: python3 tests/installer.test.py (requires g++)."""

import pathlib
import re
import subprocess
import tempfile
import unittest


ROOT = pathlib.Path(__file__).resolve().parent.parent


class InstallerTests(unittest.TestCase):
    def test_private_extraction(self):
        with tempfile.TemporaryDirectory(prefix="procmon-installer-test-") as tmp:
            directory = pathlib.Path(tmp)
            (directory / "libsysinternalsEBPF.h").write_text(
                "#include <stdbool.h>\n#include <sys/types.h>\n"
                "bool dropFile(const char*, const char*, const char*, bool, mode_t);\n"
            )
            binary = directory / "installer-test"
            subprocess.run(
                [
                    "g++", "-std=c++17", "-Wall", "-Wextra", "-Werror",
                    "-I", tmp, str(ROOT / "src/installer.cpp"),
                    str(ROOT / "tests/installer.test.cpp"),
                    "-Wl,--wrap=mkdtemp", "-o", str(binary),
                ],
                check=True,
            )
            fixture = directory / "fixture"
            fixture.mkdir()
            subprocess.run([str(binary), str(fixture)], check=True, timeout=30)

    def test_loader_has_only_private_search_directory(self):
        source = (ROOT / "src/tracer/ebpf/ebpf_tracer_engine.cpp").read_text()
        paths = re.search(r"const char\* defPaths\[\] = \{([^}]+)\};", source)
        self.assertIsNotNone(paths)
        self.assertEqual(paths[1].strip(), "GetEBPFInstallDirectory().c_str()")
        self.assertRegex(source, r"sizeof\(defPaths\) / sizeof\(\*defPaths\),\s*defPaths,")

    def test_startup_rejects_extraction_failure(self):
        source = (ROOT / "src/procmon.cpp").read_text()
        gate = re.search(r"if \(!ExtractEBPFPrograms\(\)\)\s*\{([^}]+)\}", source)
        self.assertIsNotNone(gate)
        self.assertIn("return EXIT_FAILURE;", gate[1])
        self.assertLess(gate.start(), source.index("std::make_shared<ProcmonConfiguration>"))
        self.assertIn("std::atexit([] { DeleteEBPFPrograms(); })", source)
        self.assertRegex(source, r"config\.reset\(\);\s*if \(!DeleteEBPFPrograms\(\)\)")


if __name__ == "__main__":
    unittest.main()
