#!/usr/bin/env python3
from __future__ import annotations

import unittest
from pathlib import Path


BRANCH_ROOT = Path(__file__).resolve().parent


class GateLauncherDispatcherTests(unittest.TestCase):
    def assert_dispatcher(self, rel_path: str, outer_name: str) -> None:
        source = (BRANCH_ROOT / rel_path).read_text(encoding="utf-8")
        self.assertIn(
            'SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"',
            source,
            msg=f"{rel_path} must resolve its branch-local directory before dispatch",
        )
        self.assertIn(
            f'outer_suite_wrappers/{outer_name}',
            source,
            msg=f"{rel_path} must dispatch to outer_suite_wrappers/{outer_name}",
        )
        self.assertIn(
            '"$@"',
            source,
            msg=f"{rel_path} must forward the caller arguments unchanged",
        )

    def test_strong_gate_launcher_dispatches_to_outer_wrapper(self) -> None:
        self.assert_dispatcher("lca_strong_gate.sh", "lca_strong_gate.sh")

    def test_boj3s_gate_launcher_dispatches_to_outer_wrapper(self) -> None:
        self.assert_dispatcher("lca_boj3s_gate.sh", "lca_boj3s_gate.sh")

    def test_required_repeatability_launcher_dispatches_to_outer_wrapper(self) -> None:
        self.assert_dispatcher("lca_required_repeatability.sh", "lca_required_repeatability.sh")

    def test_acceptance_repeatability_launcher_dispatches_to_outer_wrapper(self) -> None:
        self.assert_dispatcher("lca_acceptance_repeatability.sh", "lca_acceptance_repeatability.sh")

    def test_smoke_repeatability_launcher_dispatches_to_outer_wrapper(self) -> None:
        self.assert_dispatcher("lca_smoke_repeatability.sh", "lca_smoke_repeatability.sh")


if __name__ == "__main__":
    unittest.main()
