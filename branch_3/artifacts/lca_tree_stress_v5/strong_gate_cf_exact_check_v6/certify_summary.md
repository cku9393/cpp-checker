# Certification summary

overall verdict: **PASS**

이 스위트는 hidden data를 완전히 복제하는 증명은 아니지만, decomposition 계열 느린 풀이와 잘못된 풀이를 매우 강하게 걸러내도록 설계됐다.

## Stage: correctness_fuzz

status: **PASS**  
cases: 900  
timeouts: 0  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| balanced_dense | 0 | -0.003 | 1.136 | 0.015 | 0.015 | 64:0.015, 128:0.014, 256:0.013, 512:0.015, 1024:0.014 |
| broom_mixed | 0 | 1.753 | 6.219 | 1.472 | 1.725 | 64:0.014, 128:0.014, 256:0.041, 512:0.237, 1024:1.472 |
| caterpillar_rect_dense | 0 | 1.380 | 4.491 | 0.599 | 1.044 | 64:0.015, 128:0.015, 256:0.036, 512:0.133, 1024:0.599 |
| chain_unary | 0 | 0.036 | 1.088 | 0.015 | 0.026 | 64:0.014, 128:0.014, 256:0.015, 512:0.015, 1024:0.015 |
| comb_rect_dense | 0 | 1.392 | 3.954 | 0.567 | 0.712 | 64:0.014, 128:0.015, 256:0.040, 512:0.144, 1024:0.567 |
| multi_comb_cap | 0 | 0.597 | 3.061 | 0.083 | 0.111 | 64:0.014, 128:0.015, 256:0.014, 512:0.027, 1024:0.083 |
| multi_comb_rect | 0 | 0.862 | 2.638 | 0.144 | 0.222 | 64:0.014, 128:0.015, 256:0.025, 512:0.055, 1024:0.144 |
| random_recursive_mixed | 0 | 0.016 | 1.061 | 0.013 | 0.026 | 64:0.012, 128:0.013, 256:0.012, 512:0.013, 1024:0.013 |
| star_pairs | 0 | -0.032 | 1.070 | 0.015 | 0.015 | 64:0.015, 128:0.015, 256:0.014, 512:0.013, 1024:0.014 |

## Top slow cases

| stage | mode | n | seed | sec | rss_kb | case_dir |
| --- | --- | --- | --- | --- | --- | --- |
| correctness_fuzz | broom_mixed | 1024 | 3 | 1.725 | 212272 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v6/runs/correctness_fuzz/broom_mixed/n1024/seed3_L0_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 4 | 1.698 | 212480 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v6/runs/correctness_fuzz/broom_mixed/n1024/seed4_L0_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 4 | 1.650 | 212560 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v6/runs/correctness_fuzz/broom_mixed/n1024/seed4_L1_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 1 | 1.643 | 212496 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v6/runs/correctness_fuzz/broom_mixed/n1024/seed1_L1_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 5 | 1.617 | 212528 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v6/runs/correctness_fuzz/broom_mixed/n1024/seed5_L1_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 2 | 1.579 | 212416 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v6/runs/correctness_fuzz/broom_mixed/n1024/seed2_L0_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 5 | 1.525 | 212480 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v6/runs/correctness_fuzz/broom_mixed/n1024/seed5_L0_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 2 | 1.502 | 212416 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v6/runs/correctness_fuzz/broom_mixed/n1024/seed2_L1_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 1 | 1.500 | 212464 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v6/runs/correctness_fuzz/broom_mixed/n1024/seed1_L1_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 3 | 1.490 | 212304 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v6/runs/correctness_fuzz/broom_mixed/n1024/seed3_L1_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 1 | 1.454 | 212464 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v6/runs/correctness_fuzz/broom_mixed/n1024/seed1_L0_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 2 | 1.357 | 212416 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v6/runs/correctness_fuzz/broom_mixed/n1024/seed2_L1_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 4 | 1.261 | 212496 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v6/runs/correctness_fuzz/broom_mixed/n1024/seed4_L1_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 5 | 1.257 | 212576 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v6/runs/correctness_fuzz/broom_mixed/n1024/seed5_L1_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 2 | 1.148 | 212352 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v6/runs/correctness_fuzz/broom_mixed/n1024/seed2_L0_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 3 | 1.133 | 212288 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v6/runs/correctness_fuzz/broom_mixed/n1024/seed3_L0_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 1 | 1.124 | 212464 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v6/runs/correctness_fuzz/broom_mixed/n1024/seed1_L0_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 3 | 1.097 | 212288 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v6/runs/correctness_fuzz/broom_mixed/n1024/seed3_L1_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 4 | 1.044 | 62912 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v6/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed4_L1_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 4 | 1.007 | 212544 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v6/runs/correctness_fuzz/broom_mixed/n1024/seed4_L0_Q0 |
