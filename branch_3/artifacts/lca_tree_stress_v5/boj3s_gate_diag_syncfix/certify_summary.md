# Certification summary

overall verdict: **FAIL**

## Reasons

- correctness_smoke: 111 failing cases
- hard_scaling_strict: 102 failing cases
- boj_3s_large_adversarial: 30 failing cases
- boj_3s_large_mix: 15 failing cases

이 스위트는 hidden data를 완전히 복제하는 증명은 아니지만, decomposition 계열 느린 풀이와 잘못된 풀이를 매우 강하게 걸러내도록 설계됐다.

## Stage: correctness_smoke

status: **FAIL**  
cases: 288  
timeouts: 111  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| balanced_dense | 0 | 1.310 | 2.802 | 0.644 | 0.723 | 128:0.042, 256:0.093, 512:0.230, 1024:0.644 |
| caterpillar_rect_dense | 33 | 3.263 | 9.601 | 1.494 | 1.502 | 128:0.156, 256:1.494 |
| comb_rect_dense | 36 | - | - | 0.181 | 0.311 | 128:0.181 |
| multi_comb_cap | 18 | 2.188 | 5.663 | 1.323 | 1.399 | 128:0.064, 256:0.234, 512:1.323 |
| multi_comb_rect | 24 | 2.308 | 4.952 | 0.451 | 0.820 | 128:0.091, 256:0.451 |
| random_recursive_mixed | 0 | 1.135 | 2.587 | 0.474 | 0.643 | 128:0.043, 256:0.094, 512:0.183, 1024:0.474 |

## Stage: hard_scaling_strict

status: **FAIL**  
cases: 108  
timeouts: 102  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| caterpillar_mixed | 12 | - | - | - | - | - |
| caterpillar_rect_dense | 12 | - | - | - | - | - |
| comb_core | 9 | - | - | 2.026 | 2.193 | 4096:2.026 |
| comb_dense | 12 | - | - | - | - | - |
| comb_plus_unary | 12 | - | - | - | - | - |
| comb_rect_dense | 12 | - | - | - | - | - |
| multi_comb_cap | 12 | - | - | - | - | - |
| multi_comb_core | 9 | - | - | 2.441 | 2.569 | 4096:2.441 |
| multi_comb_rect | 12 | - | - | - | - | - |

## Stage: boj_3s_large_adversarial

status: **FAIL**  
cases: 30  
timeouts: 30  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| caterpillar_rect_dense | 6 | - | - | - | - | - |
| comb_dense | 6 | - | - | - | - | - |
| comb_rect_dense | 6 | - | - | - | - | - |
| multi_comb_cap | 6 | - | - | - | - | - |
| multi_comb_rect | 6 | - | - | - | - | - |

## Stage: boj_3s_large_mix

status: **FAIL**  
cases: 18  
timeouts: 15  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| balanced_dense | 3 | - | - | - | - | - |
| broom_mixed | 0 | - | - | 0.899 | 1.005 | 99999:0.899 |
| caterpillar_rect_dense | 3 | - | - | - | - | - |
| comb_rect_dense | 3 | - | - | - | - | - |
| multi_comb_cap | 3 | - | - | - | - | - |
| random_recursive_mixed | 3 | - | - | - | - | - |

## Top slow cases

| stage | mode | n | seed | sec | rss_kb | case_dir |
| --- | --- | --- | --- | --- | --- | --- |
| hard_scaling_strict | multi_comb_core | 4096 | 2 | 2.569 | 25248 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate_diag_syncfix/runs/hard_scaling_strict/multi_comb_core/n4096/seed2_L1_Q1 |
| hard_scaling_strict | multi_comb_core | 4096 | 1 | 2.441 | 24320 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate_diag_syncfix/runs/hard_scaling_strict/multi_comb_core/n4096/seed1_L1_Q1 |
| hard_scaling_strict | comb_core | 4096 | 2 | 2.193 | 34288 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate_diag_syncfix/runs/hard_scaling_strict/comb_core/n4096/seed2_L1_Q1 |
| hard_scaling_strict | comb_core | 4096 | 3 | 2.026 | 34240 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate_diag_syncfix/runs/hard_scaling_strict/comb_core/n4096/seed3_L1_Q1 |
| hard_scaling_strict | comb_core | 4096 | 1 | 1.993 | 34160 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate_diag_syncfix/runs/hard_scaling_strict/comb_core/n4096/seed1_L1_Q1 |
| hard_scaling_strict | multi_comb_core | 4096 | 3 | 1.977 | 25408 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate_diag_syncfix/runs/hard_scaling_strict/multi_comb_core/n4096/seed3_L1_Q1 |
| correctness_smoke | caterpillar_rect_dense | 256 | 3 | 1.502 | 28480 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate_diag_syncfix/runs/correctness_smoke/caterpillar_rect_dense/n256/seed3_L0_Q0 |
| correctness_smoke | caterpillar_rect_dense | 256 | 1 | 1.494 | 28576 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate_diag_syncfix/runs/correctness_smoke/caterpillar_rect_dense/n256/seed1_L0_Q0 |
| correctness_smoke | caterpillar_rect_dense | 256 | 3 | 1.490 | 28752 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate_diag_syncfix/runs/correctness_smoke/caterpillar_rect_dense/n256/seed3_L0_Q1 |
| correctness_smoke | multi_comb_cap | 512 | 3 | 1.399 | 23344 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate_diag_syncfix/runs/correctness_smoke/multi_comb_cap/n512/seed3_L0_Q1 |
| correctness_smoke | multi_comb_cap | 512 | 1 | 1.362 | 25536 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate_diag_syncfix/runs/correctness_smoke/multi_comb_cap/n512/seed1_L0_Q0 |
| correctness_smoke | multi_comb_cap | 512 | 2 | 1.356 | 25632 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate_diag_syncfix/runs/correctness_smoke/multi_comb_cap/n512/seed2_L0_Q0 |
| correctness_smoke | multi_comb_cap | 512 | 1 | 1.291 | 25280 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate_diag_syncfix/runs/correctness_smoke/multi_comb_cap/n512/seed1_L0_Q1 |
| correctness_smoke | multi_comb_cap | 512 | 3 | 1.283 | 25216 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate_diag_syncfix/runs/correctness_smoke/multi_comb_cap/n512/seed3_L0_Q0 |
| correctness_smoke | multi_comb_cap | 512 | 2 | 1.234 | 24832 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate_diag_syncfix/runs/correctness_smoke/multi_comb_cap/n512/seed2_L0_Q1 |
| boj_3s_large_mix | broom_mixed | 99999 | 3 | 1.005 | 2960 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate_diag_syncfix/runs/boj_3s_large_mix/broom_mixed/n99999/seed3_L1_Q1 |
| boj_3s_large_mix | broom_mixed | 99999 | 2 | 0.899 | 2976 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate_diag_syncfix/runs/boj_3s_large_mix/broom_mixed/n99999/seed2_L1_Q1 |
| boj_3s_large_mix | broom_mixed | 99999 | 1 | 0.886 | 2976 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate_diag_syncfix/runs/boj_3s_large_mix/broom_mixed/n99999/seed1_L1_Q1 |
| correctness_smoke | multi_comb_rect | 256 | 2 | 0.820 | 16288 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate_diag_syncfix/runs/correctness_smoke/multi_comb_rect/n256/seed2_L1_Q1 |
| correctness_smoke | multi_comb_rect | 256 | 2 | 0.763 | 16448 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate_diag_syncfix/runs/correctness_smoke/multi_comb_rect/n256/seed2_L1_Q0 |
