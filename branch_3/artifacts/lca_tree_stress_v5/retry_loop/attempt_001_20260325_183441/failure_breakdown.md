# Failure Breakdown: Attempt 1

- Timestamp: `2026-03-25 20:28:28 KST`
- Session ID: `orch_635d8afd7474`
- Execution ID: `exec_c7ad8cd4b7c1`
- Analysis state file: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/.ouroboros/failure_analysis_state.json`
- Analysis state revision: `1`

## Failure Decomposition

### AC 3: ./lca_strong_gate.sh passes as a required prerequisite gate │

- Failure type: `stall/no-activity`
- Failure family: `strong_gate_unspecified`
- Interpretation lane: `correctness-proof`
- Primary progress40 axis: `zero_span_fastpath`
- Secondary progress40 axis: `watch_diff`
- Profile mode observed: `PROFILE_SAMPLED`
- Last progress checkpoint phase: `unknown`
- Last release diag phase: `unknown`
- Suggested next probe: `LCA_STAGE_FILTER=correctness_fuzz ./lca_strong_gate.sh`
- Trace lines captured: `80`

#### Progress40 Axis Evidence

- `watch_diff`
  - `AC 3 → Bash: /bin/zsh -lc "rg -n \"retainClassWatchByKeepMask\\(|stabl...`
- `retain_compaction`
  - `AC 3 → Bash: /bin/zsh -lc "rg -n \"retainClassWatchByKeepMask\\(|stabl...`
- `zero_span_fastpath`
  - `AC 3 → Bash: /bin/zsh -lc 'rg -n "zero-span|zero span|eligibility|fast...`
  - `AC 3 → Bash: /bin/zsh -lc 'rg -n "ENABLE_LAYOUT_REUSE_ZERO_ELISION_OPT...`

- Enabled flags seen in trace: `ENABLE_AC`, `ENABLE_COMPACT_RELEASE_`, `ENABLE_DELTA_C`, `ENABLE_DELTA_PRESERVED_HIT`, `ENABLE_LAYOUT_REUSE_ZERO_ELISION_OPT`, `ENABLE_LAYOUT_SIGNATURE_GATE_OPT`
- Current summary pivot baseline: `zero-span eligibility and fastpath commit`
- Current summary residual axes: `state_materialization`, `layout_gate`, `zero_span_fastpath`

#### Phase Breakdown

- `timing-timeout` x8 | sample: `AC 3 → Bash: /bin/zsh -lc 'source ./solver_release_env.sh && PROFILE_M...`
- `build` x7 | sample: `AC 3 → Bash: /bin/zsh -lc ./build.sh`
- `certify` x4 | sample: `AC 3 → Bash: /bin/zsh -lc "sed -n '360,460p' branch_certify_suite.py"`
- `artifact-paths` x1 | sample: `AC 3 → Bash: /bin/zsh -lc 'python3 artifact_paths.py boj28350_direct_s...`
- `solver-source` x1 | sample: `AC 3 → Bash: /bin/zsh -lc "rg -n \"layout_signature_gate_opt_enabled\\...`
- `wrapper` x1 | sample: `│ ### AC 3: [FAIL] ./lca_strong_gate.sh passes as a required prerequisite gate │`

#### Code-Structure Hotspots

- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/build.sh`
  Observed mentions: `8`
  Focus ranges: 1-220, 1-260
  Enclosing symbols: no symbols inferred
  Note: observed in failed-AC trace
  Mtime: `2026-03-25 05:11:08 KST`
  Evidence lines:
    - `AC 3 → Bash: /bin/zsh -lc ./build.sh`
    - `AC 3 → Bash: /bin/zsh -lc "sed -n '1,220p' build.sh"`
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
  Observed mentions: `8`
  Focus ranges: none captured
  Enclosing symbols: function _read_tokens [9-18], function _parse_input [21-39], function _parse_output [42-49], function _build_tree [52-91]
  Note: observed in failed-AC trace
  Mtime: `2026-03-25 05:40:13 KST`
  Evidence lines:
    - `AC 3 → Bash: /bin/zsh -lc 'python3 branch_validator.py artifacts/lca_t...`
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
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/solver_release_env.sh`
  Observed mentions: `8`
  Focus ranges: 1-120, 1-220, 1-260
  Enclosing symbols: function __solver_release_env_keep_or_set [41-107]
  Note: observed in failed-AC trace, mapped from failed AC semantics, boosted by failure_analysis_state
  Mtime: `2026-03-25 20:21:25 KST`
  Evidence lines:
    - `AC 3 → Bash: /bin/zsh -lc 'source ./solver_release_env.sh && PROFILE_M...`
    - `AC 3 → Bash: /bin/zsh -lc 'source ./solver_release_env.sh && /usr/bin/...`
    - `AC 3 → Bash: /bin/zsh -lc 'source ./solver_release_env.sh && ENABLE_AC...`
    - `AC 3 → Bash: /bin/zsh -lc "sed -n '1,220p' solver_release_env.sh"`
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
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/branch_certify_suite.py`
  Observed mentions: `4`
  Focus ranges: 1-260, 1-360, 260-520, 360-460
  Enclosing symbols: function _ensure_under_lca_tree_artifacts [46-54], function _normalize_artifact_out_dir [62-63], function _normalize_cli_out_arg [66-79], function _generation_ready [82-84], function _ensure_generated_case [221-284], function _clear_case_outputs [287-291]
  Note: observed in failed-AC trace, mapped from failed AC semantics, boosted by failure_analysis_state
  Mtime: `2026-03-25 14:36:25 KST`
  Evidence lines:
    - `AC 3 → Bash: /bin/zsh -lc "sed -n '360,460p' branch_certify_suite.py"`
    - `AC 3 → Bash: /bin/zsh -lc 'python3 branch_certify_suite.py --solver bo...`
    - `AC 3 → Bash: /bin/zsh -lc 'pgrep -af "branch_certify_suite.py --solver...`
    - `AC 3 → Bash: /bin/zsh -lc "python3 branch_certify_suite.py --solver /U...`
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
   13: from artifact_paths import (
   14:     artifacts_root,
   15:     configure_branch_process_env,
   16:     ensure_under_artifacts,
   17:     resolve_output_path,
   18: )
   19: 
   20: 
   21: configure_branch_process_env()
   22: 
   23: 
   24: BRANCH_ROOT = Path(__file__).resolve().parent
   25: OUTER_ROOT = BRANCH_ROOT.parent
   26: TOOLING_ROOT = OUTER_ROOT / "lca_tree_stress_v5" / "tooling"
   27: 
   28: _branch_suite_utils_spec = importlib.util.spec_from_file_location(
```
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/suite_utils.py`
  Observed mentions: `3`
  Focus ranges: none captured
  Enclosing symbols: function parse_int_list_csv [19-23], function parse_str_list_csv [26-30], function default_solver_name [33-34], function default_solver_path [37-38]
  Note: observed in failed-AC trace, boosted by failure_analysis_state, matched pinned symbol `function parse_int_list_csv [19-23]`
  Mtime: `2026-03-24 23:13:12 KST`
  Code excerpt:
```text
   17: 
   18: 
   19: def parse_int_list_csv(s: str) -> List[int]:
   20:     s = s.strip()
   21:     if not s:
   22:         return []
   23:     return [int(x) for x in s.split(",") if x.strip()]
   24: 
   25: 
```
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/solver_release_env.sh`
  Observed mentions: `3`
  Focus ranges: none captured
  Enclosing symbols: function __solver_release_env_keep_or_set [41-107]
  Note: observed in failed-AC trace, boosted by failure_analysis_state
  Mtime: `2026-03-25 20:21:25 KST`
  Evidence lines:
    - `AC 3 → Bash: /bin/zsh -lc 'source ./solver_release_env.sh && PROFILE_M...`
    - `AC 3 → Bash: /bin/zsh -lc 'source ./solver_release_env.sh && /usr/bin/...`
    - `AC 3 → Bash: /bin/zsh -lc 'source ./solver_release_env.sh && ENABLE_AC...`
    - `AC 3 → Bash: /bin/zsh -lc "sed -n '1,220p' solver_release_env.sh"`
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
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/lca_strong_gate.sh`
  Observed mentions: `3`
  Focus ranges: none captured
  Enclosing symbols: no symbols inferred
  Note: observed in failed-AC trace, boosted by failure_analysis_state
  Mtime: `2026-03-25 11:32:56 KST`
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
- File: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/branch_certify_suite.py`
  Observed mentions: `3`
  Focus ranges: none captured
  Enclosing symbols: function _ensure_under_lca_tree_artifacts [46-54], function _normalize_artifact_out_dir [62-63], function _normalize_cli_out_arg [66-79], function _generation_ready [82-84]
  Note: observed in failed-AC trace, boosted by failure_analysis_state
  Mtime: `2026-03-25 14:36:25 KST`
  Evidence lines:
    - `AC 3 → Bash: /bin/zsh -lc "sed -n '360,460p' branch_certify_suite.py"`
    - `AC 3 → Bash: /bin/zsh -lc 'python3 branch_certify_suite.py --solver bo...`
    - `AC 3 → Bash: /bin/zsh -lc 'pgrep -af "branch_certify_suite.py --solver...`
    - `AC 3 → Bash: /bin/zsh -lc "python3 branch_certify_suite.py --solver /U...`
  Code excerpt:
```text
   44: 
   45: 
   46: def _ensure_under_lca_tree_artifacts(path: str | Path) -> Path:
   47:     resolved = ensure_under_artifacts(path)
   48:     try:
   49:         resolved.relative_to(LCA_TREE_STRESS_ARTIFACTS_ROOT)
   50:     except ValueError as exc:
   51:         raise ValueError(
   52:             f"certify helper path must stay under {LCA_TREE_STRESS_ARTIFACTS_ROOT}: {resolved}"
   53:         ) from exc
   54:     return resolved
   55: 
   56: 
```

#### Focused Artifact Snapshots

- `strong_gate` latest: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/certify.json`
  mtime: `2026-03-25 00:32:23 KST`
  summary: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/certify.json`
- `tmp` latest: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/.tmp/case_runs/seed2_L1_Q1.run.gj27eioh/solver_stderr.txt`
  mtime: `2026-03-25 20:27:53 KST`

#### Failed-AC Trace Excerpt

```text
  AC 3 → Edit: /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian...
  AC 3 → Edit: /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian...
  AC 3 → Bash: /bin/zsh -lc ./build.sh
  AC 3 → Bash: /bin/zsh -lc "sed -n '1,220p' artifacts/lca_tree_stress_v...
  AC 3 → Bash: /bin/zsh -lc 'source ./solver_release_env.sh && ENABLE_AC...
  AC 3 → Bash: /bin/zsh -lc "sed -n '1,220p' artifacts/lca_tree_stress_v...
  AC 3 → Bash: /bin/zsh -lc 'ls -l artifacts/lca_tree_stress_v5/retry_lo...
  AC 3 → Bash: /bin/zsh -lc "rg -n \"applyPieceNativeReuseForClassBaseli...
  AC 3 → Bash: /bin/zsh -lc "sed -n '12360,12680p' boj28350_resume/boj28...
  AC 3 → Bash: /bin/zsh -lc "sed -n '13180,14040p' boj28350_resume/boj28...
  AC 3 → Bash: /bin/zsh -lc "sed -n '1166,1188p' boj28350_resume/boj2835...
  AC 3 → Bash: /bin/zsh -lc "sed -n '1092,1166p' boj28350_resume/boj2835...
  AC 3 → Edit: /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian...
  AC 3 → Edit: /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian...
  AC 3 → Bash: /bin/zsh -lc "sed -n '13740,13870p' boj28350_resume/boj28...
  AC 3 → Bash: /bin/zsh -lc "sed -n '13620,13740p' boj28350_resume/boj28...
  AC 3 → Bash: /bin/zsh -lc "sed -n '13870,14030p' boj28350_resume/boj28...
  AC 3 → Edit: /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian...
  AC 3 → Bash: /bin/zsh -lc ./build.sh
  AC 3 → Bash: /bin/zsh -lc 'source ./solver_release_env.sh && ENABLE_AC...
  AC 3 → Bash: /bin/zsh -lc "sed -n '1,240p' artifacts/lca_tree_stress_v...
  AC 3 → Bash: /bin/zsh -lc 'python3 branch_validator.py artifacts/lca_t...
  AC 3 → Bash: /bin/zsh -lc "sed -n '1,240p' artifacts/lca_tree_stress_v...
  AC 3 → Bash: /bin/zsh -lc 'source ./solver_release_env.sh && ENABLE_AC...
  AC 3 → Bash: /bin/zsh -lc "sed -n '1,240p' artifacts/lca_tree_stress_v...
  AC 3 → Bash: /bin/zsh -lc 'wc -c artifacts/lca_tree_stress_v5/retry_lo...
  AC 3 → Bash: /bin/zsh -lc 'python3 branch_validator.py artifacts/lca_t...
  AC 3 → Bash: /bin/zsh -lc 'rg -n "deltaPreservedHitEnabled|deltaConnec...
  AC 3 → Bash: /bin/zsh -lc "sed -n '930,1015p' boj28350_resume/boj28350...
  AC 3 → Bash: /bin/zsh -lc "sed -n '10220,10268p' boj28350_resume/boj28...
  AC 3 → Edit: /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian...
  AC 3 → Edit: /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian...
  AC 3 → Bash: /bin/zsh -lc ./build.sh
  AC 3 → Bash: /bin/zsh -lc 'source ./solver_release_env.sh && ENABLE_AC...
  AC 3 → Bash: /bin/zsh -lc "sed -n '1,240p' artifacts/lca_tree_stress_v...
  AC 3 → Bash: /bin/zsh -lc 'cmp -s artifacts/lca_tree_stress_v5/retry_l...
  AC 3 → Bash: /bin/zsh -lc 'python3 branch_validator.py artifacts/lca_t...
  AC 3 → Bash: /bin/zsh -lc 'python3 branch_validator.py artifacts/lca_t...
  AC 3 → Bash: /bin/zsh -lc "/usr/bin/time -p zsh -lc 'source ./solver_r...
  AC 3 → Bash: /bin/zsh -lc 'python3 branch_validator.py artifacts/lca_t...
  AC 3 → Bash: /bin/zsh -lc "rg -n \"collectRelevantEndpointIdxs\\(|fall...
  AC 3 → Bash: /bin/zsh -lc "sed -n '10540,10820p' boj28350_resume/boj28...
  AC 3 → Bash: /bin/zsh -lc "sed -n '10320,10540p' boj28350_resume/boj28...
  AC 3 → Edit: /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian...
  AC 3 → Bash: /bin/zsh -lc ./build.sh
  AC 3 → Bash: /bin/zsh -lc "/usr/bin/time -p zsh -lc 'source ./solver_r...
  AC 3 → Bash: /bin/zsh -lc 'python3 branch_validator.py artifacts/lca_t...
  AC 3 → Bash: /bin/zsh -lc "rg -n \"dispatchPublishAnnotatePreserved\\(...
  AC 3 → Bash: /bin/zsh -lc "sed -n '6765,6935p' boj28350_resume/boj2835...
  AC 3 → Bash: /bin/zsh -lc "sed -n '6510,6765p' boj28350_resume/boj2835...
  AC 3 → Edit: /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian...
  AC 3 → Bash: /bin/zsh -lc ./build.sh
  AC 3 → Bash: /bin/zsh -lc "/usr/bin/time -p zsh -lc 'source ./solver_r...
  AC 3 → Bash: /bin/zsh -lc 'python3 branch_validator.py artifacts/lca_t...
  AC 3 → Bash: /bin/zsh -lc "sed -n '7174,7248p' boj28350_resume/boj2835...
  AC 3 → Bash: /bin/zsh -lc "sed -n '6828,6868p' boj28350_resume/boj2835...
  AC 3 → Bash: /bin/zsh -lc "sed -n '7248,7335p' boj28350_resume/boj2835...
  AC 3 → Edit: /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian...
  AC 3 → Bash: /bin/zsh -lc ./build.sh
  AC 3 → Bash: /bin/zsh -lc "/usr/bin/time -p zsh -lc 'source ./solver_r...
  AC 3 → Bash: /bin/zsh -lc 'python3 branch_validator.py artifacts/lca_t...
  AC 3 → Bash: /bin/zsh -lc 'ENABLE_DELTA_PRESERVED_HIT=0 ENABLE_DELTA_C...
  AC 3 → Bash: /bin/zsh -lc "find artifacts/lca_tree_stress_v5/strong_ga...
  AC 3 → Bash: /bin/zsh -lc "python3 - <<'PY'
import json
from pathlib i...
  AC 3 → Bash: /bin/zsh -lc "sed -n '1,200p' artifacts/lca_tree_stress_v...
  AC 3 → Bash: /bin/zsh -lc "find artifacts/lca_tree_stress_v5/.tmp/lca_...
  AC 3 → Bash: /bin/zsh -lc 'wc -l artifacts/lca_tree_stress_v5/strong_g...
  AC 3 → Bash: /bin/zsh -lc "sed -n '1,260p' lca_strong_gate.sh"
  AC 3 → Bash: /bin/zsh -lc "sed -n '1,320p' outer_suite_wrappers/lca_st...
  AC 3 → Bash: /bin/zsh -lc "sed -n '320,520p' outer_suite_wrappers/lca_...
  AC 3 → Bash: /bin/zsh -lc 'pgrep -af "branch_certify_suite.py --solver...
  AC 3 → Edit: /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian...
  AC 3 → Bash: /bin/zsh -lc "python3 branch_certify_suite.py --solver /U...
Level 3 complete: 0 succeeded, 1 failed
╭────────────────────────────── Partial Success ───────────────────────────────╮
│ ### AC 3: [FAIL] ./lca_strong_gate.sh passes as a required prerequisite gate │
│ Error: Stalled (no activity for 300s)                                        │
│                                                                              │
```

## Refinement Versus Previous Failure

- The failed AC set changed relative to the previous captured failure.
- Recurring code-structure hotspots: /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/branch_certify_suite.py, /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/build.sh, /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/lca_strong_gate.sh, /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/solver_release_env.sh, /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/suite_utils.py.
- New hotspots to fold into the next retry analysis: /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/branch_validator.py.
- Recurring enclosing symbols: function default_solver_name [33-34], function default_solver_path [37-38], function parse_int_list_csv [19-23], function parse_str_list_csv [26-30].
- Recurring line-range hotspots: /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/branch_certify_suite.py:1-260, /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/branch_certify_suite.py:260-520, /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/build.sh:1-220, /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/build.sh:1-260, /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/solver_release_env.sh:1-220, /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/solver_release_env.sh:1-260.
- Refine the next retry around these dominant phases: timing-timeout, build.

## Next-Retry Analysis Rule

- Before the next session edits code, read this breakdown, start from the repeated failed AC if one exists, and inspect the listed phase and code-structure hotspots before running the heavy gate again.
- If this breakdown still localizes the failure only at a broad file level, improve the retry analysis logic itself before the next heavy run so the next capture records narrower symbols, ranges, wrapper sections, and code excerpts.