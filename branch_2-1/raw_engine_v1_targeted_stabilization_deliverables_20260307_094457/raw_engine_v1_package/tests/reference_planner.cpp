#include "reference_planner.hpp"

#include <algorithm>
#include <deque>
#include <sstream>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

using namespace std;

namespace {

u64 hash_text(const string& text) {
    u64 hash = 1469598103934665603ULL;
    for (unsigned char ch : text) {
        hash ^= static_cast<u64>(ch);
        hash *= 1099511628211ULL;
    }
    return hash;
}

string hex_hash(u64 value) {
    ostringstream oss;
    oss << hex << value;
    return oss.str();
}

bool planner_stop_condition_satisfied(const RawEngine& RE, OccID targetOcc) {
    const RawOccRecord& O = RE.occ.get(targetOcc);
    const RawSkeleton& S = RE.skel.get(O.hostSkel);
    if (S.hostedOcc.size() != 1U || S.hostedOcc[0] != targetOcc) {
        return false;
    }
    if (discover_split_pair_from_support(RE, targetOcc).has_value()) {
        return false;
    }
    assert_occ_patch_consistent(RE, targetOcc);
    return true;
}

PlannerTraceEntry make_trace_entry(PlannerPrimitiveKind primitive, const string& summary) {
    PlannerTraceEntry entry;
    entry.primitive = primitive;
    entry.summary = summary;
    entry.hash = hex_hash(hash_text(summary));
    return entry;
}

PlannerResultSignature capture_planner_signature_impl(
    const RawEngine& RE,
    OccID targetOcc,
    const vector<PlannerTraceEntry>& trace
) {
    PlannerResultSignature sig;
    sig.finalState = capture_engine_state_signature(RE);
    sig.stopConditionSatisfied = planner_stop_condition_satisfied(RE, targetOcc);
    sig.targetPrepare = capture_isolate_signature(
        reference_prepare_isolate_neighborhood(RE, RE.occ.get(targetOcc).hostSkel, targetOcc)
    );
    sig.trace = trace;
    return sig;
}

void run_reference_planner_impl(
    RawEngine& RE,
    const RawPlannerCtx& ctx,
    const RawUpdaterRunOptions& runOptions,
    bool captureTrace,
    vector<PlannerTraceEntry>& trace,
    const vector<UpdJob>* initialQueue
) {
    deque<UpdJob> q;
    if (initialQueue != nullptr && !initialQueue->empty()) {
        q.insert(q.end(), initialQueue->begin(), initialQueue->end());
    } else {
        q.push_back(UpdJob{UpdJobKind::ENSURE_SOLE, ctx.targetOcc});
    }

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
                    const IsolateVertexResult result = reference_isolate_vertex(RE, O.hostSkel, job.occ);
                    if (captureTrace) {
                        ostringstream oss;
                        oss << "isolate occ=" << job.occ
                            << " occSkel=" << result.occSkel
                            << " residualSkel=" << result.residualSkel;
                        trace.push_back(make_trace_entry(PlannerPrimitiveKind::ISOLATE_VERTEX, oss.str()));
                    }
                    q.push_front(UpdJob{UpdJobKind::ENSURE_SOLE, job.occ});
                    break;
                }

                const optional<pair<Vertex, Vertex>> sep = discover_split_pair_from_support(RE, job.occ);
                if (!sep.has_value()) {
                    break;
                }

                const SplitSeparationPairResult result =
                    reference_split_separation_pair(RE, O.hostSkel, sep->first, sep->second);
                if (captureTrace) {
                    ostringstream oss;
                    oss << "split occ=" << job.occ
                        << " sep=" << sep->first << "," << sep->second
                        << " sig=" << describe_signature(capture_split_result_signature(RE, result));
                    trace.push_back(make_trace_entry(PlannerPrimitiveKind::SPLIT, oss.str()));
                }
                hooks.afterSplit(result, q);
                break;
            }

            case UpdJobKind::JOIN_PAIR: {
                const JoinSeparationPairResult result = reference_join_separation_pair(
                    RE,
                    job.leftSid,
                    job.rightSid,
                    job.aOrig,
                    job.bOrig
                );
                if (captureTrace) {
                    ostringstream oss;
                    oss << "join left=" << job.leftSid
                        << " right=" << job.rightSid
                        << " sig=" << describe_signature(capture_join_result_signature(RE, result));
                    trace.push_back(make_trace_entry(PlannerPrimitiveKind::JOIN, oss.str()));
                }
                hooks.afterJoin(result, q);
                break;
            }

            case UpdJobKind::INTEGRATE_CHILD: {
                const IntegrateResult result = reference_integrate_skeleton(RE, job.parentSid, job.childSid, job.bm);
                if (captureTrace) {
                    ostringstream oss;
                    oss << "integrate parent=" << job.parentSid
                        << " child=" << job.childSid
                        << " sig=" << describe_signature(capture_integrate_result_signature(RE, result));
                    trace.push_back(make_trace_entry(PlannerPrimitiveKind::INTEGRATE, oss.str()));
                }
                hooks.afterIntegrate(result, q);
                break;
            }
        }
    }
}

} // namespace

PlannerResultSignature capture_planner_signature(
    const RawEngine& RE,
    OccID targetOcc,
    const vector<PlannerTraceEntry>& trace
) {
    return capture_planner_signature_impl(RE, targetOcc, trace);
}

bool PlannerTraceEntry::operator==(const PlannerTraceEntry& rhs) const {
    return tuple(primitive, summary, hash) == tuple(rhs.primitive, rhs.summary, rhs.hash);
}

bool PlannerResultSignature::operator==(const PlannerResultSignature& rhs) const {
    return finalState == rhs.finalState &&
           stopConditionSatisfied == rhs.stopConditionSatisfied &&
           targetPrepare == rhs.targetPrepare &&
           trace == rhs.trace;
}

const char* planner_primitive_name(PlannerPrimitiveKind primitive) {
    switch (primitive) {
        case PlannerPrimitiveKind::ISOLATE_VERTEX:
            return "isolate_vertex";
        case PlannerPrimitiveKind::SPLIT:
            return "split";
        case PlannerPrimitiveKind::JOIN:
            return "join";
        case PlannerPrimitiveKind::INTEGRATE:
            return "integrate";
    }
    return "unknown";
}

string describe_planner_trace(const vector<PlannerTraceEntry>& trace) {
    ostringstream oss;
    for (size_t i = 0; i < trace.size(); ++i) {
        oss << i << ": " << planner_primitive_name(trace[i].primitive)
            << " hash=" << trace[i].hash
            << " " << trace[i].summary << '\n';
    }
    return oss.str();
}

string describe_planner_signature(const PlannerResultSignature& signature) {
    ostringstream oss;
    oss << "stop_condition=" << (signature.stopConditionSatisfied ? 1 : 0) << '\n';
    oss << "target_prepare=\n" << describe_signature(signature.targetPrepare) << '\n';
    oss << "final_state=\n" << describe_signature(signature.finalState);
    if (!signature.trace.empty()) {
        oss << "\ntrace=\n" << describe_planner_trace(signature.trace);
    }
    return oss.str();
}

PlannerRunResult run_reference_planner(
    const RawEngine& before,
    const RawPlannerCtx& ctx,
    const RawUpdaterRunOptions& runOptions,
    bool captureTrace,
    const vector<UpdJob>* initialQueue
) {
    PlannerRunResult result;
    result.engine = before;
    run_reference_planner_impl(result.engine, ctx, runOptions, captureTrace, result.signature.trace, initialQueue);
    result.signature = capture_planner_signature(result.engine, ctx.targetOcc, result.signature.trace);
    return result;
}

PlannerOracleComparison compare_planner_oracle(
    const RawEngine& before,
    const RawEngine& after,
    const RawPlannerCtx& ctx,
    const RawUpdaterRunOptions& runOptions,
    const vector<PlannerTraceEntry>& actualTrace,
    const vector<UpdJob>* initialQueue
) {
    PlannerOracleComparison comparison;
    comparison.expected = run_reference_planner(before, ctx, runOptions, !actualTrace.empty(), initialQueue).signature;
    comparison.actual = capture_planner_signature(after, ctx.targetOcc, actualTrace);

    if (comparison.expected.finalState != comparison.actual.finalState) {
        comparison.matches = false;
        comparison.mismatchKind = FailureMismatchKind::FINAL_STATE;
        return comparison;
    }
    if (comparison.expected.stopConditionSatisfied != comparison.actual.stopConditionSatisfied) {
        comparison.matches = false;
        comparison.mismatchKind = FailureMismatchKind::STOP_CONDITION;
        return comparison;
    }
    if (!(comparison.expected.targetPrepare == comparison.actual.targetPrepare)) {
        comparison.matches = false;
        comparison.mismatchKind = FailureMismatchKind::TARGET_PREPARE;
        return comparison;
    }
    return comparison;
}

bool check_planner_oracle(
    const RawEngine& before,
    const RawEngine& after,
    const RawPlannerCtx& ctx,
    const RawUpdaterRunOptions& runOptions,
    const vector<PlannerTraceEntry>& actualTrace,
    const vector<UpdJob>* initialQueue,
    string* failure
) {
    const PlannerOracleComparison comparison =
        compare_planner_oracle(before, after, ctx, runOptions, actualTrace, initialQueue);
    if (comparison.matches) {
        return true;
    }

    if (comparison.mismatchKind == FailureMismatchKind::FINAL_STATE) {
        if (failure != nullptr) {
            *failure =
                string("planner oracle final state mismatch\nexpected:\n") +
                describe_signature(comparison.expected.finalState) +
                "\nactual:\n" +
                describe_signature(comparison.actual.finalState) +
                "\nreference_trace:\n" +
                describe_planner_trace(comparison.expected.trace) +
                "actual_trace:\n" +
                describe_planner_trace(actualTrace);
        }
        return false;
    }

    if (comparison.mismatchKind == FailureMismatchKind::STOP_CONDITION) {
        if (failure != nullptr) {
            *failure =
                string("planner oracle stop condition mismatch\nexpected=") +
                (comparison.expected.stopConditionSatisfied ? "1" : "0") +
                " actual=" +
                (comparison.actual.stopConditionSatisfied ? "1" : "0") +
                "\nreference_trace:\n" +
                describe_planner_trace(comparison.expected.trace) +
                "actual_trace:\n" +
                describe_planner_trace(actualTrace);
        }
        return false;
    }

    if (comparison.mismatchKind == FailureMismatchKind::TARGET_PREPARE) {
        if (failure != nullptr) {
            *failure =
                string("planner oracle target prepare mismatch\nexpected:\n") +
                describe_signature(comparison.expected.targetPrepare) +
                "\nactual:\n" +
                describe_signature(comparison.actual.targetPrepare) +
                "\nreference_trace:\n" +
                describe_planner_trace(comparison.expected.trace) +
                "actual_trace:\n" +
                describe_planner_trace(actualTrace);
        }
        return false;
    }
    return false;
}
