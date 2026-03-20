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

bool active_oracle_enabled() {
    return active_test_options().oracle;
}

IsolatePrepared prepare_isolate_checked(const RawEngine& RE, RawSkelID sid, OccID occ, const string& label) {
    PrimitiveInvocation invocation;
    invocation.primitive = PrimitiveKind::ISOLATE;
    invocation.sid = sid;
    invocation.occ = occ;

    PrimitiveCallDumpGuard dumpGuard(active_test_options(), RE, invocation, effective_label("isolate", label));
    const IsolatePrepared prep = prepare_isolate_neighborhood(RE, sid, occ);
    if (active_oracle_enabled()) {
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
    if (active_oracle_enabled()) {
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
    if (active_oracle_enabled()) {
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
    if (active_oracle_enabled()) {
        string failure;
        if (!check_integrate_oracle(before, RE, invocation, result, &failure)) {
            throw runtime_error(failure + dump_path_suffix(dumpGuard.path()));
        }
    }
    dumpGuard.mark_success();
    return result;
}
