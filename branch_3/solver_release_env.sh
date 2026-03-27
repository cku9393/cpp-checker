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

export PROFILE_MODE="${PROFILE_MODE:-PROFILE_NONE}"
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
# The current branch-local comb-family probes run better with state-load
# materialization left off by default; the HDT no-touch relabel path still
# preserves the progress40 direction while avoiding this secondary overhead.
export ENABLE_STATE_LOAD_MATERIALIZATION_OPT="${ENABLE_STATE_LOAD_MATERIALIZATION_OPT:-0}"
export ENABLE_PREV_STATE_CARRY_REUSE_OPT="${ENABLE_PREV_STATE_CARRY_REUSE_OPT:-1}"
export ENABLE_CARRY_REUSE_FASTPATH_OPT="${ENABLE_CARRY_REUSE_FASTPATH_OPT:-1}"
export ENABLE_CARRY_HIT_APPLY_OPT="${ENABLE_CARRY_HIT_APPLY_OPT:-1}"
# The bundled progress39 resume scripts keep the post-carry optimizer stack on,
# and progress40 adds layout-gate work on top of that cumulative baseline.
# On the current branch-local source, leaving writeback off still avoids the
# deterministic chain-family miscompare seen in the release build while the
# later reuse/layout chain remains available for explicit probe runs.
export ENABLE_PREV_STATE_WRITEBACK_OPT="${ENABLE_PREV_STATE_WRITEBACK_OPT:-0}"
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
export ENABLE_DELTA_PRESERVED_HIT="${ENABLE_DELTA_PRESERVED_HIT:-1}"
export ENABLE_DELTA_CONNECTOR_HIT="${ENABLE_DELTA_CONNECTOR_HIT:-1}"
