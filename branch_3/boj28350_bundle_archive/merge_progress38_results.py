#!/usr/bin/env python3
import argparse, json
from pathlib import Path

def load_results(root: Path):
    completed = {}
    for d in sorted(root.iterdir()):
        if not d.is_dir():
            continue
        rp = d / 'result.json'
        if rp.exists():
            try:
                completed[d.name] = json.loads(rp.read_text())
            except Exception as e:
                completed[d.name] = {'parse_error': str(e)}
    return completed

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--root', required=True)
    ap.add_argument('--out', required=True)
    ap.add_argument('--base-source', required=True)
    ap.add_argument('--output-source', required=True)
    ap.add_argument('--missing', nargs='*', default=[])
    ap.add_argument('--status', default='partial')
    ap.add_argument('--current-conclusion', default='')
    ap.add_argument('--notes', nargs='*', default=[])
    args = ap.parse_args()
    root=Path(args.root)
    obj={
        'status': args.status,
        'base_source': args.base_source,
        'output_source': args.output_source,
        'completed_runs': load_results(root),
        'missing_runs': args.missing,
        'current_safe_conclusion': args.current_conclusion,
        'notes': args.notes,
    }
    Path(args.out).write_text(json.dumps(obj, indent=2, ensure_ascii=False))
    print('merged', len(obj['completed_runs']))
if __name__ == '__main__':
    main()
