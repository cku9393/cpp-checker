#include "test_harness.hpp"

#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>

#include "reference_model.hpp"
#include "state_dump.hpp"

using namespace std;

namespace {

const TestOptions* g_activeOptions = nullptr;

string dump_path_suffix(const optional<filesystem::path>& path) {
    if (!path.has_value()) {
        return {};
    }
    return string("\ndump=") + path->string();
}

string effective_label(const string& fallback, const string& label) {
    if (!label.empty()) {
        return label;
    }
    return fallback;
}

} // namespace

void set_active_test_options(const TestOptions* options) {
    g_activeOptions = options;
}

const TestOptions& active_test_options() {
    static const TestOptions defaults{};
    return (g_activeOptions != nullptr ? *g_activeOptions : defaults);
}

bool primitive_oracle_enabled(const TestOptions& options) {
    return options.oracleMode == OracleMode::PRIMITIVE || options.oracleMode == OracleMode::ALL;
}

bool planner_oracle_enabled(const TestOptions& options) {
    return options.oracleMode == OracleMode::PLANNER || options.oracleMode == OracleMode::ALL;
}

bool active_primitive_oracle_enabled() {
    return primitive_oracle_enabled(active_test_options());
}

bool active_planner_oracle_enabled() {
    return planner_oracle_enabled(active_test_options());
}

bool trace_enabled(const TestOptions& options) {
    return options.dumpTrace || options.traceLevel != TraceLevel::NONE;
}

bool active_trace_enabled() {
    return trace_enabled(active_test_options());
}

string trace_level_name_string(TraceLevel traceLevel) {
    switch (traceLevel) {
        case TraceLevel::NONE:
            return "none";
        case TraceLevel::SUMMARY:
            return "summary";
        case TraceLevel::FULL:
            return "full";
    }
    return "unknown";
}

string weight_profile_name_string(WeightProfile profile) {
    switch (profile) {
        case WeightProfile::RANDOM:
            return "random";
        case WeightProfile::WEIGHTED_SPLIT_HEAVY:
            return "weighted_split_heavy";
        case WeightProfile::WEIGHTED_JOIN_HEAVY:
            return "weighted_join_heavy";
        case WeightProfile::WEIGHTED_INTEGRATE_HEAVY:
            return "weighted_integrate_heavy";
        case WeightProfile::ARTIFACT_HEAVY:
            return "artifact_heavy";
        case WeightProfile::MULTIEDGE_HEAVY:
            return "multiedge_heavy";
    }
    return "unknown";
}

string precondition_bias_profile_name_string(PreconditionBiasProfile profile) {
    switch (profile) {
        case PreconditionBiasProfile::DEFAULT:
            return "default";
        case PreconditionBiasProfile::BALANCED:
            return "balanced";
        case PreconditionBiasProfile::SPLIT_HEAVY:
            return "split_heavy";
        case PreconditionBiasProfile::JOIN_HEAVY:
            return "join_heavy";
        case PreconditionBiasProfile::INTEGRATE_HEAVY:
            return "integrate_heavy";
        case PreconditionBiasProfile::ARTIFACT_HEAVY:
            return "artifact_heavy";
        case PreconditionBiasProfile::STRUCTURAL:
            return "structural";
    }
    return "unknown";
}

string scenario_family_name_string(ScenarioFamily family) {
    switch (family) {
        case ScenarioFamily::RANDOM:
            return "random";
        case ScenarioFamily::SPLIT_READY:
            return "split_ready";
        case ScenarioFamily::SPLIT_WITH_BOUNDARY_ARTIFACT:
            return "split_with_boundary_artifact";
        case ScenarioFamily::SPLIT_WITH_KEEPOCC_SIBLING:
            return "split_with_keepOcc_sibling";
        case ScenarioFamily::SPLIT_WITH_JOIN_AND_INTEGRATE:
            return "split_with_join_and_integrate";
        case ScenarioFamily::PLANNER_MIXED_TARGETED:
            return "planner_mixed_targeted";
        case ScenarioFamily::JOIN_READY:
            return "join_ready";
        case ScenarioFamily::INTEGRATE_READY:
            return "integrate_ready";
        case ScenarioFamily::PLANNER_MIXED_STRUCTURAL:
            return "planner_mixed_structural";
        case ScenarioFamily::SPLIT_TIE_READY:
            return "split_tie_ready";
        case ScenarioFamily::SPLIT_TIE_STRUCTURAL:
            return "split_tie_structural";
        case ScenarioFamily::PLANNER_TIE_MIXED:
            return "planner_tie_mixed";
        case ScenarioFamily::SPLIT_TIE_SYMMETRIC_LARGE:
            return "split_tie_symmetric_large";
        case ScenarioFamily::PLANNER_TIE_MIXED_SYMMETRIC:
            return "planner_tie_mixed_symmetric";
        case ScenarioFamily::CANONICAL_COLLISION_PROBE:
            return "canonical_collision_probe";
        case ScenarioFamily::SPLIT_TIE_ORGANIC_SYMMETRIC:
            return "split_tie_organic_symmetric";
        case ScenarioFamily::PLANNER_TIE_MIXED_ORGANIC:
            return "planner_tie_mixed_organic";
        case ScenarioFamily::AUTOMORPHISM_PROBE_LARGE:
            return "automorphism_probe_large";
        case ScenarioFamily::PLANNER_TIE_MIXED_ORGANIC_COMPARE_READY:
            return "planner_tie_mixed_organic_compare_ready";
    }
    return "unknown";
}

string split_choice_policy_mode_name_string(SplitChoicePolicyMode mode) {
    switch (mode) {
        case SplitChoicePolicyMode::FAST:
            return "fast";
        case SplitChoicePolicyMode::EXACT_SHADOW:
            return "exact_shadow";
        case SplitChoicePolicyMode::EXACT_FULL:
            return "exact_full";
    }
    return "unknown";
}

string split_choice_compare_mode_name_string(SplitChoiceCompareMode mode) {
    switch (mode) {
        case SplitChoiceCompareMode::NONE:
            return "none";
        case SplitChoiceCompareMode::EXACT_FULL:
            return "exact_full";
    }
    return "unknown";
}

string corpus_policy_name_string(CorpusPolicy policy) {
    switch (policy) {
        case CorpusPolicy::BEST:
            return "best";
        case CorpusPolicy::APPEND:
            return "append";
        case CorpusPolicy::REPLACE:
            return "replace";
    }
    return "unknown";
}

IsolatePrepared prepare_isolate_checked(const RawEngine& RE, RawSkelID sid, OccID occ, const string& label) {
    PrimitiveInvocation invocation;
    invocation.primitive = PrimitiveKind::ISOLATE;
    invocation.sid = sid;
    invocation.occ = occ;

    PrimitiveCallDumpGuard dumpGuard(active_test_options(), RE, invocation, effective_label("isolate", label));
    const IsolatePrepared prep = prepare_isolate_neighborhood(RE, sid, occ);
    if (active_primitive_oracle_enabled()) {
        string failure;
        if (!check_prepare_isolate_oracle(RE, sid, occ, prep, &failure)) {
            throw runtime_error(failure + dump_path_suffix(dumpGuard.path()));
        }
    }
    dumpGuard.mark_success();
    return prep;
}

SplitSeparationPairResult split_checked(
    RawEngine& RE,
    RawSkelID sid,
    Vertex saOrig,
    Vertex sbOrig,
    RawUpdateCtx& U,
    const string& label
) {
    PrimitiveInvocation invocation;
    invocation.primitive = PrimitiveKind::SPLIT;
    invocation.sid = sid;
    invocation.aOrig = saOrig;
    invocation.bOrig = sbOrig;

    PrimitiveCallDumpGuard dumpGuard(active_test_options(), RE, invocation, effective_label("split", label));
    const RawEngine before = RE;
    const SplitSeparationPairResult result = split_separation_pair(RE, sid, saOrig, sbOrig, U);
    if (active_primitive_oracle_enabled()) {
        string failure;
        if (!check_split_oracle(before, RE, invocation, result, &failure)) {
            throw runtime_error(failure + dump_path_suffix(dumpGuard.path()));
        }
    }
    dumpGuard.mark_success();
    return result;
}

JoinSeparationPairResult join_checked(
    RawEngine& RE,
    RawSkelID leftSid,
    RawSkelID rightSid,
    Vertex saOrig,
    Vertex sbOrig,
    RawUpdateCtx& U,
    const string& label
) {
    PrimitiveInvocation invocation;
    invocation.primitive = PrimitiveKind::JOIN;
    invocation.leftSid = leftSid;
    invocation.rightSid = rightSid;
    invocation.aOrig = saOrig;
    invocation.bOrig = sbOrig;

    PrimitiveCallDumpGuard dumpGuard(active_test_options(), RE, invocation, effective_label("join", label));
    const RawEngine before = RE;
    const JoinSeparationPairResult result = join_separation_pair(RE, leftSid, rightSid, saOrig, sbOrig, U);
    if (active_primitive_oracle_enabled()) {
        string failure;
        if (!check_join_oracle(before, RE, invocation, result, &failure)) {
            throw runtime_error(failure + dump_path_suffix(dumpGuard.path()));
        }
    }
    dumpGuard.mark_success();
    return result;
}

IntegrateResult integrate_checked(
    RawEngine& RE,
    RawSkelID parentSid,
    RawSkelID childSid,
    const vector<BoundaryMapEntry>& bm,
    RawUpdateCtx& U,
    const string& label
) {
    PrimitiveInvocation invocation;
    invocation.primitive = PrimitiveKind::INTEGRATE;
    invocation.parentSid = parentSid;
    invocation.childSid = childSid;
    invocation.boundaryMap = bm;

    PrimitiveCallDumpGuard dumpGuard(active_test_options(), RE, invocation, effective_label("integrate", label));
    const RawEngine before = RE;
    const IntegrateResult result = integrate_skeleton(RE, parentSid, childSid, bm, U);
    if (active_primitive_oracle_enabled()) {
        string failure;
        if (!check_integrate_oracle(before, RE, invocation, result, &failure)) {
            throw runtime_error(failure + dump_path_suffix(dumpGuard.path()));
        }
    }
    dumpGuard.mark_success();
    return result;
}
