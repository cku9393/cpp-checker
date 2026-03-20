# raw_engine_v1 package

First-working-version raw primitive engine, split into a small CMake project.

Structure:
- `include/raw_engine/raw_engine.hpp`
- `src/raw_core.cpp`
- `src/raw_validators.cpp`
- `src/raw_primitives.cpp`
- `src/raw_planner.cpp`
- `tests/raw_engine_cases.cpp`
- `tests/raw_engine_main.cpp`

Primary target:
- `raw_engine_tests`

CLI examples:
```bash
./raw_engine_tests --case micro
./raw_engine_tests --case fuzz --seed 44001 --iters 1
./raw_engine_tests --case planner --seed 123 --iters 1000
./raw_engine_tests --case fuzz_matrix --iters 20
./raw_engine_tests --case isolate_fuzz --seed 710001 --iters 100
./raw_engine_tests --case split_join_fuzz --seed 715001 --iters 100
./raw_engine_tests --case planner_mixed_fuzz --seed 717001 --iters 100 --step-budget 200000
./raw_engine_tests --case regression_44001
./raw_engine_tests --case regression_isolate_split_no_sep
./raw_engine_tests --case repro --repro-file counterexamples/reduced_isolate_split_seed3736675150.txt
```

Build and run:
```bash
bash build_and_run.sh
```

If `cmake` / `ctest` are not on `PATH`, pass them explicitly:
```bash
CMAKE_BIN=/path/to/cmake CTEST_BIN=/path/to/ctest bash build_and_run.sh
```

Manual commands:
```bash
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug -j
ctest --test-dir build-debug --output-on-failure
```

Known fixed repros:
```bash
./raw_engine_tests --case regression_44001
./raw_engine_tests --case regression_isolate_split_no_sep
```

Sync policy:
- `include/`, `src/`, `tests/`, `CMakeLists.txt` are the authoritative source tree.
- `raw_engine_v1.cpp` is a compatibility/export artifact, not the source of truth.
- New implementation/debug/test work must land in the split source tree first.
- If a standalone snapshot is needed again, regenerate/export from the split tree instead of patching `raw_engine_v1.cpp` directly.
