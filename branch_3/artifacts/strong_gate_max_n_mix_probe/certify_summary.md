# Certification summary

overall verdict: **PASS**

이 스위트는 hidden data를 완전히 복제하는 증명은 아니지만, decomposition 계열 느린 풀이와 잘못된 풀이를 매우 강하게 걸러내도록 설계됐다.

## Stage: max_n_mix

status: **PASS**  
cases: 28  
timeouts: 0  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| balanced_dense | 0 | 1.266 | 2.404 | 0.632 | 0.760 | 50000:0.263, 99999:0.632 |
| caterpillar_rect_dense | 0 | -0.398 | 0.759 | 0.155 | 0.185 | 50000:0.155, 99999:0.118 |
| comb_dense | 0 | 0.269 | 1.205 | 0.124 | 0.125 | 50000:0.103, 99999:0.124 |
| comb_rect_dense | 0 | 0.746 | 1.677 | 0.182 | 0.182 | 50000:0.108, 99999:0.182 |
| multi_comb_cap | 0 | 0.171 | 1.126 | 0.110 | 0.123 | 50000:0.098, 99999:0.110 |
| multi_comb_rect | 0 | 0.557 | 1.472 | 0.152 | 0.178 | 50000:0.104, 99999:0.152 |
| random_recursive_mixed | 0 | 0.632 | 1.549 | 0.333 | 0.350 | 50000:0.215, 99999:0.333 |

## Top slow cases

| stage | mode | n | seed | sec | rss_kb | case_dir |
| --- | --- | --- | --- | --- | --- | --- |
| max_n_mix | balanced_dense | 99999 | 1 | 0.760 | 31856 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/strong_gate_max_n_mix_probe/runs/max_n_mix/balanced_dense/n99999/seed1_L1_Q1 |
| max_n_mix | balanced_dense | 99999 | 2 | 0.504 | 31872 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/strong_gate_max_n_mix_probe/runs/max_n_mix/balanced_dense/n99999/seed2_L1_Q1 |
| max_n_mix | random_recursive_mixed | 99999 | 1 | 0.350 | 33056 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/strong_gate_max_n_mix_probe/runs/max_n_mix/random_recursive_mixed/n99999/seed1_L1_Q1 |
| max_n_mix | random_recursive_mixed | 99999 | 2 | 0.317 | 28992 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/strong_gate_max_n_mix_probe/runs/max_n_mix/random_recursive_mixed/n99999/seed2_L1_Q1 |
| max_n_mix | balanced_dense | 50000 | 2 | 0.294 | 21728 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/strong_gate_max_n_mix_probe/runs/max_n_mix/balanced_dense/n50000/seed2_L1_Q1 |
| max_n_mix | balanced_dense | 50000 | 1 | 0.232 | 21664 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/strong_gate_max_n_mix_probe/runs/max_n_mix/balanced_dense/n50000/seed1_L1_Q1 |
| max_n_mix | random_recursive_mixed | 50000 | 1 | 0.227 | 22448 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/strong_gate_max_n_mix_probe/runs/max_n_mix/random_recursive_mixed/n50000/seed1_L1_Q1 |
| max_n_mix | random_recursive_mixed | 50000 | 2 | 0.203 | 20048 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/strong_gate_max_n_mix_probe/runs/max_n_mix/random_recursive_mixed/n50000/seed2_L1_Q1 |
| max_n_mix | caterpillar_rect_dense | 50000 | 2 | 0.185 | 19232 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/strong_gate_max_n_mix_probe/runs/max_n_mix/caterpillar_rect_dense/n50000/seed2_L1_Q1 |
| max_n_mix | comb_rect_dense | 99999 | 2 | 0.182 | 22640 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/strong_gate_max_n_mix_probe/runs/max_n_mix/comb_rect_dense/n99999/seed2_L1_Q1 |
| max_n_mix | comb_rect_dense | 99999 | 1 | 0.182 | 22640 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/strong_gate_max_n_mix_probe/runs/max_n_mix/comb_rect_dense/n99999/seed1_L1_Q1 |
| max_n_mix | multi_comb_rect | 99999 | 1 | 0.178 | 21984 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/strong_gate_max_n_mix_probe/runs/max_n_mix/multi_comb_rect/n99999/seed1_L1_Q1 |
| max_n_mix | multi_comb_rect | 99999 | 2 | 0.126 | 22000 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/strong_gate_max_n_mix_probe/runs/max_n_mix/multi_comb_rect/n99999/seed2_L1_Q1 |
| max_n_mix | caterpillar_rect_dense | 99999 | 1 | 0.126 | 28736 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/strong_gate_max_n_mix_probe/runs/max_n_mix/caterpillar_rect_dense/n99999/seed1_L1_Q1 |
| max_n_mix | caterpillar_rect_dense | 50000 | 1 | 0.126 | 19248 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/strong_gate_max_n_mix_probe/runs/max_n_mix/caterpillar_rect_dense/n50000/seed1_L1_Q1 |
| max_n_mix | comb_dense | 99999 | 2 | 0.125 | 21872 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/strong_gate_max_n_mix_probe/runs/max_n_mix/comb_dense/n99999/seed2_L1_Q1 |
| max_n_mix | multi_comb_cap | 99999 | 1 | 0.123 | 21904 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/strong_gate_max_n_mix_probe/runs/max_n_mix/multi_comb_cap/n99999/seed1_L1_Q1 |
| max_n_mix | comb_dense | 99999 | 1 | 0.122 | 21856 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/strong_gate_max_n_mix_probe/runs/max_n_mix/comb_dense/n99999/seed1_L1_Q1 |
| max_n_mix | comb_rect_dense | 50000 | 2 | 0.116 | 15264 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/strong_gate_max_n_mix_probe/runs/max_n_mix/comb_rect_dense/n50000/seed2_L1_Q1 |
| max_n_mix | multi_comb_rect | 50000 | 1 | 0.112 | 15232 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/strong_gate_max_n_mix_probe/runs/max_n_mix/multi_comb_rect/n50000/seed1_L1_Q1 |
