#!/usr/bin/env python3
import csv, json, math, statistics, sys
from collections import defaultdict, Counter
from pathlib import Path

def read_tsv(path):
    if not Path(path).exists():
        return []
    with open(path) as f:
        return list(csv.DictReader(f, delimiter='\t'))

def to_float(x):
    try:
        return float(x)
    except:
        return 0.0

def to_int(x):
    try:
        return int(float(x))
    except:
        return 0

census, rawrows, out_summary, out_hotspots = sys.argv[1:5]
C = read_tsv(census)
R = read_tsv(rawrows)
# summary one row per case_mode+case_n
rows=[]
for mode in sorted({r['case_mode'] for r in C}):
    for n in sorted({to_int(r['case_n']) for r in C if r['case_mode']==mode}):
        rows_c=[r for r in C if r['case_mode']==mode and to_int(r['case_n'])==n]
        rows_r=[r for r in R if r['case_mode']==mode and to_int(r['case_n'])==n]
        if not rows_c: continue
        shadow_attempted=sum(to_int(r['shadow_attempted']) for r in rows_c)
        shadow_match=sum(to_int(r['shadow_match']) for r in rows_c)
        vals={
            'case_mode':mode,
            'case_n':n,
            'rows':len(rows_c),
            'shadow_prefilter_hit_ratio': sum(to_int(r['shadow_prefilter_hit']) for r in rows_c)/len(rows_c),
            'shadow_attempted_ratio': shadow_attempted/len(rows_c),
            'shadow_match_ratio': shadow_match/len(rows_c),
            'mean_fallback_total_ms': statistics.mean([to_float(r['fallback_total_ms']) for r in rows_c]),
            'mean_fallback_local_id_adj_ms': statistics.mean([to_float(r['fallback_local_id_adj_ms']) for r in rows_c]),
            'mean_fallback_step3_ms': statistics.mean([to_float(r['fallback_step3_ms']) for r in rows_c]),
            'mean_fallback_keep_order_ms': statistics.mean([to_float(r['fallback_keep_order_ms']) for r in rows_c]),
            'mean_fallback_applyPatch_ms': statistics.mean([to_float(r['fallback_applyPatch_ms']) for r in rows_c]),
            'dense_guard_ratio': statistics.mean([to_int(r['dense_guard']) for r in rows_c]),
            'E_guard_ratio': statistics.mean([to_int(r['E_guard']) for r in rows_c]),
            'Q_guard_ratio': statistics.mean([to_int(r['Q_guard']) for r in rows_c]),
        }
        if rows_r:
            # dominant hotspot among raw fields
            fields=[f for f in rows_r[0].keys() if f.endswith('_ms') and f.startswith('spqr_')]
            means={f: statistics.mean([to_float(r[f]) for r in rows_r]) for f in fields}
            dominant=max(means, key=means.get)
            vals.update({
                'mean_spqr_total_ms': statistics.mean([to_float(r['spqr_total_ms']) for r in rows_r]),
                'mean_spqr_delta_ms': statistics.mean([to_float(r['spqr_delta_ms']) for r in rows_r]),
                'dominant_rawbuild_hotspot': dominant,
                'dominant_rawbuild_hotspot_ms': means[dominant],
            })
        else:
            vals.update({'mean_spqr_total_ms':0.0,'mean_spqr_delta_ms':0.0,'dominant_rawbuild_hotspot':'','dominant_rawbuild_hotspot_ms':0.0})
        rows.append(vals)
with open(out_summary,'w',newline='') as f:
    if rows:
        w=csv.DictWriter(f, fieldnames=list(rows[0].keys()), delimiter='\t'); w.writeheader(); w.writerows(rows)
    else:
        f.write('case_mode\tcase_n\n')
# hotspots aggregate by raw field across all rows_r
hot=[]
if R:
    fields=[f for f in R[0].keys() if f.endswith('_ms') and f.startswith('spqr_')]
    for f in fields:
        hot.append({'field':f,'mean_ms':statistics.mean([to_float(r[f]) for r in R]),'max_ms':max(to_float(r[f]) for r in R),'count':len(R)})
    hot.sort(key=lambda x:(-x['mean_ms'],x['field']))
with open(out_hotspots,'w',newline='') as f:
    if hot:
        w=csv.DictWriter(f, fieldnames=list(hot[0].keys()), delimiter='\t'); w.writeheader(); w.writerows(hot)
    else:
        f.write('field\tmean_ms\tmax_ms\tcount\n')
