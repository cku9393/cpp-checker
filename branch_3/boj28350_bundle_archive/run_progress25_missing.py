#!/usr/bin/env python3
import os, sys, subprocess, json, time, pathlib, re
ROOT = pathlib.Path('/mnt/data/lca_tree_stress_v5')
GEN = ROOT / 'gen_case.py'
VAL = ROOT / 'validator.py'
SOLVER = pathlib.Path('/mnt/data/p25_release')
BASE_ENV = {
    'PROFILE_MODE': 'PROFILE_BASE',
    'PROFILE_PROGRESS_STRIDE': '16',
    'ENABLE_DELTA_PRESERVED_HIT': '1',
    'ENABLE_DELTA_CONNECTOR_HIT': '1',
    'ENABLE_REUSE_APPLY_OPT': '1',
    'ENABLE_PRESERVED_SPLIT_OPT': '1',
    'ENABLE_WATCH_SCAN_OPT': '1',
    'ENABLE_RETAIN_COMPACTION_OPT': '1',
    'ENABLE_KEPT_VECTOR_OPT': '1',
    'ENABLE_STABLE_COMPACTION_OPT': '1',
    'ENABLE_BLOCK_COPY_COMPACTION_OPT': '1',
    'ENABLE_COPY_PLAN_BUILD_OPT': '1',
    'ENABLE_RUN_DISCOVERY_FUSION_OPT': '1',
    'ENABLE_FUSED_DISCOVERY_CLASSIFY_OPT': '1',
    'ENABLE_TSCAN_CORE_OPT': '1',
    'ENABLE_TSCAN_BRANCH_STATE_OPT': '1',
    'ENABLE_STATE_LOAD_MATERIALIZATION_OPT': '1',
    'ENABLE_PREV_STATE_CARRY_REUSE_OPT': '1',
    'ENABLE_CARRY_REUSE_FASTPATH_OPT': '1',
    'ENABLE_CARRY_HIT_APPLY_OPT': '1',
}
CASES = [
    ('after_both_on_dense_1024_release', 'comb_rect_dense', 1024, 1, 2000),
    ('after_both_on_dense_1024_release_repeat', 'comb_rect_dense', 1024, 1, 2000),
    ('after_both_on_dense_4096_release', 'comb_rect_dense', 4096, 1, 480),
    ('after_both_on_multi_4096_release', 'multi_comb_rect', 4096, 1, 480),
]
KV_RE = re.compile(r'([A-Za-z0-9_./:-]+)=([^\s]+)')
def parse_all_kv(text):
    return {m.group(1): m.group(2) for m in KV_RE.finditer(text)}

def run_case(tag, mode, n, seed, timeout_sec):
    outdir = pathlib.Path('/mnt/data/progress25_runs') / tag
    outdir.mkdir(parents=True, exist_ok=True)
    in_path = outdir/'in.txt'
    out_path = outdir/'out.txt'
    err_path = outdir/'stderr.txt'
    time_path = outdir/'time.txt'
    meta_path = outdir/'meta.json'
    parent_path = outdir/'hidden_parent.txt'
    result_path = outdir/'result.json'
    if result_path.exists():
        print(f'SKIP existing {tag}')
        return json.loads(result_path.read_text())
    with open(in_path,'w') as f:
        subprocess.run([sys.executable, str(GEN), '--mode', mode, '--n', str(n), '--seed', str(seed), '--meta', str(meta_path), '--parent-out', str(parent_path)], check=True, stdout=f)
    env = os.environ.copy(); env.update(BASE_ENV)
    cmd = ['/usr/bin/time','-f','%e %M','-o',str(time_path), str(SOLVER)]
    print(f'START {tag} mode={mode} n={n} timeout={timeout_sec}', flush=True)
    start=time.time(); timed_out=False
    with open(in_path,'rb') as fin, open(out_path,'wb') as fout, open(err_path,'wb') as ferr:
        proc = subprocess.Popen(cmd, stdin=fin, stdout=fout, stderr=ferr, env=env)
        try:
            rc = proc.wait(timeout=timeout_sec)
        except subprocess.TimeoutExpired:
            timed_out=True
            proc.kill()
            rc = proc.wait()
    elapsed_wall = time.time()-start
    stdout_empty = (not out_path.exists()) or out_path.stat().st_size==0
    stderr_empty = (not err_path.exists()) or err_path.stat().st_size==0
    val_ok=False; val_msg=''
    if not stdout_empty:
        val = subprocess.run([sys.executable, str(VAL), str(in_path), str(out_path)], capture_output=True, text=True)
        val_ok = (val.returncode==0)
        val_msg = val.stdout + val.stderr
    tsec=None; rss=None
    if time_path.exists() and time_path.stat().st_size>0:
        parts = time_path.read_text().strip().split()
        if len(parts) >= 2:
            try:
                tsec=float(parts[0]); rss=int(parts[1])
            except:
                pass
    stderr_text = err_path.read_text(errors='ignore') if err_path.exists() else ''
    res = {
        'run_tag': tag,
        'mode': mode,
        'n': n,
        'seed': seed,
        'profile_mode': 'PROFILE_BASE',
        'delta_mode': 'both_on',
        'rc': rc,
        'timed_out': timed_out,
        'validator_ok': val_ok,
        'validator_msg': val_msg,
        'stdout_empty': stdout_empty,
        'stderr_empty': stderr_empty,
        'elapsed_sec': tsec if tsec is not None else round(elapsed_wall,3),
        'maxrss_kb': rss,
        'summary_kv': parse_all_kv(stderr_text),
    }
    result_path.write_text(json.dumps(res, indent=2, ensure_ascii=False))
    print(json.dumps({'run_tag': tag, 'rc': rc, 'timed_out': timed_out, 'validator_ok': val_ok, 'elapsed_sec': res['elapsed_sec']}, ensure_ascii=False), flush=True)
    return res

allres=[]
for case in CASES:
    allres.append(run_case(*case))
print('DONE all missing cases', flush=True)
