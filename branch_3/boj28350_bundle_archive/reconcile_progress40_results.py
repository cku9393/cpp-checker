#!/usr/bin/env python3
import argparse, json
from pathlib import Path

def load_json(p: Path):
    if not p.exists():
        return {}
    try:
        return json.loads(p.read_text())
    except Exception:
        return {}

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--merged', required=True)
    ap.add_argument('--root', required=True)
    ap.add_argument('--out', required=True)
    args = ap.parse_args()
    merged = load_json(Path(args.merged))
    root = Path(args.root)
    completed = merged.get('completed_runs', {})
    missing = set(merged.get('missing_runs', []))
    authoritative_rows=[]; partial_rows=[]; stale_rows=[]
    for name,row in completed.items():
        if isinstance(row, dict) and row.get('validator_ok'):
            authoritative_rows.append(name)
        else:
            partial_rows.append(name)
    if root.exists():
        for d in sorted(root.iterdir()):
            if not d.is_dir():
                continue
            has_status=(d/'status.json').exists()
            has_result=(d/'result.json').exists()
            if has_status and not has_result:
                stale_rows.append(d.name)
    out={
        'authoritative_rows': sorted(authoritative_rows),
        'partial_rows': sorted(partial_rows),
        'stale_rows': sorted(stale_rows),
        'missing_rows': sorted((missing | set(stale_rows)) - set(authoritative_rows)),
    }
    Path(args.out).write_text(json.dumps(out, indent=2, ensure_ascii=False))
    print(json.dumps(out, ensure_ascii=False))
if __name__ == '__main__':
    main()
