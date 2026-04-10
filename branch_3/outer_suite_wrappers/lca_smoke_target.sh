#!/usr/bin/env bash
set -euo pipefail

LCA_SMOKE_TARGET_CLEAN_ENV_FLAG="LCA_SMOKE_TARGET_CLEAN_ENV_READY"
LCA_SMOKE_TARGET_CLEAN_PATH="/opt/homebrew/bin:/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin"
SELF_SOURCE="${BASH_SOURCE[0]}"
SELF_SOURCE_DIR="."
case "$SELF_SOURCE" in
  */*)
    SELF_SOURCE_DIR="${SELF_SOURCE%/*}"
    ;;
esac
SELF_DIR="$(
  unset CDPATH
  cd -- "$SELF_SOURCE_DIR"
  pwd -P
)"
SELF_PATH="$SELF_DIR/${SELF_SOURCE##*/}"

if [[ "${!LCA_SMOKE_TARGET_CLEAN_ENV_FLAG:-0}" != "1" ]]; then
  clean_env_args=(
    /usr/bin/env -i
    "HOME=${HOME:-}"
    "PATH=$LCA_SMOKE_TARGET_CLEAN_PATH"
    "TERM=${TERM:-dumb}"
    "$LCA_SMOKE_TARGET_CLEAN_ENV_FLAG=1"
  )
  if [[ -n "${LCA_SMOKE_TARGET_MANIFEST:-}" ]]; then
    clean_env_args+=("LCA_SMOKE_TARGET_MANIFEST=$LCA_SMOKE_TARGET_MANIFEST")
  fi
  exec "${clean_env_args[@]}" /usr/bin/env bash "$SELF_PATH" "$@"
fi

SCRIPT_DIR="$SELF_DIR"
BRANCH_ROOT="$(cd "$SCRIPT_DIR/.." && pwd -P)"
cd "$BRANCH_ROOT"
export PYTHONDONTWRITEBYTECODE=1

ARTIFACT_RESOLVER="$BRANCH_ROOT/artifact_paths.py"
BUILD_WRAPPER="$BRANCH_ROOT/build.sh"
RELEASE_ENV="$BRANCH_ROOT/solver_release_env.sh"
RUN_CASE_HELPER="$BRANCH_ROOT/branch_run_case.py"
SMOKE_CASES_SOURCE="$BRANCH_ROOT/boj28350_resume/smoke_cases.tsv"
SMOKE_CASES_FROZEN="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/smoke/environment_validation/smoke_cases.snapshot.tsv"
SMOKE_CASES_ACTIVE=""
SOLVER="$BRANCH_ROOT/artifacts/boj28350_resume/build/solve"
BRANCH_ARTIFACTS_ROOT="$BRANCH_ROOT/artifacts"
TMP_PARENT="$BRANCH_ARTIFACTS_ROOT/lca_tree_stress_v5/.tmp"

OUTROOT=""
CASE_SELECTOR=""
ARTIFACT_SUBPATH=""
LIST_ONLY=0
TARGET_ENV_ROOT=""
TARGET_TMPDIR=""
TARGET_HOME=""
TARGET_XDG_CONFIG_HOME=""
TARGET_XDG_CACHE_HOME=""
TARGET_XDG_STATE_HOME=""
TARGET_PYCACHE_ROOT=""

fail() {
  echo "[lca_smoke_target] $*" >&2
  exit 1
}

cleanup() {
  local rc="${1:-$?}"
  trap - EXIT
  set +e
  if [[ -n "$TARGET_ENV_ROOT" && -e "$TARGET_ENV_ROOT" ]]; then
    rm -rf "$TARGET_ENV_ROOT"
  fi
  exit "$rc"
}

ensure_under_artifacts() {
  local path="$1"
  case "$path" in
    "$BRANCH_ARTIFACTS_ROOT"|"$BRANCH_ARTIFACTS_ROOT"/*)
      ;;
    *)
      fail "path escaped branch-local artifacts root: $path"
      ;;
  esac
}

ensure_under_branch_root() {
  local path="$1"
  case "$path" in
    "$BRANCH_ROOT"|"$BRANCH_ROOT"/*)
      ;;
    *)
      fail "path escaped branch root: $path"
      ;;
  esac
}

usage() {
  cat >&2 <<'EOF'
usage: ./outer_suite_wrappers/lca_smoke_target.sh --list
usage: ./outer_suite_wrappers/lca_smoke_target.sh <case-index-or-tag> [artifact_subpath]
[lca_smoke_target] replays one manifest-defined lca_smoke case with the same branch-local run_case arguments and solver env flags used by ./lca_smoke.sh
[lca_smoke_target] prefers the frozen smoke manifest snapshot from the latest published smoke run and accepts LCA_SMOKE_TARGET_MANIFEST for explicit replay snapshots
[lca_smoke_target] output stays under branch-local artifacts/lca_tree_stress_v5/smoke_target/...
EOF
  exit 2
}

require_command() {
  if ! command -v "$1" >/dev/null 2>&1; then
    fail "missing required tool: $1"
  fi
}

require_file() {
  local path="$1"
  local label="$2"
  if [[ ! -f "$path" ]]; then
    fail "missing ${label}: $path"
  fi
}

require_executable() {
  local path="$1"
  local label="$2"
  if [[ ! -x "$path" ]]; then
    fail "missing executable ${label}: $path"
  fi
}

format_replay_command() {
  python3 - "$@" <<'PY'
from __future__ import annotations

import shlex
import sys

print(shlex.join(sys.argv[1:]))
PY
}

parse_args() {
  case $# in
    1)
      if [[ "$1" == "--list" ]]; then
        LIST_ONLY=1
        return
      fi
      CASE_SELECTOR="$1"
      ;;
    2)
      if [[ "$1" == "--list" ]]; then
        usage
      fi
      CASE_SELECTOR="$1"
      ARTIFACT_SUBPATH="$2"
      ;;
    *)
      usage
      ;;
  esac
}

resolve_manifest_path() {
  local candidate="${LCA_SMOKE_TARGET_MANIFEST:-}"
  local resolved=""

  if [[ -n "$candidate" ]]; then
    resolved="$(python3 - "$BRANCH_ROOT" "$candidate" <<'PY'
from __future__ import annotations

import sys
from pathlib import Path

branch_root = Path(sys.argv[1]).resolve()
candidate = Path(sys.argv[2])
if not candidate.is_absolute():
    candidate = branch_root / candidate
print(candidate.resolve())
PY
)"
    ensure_under_branch_root "$resolved"
    require_file "$resolved" "explicit smoke target manifest"
    SMOKE_CASES_ACTIVE="$resolved"
    return
  fi

  if [[ -f "$SMOKE_CASES_FROZEN" ]]; then
    SMOKE_CASES_ACTIVE="$SMOKE_CASES_FROZEN"
    return
  fi

  SMOKE_CASES_ACTIVE="$SMOKE_CASES_SOURCE"
}

list_manifest_cases() {
  python3 - "$SMOKE_CASES_ACTIVE" <<'PY'
from __future__ import annotations

import csv
import pathlib
import re
import sys

tsv_path = pathlib.Path(sys.argv[1])


def sanitize(value: str) -> str:
    raw = value.replace("\r", "")
    raw = raw.replace(".", "p")
    raw = re.sub(r"[^A-Za-z0-9._-]", "_", raw)
    while "__" in raw:
        raw = raw.replace("__", "_")
    raw = raw.strip("_")
    return raw or "x"


with tsv_path.open(newline="", encoding="utf-8") as handle:
    rows = list(csv.DictReader(handle, delimiter="\t"))

print("case_index\tcase_tag\tstage\tmode\tn\tseed\tshuffle_labels\tshuffle_queries\ttimeout_s")
for index, row in enumerate(rows, start=1):
    stage = (row.get("stage") or "").strip()
    mode = (row.get("mode") or "").strip()
    n = (row.get("n") or "").strip()
    seed = (row.get("seed") or "").strip()
    shuffle_labels = (row.get("shuffle_labels") or "").strip()
    shuffle_queries = (row.get("shuffle_queries") or "").strip()
    timeout_s = (row.get("timeout_s") or "").strip()
    case_tag = (
        f"case{index:02d}_{sanitize(stage)}_{sanitize(mode)}_n{sanitize(n)}_"
        f"s{sanitize(seed)}_L{sanitize(shuffle_labels)}_Q{sanitize(shuffle_queries)}_"
        f"t{sanitize(timeout_s)}"
    )
    print(
        "\t".join(
            [
                str(index),
                case_tag,
                stage,
                mode,
                n,
                seed,
                shuffle_labels,
                shuffle_queries,
                timeout_s,
            ]
        )
    )
PY
}

resolve_selected_case() {
  local parsed=""
  parsed="$(
    python3 - "$SMOKE_CASES_ACTIVE" "$CASE_SELECTOR" <<'PY'
from __future__ import annotations

import csv
import pathlib
import re
import shlex
import sys

tsv_path = pathlib.Path(sys.argv[1])
selector = sys.argv[2]


def fail(message: str) -> None:
    print(f"[lca_smoke_target] {message}", file=sys.stderr)
    raise SystemExit(1)


def sanitize(value: str) -> str:
    raw = value.replace("\r", "")
    raw = raw.replace(".", "p")
    raw = re.sub(r"[^A-Za-z0-9._-]", "_", raw)
    while "__" in raw:
        raw = raw.replace("__", "_")
    raw = raw.strip("_")
    return raw or "x"


def normalized(value: str | None) -> str:
    return (value or "").replace("\r", "").strip()


def build_tag(index: int, row: dict[str, str]) -> str:
    return (
        f"case{index:02d}_{sanitize(normalized(row.get('stage')))}_"
        f"{sanitize(normalized(row.get('mode')))}_n{sanitize(normalized(row.get('n')))}_"
        f"s{sanitize(normalized(row.get('seed')))}_"
        f"L{sanitize(normalized(row.get('shuffle_labels')))}_"
        f"Q{sanitize(normalized(row.get('shuffle_queries')))}_"
        f"t{sanitize(normalized(row.get('timeout_s')))}"
    )


def validate_int(name: str, raw: str, *, allow_zero: bool = True) -> str:
    if not raw.isdigit():
        fail(f"selected smoke row has invalid {name}: {raw!r}")
    if not allow_zero and int(raw) <= 0:
        fail(f"selected smoke row must use {name} > 0 (got: {raw})")
    return raw


def validate_flag(name: str, raw: str) -> str:
    if raw not in {"0", "1"}:
        fail(f"selected smoke row has invalid {name}: {raw!r}")
    return raw


def validate_timeout(raw: str) -> str:
    try:
        value = float(raw)
    except ValueError:
        fail(f"selected smoke row has invalid timeout_s: {raw!r}")
    if value <= 0.0:
        fail(f"selected smoke row must use timeout_s > 0 (got: {raw})")
    return raw


with tsv_path.open(newline="", encoding="utf-8") as handle:
    rows = list(csv.DictReader(handle, delimiter="\t"))

selected_index = None
selected_row = None
selected_tag = None
numeric_selector = selector.isdigit()

for index, row in enumerate(rows, start=1):
    stage = normalized(row.get("stage"))
    mode = normalized(row.get("mode"))
    n = validate_int("n", normalized(row.get("n")), allow_zero=False)
    seed = validate_int("seed", normalized(row.get("seed")))
    shuffle_labels = validate_flag("shuffle_labels", normalized(row.get("shuffle_labels")))
    shuffle_queries = validate_flag("shuffle_queries", normalized(row.get("shuffle_queries")))
    timeout_s = validate_timeout(normalized(row.get("timeout_s")))
    if not stage or not mode:
        fail("selected smoke row must provide non-empty stage/mode")
    case_tag = build_tag(
        index,
        {
            "stage": stage,
            "mode": mode,
            "n": n,
            "seed": seed,
            "shuffle_labels": shuffle_labels,
            "shuffle_queries": shuffle_queries,
            "timeout_s": timeout_s,
        },
    )
    if numeric_selector and int(selector) == index:
        selected_index = index
        selected_row = {
            "stage": stage,
            "mode": mode,
            "n": n,
            "seed": seed,
            "shuffle_labels": shuffle_labels,
            "shuffle_queries": shuffle_queries,
            "timeout_s": timeout_s,
        }
        selected_tag = case_tag
        break
    if selector == case_tag:
        selected_index = index
        selected_row = {
            "stage": stage,
            "mode": mode,
            "n": n,
            "seed": seed,
            "shuffle_labels": shuffle_labels,
            "shuffle_queries": shuffle_queries,
            "timeout_s": timeout_s,
        }
        selected_tag = case_tag
        break

if selected_row is None or selected_index is None or selected_tag is None:
    fail(f"selector {selector!r} did not match any smoke case index or deterministic tag")

for key, value in (
    ("CASE_INDEX", str(selected_index)),
    ("CASE_TAG", selected_tag),
    ("STAGE", selected_row["stage"]),
    ("MODE", selected_row["mode"]),
    ("N", selected_row["n"]),
    ("SEED", selected_row["seed"]),
    ("SHUFFLE_LABELS", selected_row["shuffle_labels"]),
    ("SHUFFLE_QUERIES", selected_row["shuffle_queries"]),
    ("TIMEOUT_S", selected_row["timeout_s"]),
):
    print(f"{key}={shlex.quote(value)}")
PY
  )"
  if [[ -z "$parsed" ]]; then
    fail "failed to resolve smoke case selector: $CASE_SELECTOR"
  fi
  eval "$parsed"
}

resolve_output_root() {
  OUTROOT="$(python3 "$ARTIFACT_RESOLVER" lca_smoke_target "$ARTIFACT_SUBPATH")"
  if [[ -z "$OUTROOT" ]]; then
    fail "artifact resolver returned an empty smoke target output path"
  fi
  ensure_under_artifacts "$OUTROOT"
}

prepare_output_root() {
  if [[ -e "$OUTROOT" ]]; then
    rm -rf "$OUTROOT"
  fi
  mkdir -p "$OUTROOT"
}

prepare_target_environment() {
  TARGET_ENV_ROOT="$TMP_PARENT/lca_smoke_target.${CASE_TAG}.env"
  TARGET_TMPDIR="$TARGET_ENV_ROOT/tmp"
  TARGET_HOME="$TARGET_ENV_ROOT/home"
  TARGET_XDG_CONFIG_HOME="$TARGET_ENV_ROOT/xdg_config"
  TARGET_XDG_CACHE_HOME="$TARGET_ENV_ROOT/xdg_cache"
  TARGET_XDG_STATE_HOME="$TARGET_ENV_ROOT/xdg_state"
  TARGET_PYCACHE_ROOT="$TARGET_ENV_ROOT/pycache"

  ensure_under_artifacts "$TARGET_ENV_ROOT"
  ensure_under_artifacts "$TARGET_TMPDIR"
  ensure_under_artifacts "$TARGET_HOME"
  ensure_under_artifacts "$TARGET_XDG_CONFIG_HOME"
  ensure_under_artifacts "$TARGET_XDG_CACHE_HOME"
  ensure_under_artifacts "$TARGET_XDG_STATE_HOME"
  ensure_under_artifacts "$TARGET_PYCACHE_ROOT"

  if [[ -e "$TARGET_ENV_ROOT" ]]; then
    rm -rf "$TARGET_ENV_ROOT"
  fi

  mkdir -p \
    "$TARGET_TMPDIR" \
    "$TARGET_HOME" \
    "$TARGET_XDG_CONFIG_HOME" \
    "$TARGET_XDG_CACHE_HOME" \
    "$TARGET_XDG_STATE_HOME" \
    "$TARGET_PYCACHE_ROOT"

  export LC_ALL=C
  export LANG=C
  export TZ=UTC
  export PYTHONUTF8=1
  export PYTHONNOUSERSITE=1
  export PYTHONHASHSEED=0
  export BRANCH_ARTIFACT_TMP_ROOT="$TARGET_TMPDIR"
  export TMPDIR="$TARGET_TMPDIR"
  export TMP="$TARGET_TMPDIR"
  export TEMP="$TARGET_TMPDIR"
  export HOME="$TARGET_HOME"
  export XDG_CONFIG_HOME="$TARGET_XDG_CONFIG_HOME"
  export XDG_CACHE_HOME="$TARGET_XDG_CACHE_HOME"
  export XDG_STATE_HOME="$TARGET_XDG_STATE_HOME"
  export PYTHONPYCACHEPREFIX="$TARGET_PYCACHE_ROOT"
}

assert_target_environment() {
  local env_name=""
  for env_name in \
    BRANCH_ARTIFACT_TMP_ROOT \
    TMPDIR \
    TMP \
    TEMP \
    HOME \
    XDG_CONFIG_HOME \
    XDG_CACHE_HOME \
    XDG_STATE_HOME \
    PYTHONPYCACHEPREFIX; do
    if [[ -z "${!env_name:-}" ]]; then
      fail "required deterministic environment variable is unset: $env_name"
    fi
    ensure_under_artifacts "${!env_name}"
  done

  [[ "$BRANCH_ARTIFACT_TMP_ROOT" == "$TARGET_TMPDIR" ]] || fail "BRANCH_ARTIFACT_TMP_ROOT drifted from target tmpdir"
  [[ "$TMPDIR" == "$TARGET_TMPDIR" ]] || fail "TMPDIR drifted from target tmpdir"
  [[ "$TMP" == "$TARGET_TMPDIR" ]] || fail "TMP drifted from target tmpdir"
  [[ "$TEMP" == "$TARGET_TMPDIR" ]] || fail "TEMP drifted from target tmpdir"
  [[ "$HOME" == "$TARGET_HOME" ]] || fail "HOME drifted from target session root"
  [[ "$XDG_CONFIG_HOME" == "$TARGET_XDG_CONFIG_HOME" ]] || fail "XDG_CONFIG_HOME drifted from target session root"
  [[ "$XDG_CACHE_HOME" == "$TARGET_XDG_CACHE_HOME" ]] || fail "XDG_CACHE_HOME drifted from target session root"
  [[ "$XDG_STATE_HOME" == "$TARGET_XDG_STATE_HOME" ]] || fail "XDG_STATE_HOME drifted from target session root"
  [[ "$PYTHONPYCACHEPREFIX" == "$TARGET_PYCACHE_ROOT" ]] || fail "PYTHONPYCACHEPREFIX drifted from target session root"
}

write_target_metadata() {
  local case_dir="$1"
  local command_text="$2"

  {
    printf 'manifest_path\t%s\n' "$SMOKE_CASES_ACTIVE"
    printf 'case_index\tcase_tag\tstage\tmode\tn\tseed\tshuffle_labels\tshuffle_queries\ttimeout_s\n'
    printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
      "$CASE_INDEX" \
      "$CASE_TAG" \
      "$STAGE" \
      "$MODE" \
      "$N" \
      "$SEED" \
      "$SHUFFLE_LABELS" \
      "$SHUFFLE_QUERIES" \
      "$TIMEOUT_S"
  } > "$OUTROOT/selected_case.tsv"

  printf '%s\n' "$command_text" > "$case_dir/run_command.txt"
  printf '%s\n' "$command_text" > "$OUTROOT/latest_run_command.txt"
}

main() {
  local case_dir=""
  local build_stdout=""
  local build_stderr=""
  local rc=0
  local -a cmd
  local command_text=""

  parse_args "$@"
  trap 'cleanup "$?"' EXIT

  require_command bash
  require_command python3
  require_file "$ARTIFACT_RESOLVER" "artifact resolver"
  require_file "$RELEASE_ENV" "release env wrapper"
  require_file "$RUN_CASE_HELPER" "branch-local case helper"
  require_file "$SMOKE_CASES_SOURCE" "smoke case manifest"
  require_executable "$BUILD_WRAPPER" "build wrapper"

  if (( LIST_ONLY )); then
    resolve_manifest_path
    list_manifest_cases
    return 0
  fi

  resolve_manifest_path
  resolve_selected_case
  resolve_output_root
  prepare_output_root
  prepare_target_environment
  source "$RELEASE_ENV"
  assert_target_environment

  build_stdout="$OUTROOT/build.stdout.txt"
  build_stderr="$OUTROOT/build.stderr.txt"
  set +e
  "$BUILD_WRAPPER" >"$build_stdout" 2>"$build_stderr"
  rc=$?
  set -e
  if (( rc != 0 )); then
    fail "build failed with exit code $rc; inspect $build_stdout and $build_stderr"
  fi
  if [[ ! -x "$SOLVER" ]]; then
    fail "build completed without producing executable solver: $SOLVER"
  fi

  case_dir="$OUTROOT/$CASE_TAG"
  ensure_under_artifacts "$case_dir"
  rm -rf "$case_dir"
  mkdir -p "$case_dir"

  cmd=(
    python3
    "$RUN_CASE_HELPER"
    "$MODE"
    "$N"
    "$SEED"
    "$SHUFFLE_LABELS"
    "$SHUFFLE_QUERIES"
    "$SOLVER"
    "$case_dir"
    --timeout
    "$TIMEOUT_S"
    --env
    "DENSE_SHADOW_CASE_MODE=$MODE"
    --env
    "DENSE_SHADOW_CASE_N=$N"
    --env
    "DENSE_SHADOW_CASE_SEED=$SEED"
    --env
    "DENSE_PROFILE_OUTDIR=$case_dir"
    --env
    DENSE_DECOMPOSESERIES_ROUND45_SHADOWCHECK=1
  )
  command_text="$(format_replay_command "${cmd[@]}")"

  write_target_metadata "$case_dir" "$command_text"

  set +e
  "${cmd[@]}" >"$case_dir/run_case.stdout.txt" 2>"$case_dir/run_case.stderr.txt"
  rc=$?
  set -e
  printf '%s\n' "$rc" > "$case_dir/exit_code.txt"

  echo "[lca_smoke_target] selector=$CASE_SELECTOR case_tag=$CASE_TAG out=$case_dir rc=$rc" >&2
  if (( rc != 0 )); then
    echo "[lca_smoke_target] failed; inspect $case_dir/run_case.stderr.txt and $case_dir/run_case_result.json" >&2
    return "$rc"
  fi
  echo "[lca_smoke_target] PASS case_tag=$CASE_TAG" >&2
}

main "$@"
