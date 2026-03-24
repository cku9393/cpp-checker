#!/usr/bin/env python3
import argparse, json, os, re, subprocess, sys, threading, time, signal
from pathlib import Path

ROOT = Path('/mnt/data/lca_tree_stress_v5')
GEN = ROOT / 'gen_case.py'
VAL = ROOT / 'validator.py'
KV_RE = re.compile(r'([A-Za-z0-9_./:-]+)=([^\s]+)')


def parse_all_kv(stderr_text: str):
    kv = {}
    for m in KV_RE.finditer(stderr_text):
        kv[m.group(1)] = m.group(2)
    return kv


def atomic_write_json(path: Path, obj):
    tmp = path.with_suffix(path.suffix + '.tmp')
    data = json.dumps(obj, indent=2, ensure_ascii=False)
    with tmp.open('w', encoding='utf-8') as f:
        f.write(data)
        f.flush()
        os.fsync(f.fileno())
    tmp.replace(path)


def append_journal(path: Path, obj):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open('a', encoding='utf-8') as f:
        f.write(json.dumps(obj, ensure_ascii=False) + '\n')
        f.flush()
        os.fsync(f.fileno())


def make_env(args):
    env = os.environ.copy()
    env['PROFILE_MODE'] = args.profile_mode
    env['RUN_TAG'] = args.run_tag
    env.setdefault('PROFILE_PROGRESS_STRIDE', '16')
    env.setdefault('LOCAL_SKIP_SELF_TEST', '1')
    if args.delta_mode == 'connector_only':
        env['ENABLE_DELTA_PRESERVED_HIT'] = '0'; env['ENABLE_DELTA_CONNECTOR_HIT'] = '1'
    else:
        env['ENABLE_DELTA_PRESERVED_HIT'] = '1'; env['ENABLE_DELTA_CONNECTOR_HIT'] = '1'
    for item in args.env:
        k, v = item.split('=', 1); env[k] = v
    return env


def generate_input_if_needed(in_path: Path, meta_path: Path, parent_path: Path, mode: str, n: int, seed: int):
    if (not in_path.exists()) or in_path.stat().st_size == 0 or (not meta_path.exists()):
        with open(in_path, 'w') as f:
            subprocess.run([sys.executable, str(GEN), '--mode', mode, '--n', str(n), '--seed', str(seed), '--meta', str(meta_path), '--parent-out', str(parent_path)], check=True, stdout=f)


def load_json(p: Path):
    if not p.exists():
        return {}
    try:
        return json.loads(p.read_text())
    except Exception:
        return {}


def finalize_case_dir(outdir: Path, journal_path: Path, elapsed_override=None, finalize_reason='finalize_only'):
    status_path = outdir / 'status.json'
    heartbeat_path = outdir / 'heartbeat.json'
    result_path = outdir / 'result.json'
    stderr_path = outdir / 'stderr.txt'
    stdout_path = outdir / 'stdout.txt'
    in_path = outdir / 'in.txt'
    status = load_json(status_path)
    hb = load_json(heartbeat_path)
    stderr_text = stderr_path.read_text(errors='ignore') if stderr_path.exists() else ''
    summary = parse_all_kv(stderr_text)
    stdout_empty = (not stdout_path.exists()) or stdout_path.stat().st_size == 0
    stderr_empty = (not stderr_path.exists()) or stderr_path.stat().st_size == 0
    val_ok = False; val_msg = ''
    if in_path.exists() and stdout_path.exists() and stdout_path.stat().st_size > 0:
        val = subprocess.run([sys.executable, str(VAL), str(in_path), str(stdout_path)], capture_output=True, text=True)
        val_ok = (val.returncode == 0)
        val_msg = (val.stdout or '') + (val.stderr or '')
    result = {
        'run_tag': status.get('case_name', outdir.name),
        'mode': status.get('mode'),
        'n': status.get('n'),
        'seed': status.get('seed'),
        'profile_mode': status.get('profile_mode'),
        'delta_mode': status.get('delta_mode'),
        'rc': status.get('rc', -999),
        'timed_out': bool(status.get('timed_out', False)),
        'interrupted': bool(status.get('interrupted', False)),
        'validator_ok': val_ok,
        'validator_msg': val_msg if val_msg else finalize_reason,
        'stdout_empty': stdout_empty,
        'stderr_empty': stderr_empty,
        'elapsed_sec': elapsed_override if elapsed_override is not None else status.get('elapsed_sec'),
        'maxrss_kb': status.get('maxrss_kb'),
        'summary_kv': summary,
        'heartbeat': hb,
        'finalize_only_recovered': True,
    }
    atomic_write_json(result_path, result)
    status['state'] = 'finished'
    status['phase'] = 'finalized'
    if elapsed_override is not None:
        status['elapsed_sec'] = elapsed_override
    atomic_write_json(status_path, status)
    append_journal(journal_path, {'event': 'finalize_only', **status, 'validator_ok': val_ok})


def supervise(args):
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

    generate_input_if_needed(in_path, meta_path, parent_path, args.mode, args.n, args.seed)
    env = make_env(args)
    cmd = [args.solver]

    status = load_json(status_path)
    start_time = status.get('start_time', time.time())
    status.update({
        'case_name': args.run_tag,
        'mode': args.mode,
        'n': args.n,
        'seed': args.seed,
        'profile_mode': args.profile_mode,
        'delta_mode': args.delta_mode,
        'phase': 'running',
        'state': 'running',
        'supervisor_pid': os.getpid(),
        'cmd': cmd,
        'start_time': start_time,
    })
    atomic_write_json(status_path, status)
    append_journal(journal_path, {'event': 'running', **status})

    proc = None
    stop_hb = False

    def hb_loop():
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
                    'child_state': 'running' if proc and proc.poll() is None else 'stopped',
                }
                atomic_write_json(heartbeat_path, hb)
            except Exception as e:
                with runner_log.open('a', encoding='utf-8') as f:
                    f.write(f'heartbeat_error {e}\n')
            time.sleep(5)

    start = time.time()
    rc = None; timed_out = False; interrupted = False; maxrss_kb = None
    try:
        with open(in_path, 'rb') as fin, open(out_path, 'wb') as fout, open(err_path, 'wb') as ferr:
            proc = subprocess.Popen(cmd, stdin=fin, stdout=fout, stderr=ferr, env=env, start_new_session=True, close_fds=True)
            status['child_pid'] = proc.pid
            atomic_write_json(status_path, status)
            append_journal(journal_path, {'event': 'child_started', 'child_pid': proc.pid, 'case_name': args.run_tag})
            t = threading.Thread(target=hb_loop, daemon=True)
            t.start()
            try:
                rc = proc.wait(timeout=args.timeout_sec if args.timeout_sec > 0 else None)
            except subprocess.TimeoutExpired:
                timed_out = True
                os.killpg(proc.pid, signal.SIGKILL)
                rc = proc.wait()
            except KeyboardInterrupt:
                interrupted = True
                os.killpg(proc.pid, signal.SIGKILL)
                rc = proc.wait()
            finally:
                stop_hb = True
                t.join(timeout=2)
    except Exception as e:
        status['finalize_reason'] = f'launcher_exception: {e}'
        atomic_write_json(status_path, status)
        finalize_case_dir(outdir, journal_path, finalize_reason=status['finalize_reason'])
        raise

    elapsed_sec = round(time.time() - start, 3)
    time_path.write_text(str(elapsed_sec))
    status.update({'rc': rc, 'timed_out': timed_out, 'interrupted': interrupted, 'elapsed_sec': elapsed_sec, 'phase': 'finalizing'})
    atomic_write_json(status_path, status)

    stdout_empty = (not out_path.exists()) or out_path.stat().st_size == 0
    stderr_empty = (not err_path.exists()) or err_path.stat().st_size == 0
    val_ok = False; val_msg = ''
    if out_path.exists() and out_path.stat().st_size > 0:
        val = subprocess.run([sys.executable, str(VAL), str(in_path), str(out_path)], capture_output=True, text=True)
        val_ok = (val.returncode == 0)
        val_msg = (val.stdout or '') + (val.stderr or '')
    stderr_text = err_path.read_text(errors='ignore') if err_path.exists() else ''
    summary = parse_all_kv(stderr_text)
    hb = load_json(heartbeat_path)
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
        'elapsed_sec': elapsed_sec,
        'maxrss_kb': maxrss_kb,
        'summary_kv': summary,
        'heartbeat': hb,
    }
    atomic_write_json(result_path, result)
    status['state'] = 'finished'
    status['phase'] = 'finished'
    atomic_write_json(status_path, status)
    append_journal(journal_path, {'event': 'finished', **status, 'validator_ok': val_ok})


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
    ap.add_argument('--journal', default='/mnt/data/progress40_case_journal.jsonl')
    ap.add_argument('--detach', action='store_true')
    ap.add_argument('--_supervise', action='store_true')
    args = ap.parse_args()

    if args._supervise:
        supervise(args)
        return

    outdir = Path(args.outdir)
    outdir.mkdir(parents=True, exist_ok=True)
    status_path = outdir / 'status.json'
    status = {
        'case_name': args.run_tag,
        'mode': args.mode,
        'n': args.n,
        'seed': args.seed,
        'profile_mode': args.profile_mode,
        'delta_mode': args.delta_mode,
        'cmd': [sys.executable, __file__, '--_supervise'],
        'start_time': time.time(),
        'state': 'starting',
        'phase': 'launching',
        'launcher_pid': os.getpid(),
    }
    atomic_write_json(status_path, status)
    append_journal(Path(args.journal), {'event': 'launching', **status})

    if args.detach:
        cmd = [sys.executable, __file__, '--_supervise', '--solver', args.solver, '--run-tag', args.run_tag,
               '--mode', args.mode, '--n', str(args.n), '--seed', str(args.seed), '--profile-mode', args.profile_mode,
               '--delta-mode', args.delta_mode, '--outdir', args.outdir, '--timeout-sec', str(args.timeout_sec), '--journal', args.journal]
        for e in args.env:
            cmd += ['--env', e]
        with open(os.devnull, 'wb') as dn:
            proc = subprocess.Popen(cmd, start_new_session=True, stdin=dn, stdout=dn, stderr=dn, close_fds=True)
        status['supervisor_pid'] = proc.pid
        status['state'] = 'detached'
        status['phase'] = 'detached'
        atomic_write_json(status_path, status)
        append_journal(Path(args.journal), {'event': 'detached', **status})
        print(json.dumps({'run_tag': args.run_tag, 'supervisor_pid': proc.pid, 'state': 'detached'}))
    else:
        supervise(args)

if __name__ == '__main__':
    main()
