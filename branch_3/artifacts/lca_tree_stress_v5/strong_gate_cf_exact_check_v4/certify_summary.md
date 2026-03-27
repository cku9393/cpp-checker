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
| balanced_dense | 0 | 0.013 | 1.060 | 0.013 | 0.013 | 64:0.013, 128:0.012, 256:0.012, 512:0.013, 1024:0.013 |
| broom_mixed | 1 | 1.778 | 6.664 | 1.425 | 1.960 | 64:0.012, 128:0.013, 256:0.044, 512:0.214, 1024:1.425 |
| caterpillar_rect_dense | 0 | 1.452 | 4.711 | 0.604 | 0.959 | 64:0.013, 128:0.013, 256:0.037, 512:0.128, 1024:0.604 |
| chain_unary | 0 | 0.005 | 1.064 | 0.013 | 0.054 | 64:0.012, 128:0.013, 256:0.012, 512:0.013, 1024:0.013 |
| comb_rect_dense | 0 | 1.310 | 4.358 | 0.423 | 0.587 | 64:0.013, 128:0.013, 256:0.025, 512:0.097, 1024:0.423 |
| multi_comb_cap | 0 | 0.649 | 3.354 | 0.084 | 0.528 | 64:0.013, 128:0.013, 256:0.013, 512:0.025, 1024:0.084 |
| multi_comb_rect | 0 | 0.816 | 3.485 | 0.125 | 0.168 | 64:0.013, 128:0.013, 256:0.013, 512:0.036, 1024:0.125 |
| random_recursive_mixed | 0 | 0.088 | 1.359 | 0.017 | 0.088 | 64:0.013, 128:0.013, 256:0.013, 512:0.013, 1024:0.017 |
| star_pairs | 0 | 0.000 | 1.001 | 0.013 | 0.026 | 64:0.013, 128:0.013, 256:0.013, 512:0.013, 1024:0.013 |

## Top slow cases

| stage | mode | n | seed | sec | rss_kb | case_dir |
| --- | --- | --- | --- | --- | --- | --- |
| correctness_fuzz | broom_mixed | 1024 | 3 | 1.960 | 215776 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v4/runs/correctness_fuzz/broom_mixed/n1024/seed3_L0_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 3 | 1.858 | 215824 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v4/runs/correctness_fuzz/broom_mixed/n1024/seed3_L1_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 4 | 1.797 | 216064 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v4/runs/correctness_fuzz/broom_mixed/n1024/seed4_L1_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 5 | 1.695 | 216064 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v4/runs/correctness_fuzz/broom_mixed/n1024/seed5_L0_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 5 | 1.661 | 216096 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v4/runs/correctness_fuzz/broom_mixed/n1024/seed5_L1_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 2 | 1.656 | 215936 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v4/runs/correctness_fuzz/broom_mixed/n1024/seed2_L1_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 4 | 1.647 | 216112 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v4/runs/correctness_fuzz/broom_mixed/n1024/seed4_L0_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 4 | 1.536 | 216080 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v4/runs/correctness_fuzz/broom_mixed/n1024/seed4_L1_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 2 | 1.493 | 215952 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v4/runs/correctness_fuzz/broom_mixed/n1024/seed2_L0_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 1 | 1.425 | 216016 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v4/runs/correctness_fuzz/broom_mixed/n1024/seed1_L0_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 1 | 1.397 | 216016 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v4/runs/correctness_fuzz/broom_mixed/n1024/seed1_L1_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 5 | 1.382 | 216096 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v4/runs/correctness_fuzz/broom_mixed/n1024/seed5_L1_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 3 | 1.358 | 215824 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v4/runs/correctness_fuzz/broom_mixed/n1024/seed3_L1_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 3 | 1.168 | 215808 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v4/runs/correctness_fuzz/broom_mixed/n1024/seed3_L0_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 1 | 1.154 | 215984 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v4/runs/correctness_fuzz/broom_mixed/n1024/seed1_L1_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 4 | 1.133 | 216080 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v4/runs/correctness_fuzz/broom_mixed/n1024/seed4_L0_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 2 | 1.132 | 215984 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v4/runs/correctness_fuzz/broom_mixed/n1024/seed2_L0_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 5 | 1.071 | 216112 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v4/runs/correctness_fuzz/broom_mixed/n1024/seed5_L0_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 1 | 0.979 | 216016 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v4/runs/correctness_fuzz/broom_mixed/n1024/seed1_L0_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 3 | 0.959 | 63072 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_cf_exact_check_v4/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed3_L0_Q1 |
