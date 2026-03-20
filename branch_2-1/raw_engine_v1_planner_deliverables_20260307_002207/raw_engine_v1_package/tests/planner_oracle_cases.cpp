#include "test_harness.hpp"

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

void save_trace_if_requested(const TestOptions& options, const string& label, const vector<PlannerTraceEntry>& trace) {
    if (!options.dumpTrace) {
        return;
    }
    const filesystem::path out = artifact_subdir(options, "traces") / (current_failure_stem() + "_" + label + ".log");
    ofstream ofs(out);
    if (!ofs) {
        throw runtime_error("failed to write planner trace: " + out.string());
    }
    ofs << describe_planner_trace(trace);
}

void save_trace_force(const TestOptions& options, const string& label, const vector<PlannerTraceEntry>& trace) {
    const filesystem::path out = artifact_subdir(options, "traces") / (current_failure_stem() + "_" + label + ".log");
    ofstream ofs(out);
    if (!ofs) {
        throw runtime_error("failed to write planner trace: " + out.string());
    }
    ofs << describe_planner_trace(trace);
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

void run_actual_planner_impl(
    RawEngine& RE,
    const RawPlannerCtx& ctx,
    RawUpdateCtx& U,
    const RawUpdaterRunOptions& runOptions,
    vector<PlannerTraceEntry>& trace
) {
    deque<UpdJob> q;
    q.push_back(UpdJob{UpdJobKind::ENSURE_SOLE, ctx.targetOcc});
    const RawUpdaterHooks hooks = make_basic_hooks_for_target(ctx);

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
                const RawOccRecord& O = RE.occ.get(job.occ);
                const RawSkeleton& S = RE.skel.get(O.hostSkel);

                if (S.hostedOcc.size() != 1U || S.hostedOcc[0] != job.occ) {
                    const IsolateVertexResult result = isolate_vertex(RE, O.hostSkel, job.occ, U);
                    ostringstream oss;
                    oss << "isolate occ=" << job.occ
                        << " occSkel=" << result.occSkel
                        << " residualSkel=" << result.residualSkel;
                    trace.push_back(PlannerTraceEntry{
                        PlannerPrimitiveKind::ISOLATE_VERTEX,
                        oss.str(),
                        make_failure_signature(FailureClass::NONE, oss.str()).hash,
                    });
                    q.push_front(UpdJob{UpdJobKind::ENSURE_SOLE, job.occ});
                    break;
                }

                const optional<pair<Vertex, Vertex>> sep = discover_split_pair_from_support(RE, job.occ);
                if (!sep.has_value()) {
                    break;
                }

                const SplitSeparationPairResult result =
                    split_checked(RE, O.hostSkel, sep->first, sep->second, U, "planner_split");
                ostringstream oss;
                oss << "split occ=" << job.occ
                    << " sep=" << sep->first << "," << sep->second
                    << " sig=" << describe_signature(capture_split_result_signature(RE, result));
                trace.push_back(PlannerTraceEntry{
                    PlannerPrimitiveKind::SPLIT,
                    oss.str(),
                    make_failure_signature(FailureClass::NONE, oss.str()).hash,
                });
                hooks.afterSplit(result, q);
                break;
            }

            case UpdJobKind::JOIN_PAIR: {
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
                    make_failure_signature(FailureClass::NONE, oss.str()).hash,
                });
                hooks.afterJoin(result, q);
                break;
            }

            case UpdJobKind::INTEGRATE_CHILD: {
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
                    make_failure_signature(FailureClass::NONE, oss.str()).hash,
                });
                hooks.afterIntegrate(result, q);
                break;
            }
        }
    }
}

} // namespace

void run_planner_checked(
    RawEngine& RE,
    const RawPlannerCtx& ctx,
    RawUpdateCtx& U,
    const RawUpdaterRunOptions& runOptions,
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
    plannerDump.dumpTrace = active_test_options().dumpTrace;

    PlannerCallDumpGuard dumpGuard(active_test_options(), plannerDump, effective_label("planner", label));
    const RawEngine before = RE;
    vector<PlannerTraceEntry> trace;
    run_actual_planner_impl(RE, ctx, U, runOptions, trace);
    assert_all_live_state_valid(RE);
    save_trace_if_requested(active_test_options(), effective_label("planner", label), trace);

    if (active_planner_oracle_enabled()) {
        string failure;
        if (!check_planner_oracle(before, RE, ctx, runOptions, trace, &failure)) {
            if (active_test_options().dumpOnFail && !active_test_options().dumpTrace) {
                save_trace_force(active_test_options(), effective_label("planner", label), trace);
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
}
