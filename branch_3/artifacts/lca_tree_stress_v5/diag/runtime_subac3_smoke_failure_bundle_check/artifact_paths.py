#!/usr/bin/env python3
from __future__ import annotations
import sys
from pathlib import Path
ROOT = Path(__file__).resolve().parent
ARTIFACTS = ROOT / "artifacts"
if __name__ == "__main__":
    key = sys.argv[1]
    if key == "lca_smoke":
        print(ARTIFACTS / "lca_tree_stress_v5" / "smoke")
    elif key == "branch_run_case":
        print(ARTIFACTS / "lca_tree_stress_v5" / "run_case")
    else:
        raise SystemExit(2)
