"""Regression check for the --readers batch runner command preparation."""

from __future__ import annotations

import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
import unittest


TEST_DIR = Path(__file__).resolve().parent
REPO_ROOT = TEST_DIR.parents[2]
RUNNER = TEST_DIR / "run_tests.bat"


@unittest.skipUnless(os.name == "nt", "requires Windows cmd.exe and batch files")
class ReaderRunnerFixture(unittest.TestCase):
    def test_fixture_executables_are_current_directory_qualified(self) -> None:
        invocations = []
        unqualified = []
        for line in RUNNER.read_text(encoding="utf-8").splitlines():
            match = re.match(r"^\s*(\.\\)?[\w.-]+\.exe(?=\s|$)", line, re.IGNORECASE)
            if match:
                invocations.append(line.strip())
                if match.group(1) is None:
                    unqualified.append(line.strip())

        self.assertTrue(invocations)
        self.assertEqual(unqualified, [])
        self.assertIn(r".\reader_fixture.exe", invocations)
        self.assertIn(r".\reader_fixture.exe --installed-profile", invocations)

    def test_readers_sources_reach_compiler_through_response_file(self) -> None:
        with tempfile.TemporaryDirectory(prefix="ffx-reader-runner-") as temp:
            temp_root = Path(temp)
            build_dir = temp_root / "isolated build output"
            tools_dir = temp_root / "fake compiler"
            tools_dir.mkdir()
            capture_path = temp_root / "compiler-capture.json"
            fake_compiler_py = temp_root / "fake_compiler.py"
            fake_compiler_py.write_text(
                "import json, os, pathlib, sys\n"
                "rsp_args = [arg[1:] for arg in sys.argv[1:] if arg.startswith('@')]\n"
                "payload = {'args': sys.argv[1:], 'responses': {name: pathlib.Path(name).read_text().splitlines() for name in rsp_args}}\n"
                "pathlib.Path(os.environ['FFX_FAKE_COMPILER_CAPTURE']).write_text(json.dumps(payload))\n"
                "raise SystemExit(23)\n",
                encoding="utf-8",
            )
            (tools_dir / "cl.bat").write_text(
                '@echo off\r\npython "%FFX_FAKE_COMPILER_SCRIPT%" %*\r\nexit /b %errorlevel%\r\n',
                encoding="ascii",
            )
            vcvars = temp_root / "vcvarsall.bat"
            vcvars.write_text("@echo off\r\nexit /b 0\r\n", encoding="ascii")

            repeated_root = str(REPO_ROOT) + "\\.\\" * 96
            runner_path = repeated_root + "src\\native\\tests\\run_tests.bat"
            env = os.environ.copy()
            env.update(
                {
                    "PATH": str(tools_dir) + os.pathsep + env.get("PATH", ""),
                    "VCVARSALL": str(vcvars),
                    "FFX_TEST_BUILD": str(build_dir),
                    "FFX_FAKE_COMPILER_SCRIPT": str(fake_compiler_py),
                    "FFX_FAKE_COMPILER_CAPTURE": str(capture_path),
                }
            )
            result = subprocess.run(
                ["cmd.exe", "/d", "/c", runner_path, "--readers"],
                cwd=REPO_ROOT,
                env=env,
                text=True,
                capture_output=True,
                check=False,
            )

            self.assertTrue(capture_path.is_file(), result.stdout + result.stderr)
            capture = json.loads(capture_path.read_text(encoding="utf-8"))
            response_args = [arg for arg in capture["args"] if arg.startswith("@")]
            self.assertEqual(response_args, ["@reader_fixture.rsp"])
            self.assertEqual(result.returncode, 23, result.stdout + result.stderr)
            response_lines = capture["responses"]["reader_fixture.rsp"]
            source_args = [line[1:-1] for line in response_lines if line.startswith('"') and line.endswith('"')]

            self.assertEqual(len(source_args), 87)
            self.assertGreater(len(runner_path), 260)
            self.assertGreaterEqual(runner_path.count("\\.\\"), 96)
            self.assertTrue(source_args[0].endswith("reader_fixture.cpp"))
            self.assertTrue(source_args[-1].endswith("utility_text_plans.cpp"))
            self.assertIn("/DFFX_READER_TEST", response_lines)
            self.assertIn("/link", response_lines)
            self.assertEqual(build_dir.parent, temp_root)
            self.assertTrue(build_dir.is_dir())


if __name__ == "__main__":
    unittest.main()
