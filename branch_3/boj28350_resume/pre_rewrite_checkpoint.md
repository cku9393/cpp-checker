# Branch_3 Pre-Rewrite Checkpoint

This standalone note is the branch-local decision checkpoint that must be refreshed before any major solver rewrite or pivot begins in `branch_3`.

## Decision Gate

Do not open a major solver rewrite or pivot until both review completions below are explicitly recorded together in this note or in a retry/planning note that cites it.

1. `reviewed source set A`: branch-local `branch_3` notes and working-set materials
2. `reviewed source set B`: bundled `progress40` authoritative source/report/results set

If either review is missing, stale, or not written down, keep the solver on hold and refresh the review state first.

## Required Pre-Rewrite Citation

Before any major solver rewrite or pivot starts in `branch_3`, the opening note
for that work must explicitly cite both reviewed source sets together:

1. `branch_3 notes reviewed = COMPLETE` after re-reading the branch-local notes
   and working materials that define the allowed solver family and current
   branch drift
2. `bundled progress40 materials reviewed = COMPLETE` after re-reading the
   bundled `progress40` source, report, and merged-results package that defines
   the intended optimization corridor

If a rewrite/pivot note does not record both lines above together before solver
edits begin, that note is not a valid pre-rewrite checkpoint.

## Review Completion Refresh

Date: `2026-04-12`

### 2026-04-12 session pre-rewrite citation refresh

This `2026-04-12` entry is the current-session planning-note citation that
must be referenced before any solver rewrite or pivot is opened in `branch_3`.
It re-cites the paired review checkpoint instead of allowing a rewrite/pivot
note to start from memory or from an uncited assumption.

`2026-04-12` session citation status:

1. `branch_3 notes reviewed = COMPLETE` by re-opening the current branch-local
   planning/checkpoint notes
   (`boj28350_resume/current_state_summary.md`,
   `boj28350_resume/next_session_briefing.md`, and
   `boj28350_resume/pre_rewrite_checkpoint.md`) before any rewrite/pivot
   decision is allowed to proceed.
2. `bundled progress40 materials reviewed = COMPLETE` by re-opening the
   bundled progress40 anchor source
   (`boj28350_bundle_archive/boj28350_literature_progress40_layout_signature_reuse_gate.cpp`)
   and by explicitly carrying forward the paired authoritative package already
   locked in this checkpoint and in
   `boj28350_resume/current_state_summary.md`
   (`boj28350_bundle_archive/boj28350_progress40_layout_signature_reuse_gate_report.md`,
   `boj28350_bundle_archive/boj28350_progress40_results_merged.json`).
3. `rewrite/pivot permission`: HOLD unless the later rewrite/pivot note cites
   this `2026-04-12` refresh together with the standing paired-review gate
   below.

The purpose of this refresh is narrow and explicit: it records, in the
branch-local research log, that the session re-cited both the `branch_3`
notes review and the bundled `progress40` materials review before any
solver-side rewrite or research-direction pivot is opened.

### 2026-04-11 refresh evidence

This refresh supersedes the older `2026-04-10` status line as the active
branch-local pre-rewrite checkpoint. It re-confirms, in the current working
tree, which branch-local notes and bundled `progress40` materials were
re-read and what constraints they still impose on the next solver direction.

`2026-04-11` review completion status:

1. `reviewed source set A`: COMPLETE after re-reading the branch-local review
   gate and resume notes
   (`README.md`, `boj28350_resume/README.md`,
   `boj28350_resume/current_state_summary.md`,
   `boj28350_resume/next_session_briefing.md`,
   `boj28350_resume/pre_rewrite_checkpoint.md`,
   `boj28350_resume/pre_rewrite_synthesis_note.md`,
   `boj28350_resume/progress40_derived_reference.md`,
   `boj28350_complete_master_document_partA_raw.md`,
   `boj28350_integrated_technical_history.md`,
   `boj28350_literature_progress7_bcdecomp_report.md`,
   `literature_grade_proof_package.md`, and
   `boj28350_resume/boj28350_branch_3_solver.cpp`).
2. `reviewed source set B`: COMPLETE after re-reading the bundled
   `progress40` authoritative materials
   (`boj28350_bundle_archive/boj28350_literature_progress40_layout_signature_reuse_gate.cpp`,
   `boj28350_bundle_archive/boj28350_progress40_layout_signature_reuse_gate_report.md`,
   `boj28350_bundle_archive/boj28350_progress40_results_merged.json`).

Locked constraints from the `2026-04-11` refresh:

1. `README.md`, `boj28350_resume/README.md`, and
   `boj28350_resume/next_session_briefing.md` still treat the paired review as
   a hard gate before any major solver rewrite or pivot. The active solver is
   still supposed to be a progress40-derived resume line, so the next solver
   direction must reduce branch drift instead of widening it.
2. `boj28350_literature_progress7_bcdecomp_report.md`,
   `boj28350_integrated_technical_history.md`,
   `literature_grade_proof_package.md`, and
   `boj28350_complete_master_document_partA_raw.md` still lock the permitted
   solver family to the literature-grade BC-tree flavored explicit child
   lattice with `closeByBCPath(...)`,
   `buildClosedHandleFromWitness(...)`, exact strict-child testing, and no
   release-path owner exact rebuild fallback. A new algorithm family or a
   heuristic-only detour remains out of scope for `branch_3`.
3. `boj28350_resume/current_state_summary.md`,
   `boj28350_resume/progress40_derived_reference.md`, and the bundled
   `progress40` report/results still fix the optimization order:
   `zero-span eligibility and fastpath commit` remains the primary residual,
   while `signature source load and materialize` plus
   `layout signature compare and reuse gate core` remain secondary axes. The
   bundled package is still only `partial`, with dense `1024`
   release/repeat, `4096` representatives, and long-run terminal-row
   persistence still outside authoritative closure.
4. `boj28350_bundle_archive/boj28350_progress40_results_merged.json` still
   records
   `base_source=boj28350_literature_progress39_same_layout_zero_span_elision.cpp`
   and
   `output_source=boj28350_literature_progress40_layout_signature_reuse_gate.cpp`.
   That keeps the next solver direction inside the cumulative
   pack/normalize -> same-layout reuse -> layout-signature gate ->
   zero-span eligibility -> fastpath commit line, with the route-aware
   `time_lgate_*` / `lgate_*` attribution preserved rather than deleted.
5. The current retry-loop carry-over state
   (`.ouroboros/failure_analysis_state.json`) now pins
   `pinned_primary_axis = zero_span_fastpath`,
   `pinned_secondary_axis = null`,
   `next_probe_command = ./lca_smoke.sh`, and explicitly says to keep that
   progress40 axis parked until fresh same-worktree solver/runtime/profile
   evidence contradicts it. This checkpoint therefore keeps the next solver
   direction axis-focused instead of broadening into another rewrite family.
6. Reproducibility remains part of solver direction, not separate cleanup.
   Bundled `progress40` rows do not substitute for fresh branch-local gate
   evidence; same-worktree reruns of `./lca_strong_gate.sh` and
   `./lca_boj3s_gate.sh` are still required after solver work, and manual
   artifact cleanup between reruns remains disallowed.

### 2026-04-11 explicit hold point

This `2026-04-11` refresh is now the active Sub-AC 3 checkpoint for
`branch_3`.

1. `reviewed source set A`: COMPLETE on `2026-04-11`
2. `reviewed source set B`: COMPLETE on `2026-04-11`
3. `major solver rewrite/pivot permission`: HOLD until a later planning or
   retry note cites this `2026-04-11` checkpoint and repeats the locked
   progress40-derived solver-direction constraints above

### 2026-04-10 refresh evidence

This refresh records that the required branch-local notes review and bundled
`progress40` materials review were explicitly re-confirmed inside `branch_3`
before approving any future major solver rewrite or pivot.

`2026-04-10` review completion status:

1. `reviewed source set A`: COMPLETE after re-reading the branch-local notes
   set, including `boj28350_resume/README.md`,
   `boj28350_resume/current_state_summary.md`,
   `boj28350_resume/next_session_briefing.md`,
   `boj28350_resume/pre_rewrite_checkpoint.md`,
   `boj28350_resume/progress40_derived_reference.md`,
   `boj28350_complete_master_document_partA_raw.md`,
   `boj28350_integrated_technical_history.md`,
   `boj28350_literature_progress7_bcdecomp_report.md`,
   `literature_grade_proof_package.md`, and the active solver
   `boj28350_resume/boj28350_branch_3_solver.cpp`.
2. `reviewed source set B`: COMPLETE after re-reading the bundled
   `progress40` authoritative materials:
   `boj28350_bundle_archive/boj28350_literature_progress40_layout_signature_reuse_gate.cpp`,
   `boj28350_bundle_archive/boj28350_progress40_layout_signature_reuse_gate_report.md`,
   and `boj28350_bundle_archive/boj28350_progress40_results_merged.json`.

Locked takeaways from the `2026-04-10` refresh:

1. The branch-local notes still define `branch_3` as a progress40-derived
   resume line. `boj28350_resume/README.md` still says the active solver was
   copied from the latest `progress40` snapshot, so a major rewrite must
   re-anchor drift against that source family instead of authorizing a new
   algorithm family.
2. The literature-grade structure remains the non-negotiable guardrail:
   preserve the BC-tree flavored explicit child lattice and minimal
   closed-subtree handle path
   (`ensureLatticeChildren(...)`, `closeByBCPath(...)`,
   `buildClosedHandleFromWitness(...)`) rather than pivoting away from the
   proof-backed line.
3. The active solver still carries the bundled progress40 instrumentation and
   reuse surfaces (`time_lgate_*`, `lgate_*`,
   `materializeSupportMetadataFromCollector(...)`,
   `materializeSupportMetadataFromPieceState(...)`,
   `applyPieceNativeReuseForClass(...)`), so any later rewrite must stay inside
   that progress40-derived optimization corridor.
4. The bundled `progress40` report/results still mark the package as
   `status: partial`, with `zero-span eligibility and fastpath commit` as the
   largest residual and `signature source load and materialize` plus
   `layout signature compare and reuse gate core` as secondary axes. A rewrite
   approval that ignores this ordering would break the documented progress40
   research direction.

### 2026-04-10 explicit pre-rewrite hold point

This `2026-04-10` refresh is the active Sub-AC 3 checkpoint for `branch_3`.
It records, before any major solver rewrite or research pivot begins, that both
required source sets were reviewed again in the current branch-local workspace.

1. `reviewed source set A`: COMPLETE on `2026-04-10`
2. `reviewed source set B`: COMPLETE on `2026-04-10`
3. `major solver rewrite/pivot permission`: HOLD until a later planning or
   retry note explicitly cites this `2026-04-10` checkpoint and restates the
   progress40-derived rewrite constraints locked above.

### 2026-04-11 paired completed-review checkpoint

Before any major solver rewrite or research pivot begins in `branch_3`, this
checkpoint explicitly references both completed reviews together:

1. `completed review A`: the branch-local `branch_3` notes / proof /
   active-solver review for `reviewed source set A`, already recorded as
   COMPLETE in this checkpoint refresh
2. `completed review B`: the bundled `progress40` authoritative source /
   report / results review for `reviewed source set B`, already recorded as
   COMPLETE in this checkpoint refresh
3. `rewrite/pivot precondition`: no major solver rewrite or research pivot may
   begin unless both completed reviews above are cited together first

This paired-review entry is the branch-local pre-rewrite checkpoint record to
reuse in later planning or retry notes whenever a broad solver rewrite or
research-direction decision is being considered.

No major solver rewrite or pivot should open unless a planning or retry note
explicitly cites this `2026-04-10` refresh together with the existing rewrite
rules below.

### 2026-04-10 active hypotheses, prior failures, and solver constraints

This subsection is the branch-local pre-rewrite notes refresh for the current
`branch_3` session. It captures the active research hypotheses, the most
relevant carried-forward failures, and the solver constraints that must remain
visible before any later solver rewrite or retry plan is opened.

#### Active hypotheses

1. The progress40-derived residual order is still the main solver hypothesis:
   `zero-span eligibility and fastpath commit` stays the safest primary axis,
   while `signature source load and materialize` plus
   `layout signature compare and reuse gate core` remain secondary follow-up
   axes.
2. The latest retry-analysis state narrows the immediate reading of the current
   failure: attempt `023` is transport-disconnect noise, not fresh solver-axis
   evidence, so the solver-side primary axis stays parked at
   `zero_span_fastpath` and `secondary_axis = none` until a same-worktree rerun
   emits direct smoke or gate evidence again.
3. The older attempt-022 probe metadata
   (`watch_diff` / `retain_compaction`) is now historical context only. It can
   inform later profiling if fresh branch-local solver evidence reappears, but
   it does not outrank the current transport-shaped retry state.
4. The next branch-local signal should remain minimal first: rerun
   `./lca_smoke.sh` before widening back to heavier gates, and only reinterpret
   the solver axis after that rerun produces same-worktree artifacts.

#### Prior failures to carry forward

1. The newest formal retry failure (attempt `023`) did not produce fresh
   solver/gate artifacts. AC1 through AC6 all surfaced the same
   `stream disconnected before completion` payload, and the attempt guard
   therefore marks AC3 and AC5 as `missing_direct_gate_evidence` rather than as
   fresh solver-localized gate outcomes.
2. The latest fresh solver-side blocker remains the carried strong-gate timeout
   cluster already summarized elsewhere in this checkpoint: a prior
   `./lca_strong_gate.sh` run reported `71` timeout-only failures out of `900`,
   concentrated in `comb_rect_dense`, `caterpillar_rect_dense`,
   `multi_comb_rect`, and one `chain_unary` timeout, with
   `multi_comb_rect n=1024 L0/*` surviving near `1.856s` to `1.901s` while the
   matching `L1/*` rows tipped over the limit.
3. The branch-local `./lca_boj3s_gate.sh` evidence remains stale relative to
   the newest attempt, so no pre-rewrite planning note should treat AC3/AC5 as
   freshly closed or use carried-forward gate artifacts as same-worktree proof.

#### Solver constraints locked before rewrite

1. Preserve the literature-grade solver family. The BC-tree flavored explicit
   child lattice plus the `ensureLatticeChildren(...)`,
   `closeByBCPath(...)`, and `buildClosedHandleFromWitness(...)` path remain
   mandatory and must not be replaced with a different algorithm family or a
   heuristic-only branch-local shortcut.
2. Keep `boj28350_resume/boj28350_branch_3_solver.cpp` anchored to the
   progress40-derived line. The active solver still originates from the latest
   bundled `progress40` snapshot, so future changes should reduce branch-local
   drift rather than widen it, while preserving the route-aware `time_lgate_*`
   / `lgate_*` attribution and the current support-materialization /
   layout-gate surfaces.
3. Preserve the branch-local reproducibility contract: build only with
   `./build.sh`; treat `./lca_smoke.sh`, `./lca_strong_gate.sh`, and
   `./lca_boj3s_gate.sh` as the only acceptance wrappers; keep outputs under
   `branch_3/artifacts/...`; and do not weaken wrapper meaning just to chase a
   pass.
4. Reproducibility is part of solver work, not post-hoc cleanup. AC3 through
   AC6 still require fresh same-worktree reruns, and reruns must not depend on
   manual artifact cleanup between attempts.
5. If the next session resumes as a retry instead of a rewrite, reread the
   retry-loop notes first and obey the current pinned state:
   `pinned_primary_axis = zero_span_fastpath`,
   `pinned_secondary_axis = none`,
   `next_probe_command = ./lca_smoke.sh`.

## Current Checkpoint Status

This is the explicit pre-rewrite status line for `branch_3`.

- `reviewed source set A`: COMPLETE on `2026-04-11`
- `reviewed source set B`: COMPLETE on `2026-04-11`
- `major solver rewrite/pivot permission`: HOLD until an active planning or retry note cites this checkpoint and repeats that both source reviews are complete together

If a future session cannot honestly restate all three lines above, major solver changes do not proceed.

### 2026-04-10 Sub-AC 3 preservation note

This subsection preserves the active pre-rewrite checkpoint in a compact,
workflow-facing form so a later session can verify, before any major solver
rewrite or research-direction pivot begins, that both required source-set
reviews were completed together in the current `branch_3` workspace.

Preserved review scope:

1. `reviewed source set A`: branch-local `branch_3` notes, proof/history
   package, and active solver working set
   (`boj28350_resume/README.md`,
   `boj28350_resume/current_state_summary.md`,
   `boj28350_resume/next_session_briefing.md`,
   `boj28350_resume/pre_rewrite_checkpoint.md`,
   `boj28350_resume/progress40_derived_reference.md`,
   `boj28350_complete_master_document_partA_raw.md`,
   `boj28350_integrated_technical_history.md`,
   `boj28350_literature_progress7_bcdecomp_report.md`,
   `literature_grade_proof_package.md`,
   `boj28350_resume/boj28350_branch_3_solver.cpp`).
2. `reviewed source set B`: bundled `progress40` authoritative source/report /
   results set
   (`boj28350_bundle_archive/boj28350_literature_progress40_layout_signature_reuse_gate.cpp`,
   `boj28350_bundle_archive/boj28350_progress40_layout_signature_reuse_gate_report.md`,
   `boj28350_bundle_archive/boj28350_progress40_results_merged.json`).

Preserved go/no-go rule:

1. `source set A reviewed = COMPLETE`
2. `source set B reviewed = COMPLETE`
3. `major rewrite or pivot = HOLD` until a later planning or retry note cites
   this checkpoint and restates the locked progress40-derived constraints
4. `solver-family guardrail = ACTIVE`: keep the branch anchored to the
   literature-grade BC-tree flavored child-lattice line and the bundled
   `progress40` residual ordering rather than opening a new algorithm family
   or heuristic-only rewrite

This preservation note is intentionally redundant with the longer refresh
sections below so the pre-rewrite checkpoint can be audited quickly without
reconstructing the full history.

### Explicit pre-rewrite confirmation

The original `2026-03-28` refresh is preserved below as the first branch-local pre-rewrite checkpoint record for `branch_3`.
Before any major solver rewrite or pivot begins, both of the required review completions were re-read and explicitly confirmed in the notes:

1. `reviewed source set A`: branch-local `branch_3` notes and working-set materials
2. `reviewed source set B`: bundled `progress40` authoritative source/report/results set

No major solver rewrite or pivot should open unless this checkpoint, or a later note that cites it, confirms both review completions first.

### 2026-03-29 Sub-AC 3 source-set checkpoint refresh

This dated refresh is the current pre-rewrite planning checkpoint for `branch_3`.
It exists to record what the two required source sets still imply before any solver-side rewrite or pivot is opened.

`2026-03-29` review completion status:

1. `reviewed source set A`: COMPLETE after re-reading the branch-local notes, proof/history package, and active solver source.
2. `reviewed source set B`: COMPLETE after re-reading the bundled `progress40` authoritative source, report, and merged results.

#### Source set A takeaways locked again on 2026-03-29

1. The proof/history package still fixes the allowed structural line: BC-tree flavored explicit child lattice semantics, `ensureLatticeChildren(...)`, `closeByBCPath(...)`, and `buildClosedHandleFromWitness(...)` remain the literature-grade anchor for any further solver work.
2. The active solver still visibly carries those anchors in code, so the next rewrite must preserve them instead of replacing them with a different algorithm family or a heuristic-only branch-local shortcut.
3. The same branch-local working-set review still says the active solver has drifted away from a clean bundled `progress40` baseline, so using the current file as the only authority would widen branch drift rather than reduce it.

#### Source set B takeaways locked again on 2026-03-29

1. Bundled `progress40` still defines a cumulative layout-signature reuse-gate round, not permission to pivot to a different family.
2. The authoritative residual ordering is unchanged: `zero-span eligibility and fastpath commit` remains the largest residual (`49.9983%`), while `signature source load and materialize` (`24.9643%`) and `layout signature compare and reuse gate core` (`25.0339%`) stay secondary follow-up axes.
3. The bundled package is still only `partial` authority beyond the already closed ranges, with dense `1024` release/repeat, `4096` representatives, and long-run terminal-row persistence still requiring fresh branch-local evidence.

#### Planned rewrite or pivot implied by both source sets

1. The next solver rewrite must re-anchor `boj28350_resume/boj28350_branch_3_solver.cpp` against the bundled `progress40` line instead of extending the current separator-decomposition drift.
2. That rewrite must preserve the literature-grade decomposition semantics already evidenced in the active solver, especially the explicit child-lattice and minimal closed-subtree handle path.
3. The first solver-side attack axis stays inside the bundled `progress40` residual order: attack `zero-span eligibility and fastpath commit` first, then widen only if needed to `signature source load and materialize` and `layout signature compare and reuse gate core`.
4. Reproducibility remains part of the same rewrite contract because source set B is still partial: any planned pivot must treat dense `1024` repeat, `4096` representatives, and long-run terminal-row persistence as evidence requirements rather than as out-of-band cleanup.

This `2026-03-29` entry is therefore the active pre-rewrite checkpoint note for Sub-AC 3: both source sets were re-read, their key takeaways were restated, and the planned solver rewrite/pivot is explicitly constrained by those findings.

### Reviewed Source Set A: branch_3 notes and working set

Reviewed files:

- `boj28350_resume/README.md`
- `boj28350_resume/current_state_summary.md`
- `boj28350_resume/next_session_briefing.md`
- `boj28350_complete_master_document_partA_raw.md`
- `boj28350_integrated_technical_history.md`
- `boj28350_literature_progress7_bcdecomp_report.md`
- `literature_grade_proof_package.md`
- `boj28350_resume/boj28350_branch_3_solver.cpp`

Locked conclusions from the branch-local review:

1. The literature-grade anchor is still the `progress7` / proof-package line: BC-tree flavored decomposition, explicit child lattice, minimal closed-subtree handle semantics, and no release-path exact rebuild fallback.
2. The active `branch_3` solver is no longer a clean bundled `progress40` snapshot; it carries a substantial branch-local delta and must be compared back to the bundled source before it is used as a rewrite baseline.
3. Any major solver rewrite must preserve the literature/proof anchors above and re-anchor the active solver against the bundled `progress40` direction instead of widening branch-local drift or pivoting to a different algorithm family.

### Reviewed Source Set B: bundled progress40 authoritative set

Reviewed files:

- `boj28350_bundle_archive/boj28350_literature_progress40_layout_signature_reuse_gate.cpp`
- `boj28350_bundle_archive/boj28350_progress40_layout_signature_reuse_gate_report.md`
- `boj28350_bundle_archive/boj28350_progress40_results_merged.json`

Locked conclusions from the bundled progress40 review:

1. Progress40 is a layout-signature reuse-gate round layered on top of the prior line, not permission to jump to a different algorithm family.
2. The safest next optimization axis remains `zero-span eligibility and fastpath commit`.
3. Secondary axes stay `signature source load and materialize` plus `layout signature compare and reuse gate core`.
4. The package is still only partial-authoritative beyond the 512 matrix and `both_on_multi_1024_release`; dense `1024` repeat, `4096` representatives, and long-run terminal-row persistence still require branch-local evidence.

### 2026-03-28 Sub-AC 3 checkpoint refresh

This refresh is the current pre-rewrite evidence entry for `branch_3`.
Before any major solver rewrite or pivot opens, both required review completions were re-confirmed:

1. `reviewed source set A`: branch-local `branch_3` notes, proof/history materials, and the active solver source
2. `reviewed source set B`: bundled `progress40` authoritative source/report/results set

Rewrite-guiding insights locked by this refresh:

1. The active solver is not a byte-identical `progress40` snapshot anymore. A direct local diff against the bundled `progress40` source shows a substantial branch-local delta (`701` insertions, `26` deletions), so any major rewrite must treat `boj28350_resume/boj28350_branch_3_solver.cpp` as a mixed-lineage derivative rather than as the authoritative baseline by itself.
2. That same active solver still carries the literature/progress line that must be preserved: the BC-tree flavored explicit child lattice and minimal closed-subtree handle path are still present via `closeByBCPath(...)`, `buildClosedHandleFromWitness(...)`, and `ensureLatticeChildren(...)`, while the progress40 layout-gate counters remain present via the `time_lgate_*` / `lgate_*` instrumentation. A rewrite that drops these anchors would be a research-direction break.
3. The proof/history package remains the governing structure, not optional background. Any major rewrite must preserve the literature-grade invariants recorded across the proof package and progress7 history: explicit BC-tree child lattice semantics, unique minimal closed-subtree lifting from witnesses, and no release-path exact rebuild fallback.
4. The bundled `progress40` report still fixes the optimization order. The first solver-side attack axis is `zero-span eligibility and fastpath commit`; only after that should work widen to `signature source load and materialize` and `layout signature compare and reuse gate core`.
5. The bundled `progress40` results are still `status: partial`, with `progress38_authoritative_close_not_completed`, `both_on_dense_1024_release`, `both_on_dense_1024_release_repeat`, `both_on_dense_4096_release`, and `both_on_multi_4096_release` still missing. Therefore branch-local reproducibility hygiene and long-run terminal-row persistence remain part of the rewrite contract, not a separate cleanup task.
6. The safe rewrite posture is therefore: re-anchor against the bundled `progress40` source/report/results, preserve the literature-grade decomposition semantics already evidenced in the active solver, and reject any broader algorithm-family pivot unless a later note explicitly disproves the current progress40/proof-package line.

### 2026-03-28 retry-loop constraint and failure refresh

This refresh extends the pre-rewrite checkpoint with the latest retry-loop evidence before any further major solver rewrite begins.

Retry-loop evidence re-read for this refresh:

- `artifacts/lca_tree_stress_v5/retry_loop/latest_failure_report.md`
- `artifacts/lca_tree_stress_v5/retry_loop/latest_failure_breakdown.md`
- `artifacts/lca_tree_stress_v5/retry_loop/latest_analysis_session.md`
- `artifacts/lca_tree_stress_v5/retry_loop/latest_next_probe_result.md`
- `artifacts/lca_tree_stress_v5/retry_loop/latest_attempt_guard.md`
- `artifacts/lca_tree_stress_v5/retry_loop/latest_git_repo_health.md`
- `.ouroboros/failure_analysis_state.json`
- `.ouroboros/failure_analysis_iteration.md`
- `.ouroboros/failure_analysis_playbook.md`
- `artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_probe.latest_failure/certify_summary.md`
- `artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_probe.latest_failure/certify_rows.csv`

Locked solver constraints from the combined note + artifact review:

1. The literature-grade invariant set is still non-negotiable: BC-tree flavored explicit child lattice, `closeByBCPath(...)`, `buildClosedHandleFromWitness(...)` as the closed-subtree lift, and no release-path exact rebuild fallback remain the structure to preserve.
2. The active solver is a mixed-lineage derivative, not the authoritative baseline by itself. A current direct diff against bundled `progress40` shows `701` insertions and `26` deletions in `boj28350_resume/boj28350_branch_3_solver.cpp` relative to `boj28350_literature_progress40_layout_signature_reuse_gate.cpp`.
3. That same solver still visibly contains the progress40/per-proof anchors that the next rewrite must preserve: the `time_lgate_*` / `lgate_*` instrumentation is still present, and the decomposition path still runs through `ensureLatticeChildren(...)`, `closeByBCPath(...)`, and `buildClosedHandleFromWitness(...)`.
4. Formal validation constraints remain branch-local and unchanged: build only with `./build.sh`, treat `./lca_smoke.sh`, `./lca_strong_gate.sh`, and `./lca_boj3s_gate.sh` as the only acceptance wrappers, keep all outputs under `branch_3/artifacts/...`, and do not weaken the wrapper meaning just to chase a pass.
5. Reproducibility is still part of the solver contract, not post-hoc cleanup. Bundled `progress40` and branch-local retry notes still agree that dense `1024` repeat, `4096` representatives, and long-run terminal-row persistence are not yet fully authoritative.

Carried hypotheses that the next rewrite/retry must cite explicitly:

1. The branch-wide authoritative residual ordering is unchanged. Bundled `progress40` source/report/results still keep `zero-span eligibility and fastpath commit` as the largest residual (`49.9983%`), ahead of `layout signature compare and reuse gate core` (`25.0339%`) and `signature source load and materialize` (`24.9643%`).
2. The latest retry-loop state nevertheless pins the immediate AC3 retry to `pinned_primary_axis = state_materialization`, `pinned_secondary_axis = retain_compaction`, with `next_probe_command = LCA_STAGE_FILTER=correctness_fuzz ./lca_strong_gate.sh`. That pin must be obeyed for the next retry plan unless fresh evidence disproves it.
3. That AC3 pin is not permission to abandon the progress40 family or widen into a different algorithm family. The safe reading is narrower: keep the major rewrite inside the progress40 zero-span/layout-gate line, but use the current AC3 retry to inspect the materialization-sensitive subcluster first before broadening.
4. The most concrete solver-side surfaces named by the current evidence are the layout-gate split at `time_lgate_sig_source_load_ns`, `time_lgate_sig_materialize_ns`, `time_lgate_zero_span_eligibility_gate_ns`, and `time_lgate_fastpath_commit_core_ns`, the support-materialization corridor around `materializeSupportMetadataFromCollector(...)` / `materializeSupportMetadataFromPieceState(...)`, and the route chooser in `applyPieceNativeReuseForClass(...)`.

Prior failure observations now locked into the notes:

1. The latest formal blocker is still AC3: `./lca_strong_gate.sh` failed fresh-in-attempt with `correctness_fuzz: 71 failing cases` out of `900`, all as timeouts and `0` re/wa.
2. The fresh timeout mass is concentrated in the hard families, not random noise: `comb_rect_dense` timed out at `n=1024` in all four `L/Q` quadrants and at `n=512` for `L=1`; `caterpillar_rect_dense` shows the same shape; `multi_comb_rect` timed out at `n=1024` for `L=1` in both `Q` branches; `chain_unary` contributes one `n=512 L=1 Q=0` timeout.
3. The near-pass frontier is also concrete: the slowest successful rows are `multi_comb_rect n=1024 L0/*` at `1.856s` to `1.901s`, so the next solver-side narrowing should explain why the matching `L1/*` rows fall over the limit while `L0/*` barely survives.
4. The latest heavy next probe did real work before timing out. `LCA_STAGE_FILTER=correctness_fuzz ./lca_strong_gate.sh` reached wrapper heartbeats with `62` completed cases at `25s` and `70` completed cases at `50s` before timing out at `180s`, so this is not explained by a pure build-only, lock-only, or zero-progress quick-fail story.
5. `latest_attempt_guard.md` still rejects any nominal gate success as `missing_direct_gate_evidence`, and `latest_git_repo_health.md` still shows degraded git inspection (`git status` / `git fsck` timeout). Those are trust/forensics guards only; they do not outweigh the fresh current-attempt strong-gate failure artifact.
6. The carried-forward `boj3s_gate` evidence is still failing, but it is stale relative to the latest attempt. The current fresh blocker is AC3 strong-gate timeout behavior, so the next major rewrite/retry must answer that corridor first rather than pretending final acceptance evidence got refreshed.

## Rewrite Rule

Before any future major solver rewrite or pivot begins, cite this checkpoint together with `boj28350_resume/next_session_briefing.md` section 6 and confirm that:

1. both reviewed source sets were re-read,
2. the next move stays on the progress40-derived research line, and
3. the rewrite targets progress40 residuals before considering any broader algorithm-family change.
