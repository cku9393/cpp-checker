# Hardest-case hunt

상위 케이스는 현재 solver 기준으로 가장 느리게 측정된 조합이다. 느린 풀이를 반박하려면 이 목록에서 timeout/scale 문제가 없어야 한다.

| rank | mode | n | seed | L | Q | sec | rss_kb | val_ok | case_dir |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | multi_comb_core | 64 | 1 | 0 | 1 | 0.023 | 1600 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt_ac8_path_probe/runs/multi_comb_core/n64/seed1_L0_Q1 |
| 2 | chain_unary | 64 | 1 | 0 | 1 | 0.014 | 1680 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt_ac8_path_probe/runs/chain_unary/n64/seed1_L0_Q1 |
| 3 | comb_core | 64 | 1 | 0 | 0 | 0.014 | 1632 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt_ac8_path_probe/runs/comb_core/n64/seed1_L0_Q0 |
| 4 | comb_rect_dense | 64 | 1 | 0 | 0 | 0.014 | 1920 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt_ac8_path_probe/runs/comb_rect_dense/n64/seed1_L0_Q0 |
| 5 | comb_plus_unary | 64 | 1 | 1 | 1 | 0.013 | 1680 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt_ac8_path_probe/runs/comb_plus_unary/n64/seed1_L1_Q1 |
| 6 | comb_plus_unary | 64 | 1 | 0 | 1 | 0.013 | 1632 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt_ac8_path_probe/runs/comb_plus_unary/n64/seed1_L0_Q1 |
| 7 | star_pairs | 64 | 1 | 0 | 1 | 0.013 | 1616 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt_ac8_path_probe/runs/star_pairs/n64/seed1_L0_Q1 |
| 8 | multi_comb_cap | 64 | 1 | 0 | 0 | 0.013 | 1696 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt_ac8_path_probe/runs/multi_comb_cap/n64/seed1_L0_Q0 |
| 9 | caterpillar_mixed | 64 | 1 | 1 | 1 | 0.013 | 1872 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt_ac8_path_probe/runs/caterpillar_mixed/n64/seed1_L1_Q1 |
| 10 | broom_mixed | 64 | 1 | 1 | 1 | 0.013 | 1584 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt_ac8_path_probe/runs/broom_mixed/n64/seed1_L1_Q1 |
| 11 | balanced_sibling | 64 | 1 | 0 | 0 | 0.013 | 1616 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt_ac8_path_probe/runs/balanced_sibling/n64/seed1_L0_Q0 |
| 12 | chain_unary | 64 | 1 | 1 | 1 | 0.013 | 1664 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt_ac8_path_probe/runs/chain_unary/n64/seed1_L1_Q1 |
| 13 | comb_plus_unary | 64 | 1 | 1 | 0 | 0.013 | 1648 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt_ac8_path_probe/runs/comb_plus_unary/n64/seed1_L1_Q0 |
| 14 | broom_mixed | 64 | 1 | 0 | 1 | 0.013 | 1584 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt_ac8_path_probe/runs/broom_mixed/n64/seed1_L0_Q1 |
| 15 | comb_dense | 64 | 1 | 1 | 1 | 0.013 | 1872 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt_ac8_path_probe/runs/comb_dense/n64/seed1_L1_Q1 |
| 16 | balanced_sibling | 64 | 1 | 1 | 1 | 0.013 | 1616 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt_ac8_path_probe/runs/balanced_sibling/n64/seed1_L1_Q1 |
| 17 | broom_mixed | 64 | 1 | 1 | 0 | 0.013 | 1584 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt_ac8_path_probe/runs/broom_mixed/n64/seed1_L1_Q0 |
| 18 | comb_plus_unary | 64 | 1 | 0 | 0 | 0.013 | 1648 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt_ac8_path_probe/runs/comb_plus_unary/n64/seed1_L0_Q0 |
| 19 | caterpillar_rect_dense | 64 | 1 | 0 | 1 | 0.013 | 1856 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt_ac8_path_probe/runs/caterpillar_rect_dense/n64/seed1_L0_Q1 |
| 20 | comb_core | 64 | 1 | 0 | 1 | 0.013 | 1632 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt_ac8_path_probe/runs/comb_core/n64/seed1_L0_Q1 |
