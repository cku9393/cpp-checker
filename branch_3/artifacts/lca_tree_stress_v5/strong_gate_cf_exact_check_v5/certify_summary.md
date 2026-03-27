# Certification summary

overall verdict: **FAIL**

## Reasons

- correctness_fuzz: 1 failing cases

이 스위트는 hidden data를 완전히 복제하는 증명은 아니지만, decomposition 계열 느린 풀이와 잘못된 풀이를 매우 강하게 걸러내도록 설계됐다.

## Stage: correctness_fuzz

status: **FAIL**  
cases: 900  
timeouts: 1  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| balanced_dense | 0 | 0.000 | 1.000 | 0.013 | 0.025 | 64:0.013, 128:0.013, 256:0.013, 512:0.013, 1024:0.013 |
| broom_mixed | 1 | 1.688 | 6.009 | 1.127 | 1.623 | 64:0.013, 128:0.013, 256:0.038, 512:0.188, 1024:1.127 |
| caterpillar_rect_dense | 0 | 1.290 | 4.421 | 0.407 | 0.868 | 64:0.013, 128:0.013, 256:0.024, 512:0.092, 1024:0.407 |
| chain_unary | 0 | 0.003 | 1.037 | 0.013 | 0.025 | 64:0.012, 128:0.013, 256:0.012, 512:0.012, 1024:0.013 |
| comb_rect_dense | 0 | 1.365 | 3.810 | 0.444 | 0.746 | 64:0.012, 128:0.013, 256:0.037, 512:0.117, 1024:0.444 |
| multi_comb_cap | 0 | 0.653 | 3.417 | 0.084 | 0.736 | 64:0.013, 128:0.012, 256:0.012, 512:0.025, 1024:0.084 |
| multi_comb_rect | 0 | 0.867 | 3.600 | 0.132 | 0.260 | 64:0.012, 128:0.012, 256:0.013, 512:0.045, 1024:0.132 |
| random_recursive_mixed | 0 | 0.003 | 1.025 | 0.013 | 0.013 | 64:0.013, 128:0.012, 256:0.013, 512:0.013, 1024:0.013 |
| star_pairs | 0 | 0.004 | 1.025 | 0.013 | 0.013 | 64:0.013, 128:0.012, 256:0.012, 512:0.013, 1024:0.013 |

## Top slow cases

| stage | mode | n | seed | sec | rss_kb | case_dir |
| --- | --- | --- | --- | --- | --- | --- |
| correctness_fuzz | broom_mixed | 1024 | 1 | 1.623 | 216016 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v5/runs/correctness_fuzz/broom_mixed/n1024/seed1_L1_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 4 | 1.454 | 216160 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v5/runs/correctness_fuzz/broom_mixed/n1024/seed4_L0_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 2 | 1.338 | 216016 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v5/runs/correctness_fuzz/broom_mixed/n1024/seed2_L0_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 1 | 1.271 | 216048 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v5/runs/correctness_fuzz/broom_mixed/n1024/seed1_L0_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 2 | 1.214 | 215936 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v5/runs/correctness_fuzz/broom_mixed/n1024/seed2_L0_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 5 | 1.154 | 216176 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v5/runs/correctness_fuzz/broom_mixed/n1024/seed5_L0_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 4 | 1.148 | 216064 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v5/runs/correctness_fuzz/broom_mixed/n1024/seed4_L1_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 2 | 1.133 | 215952 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v5/runs/correctness_fuzz/broom_mixed/n1024/seed2_L1_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 3 | 1.129 | 215792 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v5/runs/correctness_fuzz/broom_mixed/n1024/seed3_L1_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 3 | 1.127 | 215792 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v5/runs/correctness_fuzz/broom_mixed/n1024/seed3_L0_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 1 | 0.991 | 216032 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v5/runs/correctness_fuzz/broom_mixed/n1024/seed1_L1_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 4 | 0.978 | 216064 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v5/runs/correctness_fuzz/broom_mixed/n1024/seed4_L1_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 2 | 0.907 | 215920 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v5/runs/correctness_fuzz/broom_mixed/n1024/seed2_L1_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 3 | 0.889 | 215824 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v5/runs/correctness_fuzz/broom_mixed/n1024/seed3_L1_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 5 | 0.885 | 216032 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v5/runs/correctness_fuzz/broom_mixed/n1024/seed5_L1_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 512 | 4 | 0.868 | 17056 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v5/runs/correctness_fuzz/caterpillar_rect_dense/n512/seed4_L1_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 1 | 0.865 | 216048 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v5/runs/correctness_fuzz/broom_mixed/n1024/seed1_L0_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 5 | 0.841 | 216112 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v5/runs/correctness_fuzz/broom_mixed/n1024/seed5_L0_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 4 | 0.803 | 216064 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v5/runs/correctness_fuzz/broom_mixed/n1024/seed4_L0_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 3 | 0.759 | 215792 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v5/runs/correctness_fuzz/broom_mixed/n1024/seed3_L0_Q0 |
