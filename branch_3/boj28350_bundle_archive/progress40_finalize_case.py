#!/usr/bin/env python3
import argparse
from pathlib import Path
from run_progress40_case_supervised import finalize_case_dir

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--case-dir', required=True)
    ap.add_argument('--journal', default='/mnt/data/progress40_case_journal.jsonl')
    ap.add_argument('--elapsed-sec', type=float, default=None)
    args = ap.parse_args()
    finalize_case_dir(Path(args.case_dir), Path(args.journal), elapsed_override=args.elapsed_sec)
    print('finalized recovered row')

if __name__ == '__main__':
    main()
