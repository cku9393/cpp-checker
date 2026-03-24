# Certification summary

overall verdict: **FAIL**

## Reasons

- correctness_fuzz: 259 failing cases

이 스위트는 hidden data를 완전히 복제하는 증명은 아니지만, decomposition 계열 느린 풀이와 잘못된 풀이를 매우 강하게 걸러내도록 설계됐다.

## Stage: correctness_fuzz

status: **FAIL**  
cases: 900  
timeouts: 198  
re/wa: 61  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| balanced_dense | 8 | 0.001 | 1.046 | 0.013 | 0.737 | 64:0.012, 128:0.013, 256:0.013, 512:0.013, 1024:0.012 |
| broom_mixed | 83 | 4.633 | 24.814 | 1.813 | 1.990 | 256:0.073, 512:1.813 |
| caterpillar_rect_dense | 22 | 1.813 | 13.419 | 0.657 | 1.944 | 64:0.013, 128:0.024, 256:0.049, 512:0.657 |
| chain_unary | 0 | 0.159 | 1.599 | 0.024 | 0.030 | 64:0.015, 128:0.013, 256:0.014, 512:0.015, 1024:0.024 |
| comb_rect_dense | 61 | 3.876 | 14.678 | 0.781 | 1.867 | 64:0.053, 128:0.781 |
| multi_comb_cap | 25 | 1.312 | 42.506 | 0.707 | 1.377 | 64:0.014, 128:0.015, 256:0.643, 512:0.054, 1024:0.707 |
| multi_comb_rect | 60 | 3.360 | 10.267 | 0.368 | 0.816 | 64:0.036, 128:0.368 |
| random_recursive_mixed | 0 | 0.195 | 1.930 | 0.025 | 0.746 | 64:0.013, 128:0.013, 256:0.013, 512:0.013, 1024:0.025 |
| star_pairs | 0 | 0.368 | 1.893 | 0.030 | 0.054 | 64:0.012, 128:0.014, 256:0.015, 512:0.029, 1024:0.030 |

## Top slow cases

| stage | mode | n | seed | sec | rss_kb | case_dir |
| --- | --- | --- | --- | --- | --- | --- |
| correctness_fuzz | broom_mixed | 512 | 3 | 1.990 | 4528 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_correctness_fuzz/runs/correctness_fuzz/broom_mixed/n512/seed3_L1_Q1 |
| correctness_fuzz | broom_mixed | 512 | 5 | 1.972 | 4528 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_correctness_fuzz/runs/correctness_fuzz/broom_mixed/n512/seed5_L1_Q0 |
| correctness_fuzz | broom_mixed | 512 | 1 | 1.963 | 4528 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_correctness_fuzz/runs/correctness_fuzz/broom_mixed/n512/seed1_L1_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 512 | 5 | 1.944 | 3536 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_correctness_fuzz/runs/correctness_fuzz/caterpillar_rect_dense/n512/seed5_L0_Q1 |
| correctness_fuzz | broom_mixed | 512 | 4 | 1.879 | 4528 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_correctness_fuzz/runs/correctness_fuzz/broom_mixed/n512/seed4_L1_Q1 |
| correctness_fuzz | comb_rect_dense | 128 | 3 | 1.867 | 10864 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_correctness_fuzz/runs/correctness_fuzz/comb_rect_dense/n128/seed3_L1_Q1 |
| correctness_fuzz | broom_mixed | 512 | 5 | 1.814 | 4512 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_correctness_fuzz/runs/correctness_fuzz/broom_mixed/n512/seed5_L0_Q1 |
| correctness_fuzz | broom_mixed | 512 | 2 | 1.813 | 4512 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_correctness_fuzz/runs/correctness_fuzz/broom_mixed/n512/seed2_L0_Q0 |
| correctness_fuzz | broom_mixed | 512 | 1 | 1.802 | 4512 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_correctness_fuzz/runs/correctness_fuzz/broom_mixed/n512/seed1_L0_Q0 |
| correctness_fuzz | broom_mixed | 512 | 3 | 1.791 | 4528 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_correctness_fuzz/runs/correctness_fuzz/broom_mixed/n512/seed3_L1_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 512 | 5 | 1.782 | 3552 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_correctness_fuzz/runs/correctness_fuzz/caterpillar_rect_dense/n512/seed5_L0_Q0 |
| correctness_fuzz | broom_mixed | 512 | 5 | 1.704 | 4480 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_correctness_fuzz/runs/correctness_fuzz/broom_mixed/n512/seed5_L0_Q0 |
| correctness_fuzz | broom_mixed | 512 | 3 | 1.678 | 4528 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_correctness_fuzz/runs/correctness_fuzz/broom_mixed/n512/seed3_L0_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 512 | 1 | 1.566 | 3584 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_correctness_fuzz/runs/correctness_fuzz/caterpillar_rect_dense/n512/seed1_L0_Q0 |
| correctness_fuzz | comb_rect_dense | 128 | 3 | 1.457 | 11872 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_correctness_fuzz/runs/correctness_fuzz/comb_rect_dense/n128/seed3_L1_Q0 |
| correctness_fuzz | multi_comb_cap | 256 | 3 | 1.377 | 10672 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_correctness_fuzz/runs/correctness_fuzz/multi_comb_cap/n256/seed3_L1_Q1 |
| correctness_fuzz | comb_rect_dense | 128 | 1 | 1.361 | 11280 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_correctness_fuzz/runs/correctness_fuzz/comb_rect_dense/n128/seed1_L1_Q0 |
| correctness_fuzz | comb_rect_dense | 128 | 1 | 1.359 | 10832 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_correctness_fuzz/runs/correctness_fuzz/comb_rect_dense/n128/seed1_L1_Q1 |
| correctness_fuzz | multi_comb_cap | 1024 | 1 | 1.297 | 4384 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_correctness_fuzz/runs/correctness_fuzz/multi_comb_cap/n1024/seed1_L1_Q1 |
| correctness_fuzz | comb_rect_dense | 128 | 5 | 1.125 | 8960 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_correctness_fuzz/runs/correctness_fuzz/comb_rect_dense/n128/seed5_L1_Q0 |
