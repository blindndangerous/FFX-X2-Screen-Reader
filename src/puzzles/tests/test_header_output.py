"""Exercise overwrite protection without the unavailable generator evidence."""
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import header_output


class HeaderOutputTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.native = Path(self.directory.name) / "native"
        self.native.mkdir()
        self.patch = patch.object(header_output, "NATIVE", self.native)
        self.patch.start()
        self.addCleanup(self.patch.stop)
        self.target = self.native / "guard.hpp"
        self.target.write_text("maintained guard\n", encoding="utf8")

    def test_changed_header_refused_without_modifying_bytes(self):
        before = self.target.read_bytes()
        with self.assertRaisesRegex(SystemExit, "guard.hpp.*--overwrite-maintained"):
            header_output.HeaderOutput([]).write(self.target.name, "lost guard\n")
        self.assertEqual(self.target.read_bytes(), before)

    def test_changed_include_refused_too(self):
        target = self.native / "utility_warps_0.inc"
        target.write_text("maintained rooms\n", encoding="utf8")
        with self.assertRaises(SystemExit):
            header_output.HeaderOutput([]).write(target.name, "lost rooms\n")
        self.assertEqual(target.read_text(), "maintained rooms\n")

    def test_identical_content_keeps_existing_bytes_and_timestamp(self):
        self.target.write_bytes(b"same\r\n")
        before = self.target.stat().st_mtime_ns
        header_output.HeaderOutput([]).write(self.target.name, "same\n")
        self.assertEqual(self.target.read_bytes(), b"same\r\n")
        self.assertEqual(self.target.stat().st_mtime_ns, before)

    def test_explicit_override_allows_changed_header(self):
        header_output.HeaderOutput(["--overwrite-maintained"]).write(self.target.name, "replacement\n")
        self.assertEqual(self.target.read_text(), "replacement\n")

    def test_new_header_allowed(self):
        header_output.HeaderOutput([]).write("new.hpp", "new\n")
        self.assertEqual((self.native / "new.hpp").read_text(), "new\n")

    def test_other_output_directory_can_overwrite_its_own_copy(self):
        output = Path(self.directory.name) / "other"
        writer = header_output.HeaderOutput(["--output-dir", str(output)])
        writer.write(self.target.name, "first\n")
        writer.write(self.target.name, "second\n")
        self.assertEqual((output / self.target.name).read_text(), "second\n")
        self.assertEqual(self.target.read_text(), "maintained guard\n")

    def test_explicit_native_directory_still_protected(self):
        writer = header_output.HeaderOutput(["--output-dir", str(self.native / ".." / "native")])
        with self.assertRaises(SystemExit):
            writer.write(self.target.name, "lost guard\n")

    def test_process_exit_codes(self):
        source = str(Path(header_output.__file__).parent)
        code = ("import sys; from pathlib import Path; "
                f"sys.path.insert(0, {source!r}); import header_output as h; "
                f"h.NATIVE=Path({str(self.native)!r}); "
                "h.HeaderOutput(sys.argv[1:]).write('guard.hpp', 'replacement\\n')")
        refused = subprocess.run([sys.executable, "-c", code], capture_output=True, text=True)
        self.assertEqual(refused.returncode, 1)
        self.assertIn(str(self.target), refused.stderr)
        allowed = subprocess.run([sys.executable, "-c", code, "--overwrite-maintained"],
                                 capture_output=True, text=True)
        self.assertEqual(allowed.returncode, 0, allowed.stderr)


if __name__ == "__main__":
    unittest.main()
