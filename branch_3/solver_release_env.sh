#!/usr/bin/env bash

# Branch-local release subset mirrored from the archived release runner.
# This enables the early performance pivots without forcing the later
# experimental pointer/patch/normalize branches that regressed the gate probe.

# Prevent the bootstrap resolver import below from leaving repo-root bytecode
# behind before the branch-local pycache prefix is established.
export PYTHONDONTWRITEBYTECODE="${PYTHONDONTWRITEBYTECODE:-1}"

# Keep transient tool output under branch-local artifacts instead of system temp.
__solver_release_env_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
__solver_release_env_tmp_override="${BRANCH_ARTIFACT_TMP_ROOT:-}"
if ! __solver_release_env_tmp="$(
  python3 - "$__solver_release_env_dir" "$__solver_release_env_tmp_override" <<'PY'
from __future__ import annotations

import sys
from pathlib import Path

branch_root = Path(sys.argv[1]).resolve()
override = sys.argv[2] if len(sys.argv) > 2 else ""
sys.path.insert(0, str(branch_root))

from artifact_paths import resolve_tmp_path

try:
    print(resolve_tmp_path(override or None))
except ValueError as exc:
    print(f"[solver_release_env] {exc}", file=sys.stderr)
    raise SystemExit(2)
PY
)"; then
  unset __solver_release_env_dir
  unset __solver_release_env_tmp_override
  unset __solver_release_env_tmp
  return 2 2>/dev/null || exit 2
fi
export BRANCH_ARTIFACT_TMP_ROOT="$__solver_release_env_tmp"
mkdir -p "$BRANCH_ARTIFACT_TMP_ROOT"
export TMPDIR="$BRANCH_ARTIFACT_TMP_ROOT"
export TMP="$BRANCH_ARTIFACT_TMP_ROOT"
export TEMP="$BRANCH_ARTIFACT_TMP_ROOT"
__solver_release_env_artifacts_root="$__solver_release_env_dir/artifacts"
__solver_release_env_resolve_existing_artifact_path() {
  local raw_value="${1-}"
  if [[ -z "$raw_value" ]]; then
    return 1
  fi
  python3 "$__solver_release_env_dir/artifact_paths.py" --ensure "$raw_value" 2>/dev/null
}
__solver_release_env_keep_or_set() {
  local var_name="$1"
  local desired="$2"
  local current=""
  local resolved_current=""
  eval "current=\${$var_name-}"
  if resolved_current="$(__solver_release_env_resolve_existing_artifact_path "$current")"; then
    current="$resolved_current"
  else
    current="$desired"
  fi
  export "$var_name=$current"
}
__solver_release_env_require_under_artifacts() {
  local var_name="$1"
  local current="$2"
  if ! python3 "$__solver_release_env_dir/artifact_paths.py" --ensure "$current" >/dev/null 2>&1; then
    echo "[solver_release_env] $var_name escaped branch-local artifacts root: $current" >&2
    return 2
  fi
}
__solver_release_env_keep_or_set HOME "$BRANCH_ARTIFACT_TMP_ROOT/home"
__solver_release_env_keep_or_set XDG_CONFIG_HOME "$BRANCH_ARTIFACT_TMP_ROOT/xdg_config"
__solver_release_env_keep_or_set XDG_CACHE_HOME "$BRANCH_ARTIFACT_TMP_ROOT/xdg_cache"
__solver_release_env_keep_or_set XDG_STATE_HOME "$BRANCH_ARTIFACT_TMP_ROOT/xdg_state"
__solver_release_env_keep_or_set PYTHONPYCACHEPREFIX "$BRANCH_ARTIFACT_TMP_ROOT/pycache"
for __solver_release_env_var_name in \
  BRANCH_ARTIFACT_TMP_ROOT \
  TMPDIR \
  TMP \
  TEMP \
  HOME \
  XDG_CONFIG_HOME \
  XDG_CACHE_HOME \
  XDG_STATE_HOME \
  PYTHONPYCACHEPREFIX; do
  eval "__solver_release_env_var_value=\${$__solver_release_env_var_name-}"
  if ! __solver_release_env_require_under_artifacts "$__solver_release_env_var_name" "$__solver_release_env_var_value"; then
    unset __solver_release_env_var_name
    unset __solver_release_env_var_value
    unset -f __solver_release_env_require_under_artifacts
    unset -f __solver_release_env_keep_or_set
    unset -f __solver_release_env_resolve_existing_artifact_path
    unset __solver_release_env_artifacts_root
    unset __solver_release_env_dir
    unset __solver_release_env_tmp_override
    unset __solver_release_env_tmp
    return 2 2>/dev/null || exit 2
  fi
done
unset __solver_release_env_var_name
unset __solver_release_env_var_value
mkdir -p "$HOME" "$XDG_CONFIG_HOME" "$XDG_CACHE_HOME" "$XDG_STATE_HOME" "$PYTHONPYCACHEPREFIX"
unset -f __solver_release_env_require_under_artifacts
unset -f __solver_release_env_keep_or_set
unset -f __solver_release_env_resolve_existing_artifact_path
unset __solver_release_env_artifacts_root
unset __solver_release_env_dir
unset __solver_release_env_tmp_override
unset __solver_release_env_tmp

# Keep the progress40 local profiling baseline enabled in release runs. The
# branch-local gate wrappers capture solver stderr into artifacts already, and
# forcing PROFILE_NONE materially slows the rect-family boj3s probes compared
# with the source's intended PROFILE_BASE default.
export PROFILE_MODE="${PROFILE_MODE:-PROFILE_BASE}"
# If the shared branch-local binary was compiled with LOCAL for diagnostics,
# skip the embedded self-test and reduce checkpoint spam in gate runs.
export LOCAL_SKIP_SELF_TEST="${LOCAL_SKIP_SELF_TEST:-1}"
export PROFILE_PROGRESS_STRIDE="${PROFILE_PROGRESS_STRIDE:-1024}"

export ENABLE_REUSE_APPLY_OPT="${ENABLE_REUSE_APPLY_OPT:-1}"
export ENABLE_PRESERVED_SPLIT_OPT="${ENABLE_PRESERVED_SPLIT_OPT:-1}"
export ENABLE_WATCH_SCAN_OPT="${ENABLE_WATCH_SCAN_OPT:-1}"
export ENABLE_RETAIN_COMPACTION_OPT="${ENABLE_RETAIN_COMPACTION_OPT:-1}"
export ENABLE_KEPT_VECTOR_OPT="${ENABLE_KEPT_VECTOR_OPT:-1}"
export ENABLE_STABLE_COMPACTION_OPT="${ENABLE_STABLE_COMPACTION_OPT:-1}"
export ENABLE_BLOCK_COPY_COMPACTION_OPT="${ENABLE_BLOCK_COPY_COMPACTION_OPT:-1}"
export ENABLE_COPY_PLAN_BUILD_OPT="${ENABLE_COPY_PLAN_BUILD_OPT:-1}"
export ENABLE_RUN_DISCOVERY_FUSION_OPT="${ENABLE_RUN_DISCOVERY_FUSION_OPT:-1}"
export ENABLE_FUSED_DISCOVERY_CLASSIFY_OPT="${ENABLE_FUSED_DISCOVERY_CLASSIFY_OPT:-1}"
export ENABLE_TSCAN_CORE_OPT="${ENABLE_TSCAN_CORE_OPT:-1}"
export ENABLE_TSCAN_BRANCH_STATE_OPT="${ENABLE_TSCAN_BRANCH_STATE_OPT:-1}"
# Keep AC3 unanimous support reuse enabled on branch_3. The re-anchored March
# 28 AC3 line still relies on this broad gate to avoid falling back to repeated
# full support rebuilds on the dense strong-gate families.
export AC3_SUPPORT_REUSE_MAX_TOUCHED="${AC3_SUPPORT_REUSE_MAX_TOUCHED:-100000}"
# The current correctness-fuzz blocker on the progress40 line is the
# `caterpillar_rect_dense n=1024` corridor. Disabling the single-positive reuse
# shortcut in release runs keeps the broader support-reuse machinery intact
# while avoiding the branch-local fastpath that pushes the exact seed-2 blocker
# over the 2s prerequisite-gate budget.
export AC3_ALLOW_SINGLE_POSITIVE_REUSE="${AC3_ALLOW_SINGLE_POSITIVE_REUSE:-0}"
# Fresh 2026-04-09 same-worktree probes on the rebuilt branch show that the
# progress40 late stack only turns into a net win once the state-load
# materialization path is re-enabled together with the pointer / pack /
# normalize chain. With that full line active, `comb_rect_dense n=1024`
# improved from about 1.96s to 1.81s and `comb_dense n=2048` from about 7.86s
# to 6.53s. Keep the flag overridable, but make the branch-local default follow
# the currently strongest reproducible progress40-aligned release mix.
export ENABLE_STATE_LOAD_MATERIALIZATION_OPT="${ENABLE_STATE_LOAD_MATERIALIZATION_OPT:-1}"
export ENABLE_PREV_STATE_CARRY_REUSE_OPT="${ENABLE_PREV_STATE_CARRY_REUSE_OPT:-1}"
export ENABLE_CARRY_REUSE_FASTPATH_OPT="${ENABLE_CARRY_REUSE_FASTPATH_OPT:-1}"
export ENABLE_CARRY_HIT_APPLY_OPT="${ENABLE_CARRY_HIT_APPLY_OPT:-1}"
# The re-anchored March 28 AC3 solver keeps the carry/writeback lane correct on
# the chain-family sentinels and needs it for the dense strong-gate corridor.
export ENABLE_PREV_STATE_WRITEBACK_OPT="${ENABLE_PREV_STATE_WRITEBACK_OPT:-1}"
# Re-enable the cumulative progress40 late stack by default. On the current
# solver build, keeping these branches off leaves the stronger pack/normalize
# line inactive and measurably slower on the representative AC5 release probes.
export ENABLE_POINTER_REBIND_OPT="${ENABLE_POINTER_REBIND_OPT:-1}"
export ENABLE_TARGET_RESOLVE_PINNING_OPT="${ENABLE_TARGET_RESOLVE_PINNING_OPT:-1}"
export ENABLE_DIRECT_PREBIND_OPT="${ENABLE_DIRECT_PREBIND_OPT:-1}"
export ENABLE_REBIND_COMMIT_OPT="${ENABLE_REBIND_COMMIT_OPT:-1}"
export ENABLE_METADATA_PATCH_SLOT_OWNER_OPT="${ENABLE_METADATA_PATCH_SLOT_OWNER_OPT:-1}"
export ENABLE_SLOT_OWNER_UPDATE_OPT="${ENABLE_SLOT_OWNER_UPDATE_OPT:-1}"
export ENABLE_SLOT_OWNER_PATCH_COMMIT_OPT="${ENABLE_SLOT_OWNER_PATCH_COMMIT_OPT:-1}"
export ENABLE_SLOT_OWNER_FIELD_PATCH_OPT="${ENABLE_SLOT_OWNER_FIELD_PATCH_OPT:-1}"
export ENABLE_COMPACT_FIELD_PACK_OPT="${ENABLE_COMPACT_FIELD_PACK_OPT:-1}"
export ENABLE_PACK_ENCODE_NORMALIZE_OPT="${ENABLE_PACK_ENCODE_NORMALIZE_OPT:-1}"
export ENABLE_PACK_ENCODE_NORMALIZE_CORE_OPT="${ENABLE_PACK_ENCODE_NORMALIZE_CORE_OPT:-1}"
export ENABLE_CANONICAL_NORMALIZE_OPT="${ENABLE_CANONICAL_NORMALIZE_OPT:-1}"
export ENABLE_LAYOUT_REUSE_ZERO_ELISION_OPT="${ENABLE_LAYOUT_REUSE_ZERO_ELISION_OPT:-1}"
export ENABLE_LAYOUT_SIGNATURE_GATE_OPT="${ENABLE_LAYOUT_SIGNATURE_GATE_OPT:-1}"
# Current branch-local AC5 probes show the dense 1024 smoke rows improve when
# the delta preserved / connector routes stay disabled by default, while the
# sampled hard-scaling cap row remains effectively unchanged. Keep the routes
# overridable, but make the reproducible release baseline follow that faster
# branch-local mix until the route-selection corridor is reworked.
export ENABLE_DELTA_PRESERVED_HIT="${ENABLE_DELTA_PRESERVED_HIT:-0}"
export ENABLE_DELTA_CONNECTOR_HIT="${ENABLE_DELTA_CONNECTOR_HIT:-0}"
# The DynamicGraph component enumerator used by the untouched fastpath produces
# wrong large-N parent trees on balanced / random max-N runs in this branch.
# Keep the untouched split path, but fall back to the safe component walk.
export ENABLE_NOTOUCH_FAST_COMPONENT_ENUM="${ENABLE_NOTOUCH_FAST_COMPONENT_ENUM:-0}"
