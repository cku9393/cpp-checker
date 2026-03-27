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
| balanced_dense | 0 | 0.816 | 1.760 | 0.462 | 0.522 | 50000:0.262, 99999:0.462 |
| caterpillar_rect_dense | 0 | 0.336 | 1.262 | 0.148 | 0.177 | 50000:0.118, 99999:0.148 |
| comb_dense | 0 | 0.400 | 1.319 | 0.130 | 0.150 | 50000:0.099, 99999:0.130 |
| comb_rect_dense | 0 | 0.700 | 1.625 | 0.164 | 0.186 | 50000:0.101, 99999:0.164 |
| multi_comb_cap | 0 | 0.063 | 1.045 | 0.147 | 0.211 | 50000:0.140, 99999:0.147 |
| multi_comb_rect | 0 | 0.854 | 1.808 | 0.168 | 0.170 | 50000:0.093, 99999:0.168 |
| random_recursive_mixed | 0 | -0.765 | 0.588 | 0.566 | 0.924 | 50000:0.566, 99999:0.333 |

## Top slow cases

| stage | mode | n | seed | sec | rss_kb | case_dir |
| --- | --- | --- | --- | --- | --- | --- |
| max_n_mix | random_recursive_mixed | 50000 | 1 | 0.924 | 22400 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/max_n_mix_probe_current/runs/max_n_mix/random_recursive_mixed/n50000/seed1_L1_Q1 |
| max_n_mix | balanced_dense | 99999 | 2 | 0.522 | 31840 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/max_n_mix_probe_current/runs/max_n_mix/balanced_dense/n99999/seed2_L1_Q1 |
| max_n_mix | balanced_dense | 99999 | 1 | 0.401 | 31808 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/max_n_mix_probe_current/runs/max_n_mix/balanced_dense/n99999/seed1_L1_Q1 |
| max_n_mix | random_recursive_mixed | 99999 | 1 | 0.334 | 33040 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/max_n_mix_probe_current/runs/max_n_mix/random_recursive_mixed/n99999/seed1_L1_Q1 |
| max_n_mix | random_recursive_mixed | 99999 | 2 | 0.332 | 29024 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/max_n_mix_probe_current/runs/max_n_mix/random_recursive_mixed/n99999/seed2_L1_Q1 |
| max_n_mix | balanced_dense | 50000 | 1 | 0.282 | 21680 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/max_n_mix_probe_current/runs/max_n_mix/balanced_dense/n50000/seed1_L1_Q1 |
| max_n_mix | balanced_dense | 50000 | 2 | 0.243 | 21728 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/max_n_mix_probe_current/runs/max_n_mix/balanced_dense/n50000/seed2_L1_Q1 |
| max_n_mix | multi_comb_cap | 50000 | 1 | 0.211 | 15056 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/max_n_mix_probe_current/runs/max_n_mix/multi_comb_cap/n50000/seed1_L1_Q1 |
| max_n_mix | random_recursive_mixed | 50000 | 2 | 0.209 | 20048 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/max_n_mix_probe_current/runs/max_n_mix/random_recursive_mixed/n50000/seed2_L1_Q1 |
| max_n_mix | comb_rect_dense | 99999 | 1 | 0.186 | 22640 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/max_n_mix_probe_current/runs/max_n_mix/comb_rect_dense/n99999/seed1_L1_Q1 |
| max_n_mix | caterpillar_rect_dense | 99999 | 2 | 0.177 | 28752 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/max_n_mix_probe_current/runs/max_n_mix/caterpillar_rect_dense/n99999/seed2_L1_Q1 |
| max_n_mix | multi_comb_rect | 99999 | 2 | 0.170 | 22000 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/max_n_mix_probe_current/runs/max_n_mix/multi_comb_rect/n99999/seed2_L1_Q1 |
| max_n_mix | multi_comb_rect | 99999 | 1 | 0.166 | 21984 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/max_n_mix_probe_current/runs/max_n_mix/multi_comb_rect/n99999/seed1_L1_Q1 |
| max_n_mix | comb_dense | 99999 | 2 | 0.150 | 21856 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/max_n_mix_probe_current/runs/max_n_mix/comb_dense/n99999/seed2_L1_Q1 |
| max_n_mix | multi_comb_cap | 99999 | 1 | 0.149 | 21904 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/max_n_mix_probe_current/runs/max_n_mix/multi_comb_cap/n99999/seed1_L1_Q1 |
| max_n_mix | multi_comb_cap | 99999 | 2 | 0.144 | 21904 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/max_n_mix_probe_current/runs/max_n_mix/multi_comb_cap/n99999/seed2_L1_Q1 |
| max_n_mix | caterpillar_rect_dense | 50000 | 2 | 0.143 | 19248 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/max_n_mix_probe_current/runs/max_n_mix/caterpillar_rect_dense/n50000/seed2_L1_Q1 |
| max_n_mix | comb_rect_dense | 99999 | 2 | 0.142 | 22640 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/max_n_mix_probe_current/runs/max_n_mix/comb_rect_dense/n99999/seed2_L1_Q1 |
| max_n_mix | caterpillar_rect_dense | 99999 | 1 | 0.120 | 28736 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/max_n_mix_probe_current/runs/max_n_mix/caterpillar_rect_dense/n99999/seed1_L1_Q1 |
| max_n_mix | comb_dense | 50000 | 2 | 0.114 | 15216 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/max_n_mix_probe_current/runs/max_n_mix/comb_dense/n50000/seed2_L1_Q1 |
