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
| balanced_dense | 0 | 0.022 | 1.154 | 0.015 | 0.029 | 64:0.013, 128:0.014, 256:0.014, 512:0.013, 1024:0.015 |
| broom_mixed | 20 | 1.546 | 5.888 | 0.339 | 0.552 | 64:0.015, 128:0.015, 256:0.058, 512:0.339 |
| caterpillar_rect_dense | 0 | 1.562 | 5.277 | 0.808 | 1.016 | 64:0.013, 128:0.013, 256:0.036, 512:0.153, 1024:0.808 |
| chain_unary | 0 | 0.035 | 1.126 | 0.014 | 0.015 | 64:0.013, 128:0.013, 256:0.013, 512:0.013, 1024:0.014 |
| comb_rect_dense | 0 | 1.280 | 3.928 | 0.381 | 0.445 | 64:0.013, 128:0.013, 256:0.025, 512:0.097, 1024:0.381 |
| multi_comb_cap | 0 | 0.678 | 3.710 | 0.093 | 0.106 | 64:0.013, 128:0.013, 256:0.013, 512:0.025, 1024:0.093 |
| multi_comb_rect | 0 | 0.925 | 3.866 | 0.157 | 0.163 | 64:0.013, 128:0.013, 256:0.013, 512:0.049, 1024:0.157 |
| random_recursive_mixed | 0 | -0.003 | 1.019 | 0.013 | 0.024 | 64:0.013, 128:0.013, 256:0.013, 512:0.012, 1024:0.013 |
| star_pairs | 0 | 0.016 | 1.097 | 0.015 | 0.036 | 64:0.014, 128:0.013, 256:0.015, 512:0.015, 1024:0.014 |

## Top slow cases

| stage | mode | n | seed | sec | rss_kb | case_dir |
| --- | --- | --- | --- | --- | --- | --- |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 2 | 1.016 | 70704 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v2/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed2_L0_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 2 | 0.960 | 70656 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v2/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed2_L0_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 1 | 0.891 | 70720 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v2/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed1_L0_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 4 | 0.867 | 70368 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v2/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed4_L0_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 1 | 0.852 | 70672 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v2/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed1_L1_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 3 | 0.833 | 70640 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v2/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed3_L0_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 1 | 0.823 | 70624 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v2/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed1_L0_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 2 | 0.822 | 70656 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v2/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed2_L1_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 5 | 0.822 | 70400 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v2/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed5_L1_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 5 | 0.814 | 70784 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v2/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed5_L0_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 1 | 0.803 | 70736 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v2/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed1_L1_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 4 | 0.795 | 70512 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v2/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed4_L1_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 3 | 0.789 | 70480 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v2/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed3_L0_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 4 | 0.784 | 70608 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v2/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed4_L1_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 2 | 0.778 | 70576 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v2/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed2_L1_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 3 | 0.775 | 70688 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v2/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed3_L1_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 5 | 0.767 | 70656 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v2/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed5_L1_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 4 | 0.765 | 70368 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v2/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed4_L0_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 5 | 0.755 | 70752 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v2/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed5_L0_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 3 | 0.737 | 70576 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v2/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed3_L1_Q0 |
