# Certification summary

overall verdict: **FAIL**

## Reasons

- correctness_fuzz: 2 failing cases

이 스위트는 hidden data를 완전히 복제하는 증명은 아니지만, decomposition 계열 느린 풀이와 잘못된 풀이를 매우 강하게 걸러내도록 설계됐다.

## Stage: correctness_fuzz

status: **FAIL**  
cases: 900  
timeouts: 2  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| balanced_dense | 0 | 0.284 | 1.889 | 0.024 | 0.036 | 64:0.013, 128:0.013, 256:0.013, 512:0.024, 1024:0.024 |
| broom_mixed | 1 | 1.648 | 6.025 | 1.027 | 1.412 | 64:0.013, 128:0.013, 256:0.036, 512:0.171, 1024:1.027 |
| caterpillar_rect_dense | 0 | 1.277 | 3.975 | 0.380 | 0.472 | 64:0.013, 128:0.013, 256:0.025, 512:0.096, 1024:0.380 |
| chain_unary | 0 | 0.040 | 1.083 | 0.015 | 0.026 | 64:0.014, 128:0.013, 256:0.013, 512:0.015, 1024:0.015 |
| comb_rect_dense | 1 | 1.271 | 3.768 | 0.419 | 0.608 | 64:0.015, 128:0.013, 256:0.031, 512:0.111, 1024:0.419 |
| multi_comb_cap | 0 | 0.607 | 2.834 | 0.080 | 0.107 | 64:0.014, 128:0.013, 256:0.014, 512:0.028, 1024:0.080 |
| multi_comb_rect | 0 | 0.788 | 3.396 | 0.136 | 0.176 | 64:0.015, 128:0.014, 256:0.015, 512:0.040, 1024:0.136 |
| random_recursive_mixed | 0 | 0.015 | 1.034 | 0.013 | 0.023 | 64:0.012, 128:0.012, 256:0.013, 512:0.013, 1024:0.013 |
| star_pairs | 0 | -0.015 | 1.234 | 0.015 | 0.015 | 64:0.014, 128:0.012, 256:0.015, 512:0.014, 1024:0.012 |

## Top slow cases

| stage | mode | n | seed | sec | rss_kb | case_dir |
| --- | --- | --- | --- | --- | --- | --- |
| correctness_fuzz | broom_mixed | 1024 | 4 | 1.412 | 225040 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check/runs/correctness_fuzz/broom_mixed/n1024/seed4_L1_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 2 | 1.392 | 224752 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check/runs/correctness_fuzz/broom_mixed/n1024/seed2_L1_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 5 | 1.307 | 225296 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check/runs/correctness_fuzz/broom_mixed/n1024/seed5_L1_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 3 | 1.111 | 224736 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check/runs/correctness_fuzz/broom_mixed/n1024/seed3_L1_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 4 | 1.070 | 224800 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check/runs/correctness_fuzz/broom_mixed/n1024/seed4_L0_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 2 | 1.056 | 224672 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check/runs/correctness_fuzz/broom_mixed/n1024/seed2_L0_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 1 | 1.052 | 224896 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check/runs/correctness_fuzz/broom_mixed/n1024/seed1_L1_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 3 | 1.048 | 224576 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check/runs/correctness_fuzz/broom_mixed/n1024/seed3_L0_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 5 | 1.036 | 224896 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check/runs/correctness_fuzz/broom_mixed/n1024/seed5_L1_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 5 | 1.027 | 224992 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check/runs/correctness_fuzz/broom_mixed/n1024/seed5_L0_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 1 | 1.024 | 225008 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check/runs/correctness_fuzz/broom_mixed/n1024/seed1_L1_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 4 | 0.904 | 225056 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check/runs/correctness_fuzz/broom_mixed/n1024/seed4_L0_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 3 | 0.825 | 224624 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check/runs/correctness_fuzz/broom_mixed/n1024/seed3_L1_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 4 | 0.822 | 224864 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check/runs/correctness_fuzz/broom_mixed/n1024/seed4_L1_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 2 | 0.822 | 224912 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check/runs/correctness_fuzz/broom_mixed/n1024/seed2_L1_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 5 | 0.710 | 225008 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check/runs/correctness_fuzz/broom_mixed/n1024/seed5_L0_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 1 | 0.675 | 224896 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check/runs/correctness_fuzz/broom_mixed/n1024/seed1_L0_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 3 | 0.665 | 224560 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check/runs/correctness_fuzz/broom_mixed/n1024/seed3_L0_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 2 | 0.636 | 225024 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check/runs/correctness_fuzz/broom_mixed/n1024/seed2_L0_Q0 |
| correctness_fuzz | comb_rect_dense | 1024 | 5 | 0.608 | 68640 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check/runs/correctness_fuzz/comb_rect_dense/n1024/seed5_L0_Q0 |
