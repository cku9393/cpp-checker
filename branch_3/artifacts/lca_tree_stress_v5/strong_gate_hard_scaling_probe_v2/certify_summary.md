# Certification summary

overall verdict: **FAIL**

## Reasons

- hard_scaling: 24 failing cases
- hard_scaling: comb_dense: alpha=1.724 > 1.450
- hard_scaling: comb_dense: ratio=8.433 > 2.900

이 스위트는 hidden data를 완전히 복제하는 증명은 아니지만, decomposition 계열 느린 풀이와 잘못된 풀이를 매우 강하게 걸러내도록 설계됐다.

## Stage: hard_scaling

status: **FAIL**  
cases: 108  
timeouts: 24  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| caterpillar_mixed | 12 | - | - | - | - | - |
| caterpillar_rect_dense | 12 | - | - | - | - | - |
| comb_core | 0 | 0.293 | 1.957 | 0.025 | 0.025 | 4096:0.013, 8192:0.013, 16384:0.013, 32768:0.025 |
| comb_dense | 0 | 1.724 | 8.433 | 1.272 | 2.109 | 4096:0.048, 8192:0.049, 16384:0.414, 32768:1.272 |
| comb_plus_unary | 0 | 0.571 | 1.949 | 0.038 | 0.065 | 4096:0.013, 8192:0.012, 16384:0.023, 32768:0.038 |
| comb_rect_dense | 0 | 0.257 | 1.396 | 0.086 | 0.117 | 4096:0.051, 8192:0.050, 16384:0.061, 32768:0.086 |
| multi_comb_cap | 0 | 0.209 | 1.287 | 0.061 | 0.081 | 4096:0.038, 8192:0.049, 16384:0.050, 32768:0.061 |
| multi_comb_core | 0 | 0.399 | 1.990 | 0.025 | 0.039 | 4096:0.013, 8192:0.013, 16384:0.025, 32768:0.025 |
| multi_comb_rect | 0 | 0.293 | 1.430 | 0.089 | 0.236 | 4096:0.049, 8192:0.050, 16384:0.062, 32768:0.089 |

Scale check hits:

- comb_dense: alpha=1.724 > 1.450
- comb_dense: ratio=8.433 > 2.900

## Top slow cases

| stage | mode | n | seed | sec | rss_kb | case_dir |
| --- | --- | --- | --- | --- | --- | --- |
| hard_scaling | comb_dense | 32768 | 3 | 2.109 | 12944 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_probe_v2/runs/hard_scaling/comb_dense/n32768/seed3_L1_Q1 |
| hard_scaling | comb_dense | 32768 | 2 | 1.272 | 12944 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_probe_v2/runs/hard_scaling/comb_dense/n32768/seed2_L1_Q1 |
| hard_scaling | comb_dense | 32768 | 1 | 1.133 | 12960 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_probe_v2/runs/hard_scaling/comb_dense/n32768/seed1_L1_Q1 |
| hard_scaling | comb_dense | 16384 | 2 | 0.692 | 10784 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_probe_v2/runs/hard_scaling/comb_dense/n16384/seed2_L1_Q1 |
| hard_scaling | comb_dense | 16384 | 3 | 0.414 | 10784 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_probe_v2/runs/hard_scaling/comb_dense/n16384/seed3_L1_Q1 |
| hard_scaling | multi_comb_rect | 32768 | 2 | 0.236 | 12896 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_probe_v2/runs/hard_scaling/multi_comb_rect/n32768/seed2_L1_Q1 |
| hard_scaling | comb_rect_dense | 16384 | 3 | 0.117 | 10896 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_probe_v2/runs/hard_scaling/comb_rect_dense/n16384/seed3_L1_Q1 |
| hard_scaling | comb_rect_dense | 32768 | 1 | 0.095 | 13040 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_probe_v2/runs/hard_scaling/comb_rect_dense/n32768/seed1_L1_Q1 |
| hard_scaling | multi_comb_rect | 32768 | 1 | 0.089 | 12896 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_probe_v2/runs/hard_scaling/multi_comb_rect/n32768/seed1_L1_Q1 |
| hard_scaling | multi_comb_rect | 32768 | 3 | 0.087 | 12912 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_probe_v2/runs/hard_scaling/multi_comb_rect/n32768/seed3_L1_Q1 |
| hard_scaling | comb_rect_dense | 32768 | 2 | 0.086 | 13024 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_probe_v2/runs/hard_scaling/comb_rect_dense/n32768/seed2_L1_Q1 |
| hard_scaling | multi_comb_cap | 4096 | 1 | 0.081 | 9088 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_probe_v2/runs/hard_scaling/multi_comb_cap/n4096/seed1_L1_Q1 |
| hard_scaling | multi_comb_rect | 16384 | 1 | 0.074 | 10704 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_probe_v2/runs/hard_scaling/multi_comb_rect/n16384/seed1_L1_Q1 |
| hard_scaling | comb_rect_dense | 32768 | 3 | 0.073 | 13024 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_probe_v2/runs/hard_scaling/comb_rect_dense/n32768/seed3_L1_Q1 |
| hard_scaling | comb_plus_unary | 32768 | 3 | 0.065 | 8880 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_probe_v2/runs/hard_scaling/comb_plus_unary/n32768/seed3_L1_Q1 |
| hard_scaling | comb_dense | 16384 | 1 | 0.063 | 10768 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_probe_v2/runs/hard_scaling/comb_dense/n16384/seed1_L1_Q1 |
| hard_scaling | multi_comb_rect | 16384 | 3 | 0.062 | 10704 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_probe_v2/runs/hard_scaling/multi_comb_rect/n16384/seed3_L1_Q1 |
| hard_scaling | multi_comb_rect | 8192 | 3 | 0.062 | 9648 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_probe_v2/runs/hard_scaling/multi_comb_rect/n8192/seed3_L1_Q1 |
| hard_scaling | comb_rect_dense | 16384 | 1 | 0.061 | 10896 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_probe_v2/runs/hard_scaling/comb_rect_dense/n16384/seed1_L1_Q1 |
| hard_scaling | multi_comb_cap | 32768 | 1 | 0.061 | 12752 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_probe_v2/runs/hard_scaling/multi_comb_cap/n32768/seed1_L1_Q1 |
