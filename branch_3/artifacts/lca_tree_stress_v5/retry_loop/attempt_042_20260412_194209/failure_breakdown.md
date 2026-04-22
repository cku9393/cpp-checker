# Failure Breakdown: Attempt 42

- Timestamp: `2026-04-12 22:59:18 KST`
- Session ID: `orch_42d1d2891e94`
- Execution ID: `exec_af1d222264e6`
- Analysis state file: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/.ouroboros/failure_analysis_state.json`
- Analysis state revision: `211`

## Failure Decomposition

### AC 1: The branch_3 research notes and bundled progress40          │

- Failure type: `stall/no-activity`
- Failure family: `generic_retry_failure`
- Interpretation lane: `pre-gate-stability`
- Primary progress40 axis: `state_materialization`
- Secondary progress40 axis: `zero_span_fastpath`
- Profile mode observed: `unknown`
- Last progress checkpoint phase: `unknown`
- Last release diag phase: `unknown`
- Suggested next probe: `./lca_smoke.sh`
- Trace lines captured: `80`

#### Narrowed Localization Snapshot

- No statement-level localization was resolved for this breakdown.

#### Progress40 Axis Evidence

- `zero_span_fastpath`
  - `signature|zero-span...`
  - `Sub-AC 2 of AC 1 → Bash: /bin/zsh -c "rg -n 'zero-span|fastpath|signature`
- `state_materialization`
  - `9: unset ENABLE_STATE_LOAD_MATERIALIZATION_OPT || true`
  - `16: unset ENABLE_STATE_LOAD_MATERIALIZATION_OPT || true`
  - `17: export ENABLE_STATE_LOAD_MATERIALIZATION_OPT="${ENABLE_STATE_LOAD_MATERIALIZATION_OPT:-0}"`

- Enabled flags seen in trace: `ENABLE_LAYOUT_R`, `ENABLE_LAYOUT_SIGNATURE_GATE_OPT`, `ENABLE_PACK_ENCODE_NORMALIZE_OPT`
- Current summary pivot baseline: `zero-span eligibility and fastpath commit`
- Current summary residual axes: `state_materialization`, `layout_gate`, `zero_span_fastpath`

#### Phase Breakdown

- `solver-source` x11 | sample: `boj28350_resume/progress40_...`
- `artifact-paths` x1 | sample: `Sub-AC 3 of AC 1 → Bash: /bin/zsh -lc "sed -n '1,260p' artifact_paths.py"`

#### Latest Next-Probe Signal

- No latest probe signal was available for this AC.

#### Current-Attempt Certify Rows

- No current-attempt certify row summary was available for this failed AC.

#### Retry-Critical Anchors

- No retry-critical anchors were resolved for this failed AC.

#### Code-Structure Hotspots

- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/boj28350_resume.py`
  Observed mentions: `2`
  Focus ranges: 1-260, 260-520
  Enclosing symbols: function parse_cases [44-46], function resolve_branch_path [49-53], function resolve_branch_solver [56-60], function sanitize_artifact_token [63-66], function write_summary [244-269], function main [272-324]
  Note: observed in failed-AC trace
  Mtime: `2026-04-10 04:54:19 KST`
  Evidence lines:
    - `Sub-AC 3 of AC 1 → Bash: /bin/zsh -lc "sed -n '1,260p' boj28350_resume.py"`
    - `Sub-AC 3 of AC 1 → Bash: /bin/zsh -lc "sed -n '260,520p' boj28350_resume.py"`
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
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifact_paths.py`
  Observed mentions: `4`
  Focus ranges: 1-260
  Enclosing symbols: function artifacts_root [80-81], function ensure_under_artifacts [84-85], function ensure_resolved_under_artifacts [88-89], function resolve_branch_artifact_path [92-102]
  Note: observed in failed-AC trace, boosted by failure_analysis_state, matched pinned symbol `function artifacts_root [80-81]`
  Mtime: `2026-04-12 07:14:52 KST`
  Evidence lines:
    - `Sub-AC 3 of AC 1 → Bash: /bin/zsh -lc "sed -n '1,260p' artifact_paths.py"`
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
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/test_build.py`
  Observed mentions: `2`
  Focus ranges: 1-220
  Enclosing symbols: class CompilerCandidateTests [14-27], function test_darwin_prefers_clang_before_gxx [15-20], function test_linux_keeps_gxx_first [22-27], class BuildWrapperEnvironmentTests [30-181]
  Note: observed in failed-AC trace
  Mtime: `2026-04-04 13:25:35 KST`
  Evidence lines:
    - `Sub-AC 2 of AC 1 → Bash: /bin/zsh -c 'head -n 5 test_build.py'`
    - `Sub-AC 3 of AC 1 → Bash: /bin/zsh -lc "sed -n '1,220p' test_build.py"`
  Code excerpt:
```text
    1: from __future__ import annotations
    2: 
    3: import os
    4: import subprocess
    5: import tempfile
    6: import unittest
    7: from unittest import mock
    8: from pathlib import Path
    9: import textwrap
   10: 
   11: import build
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
```
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/build.sh`
  Observed mentions: `2`
  Focus ranges: 1-220
  Enclosing symbols: no symbols inferred
  Note: observed in failed-AC trace
  Mtime: `2026-04-03 08:08:02 KST`
  Evidence lines:
    - `Sub-AC 2 of AC 1 → Bash: /bin/zsh -c 'head -n 5 build.sh'`
    - `Sub-AC 3 of AC 1 → Bash: /bin/zsh -lc "sed -n '1,220p' build.sh"`
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
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/test_snapshot_retry_inputs.py`
  Observed mentions: `3`
  Focus ranges: none captured
  Enclosing symbols: class SnapshotRetryInputsTests [18-336], function write_research_review_baseline [19-56], function make_fake_branch [58-81], function run_helper [83-90]
  Note: observed in failed-AC trace
  Mtime: `2026-04-12 20:50:41 KST`
  Evidence lines:
    - `test_snapshot_retry_inputs.py"`
  Code excerpt:
```text
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
   29:             Path("boj28350_resume/current_state_summary.md"): "current state\n",
   30:             Path("boj28350_resume/next_session_briefing.md"): "briefing\n",
   31:             Path("boj28350_resume/progress40_derived_reference.md"): "progress40 reference\n",
   32:             Path("boj28350_complete_master_document_partA_raw.md"): "master doc\n",
   33:             Path("boj28350_integrated_technical_history.md"): "integrated history\n",
   34:             Path("boj28350_literature_progress7_bcdecomp_report.md"): "progress7 report\n",
   35:             Path("literature_grade_proof_package.md"): "proof package\n",
   36:             Path(
   37:                 "boj28350_bundle_archive/boj28350_literature_progress40_layout_signature_reuse_gate.cpp"
   38:             ): "int main() { return 0; }\n",
   39:             Path(
   40:                 "boj28350_bundle_archive/boj28350_progress40_layout_signature_reuse_gate_report.md"
   41:             ): "progress40 report\n",
   42:             Path("boj28350_bundle_archive/boj28350_progress40_results_merged.json"): "{}\n",
   43:         }
```
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/lca_smoke.sh`
  Observed mentions: `3`
  Focus ranges: none captured
  Enclosing symbols: function fail [288-293], function sanitize_shell_state [294-300], function enter_branch_root [301-308], function normalize_existing_path [309-331]
  Note: observed in failed-AC trace, boosted by failure_analysis_state
  Mtime: `2026-04-12 21:17:44 KST`
  Code excerpt:
```text
  286: RETRY_LOOP_NEXT_GATE_COMMAND="./lca_strong_gate.sh"
  287: 
  288: fail() {
  289:   LAUNCHER_FAILURE_MESSAGE="$*"
  290:   LAUNCHER_FAILURE_RC="$SMOKE_EXIT_HARNESS_FAILURE"
  291:   exit "$SMOKE_EXIT_HARNESS_FAILURE"
  292: }
  293: 
  294: sanitize_shell_state() {
  295:   unset CDPATH BASH_ENV ENV GLOBIGNORE
```
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/branch_run_case.py`
  Observed mentions: `3`
  Focus ranges: none captured
  Enclosing symbols: function parse_env_assignments [76-83], function apply_solver_env_overrides [86-90], function build_case_solver_env [93-109], function resolve_case_outdir [112-113]
  Note: observed in failed-AC trace, boosted by failure_analysis_state, matched pinned symbol `function parse_env_assignments [76-83]`
  Mtime: `2026-04-03 08:13:24 KST`
  Code excerpt:
```text
   74: 
   75: 
   76: def parse_env_assignments(items: list[str]) -> dict[str, str]:
   77:     env: dict[str, str] = {}
   78:     for item in items:
   79:         key, sep, value = item.partition("=")
   80:         if not sep or not key:
   81:             raise ValueError(f"invalid --env assignment: {item!r}")
   82:         env[key] = value
   83:     return env
   84: 
   85: 
```

#### Focused Artifact Snapshots

- No failed-AC-specific artifact roots were mapped.

#### Failed-AC Trace Excerpt

```text
.ouroboros/run_until_pass...
    Sub-AC 3 of AC 1 → Bash: /bin/zsh -lc 'rg -n 
"prepare_retry_attempt_state.py|pre_a...
    Sub-AC 3 of AC 1 → Bash: /bin/zsh -lc "sed -n '1,240p' 
.ouroboros/retry_artifact_i...
    Sub-AC 3 of AC 1 → Bash: /bin/zsh -lc "sed -n '1,240p' 
test_snapshot_retry_inputs.py"
    Sub-AC 3 of AC 1 → Bash: /bin/zsh -lc "sed -n '1,280p' 
.ouroboros/snapshot_retry_i...
    Sub-AC 3 of AC 1 → Bash: /bin/zsh -lc "sed -n '240,520p' 
test_snapshot_retry_input...
    Sub-AC 3 of AC 1 → Bash: /bin/zsh -lc 'rg -n 
"research_review_gate|pre_rewrite_che...
    Sub-AC 3 of AC 1 → Bash: /bin/zsh -lc "sed -n '280,520p' 
.ouroboros/snapshot_retry...
    Sub-AC 3 of AC 1 → Bash: /bin/zsh -lc "sed -n '1,220p' 
boj28350_resume/pre_rewrite...
    Sub-AC 3 of AC 1 → Bash: /bin/zsh -lc "sed -n '1,220p' 
boj28350_resume/pre_rewrite...
    Sub-AC 3 of AC 1 → Bash: /bin/zsh -lc 'ls -l boj28350_resume/README.md 
boj28350_re...
    Sub-AC 3 of AC 1 → Bash: /bin/zsh -lc "sed -n '1,220p' 
boj28350_resume/current_sta...
    Sub-AC 3 of AC 1 → Bash: /bin/zsh -lc "sed -n '560,760p' 
.ouroboros/prepare_retry_...
    Sub-AC 3 of AC 1 → Bash: /bin/zsh -lc 'rg -n 
"blocked_analysis_refresh|pre-attempt...
    Sub-AC 3 of AC 1 → Bash: /bin/zsh -lc "rg -n 
\"prepare_retry_attempt_state.py|run_...
    Sub-AC 3 of AC 1 → Bash: /bin/zsh -lc "sed -n '720,900p' 
test_retry_loop_artifact_...
    Sub-AC 1: Stall detected (attempt 2/3), retrying...
    Sub-AC 3 of AC 1 → Bash: /bin/zsh -lc "sed -n '1,220p' 
boj28350_resume/README.md"
    Sub-AC 3 of AC 1 → Edit: /Users/free_1/Library/Mobile 
Documents/iCloud~md~obsidian...
    Sub-AC 3 of AC 1 → Bash: /bin/zsh -lc "rg --files . | rg 
\"(boj28350_resume/README...
    Sub-AC 3 of AC 1 → Bash: /bin/zsh -lc 'rg -n "subac3|review 
gate|analysis_refresh_...
    Sub-AC 3 of AC 1 → Bash: /bin/zsh -lc 'rg -n 
"current_state_summary|next_session_b...
    Sub-AC 3 of AC 1 → Bash: /bin/zsh -lc 'rg -n 
"boj28350_branch_3_solver.cpp|build.s...
    Sub-AC 3 of AC 1 → Bash: /bin/zsh -lc "sed -n '1,220p' 
.ouroboros/snapshot_retry_i...
    Sub-AC 3 of AC 1 → Bash: /bin/zsh -lc "sed -n '1,220p' 
.ouroboros/prepare_retry_at...
    Sub-AC 3 of AC 1 → Edit: /Users/free_1/Library/Mobile 
Documents/iCloud~md~obsidian...
    Sub-AC 3 of AC 1 → Edit: /Users/free_1/Library/Mobile 
Documents/iCloud~md~obsidian...
    Sub-AC 3 of AC 1 → Edit: /Users/free_1/Library/Mobile 
Documents/iCloud~md~obsidian...
    Sub-AC 3 of AC 1 → Edit: /Users/free_1/Library/Mobile 
Documents/iCloud~md~obsidian...
    Sub-AC 2: Stall detected (attempt 1/3), retrying...
    Sub-AC 3 of AC 1 → Bash: /bin/zsh -lc "sed -n '420,540p' 
.ouroboros/prepare_retry_...
    Sub-AC 3 of AC 1 → Bash: /bin/zsh -lc "sed -n '540,620p' 
.ouroboros/prepare_retry_...
    Sub-AC 3 of AC 1 → Edit: /Users/free_1/Library/Mobile 
Documents/iCloud~md~obsidian...
    Sub-AC 3 of AC 1 → Edit: /Users/free_1/Library/Mobile 
Documents/iCloud~md~obsidian...
    Sub-AC 3 of AC 1 → Bash: /bin/zsh -lc "sed -n '1,180p' 
test_prepare_retry_attempt_...
    Sub-AC 3 of AC 1 → Bash: /bin/zsh -lc "sed -n '1,220p' 
test_snapshot_retry_inputs.py"
    Sub-AC 3 of AC 1 → Bash: /bin/zsh -lc "sed -n '1,320p' 
test_snapshot_retry_inputs.py"
    Sub-AC 3 of AC 1 → Edit: /Users/free_1/Library/Mobile 
Documents/iCloud~md~obsidian...
    Sub-AC 3 of AC 1 → Edit: /Users/free_1/Library/Mobile 
Documents/iCloud~md~obsidian...
    Sub-AC 3 of AC 1 → Edit: /Users/free_1/Library/Mobile 
Documents/iCloud~md~obsidian...
│ ### AC 1: [FAIL] The branch_3 research notes and bundled progress40          │
│ materials are read before major solver rewrites or pivots                    │
│ Decomposed into 3 Sub-ACs                                                    │
```

### AC 2: ./lca_smoke.sh is stabilized enough to support further      │

- Failure type: `stall/no-activity`
- Failure family: `generic_retry_failure`
- Interpretation lane: `pre-gate-stability`
- Primary progress40 axis: `zero_span_fastpath`
- Secondary progress40 axis: `none`
- Profile mode observed: `unknown`
- Last progress checkpoint phase: `unknown`
- Last release diag phase: `unknown`
- Suggested next probe: `./lca_smoke.sh`
- Trace lines captured: `80`

#### Narrowed Localization Snapshot

- No statement-level localization was resolved for this breakdown.

#### Progress40 Axis Evidence

- `zero_span_fastpath`
  - `AC2 stopped in smoke publication before any fresh solver/runtime/profile evidence, so keep the summary pivot only as a parked primary axis and suppress summary-derived secondary carry-forward.`

- Enabled flags seen in trace: `ENABLE_LAYOUT_R`, `ENABLE_LAYOUT_SIGNATURE_GATE_OPT`, `ENABLE_PACK_ENCODE_NORMALIZE_OPT`
- Current summary pivot baseline: `zero-span eligibility and fastpath commit`
- Current summary residual axes: `state_materialization`, `layout_gate`, `zero_span_fastpath`

#### Phase Breakdown

- `timing-timeout` x4 | sample: `"build_missing_output|build_timeout|f...`
- `case-runner` x2 | sample: `Sub-AC 2 of AC 2 → Bash: /bin/zsh -lc "sed -n '1,260p' branch_run_case.py"`
- `artifact-paths` x2 | sample: `"run_case_result|validator_ok|out.txt...`
- `wrapper` x1 | sample: `│ ### AC 2: [FAIL] ./lca_smoke.sh is stabilized enough to support further      │`

#### Latest Next-Probe Signal

- No latest probe signal was available for this AC.

#### Current-Attempt Certify Rows

- No current-attempt certify row summary was available for this failed AC.

#### Retry-Critical Anchors

- No retry-critical anchors were resolved for this failed AC.

#### Code-Structure Hotspots

- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/lca_smoke.sh`
  Observed mentions: `20`
  Focus ranges: 3610-3610, 3632-3633, 799-802, 1083-1086, 1334-1337, 706-710
  Enclosing symbols: function write_launcher_run_tracking_artifacts [3472-3867], function parse_positive_decimal_setting [793-817], function append_launcher_run_source_snapshot_row [1079-1096], function clear_launcher_source_failure_details [1309-1361], function scan_launcher_run_history_root [671-728]
  Note: observed in failed-AC trace, mapped from failed AC semantics, promoted by retry-critical anchor hints, boosted by failure_analysis_state
  Mtime: `2026-04-12 21:17:44 KST`
  Evidence lines:
    - `outer_suite_wrappers/lca_smoke.sh'`
    - `Sub-AC 3 of AC 2 → Bash: /bin/zsh -lc "sed -n '1,260p' lca_smoke.sh"`
    - `Sub-AC 3 of AC 2 → Bash: /bin/zsh -lc "sed -n '1000,1185p' lca_smoke.sh`
    - `Sub-AC 3 of AC 2 → Bash: /bin/zsh -lc "sed -n '2960,3075p' lca_smoke.sh`
    - `Sub-AC 3 of AC 2 → Bash: /bin/zsh -lc "sed -n '4780,5515p' lca_smoke.sh`
    - `Sub-AC 3 of AC 2 → Bash: /bin/zsh -lc "sed -n '4500,4725p' lca_smoke.sh"`
  Code excerpt:
```text
 3608:     "run_finished_at_utc": summary.get("run_finished_at_utc", ""),
 3609:     "run_elapsed_seconds": summary.get("run_elapsed_seconds", ""),
 3610:     "public_status": summary.get("public_status", ""),
 3611:     "acceptance_signal_status": summary.get("acceptance_signal_status", ""),
 3612:     "iteration_support_status": summary.get("iteration_support_status", ""),
```
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/outer_suite_wrappers/lca_smoke.sh`
  Observed mentions: `2`
  Focus ranges: 497-538
  Enclosing symbols: function configure_deterministic_smoke_controls [449-500], function reset_smoke_plan [501-512], function validate_smoke_manifest_row [513-555]
  Note: observed in failed-AC trace, promoted by retry-critical anchor hints
  Mtime: `2026-04-12 20:47:25 KST`
  Evidence lines:
    - `outer_suite_wrappers/lca_smoke.sh'`
    - `Sub-AC 3 of AC 2 → Bash: /bin/zsh -lc "sed -n '1,260p' lca_smoke.sh"`
    - `Sub-AC 3 of AC 2 → Bash: /bin/zsh -lc "sed -n '1000,1185p' lca_smoke.sh`
    - `Sub-AC 3 of AC 2 → Bash: /bin/zsh -lc "sed -n '2960,3075p' lca_smoke.sh`
    - `Sub-AC 3 of AC 2 → Bash: /bin/zsh -lc "sed -n '4780,5515p' lca_smoke.sh`
    - `Sub-AC 3 of AC 2 → Bash: /bin/zsh -lc "sed -n '4500,4725p' lca_smoke.sh"`
  Code excerpt:
```text
  495:   validate_positive_timeout_setting "$SMOKE_BUILD_TIMEOUT_S" "SMOKE_BUILD_TIMEOUT_S"
  496:   validate_positive_timeout_setting "$SMOKE_CASE_WALLCLOCK_GRACE_S" "SMOKE_CASE_WALLCLOCK_GRACE_S"
  497:   SMOKE_RETRY_POLICY="harness_transient_only"
  498:   SMOKE_BUILD_RETRY_POLICY="timeout_or_unexpected_exit_once"
  499: }
  500: 
  501: reset_smoke_plan() {
  502:   SMOKE_PLAN_COUNT=0
  503:   SMOKE_PLAN_STAGE=()
  504:   SMOKE_PLAN_MODE=()
  505:   SMOKE_PLAN_N=()
  506:   SMOKE_PLAN_SEED=()
  507:   SMOKE_PLAN_SHUFFLE_LABELS=()
  508:   SMOKE_PLAN_SHUFFLE_QUERIES=()
  509:   SMOKE_PLAN_TIMEOUT=()
  510:   SMOKE_PLAN_TAG=()
  511: }
  512: 
  513: validate_smoke_manifest_row() {
  514:   local line_no="$1"
  515:   local stage="$2"
  516:   local mode="$3"
  517:   local n="$4"
  518:   local seed="$5"
  519:   local shuffle_labels="$6"
  520:   local shuffle_queries="$7"
  521:   local timeout_s="$8"
  522: 
```
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/branch_run_case.py`
  Observed mentions: `5`
  Focus ranges: 260-490, 1-260
  Enclosing symbols: function generate_case [255-287], function main [290-476], function finish [310-341], function parse_env_assignments [76-83], function apply_solver_env_overrides [86-90], function build_case_solver_env [93-109]
  Note: observed in failed-AC trace, mapped from failed AC semantics, boosted by failure_analysis_state, matched pinned symbol `function parse_env_assignments [76-83]`
  Mtime: `2026-04-03 08:13:24 KST`
  Evidence lines:
    - `Sub-AC 2 of AC 2 → Bash: /bin/zsh -lc "sed -n '1,260p' branch_run_case.py"`
    - `Sub-AC 2 of AC 2 → Bash: /bin/zsh -lc "sed -n '260,490p' branch_run_case.py"`
  Code excerpt:
```text
  258:     mode: str,
  259:     n: int,
  260:     seed: int,
  261:     shuffle_labels: int,
  262:     shuffle_queries: int,
  263:     m_cap: int = 100000,
  264: ) -> tuple[Path, Path]:
  265:     outdir = ensure_case_outdir(outdir)
  266:     outdir.mkdir(parents=True, exist_ok=True)
  267:     in_path = outdir / "in.txt"
  268:     meta_path = outdir / "meta.json"
  269:     hidden_parent_path = outdir / "hidden_parent.txt"
  270: 
  271:     parent, queries, summary = branch_gen_case.build_mode(mode, n, m_cap, seed)
  272:     if shuffle_labels:
  273:         parent, queries = branch_gen_case.permute_preserving_root(
  274:             parent, queries, seed ^ 0x9E3779B1
  275:         )
  276:     if shuffle_queries:
  277:         rng = random.Random(seed ^ 0x85EBCA77)
  278:         rng.shuffle(queries)
  279: 
  280:     meta_path.write_text(json.dumps(summary, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
  281:     branch_gen_case.write_parent_file(str(hidden_parent_path), parent)
  282:     with in_path.open("w", encoding="utf-8") as fout:
  283:         fout.write(f"{n} {len(queries)}\n")
  284:         for u, v, w in queries:
  285:             fout.write(f"{u} {v} {w}\n")
```
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/suite_utils.py`
  Observed mentions: `2`
  Focus ranges: 260-390, 1-260
  Enclosing symbols: function _run_solver_windows [231-266], function _write_time_artifact [269-271], function run_solver_with_time [274-309], function median_or_none [312-315], function parse_int_list_csv [19-23], function parse_str_list_csv [26-30]
  Note: observed in failed-AC trace, mapped from failed AC semantics
  Mtime: `2026-04-09 19:57:16 KST`
  Evidence lines:
    - `Sub-AC 2 of AC 2 → Bash: /bin/zsh -lc "sed -n '1,260p' suite_utils.py"`
    - `Sub-AC 2 of AC 2 → Bash: /bin/zsh -lc "sed -n '260,390p' suite_utils.py"`
  Code excerpt:
```text
  258:                     return int(proc.returncode), True, time.perf_counter() - t0, peak_rss_kb
  259:         sample = _sample_windows_peak_rss_kb(proc)
  260:         if sample is not None:
  261:             peak_rss_kb = sample if peak_rss_kb is None else max(peak_rss_kb, sample)
  262:         return int(proc.returncode), False, time.perf_counter() - t0, peak_rss_kb
  263:     finally:
  264:         stdin_f.close()
  265:         stdout_f.close()
  266:         stderr_f.close()
  267: 
  268: 
  269: def _write_time_artifact(time_path: Path, sec: float, rss_kb: Optional[int]) -> None:
  270:     rss_text = "-1" if rss_kb is None else str(rss_kb)
  271:     time_path.write_text(f"{sec:.6f} {rss_text}\n", encoding="utf-8")
  272: 
  273: 
  274: def run_solver_with_time(
  275:     solver: Path,
  276:     in_path: Path,
  277:     out_path: Path,
  278:     time_path: Path,
  279:     stderr_path: Path,
  280:     timeout: Optional[float],
  281:     env: Optional[Mapping[str, str]] = None,
  282:     cwd: Optional[Path] = None,
  283: ) -> Tuple[int, bool, Optional[float], Optional[int]]:
  284:     if not IS_WINDOWS and hasattr(os, "wait4"):
  285:         rc, timed_out, sec, rss_kb = _run_solver_posix(
```
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifact_paths.py`
  Observed mentions: `4`
  Focus ranges: 1-260
  Enclosing symbols: function artifacts_root [80-81], function ensure_under_artifacts [84-85], function ensure_resolved_under_artifacts [88-89], function resolve_branch_artifact_path [92-102]
  Note: observed in failed-AC trace, mapped from failed AC semantics, boosted by failure_analysis_state, matched pinned symbol `function artifacts_root [80-81]`
  Mtime: `2026-04-12 07:14:52 KST`
  Evidence lines:
    - `Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "sed -n '1,260p' artifact_paths.py"`
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
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/test_lca_smoke_wrapper.py`
  Observed mentions: `11`
  Focus ranges: none captured
  Enclosing symbols: class LcaSmokeWrapperRegressionTests [24-2933], function write_text [25-27], function make_executable [29-30], function symlink_file [32-34]
  Note: observed in failed-AC trace
  Mtime: `2026-04-12 20:43:48 KST`
  Evidence lines:
    - `test_lca_smoke_wrapper.py"`
    - `test_lca_smoke_wrapper.py`
    - `test_lca_smoke_wrapper.py |...`
  Code excerpt:
```text
   22: 
   23: 
   24: class LcaSmokeWrapperRegressionTests(unittest.TestCase):
   25:     def write_text(self, path: Path, content: str) -> None:
   26:         path.parent.mkdir(parents=True, exist_ok=True)
   27:         path.write_text(content, encoding="utf-8")
   28: 
   29:     def make_executable(self, path: Path) -> None:
   30:         path.chmod(path.stat().st_mode | 0o111)
   31: 
   32:     def symlink_file(self, target: Path, link_path: Path) -> None:
   33:         link_path.parent.mkdir(parents=True, exist_ok=True)
   34:         link_path.symlink_to(target)
   35: 
   36:     def make_fake_runtime_branch(self, temp_root: Path, *, run_case_body: str) -> Path:
   37:         branch_root = temp_root / "branch"
   38:         self.symlink_file(WRAPPER_PATH, branch_root / "outer_suite_wrappers" / "lca_smoke.sh")
   39:         self.write_text(
   40:             branch_root / "artifact_paths.py",
   41:             textwrap.dedent(
   42:                 """
   43:                 #!/usr/bin/env python3
   44:                 from __future__ import annotations
   45: 
   46:                 import argparse
   47:                 from pathlib import Path
   48: 
   49:                 BRANCH_ROOT = Path(__file__).resolve().parent
```
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/lca_smoke_repeatability.sh`
  Observed mentions: `6`
  Focus ranges: none captured
  Enclosing symbols: no symbols inferred
  Note: observed in failed-AC trace, mapped from failed AC semantics
  Mtime: `2026-03-25 11:25:35 KST`
  Evidence lines:
    - `Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc 'ls -l lca_smoke_repeatability.sh`
    - `lca_smoke_repeatability.sh"`
    - `Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc 'wc -l lca_smoke_repeatability.sh`
  Code excerpt:
```text
    1: #!/usr/bin/env bash
    2: set -euo pipefail
    3: 
    4: SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
    5: exec "$SCRIPT_DIR/outer_suite_wrappers/lca_smoke_repeatability.sh" "$@"
```
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/lca_required_repeatability.sh`
  Observed mentions: `1`
  Focus ranges: none captured
  Enclosing symbols: no symbols inferred
  Note: observed in failed-AC trace
  Mtime: `2026-04-03 05:40:47 KST`
  Evidence lines:
    - `lca_required_repeatability.sh"`
  Code excerpt:
```text
    1: #!/usr/bin/env bash
    2: set -euo pipefail
    3: 
    4: SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
    5: exec "$SCRIPT_DIR/outer_suite_wrappers/lca_required_repeatability.sh" "$@"
```

#### Focused Artifact Snapshots

- `smoke` latest: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke/summary.txt`
  mtime: `2026-04-12 19:41:58 KST`
  attempt start: `2026-04-12 19:42:09 KST`
  fresh within attempt: `no`
  summary: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke/summary.txt`
  freshness note: latest summary/file predates the failed attempt start; treat it as carried-forward evidence, not fresh gate output
- `smoke_repeatability` latest: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_repeatability/summary.txt`
  mtime: `2026-04-10 13:17:32 KST`
  attempt start: `2026-04-12 19:42:09 KST`
  fresh within attempt: `no`
  summary: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke_repeatability/summary.txt`
  freshness note: latest summary/file predates the failed attempt start; treat it as carried-forward evidence, not fresh gate output
- `tmp` latest: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/.tmp/retry_loop.runtime.env.3d7hoB/tmp/ouroboros-codex-1o2fvqdq.txt`
  mtime: `2026-04-12 22:10:18 KST`
  attempt start: `2026-04-12 19:42:09 KST`
  fresh within attempt: `yes`
  freshness note: latest summary/file was refreshed during this failed attempt

#### Failed-AC Trace Excerpt

```text
outer_suite_wrappers/lca_re...
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "sed -n '1,260p' 
outer_suite_wrappers/lca_ac...
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "sed -n '1,420p' 
test_lca_acceptance_repeata...
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc 'rg -n 
"run_smoke_once|SMOKE_WRAPPER|smoke d...
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "sed -n '1,120p' 
lca_smoke_repeatability.sh"
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "sed -n '260,620p' 
outer_suite_wrappers/lca_...
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc 'rg -n 
"repeatability|comparison|run_compari...
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "sed -n '1,260p' 
lca_smoke_repeatability.sh"
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "sed -n '1,120p' 
lca_smoke_repeatability.sh"
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "sed -n '1,160p' 
lca_smoke_repeatability.sh"
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc 'wc -l lca_smoke_repeatability.sh 
test_lca_s...
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "sed -n '620,1040p' 
outer_suite_wrappers/lca...
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "sed -n '1040,1220p' 
outer_suite_wrappers/lc...
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "sed -n '1,120p' 
test_gate_launcher_dispatch...
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc 'pytest -q 
test_lca_smoke_repeatability_wrap...
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "sed -n '3360,3405p' lca_smoke.sh"
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "sed -n '3140,3225p' lca_smoke.sh"
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc 'rg -n 
"lca_acceptance_repeatability|REQUIRE...
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc 'rg -n 
"LCA_SMOKE_EXPORT_SNAPSHOT_ROOT|check...
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "sed -n '520,760p' 
outer_suite_wrappers/lca_...
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "sed -n '980,1210p' 
outer_suite_wrappers/lca...
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "sed -n '760,980p' 
outer_suite_wrappers/lca_...
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "rg -n 
'lca_smoke_repeatability|lca_acceptan...
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "sed -n '1,260p' artifact_paths.py"
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc 'rg -n 
"gate_chain_ac2|build_gate_chain|stro...
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc 'rg -n "lca_smoke_repeatability|smoke 
repeat...
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "rg --files | rg 
'("'^lca_.*'"\\.sh"'$|''^te...
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "rg -n 
\"lca_smoke_repeatability|repeatabili...
    Sub-AC 4: Stall detected (attempt 1/3), retrying...
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "nl -ba 
outer_suite_wrappers/lca_acceptance_...
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "nl -ba 
outer_suite_wrappers/lca_acceptance_...
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "nl -ba 
outer_suite_wrappers/lca_acceptance_...
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "nl -ba 
outer_suite_wrappers/lca_acceptance_...
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "nl -ba 
outer_suite_wrappers/lca_acceptance_...
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "nl -ba 
test_lca_acceptance_repeatability_wr...
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "nl -ba 
test_lca_acceptance_repeatability_wr...
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "nl -ba 
test_lca_acceptance_repeatability_wr...
    Sub-AC 4: Stall detected (attempt 2/3), retrying...
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "sed -n '10,18p' 
test_lca_acceptance_repeata...
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "sed -n '175,240p' 
test_lca_acceptance_repea...
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "sed -n '258,390p' 
test_lca_acceptance_repea...
    Sub-ACs completed: 1/4 succeeded
│ ### AC 2: [FAIL] ./lca_smoke.sh is stabilized enough to support further      │
│ iteration                                                                    │
│ Decomposed into 4 Sub-ACs                                                    │
```

### AC 3: ./lca_strong_gate.sh passes as a required prerequisite gate │

- Failure type: `stall/no-activity`
- Failure family: `strong_gate_unspecified`
- Interpretation lane: `correctness-proof`
- Primary progress40 axis: `state_materialization`
- Secondary progress40 axis: `zero_span_fastpath`
- Profile mode observed: `unknown`
- Last progress checkpoint phase: `unknown`
- Last release diag phase: `unknown`
- Suggested next probe: `LCA_STAGE_FILTER=correctness_fuzz ./lca_strong_gate.sh`
- Trace lines captured: `80`

#### Narrowed Localization Snapshot

- No statement-level localization was resolved for this breakdown.

#### Progress40 Axis Evidence

- `zero_span_fastpath`
  - `AC 3 → Bash: /bin/zsh -lc 'rg -n "zero_span|zero-span|materializ|layou...`
  - `AC 3 → Bash: /bin/zsh -lc 'rg -n "zero_span|zero-span|materializ|layou...`
  - `AC 3 → Bash: /bin/zsh -lc "rg -n \"aggregate|zero-span|zero span|layou...`
  - `9401:                                                             __acc_lreuse(__dt_noop_commit, &g_batch_dbg.time_lreuse_noop_fastpath_commit_ns, &g_batch_dbg.time_lreuse_noop_fastpath_commit_calls);`
- `watch_diff`
  - `function retainClassWatchByKeepMask [7666-10670]`
- `retain_compaction`
  - `function retainClassWatchByKeepMask [7666-10670]`
- `state_materialization`
  - `133: export ENABLE_TSCAN_CORE_OPT="${ENABLE_TSCAN_CORE_OPT:-1}"`
  - `134: export ENABLE_TSCAN_BRANCH_STATE_OPT="${ENABLE_TSCAN_BRANCH_STATE_OPT:-1}"`
  - `147: # materialization path is re-enabled together with the pointer / pack /`
  - `152: export ENABLE_STATE_LOAD_MATERIALIZATION_OPT="${ENABLE_STATE_LOAD_MATERIALIZATION_OPT:-1}"`

- Enabled flags seen in trace: `ENABLE_LAYOUT_R`, `ENABLE_LAYOUT_SIGNATURE_GATE_OPT`, `ENABLE_PACK_ENCODE_NORMALIZE_OPT`
- Current summary pivot baseline: `zero-span eligibility and fastpath commit`
- Current summary residual axes: `state_materialization`, `layout_gate`, `zero_span_fastpath`

#### Phase Breakdown

- `timing-timeout` x3 | sample: `AC 3 → Bash: /bin/zsh -lc "sed -n '1,220p' solver_release_env.sh"`
- `certify` x1 | sample: `AC 3 → Bash: /bin/zsh -lc "sed -n '500,640p' branch_certify_suite.py"`
- `build` x1 | sample: `AC 3 → Bash: /bin/zsh -lc ./build.sh`
- `wrapper` x1 | sample: `│ ### AC 3: [FAIL] ./lca_strong_gate.sh passes as a required prerequisite gate │`

#### Latest Next-Probe Signal

- No latest probe signal was available for this AC.

#### Current-Attempt Certify Rows

- No current-attempt certify row summary was available for this failed AC.

#### Retry-Critical Anchors

- No retry-critical anchors were resolved for this failed AC.

#### Code-Structure Hotspots

- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/outer_suite_wrappers/lca_strong_gate.sh`
  Observed mentions: `4`
  Focus ranges: 498-499, 757-758, 743-747, 484-489, 430-442, 681-701
  Enclosing symbols: function clear_solver_snapshot_artifacts [494-517], function lock_age_seconds [749-761], function lock_has_recent_activity [731-748], function archive_incomplete_published_output [459-485], function clear_invalid_root_path [486-493], function prune_failure_archive [395-433]
  Note: observed in failed-AC trace, mapped from failed AC semantics, promoted by retry-critical anchor hints, inferred from truncated wrapper sed trace
  Mtime: `2026-04-12 10:57:05 KST`
  Evidence lines:
    - `AC 3 → Bash: /bin/zsh -lc "sed -n '1,260p' lca_strong_gate.sh"`
    - `│ ### AC 3: [FAIL] ./lca_strong_gate.sh passes as a required prerequisite gate │`
    - `AC 3 → Bash: /bin/zsh -lc "sed -n '250,420p' outer_suite_wrappers/lca_...`
    - `AC 3 → Bash: /bin/zsh -lc "sed -n '680,840p' outer_suite_wrappers/lca_...`
    - `AC 3 → Bash: /bin/zsh -lc "sed -n '1,220p' outer_suite_wrappers/lca_st...`
  Code excerpt:
```text
  496:   local snapshot_parent=""
  497: 
  498:   clear_invalid_root_path "$SNAPSHOT_ROOT" "solver snapshot root"
  499:   if [[ -d "$SNAPSHOT_ROOT" ]]; then
  500:     shopt -s nullglob
  501:     for stale in "$SNAPSHOT_ROOT"/lca_strong_gate.solver.*; do
```
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/branch_certify_suite.py`
  Observed mentions: `1`
  Focus ranges: 613-614, 540-549, 557-568, 500-640
  Enclosing symbols: function run_one_case [506-622], function _write_case_result [475-503], function main [625-638]
  Note: observed in failed-AC trace, mapped from failed AC semantics, promoted by retry-critical anchor hints
  Mtime: `2026-04-12 06:52:22 KST`
  Evidence lines:
    - `AC 3 → Bash: /bin/zsh -lc "sed -n '500,640p' branch_certify_suite.py"`
  Code excerpt:
```text
  611:             status="pass",
  612:             category="pass",
  613:             exit_code=0,
  614:             message="case completed successfully",
  615:             solver_exit_code=rc_sol,
  616:             validator_ok=True,
```
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp`
  Observed mentions: `0`
  Focus ranges: 9401-9402, 9394-9396, 9367-9370, 9360-9364
  Enclosing symbols: function retainClassWatchByKeepMask [7666-10670]
  Note: mapped from failed AC semantics, promoted by retry-critical anchor hints
  Mtime: `2026-04-04 17:13:04 KST`
  Code excerpt:
```text
 9399:                                                             __acc_lreuse(__dt_skip_commit, &g_batch_dbg.time_lreuse_skip_apply_commit_ns, &g_batch_dbg.time_lreuse_skip_apply_commit_calls);
 9400:                                                             g_batch_dbg.lreuse_skip_apply_commit_calls++;
 9401:                                                             __acc_lreuse(__dt_noop_commit, &g_batch_dbg.time_lreuse_noop_fastpath_commit_ns, &g_batch_dbg.time_lreuse_noop_fastpath_commit_calls);
 9402:                                                             g_batch_dbg.lreuse_noop_commit_hits++;
 9403:                                                             if (__lgate_opt) {
 9404:                                                                 long long __dt_src = std::max(1LL, __dt_sig_load / 2);
```
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/solver_release_env.sh`
  Observed mentions: `2`
  Focus ranges: 132-150, 1-220
  Enclosing symbols: function __solver_release_env_require_under_artifacts [65-186], function __solver_release_env_resolve_existing_artifact_path [45-51], function __solver_release_env_keep_or_set [52-64]
  Note: observed in failed-AC trace
  Mtime: `2026-04-12 15:08:32 KST`
  Evidence lines:
    - `AC 3 → Bash: /bin/zsh -lc "sed -n '1,220p' solver_release_env.sh"`
    - `AC 3 → Bash: /bin/zsh -lc "sed -n '132,150p' solver_release_env.sh"`
  Code excerpt:
```text
  130: export ENABLE_COPY_PLAN_BUILD_OPT="${ENABLE_COPY_PLAN_BUILD_OPT:-1}"
  131: export ENABLE_RUN_DISCOVERY_FUSION_OPT="${ENABLE_RUN_DISCOVERY_FUSION_OPT:-1}"
  132: export ENABLE_FUSED_DISCOVERY_CLASSIFY_OPT="${ENABLE_FUSED_DISCOVERY_CLASSIFY_OPT:-1}"
  133: export ENABLE_TSCAN_CORE_OPT="${ENABLE_TSCAN_CORE_OPT:-1}"
  134: export ENABLE_TSCAN_BRANCH_STATE_OPT="${ENABLE_TSCAN_BRANCH_STATE_OPT:-1}"
  135: # Keep AC3 unanimous support reuse enabled on branch_3. The re-anchored March
  136: # 28 AC3 line still relies on this broad gate to avoid falling back to repeated
  137: # full support rebuilds on the dense strong-gate families.
  138: export AC3_SUPPORT_REUSE_MAX_TOUCHED="${AC3_SUPPORT_REUSE_MAX_TOUCHED:-100000}"
  139: # The current correctness-fuzz blocker on the progress40 line is the
  140: # `caterpillar_rect_dense n=1024` corridor. Disabling the single-positive reuse
  141: # shortcut in release runs keeps the broader support-reuse machinery intact
  142: # while avoiding the branch-local fastpath that pushes the exact seed-2 blocker
  143: # over the 2s prerequisite-gate budget.
  144: export AC3_ALLOW_SINGLE_POSITIVE_REUSE="${AC3_ALLOW_SINGLE_POSITIVE_REUSE:-0}"
  145: # Fresh 2026-04-09 same-worktree probes on the rebuilt branch show that the
  146: # progress40 late stack only turns into a net win once the state-load
  147: # materialization path is re-enabled together with the pointer / pack /
  148: # normalize chain. With that full line active, `comb_rect_dense n=1024`
  149: # improved from about 1.96s to 1.81s and `comb_dense n=2048` from about 7.86s
  150: # to 6.53s. Keep the flag overridable, but make the branch-local default follow
  151: # the currently strongest reproducible progress40-aligned release mix.
  152: export ENABLE_STATE_LOAD_MATERIALIZATION_OPT="${ENABLE_STATE_LOAD_MATERIALIZATION_OPT:-1}"
```
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/lca_strong_gate.sh`
  Observed mentions: `2`
  Focus ranges: 1-260
  Enclosing symbols: no symbols inferred
  Note: observed in failed-AC trace, mapped from failed AC semantics
  Mtime: `2026-04-03 05:40:56 KST`
  Evidence lines:
    - `AC 3 → Bash: /bin/zsh -lc "sed -n '1,260p' lca_strong_gate.sh"`
    - `│ ### AC 3: [FAIL] ./lca_strong_gate.sh passes as a required prerequisite gate │`
  Code excerpt:
```text
    1: #!/usr/bin/env bash
    2: set -euo pipefail
    3: 
    4: SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
    5: exec "$SCRIPT_DIR/outer_suite_wrappers/lca_strong_gate.sh" "$@"
```
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/build.sh`
  Observed mentions: `2`
  Focus ranges: 1-260
  Enclosing symbols: no symbols inferred
  Note: observed in failed-AC trace
  Mtime: `2026-04-03 08:08:02 KST`
  Evidence lines:
    - `AC 3 → Bash: /bin/zsh -lc "sed -n '1,260p' build.sh"`
    - `AC 3 → Bash: /bin/zsh -lc ./build.sh`
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
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/lca_smoke.sh`
  Observed mentions: `3`
  Focus ranges: none captured
  Enclosing symbols: function fail [288-293], function sanitize_shell_state [294-300], function enter_branch_root [301-308], function normalize_existing_path [309-331]
  Note: observed in failed-AC trace, boosted by failure_analysis_state
  Mtime: `2026-04-12 21:17:44 KST`
  Code excerpt:
```text
  286: RETRY_LOOP_NEXT_GATE_COMMAND="./lca_strong_gate.sh"
  287: 
  288: fail() {
  289:   LAUNCHER_FAILURE_MESSAGE="$*"
  290:   LAUNCHER_FAILURE_RC="$SMOKE_EXIT_HARNESS_FAILURE"
  291:   exit "$SMOKE_EXIT_HARNESS_FAILURE"
  292: }
  293: 
  294: sanitize_shell_state() {
  295:   unset CDPATH BASH_ENV ENV GLOBIGNORE
```
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/branch_run_case.py`
  Observed mentions: `3`
  Focus ranges: none captured
  Enclosing symbols: function parse_env_assignments [76-83], function apply_solver_env_overrides [86-90], function build_case_solver_env [93-109], function resolve_case_outdir [112-113]
  Note: observed in failed-AC trace, boosted by failure_analysis_state, matched pinned symbol `function parse_env_assignments [76-83]`
  Mtime: `2026-04-03 08:13:24 KST`
  Code excerpt:
```text
   74: 
   75: 
   76: def parse_env_assignments(items: list[str]) -> dict[str, str]:
   77:     env: dict[str, str] = {}
   78:     for item in items:
   79:         key, sep, value = item.partition("=")
   80:         if not sep or not key:
   81:             raise ValueError(f"invalid --env assignment: {item!r}")
   82:         env[key] = value
   83:     return env
   84: 
   85: 
```

#### Focused Artifact Snapshots

- `strong_gate` latest: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_probe_v3.latest_failure/RESTORED_FROM_RETRY_FIXTURES.md`
  mtime: `2026-04-12 20:31:20 KST`
  attempt start: `2026-04-12 19:42:09 KST`
  fresh within attempt: `yes`
  freshness note: latest summary/file was refreshed during this failed attempt
- `tmp` latest: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/.tmp/retry_loop.runtime.env.3d7hoB/tmp/ouroboros-codex-1o2fvqdq.txt`
  mtime: `2026-04-12 22:10:18 KST`
  attempt start: `2026-04-12 19:42:09 KST`
  fresh within attempt: `yes`
  freshness note: latest summary/file was refreshed during this failed attempt

#### Failed-AC Trace Excerpt

```text
  AC 3 → Bash: /bin/zsh -lc "sed -n '1,220p' artifacts/lca_tree_stress_v...
  AC 3 → Bash: /bin/zsh -lc "sed -n '1,220p' boj28350_resume/README.md"
  AC 3 → Bash: /bin/zsh -lc "sed -n '1,260p' boj28350_resume/current_sta...
  AC 3 → Bash: /bin/zsh -lc "sed -n '1,260p' boj28350_resume/next_sessio...
  AC 3 → Bash: /bin/zsh -lc "sed -n '1,260p' lca_strong_gate.sh"
  AC 3 → Bash: /bin/zsh -lc "sed -n '1,260p' build.sh"
  AC 3 → Bash: /bin/zsh -lc "sed -n '1,220p' boj28350_resume/boj28350_br...
  AC 3 → Bash: /bin/zsh -lc 'rg -n "zero_span|zero-span|materializ|layou...
  AC 3 → Bash: /bin/zsh -lc 'rg -n "zero_span|zero-span|materializ|layou...
  AC 3 → Bash: /bin/zsh -lc "sed -n '8720,8875p' boj28350_bundle_archive...
  AC 3 → Bash: /bin/zsh -lc "sed -n '9500,9665p' boj28350_resume/boj2835...
  AC 3 → Bash: /bin/zsh -lc "find artifacts/lca_tree_stress_v5 -path '*s...
  AC 3 → Bash: /bin/zsh -lc "sed -n '1,220p' artifacts/lca_tree_stress_v...
  AC 3 → Bash: /bin/zsh -lc "sed -n '250,420p' outer_suite_wrappers/lca_...
  AC 3 → Bash: /bin/zsh -lc 'rg -n "LCA_STAGE_FILTER|correctness_fuzz|co...
  AC 3 → Bash: /bin/zsh -lc "python3 - <<'PY'
import csv
from collection...
  AC 3 → Bash: /bin/zsh -lc "sed -n '680,840p' outer_suite_wrappers/lca_...
  AC 3 → Bash: /bin/zsh -lc "sed -n '500,640p' branch_certify_suite.py"
  AC 3 → Bash: /bin/zsh -lc "sed -n '1,220p' solver_release_env.sh"
  AC 3 → Bash: /bin/zsh -lc "sed -n '1,220p' outer_suite_wrappers/lca_st...
  AC 3 → Bash: /bin/zsh -lc ./build.sh
  AC 3 → Bash: /bin/zsh -lc "rg -n \"aggregate|zero-span|zero span|layou...
  AC 3 → Bash: /bin/zsh -lc "sed -n '1,260p' boj28350_bundle_archive/boj...
  AC 3 → Bash: /bin/zsh -lc "python3 - <<'PY'
from pathlib import Path
f...
  AC 3 → Bash: /bin/zsh -lc 'LCA_STAGE_FILTER=correctness_fuzz ./lca_str...
  AC 3 → Bash: /bin/zsh -lc "sed -n '1,240p' artifacts/lca_tree_stress_v...
  AC 3 → Bash: /bin/zsh -lc "sed -n '1,260p' artifacts/lca_tree_stress_v...
  AC 3 → Bash: /bin/zsh -lc "sed -n '1,260p' artifacts/lca_tree_stress_v...
  AC 3 → Bash: /bin/zsh -lc "python3 - <<'PY'
from pathlib import Path
i...
  AC 3 → Bash: /bin/zsh -lc "python3 - <<'PY'
import csv
from collection...
  AC 3 → Bash: /bin/zsh -lc "sed -n '1,220p' artifacts/lca_tree_stress_v...
  AC 3 → Bash: /bin/zsh -lc "sed -n '1,220p' artifacts/lca_tree_stress_v...
  AC 3 → Bash: /bin/zsh -lc "sed -n '1,220p' artifacts/lca_tree_stress_v...
  AC 3 → Bash: /bin/zsh -lc "sed -n '1,220p' artifacts/lca_tree_stress_v...
  AC 3 → Bash: /bin/zsh -lc "sed -n '1,220p' artifacts/lca_tree_stress_v...
  AC 3 → Bash: /bin/zsh -lc "find artifacts/lca_tree_stress_v5/strong_ga...
  AC 3 → Bash: /bin/zsh -lc "find artifacts/lca_tree_stress_v5/strong_ga...
  AC 3 → Bash: /bin/zsh -lc "sed -n '1,260p' artifacts/lca_tree_stress_v...
  AC 3 → Bash: /bin/zsh -lc "sed -n '1,260p' artifacts/lca_tree_stress_v...
  AC 3 → Bash: /bin/zsh -lc "sed -n '1,260p' artifacts/lca_tree_stress_v...
  AC 3 → Bash: /bin/zsh -lc "sed -n '1,260p' artifacts/lca_tree_stress_v...
  AC 3 → Bash: /bin/zsh -lc "sed -n '1,220p' artifacts/lca_tree_stress_v...
  AC 3 → Bash: /bin/zsh -lc "find artifacts/lca_tree_stress_v5/strong_ga...
  AC 3 → Bash: /bin/zsh -lc "find artifacts/lca_tree_stress_v5/strong_ga...
  AC 3 → Bash: /bin/zsh -lc "sed -n '1,220p' artifacts/lca_tree_stress_v...
  AC 3 → Bash: /bin/zsh -lc "sed -n '8320,8455p' boj28350_resume/boj2835...
  AC 3 → Bash: /bin/zsh -lc 'rg -n "DENSE_PROFILE_OUTDIR|ENABLE_LAYOUT_R...
  AC 3 → Bash: /bin/zsh -lc "sed -n '1080,1215p' boj28350_resume/boj2835...
  AC 3 → Bash: /bin/zsh -lc "sed -n '1,220p' artifacts/lca_tree_stress_v...
  AC 3 → Bash: /bin/zsh -lc 'rg --files artifacts/lca_tree_stress_v5/str...
  AC 3 → Bash: /bin/zsh -lc 'rg --files artifacts/lca_tree_stress_v5/str...
  AC 3 → Bash: /bin/zsh -lc "rg --files artifacts/lca_tree_stress_v5 | r...
  AC 3 → Bash: /bin/zsh -lc 'command -v gtimeout || command -v timeout |...
  AC 3 → Bash: /bin/zsh -lc "sed -n '980,1055p' boj28350_resume/boj28350...
  AC 3 → Bash: /bin/zsh -lc 'rg -n "ac3_support_reuse_max_touched|allow_...
  AC 3 → Bash: /bin/zsh -lc "sed -n '16040,16160p' boj28350_resume/boj28...
  AC 3 → Bash: /bin/zsh -lc "sed -n '11640,11710p' boj28350_resume/boj28...
  AC 3 → Bash: /bin/zsh -lc 'rg -n "allowSupportReuseThisDeletion|suppor...
  AC 3 → Bash: /bin/zsh -lc "sed -n '16160,16440p' boj28350_resume/boj28...
  AC 3 → Bash: /bin/zsh -lc "sed -n '1,240p' artifacts/lca_tree_stress_v...
  AC 3 → Bash: /bin/zsh -lc "python3 - <<'PY'
import json
from pathlib i...
  AC 3 → Bash: /bin/zsh -lc "python3 - <<'PY'
from __future__ import ann...
  AC 3 → Bash: /bin/zsh -lc "python3 - <<'PY'
from __future__ import ann...
  AC 3 → Bash: /bin/zsh -lc "sed -n '132,150p' solver_release_env.sh"
  AC 3 → Bash: /bin/zsh -lc "sed -n '1008,1036p' boj28350_resume/boj2835...
│ ### AC 3: [FAIL] ./lca_strong_gate.sh passes as a required prerequisite gate │
│ Error: Stalled (no activity for 300s)                                        │
│                                                                              │
```

### AC 8: All generated outputs remain inside branch_3/artifacts/...  │

- Failure type: `stall/no-activity`
- Failure family: `generic_retry_failure`
- Interpretation lane: `pre-gate-stability`
- Primary progress40 axis: `zero_span_fastpath`
- Secondary progress40 axis: `state_materialization`
- Profile mode observed: `unknown`
- Last progress checkpoint phase: `unknown`
- Last release diag phase: `unknown`
- Suggested next probe: `./lca_smoke.sh`
- Trace lines captured: `62`

#### Narrowed Localization Snapshot

- No statement-level localization was resolved for this breakdown.

#### Progress40 Axis Evidence

- No direct axis evidence was extracted from the trace; fallback axis came from the current progress40 summary.

- Enabled flags seen in trace: `ENABLE_LAYOUT_R`, `ENABLE_LAYOUT_SIGNATURE_GATE_OPT`, `ENABLE_PACK_ENCODE_NORMALIZE_OPT`
- Current summary pivot baseline: `zero-span eligibility and fastpath commit`
- Current summary residual axes: `state_materialization`, `layout_gate`, `zero_span_fastpath`

#### Phase Breakdown

- `artifact-paths` x2 | sample: `AC 8 → Bash: /bin/zsh -lc "sed -n '1,260p' artifact_paths.py"`
- `timing-timeout` x1 | sample: `AC 8 → Bash: /bin/zsh -lc 'rg -n "timeout_regression|corridor|latest_f...`

#### Latest Next-Probe Signal

- No latest probe signal was available for this AC.

#### Current-Attempt Certify Rows

- No current-attempt certify row summary was available for this failed AC.

#### Retry-Critical Anchors

- No retry-critical anchors were resolved for this failed AC.

#### Code-Structure Hotspots

- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/test_boj28350_resume.py`
  Observed mentions: `3`
  Focus ranges: 260-340, 1-120, 220-420
  Enclosing symbols: class Boj28350ResumeRegressionManifestTests [238-339], function test_ac3_timeout_regression_manifest_tracks_current_timeout_corridor [263-318], function test_saved_ac3_timeout_fixture_snapshot_covers_label_sensitive_caterpillar_case [320-339], class Boj28350ResumeArtifactLocalityTests [14-235], function assert_under_artifacts [15-21], function test_build_solver_always_passes_artifact_rooted_output [23-38]
  Note: observed in failed-AC trace
  Mtime: `2026-04-09 20:12:20 KST`
  Evidence lines:
    - `AC 8 → Bash: /bin/zsh -lc "sed -n '260,340p' test_boj28350_resume.py"`
    - `AC 8 → Bash: /bin/zsh -lc "sed -n '1,120p' test_boj28350_resume.py"`
    - `AC 8 → Bash: /bin/zsh -lc "sed -n '220,420p' test_boj28350_resume.py"`
  Code excerpt:
```text
  258:         / "retry_loop"
  259:         / "ac3_timeout_regression_runs_v2"
  260:         / "correctness_fuzz_caterpillar_rect_dense_512_s1_L1_Q0_t2"
  261:     )
  262: 
  263:     def test_ac3_timeout_regression_manifest_tracks_current_timeout_corridor(self) -> None:
  264:         rows = boj28350_resume.parse_cases(self.REGRESSION_MANIFEST)
  265: 
  266:         self.assertEqual(len(rows), 7)
  267:         self.assertTrue(all(row["stage"] == "correctness_fuzz" for row in rows))
  268:         self.assertTrue(all(row["timeout_s"] == "2.0" for row in rows))
  269:         self.assertTrue(all(row["source_outcome"] == "timeout" for row in rows))
  270: 
  271:         actual = {
  272:             (
  273:                 row["mode"],
  274:                 int(row["n"]),
  275:                 int(row["seed"]),
  276:                 int(row["shuffle_labels"]),
  277:                 int(row["shuffle_queries"]),
  278:                 row["cluster_role"],
  279:             )
  280:             for row in rows
  281:         }
  282:         expected = {
  283:             ("comb_rect_dense", 1024, 1, 0, 0, "primary_confirmed_timeout"),
  284:             ("comb_rect_dense", 1024, 1, 1, 1, "primary_confirmed_timeout"),
  285:             ("multi_comb_rect", 1024, 1, 1, 0, "primary_confirmed_timeout"),
```
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/test_lca_smoke_wrapper.py`
  Observed mentions: `2`
  Focus ranges: 2390-2465, 240-320
  Enclosing symbols: class LcaSmokeWrapperRegressionTests [24-2933], function test_successful_inner_run_keeps_launcher_owned_tmpdir_intact [2344-2403], function test_missing_main_prerequisite_preserves_stale_smoke_state_until_validation_passes [2405-2457], function test_broken_main_python_prerequisite_preserves_stale_smoke_state_until_validation_passes [2459-2514], function test_success_cleanup_marks_shared_state_as_already_cleaned [241-252], function test_deterministic_controls_allow_branch_local_build_timeout_override [254-264]
  Note: observed in failed-AC trace
  Mtime: `2026-04-12 20:43:48 KST`
  Evidence lines:
    - `AC 8 → Bash: /bin/zsh -lc "sed -n '240,320p' test_lca_smoke_wrapper.py"`
    - `AC 8 → Bash: /bin/zsh -lc "sed -n '2390,2465p' test_lca_smoke_wrapper.py"`
  Code excerpt:
```text
 2388:                 ["./outer_suite_wrappers/lca_smoke.sh"],
 2389:                 cwd=branch_root,
 2390:                 capture_output=True,
 2391:                 text=True,
 2392:                 check=False,
 2393:             )
 2394: 
 2395:             self.assertEqual(result.returncode, 0, msg=result.stderr)
 2396:             self.assertTrue(
 2397:                 (launcher_tmpdir / "sentinel.txt").is_file(),
 2398:                 msg="inner smoke execution must not delete the launcher-owned tmpdir while the parent wrapper may still need it",
 2399:             )
 2400:             self.assertTrue(
 2401:                 (branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke").is_dir(),
 2402:                 msg="the smoke output tree must still publish after preserving the launcher-owned tmpdir",
 2403:             )
 2404: 
 2405:     def test_missing_main_prerequisite_preserves_stale_smoke_state_until_validation_passes(self) -> None:
 2406:         with tempfile.TemporaryDirectory() as tmp:
 2407:             branch_root = self.make_fake_runtime_branch(Path(tmp), run_case_body="from __future__ import annotations")
 2408:             artifacts_root = branch_root / "artifacts" / "lca_tree_stress_v5"
 2409:             setup_root = artifacts_root / "smoke_setup"
 2410:             session_root = artifacts_root / ".tmp" / "lca_smoke.session"
 2411:             setup_tmpdir = artifacts_root / ".tmp" / "lca_smoke.setup.tmp"
 2412:             failure_root = artifacts_root / "smoke_latest_failure"
 2413:             run_tmpdir = artifacts_root / ".tmp" / "lca_smoke.tmp.stale"
 2414:             run_workdir = artifacts_root / ".tmp" / "lca_smoke.run.stale"
 2415: 
```
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifact_paths.py`
  Observed mentions: `4`
  Focus ranges: 1-260
  Enclosing symbols: function artifacts_root [80-81], function ensure_under_artifacts [84-85], function ensure_resolved_under_artifacts [88-89], function resolve_branch_artifact_path [92-102]
  Note: observed in failed-AC trace, mapped from failed AC semantics, boosted by failure_analysis_state, matched pinned symbol `function artifacts_root [80-81]`
  Mtime: `2026-04-12 07:14:52 KST`
  Evidence lines:
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
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/lca_smoke.sh`
  Observed mentions: `4`
  Focus ranges: 5790-6015
  Enclosing symbols: function write_launcher_failure_bundle [5602-5819], function report_launcher_failure_context [5820-5863], function bootstrap_clean_env [5864-5913], function resolve_bash_bin [5914-5926]
  Note: observed in failed-AC trace, boosted by failure_analysis_state
  Mtime: `2026-04-12 21:17:44 KST`
  Evidence lines:
    - `AC 8 → Bash: /bin/zsh -lc "sed -n '5790,6015p' lca_smoke.sh"`
  Code excerpt:
```text
 5788:       echo
 5789:       echo "- Kind: \`$LAUNCHER_LAST_CHECK_KIND\`"
 5790:       echo "- Label: \`$LAUNCHER_LAST_CHECK_LABEL\`"
 5791:       echo "- Status: \`$LAUNCHER_LAST_CHECK_STATUS\`"
 5792:       echo "- Detail: \`$LAUNCHER_LAST_CHECK_DETAIL\`"
 5793:       if [[ -n "$LAUNCHER_LAST_CHECK_ARTIFACT" ]]; then
 5794:         echo "- Artifact: \`$LAUNCHER_LAST_CHECK_ARTIFACT\`"
 5795:       fi
 5796:     fi
 5797:     echo
 5798:     echo "## Commands"
 5799:     echo
 5800:     echo "Invocation command:"
 5801:     echo
 5802:     echo "\`\`\`bash"
 5803:     echo "$LAUNCHER_INVOCATION_COMMAND"
 5804:     echo "\`\`\`"
 5805:     echo
 5806:     echo "Intended inner-wrapper dispatch command:"
 5807:     echo
 5808:     echo "\`\`\`bash"
 5809:     echo "$LAUNCHER_DISPATCH_COMMAND"
 5810:     echo "\`\`\`"
 5811:     echo
 5812:     echo "## Retry Next"
 5813:     echo
 5814:     echo "- Retry command: \`./lca_smoke.sh\`"
 5815:     echo "- Guidance: \`$triage_retry_hint\`"
```
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/run.sh`
  Observed mentions: `2`
  Focus ranges: 1-260
  Enclosing symbols: no symbols inferred
  Note: observed in failed-AC trace
  Mtime: `2026-03-26 11:36:17 KST`
  Evidence lines:
    - `AC 8 → Bash: /bin/zsh -lc "sed -n '1,260p' run.sh"`
    - `AC 8 → Bash: /bin/zsh -lc 'head -n 120 run.sh'`
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
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/boj28350_resume.py`
  Observed mentions: `1`
  Focus ranges: 1-220
  Enclosing symbols: function parse_cases [44-46], function resolve_branch_path [49-53], function resolve_branch_solver [56-60], function sanitize_artifact_token [63-66]
  Note: observed in failed-AC trace
  Mtime: `2026-04-10 04:54:19 KST`
  Evidence lines:
    - `AC 8 → Bash: /bin/zsh -lc "sed -n '260,340p' test_boj28350_resume.py"`
    - `AC 8 → Bash: /bin/zsh -lc "sed -n '1,220p' boj28350_resume.py"`
    - `AC 8 → Bash: /bin/zsh -lc "sed -n '1,120p' test_boj28350_resume.py"`
    - `AC 8 → Bash: /bin/zsh -lc "sed -n '220,420p' test_boj28350_resume.py"`
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
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/.ouroboros/run_next_probe.py`
  Observed mentions: `1`
  Focus ranges: 1-260
  Enclosing symbols: function parse_args [28-35], function normalize_output [38-43], function _load_artifact_guard [46-52], function _resolve_branch_path [55-56]
  Note: observed in failed-AC trace
  Mtime: `2026-04-10 19:40:58 KST`
  Evidence lines:
    - `AC 8 → Bash: /bin/zsh -lc "sed -n '1,260p' .ouroboros/run_next_probe.py"`
    - `AC 8 → Bash: /bin/zsh -lc "find . -name 'run_next_probe.py' -o -name '...`
  Code excerpt:
```text
    1: #!/usr/bin/env python3
    2: from __future__ import annotations
    3: 
    4: import argparse
    5: import json
    6: import os
    7: import subprocess
    8: import sys
    9: import time
   10: from pathlib import Path
   11: 
   12: os.environ.setdefault("PYTHONDONTWRITEBYTECODE", "1")
   13: sys.dont_write_bytecode = True
   14: 
   15: SCRIPT_DIR = Path(__file__).resolve().parent
   16: if str(SCRIPT_DIR) not in sys.path:
   17:     sys.path.insert(0, str(SCRIPT_DIR))
   18: 
   19: from retry_artifact_io import (
   20:     prepare_output_dir,
   21:     reset_output_dir,
   22:     resolve_artifact_output_path,
   23:     resolve_branch_path as resolve_retry_branch_path,
   24:     write_text_output,
   25: )
   26: 
   27: 
   28: def parse_args() -> argparse.Namespace:
```
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/branch_run_case.py`
  Observed mentions: `3`
  Focus ranges: none captured
  Enclosing symbols: function parse_env_assignments [76-83], function apply_solver_env_overrides [86-90], function build_case_solver_env [93-109], function resolve_case_outdir [112-113]
  Note: observed in failed-AC trace, mapped from failed AC semantics, boosted by failure_analysis_state, matched pinned symbol `function parse_env_assignments [76-83]`
  Mtime: `2026-04-03 08:13:24 KST`
  Code excerpt:
```text
   74: 
   75: 
   76: def parse_env_assignments(items: list[str]) -> dict[str, str]:
   77:     env: dict[str, str] = {}
   78:     for item in items:
   79:         key, sep, value = item.partition("=")
   80:         if not sep or not key:
   81:             raise ValueError(f"invalid --env assignment: {item!r}")
   82:         env[key] = value
   83:     return env
   84: 
   85: 
```

#### Focused Artifact Snapshots

- `run_case` latest: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/run_case/diagnostics/case_probe_multi_comb_rect_no_lreuse/run_case_result.json`
  mtime: `2026-04-12 18:23:30 KST`
  attempt start: `2026-04-12 19:42:09 KST`
  fresh within attempt: `no`
  freshness note: latest summary/file predates the failed attempt start; treat it as carried-forward evidence, not fresh gate output
- `gen_case_aux` latest: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/gen_case_aux/ac8_audit/gen/hidden_parent.txt`
  mtime: `2026-03-25 18:48:35 KST`
  attempt start: `2026-04-12 19:42:09 KST`
  fresh within attempt: `no`
  freshness note: latest summary/file predates the failed attempt start; treat it as carried-forward evidence, not fresh gate output

#### Failed-AC Trace Excerpt

```text
  AC 8: Analyzing complexity...
  AC 8 → Bash: /bin/zsh -lc "sed -n '1,260p' artifact_paths.py"
  AC 8 → Bash: /bin/zsh -lc "sed -n '5790,6015p' lca_smoke.sh"
  AC 8 → Bash: /bin/zsh -lc "sed -n '1,260p' test_entrypoint_artifact_lo...
  AC 8 → Bash: /bin/zsh -lc "sed -n '1650,1765p' test_retry_loop_artifac...
  AC 8 → Bash: /bin/zsh -lc "sed -n '1765,1815p' test_retry_loop_artifac...
  AC 8 → Bash: /bin/zsh -lc "sed -n '1,260p' .ouroboros/run_next_probe.py"
  AC 8 → Bash: /bin/zsh -lc "find . -name 'run_next_probe.py' -o -name '...
  AC 8 → Bash: /bin/zsh -lc 'python3 -m pytest -q test_entrypoint_artifa...
  AC 8 → Bash: /bin/zsh -lc "python3 -m pytest -q test_branch_run_case.p...
  AC 8 → Bash: /bin/zsh -lc "sed -n '240,320p' test_lca_smoke_wrapper.py"
  AC 8 → Bash: /bin/zsh -lc "sed -n '2390,2465p' test_lca_smoke_wrapper.py"
  AC 8 → Bash: /bin/zsh -lc "sed -n '1,260p' .ouroboros/retry_artifact_i...
  AC 8 → Bash: /bin/zsh -lc 'python3 -m unittest -q test_retry_loop_arti...
  AC 8 → Bash: /bin/zsh -lc 'python3 -m unittest -q test_boj28350_resume'
    Sub-AC 3: Stall detected (attempt 1/3), retrying...
  AC 8 → Bash: /bin/zsh -lc "sed -n '260,340p' test_boj28350_resume.py"
  AC 8 → Bash: /bin/zsh -lc "sed -n '1,220p' boj28350_resume.py"
  AC 8 → Bash: /bin/zsh -lc "sed -n '1,120p' test_boj28350_resume.py"
  AC 8 → Bash: /bin/zsh -lc "find boj28350_resume -maxdepth 3 -type f | ...
  AC 8 → Bash: /bin/zsh -lc "sed -n '1,260p' outer_suite_wrappers/lca_hu...
  AC 8 → Bash: /bin/zsh -lc "sed -n '1,260p' run.sh"
  AC 8 → Bash: /bin/zsh -lc 'head -n 120 run.sh'
  AC 8 → Bash: /bin/zsh -lc 'head -n 140 outer_suite_wrappers/lca_hunt.sh'
  AC 8 → Bash: /bin/zsh -lc 'python3 -m unittest -q test_entrypoint_arti...
  AC 8 → Bash: /bin/zsh -lc 'python3 -m unittest -q test_entrypoint_arti...
  AC 8 → Bash: /bin/zsh -lc 'python3 -m unittest -q test_lca_smoke_wrapp...
  AC 8 → Bash: /bin/zsh -lc 'ls -R artifacts/lca_tree_stress_v5/strong_g...
  AC 8 → Bash: /bin/zsh -lc "sed -n '220,420p' test_boj28350_resume.py"
  AC 8 → Bash: /bin/zsh -lc "find artifacts/lca_tree_stress_v5 -maxdepth...
  AC 8 → Bash: /bin/zsh -lc "find artifacts -type f | sort | rg 'ac3_cor...
  AC 8 → Bash: /bin/zsh -lc 'ls artifacts/lca_tree_stress_v5/retry_loop/...
  AC 8 → Bash: /bin/zsh -lc "sed -n '1,40p' artifacts/lca_tree_stress_v5...
  AC 8 → Bash: /bin/zsh -lc "python3 - <<'PY'
from pathlib import Path
i...
  AC 8 → Bash: /bin/zsh -lc 'find artifacts/lca_tree_stress_v5/retry_loo...
  AC 8 → Bash: /bin/zsh -lc 'find artifacts/lca_tree_stress_v5/retry_loo...
  AC 8 → Bash: /bin/zsh -lc "python3 - <<'PY'
from pathlib import Path
f...
  AC 8 → Bash: /bin/zsh -lc "python3 - <<'PY'
from pathlib import Path
i...
  AC 8 → Bash: /bin/zsh -lc "python3 - <<'PY'
from pathlib import Path
i...
  AC 8 → Edit: /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian...
  AC 8 → Bash: /bin/zsh -lc "rg --files | rg '"'^(artifact_paths'"\\.py|...
  AC 8 → Bash: /bin/zsh -lc "rg --files | rg 'run_next_probe\\.py"'$|fai...
  AC 8 → Bash: /bin/zsh -lc "python3 - <<'PY'
from pathlib import Path
i...
  AC 8 → Bash: /bin/zsh -lc 'python3 -m unittest -q test_boj28350_resume'
  AC 8 → Bash: /bin/zsh -lc 'python3 -m unittest -q test_entrypoint_arti...
  AC 8 → Bash: /bin/zsh -lc 'rg -n "timeout_regression|corridor|latest_f...
  AC 8 → Bash: /bin/zsh -lc "pwd && rg -n \"artifacts|/tmp|mktemp|tempfi...
    Sub-AC 3: Stall detected (attempt 2/3), retrying...
  AC 8 → Bash: /bin/zsh -lc "find artifacts/lca_tree_stress_v5/strong_ga...
│ ### AC 8: [FAIL] All generated outputs remain inside branch_3/artifacts/...  │
│ Error: Stalled (no activity for 300s)                                        │
╰──────────────────────────────────────────────────────────────────────────────╯
```

## Refinement Versus Previous Failure

- Repeated failed AC(s) versus the previous attempt: 1, 2, 3, 8.
- AC 1 repeated the same failure classification `stall/no-activity`.
- AC 2 repeated the same failure classification `stall/no-activity`.
- AC 3 repeated the same failure classification `stall/no-activity`.
- AC 8 repeated the same failure classification `stall/no-activity`.
- Recurring code-structure hotspots: /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/.ouroboros/run_next_probe.py, /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifact_paths.py, /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp, /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/boj28350_resume.py, /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/branch_certify_suite.py, /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/branch_run_case.py.
- Recurring enclosing symbols: class Boj28350ResumeArtifactLocalityTests [14-235], class Boj28350ResumeRegressionManifestTests [238-339], class BuildWrapperEnvironmentTests [30-181], class CompilerCandidateTests [14-27], class LcaSmokeWrapperRegressionTests [24-2933], class SnapshotRetryInputsTests [18-336].
- Recurring line-range hotspots: /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/.ouroboros/run_next_probe.py:1-260, /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifact_paths.py:1-260, /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp:9360-9364, /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp:9367-9370, /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp:9394-9396, /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp:9401-9402.

## Next-Retry Analysis Rule

- Before the next session edits code, read this breakdown, start from the repeated failed AC if one exists, and inspect the listed phase and code-structure hotspots before running the heavy gate again.
- If this breakdown still localizes the failure only at a broad file level, improve the retry analysis logic itself before the next heavy run so the next capture records narrower symbols, ranges, wrapper sections, and code excerpts.
- When `failure_analysis_state.json` carries retry-specific line ranges or non-code transport/guard anchors, surface them first as dedicated `Retry-Critical Anchors` or `Filtered Failure Locus` sections before generic hotspots so the next solver session starts from exact slices.