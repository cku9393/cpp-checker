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
| balanced_dense | 0 | 0.834 | 2.250 | 0.135 | 0.135 | 64:0.015, 128:0.015, 256:0.030, 512:0.060, 1024:0.135 |
| broom_mixed | 0 | 1.248 | 2.979 | 0.487 | 0.855 | 64:0.015, 128:0.030, 256:0.060, 512:0.164, 1024:0.487 |
| caterpillar_rect_dense | 0 | 1.734 | 5.231 | 1.806 | 2.013 | 64:0.015, 128:0.030, 256:0.090, 512:0.345, 1024:1.806 |
| chain_unary | 0 | 1.505 | 4.378 | 0.703 | 0.727 | 64:0.015, 128:0.030, 256:0.132, 512:0.465, 1024:0.703 |
| comb_rect_dense | 0 | 1.638 | 4.156 | 1.342 | 1.448 | 64:0.015, 128:0.030, 256:0.104, 512:0.323, 1024:1.342 |
| multi_comb_cap | 0 | 1.204 | 2.997 | 0.345 | 0.345 | 64:0.015, 128:0.015, 256:0.045, 512:0.120, 1024:0.345 |
| multi_comb_rect | 0 | 1.289 | 3.382 | 0.559 | 0.574 | 64:0.015, 128:0.030, 256:0.060, 512:0.165, 1024:0.559 |
| random_recursive_mixed | 0 | 0.800 | 2.000 | 0.120 | 0.120 | 64:0.015, 128:0.015, 256:0.030, 512:0.060, 1024:0.120 |
| star_pairs | 0 | 1.115 | 3.000 | 0.253 | 0.297 | 64:0.015, 128:0.030, 256:0.090, 512:0.240, 1024:0.253 |

## Top slow cases

| stage | mode | n | seed | sec | rss_kb | case_dir |
| --- | --- | --- | --- | --- | --- | --- |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 2 | 2.013 | 623200 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/ac3_direct_state1_correctness_current/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed2_L1_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 2 | 2.012 | 623920 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/ac3_direct_state1_correctness_current/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed2_L1_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 2 | 2.003 | 618368 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/ac3_direct_state1_correctness_current/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed2_L0_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 2 | 1.989 | 619168 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/ac3_direct_state1_correctness_current/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed2_L0_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 4 | 1.827 | 552448 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/ac3_direct_state1_correctness_current/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed4_L1_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 4 | 1.825 | 551536 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/ac3_direct_state1_correctness_current/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed4_L0_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 1 | 1.823 | 556528 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/ac3_direct_state1_correctness_current/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed1_L0_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 1 | 1.815 | 557712 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/ac3_direct_state1_correctness_current/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed1_L1_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 4 | 1.814 | 551664 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/ac3_direct_state1_correctness_current/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed4_L1_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 1 | 1.811 | 557584 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/ac3_direct_state1_correctness_current/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed1_L1_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 4 | 1.802 | 552064 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/ac3_direct_state1_correctness_current/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed4_L0_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 1 | 1.800 | 557472 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/ac3_direct_state1_correctness_current/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed1_L0_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 5 | 1.797 | 527264 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/ac3_direct_state1_correctness_current/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed5_L0_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 5 | 1.784 | 527520 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/ac3_direct_state1_correctness_current/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed5_L1_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 5 | 1.784 | 527424 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/ac3_direct_state1_correctness_current/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed5_L1_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 5 | 1.764 | 527536 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/ac3_direct_state1_correctness_current/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed5_L0_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 3 | 1.740 | 514016 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/ac3_direct_state1_correctness_current/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed3_L1_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 3 | 1.739 | 513296 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/ac3_direct_state1_correctness_current/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed3_L1_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 3 | 1.736 | 513600 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/ac3_direct_state1_correctness_current/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed3_L0_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 3 | 1.719 | 513440 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/ac3_direct_state1_correctness_current/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed3_L0_Q0 |
