# Certification summary

overall verdict: **FAIL**

## Reasons

- correctness_smoke: 155 failing cases
- hard_scaling_strict: 105 failing cases
- boj_3s_large_adversarial: 30 failing cases
- boj_3s_large_mix: 18 failing cases

이 스위트는 hidden data를 완전히 복제하는 증명은 아니지만, decomposition 계열 느린 풀이와 잘못된 풀이를 매우 강하게 걸러내도록 설계됐다.

## Stage: correctness_smoke

status: **FAIL**  
cases: 288  
timeouts: 155  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| balanced_dense | 2 | 0.673 | 2.142 | 0.831 | 1.105 | 128:0.197, 256:0.273, 512:0.388, 1024:0.831 |
| caterpillar_rect_dense | 36 | - | - | 0.298 | 0.366 | 128:0.298 |
| comb_rect_dense | 36 | - | - | 0.449 | 0.718 | 128:0.449 |
| multi_comb_cap | 23 | 1.389 | 3.988 | 1.448 | 1.448 | 128:0.211, 256:0.363, 512:1.448 |
| multi_comb_rect | 24 | 1.494 | 2.816 | 0.614 | 0.919 | 128:0.218, 256:0.614 |
| random_recursive_mixed | 34 | 0.157 | 1.115 | 0.212 | 0.264 | 128:0.190, 256:0.212 |

## Stage: hard_scaling_strict

status: **FAIL**  
cases: 108  
timeouts: 105  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| caterpillar_mixed | 12 | - | - | - | - | - |
| caterpillar_rect_dense | 12 | - | - | - | - | - |
| comb_core | 12 | - | - | - | - | - |
| comb_dense | 12 | - | - | - | - | - |
| comb_plus_unary | 12 | - | - | - | - | - |
| comb_rect_dense | 12 | - | - | - | - | - |
| multi_comb_cap | 12 | - | - | - | - | - |
| multi_comb_core | 9 | - | - | 1.927 | 2.124 | 4096:1.927 |
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
timeouts: 18  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| balanced_dense | 3 | - | - | - | - | - |
| broom_mixed | 3 | - | - | - | - | - |
| caterpillar_rect_dense | 3 | - | - | - | - | - |
| comb_rect_dense | 3 | - | - | - | - | - |
| multi_comb_cap | 3 | - | - | - | - | - |
| random_recursive_mixed | 3 | - | - | - | - | - |

## Top slow cases

| stage | mode | n | seed | sec | rss_kb | case_dir |
| --- | --- | --- | --- | --- | --- | --- |
| hard_scaling_strict | multi_comb_core | 4096 | 3 | 2.124 | 25008 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/ac5_full_diag_current/runs/hard_scaling_strict/multi_comb_core/n4096/seed3_L1_Q1 |
| hard_scaling_strict | multi_comb_core | 4096 | 2 | 1.927 | 25120 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/ac5_full_diag_current/runs/hard_scaling_strict/multi_comb_core/n4096/seed2_L1_Q1 |
| hard_scaling_strict | multi_comb_core | 4096 | 1 | 1.866 | 25264 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/ac5_full_diag_current/runs/hard_scaling_strict/multi_comb_core/n4096/seed1_L1_Q1 |
| correctness_smoke | multi_comb_cap | 512 | 2 | 1.448 | 24160 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/ac5_full_diag_current/runs/correctness_smoke/multi_comb_cap/n512/seed2_L0_Q0 |
| correctness_smoke | balanced_dense | 1024 | 1 | 1.105 | 19216 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/ac5_full_diag_current/runs/correctness_smoke/balanced_dense/n1024/seed1_L1_Q0 |
| correctness_smoke | balanced_dense | 1024 | 2 | 1.099 | 16464 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/ac5_full_diag_current/runs/correctness_smoke/balanced_dense/n1024/seed2_L0_Q0 |
| correctness_smoke | balanced_dense | 1024 | 1 | 1.073 | 18880 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/ac5_full_diag_current/runs/correctness_smoke/balanced_dense/n1024/seed1_L1_Q1 |
| correctness_smoke | balanced_dense | 1024 | 2 | 1.053 | 18160 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/ac5_full_diag_current/runs/correctness_smoke/balanced_dense/n1024/seed2_L1_Q0 |
| correctness_smoke | balanced_dense | 1024 | 2 | 1.044 | 16224 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/ac5_full_diag_current/runs/correctness_smoke/balanced_dense/n1024/seed2_L0_Q1 |
| correctness_smoke | balanced_dense | 512 | 2 | 0.921 | 9168 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/ac5_full_diag_current/runs/correctness_smoke/balanced_dense/n512/seed2_L0_Q0 |
| correctness_smoke | multi_comb_rect | 256 | 2 | 0.919 | 16400 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/ac5_full_diag_current/runs/correctness_smoke/multi_comb_rect/n256/seed2_L1_Q1 |
| correctness_smoke | multi_comb_rect | 256 | 3 | 0.843 | 16000 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/ac5_full_diag_current/runs/correctness_smoke/multi_comb_rect/n256/seed3_L1_Q1 |
| correctness_smoke | balanced_dense | 1024 | 2 | 0.837 | 19056 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/ac5_full_diag_current/runs/correctness_smoke/balanced_dense/n1024/seed2_L1_Q1 |
| correctness_smoke | balanced_dense | 1024 | 1 | 0.825 | 16688 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/ac5_full_diag_current/runs/correctness_smoke/balanced_dense/n1024/seed1_L0_Q1 |
| correctness_smoke | multi_comb_rect | 256 | 1 | 0.786 | 15296 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/ac5_full_diag_current/runs/correctness_smoke/multi_comb_rect/n256/seed1_L1_Q0 |
| correctness_smoke | multi_comb_rect | 256 | 2 | 0.784 | 16528 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/ac5_full_diag_current/runs/correctness_smoke/multi_comb_rect/n256/seed2_L1_Q0 |
| correctness_smoke | balanced_dense | 1024 | 3 | 0.770 | 18864 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/ac5_full_diag_current/runs/correctness_smoke/balanced_dense/n1024/seed3_L1_Q1 |
| correctness_smoke | balanced_dense | 1024 | 1 | 0.767 | 16352 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/ac5_full_diag_current/runs/correctness_smoke/balanced_dense/n1024/seed1_L0_Q0 |
| correctness_smoke | balanced_dense | 1024 | 3 | 0.748 | 18736 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/ac5_full_diag_current/runs/correctness_smoke/balanced_dense/n1024/seed3_L1_Q0 |
| correctness_smoke | balanced_dense | 1024 | 3 | 0.721 | 16848 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/ac5_full_diag_current/runs/correctness_smoke/balanced_dense/n1024/seed3_L0_Q0 |
