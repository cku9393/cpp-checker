# branch_1 Workspace Guide

`branch_1` uses `ogdf_local_harness_bundle_v2/` as its active workspace.

Layout:

- `ogdf_local_harness_bundle_v2/`
  - active source tree
  - CLI entry: `src/rewrite_r_harness_main.cpp`
  - integrated solver path: `src/project_static_adapter.cpp`
  - built binary: `build/rewrite_r_harness`
- `README_ko.md`
  - older branch-level notes
- `ogdf_local_harness_bundle/`
  - older bundle snapshot
- `backups/`
  - archived checkpoints
- `root_legacy_dumps/`
  - root-level rewrite-r dump corpus moved under `branch_1/`
  - preserved legacy debug dumps, not the active workspace dump target

Execution model:

- `./build.sh` configures and builds `ogdf_local_harness_bundle_v2/build/rewrite_r_harness`
- `./run.sh` directly executes the branch-local solver binary
- `./smoke.sh` runs a small branch-local harness smoke and writes dumps under
  `ogdf_local_harness_bundle_v2/dumps/branch_smoke`
- outputs stay under `branch_1/ogdf_local_harness_bundle_v2/{build,dumps,artifacts}`

Recommended commands:

```bash
cd branch_1
./build.sh
./run.sh --backend ogdf --mode rewrite-seq --seed 1 --rounds 1 --dump-dir ogdf_local_harness_bundle_v2/dumps/manual
./smoke.sh
```

Notes:

- `build.sh` defaults `HARNESS_PROJECT_USE_FREE_FUNCTION_HOOKS=ON`
- set `BRANCH1_USE_OGDF=OFF` if you intentionally want the stub-only build
- current active runs should still write under `ogdf_local_harness_bundle_v2/dumps/...`
  rather than `root_legacy_dumps/`
