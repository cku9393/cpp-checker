# Certification summary

overall verdict: **FAIL**

## Reasons

- correctness_fuzz: 20 failing cases

이 스위트는 hidden data를 완전히 복제하는 증명은 아니지만, decomposition 계열 느린 풀이와 잘못된 풀이를 매우 강하게 걸러내도록 설계됐다.

## Stage: correctness_fuzz

status: **FAIL**  
cases: 900  
timeouts: 20  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| balanced_dense | 0 | 0.912 | 2.209 | 0.180 | 0.196 | 64:0.014, 128:0.025, 256:0.040, 512:0.082, 1024:0.180 |
| broom_mixed | 0 | 1.412 | 2.894 | 0.603 | 1.096 | 64:0.013, 128:0.028, 256:0.080, 512:0.216, 1024:0.603 |
| caterpillar_rect_dense | 20 | 1.602 | 3.747 | 0.421 | 0.475 | 64:0.015, 128:0.040, 256:0.112, 512:0.421 |
| chain_unary | 0 | 1.603 | 3.886 | 0.902 | 1.072 | 64:0.013, 128:0.041, 256:0.160, 512:0.616, 1024:0.902 |
| comb_rect_dense | 0 | 1.663 | 4.130 | 1.243 | 1.347 | 64:0.011, 128:0.036, 256:0.093, 512:0.301, 1024:1.243 |
| multi_comb_cap | 0 | 1.254 | 3.164 | 0.430 | 0.481 | 64:0.013, 128:0.026, 256:0.053, 512:0.136, 1024:0.430 |
| multi_comb_rect | 0 | 1.419 | 3.996 | 0.604 | 0.702 | 64:0.011, 128:0.025, 256:0.055, 512:0.151, 1024:0.604 |
| random_recursive_mixed | 0 | 0.900 | 2.272 | 0.148 | 0.168 | 64:0.011, 128:0.021, 256:0.040, 512:0.065, 1024:0.148 |
| star_pairs | 0 | 1.274 | 3.577 | 0.323 | 0.351 | 64:0.013, 128:0.030, 256:0.108, 512:0.323, 1024:0.322 |

## Top slow cases

| stage | mode | n | seed | sec | rss_kb | case_dir |
| --- | --- | --- | --- | --- | --- | --- |
| correctness_fuzz | comb_rect_dense | 1024 | 3 | 1.347 | 190992 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n1024/seed3_L1_Q0 |
| correctness_fuzz | comb_rect_dense | 1024 | 5 | 1.343 | 192752 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n1024/seed5_L0_Q0 |
| correctness_fuzz | comb_rect_dense | 1024 | 4 | 1.338 | 162176 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n1024/seed4_L1_Q0 |
| correctness_fuzz | comb_rect_dense | 1024 | 4 | 1.331 | 164368 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n1024/seed4_L1_Q1 |
| correctness_fuzz | comb_rect_dense | 1024 | 5 | 1.326 | 193152 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n1024/seed5_L0_Q1 |
| correctness_fuzz | comb_rect_dense | 1024 | 3 | 1.318 | 190848 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n1024/seed3_L1_Q1 |
| correctness_fuzz | comb_rect_dense | 1024 | 4 | 1.316 | 163984 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n1024/seed4_L0_Q1 |
| correctness_fuzz | comb_rect_dense | 1024 | 4 | 1.313 | 163392 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n1024/seed4_L0_Q0 |
| correctness_fuzz | comb_rect_dense | 1024 | 3 | 1.299 | 193296 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n1024/seed3_L0_Q1 |
| correctness_fuzz | comb_rect_dense | 1024 | 1 | 1.243 | 193248 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n1024/seed1_L1_Q1 |
| correctness_fuzz | comb_rect_dense | 1024 | 5 | 1.243 | 192080 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n1024/seed5_L1_Q0 |
| correctness_fuzz | comb_rect_dense | 1024 | 3 | 1.242 | 193296 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n1024/seed3_L0_Q0 |
| correctness_fuzz | comb_rect_dense | 1024 | 2 | 1.241 | 191040 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n1024/seed2_L0_Q1 |
| correctness_fuzz | comb_rect_dense | 1024 | 1 | 1.233 | 190752 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n1024/seed1_L1_Q0 |
| correctness_fuzz | comb_rect_dense | 1024 | 2 | 1.233 | 191008 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n1024/seed2_L1_Q0 |
| correctness_fuzz | comb_rect_dense | 1024 | 2 | 1.232 | 192992 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n1024/seed2_L1_Q1 |
| correctness_fuzz | comb_rect_dense | 1024 | 1 | 1.231 | 193088 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n1024/seed1_L0_Q1 |
| correctness_fuzz | comb_rect_dense | 1024 | 5 | 1.229 | 190816 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n1024/seed5_L1_Q1 |
| correctness_fuzz | comb_rect_dense | 1024 | 1 | 1.227 | 190816 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n1024/seed1_L0_Q0 |
| correctness_fuzz | comb_rect_dense | 1024 | 2 | 1.227 | 192112 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n1024/seed2_L0_Q0 |
