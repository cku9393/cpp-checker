#!/usr/bin/env python3
import argparse, json, os, re, resource, signal, subprocess, sys, threading, time
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
    tmp.write_text(json.dumps(obj, indent=2, ensure_ascii=False), encoding='utf-8')
    os.replace(tmp, path)
    # fsync parent dir best effort
    try:
        fd = os.open(str(path.parent), os.O_DIRECTORY)
        try:
            os.fsync(fd)
        finally:
            os.close(fd)
    except Exception:
        pass


def append_journal(path: Path, obj):
    with path.open('a', encoding='utf-8') as f:
        f.write(json.dumps(obj, ensure_ascii=False) + '\n')
        f.flush()
        os.fsync(f.fileno())


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


def process_exists(pid: int) -> bool:
    proc_stat = Path(f"/proc/{pid}/stat")
    if proc_stat.exists():
        try:
            data = proc_stat.read_text().split()
            if len(data) >= 3 and data[2] == 'Z':
                return False
        except Exception:
            pass
    try:
        os.kill(pid, 0)
        return True
    except ProcessLookupError:
        return False
    except PermissionError:
        return True


def recover_if_stale(outdir: Path, journal: Path):
    status_path = outdir / 'status.json'
    result_path = outdir / 'result.json'
    heartbeat_path = outdir / 'heartbeat.json'
    if not status_path.exists() or result_path.exists():
        return False
    try:
        status = json.loads(status_path.read_text())
    except Exception:
        return False
    pid = status.get('pid')
    state = status.get('state')
    if state != 'running' or not pid or process_exists(pid):
        return False
    hb = {}
    if heartbeat_path.exists():
        try:
            hb = json.loads(heartbeat_path.read_text())
        except Exception:
            hb = {}
    stderr_path = outdir / 'stderr.txt'
    stdout_path = outdir / 'stdout.txt'
    result = {
        'run_tag': status.get('case_name', outdir.name),
        'mode': None,
        'n': None,
        'seed': None,
        'profile_mode': None,
        'delta_mode': None,
        'rc': -999,
        'timed_out': False,
        'interrupted': False,
        'validator_ok': False,
        'validator_msg': 'recovered stale runner state',
        'stdout_empty': (not stdout_path.exists()) or stdout_path.stat().st_size == 0,
        'stderr_empty': (not stderr_path.exists()) or stderr_path.stat().st_size == 0,
        'elapsed_sec': None,
        'maxrss_kb': None,
        'summary_kv': parse_all_kv(stderr_path.read_text(errors='ignore')) if stderr_path.exists() else {},
        'heartbeat': hb,
        'synthetic': True,
    }
    atomic_write_json(result_path, result)
    status['phase'] = 'finished'
    status['state'] = 'finished'
    atomic_write_json(status_path, status)
    append_journal(journal, {'event': 'recovered_stale', **status})
    return True


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
    ap.add_argument('--journal', default='/mnt/data/progress31_case_journal.jsonl')
    ap.add_argument('--recover-only', action='store_true')
    args = ap.parse_args()

    outdir = Path(args.outdir)
    outdir.mkdir(parents=True, exist_ok=True)
    journal_path = Path(args.journal)
    if args.recover_only:
        recovered = recover_if_stale(outdir, journal_path)
        print(json.dumps({'recovered': recovered, 'outdir': str(outdir)}))
        return

    recover_if_stale(outdir, journal_path)

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

    if not ROOT.exists():
        raise SystemExit(f'missing stress suite dir: {ROOT}')

    with open(in_path, 'w', encoding='utf-8') as f:
        subprocess.run([sys.executable, str(GEN), '--mode', args.mode, '--n', str(args.n), '--seed', str(args.seed), '--meta', str(meta_path), '--parent-out', str(parent_path)], check=True, stdout=f)

    env = make_env(args)
    cmd = [args.solver]
    start_wall = time.monotonic()
    start_time_epoch = time.time()
    status = {
        'case_name': args.run_tag,
        'phase': 'starting',
        'cmd': cmd,
        'start_time': start_time_epoch,
        'state': 'starting',
        'pid': None,
        'mode': args.mode,
        'n': args.n,
        'seed': args.seed,
        'profile_mode': args.profile_mode,
        'delta_mode': args.delta_mode,
    }
    atomic_write_json(status_path, status)
    append_journal(journal_path, {'event': 'start', **status})

    stop_hb = False

    def heartbeat_loop(proc):
        last_label = None
        last_del = None
        while not stop_hb:
            try:
                stderr_text = err_path.read_text(errors='ignore') if err_path.exists() else ''
                labels = re.findall(r'phase=([^\s]+)', stderr_text)
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
                    'pid_alive': process_exists(proc.pid),
                }
                atomic_write_json(heartbeat_path, hb)
            except Exception as e:
                with runner_log.open('a', encoding='utf-8') as f:
                    f.write(f'heartbeat_error {e}\n')
            time.sleep(5)

    # launch direct child, no /usr/bin/time wrapper
    with open(in_path, 'rb') as fin, open(out_path, 'wb') as fout, open(err_path, 'wb') as ferr:
        proc = subprocess.Popen(cmd, stdin=fin, stdout=fout, stderr=ferr, env=env)
        status['phase'] = 'running'; status['state'] = 'running'; status['pid'] = proc.pid
        atomic_write_json(status_path, status)
        append_journal(journal_path, {'event': 'running', **status})
        hb_thread = threading.Thread(target=heartbeat_loop, args=(proc,), daemon=True)
        hb_thread.start()
        timed_out = False
        interrupted = False
        rc = None
        try:
            rc = proc.wait(timeout=args.timeout_sec if args.timeout_sec > 0 else None)
        except subprocess.TimeoutExpired:
            timed_out = True
            try:
                proc.kill()
            except Exception:
                pass
            rc = proc.wait()
        except KeyboardInterrupt:
            interrupted = True
            try:
                proc.kill()
            except Exception:
                pass
            rc = proc.wait()
        finally:
            stop_hb = True
            hb_thread.join(timeout=2)

    elapsed = round(time.monotonic() - start_wall, 3)
    stdout_empty = (not out_path.exists()) or out_path.stat().st_size == 0
    stderr_empty = (not err_path.exists()) or err_path.stat().st_size == 0
    val_ok = False
    val_msg = ''
    if out_path.exists() and out_path.stat().st_size > 0:
        val = subprocess.run([sys.executable, str(VAL), str(in_path), str(out_path)], capture_output=True, text=True)
        val_ok = (val.returncode == 0)
        val_msg = (val.stdout or '') + (val.stderr or '')

    stderr_text = err_path.read_text(errors='ignore') if err_path.exists() else ''
    summary = parse_all_kv(stderr_text)
    hb = json.loads(heartbeat_path.read_text()) if heartbeat_path.exists() else {}
    try:
        maxrss = resource.getrusage(resource.RUSAGE_CHILDREN).ru_maxrss
    except Exception:
        maxrss = None

    # write simple time file for compatibility
    time_path.write_text(f"{elapsed} {maxrss if maxrss is not None else 0}\n", encoding='utf-8')

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
        'elapsed_sec': elapsed,
        'maxrss_kb': maxrss,
        'summary_kv': summary,
        'heartbeat': hb,
    }
    atomic_write_json(result_path, result)
    status['phase'] = 'finished'; status['state'] = 'finished'
    atomic_write_json(status_path, status)
    append_journal(journal_path, {'event': 'finished', **status, 'rc': rc, 'timed_out': timed_out, 'validator_ok': val_ok, 'elapsed_sec': elapsed})
    print(json.dumps({'run_tag': args.run_tag, 'rc': rc, 'timed_out': timed_out, 'validator_ok': val_ok, 'elapsed_sec': elapsed}, ensure_ascii=False))

if __name__ == '__main__':
    main()
