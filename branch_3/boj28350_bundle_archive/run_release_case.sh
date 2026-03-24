#!/usr/bin/env bash
set -euo pipefail
solver="$1"
run_tag="$2"
mode="$3"
n="$4"
seed="$5"
timeout_sec="$6"
outdir="$7"
mkdir -p "$outdir"
ROOT=/mnt/data/lca_tree_stress_v5
GEN="$ROOT/gen_case.py"
VAL="$ROOT/validator.py"
IN="$outdir/in.txt"
OUT="$outdir/out.txt"
ERR="$outdir/stderr.txt"
TIMEF="$outdir/time.txt"
META="$outdir/meta.json"
PARENT="$outdir/hidden_parent.txt"
RESULT="$outdir/result.json"
python "$GEN" --mode "$mode" --n "$n" --seed "$seed" --meta "$META" --parent-out "$PARENT" > "$IN"
rm -f "$OUT" "$ERR" "$TIMEF" "$RESULT"
export PROFILE_MODE=PROFILE_BASE PROFILE_PROGRESS_STRIDE=16 ENABLE_DELTA_PRESERVED_HIT=1 ENABLE_DELTA_CONNECTOR_HIT=1
export ENABLE_REUSE_APPLY_OPT=1 ENABLE_PRESERVED_SPLIT_OPT=1 ENABLE_WATCH_SCAN_OPT=1 ENABLE_RETAIN_COMPACTION_OPT=1 ENABLE_KEPT_VECTOR_OPT=1 ENABLE_STABLE_COMPACTION_OPT=1 ENABLE_BLOCK_COPY_COMPACTION_OPT=1 ENABLE_COPY_PLAN_BUILD_OPT=1 ENABLE_RUN_DISCOVERY_FUSION_OPT=1 ENABLE_FUSED_DISCOVERY_CLASSIFY_OPT=1 ENABLE_TSCAN_CORE_OPT=1 ENABLE_TSCAN_BRANCH_STATE_OPT=1 ENABLE_STATE_LOAD_MATERIALIZATION_OPT=1 ENABLE_PREV_STATE_CARRY_REUSE_OPT=1 ENABLE_CARRY_REUSE_FASTPATH_OPT=1 ENABLE_CARRY_HIT_APPLY_OPT=1
start=$(python - <<'PY'
import time; print(time.time())
PY
)
set +e
/usr/bin/time -f '%e %M' -o "$TIMEF" timeout ${timeout_sec}s "$solver" < "$IN" > "$OUT" 2> "$ERR"
rc=$?
set -e
end=$(python - <<'PY'
import time; print(time.time())
PY
)
elapsed=$(python - <<PY
print(round(float('$end')-float('$start'),3))
PY
)
stdout_empty=true; stderr_empty=true
[[ -s "$OUT" ]] && stdout_empty=false
[[ -s "$ERR" ]] && stderr_empty=false
validator_ok=false
validator_msg=""
if [[ -s "$OUT" ]]; then
  set +e
  python "$VAL" "$IN" "$OUT" > "$outdir/validator_stdout.txt" 2> "$outdir/validator_stderr.txt"
  vrc=$?
  set -e
  if [[ $vrc -eq 0 ]]; then validator_ok=true; fi
  validator_msg=$(cat "$outdir/validator_stdout.txt" "$outdir/validator_stderr.txt")
fi
elapsed_sec=null
maxrss_kb=null
if [[ -s "$TIMEF" ]]; then
  read -r elapsed_sec maxrss_kb < "$TIMEF" || true
fi
python - <<PY
import json,re,pathlib
kv_re=re.compile(r'([A-Za-z0-9_./:-]+)=([^\s]+)')
stderr_text=pathlib.Path('$ERR').read_text(errors='ignore') if pathlib.Path('$ERR').exists() else ''
summary={m.group(1):m.group(2) for m in kv_re.finditer(stderr_text)}
obj={
 'run_tag':'$run_tag','mode':'$mode','n':int('$n'),'seed':int('$seed'),
 'profile_mode':'PROFILE_BASE','delta_mode':'both_on',
 'rc':int('$rc'),'timed_out': bool(int('$rc')==124),
 'validator_ok': True if '$validator_ok'=='true' else False,
 'validator_msg':'''$validator_msg''',
 'stdout_empty': True if '$stdout_empty'=='true' else False,
 'stderr_empty': True if '$stderr_empty'=='true' else False,
 'elapsed_sec': (float('$elapsed_sec') if '$elapsed_sec' not in ('','null') else float('$elapsed')),
 'maxrss_kb': (int('$maxrss_kb') if '$maxrss_kb' not in ('','null') else None),
 'summary_kv': summary,
}
pathlib.Path('$RESULT').write_text(json.dumps(obj,indent=2,ensure_ascii=False))
print(json.dumps({'run_tag':obj['run_tag'],'rc':obj['rc'],'timed_out':obj['timed_out'],'validator_ok':obj['validator_ok'],'elapsed_sec':obj['elapsed_sec']},ensure_ascii=False))
PY
