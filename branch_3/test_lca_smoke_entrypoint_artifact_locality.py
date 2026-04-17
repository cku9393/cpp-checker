#!/usr/bin/env python3
from __future__ import annotations

import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parent
LCA_SMOKE_SOURCE = (ROOT / "lca_smoke.sh").read_text(encoding="utf-8")


class LcaSmokeEntrypointArtifactLocalityTests(unittest.TestCase):
    def test_xtrace_log_is_pinned_to_branch_artifacts(self) -> None:
        self.assertIn(
            'mkdir -p "$SCRIPT_DIR/artifacts"',
            LCA_SMOKE_SOURCE,
            msg="top-level lca_smoke xtrace must create its trace root under the wrapper's branch-local artifacts tree",
        )
        self.assertIn(
            'exec 9>>"$SCRIPT_DIR/artifacts/trace.log"',
            LCA_SMOKE_SOURCE,
            msg="top-level lca_smoke xtrace must keep trace.log under branch-local artifacts instead of the caller cwd",
        )


if __name__ == "__main__":
    unittest.main()
