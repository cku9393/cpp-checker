#!/usr/bin/env python3
from __future__ import annotations

import re
import unittest
from pathlib import Path


WRAPPER_PATH = Path(__file__).resolve().parent / "outer_suite_wrappers" / "lca_smoke.sh"
WRAPPER_SOURCE = WRAPPER_PATH.read_text(encoding="utf-8")


class LcaSmokeWrapperRegressionTests(unittest.TestCase):
    def assert_helper_success_branch_returns_zero(self, helper_name: str, status_name: str) -> None:
        pattern = rf"""
            {re.escape(helper_name)}\(\)\s*\{{.*?
            record_setup_check\s+"{re.escape(status_name)}"\s+"\$label"\s+"\$path"\s*
            return\s+0
        """
        self.assertRegex(
            WRAPPER_SOURCE,
            re.compile(pattern, re.DOTALL | re.VERBOSE),
            msg=f"{helper_name} must return success after recording {status_name}",
        )

    def test_file_preflight_helper_returns_success_after_recording_present_file(self) -> None:
        self.assert_helper_success_branch_returns_zero("check_required_file_recorded", "file")

    def test_executable_preflight_helper_returns_success_after_recording_present_executable(self) -> None:
        self.assert_helper_success_branch_returns_zero("check_required_executable_recorded", "executable")


if __name__ == "__main__":
    unittest.main()
