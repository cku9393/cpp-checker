#include "test_harness.hpp"

#include <algorithm>
#include <deque>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

#include "reference_planner.hpp"
#include "state_dump.hpp"

using namespace std;

namespace {

vector<PlannerTraceEntry> g_lastPlannerTrace;

string effective_label(const string& fallback, const string& label) {
    if (!label.empty()) {
        return label;
    }
    return fallback;
}

string dump_path_suffix(const optional<filesystem::path>& path) {
    if (!path.has_value()) {
        return {};
    }
    return string("\ndump=") + path->string();
}

void assert_all_live_state_valid(const RawEngine& RE) {
    for (u32 sid = 0; sid < RE.skel.a.size(); ++sid) {
        if (!RE.skel.a[sid].alive) {
            continue;
        }
        assert_skeleton_wellformed(RE, sid);
    }
    for (u32 occ = 0; occ < RE.occ.a.size(); ++occ) {
        if (!RE.occ.a[occ].alive) {
            continue;
        }
        assert_occ_patch_consistent(RE, occ);
    }
}

string render_trace_with_level(const vector<PlannerTraceEntry>& trace, TraceLevel traceLevel, size_t prefixLength) {
    if (traceLevel == TraceLevel::NONE) {
        return {};
    }

    const size_t limit = (prefixLength == 0U ? trace.size() : min(prefixLength, trace.size()));
    if (traceLevel == TraceLevel::FULL) {
        vector<PlannerTraceEntry> prefix(trace.begin(), trace.begin() + limit);
        return describe_planner_trace(prefix);
    }

    ostringstream oss;
    oss << "trace_count=" << trace.size() << '\n';
    oss << "trace_prefix_count=" << limit << '\n';
    oss << "trace_prefix_hash=" << hash_planner_trace_prefix(trace, limit) << '\n';
    const size_t summaryLimit = min<size_t>(limit, 12U);
    for (size_t i = 0; i < summaryLimit; ++i) {
        oss << i << ": " << planner_primitive_name(trace[i].primitive)
            << " hash=" << trace[i].hash
            << " " << trace[i].summary << '\n';
    }
    return oss.str();
}

void save_trace_file(
    const TestOptions& options,
    const string& label,
    const vector<PlannerTraceEntry>& trace,
    TraceLevel traceLevel,
    size_t prefixLength
) {
    if (traceLevel == TraceLevel::NONE) {
        return;
    }
    const filesystem::path out = artifact_subdir(options, "traces") / (current_failure_stem() + "_" + label + ".log");
    ofstream ofs(out);
    if (!ofs) {
        throw runtime_error("failed to write planner trace: " + out.string());
    }
    ofs << render_trace_with_level(trace, traceLevel, prefixLength);
    enforce_artifact_retention(options);
}

void save_trace_force(const TestOptions& options, const string& label, const vector<PlannerTraceEntry>& trace) {
    save_trace_file(
        options,
        label,
        trace,
        options.dumpTrace ? TraceLevel::FULL : options.traceLevel,
        0U
    );
}

void save_trace_if_requested(const TestOptions& options, const string& label, const vector<PlannerTraceEntry>& trace) {
    if (options.keepOnlyFailures || !active_trace_enabled()) {
        return;
    }
    save_trace_file(options, label, trace, options.dumpTrace ? TraceLevel::FULL : options.traceLevel, 0U);
}

string describe_keep_occ_set(const unordered_set<OccID>& keepOcc) {
    vector<OccID> ordered(keepOcc.begin(), keepOcc.end());
    sort(ordered.begin(), ordered.end());
    ostringstream oss;
    for (size_t i = 0; i < ordered.size(); ++i) {
        if (i != 0) {
            oss << ',';
        }
        oss << ordered[i];
    }
    return oss.str();
}

void merge_coverage(PlannerCoverageSummary& dst, const PlannerCoverageSummary& src) {
    dst.splitReadyCount += src.splitReadyCount;
    dst.boundaryOnlyChildCount += src.boundaryOnlyChildCount;
    dst.joinCandidateCount += src.joinCandidateCount;
    dst.integrateCandidateCount += src.integrateCandidateCount;
    dst.actualSplitHits += src.actualSplitHits;
    dst.actualJoinHits += src.actualJoinHits;
    dst.actualIntegrateHits += src.actualIntegrateHits;
}

PlannerCoverageSummary coverage_from_initial_queue(const vector<UpdJob>* initialQueue) {
    PlannerCoverageSummary coverage;
    if (initialQueue == nullptr) {
        return coverage;
    }
    for (const UpdJob& job : *initialQueue) {
        if (job.kind == UpdJobKind::JOIN_PAIR) {
            ++coverage.joinCandidateCount;
        } else if (job.kind == UpdJobKind::INTEGRATE_CHILD) {
            ++coverage.integrateCandidateCount;
        }
    }
    return coverage;
}

PlannerCoverageSummary coverage_from_split_result(const SplitSeparationPairResult& result, const RawPlannerCtx& ctx) {
    PlannerCoverageSummary coverage;
    const int anchorIdx = choose_anchor_child(result, ctx);
    if (anchorIdx < 0) {
        return coverage;
    }
    for (int i = 0; i < static_cast<int>(result.child.size()); ++i) {
        if (i == anchorIdx) {
            continue;
        }
        const SplitChildInfo& child = result.child[static_cast<size_t>(i)];
        if (child.boundaryOnly) {
            ++coverage.boundaryOnlyChildCount;
            ++coverage.integrateCandidateCount;
        } else if (child_intersects_keep_occ(child, ctx)) {
            ++coverage.joinCandidateCount;
        }
    }
    return coverage;
}

void run_actual_planner_impl(
    RawEngine& RE,
    const RawPlannerCtx& ctx,
    RawUpdateCtx& U,
    const RawUpdaterRunOptions& runOptions,
    const vector<UpdJob>* initialQueue,
    vector<PlannerTraceEntry>& trace,
    PlannerPhase* activePhase,
    PlannerCoverageSummary* coverage
) {
    deque<UpdJob> q;
    if (initialQueue != nullptr && !initialQueue->empty()) {
        q.insert(q.end(), initialQueue->begin(), initialQueue->end());
    } else {
        q.push_back(UpdJob{UpdJobKind::ENSURE_SOLE, ctx.targetOcc});
    }
    const RawUpdaterHooks hooks = make_basic_hooks_for_target(ctx);
    if (coverage != nullptr) {
        merge_coverage(*coverage, coverage_from_initial_queue(initialQueue));
    }

    size_t steps = 0;
    while (!q.empty()) {
        ++steps;
        if (runOptions.stepBudget != 0U && steps > runOptions.stepBudget) {
            throw runtime_error("raw updater step budget exceeded");
        }

        UpdJob job = move(q.front());
        q.pop_front();

        switch (job.kind) {
            case UpdJobKind::ENSURE_SOLE: {
                if (activePhase != nullptr) {
                    *activePhase = PlannerPhase::ENSURE_SOLE;
                }
                const RawOccRecord& O = RE.occ.get(job.occ);
                const RawSkeleton& S = RE.skel.get(O.hostSkel);

                if (S.hostedOcc.size() != 1U || S.hostedOcc[0] != job.occ) {
                    if (activePhase != nullptr) {
                        *activePhase = PlannerPhase::ISOLATE;
                    }
                    const IsolateVertexResult result = isolate_vertex(RE, O.hostSkel, job.occ, U);
                    ostringstream oss;
                    oss << "isolate occ=" << job.occ
                        << " occSkel=" << result.occSkel
                        << " residualSkel=" << result.residualSkel;
                    trace.push_back(PlannerTraceEntry{
                        PlannerPrimitiveKind::ISOLATE_VERTEX,
                        oss.str(),
                        stable_hash_text(oss.str()),
                    });
                    q.push_front(UpdJob{UpdJobKind::ENSURE_SOLE, job.occ});
                    break;
                }

                const optional<pair<Vertex, Vertex>> sep = discover_split_pair_from_support(RE, job.occ);
                if (!sep.has_value()) {
                    break;
                }
                if (coverage != nullptr) {
                    ++coverage->splitReadyCount;
                }

                if (activePhase != nullptr) {
                    *activePhase = PlannerPhase::SPLIT;
                }
                const SplitSeparationPairResult result =
                    split_checked(RE, O.hostSkel, sep->first, sep->second, U, "planner_split");
                if (coverage != nullptr) {
                    ++coverage->actualSplitHits;
                    merge_coverage(*coverage, coverage_from_split_result(result, ctx));
                }
                ostringstream oss;
                oss << "split occ=" << job.occ
                    << " sep=" << sep->first << "," << sep->second
                    << " sig=" << describe_signature(capture_split_result_signature(RE, result));
                trace.push_back(PlannerTraceEntry{
                    PlannerPrimitiveKind::SPLIT,
                    oss.str(),
                    stable_hash_text(oss.str()),
                });
                hooks.afterSplit(result, q);
                break;
            }

            case UpdJobKind::JOIN_PAIR: {
                if (activePhase != nullptr) {
                    *activePhase = PlannerPhase::JOIN;
                }
                const JoinSeparationPairResult result = join_checked(
                    RE,
                    job.leftSid,
                    job.rightSid,
                    job.aOrig,
                    job.bOrig,
                    U,
                    "planner_join"
                );
                ostringstream oss;
                oss << "join left=" << job.leftSid
                    << " right=" << job.rightSid
                    << " sig=" << describe_signature(capture_join_result_signature(RE, result));
                trace.push_back(PlannerTraceEntry{
                    PlannerPrimitiveKind::JOIN,
                    oss.str(),
                    stable_hash_text(oss.str()),
                });
                if (coverage != nullptr) {
                    ++coverage->actualJoinHits;
                }
                hooks.afterJoin(result, q);
                break;
            }

            case UpdJobKind::INTEGRATE_CHILD: {
                if (activePhase != nullptr) {
                    *activePhase = PlannerPhase::INTEGRATE;
                }
                const IntegrateResult result = integrate_checked(
                    RE,
                    job.parentSid,
                    job.childSid,
                    job.bm,
                    U,
                    "planner_integrate"
                );
                ostringstream oss;
                oss << "integrate parent=" << job.parentSid
                    << " child=" << job.childSid
                    << " sig=" << describe_signature(capture_integrate_result_signature(RE, result));
                trace.push_back(PlannerTraceEntry{
                    PlannerPrimitiveKind::INTEGRATE,
                    oss.str(),
                    stable_hash_text(oss.str()),
                });
                if (coverage != nullptr) {
                    ++coverage->actualIntegrateHits;
                }
                hooks.afterIntegrate(result, q);
                break;
            }
        }
    }
    if (activePhase != nullptr) {
        *activePhase = PlannerPhase::NONE;
    }
}

} // namespace

const vector<PlannerTraceEntry>& last_planner_trace() {
    return g_lastPlannerTrace;
}

void execute_planner_capture(
    RawEngine& RE,
    const RawPlannerCtx& ctx,
    RawUpdateCtx& U,
    const RawUpdaterRunOptions& runOptions,
    const vector<UpdJob>* initialQueue,
    vector<PlannerTraceEntry>& trace,
    PlannerPhase* activePhase
) {
    run_actual_planner_impl(RE, ctx, U, runOptions, initialQueue, trace, activePhase, nullptr);
}

PlannerExecutionResult run_planner_checked_capture(
    RawEngine& RE,
    const RawPlannerCtx& ctx,
    RawUpdateCtx& U,
    const RawUpdaterRunOptions& runOptions,
    const vector<UpdJob>* initialQueue,
    const string& label
) {
    PlannerStateDump plannerDump;
    plannerDump.engine = RE;
    plannerDump.caseName = current_failure_context().caseName;
    plannerDump.seed = current_failure_context().seed;
    plannerDump.iter = current_failure_context().iter;
    plannerDump.targetOcc = ctx.targetOcc;
    plannerDump.keepOcc.assign(ctx.keepOcc.begin(), ctx.keepOcc.end());
    sort(plannerDump.keepOcc.begin(), plannerDump.keepOcc.end());
    plannerDump.stepBudget = runOptions.stepBudget;
    plannerDump.tracePrefixLength = 0U;
    if (initialQueue != nullptr) {
        plannerDump.initialQueue = *initialQueue;
    }
    plannerDump.traceLevel = active_test_options().dumpTrace ? TraceLevel::FULL : active_test_options().traceLevel;

    PlannerCallDumpGuard dumpGuard(active_test_options(), plannerDump, effective_label("planner", label));
    const RawEngine before = RE;
    PlannerExecutionResult result;
    run_actual_planner_impl(RE, ctx, U, runOptions, initialQueue, result.trace, nullptr, &result.coverage);
    g_lastPlannerTrace = result.trace;
    assert_all_live_state_valid(RE);
    save_trace_if_requested(active_test_options(), effective_label("planner", label), result.trace);

    if (active_planner_oracle_enabled()) {
        string failure;
        if (!check_planner_oracle(before, RE, ctx, runOptions, result.trace, initialQueue, &failure)) {
            if (active_trace_enabled()) {
                save_trace_force(active_test_options(), effective_label("planner", label), result.trace);
            }
            const FailureContextSnapshot ctxSnap = current_failure_context();
            ostringstream oss;
            oss << "planner_oracle_mismatch"
                << "\ncase=" << ctxSnap.caseName
                << "\nseed=" << (ctxSnap.seed.has_value() ? to_string(*ctxSnap.seed) : string("none"))
                << "\niter=" << ctxSnap.iter
                << "\ntargetOcc=" << ctx.targetOcc
                << "\nkeepOcc=" << describe_keep_occ_set(ctx.keepOcc)
                << "\nstepBudget=" << runOptions.stepBudget
                << '\n' << failure;
            throw runtime_error(oss.str() + dump_path_suffix(dumpGuard.path()));
        }
    }
    dumpGuard.mark_success();
    return result;
}

void run_planner_checked(
    RawEngine& RE,
    const RawPlannerCtx& ctx,
    RawUpdateCtx& U,
    const RawUpdaterRunOptions& runOptions,
    const string& label
) {
    (void)run_planner_checked_capture(RE, ctx, U, runOptions, nullptr, label);
}
