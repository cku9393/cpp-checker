# Ouroboros Start Prompts

Use one branch per session.
Do not run `branch_2_2` and `branch_3` in the same Ouroboros session.

The validation standard is the `lca_tree_stress_v5/tooling/` certification harness, but each
branch must build its own solver and keep outputs inside its own `artifacts/`
tree.

See also: [OUROBOROS_PREP.md](/Users/free_1/Library/Mobile%20Documents/iCloud~md~obsidian/Documents/cpp-checker/OUROBOROS_PREP.md)

## Recommended First Session: `branch_3`

```text
You are working only on /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3.

Goal:
- Continue the progress40-derived BOJ 28350 research line inside branch_3 and make that research-direction solver reproducibly satisfy the outer lca_tree_stress_v5 standard.
- Success does not mean a one-off run. Success means the same branch-root commands can be rerun and still meet the preset standard.
- A failed gate is not a stopping condition by itself. Keep iterating on the branch-local solver and supporting branch-local helpers until the required gates pass or you are truly blocked.

Active source:
- /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/boj28350_resume/boj28350_branch_3_solver.cpp

Research context to read before major edits:
- /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/boj28350_resume/README.md
- /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/boj28350_resume/current_state_summary.md
- /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/boj28350_resume/next_session_briefing.md
- /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/boj28350_complete_master_document_partA_raw.md
- /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/boj28350_integrated_technical_history.md
- /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/boj28350_literature_progress7_bcdecomp_report.md
- /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/literature_grade_proof_package.md
- /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/boj28350_bundle_archive/boj28350_literature_progress40_layout_signature_reuse_gate.cpp
- /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/boj28350_bundle_archive/boj28350_progress40_layout_signature_reuse_gate_report.md
- /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/boj28350_bundle_archive/boj28350_progress40_results_merged.json
- /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/latest_failure_report.md
- /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/latest_failure_breakdown.md
- /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/latest_analysis_session.md
- /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/.ouroboros/failure_analysis_state.json
- /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/.ouroboros/failure_analysis_iteration.md
- /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/.ouroboros/failure_analysis_playbook.md

Rules:
- Work from the branch root.
- Build only with ./build.sh.
- Validate only with these branch-local wrappers:
  - ./outer_suite_wrappers/lca_smoke.sh
  - ./outer_suite_wrappers/lca_strong_gate.sh
  - ./outer_suite_wrappers/lca_rebuttal_gate.sh
  - ./outer_suite_wrappers/lca_boj3s_gate.sh
  - ./outer_suite_wrappers/lca_hunt.sh
- Do not switch the target to the outer lca_tree_stress_v5 solver source.
- Keep outputs inside branch_3/artifacts/...
- Prefer the smallest validating command that gives useful signal before escalating to heavier gates.
- When you change code, explain which failure mode or bottleneck you are targeting.
- Treat branch_3 as a progress40-derived research branch, not as a blank-slate solver branch.
- Preserve the literature/progress40 algorithmic direction and accumulated research intent.
- Do not replace the active solver with a fundamentally different branch-local algorithm family just to chase short-term gate passes.
- If the current branch_3 file has drifted away from progress40, first reconstruct the intended research direction from the branch_3 notes and bundled progress40 materials, then optimize within that line.
- If this is a retry attempt after a failed workflow, read the latest failure report first and use it to identify the failing AC, blocked downstream gates, and the last relevant artifact summaries.
- If this is a retry attempt after a failed workflow, also read the latest failure breakdown and use it to identify which execution phase and which code structures were most implicated.
- If this is a retry attempt after a failed workflow, also read the latest analysis-session handoff and failure-analysis state so the next retry inherits the narrowed AC, path, symbol, and line-range focus.
- If this is a retry attempt after a failed workflow, also read the latest failure-analysis iteration ledger and treat it as mandatory carry-over context from the analysis mini-session.
- If this is a retry attempt after a failed workflow, also read the failure-analysis playbook and apply any rule that the previous analysis mini-session added to sharpen the next diagnosis.
- If repeated failures are still localized only at a broad file level, tighten the retry tooling itself so the next failure report narrows the issue to specific wrapper sections, functions, line ranges, or code snippets.

Target ladder:
1. Stabilize ./outer_suite_wrappers/lca_smoke.sh
2. Reach PASS on ./outer_suite_wrappers/lca_strong_gate.sh
3. If needed, use ./outer_suite_wrappers/lca_rebuttal_gate.sh to isolate scaling weaknesses
4. Push toward PASS on ./outer_suite_wrappers/lca_boj3s_gate.sh
5. Use ./outer_suite_wrappers/lca_hunt.sh to identify the slowest validated cases and optimize them
6. Re-run the relevant gate to confirm reproducibility

Important criteria:
- strong_gate target: PASS
- stretch target: boj_3s_hard_gate PASS
- artifacts and any solver-side auxiliary outputs must remain inside branch_3/artifacts/lca_tree_stress_v5/...

Execution policy:
- Start by reading the branch_3 research notes and bundled progress40 materials, then inspect the branch-local wrappers and current solver structure.
- Treat the four additional branch_3 history/proof documents as required reading before major rewrites, not optional background.
- Then run the lightest meaningful validation step.
- Iterate: inspect failure -> patch solver -> rerun the narrowest gate -> summarize progress.
- After a gate or workflow failure, do not immediately restart blindly. First inspect where the failure occurred, record the concrete failure path and artifact evidence, decompose the stuck path into phases and relevant code hotspots, then continue from that diagnosis.
- On repeated failures, refine the diagnosis instead of rewriting it from scratch: compare the new failure against the previous breakdown, identify recurring hotspots, and tighten the next retry around those repeated bottlenecks.
- Apply the same rule to the analysis mini-session itself: each new analysis pass should start from the previous analysis handoff/state/playbook and leave behind a narrower analysis target than before.
- If the current failure breakdown is still too coarse to guide a fix, extend the branch-local failure-analysis logic before the next heavy retry so the next report captures narrower code-local evidence.
- Do not stop after one failed gate if there is still a branch-local path forward; keep going until blocked or until you achieve the required reproducible gate passes.
```

## Second Session: `branch_2_2`

```text
You are working only on /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_2_2.

Goal:
- Optimize the branch-local Round 45 solver so that this branch reproducibly satisfies the outer lca_tree_stress_v5 standard.
- "Success" means repeatable preset PASS, not a one-off successful run.

Active source:
- /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_2_2/round45_resume/round45_branch_2_2_solver.cpp

Rules:
- Work from the branch root.
- Build only with ./build.sh.
- Use these branch-local wrappers for the outer standard:
  - ./outer_suite_wrappers/lca_smoke.sh
  - ./outer_suite_wrappers/lca_strong_gate.sh
  - ./outer_suite_wrappers/lca_rebuttal_gate.sh
  - ./outer_suite_wrappers/lca_boj3s_gate.sh
  - ./outer_suite_wrappers/lca_hunt.sh
- The outer standard wrappers use the plain solver binary at round45_resume/solve.
- The internal Round 45 profiling flow still exists through:
  - ./build.sh
  - ./run.sh
  - ./smoke.sh
- Keep outputs inside branch_2_2/artifacts/...
- Preserve branch-local row-file emission and profiling outputs.
- Do not switch the target to the outer lca_tree_stress_v5 solver source.

Target ladder:
1. Stabilize ./outer_suite_wrappers/lca_smoke.sh
2. Reach PASS on ./outer_suite_wrappers/lca_strong_gate.sh
3. If scaling fails, use ./outer_suite_wrappers/lca_rebuttal_gate.sh and ./outer_suite_wrappers/lca_hunt.sh to isolate the slow modes and sizes
4. Push toward PASS on ./outer_suite_wrappers/lca_boj3s_gate.sh
5. Re-run the relevant gate to confirm reproducibility

Important criteria:
- strong_gate target: PASS
- stretch target: boj_3s_hard_gate PASS
- artifacts and row files must remain inside branch_2_2/artifacts/lca_tree_stress_v5/... or branch_2_2/artifacts/round45_resume/...

Execution policy:
- Start by checking the wrapper flow and the current active solver.
- Use the smallest useful validation command first.
- If correctness is fine but scaling fails, focus on the concrete hot path shown by the failing gate or hunt output.
- After each meaningful patch, rerun the narrowest gate that can confirm whether the change helped.
- Continue until blocked or until you improve the target gate status in a reproducible way.
```

## Minimal Operator Notes

- Start with `branch_3` first.
- Open a separate Ouroboros session for `branch_2_2` after the `branch_3` command set is stable.
- If you need a shorter prompt, copy one of the blocks above and remove the explanatory lines, but keep:
  - target branch
  - active source path
  - allowed wrapper commands
  - artifact-locality rule
  - success definition as reproducible preset `PASS`
