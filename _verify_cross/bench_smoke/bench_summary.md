# Benchmark summary (min over seeds)

solver: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/solve`

cmd: `bench_report.py --solver ./solve --out _verify_cross/bench_smoke --sizes 128,256 --seeds 1 --modes comb_rect_dense,multi_comb_rect --shuffle-labels --shuffle-queries --timeout 2.0`

| mode | 128 | 256 |
| --- | --- | --- |
| comb_rect_dense | 130.1ms | 22.9ms |
| multi_comb_rect | 12.6ms | 12.8ms |
