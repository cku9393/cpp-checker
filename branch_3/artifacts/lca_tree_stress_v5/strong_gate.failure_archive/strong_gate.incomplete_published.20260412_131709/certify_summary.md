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
| balanced_dense | 0 | 0.827 | 2.198 | 0.132 | 0.135 | 64:0.015, 128:0.015, 256:0.030, 512:0.060, 1024:0.132 |
| broom_mixed | 0 | 1.247 | 3.058 | 0.491 | 0.870 | 64:0.015, 128:0.030, 256:0.060, 512:0.160, 1024:0.491 |
| caterpillar_rect_dense | 0 | 1.733 | 5.205 | 1.801 | 1.995 | 64:0.015, 128:0.030, 256:0.090, 512:0.346, 1024:1.801 |
| chain_unary | 0 | 1.504 | 4.363 | 0.698 | 0.864 | 64:0.015, 128:0.030, 256:0.131, 512:0.471, 1024:0.698 |
| comb_rect_dense | 0 | 1.599 | 4.047 | 1.216 | 1.275 | 64:0.015, 128:0.030, 256:0.090, 512:0.300, 1024:1.216 |
| multi_comb_cap | 0 | 1.178 | 2.999 | 0.330 | 0.353 | 64:0.015, 128:0.015, 256:0.045, 512:0.110, 1024:0.330 |
| multi_comb_rect | 0 | 1.243 | 3.334 | 0.501 | 0.538 | 64:0.015, 128:0.030, 256:0.060, 512:0.150, 1024:0.501 |
| random_recursive_mixed | 0 | 0.791 | 1.999 | 0.117 | 0.120 | 64:0.015, 128:0.015, 256:0.030, 512:0.060, 1024:0.117 |
| star_pairs | 0 | 1.099 | 2.926 | 0.240 | 0.256 | 64:0.015, 128:0.030, 256:0.088, 512:0.240, 1024:0.240 |

## Top slow cases

| stage | mode | n | seed | sec | rss_kb | case_dir |
| --- | --- | --- | --- | --- | --- | --- |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 2 | 1.995 | 615888 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed2_L1_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 2 | 1.976 | 611136 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed2_L1_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 2 | 1.975 | 611264 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed2_L0_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 2 | 1.968 | 606352 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed2_L0_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 4 | 1.823 | 539312 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed4_L0_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 4 | 1.815 | 539120 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed4_L1_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 1 | 1.810 | 545168 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed1_L0_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 1 | 1.806 | 544768 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed1_L1_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 4 | 1.805 | 539184 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed4_L0_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 1 | 1.802 | 545056 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed1_L0_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 4 | 1.800 | 543280 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed4_L1_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 1 | 1.792 | 550288 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed1_L1_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 5 | 1.753 | 515520 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed5_L1_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 5 | 1.751 | 515536 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed5_L0_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 5 | 1.745 | 514912 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed5_L1_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 3 | 1.736 | 501344 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed3_L0_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 3 | 1.736 | 501920 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed3_L1_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 5 | 1.735 | 515456 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed5_L0_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 3 | 1.733 | 502208 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed3_L1_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 3 | 1.731 | 506896 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed3_L0_Q0 |
