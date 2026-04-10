# Pre-Rewrite Synthesis Note

Date: `2026-04-04`

This is the short branch-local pre-rewrite checkpoint summary to cite before any
major solver rewrite or pivot in `branch_3`. It distills the current
branch-local research notes plus the bundled `progress40`
source/report/results set and does not replace the fuller ledger in
`boj28350_resume/pre_rewrite_checkpoint.md`.

## 2026-04-04 Sub-AC 3 pre-rewrite checkpoint

This note is the active brief planning/research-log checkpoint for the current
`branch_3` line.

- `reviewed source set A`: COMPLETE on `2026-04-04`
- `reviewed source set B`: COMPLETE on `2026-04-04`
- `rewrite/pivot start condition`: do not begin any major solver rewrite or
  pivot until a later planning or retry note cites this checkpoint together
  with `boj28350_resume/pre_rewrite_checkpoint.md`

The paired review remains the gate: both source sets were reviewed before any
later solver rewrite or pivot may begin, and the allowed direction stays inside
the literature-grade plus bundled `progress40` research line summarized below.

## 2026-04-04 refresh synthesis

This dated refresh reaffirms the two source sets that must ground any later
major rewrite:

- branch-local notes side: `boj28350_resume/README.md`,
  `boj28350_resume/current_state_summary.md`,
  `boj28350_resume/next_session_briefing.md`,
  `boj28350_complete_master_document_partA_raw.md`,
  `boj28350_integrated_technical_history.md`,
  `boj28350_literature_progress7_bcdecomp_report.md`,
  `literature_grade_proof_package.md`,
  `boj28350_resume/progress40_derived_reference.md`,
  `boj28350_resume/pre_rewrite_checkpoint.md`
- bundled progress40 side:
  `boj28350_bundle_archive/boj28350_literature_progress40_layout_signature_reuse_gate.cpp`,
  `boj28350_bundle_archive/boj28350_progress40_layout_signature_reuse_gate_report.md`,
  `boj28350_bundle_archive/boj28350_progress40_results_merged.json`

Short synthesis locked again on `2026-04-04`:

1. The branch-local notes still say the active solver started from a bundled
   `progress40` snapshot but has drifted into a separator-decomposition branch.
   Any later major rewrite must reduce that drift by re-anchoring to bundled
   `progress40`, not by extending the branch-local detour.
2. The proof/history package still fixes the allowed structure: preserve the
   literature-grade BC-tree flavored explicit child lattice and the
   `ensureLatticeChildren(...)`, `closeByBCPath(...)`,
   `buildClosedHandleFromWitness(...)` path rather than pivoting to a different
   algorithm family.
3. The bundled `progress40` materials still fix the optimization corridor:
   route-aware `lgate_*` / `time_lgate_*` attribution stays in scope, and the
   first residual axis remains `zero-span eligibility and fastpath commit`,
   with `signature source load and materialize` plus
   `layout signature compare and reuse gate core` remaining secondary.
4. The bundled package is still only `partial`, so it cannot substitute for
   fresh same-worktree branch-local required-gate evidence. Any future rewrite
   plan must keep reproducibility and fresh `./lca_strong_gate.sh` /
   `./lca_boj3s_gate.sh` closure in the contract.

## Source sets summarized

- `reviewed source set A`: branch-local notes and proof/history package:
  `boj28350_resume/README.md`,
  `boj28350_resume/current_state_summary.md`,
  `boj28350_resume/next_session_briefing.md`,
  `boj28350_complete_master_document_partA_raw.md`,
  `boj28350_integrated_technical_history.md`,
  `boj28350_literature_progress7_bcdecomp_report.md`,
  `literature_grade_proof_package.md`,
  `boj28350_resume/progress40_derived_reference.md`,
  `boj28350_resume/pre_rewrite_checkpoint.md`
- `reviewed source set B`: bundled `progress40` materials:
  `boj28350_bundle_archive/boj28350_literature_progress40_layout_signature_reuse_gate.cpp`,
  `boj28350_bundle_archive/boj28350_progress40_layout_signature_reuse_gate_report.md`,
  `boj28350_bundle_archive/boj28350_progress40_results_merged.json`

## Key takeaways from source set A

1. The allowed solver family is still the literature-grade line from the
   progress7/proof-package materials: BC-tree flavored decomposition, explicit
   child lattice semantics, and the minimal closed-subtree handle lift remain
   the structural anchor.
2. In concrete solver terms, a major rewrite must preserve the
   `ensureLatticeChildren(...)`, `closeByBCPath(...)`, and
   `buildClosedHandleFromWitness(...)` path rather than swap in a different
   algorithm family or a heuristic-only branch-local shortcut.
3. The branch-local notes still describe the active solver as having
   separator-decomposition drift relative to the intended line, so the next
   rewrite should reduce that drift, not widen it.
4. Branch-local validation and reproducibility constraints are part of the
   research contract: later closure still requires fresh same-working-tree
   branch-local evidence from `./lca_strong_gate.sh` and
   `./lca_boj3s_gate.sh`, with no manual artifact cleanup between reruns.

## Key takeaways from source set B

1. Bundled `progress40` is still the authoritative optimization direction for
   this branch. It is not a new solver family; it is the accumulated
   pack/normalize -> same-layout reuse -> layout-signature gate ->
   zero-span eligibility -> fastpath commit line on top of `progress39`.
2. The active solver still carries the route-aware `time_lgate_*` / `lgate_*`
   surfaces from that line, so future optimization should narrow cost inside
   this corridor instead of deleting or bypassing it.
3. The bundled report/results keep the residual order unchanged: the safe next
   primary axis is `zero-span eligibility and fastpath commit`, while
   `signature source load and materialize` and
   `layout signature compare and reuse gate core` stay secondary.
4. The bundled package remains `partial`, so its validator-clean rows are
   useful anchor evidence but do not replace fresh branch-local required-gate
   reruns.

## Locked rewrite constraints

1. Stay inside the current research family. A major rewrite or pivot must
   re-anchor `boj28350_resume/boj28350_branch_3_solver.cpp` to the bundled
   `progress40` line while preserving the literature-grade BC-tree / closed
   handle structure from source set A.
2. Preserve the cumulative `progress40` optimization corridor. The next rewrite
   should attack cost inside the existing layout-gate and zero-span fastpath
   corridor, not turn this branch into a different decomposition line.
3. Keep the residual order fixed unless fresh evidence disproves it. The safe
   next axis remains `zero-span eligibility and fastpath commit`; secondary
   axes remain `signature source load and materialize` and
   `layout signature compare and reuse gate core`.
4. Treat reproducibility as part of the rewrite contract. Any later rewrite or
   pivot still needs fresh same-working-tree branch-local evidence from
   `./lca_strong_gate.sh` and `./lca_boj3s_gate.sh`, with no manual artifact
   cleanup between reruns.

## Use

Any future planning or retry note that opens a major solver rewrite should cite
this summary together with `boj28350_resume/pre_rewrite_checkpoint.md` and
restate that both reviewed source sets are complete.
