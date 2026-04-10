#!/usr/bin/env python3
from __future__ import annotations

import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parent
ARTIFACT_RESOLVER_SOURCE = (ROOT / "artifact_paths.py").read_text(encoding="utf-8")
RUN_WRAPPER_SOURCE = (ROOT / "run.sh").read_text(encoding="utf-8")
HUNT_WRAPPER_SOURCE = (ROOT / "outer_suite_wrappers" / "lca_hunt.sh").read_text(
    encoding="utf-8"
)


class DirectSolverEntrypointArtifactLocalityTests(unittest.TestCase):
    def test_artifact_resolver_disables_repo_local_bytecode_output(self) -> None:
        self.assertIn(
            'os.environ.setdefault("PYTHONDONTWRITEBYTECODE", "1")',
            ARTIFACT_RESOLVER_SOURCE,
            msg="artifact_paths.py must suppress repo-local bytecode when wrappers invoke it directly",
        )
        self.assertIn(
            "sys.dont_write_bytecode = True",
            ARTIFACT_RESOLVER_SOURCE,
            msg="artifact_paths.py must suppress __pycache__ writes outside artifacts when invoked directly",
        )

    def test_run_wrapper_routes_direct_solver_aux_output_under_artifacts(self) -> None:
        self.assertIn(
            'DENSE_PROFILE_OUTDIR="$(python3 "$ARTIFACT_RESOLVER" boj28350_direct_solver_aux "${DENSE_PROFILE_OUTDIR:-}")"',
            RUN_WRAPPER_SOURCE,
            msg="run.sh must canonicalize direct-solver auxiliary output through artifact_paths.py",
        )
        self.assertIn(
            'mkdir -p "$DENSE_PROFILE_OUTDIR"',
            RUN_WRAPPER_SOURCE,
            msg="run.sh must create the resolved artifact-rooted output directory before execution",
        )
        self.assertIn(
            'cd "$DENSE_PROFILE_OUTDIR"',
            RUN_WRAPPER_SOURCE,
            msg="run.sh must execute from the artifact-rooted output directory so incidental outputs stay local",
        )


class HuntWrapperArtifactLocalityTests(unittest.TestCase):
    def test_hunt_wrapper_resolves_output_root_via_artifact_paths(self) -> None:
        self.assertIn(
            'OUTDIR="$(python3 "$BRANCH/artifact_paths.py" lca_hunt "${1:-}")"',
            HUNT_WRAPPER_SOURCE,
            msg="lca_hunt wrapper must resolve its output root via the branch-local artifact resolver",
        )
        self.assertIn(
            '"$ARTIFACTS_ROOT"|"$ARTIFACTS_ROOT"/*)',
            HUNT_WRAPPER_SOURCE,
            msg="lca_hunt wrapper must reject output roots that escape branch-local artifacts",
        )
        self.assertIn(
            'mkdir -p "$OUTDIR"',
            HUNT_WRAPPER_SOURCE,
            msg="lca_hunt wrapper must create its resolved artifact-rooted output directory before execution",
        )
        self.assertIn(
            'cd "$OUTDIR"',
            HUNT_WRAPPER_SOURCE,
            msg="lca_hunt wrapper must execute inside the artifact-rooted output directory",
        )
        self.assertIn(
            'exec "$TOOLING_ROOT/hunt.sh" "$SOLVER" "$OUTDIR" "$SIZES" "$SEEDS" "$TIMEOUT"',
            HUNT_WRAPPER_SOURCE,
            msg="lca_hunt wrapper must keep the hunt helper bound to the resolved artifact-rooted output directory",
        )


if __name__ == "__main__":
    unittest.main()
