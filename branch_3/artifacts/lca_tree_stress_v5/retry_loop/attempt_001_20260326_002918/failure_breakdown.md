# Failure Breakdown: Attempt 1

- Timestamp: `2026-03-26 05:46:51 KST`
- Session ID: `orch_1736140fd0df`
- Execution ID: `exec_19e8677de11e`
- Analysis state file: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/.ouroboros/failure_analysis_state.json`
- Analysis state revision: `5`

## Failure Decomposition

### AC 4: Formal closure requires running ./lca_strong_gate.sh twice  │

- Failure type: `stall/no-activity`
- Failure family: `strong_gate_unspecified`
- Interpretation lane: `correctness-proof`
- Primary progress40 axis: `zero_span_fastpath`
- Secondary progress40 axis: `state_materialization`
- Profile mode observed: `PROFILE_SAMPLED`
- Last progress checkpoint phase: `unknown`
- Last release diag phase: `unknown`
- Suggested next probe: `LCA_STAGE_FILTER=correctness_fuzz ./lca_strong_gate.sh`
- Trace lines captured: `80`

#### Progress40 Axis Evidence

- No direct axis evidence was extracted from the trace; fallback axis came from the current progress40 summary.

- Enabled flags seen in trace: `ENABLE_AC`, `ENABLE_AC3_INIT_DIAG`, `ENABLE_AC3_PHASE_DIAG`, `ENABLE_COMPACT_FIELD_PACK_OPT`, `ENABLE_COMPACT_RELEASE_DIAG`, `ENABLE_DE`, `ENABLE_DELTA`, `ENABLE_DELTA_PRESERVED_HIT`, `ENABLE_LA`, `ENABLE_LAYOUT_SIGNATURE_G`, `ENABLE_LAYOUT_SIGNATURE_GATE_OPT`, `ENABLE_PACK_ENCODE_NORMALIZE_OPT`, `ENABLE_POINTER_REBIND_OPT`, `ENABLE_PREV_STATE_WRITEBACK_OPT`, `ENABLE_STATE_LOAD_MATERIALIZATION_OPT`
- Current summary pivot baseline: `zero-span eligibility and fastpath commit`
- Current summary residual axes: `state_materialization`, `layout_gate`, `zero_span_fastpath`

#### Phase Breakdown

- `timing-timeout` x6 | sample: `solver_release_env.sh"`
- `artifact-paths` x4 | sample: `Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "sed -n '1,260p' artifact_paths.py"`
- `wrapper` x2 | sample: `│ ### AC 4: [FAIL] Formal closure requires running ./lca_strong_gate.sh twice  │`
- `case-runner` x1 | sample: `Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "sed -n '1,260p' branch_run_case.py"`

#### Latest Next-Probe Signal

- Command: `LCA_STAGE_FILTER=correctness_fuzz ./lca_strong_gate.sh`
- Exit code: `124`
- Elapsed seconds: `1.003`
- Quick-fail lock signal: `no`
- Stderr log: `artifacts/lca_tree_stress_v5/retry_loop/attempt_001_20260325_183441/next_probe.stderr.log`

#### Code-Structure Hotspots

- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/lca_smoke_target.sh`
  Observed mentions: `7`
  Focus ranges: none captured
  Enclosing symbols: no symbols inferred
  Note: observed in failed-AC trace
  Mtime: `2026-03-26 01:01:46 KST`
  Evidence lines:
    - `Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc 'bash -n lca_smoke_target.sh'`
    - `Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc 'chmod +x lca_smoke_target.sh`
    - `Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc './lca_smoke_target.sh --list'`
    - `Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc './lca_smoke_target.sh 1`
    - `Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "nl -ba lca_smoke_target.sh | sed -n`
  Code excerpt:
```text
    1: #!/usr/bin/env bash
    2: set -euo pipefail
    3: 
    4: SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
    5: exec "$SCRIPT_DIR/outer_suite_wrappers/lca_smoke_target.sh" "$@"
```
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifact_paths.py`
  Observed mentions: `3`
  Focus ranges: 1-180, 1-220, 1-240, 1-260, 1-320
  Enclosing symbols: function artifacts_root [34-35], function ensure_under_artifacts [38-39], function branch_tmp_root [42-43], function configure_branch_process_env [46-71]
  Note: observed in failed-AC trace
  Mtime: `2026-03-26 01:01:46 KST`
  Evidence lines:
    - `Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "sed -n '1,260p' artifact_paths.py"`
    - `Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc 'python3 artifact_paths.py`
    - `Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "nl -ba artifact_paths.py | sed -n`
  Code excerpt:
```text
    1: #!/usr/bin/env python3
    2: from __future__ import annotations
    3: 
    4: import argparse
    5: import os
    6: import shutil
    7: import sys
    8: from pathlib import Path
    9: 
   10: 
   11: BRANCH_ROOT = Path(__file__).resolve().parent
   12: DEFAULT_ARTIFACTS_ROOT = (BRANCH_ROOT / "artifacts").resolve()
   13: ARTIFACTS_ROOT = DEFAULT_ARTIFACTS_ROOT
   14: BRANCH_TMP_SUBPATH = ("lca_tree_stress_v5", ".tmp")
   15: 
   16: DEFAULT_OUTPUT_SUBPATHS: dict[str, tuple[str, ...]] = {
   17:     "boj28350_build": ("boj28350_resume", "build"),
   18:     "boj28350_direct_solver_aux": ("boj28350_resume", "direct_solver_aux"),
   19:     "boj28350_smoke": ("boj28350_resume", "smoke"),
   20:     "branch_certify_suite": ("lca_tree_stress_v5", "certify_suite"),
   21:     "branch_gen_case_aux": ("lca_tree_stress_v5", "gen_case_aux"),
   22:     "branch_run_case": ("lca_tree_stress_v5", "run_case"),
   23:     "lca_smoke": ("lca_tree_stress_v5", "smoke"),
   24:     "lca_smoke_target": ("lca_tree_stress_v5", "smoke_target"),
   25:     "lca_smoke_repeatability": ("lca_tree_stress_v5", "smoke_repeatability"),
   26:     "lca_strong_gate": ("lca_tree_stress_v5", "strong_gate"),
   27:     "lca_rebuttal_gate": ("lca_tree_stress_v5", "rebuttal_gate"),
   28:     "lca_boj3s_gate": ("lca_tree_stress_v5", "boj3s_gate"),
```
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/solver_release_env.sh`
  Observed mentions: `3`
  Focus ranges: none captured
  Enclosing symbols: function __solver_release_env_keep_or_set [41-112]
  Note: observed in failed-AC trace, boosted by failure_analysis_state
  Mtime: `2026-03-26 05:29:12 KST`
  Evidence lines:
    - `solver_release_env.sh"`
    - `AC 4 → Bash: /bin/zsh -lc "sed -n '1,180p' solver_release_env.sh`
  Code excerpt:
```text
   39: export TEMP="$BRANCH_ARTIFACT_TMP_ROOT"
   40: __solver_release_env_artifacts_root="$__solver_release_env_dir/artifacts"
   41: __solver_release_env_keep_or_set() {
   42:   local var_name="$1"
   43:   local desired="$2"
   44:   local current=""
   45:   eval "current=\${$var_name-}"
   46:   case "$current" in
   47:     "$__solver_release_env_artifacts_root"|"$__solver_release_env_artifacts_root"/*)
   48:       ;;
   49:     *)
   50:       current="$desired"
   51:       ;;
   52:   esac
   53:   export "$var_name=$current"
   54: }
   55: __solver_release_env_keep_or_set HOME "$BRANCH_ARTIFACT_TMP_ROOT/home"
   56: __solver_release_env_keep_or_set XDG_CONFIG_HOME "$BRANCH_ARTIFACT_TMP_ROOT/xdg_config"
   57: __solver_release_env_keep_or_set XDG_CACHE_HOME "$BRANCH_ARTIFACT_TMP_ROOT/xdg_cache"
   58: __solver_release_env_keep_or_set XDG_STATE_HOME "$BRANCH_ARTIFACT_TMP_ROOT/xdg_state"
   59: __solver_release_env_keep_or_set PYTHONPYCACHEPREFIX "$BRANCH_ARTIFACT_TMP_ROOT/pycache"
   60: mkdir -p "$HOME" "$XDG_CONFIG_HOME" "$XDG_CACHE_HOME" "$XDG_STATE_HOME" "$PYTHONPYCACHEPREFIX"
   61: unset -f __solver_release_env_keep_or_set
   62: unset __solver_release_env_artifacts_root
   63: unset __solver_release_env_dir
   64: unset __solver_release_env_tmp_override
   65: unset __solver_release_env_tmp
   66: 
```
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/build.sh`
  Observed mentions: `3`
  Focus ranges: none captured
  Enclosing symbols: no symbols inferred
  Note: observed in failed-AC trace, boosted by failure_analysis_state
  Mtime: `2026-03-25 05:11:08 KST`
  Evidence lines:
    - `AC 4 → Bash: /bin/zsh -lc "sed -n '1,220p' build.sh`
  Code excerpt:
```text
    1: #!/usr/bin/env bash
    2: set -euo pipefail
    3: cd "$(dirname "$0")"
    4: export PYTHONDONTWRITEBYTECODE=1
    5: source ./solver_release_env.sh
    6: python3 boj28350_resume.py build "$@"
    7: chmod +x ./boj28350_resume/solve
```
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/branch_validator.py`
  Observed mentions: `3`
  Focus ranges: none captured
  Enclosing symbols: function _read_tokens [9-18], function _parse_input [21-39], function _parse_output [42-49], function _build_tree [52-91]
  Note: observed in failed-AC trace, boosted by failure_analysis_state, matched pinned symbol `function _read_tokens [9-18]`
  Mtime: `2026-03-25 05:40:13 KST`
  Code excerpt:
```text
    7: 
    8: 
    9: def _read_tokens(path: Path) -> list[int]:
   10:     try:
   11:         data = path.read_text(encoding="utf-8")
   12:     except OSError as exc:
   13:         raise ValueError(f"failed to read {path}: {exc}") from exc
   14:     tokens = data.split()
   15:     try:
   16:         return [int(token) for token in tokens]
   17:     except ValueError as exc:
   18:         raise ValueError(f"non-integer token in {path}") from exc
   19: 
   20: 
```
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/branch_certify_suite.py`
  Observed mentions: `3`
  Focus ranges: none captured
  Enclosing symbols: function _ensure_under_lca_tree_artifacts [49-57], function _normalize_artifact_out_dir [65-66], function _normalize_cli_out_arg [69-82], function _generation_ready [85-87]
  Note: observed in failed-AC trace, boosted by failure_analysis_state
  Mtime: `2026-03-26 00:38:10 KST`
  Code excerpt:
```text
   47: 
   48: 
   49: def _ensure_under_lca_tree_artifacts(path: str | Path) -> Path:
   50:     resolved = ensure_under_artifacts(path)
   51:     try:
   52:         resolved.relative_to(LCA_TREE_STRESS_ARTIFACTS_ROOT)
   53:     except ValueError as exc:
   54:         raise ValueError(
   55:             f"certify helper path must stay under {LCA_TREE_STRESS_ARTIFACTS_ROOT}: {resolved}"
   56:         ) from exc
   57:     return resolved
   58: 
   59: 
```
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/suite_utils.py`
  Observed mentions: `3`
  Focus ranges: 1-220, 1-240, 1-260, 1-380, 100-340, 240-360
  Enclosing symbols: function parse_int_list_csv [19-23], function parse_str_list_csv [26-30], function default_solver_name [33-34], function default_solver_path [37-38], function _kill_process [92-104], function run_cmd [107-138]
  Note: observed in failed-AC trace, mapped from failed AC semantics
  Mtime: `2026-03-24 23:13:12 KST`
  Evidence lines:
    - `Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "sed -n '1,220p' suite_utils.py"`
    - `AC 4 → Bash: /bin/zsh -lc "sed -n '1,260p' suite_utils.py"`
    - `AC 4 → Bash: /bin/zsh -lc "sed -n '260,340p' suite_utils.py"`
  Code excerpt:
```text
    1: #!/usr/bin/env python3
    2: from __future__ import annotations
    3: 
    4: import math
    5: import os
    6: import signal
    7: import statistics
    8: import subprocess
    9: import sys
   10: import time
   11: from pathlib import Path
   12: from typing import List, Mapping, Optional, Sequence, Tuple
   13: 
   14: 
   15: IS_WINDOWS = os.name == "nt"
   16: IS_DARWIN = sys.platform == "darwin"
   17: 
   18: 
   19: def parse_int_list_csv(s: str) -> List[int]:
   20:     s = s.strip()
   21:     if not s:
   22:         return []
   23:     return [int(x) for x in s.split(",") if x.strip()]
   24: 
   25: 
   26: def parse_str_list_csv(s: str) -> List[str]:
   27:     s = s.strip()
   28:     if not s:
```
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/lca_strong_gate.sh`
  Observed mentions: `3`
  Focus ranges: 1-220, 1-260
  Enclosing symbols: no symbols inferred
  Note: observed in failed-AC trace, mapped from failed AC semantics
  Mtime: `2026-03-25 11:32:56 KST`
  Evidence lines:
    - `AC 4 → Bash: /bin/zsh -lc "sed -n '1,260p' lca_strong_gate.sh`
    - `│ ### AC 4: [FAIL] Formal closure requires running ./lca_strong_gate.sh twice  │`
    - `latest_next_probe_result.md → Command: LCA_STAGE_FILTER=correctness_fuzz ./lca_strong_gate.sh`
  Code excerpt:
```text
    1: #!/usr/bin/env bash
    2: set -euo pipefail
    3: 
    4: SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
    5: exec "$SCRIPT_DIR/outer_suite_wrappers/lca_strong_gate.sh" "$@"
```

#### Focused Artifact Snapshots

- `strong_gate` latest: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/certify.json`
  mtime: `2026-03-25 00:32:23 KST`
  summary: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/certify.json`
- `tmp` latest: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/.tmp/sfcert3.tid2de26/certify.json`
  mtime: `2026-03-26 05:27:00 KST`
  summary: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/.tmp/sfcert3.tid2de26/certify.json`

#### Failed-AC Trace Excerpt

```text
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "sed -n '1,220p' 
outer_suite_wrappers/lca_sm...
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc './lca_smoke_target.sh --list'
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "sed -n '220,420p' 
outer_suite_wrappers/lca_...
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc './lca_smoke_target.sh 1 
selfcheck_case1'
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "sed -n '1,80p' 
artifacts/lca_tree_stress_v5...
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "sed -n '1,80p' 
artifacts/lca_tree_stress_v5...
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc 'rg --files 
artifacts/lca_tree_stress_v5/smo...
    Sub-AC 4 of AC 2 → Edit: /Users/free_1/Library/Mobile 
Documents/iCloud~md~obsidian...
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc 'bash -n 
outer_suite_wrappers/lca_smoke_targ...
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc './lca_smoke_target.sh --list'
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc './lca_smoke_target.sh 1 
selfcheck_case1b'
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "nl -ba artifact_paths.py | sed -n 
'1,60p'"
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "nl -ba lca_smoke_target.sh | sed -n 
'1,40p'"
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "nl -ba 
outer_suite_wrappers/lca_smoke_targe...
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "nl -ba outer_suite_wrappers/README.md
| sed...
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "nl -ba README.md | sed -n '35,95p'"
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "nl -ba 
outer_suite_wrappers/lca_smoke_targe...
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc "nl -ba 
outer_suite_wrappers/lca_smoke_targe...
    Sub-AC 4 of AC 2 → Edit: /Users/free_1/Library/Mobile 
Documents/iCloud~md~obsidian...
    Sub-AC 4 of AC 2 → Bash: /bin/zsh -lc 'bash -n 
outer_suite_wrappers/lca_smoke_targ...
    Sub-ACs completed: 4/4 succeeded
  AC 4: Analyzing complexity...
  AC 5: Analyzing complexity...
  AC 4 → Bash: /bin/zsh -lc "pwd && rg --files -g 'AGENTS.md' -g 'boj283...
  AC 4 → Bash: /bin/zsh -lc 'wc -l boj28350_resume/README.md boj28350_re...
  AC 4 → Bash: /bin/zsh -lc "sed -n '1,220p' boj28350_resume/README.md
p...
  AC 4 → Bash: /bin/zsh -lc "sed -n '1,260p' artifacts/lca_tree_stress_v...
  AC 4 → Bash: /bin/zsh -lc "sed -n '1,260p' lca_strong_gate.sh
printf '...
  AC 4 → Bash: /bin/zsh -lc "sed -n '1,220p' build.sh
printf '\\n===== b...
  AC 4 → Bash: /bin/zsh -lc "sed -n '1,180p' solver_release_env.sh
print...
  AC 4 → Bash: /bin/zsh -lc "ls -ld artifacts/lca_tree_stress_v5/.locks/...
  AC 4 → Bash: /bin/zsh -lc "find artifacts/lca_tree_stress_v5/.tmp -max...
  AC 4 → Bash: /bin/zsh -lc "printf 'strong pid: '; cat artifacts/lca_tr...
  AC 4 → Bash: /bin/zsh -lc "stat -f '%N %Sm %z' -t '%Y-%m-%d %H:%M:%S %...
  AC 4 → Bash: /bin/zsh -lc 'for pid in $(cat artifacts/lca_tree_stress_...
  AC 4 → Bash: /bin/zsh -lc "find artifacts/lca_tree_stress_v5/.tmp/lca_...
  AC 4 → Bash: /bin/zsh -lc "sed -n '260,420p' outer_suite_wrappers/lca_...
  AC 4 → Bash: /bin/zsh -lc "rg -n \"build\\.sh|BUILD_WRAPPER|run_certif...
  AC 4 → Bash: /bin/zsh -lc "sed -n '1,260p' suite_presets/strong_gate.j...
  AC 4 → Bash: /bin/zsh -lc "date '+%Y-%m-%d %H:%M:%S %Z'
ls -ld artifac...
  AC 4 → Bash: /bin/zsh -lc 'WORKDIR=$(find artifacts/lca_tree_stress_v5...
  AC 4 → Bash: /bin/zsh -lc "find artifacts/lca_tree_stress_v5/.tmp/case...
  AC 4 → Bash: /bin/zsh -lc "find artifacts/lca_tree_stress_v5/.tmp/case...
  AC 4 → Bash: /bin/zsh -lc "python3 - <<'PY'
from pathlib import Path
r...
  AC 4 → Bash: /bin/zsh -lc "python3 - <<'PY'
from pathlib import Path
r...
  AC 4 → Bash: /bin/zsh -lc "python3 - <<'PY'
from pathlib import Path
i...
  AC 4 → Bash: /bin/zsh -lc "sed -n '1,260p' suite_utils.py"
  AC 4 → Bash: /bin/zsh -lc 'rg -n "run_solver_with_time|timeout|time.tx...
  AC 4 → Bash: /bin/zsh -lc "sed -n '260,340p' suite_utils.py"
│ ### AC 4: [FAIL] Formal closure requires running ./lca_strong_gate.sh twice  │
│ in a row on the same working tree with both runs PASS                        │
│ Error: Stalled (no activity for 300s)                                        │
```

### AC 6: Formal closure also requires running ./lca_boj3s_gate.sh    │

- Failure type: `stall/no-activity`
- Failure family: `boj3s_gate_unspecified`
- Interpretation lane: `performance-profile`
- Primary progress40 axis: `zero_span_fastpath`
- Secondary progress40 axis: `state_materialization`
- Profile mode observed: `PROFILE_SAMPLED`
- Last progress checkpoint phase: `unknown`
- Last release diag phase: `unknown`
- Suggested next probe: `PROFILE_MODE=PROFILE_SAMPLED ./lca_boj3s_gate.sh`
- Trace lines captured: `43`

#### Progress40 Axis Evidence

- No direct axis evidence was extracted from the trace; fallback axis came from the current progress40 summary.

- Enabled flags seen in trace: `ENABLE_AC`, `ENABLE_AC3_INIT_DIAG`, `ENABLE_AC3_PHASE_DIAG`, `ENABLE_COMPACT_FIELD_PACK_OPT`, `ENABLE_COMPACT_RELEASE_DIAG`, `ENABLE_DE`, `ENABLE_DELTA`, `ENABLE_DELTA_PRESERVED_HIT`, `ENABLE_LA`, `ENABLE_LAYOUT_SIGNATURE_G`, `ENABLE_LAYOUT_SIGNATURE_GATE_OPT`, `ENABLE_PACK_ENCODE_NORMALIZE_OPT`, `ENABLE_POINTER_REBIND_OPT`, `ENABLE_PREV_STATE_WRITEBACK_OPT`, `ENABLE_STATE_LOAD_MATERIALIZATION_OPT`
- Current summary pivot baseline: `zero-span eligibility and fastpath commit`
- Current summary residual axes: `state_materialization`, `layout_gate`, `zero_span_fastpath`

#### Phase Breakdown

- `timing-timeout` x1 | sample: `AC 6 → Bash: /bin/zsh -lc "sed -n '1,260p' solver_release_env.sh"`
- `certify` x1 | sample: `AC 6 → Bash: /bin/zsh -lc "pgrep -af 'branch_certify_suite.py|solver_s...`
- `wrapper` x1 | sample: `│ ### AC 6: [FAIL] Formal closure also requires running ./lca_boj3s_gate.sh    │`

#### Latest Next-Probe Signal

- No latest probe signal was available for this AC.

#### Code-Structure Hotspots

- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/solver_release_env.sh`
  Observed mentions: `3`
  Focus ranges: none captured
  Enclosing symbols: function __solver_release_env_keep_or_set [41-112]
  Note: observed in failed-AC trace, boosted by failure_analysis_state
  Mtime: `2026-03-26 05:29:12 KST`
  Evidence lines:
    - `AC 6 → Bash: /bin/zsh -lc "sed -n '1,260p' solver_release_env.sh"`
  Code excerpt:
```text
   39: export TEMP="$BRANCH_ARTIFACT_TMP_ROOT"
   40: __solver_release_env_artifacts_root="$__solver_release_env_dir/artifacts"
   41: __solver_release_env_keep_or_set() {
   42:   local var_name="$1"
   43:   local desired="$2"
   44:   local current=""
   45:   eval "current=\${$var_name-}"
   46:   case "$current" in
   47:     "$__solver_release_env_artifacts_root"|"$__solver_release_env_artifacts_root"/*)
   48:       ;;
   49:     *)
   50:       current="$desired"
   51:       ;;
   52:   esac
   53:   export "$var_name=$current"
   54: }
   55: __solver_release_env_keep_or_set HOME "$BRANCH_ARTIFACT_TMP_ROOT/home"
   56: __solver_release_env_keep_or_set XDG_CONFIG_HOME "$BRANCH_ARTIFACT_TMP_ROOT/xdg_config"
   57: __solver_release_env_keep_or_set XDG_CACHE_HOME "$BRANCH_ARTIFACT_TMP_ROOT/xdg_cache"
   58: __solver_release_env_keep_or_set XDG_STATE_HOME "$BRANCH_ARTIFACT_TMP_ROOT/xdg_state"
   59: __solver_release_env_keep_or_set PYTHONPYCACHEPREFIX "$BRANCH_ARTIFACT_TMP_ROOT/pycache"
   60: mkdir -p "$HOME" "$XDG_CONFIG_HOME" "$XDG_CACHE_HOME" "$XDG_STATE_HOME" "$PYTHONPYCACHEPREFIX"
   61: unset -f __solver_release_env_keep_or_set
   62: unset __solver_release_env_artifacts_root
   63: unset __solver_release_env_dir
   64: unset __solver_release_env_tmp_override
   65: unset __solver_release_env_tmp
   66: 
```
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/build.sh`
  Observed mentions: `3`
  Focus ranges: none captured
  Enclosing symbols: no symbols inferred
  Note: observed in failed-AC trace, boosted by failure_analysis_state
  Mtime: `2026-03-25 05:11:08 KST`
  Evidence lines:
    - `AC 6 → Bash: /bin/zsh -lc "sed -n '1,220p' build.sh"`
  Code excerpt:
```text
    1: #!/usr/bin/env bash
    2: set -euo pipefail
    3: cd "$(dirname "$0")"
    4: export PYTHONDONTWRITEBYTECODE=1
    5: source ./solver_release_env.sh
    6: python3 boj28350_resume.py build "$@"
    7: chmod +x ./boj28350_resume/solve
```
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/branch_validator.py`
  Observed mentions: `3`
  Focus ranges: none captured
  Enclosing symbols: function _read_tokens [9-18], function _parse_input [21-39], function _parse_output [42-49], function _build_tree [52-91]
  Note: observed in failed-AC trace, boosted by failure_analysis_state, matched pinned symbol `function _read_tokens [9-18]`
  Mtime: `2026-03-25 05:40:13 KST`
  Code excerpt:
```text
    7: 
    8: 
    9: def _read_tokens(path: Path) -> list[int]:
   10:     try:
   11:         data = path.read_text(encoding="utf-8")
   12:     except OSError as exc:
   13:         raise ValueError(f"failed to read {path}: {exc}") from exc
   14:     tokens = data.split()
   15:     try:
   16:         return [int(token) for token in tokens]
   17:     except ValueError as exc:
   18:         raise ValueError(f"non-integer token in {path}") from exc
   19: 
   20: 
```
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/branch_certify_suite.py`
  Observed mentions: `3`
  Focus ranges: none captured
  Enclosing symbols: function _ensure_under_lca_tree_artifacts [49-57], function _normalize_artifact_out_dir [65-66], function _normalize_cli_out_arg [69-82], function _generation_ready [85-87]
  Note: observed in failed-AC trace, boosted by failure_analysis_state
  Mtime: `2026-03-26 00:38:10 KST`
  Evidence lines:
    - `AC 6 → Bash: /bin/zsh -lc "pgrep -af 'branch_certify_suite.py|solver_s...`
  Code excerpt:
```text
   47: 
   48: 
   49: def _ensure_under_lca_tree_artifacts(path: str | Path) -> Path:
   50:     resolved = ensure_under_artifacts(path)
   51:     try:
   52:         resolved.relative_to(LCA_TREE_STRESS_ARTIFACTS_ROOT)
   53:     except ValueError as exc:
   54:         raise ValueError(
   55:             f"certify helper path must stay under {LCA_TREE_STRESS_ARTIFACTS_ROOT}: {resolved}"
   56:         ) from exc
   57:     return resolved
   58: 
   59: 
```
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/lca_boj3s_gate.sh`
  Observed mentions: `2`
  Focus ranges: 1-260
  Enclosing symbols: no symbols inferred
  Note: observed in failed-AC trace, mapped from failed AC semantics
  Mtime: `2026-03-26 04:05:51 KST`
  Evidence lines:
    - `AC 6 → Bash: /bin/zsh -lc "sed -n '1,260p' lca_boj3s_gate.sh"`
    - `│ ### AC 6: [FAIL] Formal closure also requires running ./lca_boj3s_gate.sh    │`
  Code excerpt:
```text
    1: #!/usr/bin/env bash
    2: set -euo pipefail
    3: 
    4: SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
    5: exec "$SCRIPT_DIR/outer_suite_wrappers/lca_boj3s_gate.sh" "$@"
```
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/solver_release_env.sh`
  Observed mentions: `1`
  Focus ranges: 1-160, 1-180, 1-220, 1-260, 220-520
  Enclosing symbols: function __solver_release_env_keep_or_set [41-112]
  Note: observed in failed-AC trace, boosted by failure_analysis_state
  Mtime: `2026-03-26 05:29:12 KST`
  Evidence lines:
    - `AC 6 → Bash: /bin/zsh -lc "sed -n '1,260p' solver_release_env.sh"`
  Code excerpt:
```text
    1: #!/usr/bin/env bash
    2: 
    3: # Branch-local release subset mirrored from the archived release runner.
    4: # This enables the early performance pivots without forcing the later
    5: # experimental pointer/patch/normalize branches that regressed the gate probe.
    6: 
    7: # Keep transient tool output under branch-local artifacts instead of system temp.
    8: __solver_release_env_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    9: __solver_release_env_tmp_override="${BRANCH_ARTIFACT_TMP_ROOT:-}"
   10: if ! __solver_release_env_tmp="$(
   11:   python3 - "$__solver_release_env_dir" "$__solver_release_env_tmp_override" <<'PY'
   12: from __future__ import annotations
   13: 
   14: import sys
   15: from pathlib import Path
   16: 
   17: branch_root = Path(sys.argv[1]).resolve()
   18: override = sys.argv[2] if len(sys.argv) > 2 else ""
   19: sys.path.insert(0, str(branch_root))
   20: 
   21: from artifact_paths import resolve_tmp_path
   22: 
   23: try:
   24:     print(resolve_tmp_path(override or None))
   25: except ValueError as exc:
   26:     print(f"[solver_release_env] {exc}", file=sys.stderr)
   27:     raise SystemExit(2)
   28: PY
```
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/build.sh`
  Observed mentions: `1`
  Focus ranges: 1-160, 1-220, 1-280
  Enclosing symbols: no symbols inferred
  Note: observed in failed-AC trace, boosted by failure_analysis_state
  Mtime: `2026-03-25 05:11:08 KST`
  Evidence lines:
    - `AC 6 → Bash: /bin/zsh -lc "sed -n '1,220p' build.sh"`
  Code excerpt:
```text
    1: #!/usr/bin/env bash
    2: set -euo pipefail
    3: cd "$(dirname "$0")"
    4: export PYTHONDONTWRITEBYTECODE=1
    5: source ./solver_release_env.sh
    6: python3 boj28350_resume.py build "$@"
    7: chmod +x ./boj28350_resume/solve
```
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/branch_certify_suite.py`
  Observed mentions: `1`
  Focus ranges: 1-60, 1-90, 1-260, 260-520, 280-390
  Enclosing symbols: function _ensure_under_lca_tree_artifacts [49-57], function _normalize_artifact_out_dir [65-66], function _normalize_cli_out_arg [69-82], function _generation_ready [85-87], function _ensure_generated_case [224-287], function _clear_case_outputs [290-294]
  Note: observed in failed-AC trace, mapped from failed AC semantics, boosted by failure_analysis_state
  Mtime: `2026-03-26 00:38:10 KST`
  Evidence lines:
    - `AC 6 → Bash: /bin/zsh -lc "pgrep -af 'branch_certify_suite.py|solver_s...`
  Code excerpt:
```text
    1: #!/usr/bin/env python3
    2: from __future__ import annotations
    3: 
    4: import os
    5: import shutil
    6: import sys
    7: import tempfile
    8: import time
    9: import importlib.util
   10: from contextlib import contextmanager
   11: from pathlib import Path
   12: 
   13: os.environ["PYTHONDONTWRITEBYTECODE"] = "1"
   14: sys.dont_write_bytecode = True
   15: 
   16: from artifact_paths import (
   17:     artifacts_root,
   18:     configure_branch_process_env,
   19:     ensure_under_artifacts,
   20:     resolve_output_path,
   21: )
   22: 
   23: 
   24: configure_branch_process_env()
   25: 
   26: 
   27: BRANCH_ROOT = Path(__file__).resolve().parent
   28: OUTER_ROOT = BRANCH_ROOT.parent
```

#### Focused Artifact Snapshots

- `boj3s_gate` latest: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/retry_loop/ac5_formal_attempt_current.latest_failure/certify.json`
  mtime: `2026-03-26 05:35:03 KST`
  summary: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/retry_loop/ac5_formal_attempt_current.latest_failure/certify.json`
- `tmp` latest: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/.tmp/sfcert3.tid2de26/certify.json`
  mtime: `2026-03-26 05:27:00 KST`
  summary: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/.tmp/sfcert3.tid2de26/certify.json`

#### Failed-AC Trace Excerpt

```text
  AC 6: Analyzing complexity...
  AC 6 → Bash: /bin/zsh -lc 'ls artifacts/lca_tree_stress_v5/retry_loop ...
  AC 6 → Bash: /bin/zsh -lc "sed -n '1,220p' boj28350_resume/current_sta...
  AC 6 → Bash: /bin/zsh -lc "sed -n '1,260p' artifacts/lca_tree_stress_v...
  AC 6 → Bash: /bin/zsh -lc "sed -n '1,260p' artifacts/lca_tree_stress_v...
  AC 6 → Bash: /bin/zsh -lc "sed -n '1,260p' artifacts/lca_tree_stress_v...
  AC 6 → Bash: /bin/zsh -lc "sed -n '1,260p' .ouroboros/failure_analysis...
  AC 6 → Bash: /bin/zsh -lc "sed -n '1,260p' artifacts/lca_tree_stress_v...
  AC 6 → Bash: /bin/zsh -lc "sed -n '1,220p' artifacts/lca_tree_stress_v...
  AC 6 → Bash: /bin/zsh -lc "sed -n '1,220p' artifacts/lca_tree_stress_v...
  AC 6 → Bash: /bin/zsh -lc "sed -n '1,260p' solver_release_env.sh"
  AC 6 → Bash: /bin/zsh -lc "sed -n '1,260p' .ouroboros/failure_analysis...
  AC 6 → Bash: /bin/zsh -lc "sed -n '1,260p' lca_boj3s_gate.sh"
  AC 6 → Bash: /bin/zsh -lc "sed -n '1,260p' .ouroboros/failure_analysis...
  AC 6 → Bash: /bin/zsh -lc "sed -n '1,360p' outer_suite_wrappers/lca_bo...
  AC 6 → Bash: /bin/zsh -lc "sed -n '1,220p' build.sh"
  AC 6 → Bash: /bin/zsh -lc "find artifacts/lca_tree_stress_v5 -maxdepth...
  AC 6 → Bash: /bin/zsh -lc 'for d in artifacts/lca_tree_stress_v5/boj3s...
  AC 6 → Bash: /bin/zsh -lc "if [ -f artifacts/lca_tree_stress_v5/boj3s_...
  AC 6 → Bash: /bin/zsh -lc "find artifacts/lca_tree_stress_v5 -maxdepth...
  AC 6 → Bash: /bin/zsh -lc "sed -n '1,260p' artifacts/lca_tree_stress_v...
  AC 6 → Bash: /bin/zsh -lc "sed -n '1,260p' boj28350_resume/next_sessio...
  AC 6 → Bash: /bin/zsh -lc "python3 - <<'PY'
import csv
from pathlib im...
  AC 6 → Bash: /bin/zsh -lc 'if [ -f artifacts/lca_tree_stress_v5/.locks...
  AC 6 → Bash: /bin/zsh -lc "pgrep -af 'branch_certify_suite.py|solver_s...
  AC 6 → Bash: /bin/zsh -lc "find artifacts/lca_tree_stress_v5/.tmp -max...
  AC 6 → Bash: /bin/zsh -lc 'find artifacts/lca_tree_stress_v5/.tmp/lca_...
  AC 6 → Bash: /bin/zsh -lc "python3 - <<'PY'
from pathlib import Path
r...
  AC 6 → Bash: /bin/zsh -lc 'find artifacts/lca_tree_stress_v5/.tmp/lca_...
  AC 6 → Bash: /bin/zsh -lc 'for d in artifacts/lca_tree_stress_v5/boj3s...
  AC 6 → Bash: /bin/zsh -lc "find artifacts/lca_tree_stress_v5 -maxdepth...
  AC 6 → Bash: /bin/zsh -lc "if [ -f artifacts/lca_tree_stress_v5/boj3s_...
  AC 6 → Bash: /bin/zsh -lc 'find artifacts/lca_tree_stress_v5/.locks -m...
  AC 6 → Bash: /bin/zsh -lc 'find artifacts/lca_tree_stress_v5/.tmp -max...
Level 4 complete: 0 succeeded, 1 failed
╭────────────────────────────── Partial Success ───────────────────────────────╮
│ ### AC 6: [FAIL] Formal closure also requires running ./lca_boj3s_gate.sh    │
│ twice in a row on the same working tree with both runs PASS                  │
│ Error: Stalled (no activity for 300s)                                        │
```

## Refinement Versus Previous Failure

- The failed AC set changed relative to the previous captured failure.
- Recurring code-structure hotspots: /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/branch_certify_suite.py, /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/branch_validator.py, /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/build.sh, /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/lca_strong_gate.sh, /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/solver_release_env.sh, /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/suite_utils.py.
- New hotspots to fold into the next retry analysis: /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifact_paths.py, /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/lca_boj3s_gate.sh, /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/lca_smoke_target.sh.
- Recurring enclosing symbols: function _build_tree [52-91], function _parse_input [21-39], function _parse_output [42-49], function _read_tokens [9-18], function default_solver_name [33-34], function default_solver_path [37-38].
- Recurring line-range hotspots: /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/branch_certify_suite.py:1-260, /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/branch_certify_suite.py:260-520, /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/build.sh:1-220, /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/solver_release_env.sh:1-220, /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/solver_release_env.sh:1-260.
- Refine the next retry around these dominant phases: timing-timeout, artifact-paths, certify.

## Next-Retry Analysis Rule

- Before the next session edits code, read this breakdown, start from the repeated failed AC if one exists, and inspect the listed phase and code-structure hotspots before running the heavy gate again.
- If this breakdown still localizes the failure only at a broad file level, improve the retry analysis logic itself before the next heavy run so the next capture records narrower symbols, ranges, wrapper sections, and code excerpts.