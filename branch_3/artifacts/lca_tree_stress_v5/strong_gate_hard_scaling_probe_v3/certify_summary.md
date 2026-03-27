# Certification summary

overall verdict: **FAIL**

## Reasons

- hard_scaling: 24 failing cases

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
| comb_core | 0 | 0.184 | 2.015 | 0.025 | 0.038 | 4096:0.021, 8192:0.013, 16384:0.025, 32768:0.025 |
| comb_dense | 0 | 0.283 | 1.460 | 0.086 | 0.644 | 4096:0.047, 8192:0.050, 16384:0.059, 32768:0.086 |
| comb_plus_unary | 0 | 0.572 | 2.182 | 0.036 | 0.063 | 4096:0.013, 8192:0.012, 16384:0.025, 32768:0.036 |
| comb_rect_dense | 0 | 0.259 | 1.242 | 0.084 | 0.116 | 4096:0.049, 8192:0.061, 16384:0.074, 32768:0.084 |
| multi_comb_cap | 0 | 0.166 | 1.333 | 0.050 | 0.060 | 4096:0.038, 8192:0.038, 16384:0.050, 32768:0.050 |
| multi_comb_core | 0 | 0.273 | 1.875 | 0.024 | 0.036 | 4096:0.013, 8192:0.013, 16384:0.013, 32768:0.024 |
| multi_comb_rect | 0 | 0.102 | 1.236 | 0.062 | 0.079 | 4096:0.048, 8192:0.053, 16384:0.050, 32768:0.062 |

## Top slow cases

| stage | mode | n | seed | sec | rss_kb | case_dir |
| --- | --- | --- | --- | --- | --- | --- |
| hard_scaling | comb_dense | 4096 | 3 | 0.644 | 9136 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_probe_v3/runs/hard_scaling/comb_dense/n4096/seed3_L1_Q1 |
| hard_scaling | comb_rect_dense | 32768 | 1 | 0.116 | 13040 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_probe_v3/runs/hard_scaling/comb_rect_dense/n32768/seed1_L1_Q1 |
| hard_scaling | comb_dense | 32768 | 2 | 0.089 | 12944 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_probe_v3/runs/hard_scaling/comb_dense/n32768/seed2_L1_Q1 |
| hard_scaling | comb_dense | 32768 | 1 | 0.086 | 12960 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_probe_v3/runs/hard_scaling/comb_dense/n32768/seed1_L1_Q1 |
| hard_scaling | comb_rect_dense | 32768 | 2 | 0.084 | 13040 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_probe_v3/runs/hard_scaling/comb_rect_dense/n32768/seed2_L1_Q1 |
| hard_scaling | multi_comb_rect | 8192 | 2 | 0.079 | 9648 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_probe_v3/runs/hard_scaling/multi_comb_rect/n8192/seed2_L1_Q1 |
| hard_scaling | comb_rect_dense | 16384 | 2 | 0.074 | 10896 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_probe_v3/runs/hard_scaling/comb_rect_dense/n16384/seed2_L1_Q1 |
| hard_scaling | comb_rect_dense | 16384 | 1 | 0.074 | 10896 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_probe_v3/runs/hard_scaling/comb_rect_dense/n16384/seed1_L1_Q1 |
| hard_scaling | comb_dense | 32768 | 3 | 0.072 | 12944 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_probe_v3/runs/hard_scaling/comb_dense/n32768/seed3_L1_Q1 |
| hard_scaling | comb_rect_dense | 32768 | 3 | 0.070 | 13040 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_probe_v3/runs/hard_scaling/comb_rect_dense/n32768/seed3_L1_Q1 |
| hard_scaling | comb_rect_dense | 16384 | 3 | 0.069 | 10880 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_probe_v3/runs/hard_scaling/comb_rect_dense/n16384/seed3_L1_Q1 |
| hard_scaling | comb_dense | 16384 | 1 | 0.068 | 10768 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_probe_v3/runs/hard_scaling/comb_dense/n16384/seed1_L1_Q1 |
| hard_scaling | comb_plus_unary | 32768 | 3 | 0.063 | 8880 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_probe_v3/runs/hard_scaling/comb_plus_unary/n32768/seed3_L1_Q1 |
| hard_scaling | multi_comb_rect | 32768 | 3 | 0.063 | 12912 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_probe_v3/runs/hard_scaling/multi_comb_rect/n32768/seed3_L1_Q1 |
| hard_scaling | multi_comb_rect | 16384 | 3 | 0.062 | 10704 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_probe_v3/runs/hard_scaling/multi_comb_rect/n16384/seed3_L1_Q1 |
| hard_scaling | multi_comb_rect | 32768 | 1 | 0.062 | 12896 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_probe_v3/runs/hard_scaling/multi_comb_rect/n32768/seed1_L1_Q1 |
| hard_scaling | comb_rect_dense | 8192 | 3 | 0.062 | 9840 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_probe_v3/runs/hard_scaling/comb_rect_dense/n8192/seed3_L1_Q1 |
| hard_scaling | comb_rect_dense | 8192 | 1 | 0.061 | 9840 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_probe_v3/runs/hard_scaling/comb_rect_dense/n8192/seed1_L1_Q1 |
| hard_scaling | comb_rect_dense | 8192 | 2 | 0.060 | 9840 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_probe_v3/runs/hard_scaling/comb_rect_dense/n8192/seed2_L1_Q1 |
| hard_scaling | multi_comb_rect | 32768 | 2 | 0.060 | 12896 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_probe_v3/runs/hard_scaling/multi_comb_rect/n32768/seed2_L1_Q1 |
