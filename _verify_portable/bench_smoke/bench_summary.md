# Benchmark summary (min over seeds)

solver: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/solve`

cmd: `bench_report.py --solver ./solve --out _verify_portable/bench_smoke --modes comb_rect_dense,multi_comb_rect --sizes 128,256 --seeds 1 --shuffle-labels --shuffle-queries --timeout 2.0 --keep`

| mode | 128 | 256 |
| --- | --- | --- |
| comb_rect_dense | 412.1ms | 25.1ms |
| multi_comb_rect | 12.6ms | 11.1ms |
