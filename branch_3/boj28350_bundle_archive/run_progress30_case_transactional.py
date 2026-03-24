#!/usr/bin/env python3
import argparse, json, os, re, signal, subprocess, sys, threading, time
from pathlib import Path
ROOT = Path('/mnt/data/lca_tree_stress_v5')
GEN = ROOT / 'gen_case.py'
VAL = ROOT / 'validator.py'
TIME_BIN = '/usr/bin/time'
KV_RE = re.compile(r'([A-Za-z0-9_./:-]+)=([^\s]+)')


def parse_all_kv(stderr_text: str):
    kv = {}
    for m in KV_RE.finditer(stderr_text):
        kv[m.group(1)] = m.group(2)
    return kv


def atomic_write_json(path: Path, obj):
    tmp = path.with_suffix(path.suffix + '.tmp')
    tmp.write_text(json.dumps(obj, indent=2, ensure_ascii=False))
    tmp.replace(path)


def append_journal(path: Path, obj):
    with path.open('a', encoding='utf-8') as f:
        f.write(json.dumps(obj, ensure_ascii=False) + '\n')


def make_env(args):
    env = os.environ.copy()
    env['PROFILE_MODE'] = args.profile_mode
    env['RUN_TAG'] = args.run_tag
    env.setdefault('PROFILE_PROGRESS_STRIDE', '16')
    if args.delta_mode == 'connector_only':
        env['ENABLE_DELTA_PRESERVED_HIT'] = '0'; env['ENABLE_DELTA_CONNECTOR_HIT'] = '1'
    else:
        env['ENABLE_DELTA_PRESERVED_HIT'] = '1'; env['ENABLE_DELTA_CONNECTOR_HIT'] = '1'
    for item in args.env:
        k, v = item.split('=', 1); env[k] = v
    return env


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--solver', required=True)
    ap.add_argument('--run-tag', required=True)
    ap.add_argument('--mode', required=True)
    ap.add_argument('--n', type=int, required=True)
    ap.add_argument('--seed', type=int, default=1)
    ap.add_argument('--profile-mode', default='PROFILE_BASE')
    ap.add_argument('--delta-mode', choices=['connector_only', 'both_on'], required=True)
    ap.add_argument('--outdir', required=True)
    ap.add_argument('--timeout-sec', type=float, default=0.0)
    ap.add_argument('--env', action='append', default=[])
    ap.add_argument('--journal', default='/mnt/data/progress30_case_journal.jsonl')
    args = ap.parse_args()

    outdir = Path(args.outdir)
    outdir.mkdir(parents=True, exist_ok=True)
    in_path = outdir / 'in.txt'
    out_path = outdir / 'stdout.txt'
    err_path = outdir / 'stderr.txt'
    time_path = outdir / 'time.txt'
    meta_path = outdir / 'meta.json'
    parent_path = outdir / 'hidden_parent.txt'
    result_path = outdir / 'result.json'
    status_path = outdir / 'status.json'
    heartbeat_path = outdir / 'heartbeat.json'
    runner_log = outdir / 'runner.log'
    journal_path = Path(args.journal)

    # ensure suite exists
    if not ROOT.exists():
        raise SystemExit(f'missing stress suite dir: {ROOT}')

    with open(in_path, 'w') as f:
        subprocess.run([sys.executable, str(GEN), '--mode', args.mode, '--n', str(args.n), '--seed', str(args.seed), '--meta', str(meta_path), '--parent-out', str(parent_path)], check=True, stdout=f)

    env = make_env(args)
    cmd = [TIME_BIN, '-f', '%e %M', '-o', str(time_path), args.solver]

    start = time.time()
    status = {
        'case_name': args.run_tag,
        'phase': 'starting',
        'cmd': cmd,
        'start_time': start,
        'state': 'starting',
        'pid': None,
    }
    atomic_write_json(status_path, status)
    append_journal(journal_path, {'event': 'start', **status})

    stop_hb = False
    proc = None

    def heartbeat_loop():
        last_label = None
        last_del = None
        while not stop_hb:
            try:
                stderr_text = err_path.read_text(errors='ignore') if err_path.exists() else ''
                labels = re.findall(r'(case start|initialization complete|summary|checkpoint[^\n]*)', stderr_text)
                dels = re.findall(r'debug_progress_last_deletion=([0-9-]+)', stderr_text)
                if labels:
                    last_label = labels[-1]
                if dels:
                    last_del = int(dels[-1])
                hb = {
                    'last_checkpoint_label': last_label,
                    'last_deletion_index': last_del,
                    'last_update_time': time.time(),
                    'stderr_size': err_path.stat().st_size if err_path.exists() else 0,
                    'stdout_size': out_path.stat().st_size if out_path.exists() else 0,
                }
                atomic_write_json(heartbeat_path, hb)
            except Exception as e:
                with runner_log.open('a', encoding='utf-8') as f:
                    f.write(f'heartbeat_error {e}\n')
            time.sleep(5)

    with open(in_path, 'rb') as fin, open(out_path, 'wb') as fout, open(err_path, 'wb') as ferr:
        proc = subprocess.Popen(cmd, stdin=fin, stdout=fout, stderr=ferr, env=env)
        status['phase'] = 'running'; status['state'] = 'running'; status['pid'] = proc.pid
        atomic_write_json(status_path, status)
        append_journal(journal_path, {'event': 'running', **status})
        hb_thread = threading.Thread(target=heartbeat_loop, daemon=True)
        hb_thread.start()
        timed_out = False
        interrupted = False
        rc = None
        try:
            rc = proc.wait(timeout=args.timeout_sec if args.timeout_sec > 0 else None)
        except subprocess.TimeoutExpired:
            timed_out = True
            proc.kill(); rc = proc.wait()
        except KeyboardInterrupt:
            interrupted = True
            proc.kill(); rc = proc.wait()
        finally:
            stop_hb = True
            hb_thread.join(timeout=2)

    elapsed = round(time.time() - start, 3)
    stdout_empty = (not out_path.exists()) or out_path.stat().st_size == 0
    stderr_empty = (not err_path.exists()) or err_path.stat().st_size == 0
    val_ok = False
    val_msg = ''
    if out_path.exists() and out_path.stat().st_size > 0:
        val = subprocess.run([sys.executable, str(VAL), str(in_path), str(out_path)], capture_output=True, text=True)
        val_ok = (val.returncode == 0)
        val_msg = val.stdout + val.stderr
    tsec = None; rss = None
    if time_path.exists() and time_path.stat().st_size > 0:
        parts = time_path.read_text().strip().split()
        if len(parts) >= 2:
            try:
                tsec = float(parts[0]); rss = int(parts[1])
            except Exception:
                pass
    stderr_text = err_path.read_text(errors='ignore') if err_path.exists() else ''
    summary = parse_all_kv(stderr_text)
    hb = json.loads(heartbeat_path.read_text()) if heartbeat_path.exists() else {}

    result = {
        'run_tag': args.run_tag,
        'mode': args.mode,
        'n': args.n,
        'seed': args.seed,
        'profile_mode': args.profile_mode,
        'delta_mode': args.delta_mode,
        'rc': rc,
        'timed_out': timed_out,
        'interrupted': interrupted,
        'validator_ok': val_ok,
        'validator_msg': val_msg,
        'stdout_empty': stdout_empty,
        'stderr_empty': stderr_empty,
        'elapsed_sec': tsec if tsec is not None else elapsed,
        'maxrss_kb': rss,
        'summary_kv': summary,
        'heartbeat': hb,
    }
    atomic_write_json(result_path, result)
    status['phase'] = 'finished'
    status['state'] = 'finished'
    atomic_write_json(status_path, status)
    append_journal(journal_path, {'event': 'finished', **status, 'rc': rc, 'timed_out': timed_out, 'validator_ok': val_ok, 'elapsed_sec': result['elapsed_sec']})
    print(json.dumps({'run_tag': args.run_tag, 'rc': rc, 'timed_out': timed_out, 'validator_ok': val_ok, 'elapsed_sec': result['elapsed_sec']}, ensure_ascii=False))

if __name__ == '__main__':
    main()
