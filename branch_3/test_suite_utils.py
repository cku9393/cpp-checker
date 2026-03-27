#!/usr/bin/env python3
from __future__ import annotations

import os
import signal
import sys
import types
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory
from unittest import mock

os.environ["PYTHONDONTWRITEBYTECODE"] = "1"
sys.dont_write_bytecode = True

from artifact_paths import branch_tmp_root, configure_branch_process_env, ensure_under_artifacts
import suite_utils


configure_branch_process_env()

TEST_TMP_ROOT = ensure_under_artifacts(branch_tmp_root() / "python_unittest" / "suite_utils")


def artifact_tempdir() -> TemporaryDirectory[str]:
    TEST_TMP_ROOT.mkdir(parents=True, exist_ok=True)
    return TemporaryDirectory(dir=TEST_TMP_ROOT, prefix="run_solver_posix.")


@unittest.skipUnless(not suite_utils.IS_WINDOWS and hasattr(os, "wait4"), "POSIX wait4 only")
class RunSolverPosixTests(unittest.TestCase):
    def _make_paths(self, tmpdir: str) -> tuple[Path, Path, Path, Path]:
        root = Path(tmpdir)
        solver = root / "solver"
        solver.write_text("#!/bin/sh\nexit 0\n", encoding="utf-8")
        solver.chmod(0o755)
        in_path = root / "in.txt"
        in_path.write_text("", encoding="utf-8")
        return solver, in_path, root / "out.txt", root / "solver_stderr.txt"

    def test_run_solver_posix_returns_exit_status_and_rss_on_normal_exit(self) -> None:
        with artifact_tempdir() as tmpdir:
            solver, in_path, out_path, stderr_path = self._make_paths(tmpdir)
            proc = types.SimpleNamespace(pid=4321, returncode=None)
            rusage = types.SimpleNamespace(ru_maxrss=4096)

            with (
                mock.patch.object(suite_utils, "_spawn_process", return_value=proc),
                mock.patch.object(
                    suite_utils.os,
                    "wait4",
                    return_value=(proc.pid, 7 << 8, rusage),
                ) as wait4_mock,
                mock.patch.object(suite_utils.time, "perf_counter", side_effect=[10.0, 10.25]),
                mock.patch.object(suite_utils.time, "sleep") as sleep_mock,
                mock.patch.object(suite_utils, "_kill_process") as kill_mock,
            ):
                rc, timed_out, sec, rss_kb = suite_utils._run_solver_posix(
                    solver,
                    in_path,
                    out_path,
                    stderr_path,
                    timeout=1.0,
                    env=None,
                    cwd=None,
                )

        self.assertEqual(rc, 7)
        self.assertFalse(timed_out)
        self.assertEqual(sec, 0.25)
        self.assertEqual(rss_kb, suite_utils._normalize_posix_rss_kb(4096))
        self.assertEqual(proc.returncode, 7)
        self.assertEqual(wait4_mock.call_count, 1)
        kill_mock.assert_not_called()
        sleep_mock.assert_not_called()

    def test_run_solver_posix_uses_proc_wait_on_timeout(self) -> None:
        with artifact_tempdir() as tmpdir:
            solver, in_path, out_path, stderr_path = self._make_paths(tmpdir)

            class FakeProc:
                def __init__(self) -> None:
                    self.pid = 9876
                    self.returncode = None
                    self.wait_calls = 0

                def wait(self) -> int:
                    self.wait_calls += 1
                    self.returncode = -signal.SIGKILL
                    return self.returncode

            proc = FakeProc()
            rusage = types.SimpleNamespace(ru_maxrss=0)

            with (
                mock.patch.object(suite_utils, "_spawn_process", return_value=proc),
                mock.patch.object(
                    suite_utils.os,
                    "wait4",
                    side_effect=[(0, 0, rusage)],
                ) as wait4_mock,
                mock.patch.object(suite_utils.time, "perf_counter", side_effect=[20.0, 20.2, 20.3]),
                mock.patch.object(suite_utils.time, "sleep") as sleep_mock,
                mock.patch.object(suite_utils, "_kill_process") as kill_mock,
            ):
                rc, timed_out, sec, rss_kb = suite_utils._run_solver_posix(
                    solver,
                    in_path,
                    out_path,
                    stderr_path,
                    timeout=0.1,
                    env=None,
                    cwd=None,
                )

        self.assertEqual(rc, -signal.SIGKILL)
        self.assertTrue(timed_out)
        self.assertAlmostEqual(sec, 0.3, places=9)
        self.assertIsNone(rss_kb)
        self.assertEqual(proc.wait_calls, 1)
        self.assertEqual(wait4_mock.call_count, 1)
        kill_mock.assert_called_once_with(proc)
        sleep_mock.assert_not_called()


if __name__ == "__main__":
    unittest.main()
