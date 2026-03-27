# Certification summary

overall verdict: **FAIL**

## Reasons

- hard_scaling: 23 failing cases
- hard_scaling: multi_comb_cap: ratio=7.977 > 2.900

이 스위트는 hidden data를 완전히 복제하는 증명은 아니지만, decomposition 계열 느린 풀이와 잘못된 풀이를 매우 강하게 걸러내도록 설계됐다.

## Stage: hard_scaling

status: **FAIL**  
cases: 108  
timeouts: 23  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| caterpillar_mixed | 11 | - | - | 10.764 | 10.764 | 4096:10.764 |
| caterpillar_rect_dense | 12 | - | - | - | - | - |
| comb_core | 0 | 0.299 | 2.010 | 0.025 | 0.504 | 4096:0.013, 8192:0.012, 16384:0.012, 32768:0.025 |
| comb_dense | 0 | 0.220 | 1.366 | 0.061 | 0.577 | 4096:0.038, 8192:0.051, 16384:0.061, 32768:0.059 |
| comb_plus_unary | 0 | 0.281 | 1.891 | 0.024 | 0.024 | 4096:0.012, 8192:0.013, 16384:0.012, 32768:0.024 |
| comb_rect_dense | 0 | 0.009 | 1.019 | 0.049 | 0.069 | 4096:0.048, 8192:0.047, 16384:0.048, 32768:0.049 |
| multi_comb_cap | 0 | 1.002 | 7.977 | 0.386 | 0.607 | 4096:0.038, 8192:0.050, 16384:0.048, 32768:0.386 |
| multi_comb_core | 0 | 0.377 | 2.041 | 0.025 | 0.025 | 4096:0.011, 8192:0.011, 16384:0.012, 32768:0.025 |
| multi_comb_rect | 0 | 0.227 | 1.342 | 0.063 | 0.531 | 4096:0.037, 8192:0.048, 16384:0.047, 32768:0.063 |

Scale check hits:

- multi_comb_cap: ratio=7.977 > 2.900

## Top slow cases

| stage | mode | n | seed | sec | rss_kb | case_dir |
| --- | --- | --- | --- | --- | --- | --- |
| hard_scaling | caterpillar_mixed | 4096 | 3 | 10.764 | 10992 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_retry1/runs/hard_scaling/caterpillar_mixed/n4096/seed3_L1_Q1 |
| hard_scaling | multi_comb_cap | 32768 | 1 | 0.607 | 12736 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_retry1/runs/hard_scaling/multi_comb_cap/n32768/seed1_L1_Q1 |
| hard_scaling | comb_dense | 8192 | 2 | 0.577 | 9680 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_retry1/runs/hard_scaling/comb_dense/n8192/seed2_L1_Q1 |
| hard_scaling | multi_comb_rect | 32768 | 3 | 0.531 | 12896 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_retry1/runs/hard_scaling/multi_comb_rect/n32768/seed3_L1_Q1 |
| hard_scaling | comb_core | 4096 | 1 | 0.504 | 2640 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_retry1/runs/hard_scaling/comb_core/n4096/seed1_L1_Q1 |
| hard_scaling | multi_comb_cap | 32768 | 2 | 0.386 | 12752 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_retry1/runs/hard_scaling/multi_comb_cap/n32768/seed2_L1_Q1 |
| hard_scaling | comb_dense | 32768 | 3 | 0.077 | 12944 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_retry1/runs/hard_scaling/comb_dense/n32768/seed3_L1_Q1 |
| hard_scaling | comb_rect_dense | 16384 | 1 | 0.069 | 10912 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_retry1/runs/hard_scaling/comb_rect_dense/n16384/seed1_L1_Q1 |
| hard_scaling | multi_comb_rect | 8192 | 3 | 0.064 | 9648 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_retry1/runs/hard_scaling/multi_comb_rect/n8192/seed3_L1_Q1 |
| hard_scaling | multi_comb_rect | 32768 | 1 | 0.063 | 12912 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_retry1/runs/hard_scaling/multi_comb_rect/n32768/seed1_L1_Q1 |
| hard_scaling | comb_dense | 16384 | 1 | 0.061 | 10768 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_retry1/runs/hard_scaling/comb_dense/n16384/seed1_L1_Q1 |
| hard_scaling | comb_dense | 16384 | 2 | 0.061 | 10768 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_retry1/runs/hard_scaling/comb_dense/n16384/seed2_L1_Q1 |
| hard_scaling | comb_dense | 32768 | 2 | 0.059 | 12944 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_retry1/runs/hard_scaling/comb_dense/n32768/seed2_L1_Q1 |
| hard_scaling | comb_dense | 32768 | 1 | 0.059 | 12944 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_retry1/runs/hard_scaling/comb_dense/n32768/seed1_L1_Q1 |
| hard_scaling | multi_comb_cap | 32768 | 3 | 0.057 | 12736 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_retry1/runs/hard_scaling/multi_comb_cap/n32768/seed3_L1_Q1 |
| hard_scaling | comb_dense | 8192 | 1 | 0.051 | 9680 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_retry1/runs/hard_scaling/comb_dense/n8192/seed1_L1_Q1 |
| hard_scaling | multi_comb_cap | 8192 | 1 | 0.050 | 9584 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_retry1/runs/hard_scaling/multi_comb_cap/n8192/seed1_L1_Q1 |
| hard_scaling | multi_comb_cap | 16384 | 2 | 0.050 | 10592 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_retry1/runs/hard_scaling/multi_comb_cap/n16384/seed2_L1_Q1 |
| hard_scaling | comb_rect_dense | 32768 | 3 | 0.050 | 13024 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_retry1/runs/hard_scaling/comb_rect_dense/n32768/seed3_L1_Q1 |
| hard_scaling | multi_comb_cap | 8192 | 2 | 0.050 | 9568 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_retry1/runs/hard_scaling/multi_comb_cap/n8192/seed2_L1_Q1 |
