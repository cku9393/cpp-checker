#!/usr/bin/env python3
from __future__ import annotations
import argparse
import json
import sys
from pathlib import Path

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("mode")
    ap.add_argument("n")
    ap.add_argument("seed")
    ap.add_argument("shuffle_labels")
    ap.add_argument("shuffle_queries")
    ap.add_argument("solver")
    ap.add_argument("outdir")
    ap.add_argument("--timeout")
    ap.add_argument("--env", action="append", default=[])
    args = ap.parse_args()
    outdir = Path(args.outdir)
    outdir.mkdir(parents=True, exist_ok=True)
    (outdir / "in.txt").write_text("3 1\n2 3 2\n", encoding="utf-8")
    (outdir / "meta.json").write_text('{"mode":"fake_mode"}\n', encoding="utf-8")
    (outdir / "hidden_parent.txt").write_text("0\n", encoding="utf-8")
    (outdir / "out.txt").write_text("0\n0\n0\n", encoding="utf-8")
    (outdir / "time.txt").write_text("0.001 1234\n", encoding="utf-8")
    (outdir / "solver_stderr.txt").write_text("fake solver stderr\n", encoding="utf-8")
    payload = {
        "status": "solver_acceptance_failure",
        "category": "solver",
        "exit_code": 1,
        "message": "query #1 mismatch: lca(2, 3)=1, expected 2",
        "solver_exit_code": None,
        "solver_signal": None,
        "timed_out": False,
        "validator_ok": False,
        "sec": 0.001,
        "rss_kb": 1234,
    }
    (outdir / "run_case_result.json").write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    print("[run_case] fake case generated")
    print("[run_case] validator failed: query #1 mismatch: lca(2, 3)=1, expected 2", file=sys.stderr)
    return 1

if __name__ == "__main__":
    raise SystemExit(main())
