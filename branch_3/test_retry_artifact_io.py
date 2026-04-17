#!/usr/bin/env python3
from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parent
OUROBOROS_ROOT = ROOT / ".ouroboros"
if str(OUROBOROS_ROOT) not in sys.path:
    sys.path.insert(0, str(OUROBOROS_ROOT))

import retry_artifact_io  # type: ignore


class RetryArtifactIoTests(unittest.TestCase):
    def setUp(self) -> None:
        self.tempdir = tempfile.TemporaryDirectory()
        self.branch_root = Path(self.tempdir.name) / "branch_3"
        self.artifacts_root = self.branch_root / "artifacts"
        self.branch_root.mkdir(parents=True, exist_ok=True)
        self.artifacts_root.mkdir(parents=True, exist_ok=True)

    def tearDown(self) -> None:
        self.tempdir.cleanup()

    def ensure_under_artifacts(self, path_like: str | Path) -> Path:
        path = Path(path_like).resolve()
        try:
            path.relative_to(self.artifacts_root.resolve())
        except ValueError as exc:
            raise ValueError(
                f"output path must stay under {self.artifacts_root.resolve()}: {path}"
            ) from exc
        return path

    def test_resolve_branch_path_collapses_branch_prefixed_artifact_paths(self) -> None:
        raw = (
            f"{self.branch_root.name}/artifacts/artifacts/"
            "lca_tree_stress_v5/retry_loop/latest_failure_report.md"
        )

        resolved = retry_artifact_io.resolve_branch_path(self.branch_root, raw)

        self.assertEqual(
            resolved,
            (
                self.artifacts_root
                / "lca_tree_stress_v5"
                / "retry_loop"
                / "latest_failure_report.md"
            ).resolve(),
        )

    def test_resolve_branch_path_collapses_branch_prefixed_ouroboros_paths(self) -> None:
        raw = f"{self.branch_root.name}/.ouroboros/failure_analysis_state.json"

        resolved = retry_artifact_io.resolve_branch_path(self.branch_root, raw)

        self.assertEqual(
            resolved,
            (self.branch_root / ".ouroboros" / "failure_analysis_state.json").resolve(),
        )

    def test_resolve_artifact_output_path_accepts_branch_prefixed_artifact_paths(self) -> None:
        raw = f"{self.branch_root.name}/artifacts/lca_tree_stress_v5/retry_loop/soft_stop_request.json"

        resolved = retry_artifact_io.resolve_artifact_output_path(
            self.branch_root,
            raw,
            self.ensure_under_artifacts,
        )

        self.assertEqual(
            resolved,
            (
                self.artifacts_root
                / "lca_tree_stress_v5"
                / "retry_loop"
                / "soft_stop_request.json"
            ).resolve(),
        )

    def test_resolve_artifact_output_path_rejects_non_artifact_paths(self) -> None:
        with self.assertRaisesRegex(ValueError, "output path must stay under"):
            retry_artifact_io.resolve_artifact_output_path(
                self.branch_root,
                ".ouroboros/soft_stop_request.json",
                self.ensure_under_artifacts,
            )


if __name__ == "__main__":
    unittest.main()
