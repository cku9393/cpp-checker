# Certification summary

overall verdict: **FAIL**

## Reasons

- boj_3s_large_mix: 3 failing cases

이 스위트는 hidden data를 완전히 복제하는 증명은 아니지만, decomposition 계열 느린 풀이와 잘못된 풀이를 매우 강하게 걸러내도록 설계됐다.

## Stage: correctness_smoke

status: **PASS**  
cases: 288  
timeouts: 0  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| balanced_dense | 0 | -0.005 | 1.064 | 0.015 | 0.028 | 128:0.015, 256:0.015, 512:0.014, 1024:0.015 |
| caterpillar_rect_dense | 0 | 0.471 | 3.497 | 0.127 | 0.143 | 128:0.015, 256:0.036, 512:0.127, 1024:0.029 |
| comb_rect_dense | 0 | 0.230 | 1.700 | 0.026 | 0.498 | 128:0.015, 256:0.015, 512:0.015, 1024:0.026 |
| multi_comb_cap | 0 | 0.135 | 2.551 | 0.038 | 0.046 | 128:0.015, 256:0.015, 512:0.038, 1024:0.015 |
| multi_comb_rect | 0 | 0.106 | 2.082 | 0.054 | 0.060 | 128:0.015, 256:0.026, 512:0.054, 1024:0.015 |
| random_recursive_mixed | 0 | 0.244 | 2.118 | 0.026 | 0.034 | 128:0.014, 256:0.014, 512:0.012, 1024:0.026 |

## Stage: hard_scaling_strict

status: **PASS**  
cases: 108  
timeouts: 0  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| caterpillar_mixed | 0 | 0.232 | 1.298 | 0.060 | 0.076 | 4096:0.038, 8192:0.042, 16384:0.054, 32768:0.060 |
| caterpillar_rect_dense | 0 | 0.288 | 1.271 | 0.072 | 0.176 | 4096:0.040, 8192:0.051, 16384:0.063, 32768:0.072 |
| comb_core | 0 | 0.359 | 1.724 | 0.028 | 0.031 | 4096:0.012, 8192:0.015, 16384:0.016, 32768:0.028 |
| comb_dense | 0 | 0.100 | 1.242 | 0.058 | 0.061 | 4096:0.049, 8192:0.043, 16384:0.053, 32768:0.058 |
| comb_plus_unary | 0 | 0.395 | 1.692 | 0.027 | 0.029 | 4096:0.012, 8192:0.012, 16384:0.016, 32768:0.027 |
| comb_rect_dense | 0 | 0.204 | 1.436 | 0.061 | 0.061 | 4096:0.039, 8192:0.041, 16384:0.043, 32768:0.061 |
| multi_comb_cap | 0 | 0.155 | 1.204 | 0.061 | 0.076 | 4096:0.045, 8192:0.042, 16384:0.051, 32768:0.061 |
| multi_comb_core | 0 | 0.311 | 2.313 | 0.026 | 0.030 | 4096:0.012, 8192:0.013, 16384:0.011, 32768:0.026 |
| multi_comb_rect | 0 | 0.123 | 1.316 | 0.059 | 0.060 | 4096:0.044, 8192:0.045, 16384:0.045, 32768:0.059 |

## Stage: boj_3s_large_adversarial

status: **PASS**  
cases: 30  
timeouts: 0  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| caterpillar_rect_dense | 0 | 0.657 | 1.304 | 0.115 | 0.127 | 50000:0.073, 75000:0.095, 99999:0.115 |
| comb_dense | 0 | 0.033 | 1.212 | 0.076 | 0.077 | 50000:0.073, 75000:0.063, 99999:0.076 |
| comb_rect_dense | 0 | 0.392 | 1.195 | 0.087 | 0.089 | 50000:0.067, 75000:0.080, 99999:0.087 |
| multi_comb_cap | 0 | 0.274 | 1.350 | 0.100 | 0.114 | 50000:0.074, 75000:0.100, 99999:0.087 |
| multi_comb_rect | 0 | 0.920 | 1.718 | 0.119 | 0.128 | 50000:0.061, 75000:0.069, 99999:0.119 |

## Stage: boj_3s_large_mix

status: **FAIL**  
cases: 18  
timeouts: 3  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| balanced_dense | 0 | - | - | 0.444 | 0.460 | 99999:0.444 |
| broom_mixed | 3 | - | - | - | - | - |
| caterpillar_rect_dense | 0 | - | - | 0.101 | 0.204 | 99999:0.101 |
| comb_rect_dense | 0 | - | - | 0.123 | 0.183 | 99999:0.123 |
| multi_comb_cap | 0 | - | - | 0.071 | 0.118 | 99999:0.071 |
| random_recursive_mixed | 0 | - | - | 0.251 | 0.256 | 99999:0.251 |

## Top slow cases

| stage | mode | n | seed | sec | rss_kb | case_dir |
| --- | --- | --- | --- | --- | --- | --- |
| correctness_smoke | comb_rect_dense | 128 | 1 | 0.498 | 2320 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/ac4_probe_fresh/runs/correctness_smoke/comb_rect_dense/n128/seed1_L0_Q0 |
| boj_3s_large_mix | balanced_dense | 99999 | 2 | 0.460 | 31984 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/ac4_probe_fresh/runs/boj_3s_large_mix/balanced_dense/n99999/seed2_L1_Q1 |
| boj_3s_large_mix | balanced_dense | 99999 | 1 | 0.444 | 31968 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/ac4_probe_fresh/runs/boj_3s_large_mix/balanced_dense/n99999/seed1_L1_Q1 |
| boj_3s_large_mix | balanced_dense | 99999 | 3 | 0.388 | 32000 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/ac4_probe_fresh/runs/boj_3s_large_mix/balanced_dense/n99999/seed3_L1_Q1 |
| correctness_smoke | comb_rect_dense | 128 | 3 | 0.267 | 2320 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/ac4_probe_fresh/runs/correctness_smoke/comb_rect_dense/n128/seed3_L0_Q0 |
| boj_3s_large_mix | random_recursive_mixed | 99999 | 3 | 0.256 | 30912 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/ac4_probe_fresh/runs/boj_3s_large_mix/random_recursive_mixed/n99999/seed3_L1_Q1 |
| boj_3s_large_mix | random_recursive_mixed | 99999 | 1 | 0.251 | 33104 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/ac4_probe_fresh/runs/boj_3s_large_mix/random_recursive_mixed/n99999/seed1_L1_Q1 |
| boj_3s_large_mix | random_recursive_mixed | 99999 | 2 | 0.237 | 29168 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/ac4_probe_fresh/runs/boj_3s_large_mix/random_recursive_mixed/n99999/seed2_L1_Q1 |
| boj_3s_large_mix | caterpillar_rect_dense | 99999 | 2 | 0.204 | 28784 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/ac4_probe_fresh/runs/boj_3s_large_mix/caterpillar_rect_dense/n99999/seed2_L1_Q1 |
| boj_3s_large_mix | comb_rect_dense | 99999 | 2 | 0.183 | 23072 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/ac4_probe_fresh/runs/boj_3s_large_mix/comb_rect_dense/n99999/seed2_L1_Q1 |
| hard_scaling_strict | caterpillar_rect_dense | 16384 | 1 | 0.176 | 12608 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/ac4_probe_fresh/runs/hard_scaling_strict/caterpillar_rect_dense/n16384/seed1_L1_Q1 |
| hard_scaling_strict | caterpillar_rect_dense | 8192 | 1 | 0.158 | 11024 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/ac4_probe_fresh/runs/hard_scaling_strict/caterpillar_rect_dense/n8192/seed1_L1_Q1 |
| hard_scaling_strict | caterpillar_rect_dense | 4096 | 2 | 0.144 | 10272 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/ac4_probe_fresh/runs/hard_scaling_strict/caterpillar_rect_dense/n4096/seed2_L1_Q1 |
| correctness_smoke | caterpillar_rect_dense | 512 | 2 | 0.143 | 17088 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/ac4_probe_fresh/runs/correctness_smoke/caterpillar_rect_dense/n512/seed2_L1_Q1 |
| correctness_smoke | caterpillar_rect_dense | 512 | 1 | 0.135 | 17232 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/ac4_probe_fresh/runs/correctness_smoke/caterpillar_rect_dense/n512/seed1_L0_Q1 |
| correctness_smoke | caterpillar_rect_dense | 512 | 3 | 0.135 | 17216 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/ac4_probe_fresh/runs/correctness_smoke/caterpillar_rect_dense/n512/seed3_L1_Q1 |
| correctness_smoke | caterpillar_rect_dense | 512 | 1 | 0.135 | 17232 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/ac4_probe_fresh/runs/correctness_smoke/caterpillar_rect_dense/n512/seed1_L1_Q1 |
| correctness_smoke | caterpillar_rect_dense | 512 | 2 | 0.129 | 17136 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/ac4_probe_fresh/runs/correctness_smoke/caterpillar_rect_dense/n512/seed2_L0_Q1 |
| correctness_smoke | caterpillar_rect_dense | 512 | 3 | 0.128 | 17232 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/ac4_probe_fresh/runs/correctness_smoke/caterpillar_rect_dense/n512/seed3_L0_Q0 |
| boj_3s_large_adversarial | multi_comb_rect | 99999 | 1 | 0.128 | 22048 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/ac4_probe_fresh/runs/boj_3s_large_adversarial/multi_comb_rect/n99999/seed1_L1_Q1 |
