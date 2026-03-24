#!/usr/bin/env python3
import csv, itertools, subprocess, sys, json
from pathlib import Path
modes=['comb_plus_unary','comb_core','multi_comb_core','comb_dense','comb_rect_dense','caterpillar_rect_dense','multi_comb_rect','multi_comb_cap','chain_unary','balanced_dense','random_recursive_mixed']
sizes=[128,256,384,512]
seeds=[1,2,3]
extra=[('comb_plus_unary',2048),('comb_plus_unary',4096),('comb_plus_unary',8192),('comb_core',4096),('comb_core',8192),('comb_core',16384),('multi_comb_core',4096),('multi_comb_core',8192),('comb_dense',1024),('comb_dense',2048),('comb_dense',4096),('comb_rect_dense',2048),('comb_rect_dense',4096),('caterpillar_rect_dense',2048),('caterpillar_rect_dense',4096),('multi_comb_rect',2048),('multi_comb_rect',4096),('multi_comb_cap',2048),('multi_comb_cap',4096)]
solver_ref=sys.argv[1]; solver_new=sys.argv[2]; out_json=sys.argv[3]; out_tsv=sys.argv[4]
rows=[]; mismatch=0; total=0
Path('correctness_tmp').mkdir(exist_ok=True)
case_id=0
for mode in modes:
  for n in sizes:
    for seed in seeds:
      case_id+=1
      inp=Path('correctness_tmp')/f'c{case_id}.in'
      cmd=['python3','gen_case.py','--mode',mode,'--n',str(n),'--seed',str(seed),'--shuffle-labels','--shuffle-queries']
      with open(inp,'w') as f: subprocess.run(cmd, check=True, stdout=f)
      ref=inp.with_suffix('.ref'); cand=inp.with_suffix('.cand')
      subprocess.run([solver_ref], stdin=open(inp), stdout=open(ref,'w'), stderr=subprocess.DEVNULL, check=True)
      subprocess.run([solver_new], stdin=open(inp), stdout=open(cand,'w'), stderr=subprocess.DEVNULL, check=True)
      v=subprocess.run(['python3','validator.py',str(inp),str(cand)], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
      same=(ref.read_text()==cand.read_text())
      if not same: mismatch+=1
      rows.append({'mode':mode,'n':n,'seed':seed,'kind':'base','candidate_validator':'OK' if v.returncode==0 else 'FAIL','same':int(same)})
      total+=1
for mode,n in extra:
  case_id+=1
  inp=Path('correctness_tmp')/f'c{case_id}.in'
  cmd=['python3','gen_case.py','--mode',mode,'--n',str(n),'--seed','1','--shuffle-labels','--shuffle-queries']
  with open(inp,'w') as f: subprocess.run(cmd, check=True, stdout=f)
  ref=inp.with_suffix('.ref'); cand=inp.with_suffix('.cand')
  subprocess.run([solver_ref], stdin=open(inp), stdout=open(ref,'w'), stderr=subprocess.DEVNULL, check=True)
  subprocess.run([solver_new], stdin=open(inp), stdout=open(cand,'w'), stderr=subprocess.DEVNULL, check=True)
  v=subprocess.run(['python3','validator.py',str(inp),str(cand)], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
  same=(ref.read_text()==cand.read_text())
  if not same: mismatch+=1
  rows.append({'mode':mode,'n':n,'seed':1,'kind':'extra','candidate_validator':'OK' if v.returncode==0 else 'FAIL','same':int(same)})
  total+=1
with open(out_tsv,'w',newline='') as f:
  w=csv.DictWriter(f, fieldnames=['mode','n','seed','kind','candidate_validator','same'], delimiter='\t'); w.writeheader(); w.writerows(rows)
Path(out_json).write_text(json.dumps({'total_cases':total,'mismatch':mismatch,'all_same': mismatch==0}, indent=2))
