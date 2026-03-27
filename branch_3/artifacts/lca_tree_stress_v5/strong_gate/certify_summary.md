# Certification summary

overall verdict: **PASS**

이 스위트는 hidden data를 완전히 복제하는 증명은 아니지만, decomposition 계열 느린 풀이와 잘못된 풀이를 매우 강하게 걸러내도록 설계됐다.

## Stage: correctness_fuzz

status: **PASS**  
cases: 900  
timeouts: 0  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| balanced_dense | 0 | -0.018 | 1.001 | 0.013 | 0.025 | 64:0.013, 128:0.013, 256:0.013, 512:0.013, 1024:0.013 |
| broom_mixed | 0 | 0.001 | 1.004 | 0.013 | 0.013 | 64:0.013, 128:0.013, 256:0.013, 512:0.013, 1024:0.013 |
| caterpillar_rect_dense | 0 | 0.497 | 3.957 | 0.099 | 0.113 | 64:0.013, 128:0.013, 256:0.025, 512:0.099, 1024:0.025 |
| chain_unary | 0 | 0.026 | 1.091 | 0.015 | 0.026 | 64:0.014, 128:0.015, 256:0.015, 512:0.015, 1024:0.015 |
| comb_rect_dense | 0 | 0.312 | 3.114 | 0.118 | 0.133 | 64:0.014, 128:0.015, 256:0.038, 512:0.118, 1024:0.015 |
| multi_comb_cap | 0 | 0.098 | 1.940 | 0.029 | 0.030 | 64:0.015, 128:0.015, 256:0.015, 512:0.029, 1024:0.015 |
| multi_comb_rect | 0 | 0.154 | 2.872 | 0.043 | 0.052 | 64:0.015, 128:0.014, 256:0.015, 512:0.043, 1024:0.015 |
| random_recursive_mixed | 0 | 0.000 | 1.000 | 0.013 | 0.025 | 64:0.013, 128:0.013, 256:0.013, 512:0.013, 1024:0.013 |
| star_pairs | 0 | 0.020 | 1.051 | 0.015 | 0.015 | 64:0.014, 128:0.015, 256:0.014, 512:0.015, 1024:0.015 |

## Stage: hard_scaling

status: **PASS**  
cases: 108  
timeouts: 0  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| caterpillar_mixed | 0 | 0.227 | 1.434 | 0.038 | 0.038 | 4096:0.025, 8192:0.025, 16384:0.036, 32768:0.038 |
| caterpillar_rect_dense | 0 | 0.274 | 1.379 | 0.042 | 0.044 | 4096:0.025, 8192:0.029, 16384:0.040, 32768:0.042 |
| comb_core | 0 | 0.001 | 1.001 | 0.013 | 0.013 | 4096:0.013, 8192:0.013, 16384:0.013, 32768:0.013 |
| comb_dense | 0 | 0.220 | 1.480 | 0.037 | 0.038 | 4096:0.025, 8192:0.025, 16384:0.037, 32768:0.037 |
| comb_plus_unary | 0 | 0.299 | 1.998 | 0.025 | 0.025 | 4096:0.013, 8192:0.013, 16384:0.013, 32768:0.025 |
| comb_rect_dense | 0 | 0.234 | 1.497 | 0.038 | 0.038 | 4096:0.025, 8192:0.025, 16384:0.038, 32768:0.038 |
| multi_comb_cap | 0 | 0.229 | 1.476 | 0.038 | 0.038 | 4096:0.025, 8192:0.024, 16384:0.036, 32768:0.038 |
| multi_comb_core | 0 | 0.286 | 2.208 | 0.025 | 0.025 | 4096:0.013, 8192:0.013, 16384:0.011, 32768:0.025 |
| multi_comb_rect | 0 | 0.202 | 1.395 | 0.036 | 0.036 | 4096:0.025, 8192:0.025, 16384:0.035, 32768:0.036 |

## Stage: max_n_mix

status: **PASS**  
cases: 28  
timeouts: 0  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| balanced_dense | 0 | 0.763 | 1.697 | 0.228 | 0.234 | 50000:0.134, 99999:0.228 |
| caterpillar_rect_dense | 0 | 0.362 | 1.285 | 0.052 | 0.055 | 50000:0.041, 99999:0.052 |
| comb_dense | 0 | 0.341 | 1.267 | 0.054 | 0.056 | 50000:0.043, 99999:0.054 |
| comb_rect_dense | 0 | 1.053 | 2.075 | 0.081 | 0.090 | 50000:0.039, 99999:0.081 |
| multi_comb_cap | 0 | 0.733 | 1.662 | 0.058 | 0.058 | 50000:0.035, 99999:0.058 |
| multi_comb_rect | 0 | 0.579 | 1.493 | 0.054 | 0.058 | 50000:0.036, 99999:0.054 |
| random_recursive_mixed | 0 | 0.572 | 1.486 | 0.156 | 0.159 | 50000:0.105, 99999:0.156 |

## Top slow cases

| stage | mode | n | seed | sec | rss_kb | case_dir |
| --- | --- | --- | --- | --- | --- | --- |
| max_n_mix | balanced_dense | 99999 | 2 | 0.234 | 31824 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/max_n_mix/balanced_dense/n99999/seed2_L1_Q1 |
| max_n_mix | balanced_dense | 99999 | 1 | 0.222 | 31840 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/max_n_mix/balanced_dense/n99999/seed1_L1_Q1 |
| max_n_mix | random_recursive_mixed | 99999 | 1 | 0.159 | 33072 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/max_n_mix/random_recursive_mixed/n99999/seed1_L1_Q1 |
| max_n_mix | random_recursive_mixed | 99999 | 2 | 0.153 | 29088 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/max_n_mix/random_recursive_mixed/n99999/seed2_L1_Q1 |
| max_n_mix | balanced_dense | 50000 | 1 | 0.142 | 21776 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/max_n_mix/balanced_dense/n50000/seed1_L1_Q1 |
| correctness_fuzz | comb_rect_dense | 512 | 5 | 0.133 | 17824 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n512/seed5_L0_Q1 |
| correctness_fuzz | comb_rect_dense | 512 | 1 | 0.128 | 17744 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n512/seed1_L1_Q1 |
| max_n_mix | balanced_dense | 50000 | 2 | 0.126 | 21840 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/max_n_mix/balanced_dense/n50000/seed2_L1_Q1 |
| correctness_fuzz | comb_rect_dense | 512 | 5 | 0.124 | 17808 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n512/seed5_L1_Q1 |
| correctness_fuzz | comb_rect_dense | 512 | 1 | 0.123 | 17680 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n512/seed1_L0_Q1 |
| correctness_fuzz | comb_rect_dense | 512 | 4 | 0.123 | 17600 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n512/seed4_L1_Q1 |
| correctness_fuzz | comb_rect_dense | 512 | 2 | 0.122 | 17600 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n512/seed2_L1_Q0 |
| correctness_fuzz | comb_rect_dense | 512 | 4 | 0.122 | 17584 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n512/seed4_L0_Q1 |
| correctness_fuzz | comb_rect_dense | 512 | 2 | 0.121 | 17648 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n512/seed2_L0_Q0 |
| correctness_fuzz | comb_rect_dense | 512 | 3 | 0.120 | 17680 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n512/seed3_L0_Q1 |
| correctness_fuzz | comb_rect_dense | 512 | 3 | 0.119 | 17680 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n512/seed3_L0_Q0 |
| correctness_fuzz | comb_rect_dense | 512 | 2 | 0.118 | 17664 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n512/seed2_L1_Q1 |
| correctness_fuzz | comb_rect_dense | 512 | 3 | 0.118 | 17744 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n512/seed3_L1_Q0 |
| correctness_fuzz | comb_rect_dense | 512 | 2 | 0.117 | 17664 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n512/seed2_L0_Q1 |
| correctness_fuzz | comb_rect_dense | 512 | 5 | 0.117 | 17856 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n512/seed5_L1_Q0 |
