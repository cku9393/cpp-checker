# Certification summary

overall verdict: **FAIL**

## Reasons

- correctness_fuzz: 4 failing cases

이 스위트는 hidden data를 완전히 복제하는 증명은 아니지만, decomposition 계열 느린 풀이와 잘못된 풀이를 매우 강하게 걸러내도록 설계됐다.

## Stage: correctness_fuzz

status: **FAIL**  
cases: 900  
timeouts: 4  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| balanced_dense | 0 | -0.014 | 1.036 | 0.015 | 0.015 | 64:0.015, 128:0.014, 256:0.014, 512:0.014, 1024:0.014 |
| broom_mixed | 3 | 1.645 | 5.801 | 1.033 | 1.617 | 64:0.014, 128:0.014, 256:0.040, 512:0.234, 1024:1.033 |
| caterpillar_rect_dense | 1 | 1.298 | 4.256 | 0.503 | 0.673 | 64:0.013, 128:0.023, 256:0.037, 512:0.118, 1024:0.503 |
| chain_unary | 0 | 0.032 | 1.088 | 0.015 | 0.022 | 64:0.013, 128:0.013, 256:0.014, 512:0.015, 1024:0.013 |
| comb_rect_dense | 0 | 1.376 | 3.968 | 0.479 | 0.539 | 64:0.013, 128:0.013, 256:0.036, 512:0.121, 1024:0.479 |
| multi_comb_cap | 0 | 0.618 | 3.103 | 0.085 | 0.105 | 64:0.014, 128:0.013, 256:0.014, 512:0.027, 1024:0.085 |
| multi_comb_rect | 0 | 0.897 | 3.721 | 0.142 | 0.151 | 64:0.013, 128:0.012, 256:0.013, 512:0.047, 1024:0.142 |
| random_recursive_mixed | 0 | 0.000 | 1.003 | 0.013 | 0.036 | 64:0.013, 128:0.013, 256:0.013, 512:0.013, 1024:0.013 |
| star_pairs | 0 | -0.003 | 1.158 | 0.015 | 0.029 | 64:0.013, 128:0.015, 256:0.014, 512:0.013, 1024:0.014 |

## Top slow cases

| stage | mode | n | seed | sec | rss_kb | case_dir |
| --- | --- | --- | --- | --- | --- | --- |
| correctness_fuzz | broom_mixed | 1024 | 5 | 1.617 | 225168 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v3/runs/correctness_fuzz/broom_mixed/n1024/seed5_L1_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 1 | 1.420 | 225088 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v3/runs/correctness_fuzz/broom_mixed/n1024/seed1_L0_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 4 | 1.339 | 225056 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v3/runs/correctness_fuzz/broom_mixed/n1024/seed4_L1_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 3 | 1.236 | 225024 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v3/runs/correctness_fuzz/broom_mixed/n1024/seed3_L1_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 5 | 1.236 | 225088 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v3/runs/correctness_fuzz/broom_mixed/n1024/seed5_L1_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 4 | 1.212 | 225008 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v3/runs/correctness_fuzz/broom_mixed/n1024/seed4_L0_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 2 | 1.204 | 224912 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v3/runs/correctness_fuzz/broom_mixed/n1024/seed2_L0_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 1 | 1.167 | 224880 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v3/runs/correctness_fuzz/broom_mixed/n1024/seed1_L1_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 2 | 1.033 | 224816 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v3/runs/correctness_fuzz/broom_mixed/n1024/seed2_L1_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 1 | 1.024 | 224816 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v3/runs/correctness_fuzz/broom_mixed/n1024/seed1_L1_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 4 | 1.000 | 225056 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v3/runs/correctness_fuzz/broom_mixed/n1024/seed4_L1_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 3 | 0.998 | 224944 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v3/runs/correctness_fuzz/broom_mixed/n1024/seed3_L1_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 1 | 0.918 | 225056 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v3/runs/correctness_fuzz/broom_mixed/n1024/seed1_L0_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 2 | 0.816 | 224720 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v3/runs/correctness_fuzz/broom_mixed/n1024/seed2_L1_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 4 | 0.815 | 225088 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v3/runs/correctness_fuzz/broom_mixed/n1024/seed4_L0_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 2 | 0.741 | 224848 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v3/runs/correctness_fuzz/broom_mixed/n1024/seed2_L0_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 3 | 0.705 | 224624 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v3/runs/correctness_fuzz/broom_mixed/n1024/seed3_L0_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 1 | 0.673 | 68080 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v3/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed1_L1_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 2 | 0.629 | 68096 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v3/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed2_L0_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 5 | 0.612 | 68128 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v3/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed5_L1_Q1 |
