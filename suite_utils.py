#!/usr/bin/env python3
from __future__ import annotations

import math
import os
import signal
import statistics
import subprocess
import sys
import time
from pathlib import Path
from typing import List, Optional, Sequence, Tuple


IS_WINDOWS = os.name == "nt"
IS_DARWIN = sys.platform == "darwin"


def parse_int_list_csv(s: str) -> List[int]:
    s = s.strip()
    if not s:
        return []
    return [int(x) for x in s.split(",") if x.strip()]


def parse_str_list_csv(s: str) -> List[str]:
    s = s.strip()
    if not s:
        return []
    return [x.strip() for x in s.split(",") if x.strip()]


def default_solver_name() -> str:
    return "solve.exe" if IS_WINDOWS else "solve"


def default_solver_path(root: Path) -> Path:
    return (root / default_solver_name()).resolve()


def resolve_solver_path(path_like: str | Path, *, root: Optional[Path] = None) -> Path:
    p = Path(path_like)
    if not p.is_absolute() and root is not None:
        p = root / p
    p = p.resolve()
    if p.exists():
        return p
    if IS_WINDOWS and p.suffix.lower() != ".exe":
        exe = p.with_suffix(".exe")
        if exe.exists():
            return exe
    return p


def ensure_executable(p: Path) -> Path:
    if not p.exists():
        raise FileNotFoundError(str(p))
    if IS_WINDOWS:
        if not p.is_file():
            raise PermissionError(f"Not a file: {p}")
        return p
    if p.is_file() and not os.access(p, os.X_OK):
        raise PermissionError(f"Not executable: {p}")
    return p


def _spawn_process(cmd: List[str], stdin, stdout, stderr) -> subprocess.Popen:
    kwargs = {
        "stdin": stdin,
        "stdout": stdout,
        "stderr": stderr,
    }
    if IS_WINDOWS:
        kwargs["creationflags"] = getattr(subprocess, "CREATE_NEW_PROCESS_GROUP", 0)
    else:
        kwargs["start_new_session"] = True
    return subprocess.Popen(cmd, **kwargs)


def _kill_process(proc: subprocess.Popen) -> None:
    if IS_WINDOWS:
        proc.kill()
        return
    try:
        os.killpg(proc.pid, signal.SIGKILL)
    except ProcessLookupError:
        pass


def run_cmd(
    cmd: List[str],
    *,
    stdin_path: Optional[Path] = None,
    stdout_path: Optional[Path] = None,
    stderr_path: Optional[Path] = None,
    timeout: Optional[float] = None,
) -> Tuple[int, bool, float]:
    t0 = time.perf_counter()
    stdin_f = open(stdin_path, "rb") if stdin_path else None
    stdout_f = open(stdout_path, "wb") if stdout_path else subprocess.DEVNULL
    stderr_f = open(stderr_path, "wb") if stderr_path else subprocess.DEVNULL
    try:
        proc = _spawn_process(cmd, stdin_f, stdout_f, stderr_f)
        try:
            proc.wait(timeout=timeout)
            timed_out = False
        except subprocess.TimeoutExpired:
            timed_out = True
            _kill_process(proc)
            proc.wait()
        rc = int(proc.returncode)
    finally:
        if stdin_f:
            stdin_f.close()
        if stdout_f not in (None, subprocess.DEVNULL):
            stdout_f.close()
        if stderr_f not in (None, subprocess.DEVNULL):
            stderr_f.close()
    return rc, timed_out, time.perf_counter() - t0


def _waitstatus_to_exitcode(status: int) -> int:
    if hasattr(os, "waitstatus_to_exitcode"):
        return os.waitstatus_to_exitcode(status)
    if os.WIFSIGNALED(status):
        return -os.WTERMSIG(status)
    if os.WIFEXITED(status):
        return os.WEXITSTATUS(status)
    return status


def _normalize_posix_rss_kb(raw_rss: int) -> int:
    if IS_DARWIN:
        return int((raw_rss + 1023) // 1024)
    return int(raw_rss)


def _run_solver_posix(
    solver: Path,
    in_path: Path,
    out_path: Path,
    stderr_path: Path,
    timeout: Optional[float],
) -> Tuple[int, bool, float, Optional[int]]:
    stdin_f = open(in_path, "rb")
    stdout_f = open(out_path, "wb")
    stderr_f = open(stderr_path, "wb")
    try:
        proc = _spawn_process([str(solver)], stdin_f, stdout_f, stderr_f)
        t0 = time.perf_counter()
        while True:
            pid, status, rusage = os.wait4(proc.pid, os.WNOHANG)
            if pid == proc.pid:
                elapsed = time.perf_counter() - t0
                proc.returncode = _waitstatus_to_exitcode(status)
                rss_kb = _normalize_posix_rss_kb(rusage.ru_maxrss)
                return int(proc.returncode), False, elapsed, rss_kb
            if timeout is not None and time.perf_counter() - t0 > timeout:
                _kill_process(proc)
                pid, status, rusage = os.wait4(proc.pid, 0)
                elapsed = time.perf_counter() - t0
                proc.returncode = _waitstatus_to_exitcode(status)
                rss_kb = _normalize_posix_rss_kb(rusage.ru_maxrss)
                return int(proc.returncode), True, elapsed, rss_kb
            time.sleep(0.01)
    finally:
        stdin_f.close()
        stdout_f.close()
        stderr_f.close()


def _sample_windows_peak_rss_kb(proc: subprocess.Popen) -> Optional[int]:
    try:
        import ctypes
        from ctypes import wintypes
    except ImportError:
        return None

    PROCESS_QUERY_INFORMATION = 0x0400
    PROCESS_QUERY_LIMITED_INFORMATION = 0x1000

    class PROCESS_MEMORY_COUNTERS(ctypes.Structure):
        _fields_ = [
            ("cb", wintypes.DWORD),
            ("PageFaultCount", wintypes.DWORD),
            ("PeakWorkingSetSize", ctypes.c_size_t),
            ("WorkingSetSize", ctypes.c_size_t),
            ("QuotaPeakPagedPoolUsage", ctypes.c_size_t),
            ("QuotaPagedPoolUsage", ctypes.c_size_t),
            ("QuotaPeakNonPagedPoolUsage", ctypes.c_size_t),
            ("QuotaNonPagedPoolUsage", ctypes.c_size_t),
            ("PagefileUsage", ctypes.c_size_t),
            ("PeakPagefileUsage", ctypes.c_size_t),
        ]

    kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
    psapi = ctypes.WinDLL("psapi", use_last_error=True)
    access = PROCESS_QUERY_INFORMATION | PROCESS_QUERY_LIMITED_INFORMATION
    handle = kernel32.OpenProcess(access, False, proc.pid)
    if not handle:
        return None
    try:
        counters = PROCESS_MEMORY_COUNTERS()
        counters.cb = ctypes.sizeof(PROCESS_MEMORY_COUNTERS)
        if not psapi.GetProcessMemoryInfo(handle, ctypes.byref(counters), counters.cb):
            return None
        return int(counters.PeakWorkingSetSize // 1024)
    finally:
        kernel32.CloseHandle(handle)


def _run_solver_windows(
    solver: Path,
    in_path: Path,
    out_path: Path,
    stderr_path: Path,
    timeout: Optional[float],
) -> Tuple[int, bool, float, Optional[int]]:
    stdin_f = open(in_path, "rb")
    stdout_f = open(out_path, "wb")
    stderr_f = open(stderr_path, "wb")
    try:
        proc = _spawn_process([str(solver)], stdin_f, stdout_f, stderr_f)
        t0 = time.perf_counter()
        peak_rss_kb = _sample_windows_peak_rss_kb(proc)
        while True:
            sample = _sample_windows_peak_rss_kb(proc)
            if sample is not None:
                peak_rss_kb = sample if peak_rss_kb is None else max(peak_rss_kb, sample)
            try:
                proc.wait(timeout=0.05)
                break
            except subprocess.TimeoutExpired:
                if timeout is not None and time.perf_counter() - t0 > timeout:
                    _kill_process(proc)
                    proc.wait()
                    return int(proc.returncode), True, time.perf_counter() - t0, peak_rss_kb
        sample = _sample_windows_peak_rss_kb(proc)
        if sample is not None:
            peak_rss_kb = sample if peak_rss_kb is None else max(peak_rss_kb, sample)
        return int(proc.returncode), False, time.perf_counter() - t0, peak_rss_kb
    finally:
        stdin_f.close()
        stdout_f.close()
        stderr_f.close()


def _write_time_artifact(time_path: Path, sec: float, rss_kb: Optional[int]) -> None:
    rss_text = "-1" if rss_kb is None else str(rss_kb)
    time_path.write_text(f"{sec:.6f} {rss_text}\n", encoding="utf-8")


def run_solver_with_time(
    solver: Path,
    in_path: Path,
    out_path: Path,
    time_path: Path,
    stderr_path: Path,
    timeout: Optional[float],
) -> Tuple[int, bool, Optional[float], Optional[int]]:
    if not IS_WINDOWS and hasattr(os, "wait4"):
        rc, timed_out, sec, rss_kb = _run_solver_posix(solver, in_path, out_path, stderr_path, timeout)
    else:
        rc, timed_out, sec, rss_kb = _run_solver_windows(solver, in_path, out_path, stderr_path, timeout)

    if timed_out:
        return rc, True, None, None

    _write_time_artifact(time_path, sec, rss_kb)
    return rc, False, sec, rss_kb


def median_or_none(vals: Sequence[float]) -> Optional[float]:
    if not vals:
        return None
    return float(statistics.median(vals))


def geometric_median_growth(points: Sequence[Tuple[int, float]]) -> Optional[float]:
    if len(points) < 2:
        return None
    pts = sorted(points)
    ratios = []
    for i in range(len(pts) - 1):
        _, t1 = pts[i]
        _, t2 = pts[i + 1]
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
        return x.replace("|", "\\|")

    out = []
    out.append("| " + " | ".join(map(esc, headers)) + " |")
    out.append("| " + " | ".join(["---"] * len(headers)) + " |")
    for r in rows:
        out.append("| " + " | ".join(map(esc, r)) + " |")
    return "\n".join(out) + "\n"
