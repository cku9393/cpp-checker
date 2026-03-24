# Ouroboros Start Prompts

Use one branch per session.
Do not run `branch_2_2` and `branch_3` in the same Ouroboros session.

The validation standard is the repo-root certification harness, but each
branch must build its own solver and keep outputs inside its own `artifacts/`
tree.

See also: [OUROBOROS_PREP.md](/Users/free_1/Library/Mobile%20Documents/iCloud~md~obsidian/Documents/cpp-checker/OUROBOROS_PREP.md)

## Recommended First Session: `branch_3`

```text
You are working only on /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3.

Goal:
- Optimize the branch-local solver so that this branch reproducibly satisfies the outer lca_tree_stress_v5 standard.
- "Success" does not mean a one-off run. Success means the same branch-root commands can be rerun and still meet the preset standard.

Active source:
- /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/boj28350_resume/boj28350_branch_3_solver.cpp

Rules:
- Work from the branch root.
- Build only with ./build.sh.
- Validate only with these branch-local wrappers:
  - ./lca_smoke.sh
  - ./lca_strong_gate.sh
  - ./lca_rebuttal_gate.sh
  - ./lca_boj3s_gate.sh
  - ./lca_hunt.sh
- Do not switch the target to the outer lca_tree_stress_v5 solver source.
- Keep outputs inside branch_3/artifacts/...
- Prefer the smallest validating command that gives useful signal before escalating to heavier gates.
- When you change code, explain which failure mode or bottleneck you are targeting.

Target ladder:
1. Stabilize ./lca_smoke.sh
2. Reach PASS on ./lca_strong_gate.sh
3. If needed, use ./lca_rebuttal_gate.sh to isolate scaling weaknesses
4. Push toward PASS on ./lca_boj3s_gate.sh
5. Use ./lca_hunt.sh to identify the slowest validated cases and optimize them
6. Re-run the relevant gate to confirm reproducibility

Important criteria:
- strong_gate target: PASS
- stretch target: boj_3s_hard_gate PASS
- artifacts and any solver-side auxiliary outputs must remain inside branch_3/artifacts/lca_tree_stress_v5/...

Execution policy:
- Start by inspecting the branch-local wrappers and current solver structure.
- Then run the lightest meaningful validation step.
- Iterate: inspect failure -> patch solver -> rerun the narrowest gate -> summarize progress.
- Do not stop after analysis; keep going until blocked or until you reach a meaningful gate improvement.
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
  - ./lca_smoke.sh
  - ./lca_strong_gate.sh
  - ./lca_rebuttal_gate.sh
  - ./lca_boj3s_gate.sh
  - ./lca_hunt.sh
- The outer standard wrappers use the plain solver binary at round45_resume/solve.
- The internal Round 45 profiling flow still exists through:
  - ./build.sh
  - ./run.sh
  - ./smoke.sh
- Keep outputs inside branch_2_2/artifacts/...
- Preserve branch-local row-file emission and profiling outputs.
- Do not switch the target to the outer lca_tree_stress_v5 solver source.

Target ladder:
1. Stabilize ./lca_smoke.sh
2. Reach PASS on ./lca_strong_gate.sh
3. If scaling fails, use ./lca_rebuttal_gate.sh and ./lca_hunt.sh to isolate the slow modes and sizes
4. Push toward PASS on ./lca_boj3s_gate.sh
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
