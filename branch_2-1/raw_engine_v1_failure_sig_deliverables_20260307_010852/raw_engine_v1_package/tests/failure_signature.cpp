#include "failure_signature.hpp"

#include <algorithm>
#include <array>
#include <sstream>
#include <tuple>

#include "reference_model.hpp"
#include "reference_planner.hpp"

using namespace std;

namespace {

u64 fnv1a_update(u64 hash, u64 value) {
    for (int shift = 0; shift < 64; shift += 8) {
        hash ^= static_cast<unsigned char>((value >> shift) & 0xFFU);
        hash *= 1099511628211ULL;
    }
    return hash;
}

u64 fnv1a_update(u64 hash, string_view text) {
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

bool same_optional_hash(const string& lhs, const string& rhs) {
    return lhs.empty() || rhs.empty() || lhs == rhs;
}

u64 begin_hash() {
    return 1469598103934665603ULL;
}

u64 hash_port(u64 hash, const PortSignature& port) {
    hash = fnv1a_update(hash, static_cast<u64>(port.kind));
    hash = fnv1a_update(hash, static_cast<u64>(port.attachOrig));
    hash = fnv1a_update(hash, static_cast<u64>(port.br));
    hash = fnv1a_update(hash, static_cast<u64>(port.side));
    return hash;
}

u64 hash_tiny_graph(u64 hash, const TinyGraphSignature& sig) {
    hash = fnv1a_update(hash, static_cast<u64>(sig.verts.size()));
    for (Vertex v : sig.verts) {
        hash = fnv1a_update(hash, static_cast<u64>(v));
    }
    hash = fnv1a_update(hash, static_cast<u64>(sig.edges.size()));
    for (const auto& edge : sig.edges) {
        hash = fnv1a_update(hash, static_cast<u64>(edge.first));
        hash = fnv1a_update(hash, static_cast<u64>(edge.second));
    }
    return hash;
}

u64 hash_skeleton(u64 hash, const CanonicalSkeletonSignature& sig) {
    hash = fnv1a_update(hash, static_cast<u64>(sig.vertices.size()));
    for (const string& v : sig.vertices) {
        hash = fnv1a_update(hash, v);
    }
    hash = fnv1a_update(hash, static_cast<u64>(sig.edges.size()));
    for (const string& e : sig.edges) {
        hash = fnv1a_update(hash, e);
    }
    hash = fnv1a_update(hash, static_cast<u64>(sig.hostedOcc.size()));
    for (OccID occ : sig.hostedOcc) {
        hash = fnv1a_update(hash, static_cast<u64>(occ));
    }
    return hash;
}

u64 hash_occurrence(u64 hash, const CanonicalOccurrenceSignature& sig) {
    hash = fnv1a_update(hash, static_cast<u64>(sig.occ));
    hash = fnv1a_update(hash, static_cast<u64>(sig.orig));
    hash = fnv1a_update(hash, sig.hostSkel);
    hash = fnv1a_update(hash, sig.centerV);
    hash = fnv1a_update(hash, static_cast<u64>(sig.allocNbr.size()));
    for (OccID occ : sig.allocNbr) {
        hash = fnv1a_update(hash, static_cast<u64>(occ));
    }
    hash = fnv1a_update(hash, static_cast<u64>(sig.corePatchEdges.size()));
    for (const string& edge : sig.corePatchEdges) {
        hash = fnv1a_update(hash, edge);
    }
    return hash;
}

template <class Enum, size_t N>
optional<Enum> parse_named_enum(string_view text, const array<pair<const char*, Enum>, N>& values) {
    for (const auto& entry : values) {
        if (text == entry.first) {
            return entry.second;
        }
    }
    return nullopt;
}

} // namespace

bool FailureSignature::empty() const {
    return failureClass == FailureClass::NONE;
}

bool FailureSignature::same_failure(const FailureSignature& rhs) const {
    if (failureClass != rhs.failureClass) {
        return false;
    }
    if (stage != FailureStage::NONE && rhs.stage != FailureStage::NONE && stage != rhs.stage) {
        return false;
    }
    if (primitiveKind != PrimitiveKind::NONE && rhs.primitiveKind != PrimitiveKind::NONE &&
        primitiveKind != rhs.primitiveKind) {
        return false;
    }
    if (plannerPhase != PlannerPhase::NONE && rhs.plannerPhase != PlannerPhase::NONE &&
        plannerPhase != rhs.plannerPhase) {
        return false;
    }
    if (targetOcc != NIL_U32 && rhs.targetOcc != NIL_U32 && targetOcc != rhs.targetOcc) {
        return false;
    }
    if (mismatchKind != FailureMismatchKind::NONE && rhs.mismatchKind != FailureMismatchKind::NONE &&
        mismatchKind != rhs.mismatchKind) {
        return false;
    }
    return same_optional_hash(canonicalStateHash, rhs.canonicalStateHash) &&
           same_optional_hash(oracleHash, rhs.oracleHash) &&
           same_optional_hash(tracePrefixHash, rhs.tracePrefixHash);
}

const char* primitive_name(PrimitiveKind primitive) {
    switch (primitive) {
        case PrimitiveKind::NONE:
            return "none";
        case PrimitiveKind::ISOLATE:
            return "isolate";
        case PrimitiveKind::SPLIT:
            return "split";
        case PrimitiveKind::JOIN:
            return "join";
        case PrimitiveKind::INTEGRATE:
            return "integrate";
    }
    return "unknown";
}

optional<PrimitiveKind> parse_primitive_kind(const string& text) {
    static const array<pair<const char*, PrimitiveKind>, 5> values = {{
        {"none", PrimitiveKind::NONE},
        {"isolate", PrimitiveKind::ISOLATE},
        {"split", PrimitiveKind::SPLIT},
        {"join", PrimitiveKind::JOIN},
        {"integrate", PrimitiveKind::INTEGRATE},
    }};
    return parse_named_enum<PrimitiveKind>(text, values);
}

string primitive_name_string(PrimitiveKind primitive) {
    return primitive_name(primitive);
}

const char* failure_class_name(FailureClass failureClass) {
    switch (failureClass) {
        case FailureClass::NONE:
            return "none";
        case FailureClass::VALIDATOR_FAILURE:
            return "validator_failure";
        case FailureClass::PRIMITIVE_ORACLE_MISMATCH:
            return "primitive_oracle_mismatch";
        case FailureClass::PLANNER_ORACLE_MISMATCH:
            return "planner_oracle_mismatch";
        case FailureClass::STEP_BUDGET_EXCEEDED:
            return "step_budget_exceeded";
        case FailureClass::SANITIZER_FAILURE:
            return "sanitizer_failure";
        case FailureClass::CRASH:
            return "crash";
    }
    return "unknown";
}

optional<FailureClass> parse_failure_class(string_view text) {
    static const array<pair<const char*, FailureClass>, 7> values = {{
        {"none", FailureClass::NONE},
        {"validator_failure", FailureClass::VALIDATOR_FAILURE},
        {"primitive_oracle_mismatch", FailureClass::PRIMITIVE_ORACLE_MISMATCH},
        {"planner_oracle_mismatch", FailureClass::PLANNER_ORACLE_MISMATCH},
        {"step_budget_exceeded", FailureClass::STEP_BUDGET_EXCEEDED},
        {"sanitizer_failure", FailureClass::SANITIZER_FAILURE},
        {"crash", FailureClass::CRASH},
    }};
    return parse_named_enum<FailureClass>(text, values);
}

string failure_class_name_string(FailureClass failureClass) {
    return failure_class_name(failureClass);
}

const char* failure_stage_name(FailureStage stage) {
    switch (stage) {
        case FailureStage::NONE:
            return "none";
        case FailureStage::PRIMITIVE:
            return "primitive";
        case FailureStage::PLANNER:
            return "planner";
        case FailureStage::REDUCER:
            return "reducer";
        case FailureStage::REPLAY:
            return "replay";
    }
    return "unknown";
}

optional<FailureStage> parse_failure_stage(string_view text) {
    static const array<pair<const char*, FailureStage>, 5> values = {{
        {"none", FailureStage::NONE},
        {"primitive", FailureStage::PRIMITIVE},
        {"planner", FailureStage::PLANNER},
        {"reducer", FailureStage::REDUCER},
        {"replay", FailureStage::REPLAY},
    }};
    return parse_named_enum<FailureStage>(text, values);
}

string failure_stage_name_string(FailureStage stage) {
    return failure_stage_name(stage);
}

const char* planner_phase_name(PlannerPhase phase) {
    switch (phase) {
        case PlannerPhase::NONE:
            return "none";
        case PlannerPhase::ENSURE_SOLE:
            return "ensure_sole";
        case PlannerPhase::ISOLATE:
            return "isolate";
        case PlannerPhase::SPLIT:
            return "split";
        case PlannerPhase::JOIN:
            return "join";
        case PlannerPhase::INTEGRATE:
            return "integrate";
    }
    return "unknown";
}

optional<PlannerPhase> parse_planner_phase(string_view text) {
    static const array<pair<const char*, PlannerPhase>, 6> values = {{
        {"none", PlannerPhase::NONE},
        {"ensure_sole", PlannerPhase::ENSURE_SOLE},
        {"isolate", PlannerPhase::ISOLATE},
        {"split", PlannerPhase::SPLIT},
        {"join", PlannerPhase::JOIN},
        {"integrate", PlannerPhase::INTEGRATE},
    }};
    return parse_named_enum<PlannerPhase>(text, values);
}

string planner_phase_name_string(PlannerPhase phase) {
    return planner_phase_name(phase);
}

const char* failure_mismatch_name(FailureMismatchKind mismatchKind) {
    switch (mismatchKind) {
        case FailureMismatchKind::NONE:
            return "none";
        case FailureMismatchKind::VALIDATION:
            return "validation";
        case FailureMismatchKind::PREPARE:
            return "prepare";
        case FailureMismatchKind::SPLIT_RESULT:
            return "split_result";
        case FailureMismatchKind::SPLIT_STATE:
            return "split_state";
        case FailureMismatchKind::JOIN_RESULT:
            return "join_result";
        case FailureMismatchKind::JOIN_STATE:
            return "join_state";
        case FailureMismatchKind::INTEGRATE_RESULT:
            return "integrate_result";
        case FailureMismatchKind::INTEGRATE_STATE:
            return "integrate_state";
        case FailureMismatchKind::FINAL_STATE:
            return "final_state";
        case FailureMismatchKind::STOP_CONDITION:
            return "stop_condition";
        case FailureMismatchKind::TARGET_PREPARE:
            return "target_prepare";
        case FailureMismatchKind::TRACE_PREFIX:
            return "trace_prefix";
        case FailureMismatchKind::STEP_BUDGET:
            return "step_budget";
        case FailureMismatchKind::SANITIZER:
            return "sanitizer";
        case FailureMismatchKind::CRASH:
            return "crash";
    }
    return "unknown";
}

optional<FailureMismatchKind> parse_failure_mismatch(string_view text) {
    static const array<pair<const char*, FailureMismatchKind>, 16> values = {{
        {"none", FailureMismatchKind::NONE},
        {"validation", FailureMismatchKind::VALIDATION},
        {"prepare", FailureMismatchKind::PREPARE},
        {"split_result", FailureMismatchKind::SPLIT_RESULT},
        {"split_state", FailureMismatchKind::SPLIT_STATE},
        {"join_result", FailureMismatchKind::JOIN_RESULT},
        {"join_state", FailureMismatchKind::JOIN_STATE},
        {"integrate_result", FailureMismatchKind::INTEGRATE_RESULT},
        {"integrate_state", FailureMismatchKind::INTEGRATE_STATE},
        {"final_state", FailureMismatchKind::FINAL_STATE},
        {"stop_condition", FailureMismatchKind::STOP_CONDITION},
        {"target_prepare", FailureMismatchKind::TARGET_PREPARE},
        {"trace_prefix", FailureMismatchKind::TRACE_PREFIX},
        {"step_budget", FailureMismatchKind::STEP_BUDGET},
        {"sanitizer", FailureMismatchKind::SANITIZER},
        {"crash", FailureMismatchKind::CRASH},
    }};
    return parse_named_enum<FailureMismatchKind>(text, values);
}

string failure_mismatch_name_string(FailureMismatchKind mismatchKind) {
    return failure_mismatch_name(mismatchKind);
}

string stable_hash_text(string_view text) {
    return hex_hash(fnv1a_update(begin_hash(), text));
}

string hash_isolate_signature(const IsolatePreparedSignature& sig) {
    u64 hash = begin_hash();
    hash = fnv1a_update(hash, static_cast<u64>(sig.orig));
    hash = fnv1a_update(hash, static_cast<u64>(sig.allocNbr.size()));
    for (OccID occ : sig.allocNbr) {
        hash = fnv1a_update(hash, static_cast<u64>(occ));
    }
    hash = fnv1a_update(hash, static_cast<u64>(sig.ports.size()));
    for (const PortSignature& port : sig.ports) {
        hash = hash_port(hash, port);
    }
    return hex_hash(hash_tiny_graph(hash, sig.core));
}

string hash_split_result_signature(const SplitResultSignature& sig) {
    u64 hash = begin_hash();
    hash = fnv1a_update(hash, static_cast<u64>(sig.aOrig));
    hash = fnv1a_update(hash, static_cast<u64>(sig.bOrig));
    hash = fnv1a_update(hash, static_cast<u64>(sig.child.size()));
    for (const SplitChildSignature& child : sig.child) {
        hash = fnv1a_update(hash, static_cast<u64>(child.boundaryOnly ? 1U : 0U));
        hash = fnv1a_update(hash, static_cast<u64>(child.hostedOcc.size()));
        for (OccID occ : child.hostedOcc) {
            hash = fnv1a_update(hash, static_cast<u64>(occ));
        }
        hash = hash_skeleton(hash, child.graph);
    }
    return hex_hash(hash);
}

string hash_merge_result_signature(const MergeResultSignature& sig) {
    u64 hash = begin_hash();
    hash = hash_skeleton(hash, sig.mergedSkeleton);
    hash = fnv1a_update(hash, static_cast<u64>(sig.hostedOccurrence.size()));
    for (const CanonicalOccurrenceSignature& occ : sig.hostedOccurrence) {
        hash = hash_occurrence(hash, occ);
    }
    return hex_hash(hash);
}

string hash_engine_state_signature(const EngineStateSignature& sig) {
    u64 hash = begin_hash();
    hash = fnv1a_update(hash, static_cast<u64>(sig.skeletons.size()));
    for (const CanonicalSkeletonSignature& skeleton : sig.skeletons) {
        hash = hash_skeleton(hash, skeleton);
    }
    hash = fnv1a_update(hash, static_cast<u64>(sig.occurrences.size()));
    for (const CanonicalOccurrenceSignature& occ : sig.occurrences) {
        hash = hash_occurrence(hash, occ);
    }
    return hex_hash(hash);
}

string hash_engine_state(const RawEngine& RE) {
    return hash_engine_state_signature(capture_engine_state_signature(RE));
}

string hash_planner_trace_prefix(const vector<PlannerTraceEntry>& trace, size_t prefixLen) {
    const size_t limit = (prefixLen == 0U ? trace.size() : min(prefixLen, trace.size()));
    u64 hash = begin_hash();
    hash = fnv1a_update(hash, static_cast<u64>(limit));
    for (size_t i = 0; i < limit; ++i) {
        hash = fnv1a_update(hash, static_cast<u64>(trace[i].primitive));
        hash = fnv1a_update(hash, trace[i].hash);
    }
    return hex_hash(hash);
}

string failure_signature_hash(const FailureSignature& sig) {
    u64 hash = begin_hash();
    hash = fnv1a_update(hash, static_cast<u64>(sig.failureClass));
    hash = fnv1a_update(hash, static_cast<u64>(sig.stage));
    hash = fnv1a_update(hash, static_cast<u64>(sig.primitiveKind));
    hash = fnv1a_update(hash, static_cast<u64>(sig.plannerPhase));
    hash = fnv1a_update(hash, static_cast<u64>(sig.targetOcc));
    hash = fnv1a_update(hash, static_cast<u64>(sig.mismatchKind));
    hash = fnv1a_update(hash, sig.canonicalStateHash);
    hash = fnv1a_update(hash, sig.oracleHash);
    hash = fnv1a_update(hash, sig.tracePrefixHash);
    return hex_hash(hash);
}

FailureSignature make_failure_signature(FailureClass failureClass, const string& detail) {
    FailureSignature sig;
    sig.failureClass = failureClass;
    sig.detail = detail;
    return sig;
}

void write_failure_signature_machine(ostream& os, const FailureSignature& sig) {
    os << "FAIL_CLASS=" << failure_class_name_string(sig.failureClass) << '\n';
    os << "FAIL_STAGE=" << failure_stage_name_string(sig.stage) << '\n';
    os << "FAIL_PRIMITIVE=" << primitive_name_string(sig.primitiveKind) << '\n';
    os << "FAIL_PLANNER_PHASE=" << planner_phase_name_string(sig.plannerPhase) << '\n';
    os << "FAIL_TARGET_OCC=" << sig.targetOcc << '\n';
    os << "FAIL_MISMATCH=" << failure_mismatch_name_string(sig.mismatchKind) << '\n';
    if (!sig.canonicalStateHash.empty()) {
        os << "FAIL_STATE_HASH=" << sig.canonicalStateHash << '\n';
    }
    if (!sig.oracleHash.empty()) {
        os << "FAIL_ORACLE_HASH=" << sig.oracleHash << '\n';
    }
    if (!sig.tracePrefixHash.empty()) {
        os << "FAIL_TRACE_HASH=" << sig.tracePrefixHash << '\n';
    }
    if (!sig.empty()) {
        os << "FAIL_HASH=" << failure_signature_hash(sig) << '\n';
    }
}

FailureSignature parse_failure_signature_machine(const string& text) {
    FailureSignature sig;
    istringstream iss(text);
    string line;
    while (getline(iss, line)) {
        if (line.rfind("FAIL_CLASS=", 0) == 0) {
            if (const optional<FailureClass> value = parse_failure_class(line.substr(11)); value.has_value()) {
                sig.failureClass = *value;
            }
        } else if (line.rfind("FAIL_STAGE=", 0) == 0) {
            if (const optional<FailureStage> value = parse_failure_stage(line.substr(11)); value.has_value()) {
                sig.stage = *value;
            }
        } else if (line.rfind("FAIL_PRIMITIVE=", 0) == 0) {
            if (const optional<PrimitiveKind> value = parse_primitive_kind(line.substr(15)); value.has_value()) {
                sig.primitiveKind = *value;
            }
        } else if (line.rfind("FAIL_PLANNER_PHASE=", 0) == 0) {
            if (const optional<PlannerPhase> value = parse_planner_phase(line.substr(19)); value.has_value()) {
                sig.plannerPhase = *value;
            }
        } else if (line.rfind("FAIL_TARGET_OCC=", 0) == 0) {
            sig.targetOcc = static_cast<OccID>(stoul(line.substr(16)));
        } else if (line.rfind("FAIL_MISMATCH=", 0) == 0) {
            if (const optional<FailureMismatchKind> value = parse_failure_mismatch(line.substr(14)); value.has_value()) {
                sig.mismatchKind = *value;
            }
        } else if (line.rfind("FAIL_STATE_HASH=", 0) == 0) {
            sig.canonicalStateHash = line.substr(16);
        } else if (line.rfind("FAIL_ORACLE_HASH=", 0) == 0) {
            sig.oracleHash = line.substr(17);
        } else if (line.rfind("FAIL_TRACE_HASH=", 0) == 0) {
            sig.tracePrefixHash = line.substr(16);
        }
    }
    sig.detail = text;
    return sig;
}
