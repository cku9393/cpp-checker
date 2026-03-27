# Certification summary

overall verdict: **FAIL**

## Reasons

- hard_scaling: 12 failing cases

이 스위트는 hidden data를 완전히 복제하는 증명은 아니지만, decomposition 계열 느린 풀이와 잘못된 풀이를 매우 강하게 걸러내도록 설계됐다.

## Stage: hard_scaling

status: **FAIL**  
cases: 108  
timeouts: 12  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| caterpillar_mixed | 6 | 0.084 | 1.060 | 0.038 | 0.038 | 4096:0.036, 8192:0.038 |
| caterpillar_rect_dense | 6 | 0.330 | 1.257 | 0.047 | 0.048 | 4096:0.037, 8192:0.047 |
| comb_core | 0 | 0.199 | 1.993 | 0.025 | 0.025 | 4096:0.016, 8192:0.013, 16384:0.013, 32768:0.025 |
| comb_dense | 0 | 0.221 | 1.321 | 0.062 | 0.071 | 4096:0.037, 8192:0.049, 16384:0.050, 32768:0.062 |
| comb_plus_unary | 0 | 0.512 | 2.866 | 0.036 | 0.038 | 4096:0.011, 8192:0.013, 16384:0.013, 32768:0.036 |
| comb_rect_dense | 0 | 0.220 | 1.363 | 0.057 | 0.059 | 4096:0.038, 8192:0.036, 16384:0.049, 32768:0.057 |
| multi_comb_cap | 0 | 0.166 | 1.283 | 0.049 | 0.050 | 4096:0.037, 8192:0.037, 16384:0.048, 32768:0.049 |
| multi_comb_core | 0 | 0.299 | 2.001 | 0.025 | 0.025 | 4096:0.013, 8192:0.013, 16384:0.013, 32768:0.025 |
| multi_comb_rect | 0 | 0.160 | 1.277 | 0.050 | 0.059 | 4096:0.038, 8192:0.038, 16384:0.048, 32768:0.050 |

## Top slow cases

| stage | mode | n | seed | sec | rss_kb | case_dir |
| --- | --- | --- | --- | --- | --- | --- |
| hard_scaling | comb_dense | 32768 | 3 | 0.071 | 12928 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_codex/runs/hard_scaling/comb_dense/n32768/seed3_L1_Q1 |
| hard_scaling | comb_dense | 32768 | 2 | 0.062 | 12928 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_codex/runs/hard_scaling/comb_dense/n32768/seed2_L1_Q1 |
| hard_scaling | comb_dense | 32768 | 1 | 0.060 | 12928 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_codex/runs/hard_scaling/comb_dense/n32768/seed1_L1_Q1 |
| hard_scaling | comb_rect_dense | 32768 | 1 | 0.059 | 13024 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_codex/runs/hard_scaling/comb_rect_dense/n32768/seed1_L1_Q1 |
| hard_scaling | multi_comb_rect | 32768 | 1 | 0.059 | 12880 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_codex/runs/hard_scaling/multi_comb_rect/n32768/seed1_L1_Q1 |
| hard_scaling | comb_rect_dense | 32768 | 3 | 0.057 | 13008 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_codex/runs/hard_scaling/comb_rect_dense/n32768/seed3_L1_Q1 |
| hard_scaling | comb_dense | 16384 | 3 | 0.051 | 10752 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_codex/runs/hard_scaling/comb_dense/n16384/seed3_L1_Q1 |
| hard_scaling | multi_comb_cap | 32768 | 1 | 0.050 | 12720 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_codex/runs/hard_scaling/multi_comb_cap/n32768/seed1_L1_Q1 |
| hard_scaling | comb_dense | 16384 | 2 | 0.050 | 10752 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_codex/runs/hard_scaling/comb_dense/n16384/seed2_L1_Q1 |
| hard_scaling | multi_comb_cap | 4096 | 1 | 0.050 | 9056 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_codex/runs/hard_scaling/multi_comb_cap/n4096/seed1_L1_Q1 |
| hard_scaling | multi_comb_rect | 16384 | 2 | 0.050 | 10688 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_codex/runs/hard_scaling/multi_comb_rect/n16384/seed2_L1_Q1 |
| hard_scaling | comb_rect_dense | 16384 | 2 | 0.050 | 10864 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_codex/runs/hard_scaling/comb_rect_dense/n16384/seed2_L1_Q1 |
| hard_scaling | multi_comb_rect | 32768 | 3 | 0.050 | 12880 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_codex/runs/hard_scaling/multi_comb_rect/n32768/seed3_L1_Q1 |
| hard_scaling | comb_rect_dense | 32768 | 2 | 0.050 | 13008 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_codex/runs/hard_scaling/comb_rect_dense/n32768/seed2_L1_Q1 |
| hard_scaling | multi_comb_cap | 32768 | 2 | 0.049 | 12720 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_codex/runs/hard_scaling/multi_comb_cap/n32768/seed2_L1_Q1 |
| hard_scaling | comb_dense | 8192 | 3 | 0.049 | 9664 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_codex/runs/hard_scaling/comb_dense/n8192/seed3_L1_Q1 |
| hard_scaling | comb_rect_dense | 16384 | 1 | 0.049 | 10880 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_codex/runs/hard_scaling/comb_rect_dense/n16384/seed1_L1_Q1 |
| hard_scaling | comb_dense | 16384 | 1 | 0.049 | 10752 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_codex/runs/hard_scaling/comb_dense/n16384/seed1_L1_Q1 |
| hard_scaling | comb_dense | 8192 | 1 | 0.049 | 9664 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_codex/runs/hard_scaling/comb_dense/n8192/seed1_L1_Q1 |
| hard_scaling | comb_rect_dense | 4096 | 1 | 0.049 | 9264 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_codex/runs/hard_scaling/comb_rect_dense/n4096/seed1_L1_Q1 |
