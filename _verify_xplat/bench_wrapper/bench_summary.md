# Benchmark summary (min over seeds)

solver: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/solve`

cmd: `bench_report.py --solver ./solve --out _verify_xplat/bench_wrapper --sizes 128,256 --seeds 1 --modes comb_rect_dense,multi_comb_rect --shuffle-labels --shuffle-queries --timeout 2.0`

| mode | 128 | 256 |
| --- | --- | --- |
| comb_rect_dense | 13.1ms | 24.8ms |
| multi_comb_rect | 12.2ms | 12.6ms |
