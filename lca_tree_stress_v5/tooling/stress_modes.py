#!/usr/bin/env python3
from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

from suite_utils import default_solver_path, resolve_solver_path


ROOT = Path(__file__).resolve().parent
RUN_CASE = ROOT / "run_case.py"
DEFAULT_MODES = [
    "comb_core",
    "comb_plus_unary",
    "comb_dense",
    "comb_rect_dense",
    "multi_comb_core",
    "multi_comb_rect",
    "multi_comb_cap",
    "chain_unary",
    "star_pairs",
    "balanced_sibling",
    "balanced_dense",
    "broom_mixed",
    "caterpillar_mixed",
    "caterpillar_rect_dense",
    "random_recursive_mixed",
]
DEFAULT_SIZES = [9999, 19999, 39999, 79999, 99999]


def _parse_csv_ints(s: str) -> list[int]:
    return [int(x) for x in s.split(",") if x.strip()]


def _parse_csv_strs(s: str) -> list[str]:
    return [x.strip() for x in s.split(",") if x.strip()]


def main() -> int:
    ap = argparse.ArgumentParser(description="Run the predefined stress sweep.")
    ap.add_argument("solver", nargs="?", default=str(default_solver_path(ROOT)))
    ap.add_argument("seed", nargs="?", type=int, default=1)
    ap.add_argument("shuffle_labels", nargs="?", type=int, default=1)
    ap.add_argument("shuffle_queries", nargs="?", type=int, default=1)
    ap.add_argument("--sizes", default=",".join(str(x) for x in DEFAULT_SIZES))
    ap.add_argument("--modes", default=",".join(DEFAULT_MODES))
    ap.add_argument("--out-root", default="_runs")
    args = ap.parse_args()

    solver = resolve_solver_path(args.solver, root=ROOT)
    sizes = _parse_csv_ints(args.sizes)
    modes = _parse_csv_strs(args.modes)
    out_root = (ROOT / args.out_root).resolve()
    out_root.mkdir(parents=True, exist_ok=True)

    for mode in modes:
        print(f"===== {mode} =====")
        for n in sizes:
            outdir = out_root / f"{mode}_{n}_{args.seed}"
            cmd = [
                sys.executable,
                str(RUN_CASE),
                mode,
                str(n),
                str(args.seed),
                str(args.shuffle_labels),
                str(args.shuffle_queries),
                str(solver),
                str(outdir),
            ]
            res = subprocess.run(cmd)
            if res.returncode != 0:
                return res.returncode
        print()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
