#!/usr/bin/env python3
from __future__ import annotations

import math
import os
import signal
import statistics
import subprocess
import time
from pathlib import Path
from typing import Iterable, List, Optional, Sequence, Tuple


def parse_int_list_csv(s: str) -> List[int]:
    s = s.strip()
    if not s:
        return []
    return [int(x) for x in s.split(',') if x.strip()]


def parse_str_list_csv(s: str) -> List[str]:
    s = s.strip()
    if not s:
        return []
    return [x.strip() for x in s.split(',') if x.strip()]


def ensure_executable(p: Path) -> None:
    if not p.exists():
        raise FileNotFoundError(str(p))
    if p.is_file() and not os.access(p, os.X_OK):
        raise PermissionError(f"Not executable: {p}")


def run_cmd(
    cmd: List[str],
    *,
    stdin_path: Optional[Path] = None,
    stdout_path: Optional[Path] = None,
    stderr_path: Optional[Path] = None,
    timeout: Optional[float] = None,
) -> Tuple[int, bool, float]:
    t0 = time.perf_counter()
    stdin_f = open(stdin_path, 'rb') if stdin_path else None
    stdout_f = open(stdout_path, 'wb') if stdout_path else subprocess.DEVNULL
    stderr_f = open(stderr_path, 'wb') if stderr_path else subprocess.DEVNULL
    try:
        p = subprocess.Popen(
            cmd,
            stdin=stdin_f,
            stdout=stdout_f,
            stderr=stderr_f,
            preexec_fn=os.setsid,
        )
        try:
            p.wait(timeout=timeout)
            timed_out = False
        except subprocess.TimeoutExpired:
            timed_out = True
            try:
                os.killpg(p.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
            p.wait()
        rc = int(p.returncode)
    finally:
        if stdin_f:
            stdin_f.close()
        if stdout_f not in (None, subprocess.DEVNULL):
            stdout_f.close()
        if stderr_f not in (None, subprocess.DEVNULL):
            stderr_f.close()
    return rc, timed_out, time.perf_counter() - t0


def run_solver_with_time(
    solver: Path,
    in_path: Path,
    out_path: Path,
    time_path: Path,
    stderr_path: Path,
    timeout: Optional[float],
) -> Tuple[int, bool, Optional[float], Optional[int]]:
    cmd = ['/usr/bin/time', '-f', '%e %M', '-o', str(time_path), str(solver)]
    rc, timed_out, _ = run_cmd(
        cmd,
        stdin_path=in_path,
        stdout_path=out_path,
        stderr_path=stderr_path,
        timeout=timeout,
    )
    if timed_out:
        return rc, True, None, None
    if not time_path.exists():
        return rc, False, None, None
    try:
        txt = time_path.read_text().strip().split()
        sec = float(txt[0])
        rss = int(txt[1])
        return rc, False, sec, rss
    except Exception:
        return rc, False, None, None


def median_or_none(vals: Sequence[float]) -> Optional[float]:
    if not vals:
        return None
    return float(statistics.median(vals))


def geometric_median_growth(points: Sequence[Tuple[int, float]]) -> Optional[float]:
    """Return maximum consecutive growth ratio over size-doubled-ish points."""
    if len(points) < 2:
        return None
    pts = sorted(points)
    ratios = []
    for i in range(len(pts) - 1):
        n1, t1 = pts[i]
        n2, t2 = pts[i + 1]
        if t1 > 0:
            ratios.append(t2 / t1)
    return max(ratios) if ratios else None


def fit_loglog_slope(points: Sequence[Tuple[int, float]]) -> Optional[float]:
    pts = [(float(n), float(t)) for n, t in points if n > 0 and t and t > 0]
    if len(pts) < 2:
        return None
    xs = [math.log(n) for n, _ in pts]
    ys = [math.log(t) for _, t in pts]
    mx = sum(xs) / len(xs)
    my = sum(ys) / len(ys)
    num = sum((x - mx) * (y - my) for x, y in zip(xs, ys))
    den = sum((x - mx) ** 2 for x in xs)
    if den == 0:
        return None
    return num / den


def markdown_table(headers: List[str], rows: List[List[str]]) -> str:
    def esc(x: str) -> str:
        return x.replace('|', '\\|')
    out = []
    out.append('| ' + ' | '.join(map(esc, headers)) + ' |')
    out.append('| ' + ' | '.join(['---'] * len(headers)) + ' |')
    for r in rows:
        out.append('| ' + ' | '.join(map(esc, r)) + ' |')
    return '\n'.join(out) + '\n'
