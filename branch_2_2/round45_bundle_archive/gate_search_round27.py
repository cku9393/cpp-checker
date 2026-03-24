#!/usr/bin/env python3
import csv, json, itertools, statistics, sys
from pathlib import Path

def to_int(x):
    try:
        return int(float(x))
    except:
        return 0

def to_float(x):
    try:
        return float(x)
    except:
        return 0.0
rows=list(csv.DictReader(open(sys.argv[1]), delimiter='\t'))
out_tsv=sys.argv[2]
out_json=sys.argv[3]
# only attempted rows matter
rows=[r for r in rows if to_int(r.get('shadow_attempted',0))==1]
shape_values=sorted(set(r.get('badQueriesShape','') for r in rows if r.get('badQueriesShape','')))
allowlists=[set(), set(shape_values)]
# also singleton first few shapes
for s in shape_values[:4]: allowlists.append({s})
results=[]
for cc_req in [0,1]:
  for bz_mode in ['any','zero','small']:
    for Amax in [8,16,32,64]:
      for Bmax in [0,4,8,16,32,64]:
        for Emin in [150000,155000,160000]:
          for Emax in [170000,175000,180000]:
            if Emax < Emin: continue
            for Qmin in [55000,58000,62000]:
              for Qmax in [62000,68000,70000]:
                if Qmax < Qmin: continue
                for Rmin in [0.36,0.38,0.39]:
                  for Rmax in [0.40,0.41,0.43]:
                    if Rmax < Rmin: continue
                    for allow in allowlists:
                      matched=[]
                      for r in rows:
                        if to_int(r['dense_guard'])!=1 or to_int(r['E_guard'])!=1 or to_int(r['Q_guard'])!=1 or to_int(r['boundary_guard'])!=0:
                            continue
                        if cc_req and to_int(r['ccOne'])!=1: continue
                        bsz=to_int(r['boundarySize']); att=to_int(r['attachCuts']); e=to_int(r['currentE']); q=to_int(r['currentQ']); ratio=to_float(r['q_over_e']); shape=r.get('badQueriesShape','')
                        if bz_mode=='zero' and to_int(r['boundaryZero'])!=1: continue
                        if bz_mode=='small' and bsz>Bmax: continue
                        if att>Amax or e<Emin or e>Emax or q<Qmin or q>Qmax or ratio<Rmin or ratio>Rmax: continue
                        if allow and shape not in allow: continue
                        matched.append(r)
                      if not matched:
                        continue
                      mismatch=sum(1 for r in matched if to_int(r['shadow_match'])!=1)
                      validator_fail=sum(to_int(r.get('validator_shadow_fail',0)) for r in matched)
                      modes_by_case={}
                      for mode in ['comb_dense','comb_rect_dense','caterpillar_rect_dense']:
                        sub=[r for r in matched if r['case_mode']==mode and to_int(r['case_n'])==8192]
                        total=[r for r in rows if r['case_mode']==mode and to_int(r['case_n'])==8192]
                        modes_by_case[mode]= (len(sub)/len(total) if total else 0.0)
                      mean_saved=statistics.mean([to_float(r['fallback_total_ms'])-to_float(r.get('spqr_total_ms',0)) for r in matched])
                      results.append({
                        'cc_req':cc_req,'bz_mode':bz_mode,'Amax':Amax,'Bmax':Bmax,'Emin':Emin,'Emax':Emax,'Qmin':Qmin,'Qmax':Qmax,'Rmin':Rmin,'Rmax':Rmax,
                        'allow_shapes':'|'.join(sorted(allow)) if allow else '*',
                        'coverage_count':len(matched),'shadow_attempted_count':len(matched),'shadow_match_count':sum(to_int(r['shadow_match']) for r in matched),
                        'shadow_mismatch_count':mismatch,'validator_shadow_fail_count':validator_fail,'mean_fallback_ms_saved_proxy':round(mean_saved,6),
                        'comb_dense_8192_release_hit_ratio':modes_by_case['comb_dense'],
                        'comb_rect_dense_8192_release_hit_ratio':modes_by_case['comb_rect_dense'],
                        'caterpillar_rect_dense_8192_release_hit_ratio':modes_by_case['caterpillar_rect_dense'],
                      })
# choose best gate satisfying conditions
results.sort(key=lambda r:(r['shadow_mismatch_count'], r['validator_shadow_fail_count'], -r['mean_fallback_ms_saved_proxy'], -r['coverage_count']))
with open(out_tsv,'w',newline='') as f:
    if results:
        w=csv.DictWriter(f, fieldnames=list(results[0].keys()), delimiter='\t'); w.writeheader(); w.writerows(results)
    else:
        f.write('coverage_count\n')
best=None
for r in results:
    if r['shadow_attempted_count']<=0: continue
    if r['shadow_match_count']!=r['shadow_attempted_count']: continue
    if r['shadow_mismatch_count']!=0: continue
    if r['validator_shadow_fail_count']!=0: continue
    if r['comb_dense_8192_release_hit_ratio']<0.2: continue
    if max(r['comb_rect_dense_8192_release_hit_ratio'], r['caterpillar_rect_dense_8192_release_hit_ratio'])<0.2: continue
    if r['mean_fallback_ms_saved_proxy']<=0: continue
    best=r; break
if best is None:
    out={'gate_found':False,'reason':'no zero-mismatch positive-speedup gate under searched family','searched_rows':len(rows)}
else:
    out={'gate_found':True,'selected_gate':best}
Path(out_json).write_text(json.dumps(out, indent=2))
