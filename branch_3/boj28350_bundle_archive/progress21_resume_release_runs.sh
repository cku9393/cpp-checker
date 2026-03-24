#!/usr/bin/env bash
set -euo pipefail
cd /mnt/data
rm -rf /mnt/data/lca_tree_stress_v5
unzip -q /mnt/data/lca_tree_stress_v5.zip -d /mnt/data

g++ -O2 -std=gnu++17 -pipe /mnt/data/boj28350_literature_progress21_transition_state_branch_state_load_core.cpp -o /mnt/data/boj28350_progress21_release

cat > /mnt/data/run_progress21_case_heartbeat.py <<'PY'
#!/usr/bin/env python3
import argparse, json, os, re, subprocess, sys, time
from pathlib import Path
ROOT = Path('/mnt/data/lca_tree_stress_v5')
GEN = ROOT / 'gen_case.py'
VAL = ROOT / 'validator.py'
TIME_BIN = '/usr/bin/time'
KV_RE = re.compile(r'([A-Za-z0-9_./:-]+)=([^\s]+)')

def parse_summary(stderr_text: str):
    summary_line = None
    for line in stderr_text.splitlines():
        if 'phase=summary' in line:
            summary_line = line
    target = summary_line or stderr_text
    kv = {}
    for m in KV_RE.finditer(target):
        kv[m.group(1)] = m.group(2)
    return kv
ap = argparse.ArgumentParser()
ap.add_argument('--solver', required=True)
ap.add_argument('--run-tag', required=True)
ap.add_argument('--mode', required=True)
ap.add_argument('--n', type=int, required=True)
ap.add_argument('--seed', type=int, default=1)
ap.add_argument('--profile-mode', default='PROFILE_BASE')
ap.add_argument('--delta-mode', choices=['connector_only','both_on'], required=True)
ap.add_argument('--outdir', required=True)
ap.add_argument('--timeout-sec', type=float, default=0.0)
ap.add_argument('--release-diag', action='store_true')
ap.add_argument('--env', action='append', default=[])
args = ap.parse_args()
outdir = Path(args.outdir); outdir.mkdir(parents=True, exist_ok=True)
in_path = outdir/'in.txt'; out_path = outdir/'out.txt'; err_path = outdir/'stderr.txt'; time_path = outdir/'time.txt'; meta_path = outdir/'meta.json'; hidden_parent = outdir/'hidden_parent.txt'; result_path = outdir/'result.json'
with open(in_path,'w') as f:
    subprocess.run([sys.executable,str(GEN),'--mode',args.mode,'--n',str(args.n),'--seed',str(args.seed),'--meta',str(meta_path),'--parent-out',str(hidden_parent)],check=True,stdout=f)
env = os.environ.copy(); env['PROFILE_MODE']=args.profile_mode; env['RUN_TAG']=args.run_tag; env['PROFILE_PROGRESS_STRIDE']=env.get('PROFILE_PROGRESS_STRIDE','16')
if args.delta_mode=='connector_only': env['ENABLE_DELTA_PRESERVED_HIT']='0'; env['ENABLE_DELTA_CONNECTOR_HIT']='1'
else: env['ENABLE_DELTA_PRESERVED_HIT']='1'; env['ENABLE_DELTA_CONNECTOR_HIT']='1'
if args.release_diag: env['ENABLE_COMPACT_RELEASE_DIAG']='1'
for item in args.env:
    k,v=item.split('=',1); env[k]=v
cmd=[TIME_BIN,'-f','%e %M','-o',str(time_path),args.solver]
print(json.dumps({'event':'start','run_tag':args.run_tag,'mode':args.mode,'n':args.n,'delta_mode':args.delta_mode,'profile_mode':args.profile_mode,'timeout_sec':args.timeout_sec}), flush=True)
start=time.time(); rc=None; timed_out=False
with open(in_path,'rb') as fin, open(out_path,'wb') as fout, open(err_path,'wb') as ferr:
    proc=subprocess.Popen(cmd,stdin=fin,stdout=fout,stderr=ferr,env=env)
    last=0.0
    while True:
        rc=proc.poll(); now=time.time()
        if rc is not None: break
        if args.timeout_sec>0 and now-start>args.timeout_sec:
            timed_out=True; proc.kill(); rc=proc.wait(timeout=10); break
        if now-last>=25.0:
            last=now
            print(json.dumps({'event':'heartbeat','run_tag':args.run_tag,'elapsed_sec':round(now-start,1),'out_size':out_path.stat().st_size if out_path.exists() else 0,'err_size':err_path.stat().st_size if err_path.exists() else 0}), flush=True)
        time.sleep(1)
wall=round(time.time()-start,3)
validator_ok=False; validator_msg=''
if out_path.exists() and out_path.stat().st_size>0:
    val=subprocess.run([sys.executable,str(VAL),str(in_path),str(out_path)],capture_output=True,text=True)
    validator_ok=(val.returncode==0); validator_msg=val.stdout+val.stderr
elapsed_sec=None; maxrss_kb=None
if time_path.exists() and time_path.stat().st_size>0:
    parts=time_path.read_text().strip().split()
    if len(parts)>=2:
        try: elapsed_sec=float(parts[0]); maxrss_kb=int(parts[1])
        except Exception: pass
stderr_text=err_path.read_text(errors='ignore') if err_path.exists() else ''
summary_kv=parse_summary(stderr_text)
result={'run_tag':args.run_tag,'mode':args.mode,'n':args.n,'seed':args.seed,'solver':args.solver,'profile_mode':args.profile_mode,'delta_mode':args.delta_mode,'release_diag':args.release_diag,'rc':rc,'timed_out':timed_out,'validator_ok':validator_ok,'validator_msg':validator_msg,'stdout_empty':(not out_path.exists()) or out_path.stat().st_size==0,'stderr_empty':(not err_path.exists()) or err_path.stat().st_size==0,'elapsed_sec':elapsed_sec if elapsed_sec is not None else wall,'maxrss_kb':maxrss_kb,'summary_kv':summary_kv}
result_path.write_text(json.dumps(result,indent=2,ensure_ascii=False))
print(json.dumps({'event':'done','run_tag':args.run_tag,'rc':rc,'timed_out':timed_out,'validator_ok':validator_ok,'elapsed_sec':result['elapsed_sec']}), flush=True)
PY
chmod +x /mnt/data/run_progress21_case_heartbeat.py

COMMON=(
  --env ENABLE_REUSE_APPLY_OPT=1
  --env ENABLE_PRESERVED_SPLIT_OPT=1
  --env ENABLE_WATCH_SCAN_OPT=1
  --env ENABLE_RETAIN_COMPACTION_OPT=1
  --env ENABLE_KEPT_VECTOR_OPT=1
  --env ENABLE_STABLE_COMPACTION_OPT=1
  --env ENABLE_BLOCK_COPY_COMPACTION_OPT=1
  --env ENABLE_COPY_PLAN_BUILD_OPT=1
  --env ENABLE_RUN_DISCOVERY_FUSION_OPT=1
  --env ENABLE_FUSED_DISCOVERY_CLASSIFY_OPT=1
  --env ENABLE_TSCAN_CORE_OPT=1
  --env ENABLE_TSCAN_BRANCH_STATE_OPT=1
)

mkdir -p /mnt/data/progress21_runs
python /mnt/data/run_progress21_case_heartbeat.py --solver /mnt/data/boj28350_progress21_release --run-tag p21_after_both_on_dense_1024_release_clean --mode comb_rect_dense --n 1024 --profile-mode PROFILE_BASE --delta-mode both_on --outdir /mnt/data/progress21_runs/p21_after_both_on_dense_1024_release_clean --timeout-sec 1500 "${COMMON[@]}"
python /mnt/data/run_progress21_case_heartbeat.py --solver /mnt/data/boj28350_progress21_release --run-tag p21_after_both_on_dense_1024_release_repeat --mode comb_rect_dense --n 1024 --profile-mode PROFILE_BASE --delta-mode both_on --outdir /mnt/data/progress21_runs/p21_after_both_on_dense_1024_release_repeat --timeout-sec 1500 "${COMMON[@]}"
python /mnt/data/run_progress21_case_heartbeat.py --solver /mnt/data/boj28350_progress21_release --run-tag p21_after_both_on_dense_4096_release --mode comb_rect_dense --n 4096 --profile-mode PROFILE_BASE --delta-mode both_on --outdir /mnt/data/progress21_runs/p21_after_both_on_dense_4096_release --timeout-sec 420 "${COMMON[@]}"
python /mnt/data/run_progress21_case_heartbeat.py --solver /mnt/data/boj28350_progress21_release --run-tag p21_after_both_on_multi_4096_release --mode multi_comb_rect --n 4096 --profile-mode PROFILE_BASE --delta-mode both_on --outdir /mnt/data/progress21_runs/p21_after_both_on_multi_4096_release --timeout-sec 420 "${COMMON[@]}"
