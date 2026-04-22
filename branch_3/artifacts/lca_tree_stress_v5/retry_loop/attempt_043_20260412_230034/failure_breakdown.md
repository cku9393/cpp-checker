# Failure Breakdown: Attempt 43

- Timestamp: `2026-04-13 00:14:59 KST`
- Session ID: `orch_38464c9afd21`
- Execution ID: `exec_b332ebc5f078`
- Analysis state file: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/.ouroboros/failure_analysis_state.json`
- Analysis state revision: `212`

## Failure Decomposition

### AC 1: The branch_3 research notes and bundled progress40          │

- Failure type: `stall/no-activity`
- Failure family: `generic_retry_failure`
- Interpretation lane: `pre-gate-stability`
- Primary progress40 axis: `state_materialization`
- Secondary progress40 axis: `layout_gate`
- Profile mode observed: `unknown`
- Last progress checkpoint phase: `unknown`
- Last release diag phase: `unknown`
- Suggested next probe: `./lca_smoke.sh`
- Trace lines captured: `47`

#### Narrowed Localization Snapshot

- No statement-level localization was resolved for this breakdown.

#### Progress40 Axis Evidence

- `state_materialization`
  - `9: unset ENABLE_STATE_LOAD_MATERIALIZATION_OPT || true`
  - `16: unset ENABLE_STATE_LOAD_MATERIALIZATION_OPT || true`
  - `17: export ENABLE_STATE_LOAD_MATERIALIZATION_OPT="${ENABLE_STATE_LOAD_MATERIALIZATION_OPT:-0}"`
- Current summary pivot baseline: `zero-span eligibility and fastpath commit`
- Current summary residual axes: `state_materialization`, `layout_gate`, `zero_span_fastpath`

#### Phase Breakdown

- `solver-source` x4 | sample: `AC 1 → Bash: /bin/zsh -lc 'rg -n "progress40|current_state_summary|nex...`

#### Latest Next-Probe Signal

- No latest probe signal was available for this AC.

#### Current-Attempt Certify Rows

- No current-attempt certify row summary was available for this failed AC.

#### Retry-Critical Anchors

- No retry-critical anchors were resolved for this failed AC.

#### Code-Structure Hotspots

- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/boj28350_resume.py`
  Observed mentions: `5`
  Focus ranges: 240-380, 1-260
  Enclosing symbols: function run_case [153-241], function write_summary [244-269], function main [272-324], function parse_cases [44-46], function resolve_branch_path [49-53], function resolve_branch_solver [56-60]
  Note: observed in failed-AC trace, boosted by failure_analysis_state, matched pinned symbol `function parse_cases [44-46]`
  Mtime: `2026-04-10 04:54:19 KST`
  Evidence lines:
    - `AC 1 → Bash: /bin/zsh -lc "sed -n '1,260p' boj28350_resume.py"`
    - `AC 1 → Bash: /bin/zsh -lc 'head -n 200 test_boj28350_resume.py'`
    - `AC 1 → Bash: /bin/zsh -lc "sed -n '240,380p' boj28350_resume.py"`
  Code excerpt:
```text
  238:         "elapsed_s": "" if sec is None else f"{sec:.6f}",
  239:         "mem_kb": "" if rss_kb is None else str(rss_kb),
  240:         "case_dir": str(case_dir),
  241:     }
  242: 
  243: 
  244: def write_summary(rows: Iterable[dict[str, str]], out_path: Path) -> None:
  245:     rows = list(rows)
  246:     out_path = resolve_smoke_summary_path(out_path)
  247:     out_path.parent.mkdir(parents=True, exist_ok=True)
  248:     with out_path.open("w", newline="", encoding="utf-8") as f:
  249:         writer = csv.DictWriter(
  250:             f,
  251:             fieldnames=[
  252:                 "stage",
  253:                 "mode",
  254:                 "n",
  255:                 "seed",
  256:                 "shuffle_labels",
  257:                 "shuffle_queries",
  258:                 "timeout_s",
  259:                 "rc",
  260:                 "validator",
  261:                 "validator_msg",
  262:                 "elapsed_s",
  263:                 "mem_kb",
  264:                 "case_dir",
  265:             ],
```
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/test_snapshot_retry_inputs.py`
  Observed mentions: `1`
  Focus ranges: 1-260
  Enclosing symbols: class SnapshotRetryInputsTests [18-336], function write_research_review_baseline [19-56], function make_fake_branch [58-81], function run_helper [83-90]
  Note: observed in failed-AC trace
  Mtime: `2026-04-12 20:50:41 KST`
  Evidence lines:
    - `AC 1 → Bash: /bin/zsh -lc "sed -n '1,260p' test_snapshot_retry_inputs.py"`
  Code excerpt:
```text
    1: #!/usr/bin/env python3
    2: from __future__ import annotations
    3: 
    4: import hashlib
    5: import json
    6: import subprocess
    7: import sys
    8: import tempfile
    9: import textwrap
   10: import unittest
   11: from pathlib import Path
   12: 
   13: 
   14: REPO_ROOT = Path(__file__).resolve().parent
   15: HELPER_PATH = REPO_ROOT / ".ouroboros" / "snapshot_retry_inputs.py"
   16: 
   17: 
   18: class SnapshotRetryInputsTests(unittest.TestCase):
   19:     def write_research_review_baseline(
   20:         self,
   21:         branch_root: Path,
   22:         *,
   23:         source_set_a_complete: bool = True,
   24:         source_set_b_complete: bool = True,
   25:     ) -> None:
   26:         baseline_files = {
   27:             Path("README.md"): "# branch workspace guide\n",
   28:             Path("boj28350_resume/README.md"): "resume readme\n",
```
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/test_build.py`
  Observed mentions: `3`
  Focus ranges: none captured
  Enclosing symbols: class CompilerCandidateTests [14-27], function test_darwin_prefers_clang_before_gxx [15-20], function test_linux_keeps_gxx_first [22-27], class BuildWrapperEnvironmentTests [30-181]
  Note: observed in failed-AC trace, boosted by failure_analysis_state, matched pinned symbol `class CompilerCandidateTests [14-27]`
  Mtime: `2026-04-04 13:25:35 KST`
  Code excerpt:
```text
   12: 
   13: 
   14: class CompilerCandidateTests(unittest.TestCase):
   15:     def test_darwin_prefers_clang_before_gxx(self) -> None:
   16:         with mock.patch.object(build, "IS_WINDOWS", False), \
   17:              mock.patch.object(build.sys, "platform", "darwin"), \
   18:              mock.patch.dict(os.environ, {}, clear=False):
   19:             os.environ.pop("CXX", None)
   20:             self.assertEqual(build._compiler_candidates(None), ["clang++", "g++", "c++"])
   21: 
   22:     def test_linux_keeps_gxx_first(self) -> None:
   23:         with mock.patch.object(build, "IS_WINDOWS", False), \
   24:              mock.patch.object(build.sys, "platform", "linux"), \
   25:              mock.patch.dict(os.environ, {}, clear=False):
   26:             os.environ.pop("CXX", None)
   27:             self.assertEqual(build._compiler_candidates(None), ["g++", "clang++", "c++"])
   28: 
   29: 
```
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/build.sh`
  Observed mentions: `3`
  Focus ranges: none captured
  Enclosing symbols: no symbols inferred
  Note: observed in failed-AC trace, boosted by failure_analysis_state
  Mtime: `2026-04-03 08:08:02 KST`
  Code excerpt:
```text
    1: #!/usr/bin/env bash
    2: set -euo pipefail
    3: 
    4: SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
    5: 
    6: # Keep release builds reproducible by clearing ambient compiler/profile knobs
    7: # before the branch-local runtime envelope rehydrates its artifact-rooted env.
    8: unset CXX || true
    9: unset ENABLE_STATE_LOAD_MATERIALIZATION_OPT || true
   10: export PROFILE_MODE="${PROFILE_MODE:-PROFILE_BASE}"
   11: export BRANCH_NON_ARTIFACT_BYTECODE_PURGED=1
   12: 
   13: source "$SCRIPT_DIR/solver_release_env.sh"
   14: 
   15: unset CXX || true
   16: unset ENABLE_STATE_LOAD_MATERIALIZATION_OPT || true
   17: export ENABLE_STATE_LOAD_MATERIALIZATION_OPT="${ENABLE_STATE_LOAD_MATERIALIZATION_OPT:-0}"
   18: export PROFILE_MODE="${PROFILE_MODE:-PROFILE_BASE}"
   19: export BRANCH_NON_ARTIFACT_BYTECODE_PURGED=1
   20: 
   21: BUILD_WRAPPER_CWD="$(python3 "$SCRIPT_DIR/artifact_paths.py" boj28350_build)"
   22: mkdir -p "$BUILD_WRAPPER_CWD"
   23: cd "$BUILD_WRAPPER_CWD"
   24: 
   25: if [[ -f "$SCRIPT_DIR/build.py" ]]; then
   26:   exec python3 "$SCRIPT_DIR/build.py" "$@"
   27: fi
   28: 
```
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifact_paths.py`
  Observed mentions: `3`
  Focus ranges: none captured
  Enclosing symbols: function artifacts_root [80-81], function ensure_under_artifacts [84-85], function ensure_resolved_under_artifacts [88-89], function resolve_branch_artifact_path [92-102]
  Note: observed in failed-AC trace, boosted by failure_analysis_state, matched pinned symbol `function artifacts_root [80-81]`
  Mtime: `2026-04-12 07:14:52 KST`
  Code excerpt:
```text
   78: 
   79: 
   80: def artifacts_root() -> Path:
   81:     return ARTIFACTS_ROOT
   82: 
   83: 
```
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/test_boj28350_resume.py`
  Observed mentions: `1`
  Focus ranges: none captured
  Enclosing symbols: class Boj28350ResumeArtifactLocalityTests [14-235], function assert_under_artifacts [15-21], function test_build_solver_always_passes_artifact_rooted_output [23-38], function test_build_solver_executes_from_artifact_build_directory [40-54]
  Note: observed in failed-AC trace
  Mtime: `2026-04-09 20:12:20 KST`
  Evidence lines:
    - `AC 1 → Bash: /bin/zsh -lc 'head -n 200 test_boj28350_resume.py'`
  Code excerpt:
```text
   12: 
   13: 
   14: class Boj28350ResumeArtifactLocalityTests(unittest.TestCase):
   15:     def assert_under_artifacts(self, value: str | Path) -> Path:
   16:         path = Path(value).resolve()
   17:         try:
   18:             path.relative_to(artifacts_root())
   19:         except ValueError:
   20:             self.fail(f"path escaped artifacts root: {path}")
   21:         return path
   22: 
   23:     def test_build_solver_always_passes_artifact_rooted_output(self) -> None:
   24:         with mock.patch.object(
   25:             boj28350_resume.subprocess,
   26:             "run",
   27:             return_value=SimpleNamespace(returncode=0),
   28:         ) as run_mock:
   29:             rc = boj28350_resume.build_solver(None, "auto", [], None, None)
   30: 
   31:         self.assertEqual(rc, 0)
   32:         cmd = run_mock.call_args.args[0]
   33:         out_path = Path(cmd[cmd.index("--out") + 1])
   34:         self.assertEqual(
   35:             out_path,
   36:             artifacts_root() / "boj28350_resume" / "build" / boj28350_resume.DEFAULT_SOLVER.name,
   37:         )
   38:         self.assert_under_artifacts(out_path)
   39: 
```

#### Focused Artifact Snapshots

- No failed-AC-specific artifact roots were mapped.

#### Failed-AC Trace Excerpt

```text
  AC 1: Analyzing complexity...
  AC 1 → Bash: /bin/zsh -lc 'rg -n "progress40|current_state_summary|nex...
  AC 1 → Bash: /bin/zsh -lc "find boj28350_resume boj28350_bundle_archiv...
  AC 1 → Bash: /bin/zsh -lc 'ls -la'
  AC 1 → Bash: /bin/zsh -lc "sed -n '1,260p' boj28350_resume.py"
  AC 1 → Bash: /bin/zsh -lc 'head -n 200 test_boj28350_resume.py'
  AC 1 → Bash: /bin/zsh -lc 'head -n 160 boj28350_resume/current_state_s...
  AC 1 → Bash: /bin/zsh -lc 'head -n 200 boj28350_resume/next_session_br...
  AC 1 → Bash: /bin/zsh -lc 'head -n 160 boj28350_resume/progress40_deri...
  AC 1 → Bash: /bin/zsh -lc "rg -n \"argparse|ArgumentParser|subparsers|...
  AC 1 → Bash: /bin/zsh -lc "sed -n '240,380p' boj28350_resume.py"
  AC 1 → Bash: /bin/zsh -lc 'rg -n "pre_rewrite|checkpoint|synthesis|pro...
  AC 1 → Bash: /bin/zsh -lc "sed -n '1,260p' test_snapshot_retry_inputs.py"
  AC 1 → Bash: /bin/zsh -lc "sed -n '1,320p' .ouroboros/snapshot_retry_i...
  AC 1 → Bash: /bin/zsh -lc "sed -n '260,420p' test_snapshot_retry_input...
  AC 1 → Bash: /bin/zsh -lc "sed -n '1,260p' .ouroboros/research_review_...
  AC 1 → Bash: /bin/zsh -lc "find .ouroboros -maxdepth 2 -type f | sort ...
  AC 1 → Bash: /bin/zsh -lc "sed -n '1,120p' .ouroboros/prepare_retry_at...
  AC 1 → Bash: /bin/zsh -lc "sed -n '1,120p' boj28350_literature_progres...
  AC 1 → Bash: /bin/zsh -lc 'head -c 2500 boj28350_bundle_archive/boj283...
  AC 1 → Bash: /bin/zsh -lc 'head -c 2500 boj28350_bundle_archive/boj283...
  AC 1 → Bash: /bin/zsh -lc "stat -f '%N %z bytes %Sm' boj28350_resume/R...
  AC 1 → Bash: /bin/zsh -lc "sed -n '1,220p' boj28350_resume/progress40_...
  AC 1 → Bash: /bin/zsh -lc "sed -n '1,220p' boj28350_resume/pre_rewrite...
  AC 1 → Bash: /bin/zsh -lc "sed -n '1,200p' boj28350_resume/pre_rewrite...
  AC 1 → Bash: /bin/zsh -lc "sed -n '1,80p' boj28350_resume/boj28350_bra...
  AC 1 → Bash: /bin/zsh -lc 'head -n 120 boj28350_bundle_archive/boj2835...
  AC 1 → Bash: /bin/zsh -lc 'head -n 120 boj28350_bundle_archive/boj2835...
  AC 1 → Bash: /bin/zsh -lc 'head -n 120 boj28350_resume/README.md'
  AC 1 → Bash: /bin/zsh -lc "sed -n '1,220p' boj28350_resume/README.md"
  AC 1 → Bash: /bin/zsh -lc 'wc -l boj28350_resume/README.md boj28350_co...
  AC 1 → Bash: /bin/zsh -lc "perl -ne 'print if "'$. <= 120'"' boj28350_...
  AC 1 → Bash: /bin/zsh -lc "grep -n '' boj28350_resume/README.md | head...
  AC 1 → Bash: /bin/zsh -lc 'head -c 3500 literature_grade_proof_package...
  AC 1 → Bash: /bin/zsh -lc 'od -An -c -N 512 boj28350_resume/README.md'
  AC 1 → Bash: /bin/zsh -lc "sed -n '1,160p' literature_grade_proof_pack...
  AC 1 → Bash: /bin/zsh -lc "sed -n '1,160p' boj28350_integrated_technic...
  AC 1 → Bash: /bin/zsh -lc 'head -c 3500 boj28350_integrated_technical_...
  AC 1 → Bash: /bin/zsh -lc "sed -n '1,120p' boj28350_complete_master_do...
  AC 1 → Bash: /bin/zsh -lc 'head -c 3500 boj28350_complete_master_docum...
  AC 1 → Bash: /bin/zsh -lc "pwd && rg --files | rg '"'^(boj28350_resume...
  AC 1 → Bash: /bin/zsh -lc "rg --files boj28350_resume boj28350_bundle_...
Level 1 complete: 0 succeeded, 2 failed
╭────────────────────────────── Partial Success ───────────────────────────────╮
│ ### AC 1: [FAIL] The branch_3 research notes and bundled progress40          │
│ materials are read before major solver rewrites or pivots                    │
│ Error: Stalled (no activity for 300s)                                        │
```

### AC 8: All generated outputs remain inside branch_3/artifacts/...  │

- Failure type: `stall/no-activity`
- Failure family: `generic_retry_failure`
- Interpretation lane: `pre-gate-stability`
- Primary progress40 axis: `state_materialization`
- Secondary progress40 axis: `layout_gate`
- Profile mode observed: `unknown`
- Last progress checkpoint phase: `unknown`
- Last release diag phase: `unknown`
- Suggested next probe: `./lca_smoke.sh`
- Trace lines captured: `25`

#### Narrowed Localization Snapshot

- No statement-level localization was resolved for this breakdown.

#### Progress40 Axis Evidence

- `state_materialization`
  - `9: unset ENABLE_STATE_LOAD_MATERIALIZATION_OPT || true`
  - `16: unset ENABLE_STATE_LOAD_MATERIALIZATION_OPT || true`
  - `17: export ENABLE_STATE_LOAD_MATERIALIZATION_OPT="${ENABLE_STATE_LOAD_MATERIALIZATION_OPT:-0}"`
- Current summary pivot baseline: `zero-span eligibility and fastpath commit`
- Current summary residual axes: `state_materialization`, `layout_gate`, `zero_span_fastpath`

#### Phase Breakdown

- `artifact-paths` x2 | sample: `AC 8 → Bash: /bin/zsh -lc "sed -n '260,520p' artifact_paths.py"`

#### Latest Next-Probe Signal

- No latest probe signal was available for this AC.

#### Current-Attempt Certify Rows

- No current-attempt certify row summary was available for this failed AC.

#### Retry-Critical Anchors

- No retry-critical anchors were resolved for this failed AC.

#### Code-Structure Hotspots

- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifact_paths.py`
  Observed mentions: `5`
  Focus ranges: 1-260, 260-520
  Enclosing symbols: function artifacts_root [80-81], function ensure_under_artifacts [84-85], function ensure_resolved_under_artifacts [88-89], function resolve_branch_artifact_path [92-102], function _ensure_artifact_process_dir [253-266], function _is_under_artifacts [269-270]
  Note: observed in failed-AC trace, mapped from failed AC semantics, boosted by failure_analysis_state, matched pinned symbol `function artifacts_root [80-81]`
  Mtime: `2026-04-12 07:14:52 KST`
  Evidence lines:
    - `AC 8 → Bash: /bin/zsh -lc "sed -n '260,520p' artifact_paths.py"`
    - `AC 8 → Bash: /bin/zsh -lc "sed -n '1,260p' artifact_paths.py"`
  Code excerpt:
```text
    1: #!/usr/bin/env python3
    2: from __future__ import annotations
    3: 
    4: import argparse
    5: import hashlib
    6: import json
    7: import os
    8: import stat
    9: import shutil
   10: import sys
   11: from pathlib import Path
   12: 
   13: os.environ.setdefault("PYTHONDONTWRITEBYTECODE", "1")
   14: sys.dont_write_bytecode = True
   15: 
   16: 
   17: BRANCH_ROOT = Path(__file__).resolve().parent
   18: DEFAULT_ARTIFACTS_ROOT = (BRANCH_ROOT / "artifacts").resolve()
   19: ARTIFACTS_ROOT = DEFAULT_ARTIFACTS_ROOT
   20: BRANCH_TMP_SUBPATH = ("lca_tree_stress_v5", ".tmp")
   21: NON_ARTIFACT_TREE_STATE_SCHEMA = "branch_non_artifact_tree_state_v1"
   22: NON_ARTIFACT_TREE_VERIFY_ESCAPE_EXIT = 3
   23: NON_ARTIFACT_SCAN_IGNORED_ROOTS = frozenset({".git"})
   24: PROCESS_STATE_SUBPATH = (".process_state",)
   25: NON_ARTIFACT_PURGE_ENV = "BRANCH_NON_ARTIFACT_BYTECODE_PURGED"
   26: NON_ARTIFACT_HASH_MAX_BYTES = int(os.environ.get("BRANCH_NON_ARTIFACT_HASH_MAX_BYTES", "0") or "0")
   27: NON_ARTIFACT_CREATED_SOURCE_WARNING_SUFFIXES = frozenset(
   28:     {
```
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/boj28350_resume.py`
  Observed mentions: `4`
  Focus ranges: 1-260
  Enclosing symbols: function parse_cases [44-46], function resolve_branch_path [49-53], function resolve_branch_solver [56-60], function sanitize_artifact_token [63-66]
  Note: observed in failed-AC trace, boosted by failure_analysis_state, matched pinned symbol `function parse_cases [44-46]`
  Mtime: `2026-04-10 04:54:19 KST`
  Evidence lines:
    - `AC 8 → Bash: /bin/zsh -lc "sed -n '1,260p' boj28350_resume.py"`
  Code excerpt:
```text
    1: #!/usr/bin/env python3
    2: from __future__ import annotations
    3: 
    4: import argparse
    5: import csv
    6: import json
    7: import os
    8: import random
    9: import re
   10: import shutil
   11: import subprocess
   12: import sys
   13: from pathlib import Path
   14: from typing import Iterable
   15: 
   16: os.environ["PYTHONDONTWRITEBYTECODE"] = "1"
   17: sys.dont_write_bytecode = True
   18: 
   19: try:
   20:     import branch_gen_case_local as branch_gen_case
   21: except ModuleNotFoundError:
   22:     import branch_gen_case
   23: from artifact_paths import configure_branch_process_env, default_output_path, ensure_under_artifacts, resolve_output_path
   24: try:
   25:     from branch_validator_local import validate_case
   26: except ModuleNotFoundError:
   27:     from branch_validator import validate_case
   28: from suite_utils import ensure_executable, resolve_solver_path, run_solver_with_time
```
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/test_build.py`
  Observed mentions: `3`
  Focus ranges: none captured
  Enclosing symbols: class CompilerCandidateTests [14-27], function test_darwin_prefers_clang_before_gxx [15-20], function test_linux_keeps_gxx_first [22-27], class BuildWrapperEnvironmentTests [30-181]
  Note: observed in failed-AC trace, boosted by failure_analysis_state, matched pinned symbol `class CompilerCandidateTests [14-27]`
  Mtime: `2026-04-04 13:25:35 KST`
  Code excerpt:
```text
   12: 
   13: 
   14: class CompilerCandidateTests(unittest.TestCase):
   15:     def test_darwin_prefers_clang_before_gxx(self) -> None:
   16:         with mock.patch.object(build, "IS_WINDOWS", False), \
   17:              mock.patch.object(build.sys, "platform", "darwin"), \
   18:              mock.patch.dict(os.environ, {}, clear=False):
   19:             os.environ.pop("CXX", None)
   20:             self.assertEqual(build._compiler_candidates(None), ["clang++", "g++", "c++"])
   21: 
   22:     def test_linux_keeps_gxx_first(self) -> None:
   23:         with mock.patch.object(build, "IS_WINDOWS", False), \
   24:              mock.patch.object(build.sys, "platform", "linux"), \
   25:              mock.patch.dict(os.environ, {}, clear=False):
   26:             os.environ.pop("CXX", None)
   27:             self.assertEqual(build._compiler_candidates(None), ["g++", "clang++", "c++"])
   28: 
   29: 
```
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/build.sh`
  Observed mentions: `3`
  Focus ranges: none captured
  Enclosing symbols: no symbols inferred
  Note: observed in failed-AC trace, boosted by failure_analysis_state
  Mtime: `2026-04-03 08:08:02 KST`
  Code excerpt:
```text
    1: #!/usr/bin/env bash
    2: set -euo pipefail
    3: 
    4: SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
    5: 
    6: # Keep release builds reproducible by clearing ambient compiler/profile knobs
    7: # before the branch-local runtime envelope rehydrates its artifact-rooted env.
    8: unset CXX || true
    9: unset ENABLE_STATE_LOAD_MATERIALIZATION_OPT || true
   10: export PROFILE_MODE="${PROFILE_MODE:-PROFILE_BASE}"
   11: export BRANCH_NON_ARTIFACT_BYTECODE_PURGED=1
   12: 
   13: source "$SCRIPT_DIR/solver_release_env.sh"
   14: 
   15: unset CXX || true
   16: unset ENABLE_STATE_LOAD_MATERIALIZATION_OPT || true
   17: export ENABLE_STATE_LOAD_MATERIALIZATION_OPT="${ENABLE_STATE_LOAD_MATERIALIZATION_OPT:-0}"
   18: export PROFILE_MODE="${PROFILE_MODE:-PROFILE_BASE}"
   19: export BRANCH_NON_ARTIFACT_BYTECODE_PURGED=1
   20: 
   21: BUILD_WRAPPER_CWD="$(python3 "$SCRIPT_DIR/artifact_paths.py" boj28350_build)"
   22: mkdir -p "$BUILD_WRAPPER_CWD"
   23: cd "$BUILD_WRAPPER_CWD"
   24: 
   25: if [[ -f "$SCRIPT_DIR/build.py" ]]; then
   26:   exec python3 "$SCRIPT_DIR/build.py" "$@"
   27: fi
   28: 
```
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/test_lca_strong_gate_wrapper.py`
  Observed mentions: `1`
  Focus ranges: none captured
  Enclosing symbols: class LcaStrongGateWrapperRegressionTests [17-1467], function test_branch_local_materialized_strong_gate_preset_cache_stays_available [18-33], function test_wrapper_ignores_sighup_during_long_gate_runs [35-40], function write_text [42-44]
  Note: observed in failed-AC trace
  Mtime: `2026-04-12 06:27:03 KST`
  Evidence lines:
    - `AC 8 → Bash: /bin/zsh -lc "pytest -q test_lca_strong_gate_wrapper.py -...`
  Code excerpt:
```text
   15: 
   16: 
   17: class LcaStrongGateWrapperRegressionTests(unittest.TestCase):
   18:     def test_branch_local_materialized_strong_gate_preset_cache_stays_available(self) -> None:
   19:         repo_root = Path(__file__).resolve().parent
   20:         preset_cache = repo_root / "artifacts" / "lca_tree_stress_v5" / ".preset_cache" / "lca_strong_gate.json"
   21: 
   22:         self.assertTrue(
   23:             preset_cache.is_file(),
   24:             msg="branch_3 should ship a materialized strong-gate preset cache so dataless iCloud presets do not block AC3 preflight",
   25:         )
   26: 
   27:         payload = json.loads(preset_cache.read_text(encoding="utf-8"))
   28:         self.assertEqual(payload.get("name"), "strong_gate")
   29:         self.assertEqual(
   30:             [stage.get("name") for stage in payload.get("stages", [])],
   31:             ["correctness_fuzz", "hard_scaling", "max_n_mix"],
   32:             msg="the branch-local preset cache must preserve the full strong-gate stage surface, not a narrowed probe snapshot",
   33:         )
   34: 
   35:     def test_wrapper_ignores_sighup_during_long_gate_runs(self) -> None:
   36:         self.assertIn(
   37:             "trap '' HUP",
   38:             WRAPPER_SOURCE,
   39:             msg="strong gate wrapper should ignore SIGHUP so long certify runs are not torn down by parent-session hangups",
   40:         )
   41: 
   42:     def write_text(self, path: Path, content: str) -> None:
```
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/test_lca_boj3s_gate_wrapper.py`
  Observed mentions: `1`
  Focus ranges: none captured
  Enclosing symbols: class LcaBoj3sGateWrapperRegressionTests [18-1525], function write_text [19-21], function make_executable [23-24], function make_fake_branch [26-227]
  Note: observed in failed-AC trace
  Mtime: `2026-04-10 09:07:50 KST`
  Evidence lines:
    - `AC 8 → Bash: /bin/zsh -lc "pytest -q test_lca_boj3s_gate_wrapper.py -k...`
  Code excerpt:
```text
   16: 
   17: 
   18: class LcaBoj3sGateWrapperRegressionTests(unittest.TestCase):
   19:     def write_text(self, path: Path, content: str) -> None:
   20:         path.parent.mkdir(parents=True, exist_ok=True)
   21:         path.write_text(content, encoding="utf-8")
   22: 
   23:     def make_executable(self, path: Path) -> None:
   24:         path.chmod(path.stat().st_mode | 0o111)
   25: 
   26:     def make_fake_branch(self, temp_root: Path) -> Path:
   27:         branch_root = temp_root / "branch"
   28:         tooling_root = temp_root / "lca_tree_stress_v5" / "tooling"
   29:         self.write_text(branch_root / "outer_suite_wrappers" / "lca_boj3s_gate.sh", WRAPPER_SOURCE)
   30:         self.write_text(
   31:             branch_root / "artifact_paths.py",
   32:             textwrap.dedent(
   33:                 """
   34:                 #!/usr/bin/env python3
   35:                 from __future__ import annotations
   36: 
   37:                 import argparse
   38:                 from pathlib import Path
   39: 
   40:                 BRANCH_ROOT = Path(__file__).resolve().parent
   41:                 ARTIFACTS_ROOT = (BRANCH_ROOT / "artifacts").resolve()
   42:                 OUTPUTS = {
   43:                     "lca_boj3s_gate": ARTIFACTS_ROOT / "lca_tree_stress_v5" / "boj3s_gate",
```
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/run.sh`
  Observed mentions: `1`
  Focus ranges: none captured
  Enclosing symbols: no symbols inferred
  Note: observed in failed-AC trace
  Mtime: `2026-03-26 11:36:17 KST`
  Evidence lines:
    - `AC 8 → Bash: /bin/zsh -lc 'ls -l run.sh outer_suite_wrappers/lca_hunt.sh'`
  Code excerpt:
```text
    1: #!/usr/bin/env bash
    2: set -euo pipefail
    3: 
    4: ROOT="$(cd "$(dirname "$0")" && pwd)"
    5: SOLVER="${BRANCH3_SOLVER:-$ROOT/boj28350_resume/solve}"
    6: RELEASE_ENV="$ROOT/solver_release_env.sh"
    7: ARTIFACT_RESOLVER="$ROOT/artifact_paths.py"
    8: 
    9: if [[ ! -x "$SOLVER" ]]; then
   10:   echo "[branch_3/run.sh] missing solver binary at $SOLVER" >&2
   11:   echo "[branch_3/run.sh] run ./build.sh first" >&2
   12:   exit 2
   13: fi
   14: 
   15: if [[ ! -f "$ARTIFACT_RESOLVER" ]]; then
   16:   echo "[branch_3/run.sh] missing artifact resolver at $ARTIFACT_RESOLVER" >&2
   17:   exit 2
   18: fi
   19: 
   20: if [[ -f "$RELEASE_ENV" ]]; then
   21:   source "$RELEASE_ENV"
   22: fi
   23: 
   24: DENSE_PROFILE_OUTDIR="$(python3 "$ARTIFACT_RESOLVER" boj28350_direct_solver_aux "${DENSE_PROFILE_OUTDIR:-}")"
   25: export DENSE_PROFILE_OUTDIR
   26: mkdir -p "$DENSE_PROFILE_OUTDIR"
   27: cd "$DENSE_PROFILE_OUTDIR"
   28: exec "$SOLVER" "$@"
```
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/outer_suite_wrappers/lca_hunt.sh`
  Observed mentions: `1`
  Focus ranges: none captured
  Enclosing symbols: function usage [17-52]
  Note: observed in failed-AC trace
  Mtime: `2026-03-26 13:26:13 KST`
  Evidence lines:
    - `AC 8 → Bash: /bin/zsh -lc 'ls -l run.sh outer_suite_wrappers/lca_hunt.sh'`
  Code excerpt:
```text
   15: ARTIFACTS_ROOT="$BRANCH/artifacts/lca_tree_stress_v5"
   16: 
   17: usage() {
   18:   cat >&2 <<'EOF'
   19: usage: ./outer_suite_wrappers/lca_hunt.sh [label] [sizes_csv] [seeds_csv] [timeout_sec]
   20: [lca_hunt] diagnostic-only helper for hardest-case search and reporting
   21: [lca_hunt] formal acceptance uses ./outer_suite_wrappers/lca_strong_gate.sh and ./outer_suite_wrappers/lca_boj3s_gate.sh
   22: EOF
   23: }
   24: 
   25: if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
   26:   usage
   27:   exit 0
   28: fi
   29: 
   30: source "$BRANCH/solver_release_env.sh"
   31: 
   32: case "$OUTDIR" in
   33:   "$ARTIFACTS_ROOT"|"$ARTIFACTS_ROOT"/*)
   34:     ;;
   35:   *)
   36:     echo "[lca_hunt] path escaped branch-local artifacts root: $OUTDIR" >&2
   37:     exit 1
   38:     ;;
   39: esac
   40: 
   41: mkdir -p "$OUTDIR"
   42: cd "$OUTDIR"
```

#### Focused Artifact Snapshots

- `run_case` latest: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/run_case/diagnostics/case_probe_multi_comb_rect_no_lreuse/run_case_result.json`
  mtime: `2026-04-12 18:23:30 KST`
  attempt start: `2026-04-12 23:00:34 KST`
  fresh within attempt: `no`
  freshness note: latest summary/file predates the failed attempt start; treat it as carried-forward evidence, not fresh gate output
- `gen_case_aux` latest: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/gen_case_aux/ac8_audit/gen/hidden_parent.txt`
  mtime: `2026-03-25 18:48:35 KST`
  attempt start: `2026-04-12 23:00:34 KST`
  fresh within attempt: `no`
  freshness note: latest summary/file predates the failed attempt start; treat it as carried-forward evidence, not fresh gate output

#### Failed-AC Trace Excerpt

```text
  AC 8: Analyzing complexity...
  AC 8 → Bash: /bin/zsh -lc "sed -n '1,260p' boj28350_resume.py"
  AC 8 → Bash: /bin/zsh -lc "sed -n '260,520p' artifact_paths.py"
  AC 8 → Bash: /bin/zsh -lc "sed -n '1,260p' artifact_paths.py"
  AC 8 → Bash: /bin/zsh -lc "pytest -q test_lca_boj3s_gate_wrapper.py -k...
  AC 8 → Bash: /bin/zsh -lc "pytest -q test_lca_strong_gate_wrapper.py -...
  AC 8 → Bash: /bin/zsh -lc "sed -n '1,260p' test_entrypoint_artifact_lo...
  AC 8 → Bash: /bin/zsh -lc "sed -n '1,320p' test_retry_loop_artifact_lo...
  AC 8 → Bash: /bin/zsh -lc "sed -n '1,260p' test_lca_smoke_entrypoint_a...
  AC 8 → Bash: /bin/zsh -lc "sed -n '1,260p' .ouroboros/snapshot_retry_i...
  AC 8 → Bash: /bin/zsh -lc "sed -n '1,280p' .ouroboros/run_until_pass_p...
  AC 8 → Bash: /bin/zsh -lc "sed -n '1,260p' .ouroboros/snapshot_retry_r...
  AC 8 → Bash: /bin/zsh -lc "rg -n \"cwd=|open\\(|write_text\\(|mkdtemp|...
  AC 8 → Bash: /bin/zsh -lc "sed -n '320,760p' test_retry_loop_artifact_...
  AC 8 → Bash: /bin/zsh -lc "rg -n \"subprocess|Popen|communicate|wait\\...
  AC 8 → Bash: /bin/zsh -lc "ps -o pid,ppid,etime,command -ax | rg 'pyte...
  AC 8 → Bash: /bin/zsh -lc "pytest -q test_retry_loop_artifact_locality...
  AC 8 → Bash: /bin/zsh -lc 'ls -l run.sh outer_suite_wrappers/lca_hunt.sh'
  AC 8 → Bash: /bin/zsh -lc "rg -n 'mkdir -p \"\\"'$SCRIPT_DIR/artifacts...
  AC 8 → Bash: /bin/zsh -lc "sed -n '1,260p' .ouroboros/retry_artifact_i...
  AC 8 → Bash: /bin/zsh -lc "rg -n 'write_text\\(|open\\(|mkdir\\(|mktem...
  AC 8 → Bash: /bin/zsh -lc "rg -n 'branch_prefixed|prefixed_artifact|ar...
│ ### AC 8: [FAIL] All generated outputs remain inside branch_3/artifacts/...  │
│ Error: Stalled (no activity for 300s)                                        │
╰──────────────────────────────────────────────────────────────────────────────╯
```

## Refinement Versus Previous Failure

- Repeated failed AC(s) versus the previous attempt: 1, 8.
- AC 1 repeated the same failure classification `stall/no-activity`.
- AC 8 repeated the same failure classification `stall/no-activity`.
- Recurring code-structure hotspots: /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifact_paths.py, /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/boj28350_resume.py, /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/build.sh, /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/run.sh, /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/test_boj28350_resume.py, /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/test_build.py.
- New hotspots to fold into the next retry analysis: /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/outer_suite_wrappers/lca_hunt.sh, /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/test_lca_boj3s_gate_wrapper.py, /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/test_lca_strong_gate_wrapper.py.
- Recurring enclosing symbols: class Boj28350ResumeArtifactLocalityTests [14-235], class BuildWrapperEnvironmentTests [30-181], class CompilerCandidateTests [14-27], class SnapshotRetryInputsTests [18-336], function artifacts_root [80-81], function assert_under_artifacts [15-21].
- Recurring line-range hotspots: /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifact_paths.py:1-260, /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/boj28350_resume.py:1-260.
- AC 1 kept the same primary progress40 axis `state_materialization`.

## Next-Retry Analysis Rule

- Before the next session edits code, read this breakdown, start from the repeated failed AC if one exists, and inspect the listed phase and code-structure hotspots before running the heavy gate again.
- If this breakdown still localizes the failure only at a broad file level, improve the retry analysis logic itself before the next heavy run so the next capture records narrower symbols, ranges, wrapper sections, and code excerpts.
- When `failure_analysis_state.json` carries retry-specific line ranges or non-code transport/guard anchors, surface them first as dedicated `Retry-Critical Anchors` or `Filtered Failure Locus` sections before generic hotspots so the next solver session starts from exact slices.