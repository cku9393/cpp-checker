# Certification summary

overall verdict: **FAIL**

## Reasons

- correctness_smoke: 64 failing cases
- hard_scaling_strict: 99 failing cases
- boj_3s_large_adversarial: 30 failing cases
- boj_3s_large_mix: 18 failing cases

이 스위트는 hidden data를 완전히 복제하는 증명은 아니지만, decomposition 계열 느린 풀이와 잘못된 풀이를 매우 강하게 걸러내도록 설계됐다.

## Stage: correctness_smoke

status: **FAIL**  
cases: 288  
timeouts: 24  
re/wa: 40  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| balanced_dense | 2 | 1.001 | 2.150 | 0.360 | 0.398 | 128:0.045, 256:0.082, 512:0.167, 1024:0.360 |
| caterpillar_rect_dense | 44 | - | - | 0.059 | 0.076 | 128:0.059 |
| comb_rect_dense | 12 | 1.563 | 3.231 | 0.574 | 0.727 | 128:0.066, 256:0.178, 512:0.574 |
| multi_comb_cap | 0 | 1.317 | 2.839 | 0.708 | 0.782 | 128:0.047, 256:0.094, 512:0.249, 1024:0.708 |
| multi_comb_rect | 0 | 1.419 | 3.432 | 1.053 | 1.127 | 128:0.054, 256:0.120, 512:0.307, 1024:1.053 |
| random_recursive_mixed | 6 | 0.957 | 2.101 | 0.302 | 0.337 | 128:0.041, 256:0.074, 512:0.144, 1024:0.302 |

## Stage: hard_scaling_strict

status: **FAIL**  
cases: 108  
timeouts: 99  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| caterpillar_mixed | 12 | - | - | - | - | - |
| caterpillar_rect_dense | 12 | - | - | - | - | - |
| comb_core | 9 | - | - | 1.117 | 1.319 | 4096:1.117 |
| comb_dense | 12 | - | - | - | - | - |
| comb_plus_unary | 9 | - | - | 1.162 | 1.267 | 4096:1.162 |
| comb_rect_dense | 12 | - | - | - | - | - |
| multi_comb_cap | 12 | - | - | - | - | - |
| multi_comb_core | 9 | - | - | 1.947 | 1.965 | 4096:1.947 |
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
re/wa: 3  

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
| hard_scaling_strict | multi_comb_core | 4096 | 3 | 1.965 | 21568 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/retry_loop/ac5_formal_attempt_current/runs/hard_scaling_strict/multi_comb_core/n4096/seed3_L1_Q1 |
| hard_scaling_strict | multi_comb_core | 4096 | 1 | 1.947 | 21856 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/retry_loop/ac5_formal_attempt_current/runs/hard_scaling_strict/multi_comb_core/n4096/seed1_L1_Q1 |
| hard_scaling_strict | multi_comb_core | 4096 | 2 | 1.378 | 23056 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/retry_loop/ac5_formal_attempt_current/runs/hard_scaling_strict/multi_comb_core/n4096/seed2_L1_Q1 |
| hard_scaling_strict | comb_core | 4096 | 1 | 1.319 | 33744 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/retry_loop/ac5_formal_attempt_current/runs/hard_scaling_strict/comb_core/n4096/seed1_L1_Q1 |
| boj_3s_large_mix | broom_mixed | 99999 | 3 | 1.313 | 3008 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/retry_loop/ac5_formal_attempt_current/runs/boj_3s_large_mix/broom_mixed/n99999/seed3_L1_Q1 |
| hard_scaling_strict | comb_plus_unary | 4096 | 2 | 1.267 | 34160 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/retry_loop/ac5_formal_attempt_current/runs/hard_scaling_strict/comb_plus_unary/n4096/seed2_L1_Q1 |
| boj_3s_large_mix | broom_mixed | 99999 | 1 | 1.172 | 3008 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/retry_loop/ac5_formal_attempt_current/runs/boj_3s_large_mix/broom_mixed/n99999/seed1_L1_Q1 |
| hard_scaling_strict | comb_plus_unary | 4096 | 1 | 1.162 | 34208 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/retry_loop/ac5_formal_attempt_current/runs/hard_scaling_strict/comb_plus_unary/n4096/seed1_L1_Q1 |
| hard_scaling_strict | comb_plus_unary | 4096 | 3 | 1.132 | 34192 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/retry_loop/ac5_formal_attempt_current/runs/hard_scaling_strict/comb_plus_unary/n4096/seed3_L1_Q1 |
| correctness_smoke | multi_comb_rect | 1024 | 2 | 1.127 | 24160 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/retry_loop/ac5_formal_attempt_current/runs/correctness_smoke/multi_comb_rect/n1024/seed2_L1_Q1 |
| hard_scaling_strict | comb_core | 4096 | 3 | 1.117 | 33552 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/retry_loop/ac5_formal_attempt_current/runs/hard_scaling_strict/comb_core/n4096/seed3_L1_Q1 |
| correctness_smoke | multi_comb_rect | 1024 | 1 | 1.106 | 23792 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/retry_loop/ac5_formal_attempt_current/runs/correctness_smoke/multi_comb_rect/n1024/seed1_L1_Q1 |
| correctness_smoke | multi_comb_rect | 1024 | 2 | 1.092 | 24496 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/retry_loop/ac5_formal_attempt_current/runs/correctness_smoke/multi_comb_rect/n1024/seed2_L1_Q0 |
| correctness_smoke | multi_comb_rect | 1024 | 3 | 1.088 | 23024 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/retry_loop/ac5_formal_attempt_current/runs/correctness_smoke/multi_comb_rect/n1024/seed3_L1_Q0 |
| correctness_smoke | multi_comb_rect | 1024 | 1 | 1.087 | 23392 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/retry_loop/ac5_formal_attempt_current/runs/correctness_smoke/multi_comb_rect/n1024/seed1_L0_Q0 |
| hard_scaling_strict | comb_core | 4096 | 2 | 1.065 | 33712 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/retry_loop/ac5_formal_attempt_current/runs/hard_scaling_strict/comb_core/n4096/seed2_L1_Q1 |
| correctness_smoke | multi_comb_rect | 1024 | 3 | 1.064 | 23680 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/retry_loop/ac5_formal_attempt_current/runs/correctness_smoke/multi_comb_rect/n1024/seed3_L1_Q1 |
| correctness_smoke | multi_comb_rect | 1024 | 2 | 1.041 | 24496 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/retry_loop/ac5_formal_attempt_current/runs/correctness_smoke/multi_comb_rect/n1024/seed2_L0_Q0 |
| correctness_smoke | multi_comb_rect | 1024 | 1 | 1.040 | 22816 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/retry_loop/ac5_formal_attempt_current/runs/correctness_smoke/multi_comb_rect/n1024/seed1_L1_Q0 |
| correctness_smoke | multi_comb_rect | 1024 | 1 | 1.037 | 23552 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/retry_loop/ac5_formal_attempt_current/runs/correctness_smoke/multi_comb_rect/n1024/seed1_L0_Q1 |
