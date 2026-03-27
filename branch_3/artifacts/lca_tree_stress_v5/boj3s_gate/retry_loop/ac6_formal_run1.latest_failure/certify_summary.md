# Certification summary

overall verdict: **FAIL**

## Reasons

- correctness_smoke: 72 failing cases
- hard_scaling_strict: 90 failing cases
- hard_scaling_strict: comb_core: alpha=2.029 > 1.350
- hard_scaling_strict: comb_core: ratio=4.082 > 2.600
- hard_scaling_strict: comb_plus_unary: alpha=2.051 > 1.350
- hard_scaling_strict: comb_plus_unary: ratio=4.145 > 2.600
- hard_scaling_strict: multi_comb_core: alpha=2.024 > 1.350
- hard_scaling_strict: multi_comb_core: ratio=4.067 > 2.600
- boj_3s_large_adversarial: 30 failing cases
- boj_3s_large_mix: 15 failing cases

이 스위트는 hidden data를 완전히 복제하는 증명은 아니지만, decomposition 계열 느린 풀이와 잘못된 풀이를 매우 강하게 걸러내도록 설계됐다.

## Stage: correctness_smoke

status: **FAIL**  
cases: 288  
timeouts: 72  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| balanced_dense | 0 | 1.015 | 2.761 | 0.200 | 0.219 | 128:0.024, 256:0.037, 512:0.073, 1024:0.200 |
| caterpillar_rect_dense | 24 | 3.408 | 10.616 | 0.533 | 0.972 | 128:0.050, 256:0.533 |
| comb_rect_dense | 24 | 3.160 | 8.939 | 0.478 | 0.935 | 128:0.053, 256:0.478 |
| multi_comb_cap | 12 | 1.975 | 5.389 | 0.387 | 0.514 | 128:0.025, 256:0.072, 512:0.387 |
| multi_comb_rect | 12 | 2.430 | 7.085 | 0.888 | 1.322 | 128:0.031, 256:0.125, 512:0.888 |
| random_recursive_mixed | 0 | 1.135 | 2.823 | 0.144 | 0.158 | 128:0.013, 256:0.035, 512:0.061, 1024:0.144 |

## Stage: hard_scaling_strict

status: **FAIL**  
cases: 108  
timeouts: 90  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| caterpillar_mixed | 12 | - | - | - | - | - |
| caterpillar_rect_dense | 12 | - | - | - | - | - |
| comb_core | 6 | 2.029 | 4.082 | 1.573 | 1.574 | 4096:0.385, 8192:1.573 |
| comb_dense | 12 | - | - | - | - | - |
| comb_plus_unary | 6 | 2.051 | 4.145 | 2.526 | 2.535 | 4096:0.610, 8192:2.526 |
| comb_rect_dense | 12 | - | - | - | - | - |
| multi_comb_cap | 12 | - | - | - | - | - |
| multi_comb_core | 6 | 2.024 | 4.067 | 1.314 | 1.318 | 4096:0.323, 8192:1.314 |
| multi_comb_rect | 12 | - | - | - | - | - |

Scale check hits:

- comb_core: alpha=2.029 > 1.350
- comb_core: ratio=4.082 > 2.600
- comb_plus_unary: alpha=2.051 > 1.350
- comb_plus_unary: ratio=4.145 > 2.600
- multi_comb_core: alpha=2.024 > 1.350
- multi_comb_core: ratio=4.067 > 2.600

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
| broom_mixed | 0 | - | - | 0.673 | 0.675 | 99999:0.673 |
| caterpillar_rect_dense | 3 | - | - | - | - | - |
| comb_rect_dense | 3 | - | - | - | - | - |
| multi_comb_cap | 3 | - | - | - | - | - |
| random_recursive_mixed | 3 | - | - | - | - | - |

## Top slow cases

| stage | mode | n | seed | sec | rss_kb | case_dir |
| --- | --- | --- | --- | --- | --- | --- |
| hard_scaling_strict | comb_plus_unary | 8192 | 1 | 2.535 | 27776 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/retry_loop/ac6_formal_run1/runs/hard_scaling_strict/comb_plus_unary/n8192/seed1_L1_Q1 |
| hard_scaling_strict | comb_plus_unary | 8192 | 3 | 2.526 | 27824 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/retry_loop/ac6_formal_run1/runs/hard_scaling_strict/comb_plus_unary/n8192/seed3_L1_Q1 |
| hard_scaling_strict | comb_plus_unary | 8192 | 2 | 2.499 | 27888 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/retry_loop/ac6_formal_run1/runs/hard_scaling_strict/comb_plus_unary/n8192/seed2_L1_Q1 |
| hard_scaling_strict | comb_core | 8192 | 1 | 1.574 | 26848 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/retry_loop/ac6_formal_run1/runs/hard_scaling_strict/comb_core/n8192/seed1_L1_Q1 |
| hard_scaling_strict | comb_core | 8192 | 3 | 1.573 | 26960 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/retry_loop/ac6_formal_run1/runs/hard_scaling_strict/comb_core/n8192/seed3_L1_Q1 |
| hard_scaling_strict | comb_core | 8192 | 2 | 1.567 | 27040 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/retry_loop/ac6_formal_run1/runs/hard_scaling_strict/comb_core/n8192/seed2_L1_Q1 |
| correctness_smoke | multi_comb_rect | 512 | 2 | 1.322 | 61984 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/retry_loop/ac6_formal_run1/runs/correctness_smoke/multi_comb_rect/n512/seed2_L1_Q0 |
| hard_scaling_strict | multi_comb_core | 8192 | 3 | 1.318 | 28160 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/retry_loop/ac6_formal_run1/runs/hard_scaling_strict/multi_comb_core/n8192/seed3_L1_Q1 |
| correctness_smoke | multi_comb_rect | 512 | 2 | 1.316 | 61968 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/retry_loop/ac6_formal_run1/runs/correctness_smoke/multi_comb_rect/n512/seed2_L1_Q1 |
| hard_scaling_strict | multi_comb_core | 8192 | 1 | 1.314 | 27952 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/retry_loop/ac6_formal_run1/runs/hard_scaling_strict/multi_comb_core/n8192/seed1_L1_Q1 |
| hard_scaling_strict | multi_comb_core | 8192 | 2 | 1.309 | 27968 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/retry_loop/ac6_formal_run1/runs/hard_scaling_strict/multi_comb_core/n8192/seed2_L1_Q1 |
| correctness_smoke | multi_comb_rect | 512 | 1 | 1.188 | 64752 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/retry_loop/ac6_formal_run1/runs/correctness_smoke/multi_comb_rect/n512/seed1_L1_Q1 |
| correctness_smoke | multi_comb_rect | 512 | 1 | 1.186 | 64736 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/retry_loop/ac6_formal_run1/runs/correctness_smoke/multi_comb_rect/n512/seed1_L1_Q0 |
| correctness_smoke | caterpillar_rect_dense | 256 | 3 | 0.972 | 55664 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/retry_loop/ac6_formal_run1/runs/correctness_smoke/caterpillar_rect_dense/n256/seed3_L1_Q0 |
| correctness_smoke | caterpillar_rect_dense | 256 | 3 | 0.937 | 55728 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/retry_loop/ac6_formal_run1/runs/correctness_smoke/caterpillar_rect_dense/n256/seed3_L1_Q1 |
| correctness_smoke | comb_rect_dense | 256 | 2 | 0.935 | 50064 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/retry_loop/ac6_formal_run1/runs/correctness_smoke/comb_rect_dense/n256/seed2_L1_Q0 |
| correctness_smoke | multi_comb_rect | 512 | 2 | 0.911 | 43024 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/retry_loop/ac6_formal_run1/runs/correctness_smoke/multi_comb_rect/n512/seed2_L0_Q1 |
| correctness_smoke | comb_rect_dense | 256 | 2 | 0.899 | 50048 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/retry_loop/ac6_formal_run1/runs/correctness_smoke/comb_rect_dense/n256/seed2_L1_Q1 |
| correctness_smoke | multi_comb_rect | 512 | 1 | 0.888 | 43536 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/retry_loop/ac6_formal_run1/runs/correctness_smoke/multi_comb_rect/n512/seed1_L0_Q0 |
| correctness_smoke | multi_comb_rect | 512 | 1 | 0.887 | 43088 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/retry_loop/ac6_formal_run1/runs/correctness_smoke/multi_comb_rect/n512/seed1_L0_Q1 |
