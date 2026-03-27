# Certification summary

overall verdict: **FAIL**

## Reasons

- correctness_fuzz: 98 failing cases

이 스위트는 hidden data를 완전히 복제하는 증명은 아니지만, decomposition 계열 느린 풀이와 잘못된 풀이를 매우 강하게 걸러내도록 설계됐다.

## Stage: correctness_fuzz

status: **FAIL**  
cases: 900  
timeouts: 98  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| balanced_dense | 0 | 0.229 | 1.621 | 0.382 | 0.494 | 64:0.206, 128:0.165, 256:0.177, 512:0.235, 1024:0.382 |
| broom_mixed | 0 | 0.630 | 2.593 | 0.943 | 1.319 | 64:0.156, 128:0.168, 256:0.220, 512:0.364, 1024:0.943 |
| caterpillar_rect_dense | 29 | 1.118 | 4.008 | 1.605 | 1.789 | 64:0.156, 128:0.189, 256:0.400, 512:1.605 |
| chain_unary | 0 | 0.804 | 2.697 | 1.486 | 1.905 | 64:0.201, 128:0.232, 256:0.412, 512:1.111, 1024:1.486 |
| comb_rect_dense | 38 | 1.081 | 4.109 | 1.837 | 1.866 | 64:0.190, 128:0.224, 256:0.447, 512:1.837 |
| multi_comb_cap | 0 | 0.636 | 3.504 | 1.523 | 1.892 | 64:0.226, 128:0.239, 256:0.303, 512:0.435, 1024:1.523 |
| multi_comb_rect | 20 | 0.606 | 2.212 | 0.748 | 0.968 | 64:0.210, 128:0.229, 256:0.338, 512:0.748 |
| random_recursive_mixed | 11 | 0.462 | 2.352 | 0.478 | 0.975 | 64:0.158, 128:0.162, 256:0.381, 512:0.436, 1024:0.478 |
| star_pairs | 0 | 0.571 | 2.343 | 0.585 | 0.662 | 64:0.151, 128:0.167, 256:0.248, 512:0.581, 1024:0.585 |

## Top slow cases

| stage | mode | n | seed | sec | rss_kb | case_dir |
| --- | --- | --- | --- | --- | --- | --- |
| correctness_fuzz | chain_unary | 1024 | 1 | 1.905 | 36240 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_filter_probe_direct/runs/correctness_fuzz/chain_unary/n1024/seed1_L1_Q1 |
| correctness_fuzz | multi_comb_cap | 1024 | 4 | 1.892 | 136192 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_filter_probe_direct/runs/correctness_fuzz/multi_comb_cap/n1024/seed4_L1_Q1 |
| correctness_fuzz | comb_rect_dense | 512 | 1 | 1.866 | 212256 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_filter_probe_direct/runs/correctness_fuzz/comb_rect_dense/n512/seed1_L1_Q0 |
| correctness_fuzz | multi_comb_cap | 1024 | 2 | 1.833 | 134240 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_filter_probe_direct/runs/correctness_fuzz/multi_comb_cap/n1024/seed2_L1_Q0 |
| correctness_fuzz | comb_rect_dense | 512 | 1 | 1.808 | 212192 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_filter_probe_direct/runs/correctness_fuzz/comb_rect_dense/n512/seed1_L1_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 512 | 4 | 1.789 | 140704 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_filter_probe_direct/runs/correctness_fuzz/caterpillar_rect_dense/n512/seed4_L0_Q1 |
| correctness_fuzz | multi_comb_cap | 1024 | 4 | 1.770 | 136160 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_filter_probe_direct/runs/correctness_fuzz/multi_comb_cap/n1024/seed4_L1_Q0 |
| correctness_fuzz | multi_comb_cap | 1024 | 2 | 1.768 | 134160 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_filter_probe_direct/runs/correctness_fuzz/multi_comb_cap/n1024/seed2_L1_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 512 | 5 | 1.715 | 211792 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_filter_probe_direct/runs/correctness_fuzz/caterpillar_rect_dense/n512/seed5_L1_Q0 |
| correctness_fuzz | multi_comb_cap | 1024 | 5 | 1.688 | 124064 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_filter_probe_direct/runs/correctness_fuzz/multi_comb_cap/n1024/seed5_L1_Q0 |
| correctness_fuzz | multi_comb_cap | 1024 | 5 | 1.687 | 123888 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_filter_probe_direct/runs/correctness_fuzz/multi_comb_cap/n1024/seed5_L1_Q1 |
| correctness_fuzz | chain_unary | 1024 | 3 | 1.676 | 36208 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_filter_probe_direct/runs/correctness_fuzz/chain_unary/n1024/seed3_L1_Q0 |
| correctness_fuzz | chain_unary | 1024 | 2 | 1.652 | 36144 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_filter_probe_direct/runs/correctness_fuzz/chain_unary/n1024/seed2_L1_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 512 | 1 | 1.642 | 158752 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_filter_probe_direct/runs/correctness_fuzz/caterpillar_rect_dense/n512/seed1_L1_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 512 | 1 | 1.636 | 134336 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_filter_probe_direct/runs/correctness_fuzz/caterpillar_rect_dense/n512/seed1_L0_Q0 |
| correctness_fuzz | chain_unary | 1024 | 3 | 1.633 | 36256 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_filter_probe_direct/runs/correctness_fuzz/chain_unary/n1024/seed3_L1_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 512 | 5 | 1.625 | 141200 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_filter_probe_direct/runs/correctness_fuzz/caterpillar_rect_dense/n512/seed5_L0_Q1 |
| correctness_fuzz | chain_unary | 1024 | 1 | 1.622 | 36224 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_filter_probe_direct/runs/correctness_fuzz/chain_unary/n1024/seed1_L1_Q0 |
| correctness_fuzz | chain_unary | 1024 | 3 | 1.611 | 36144 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_filter_probe_direct/runs/correctness_fuzz/chain_unary/n1024/seed3_L0_Q0 |
| correctness_fuzz | multi_comb_cap | 1024 | 3 | 1.606 | 113904 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_filter_probe_direct/runs/correctness_fuzz/multi_comb_cap/n1024/seed3_L1_Q1 |
