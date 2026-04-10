from __future__ import annotations

import os
import subprocess
import tempfile
import unittest
from unittest import mock
from pathlib import Path
import textwrap

import build


class CompilerCandidateTests(unittest.TestCase):
    def test_darwin_prefers_clang_before_gxx(self) -> None:
        with mock.patch.object(build, "IS_WINDOWS", False), \
             mock.patch.object(build.sys, "platform", "darwin"), \
             mock.patch.dict(os.environ, {}, clear=False):
            os.environ.pop("CXX", None)
            self.assertEqual(build._compiler_candidates(None), ["clang++", "g++", "c++"])

    def test_linux_keeps_gxx_first(self) -> None:
        with mock.patch.object(build, "IS_WINDOWS", False), \
             mock.patch.object(build.sys, "platform", "linux"), \
             mock.patch.dict(os.environ, {}, clear=False):
            os.environ.pop("CXX", None)
            self.assertEqual(build._compiler_candidates(None), ["g++", "clang++", "c++"])


class BuildWrapperEnvironmentTests(unittest.TestCase):
    def test_build_wrapper_scrubs_ambient_compiler_and_probe_flags_before_release_env(self) -> None:
        root = Path(__file__).resolve().parent
        build_wrapper = (root / "build.sh").read_text(encoding="utf-8")

        with tempfile.TemporaryDirectory() as tmp:
            branch_root = Path(tmp) / "branch"
            branch_root.mkdir()

            (branch_root / "build.sh").write_text(build_wrapper, encoding="utf-8")
            (branch_root / "build.sh").chmod(0o755)
            (branch_root / "artifact_paths.py").write_text(
                textwrap.dedent(
                    """
                    #!/usr/bin/env python3
                    from __future__ import annotations

                    import argparse
                    from pathlib import Path

                    BRANCH_ROOT = Path(__file__).resolve().parent
                    ARTIFACTS_ROOT = BRANCH_ROOT / "artifacts"

                    def main() -> int:
                        parser = argparse.ArgumentParser()
                        parser.add_argument("key")
                        parser.add_argument("path", nargs="?")
                        args = parser.parse_args()
                        if args.key != "boj28350_build":
                            raise SystemExit(2)
                        print((ARTIFACTS_ROOT / "boj28350_resume" / "build").resolve())
                        return 0

                    if __name__ == "__main__":
                        raise SystemExit(main())
                    """
                ).strip()
                + "\n",
                encoding="utf-8",
            )
            (branch_root / "solver_release_env.sh").write_text(
                textwrap.dedent(
                    """
                    #!/usr/bin/env bash
                    set -euo pipefail

                    if [[ -n "${CXX:-}" ]]; then
                      echo "ambient CXX leaked into solver_release_env" >&2
                      return 17 2>/dev/null || exit 17
                    fi
                    if [[ -n "${ENABLE_STATE_LOAD_MATERIALIZATION_OPT:-}" ]]; then
                      echo "ambient ENABLE_STATE_LOAD_MATERIALIZATION_OPT leaked into solver_release_env" >&2
                      return 18 2>/dev/null || exit 18
                    fi
                    export ENABLE_STATE_LOAD_MATERIALIZATION_OPT="${ENABLE_STATE_LOAD_MATERIALIZATION_OPT:-0}"
                    export PROFILE_MODE="${PROFILE_MODE:-PROFILE_NONE}"
                    """
                ).strip()
                + "\n",
                encoding="utf-8",
            )
            (branch_root / "boj28350_resume.py").write_text(
                textwrap.dedent(
                    """
                    #!/usr/bin/env python3
                    from __future__ import annotations

                    import json
                    import os
                    from pathlib import Path

                    Path("build_env.json").write_text(
                        json.dumps(
                            {
                                "CXX": os.environ.get("CXX", ""),
                                "ENABLE_STATE_LOAD_MATERIALIZATION_OPT": os.environ.get(
                                    "ENABLE_STATE_LOAD_MATERIALIZATION_OPT", ""
                                ),
                                "PROFILE_MODE": os.environ.get("PROFILE_MODE", ""),
                                "PWD": os.getcwd(),
                            },
                            sort_keys=True,
                        )
                        + "\\n",
                        encoding="utf-8",
                    )
                    """
                ).strip()
                + "\n",
                encoding="utf-8",
            )

            env = {
                "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
                "TERM": os.environ.get("TERM", "dumb"),
                "CXX": "/usr/bin/false",
                "ENABLE_STATE_LOAD_MATERIALIZATION_OPT": "1",
            }
            result = subprocess.run(
                ["bash", "-lc", "./build.sh"],
                cwd=branch_root,
                env=env,
                capture_output=True,
                text=True,
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)
            artifact_build_root = branch_root / "artifacts" / "boj28350_resume" / "build"
            payload = (artifact_build_root / "build_env.json").read_text(encoding="utf-8")
            self.assertIn('"CXX": ""', payload)
            self.assertIn('"ENABLE_STATE_LOAD_MATERIALIZATION_OPT": "0"', payload)
            self.assertIn('"PROFILE_MODE": "PROFILE_BASE"', payload)
            self.assertIn(f'"PWD": "{artifact_build_root}"', payload)
            self.assertFalse((branch_root / "build_env.json").exists())

    def test_compiler_subprocess_runs_from_artifact_output_directory(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source_dir = root / "src"
            output_dir = root / "artifacts" / "boj28350_resume" / "build"
            source_dir.mkdir(parents=True, exist_ok=True)
            output_dir.mkdir(parents=True, exist_ok=True)

            source_path = source_dir / "solver.cpp"
            source_path.write_text("int main() { return 0; }\n", encoding="utf-8")
            output_path = output_dir / "solve"
            compiler_cwds: list[Path] = []

            def fake_build_commands(
                compiler: str,
                source: Path,
                output: Path,
                static_mode: str,
                defines: list[str],
            ) -> list[list[str]]:
                return [[compiler, str(source), "-o", str(output)]]

            def fake_run(cmd: list[str], *, cwd: Path, capture_output: bool, text: bool):
                compiler_cwds.append(Path(cwd))
                Path(cmd[-1]).write_text("binary\n", encoding="utf-8")
                return subprocess.CompletedProcess(cmd, 0, "", "")

            with mock.patch.object(build, "ROOT", root), \
                 mock.patch.object(build, "resolve_output_path", return_value=output_path), \
                 mock.patch.object(build, "_compiler_candidates", return_value=["clang++"]), \
                 mock.patch.object(build, "_compiler_path", return_value="/usr/bin/clang++"), \
                 mock.patch.object(build, "_build_commands", side_effect=fake_build_commands), \
                 mock.patch.object(build.subprocess, "run", side_effect=fake_run), \
                 mock.patch.object(build.sys, "argv", ["build.py", "--source", "src/solver.cpp", "--out", str(output_path)]):
                self.assertEqual(build.main(), 0)

            self.assertEqual(compiler_cwds, [output_dir])


class BuildMetadataDependencyTests(unittest.TestCase):
    def test_existing_build_is_not_current_when_local_include_dependency_changes(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source_dir = root / "src"
            out_dir = root / "out"
            source_dir.mkdir(parents=True, exist_ok=True)
            out_dir.mkdir(parents=True, exist_ok=True)

            include_path = source_dir / "solver_body.inc"
            include_path.write_text("int solver_body() { return 1; }\n", encoding="utf-8")
            source_path = source_dir / "solver.cpp"
            source_path.write_text(
                '#include "solver_body.inc"\nint main() { return solver_body(); }\n',
                encoding="utf-8",
            )
            output_path = out_dir / "solve"
            output_path.write_text("binary\n", encoding="utf-8")

            with mock.patch.object(build, "ROOT", root):
                build._write_build_metadata(
                    output_path,
                    source=source_path,
                    compiler="/usr/bin/clang++",
                    command=["clang++", str(source_path), "-o", str(output_path)],
                    requested_compiler=None,
                    static_mode="auto",
                    defines=[],
                )

                self.assertTrue(
                    build._existing_build_is_current(
                        source=source_path,
                        output=output_path,
                        requested_compiler=None,
                        static_mode="auto",
                        defines=[],
                    )
                )

                include_path.write_text("int solver_body() { return 2; }\n", encoding="utf-8")

                self.assertFalse(
                    build._existing_build_is_current(
                        source=source_path,
                        output=output_path,
                        requested_compiler=None,
                        static_mode="auto",
                        defines=[],
                    ),
                    msg="changing a quoted local include must invalidate the cached build metadata",
                )


if __name__ == "__main__":
    unittest.main()
