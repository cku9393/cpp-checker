# branch_2-1 Workspace Guide

`branch_2-1` uses `raw_engine_v1_package/` as its active workspace.

Layout:

- `raw_engine_v1_package/`
  - active source tree
  - solver implementation: `src/*.cpp`
  - CLI entry: `tests/raw_engine_main.cpp`
  - built binary: `build-debug/tests/raw_engine_tests`
- zipped deliverables and dated bundles
  - historical snapshots only

Execution model:

- `./build.sh` builds `raw_engine_v1_package/build-debug/tests/raw_engine_tests`
- `./run.sh` directly executes the branch-local `raw_engine_tests` binary
- `./smoke.sh` runs the branch-local `core` CTest tier
- outputs stay under `branch_2-1/raw_engine_v1_package/{build-*,artifacts}`

Recommended commands:

```bash
cd branch_2-1
./build.sh
./run.sh --case micro
./smoke.sh
```

Notes:

- keep build directories under `raw_engine_v1_package/build-*`
- keep artifact paths under `raw_engine_v1_package/artifacts/...`
- archived zips at branch root are reference deliverables, not the active workspace
