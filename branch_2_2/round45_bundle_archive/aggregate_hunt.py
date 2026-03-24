#!/usr/bin/env python3
from __future__ import annotations

import argparse, csv
from pathlib import Path


def markdown_table(headers, rows):
    out = []
    out.append('| ' + ' | '.join(headers) + ' |')
    out.append('| ' + ' | '.join(['---'] * len(headers)) + ' |')
    for row in rows:
        out.append('| ' + ' | '.join(row) + ' |')
    return '\n'.join(out) + '\n'


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--out', required=True)
    ap.add_argument('--top-k', type=int, default=20)
    args = ap.parse_args()

    out = Path(args.out).resolve()
    csv_path = out / 'hunt.csv'
    latest = {}
    with open(csv_path, newline='') as f:
        for r in csv.DictReader(f):
            key = (r['mode'], r['n'], r['seed'], r['shuffle_labels'], r['shuffle_queries'])
            latest[key] = r
    rows = []
    for r in latest.values():
        sec = float(r['sec']) if r['sec'] else None
        rss = int(r['rss_kb']) if r['rss_kb'] else None
        rows.append((r, sec, rss))
    ranked = sorted([x for x in rows if x[1] is not None], key=lambda x: x[1], reverse=True)
    top = ranked[:args.top_k]
    summary = out / 'hunt_summary.md'
    with open(summary, 'w', encoding='utf-8') as f:
        f.write('# Hardest-case hunt\n\n')
        f.write(markdown_table(
            ['rank','mode','n','seed','L','Q','sec','rss_kb','val_ok','case_dir'],
            [[str(i+1), r['mode'], r['n'], r['seed'], r['shuffle_labels'], r['shuffle_queries'], f'{sec:.3f}', str(rss), r['val_ok'], r['case_dir']] for i,(r,sec,rss) in enumerate(top)]
        ))
    print(summary)

if __name__ == '__main__':
    main()
