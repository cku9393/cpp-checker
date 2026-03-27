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
| balanced_dense | 0 | 0.295 | 1.979 | 0.025 | 0.027 | 128:0.013, 256:0.013, 512:0.013, 1024:0.025 |
| caterpillar_rect_dense | 0 | 1.933 | 4.784 | 0.651 | 1.359 | 128:0.013, 256:0.038, 512:0.183, 1024:0.651 |
| comb_rect_dense | 0 | 1.679 | 3.935 | 0.770 | 1.391 | 128:0.025, 256:0.050, 512:0.196, 1024:0.770 |
| multi_comb_cap | 0 | 1.205 | 3.869 | 0.142 | 0.298 | 128:0.013, 256:0.013, 512:0.037, 1024:0.142 |
| multi_comb_rect | 0 | 1.430 | 3.648 | 0.239 | 0.397 | 128:0.012, 256:0.024, 512:0.066, 1024:0.239 |
| random_recursive_mixed | 0 | 0.300 | 2.006 | 0.025 | 0.046 | 128:0.013, 256:0.013, 512:0.013, 1024:0.025 |

## Stage: hard_scaling_strict

status: **PASS**  
cases: 108  
timeouts: 0  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| caterpillar_mixed | 0 | 0.157 | 1.246 | 0.062 | 0.089 | 4096:0.046, 8192:0.048, 16384:0.060, 32768:0.062 |
| caterpillar_rect_dense | 0 | 0.236 | 1.268 | 0.060 | 0.069 | 4096:0.036, 8192:0.046, 16384:0.050, 32768:0.060 |
| comb_core | 0 | 0.417 | 2.158 | 0.027 | 0.037 | 4096:0.011, 8192:0.011, 16384:0.013, 32768:0.027 |
| comb_dense | 0 | 0.239 | 1.361 | 0.059 | 0.060 | 4096:0.038, 8192:0.036, 16384:0.049, 32768:0.059 |
| comb_plus_unary | 0 | 0.339 | 1.820 | 0.026 | 0.026 | 4096:0.013, 8192:0.013, 16384:0.014, 32768:0.026 |
| comb_rect_dense | 0 | 0.221 | 1.300 | 0.060 | 0.072 | 4096:0.038, 8192:0.041, 16384:0.046, 32768:0.060 |
| multi_comb_cap | 0 | 0.084 | 1.268 | 0.048 | 0.050 | 4096:0.038, 8192:0.048, 16384:0.043, 32768:0.047 |
| multi_comb_core | 0 | 0.310 | 2.164 | 0.027 | 0.028 | 4096:0.013, 8192:0.015, 16384:0.013, 32768:0.027 |
| multi_comb_rect | 0 | 0.066 | 1.270 | 0.048 | 0.060 | 4096:0.045, 8192:0.037, 16384:0.047, 32768:0.048 |

## Stage: boj_3s_large_adversarial

status: **PASS**  
cases: 30  
timeouts: 0  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| caterpillar_rect_dense | 0 | 0.466 | 1.653 | 0.094 | 0.098 | 50000:0.057, 75000:0.094, 99999:0.076 |
| comb_dense | 0 | -0.065 | 1.136 | 0.073 | 0.078 | 50000:0.073, 75000:0.063, 99999:0.071 |
| comb_rect_dense | 0 | 1.258 | 2.076 | 0.175 | 0.230 | 50000:0.070, 75000:0.084, 99999:0.175 |
| multi_comb_cap | 0 | 0.876 | 1.790 | 0.101 | 0.126 | 50000:0.053, 75000:0.057, 99999:0.101 |
| multi_comb_rect | 0 | 0.437 | 1.248 | 0.091 | 0.095 | 50000:0.067, 75000:0.084, 99999:0.091 |

## Stage: boj_3s_large_mix

status: **FAIL**  
cases: 18  
timeouts: 3  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| balanced_dense | 0 | - | - | 0.361 | 0.410 | 99999:0.361 |
| broom_mixed | 3 | - | - | - | - | - |
| caterpillar_rect_dense | 0 | - | - | 0.099 | 0.117 | 99999:0.099 |
| comb_rect_dense | 0 | - | - | 0.083 | 0.097 | 99999:0.083 |
| multi_comb_cap | 0 | - | - | 0.085 | 0.098 | 99999:0.085 |
| random_recursive_mixed | 0 | - | - | 0.205 | 0.213 | 99999:0.205 |

## Top slow cases

| stage | mode | n | seed | sec | rss_kb | case_dir |
| --- | --- | --- | --- | --- | --- | --- |
| correctness_smoke | comb_rect_dense | 1024 | 3 | 1.391 | 63856 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/runs/correctness_smoke/comb_rect_dense/n1024/seed3_L1_Q1 |
| correctness_smoke | caterpillar_rect_dense | 1024 | 1 | 1.359 | 63472 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/runs/correctness_smoke/caterpillar_rect_dense/n1024/seed1_L1_Q0 |
| correctness_smoke | comb_rect_dense | 1024 | 3 | 1.354 | 63856 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/runs/correctness_smoke/comb_rect_dense/n1024/seed3_L1_Q0 |
| correctness_smoke | caterpillar_rect_dense | 512 | 1 | 0.931 | 17520 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/runs/correctness_smoke/caterpillar_rect_dense/n512/seed1_L0_Q0 |
| correctness_smoke | caterpillar_rect_dense | 1024 | 1 | 0.895 | 63424 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/runs/correctness_smoke/caterpillar_rect_dense/n1024/seed1_L0_Q1 |
| correctness_smoke | comb_rect_dense | 1024 | 2 | 0.862 | 63984 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/runs/correctness_smoke/comb_rect_dense/n1024/seed2_L1_Q1 |
| correctness_smoke | comb_rect_dense | 1024 | 1 | 0.848 | 63840 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/runs/correctness_smoke/comb_rect_dense/n1024/seed1_L0_Q1 |
| correctness_smoke | comb_rect_dense | 1024 | 2 | 0.803 | 63920 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/runs/correctness_smoke/comb_rect_dense/n1024/seed2_L1_Q0 |
| correctness_smoke | comb_rect_dense | 1024 | 1 | 0.781 | 63920 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/runs/correctness_smoke/comb_rect_dense/n1024/seed1_L1_Q0 |
| correctness_smoke | comb_rect_dense | 1024 | 2 | 0.760 | 63968 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/runs/correctness_smoke/comb_rect_dense/n1024/seed2_L0_Q1 |
| correctness_smoke | comb_rect_dense | 1024 | 1 | 0.757 | 63872 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/runs/correctness_smoke/comb_rect_dense/n1024/seed1_L1_Q1 |
| correctness_smoke | caterpillar_rect_dense | 1024 | 1 | 0.748 | 63472 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/runs/correctness_smoke/caterpillar_rect_dense/n1024/seed1_L1_Q1 |
| correctness_smoke | comb_rect_dense | 1024 | 1 | 0.748 | 63904 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/runs/correctness_smoke/comb_rect_dense/n1024/seed1_L0_Q0 |
| correctness_smoke | comb_rect_dense | 1024 | 2 | 0.745 | 63968 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/runs/correctness_smoke/comb_rect_dense/n1024/seed2_L0_Q0 |
| correctness_smoke | comb_rect_dense | 1024 | 3 | 0.717 | 63808 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/runs/correctness_smoke/comb_rect_dense/n1024/seed3_L0_Q1 |
| correctness_smoke | caterpillar_rect_dense | 1024 | 2 | 0.697 | 63440 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/runs/correctness_smoke/caterpillar_rect_dense/n1024/seed2_L0_Q0 |
| correctness_smoke | comb_rect_dense | 1024 | 3 | 0.696 | 63808 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/runs/correctness_smoke/comb_rect_dense/n1024/seed3_L0_Q0 |
| correctness_smoke | caterpillar_rect_dense | 1024 | 3 | 0.675 | 63360 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/runs/correctness_smoke/caterpillar_rect_dense/n1024/seed3_L0_Q1 |
| correctness_smoke | caterpillar_rect_dense | 1024 | 1 | 0.653 | 63376 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/runs/correctness_smoke/caterpillar_rect_dense/n1024/seed1_L0_Q0 |
| correctness_smoke | caterpillar_rect_dense | 1024 | 3 | 0.649 | 63456 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/runs/correctness_smoke/caterpillar_rect_dense/n1024/seed3_L1_Q0 |
