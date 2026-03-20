#include "state_dump.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "reference_model.hpp"
#include "reference_planner.hpp"
#include "split_choice_oracle.hpp"
#include "test_harness.hpp"

using namespace std;

namespace {

optional<filesystem::path> g_pendingDumpPath;
u64 g_dumpCounter = 0;

string shell_quote(const string& in);

filesystem::path default_artifact_dir(const TestOptions& options) {
    if (!options.executablePath.empty()) {
        const filesystem::path exe = filesystem::absolute(options.executablePath);
        return exe.parent_path() / "artifacts";
    }
    return filesystem::current_path() / "artifacts";
}

bool contains_text(string_view haystack, string_view needle) {
    return haystack.find(needle) != string_view::npos;
}

string read_text_file(const filesystem::path& path) {
    ifstream ifs(path);
    if (!ifs) {
        return {};
    }
    ostringstream oss;
    oss << ifs.rdbuf();
    return oss.str();
}

FailureSignature parse_logged_failure_signature(const string& text) {
    return parse_failure_signature_machine(text);
}

FailureSignature classify_child_replay_result(int exitCode, const string& text) {
    FailureSignature sig = parse_logged_failure_signature(text);
    if (sig.failureClass != FailureClass::NONE || exitCode == 0) {
        sig.stage = (sig.stage == FailureStage::NONE ? FailureStage::REDUCER : sig.stage);
        return sig;
    }
    if (contains_text(text, "AddressSanitizer") ||
        contains_text(text, "UndefinedBehaviorSanitizer") ||
        contains_text(text, "runtime error:")) {
        FailureSignature sanitizer = make_failure_signature(FailureClass::SANITIZER_FAILURE, text);
        sanitizer.stage = FailureStage::REDUCER;
        sanitizer.mismatchKind = FailureMismatchKind::SANITIZER;
        return sanitizer;
    }
    FailureSignature crash = make_failure_signature(FailureClass::CRASH, text);
    crash.stage = FailureStage::REDUCER;
    crash.mismatchKind = FailureMismatchKind::CRASH;
    return crash;
}

vector<filesystem::directory_entry> list_artifact_files(const filesystem::path& dir) {
    vector<filesystem::directory_entry> out;
    if (!filesystem::exists(dir)) {
        return out;
    }
    for (const auto& entry : filesystem::directory_iterator(dir)) {
        if (entry.is_regular_file()) {
            out.push_back(entry);
        }
    }
    sort(out.begin(), out.end(), [](const auto& lhs, const auto& rhs) {
        return filesystem::last_write_time(lhs.path()) < filesystem::last_write_time(rhs.path());
    });
    return out;
}

void maybe_compress_artifact(const TestOptions& options, const filesystem::path& path) {
    if (!options.compressArtifacts) {
        return;
    }
    const string ext = path.extension().string();
    if (ext == ".gz" || ext == ".txt") {
        return;
    }
    ostringstream cmd;
    cmd << "/usr/bin/gzip -f " << shell_quote(path.string());
    (void)std::system(cmd.str().c_str());
}

void prune_artifact_category(const filesystem::path& dir, size_t& currentCount, size_t maxArtifacts) {
    if (maxArtifacts == 0U || currentCount <= maxArtifacts) {
        return;
    }
    for (const auto& entry : list_artifact_files(dir)) {
        if (currentCount <= maxArtifacts) {
            break;
        }
        filesystem::remove(entry.path());
        --currentCount;
    }
}

void apply_artifact_retention(const TestOptions& options) {
    if (options.maxArtifacts == 0U) {
        return;
    }

    const filesystem::path root = resolve_artifact_dir(options);
    const array<string, 6> leaves = {{"counterexamples", "traces", "logs", "corpus", "reduced", "reduced_planner"}};
    size_t currentCount = 0;
    for (const string& leaf : leaves) {
        currentCount += list_artifact_files(root / leaf).size();
    }
    if (currentCount <= options.maxArtifacts) {
        return;
    }

    for (const string& leaf : array<string, 4>{{"traces", "logs", "counterexamples", "corpus"}}) {
        prune_artifact_category(root / leaf, currentCount, options.maxArtifacts);
    }
    for (const string& leaf : array<string, 2>{{"reduced", "reduced_planner"}}) {
        prune_artifact_category(root / leaf, currentCount, options.maxArtifacts);
    }
}

template <class T>
void rebuild_free_list(SlotPool<T>& pool) {
    pool.freeHead = NIL_U32;
    for (u32 i = static_cast<u32>(pool.a.size()); i > 0; --i) {
        typename SlotPool<T>::Slot& slot = pool.a[i - 1U];
        if (slot.alive) {
            slot.nextFree = NIL_U32;
            continue;
        }
        slot.nextFree = pool.freeHead;
        pool.freeHead = i - 1U;
    }
}

void rebuild_vertex_adjacency(RawEngine& RE) {
    for (auto& slot : RE.V.a) {
        slot.val.adj.clear();
    }
    for (u32 eid = 0; eid < RE.E.a.size(); ++eid) {
        if (!RE.E.a[eid].alive) {
            continue;
        }
        const RawEdge& e = RE.E.a[eid].val;
        RE.V.a[e.a].val.adj.push_back(eid);
        RE.V.a[e.b].val.adj.push_back(eid);
    }
}

void rebuild_occ_index(RawEngine& RE) {
    RE.occOfOrig.clear();
    for (u32 occ = 0; occ < RE.occ.a.size(); ++occ) {
        if (!RE.occ.a[occ].alive) {
            continue;
        }
        RE.occOfOrig[RE.occ.a[occ].val.orig].push_back(occ);
    }
    for (auto& entry : RE.occOfOrig) {
        sort(entry.second.begin(), entry.second.end());
    }
}

void rebuild_engine_indexes(RawEngine& RE) {
    rebuild_vertex_adjacency(RE);
    rebuild_occ_index(RE);
    rebuild_free_list(RE.V);
    rebuild_free_list(RE.E);
    rebuild_free_list(RE.skel);
    rebuild_free_list(RE.occ);
}

string shell_quote(const string& in) {
#ifdef _WIN32
    string out = "\"";
    for (char ch : in) {
        if (ch == '"') {
            out += "\\\"";
        } else {
            out += ch;
        }
    }
    out += '"';
    return out;
#else
    string out = "'";
    for (char ch : in) {
        if (ch == '\'') {
            out += "'\\''";
        } else {
            out += ch;
        }
    }
    out += '\'';
    return out;
#endif
}

struct VertexRec {
    u32 id = 0;
    RawVertexKind kind = RawVertexKind::REAL;
    Vertex orig = NIL_U32;
    OccID occ = 0;
};

struct EdgeRec {
    u32 id = 0;
    RawVID a = 0;
    RawVID b = 0;
    RawEdgeKind kind = RawEdgeKind::CORE_REAL;
    BridgeRef br = 0;
    u8 side = 0;
};

struct SkeletonRec {
    u32 id = 0;
    vector<RawVID> verts;
    vector<RawEID> edges;
    vector<OccID> hostedOcc;
};

struct OccRec {
    u32 id = 0;
    Vertex orig = NIL_U32;
    RawSkelID hostSkel = NIL_U32;
    RawVID centerV = NIL_U32;
    vector<OccID> allocNbr;
    vector<RawEID> corePatchEdges;
};

void require_tag(istream& is, const string& expected) {
    string got;
    if (!(is >> got) || got != expected) {
        throw runtime_error("state dump parse error: expected " + expected);
    }
}

template <class T>
vector<T> read_counted_vector(istream& is) {
    size_t count = 0;
    is >> count;
    if (!is) {
        throw runtime_error("state dump parse error: missing count");
    }
    vector<T> out(count);
    for (size_t i = 0; i < count; ++i) {
        is >> out[i];
        if (!is) {
            throw runtime_error("state dump parse error: missing vector item");
        }
    }
    return out;
}

template <class T>
void write_counted_vector(ostream& os, const vector<T>& values) {
    os << values.size();
    for (const T& value : values) {
        os << ' ' << value;
    }
}

void write_upd_job(ostream& os, const UpdJob& job) {
    os << "job " << static_cast<int>(job.kind)
       << ' ' << job.occ
       << ' ' << job.leftSid
       << ' ' << job.rightSid
       << ' ' << job.aOrig
       << ' ' << job.bOrig
       << ' ' << job.parentSid
       << ' ' << job.childSid
       << ' ' << job.bm.size();
    for (const BoundaryMapEntry& entry : job.bm) {
        os << ' ' << entry.childOrig << ' ' << entry.parentOrig;
    }
    os << '\n';
}

UpdJob read_upd_job(istream& is) {
    UpdJob job;
    int kind = 0;
    size_t bmCount = 0;
    require_tag(is, "job");
    is >> kind >> job.occ >> job.leftSid >> job.rightSid >> job.aOrig >> job.bOrig >> job.parentSid >> job.childSid;
    job.kind = static_cast<UpdJobKind>(kind);
    is >> bmCount;
    job.bm.resize(bmCount);
    for (size_t i = 0; i < bmCount; ++i) {
        is >> job.bm[i].childOrig >> job.bm[i].parentOrig;
    }
    return job;
}

vector<OccID> collect_live_occurrences(const RawEngine& RE) {
    vector<OccID> out;
    for (u32 occ = 0; occ < RE.occ.a.size(); ++occ) {
        if (RE.occ.a[occ].alive) {
            out.push_back(occ);
        }
    }
    return out;
}

void assert_engine_bookkeeping_sane_local(const RawEngine& RE) {
    unordered_map<RawVID, int> vertexOwnerCnt;
    unordered_map<RawEID, int> edgeOwnerCnt;
    unordered_map<OccID, int> occHostCnt;

    for (u32 sid = 0; sid < RE.skel.a.size(); ++sid) {
        if (!RE.skel.a[sid].alive) {
            continue;
        }
        const RawSkeleton& S = RE.skel.get(sid);
        assert_skeleton_wellformed(RE, sid);

        unordered_set<RawVID> localV;
        unordered_set<RawEID> localE;
        unordered_set<OccID> localOcc;
        for (RawVID v : S.verts) {
            if (!localV.insert(v).second || !RE.V.a[v].alive) {
                throw runtime_error("state dump invalid vertex ownership");
            }
            ++vertexOwnerCnt[v];
        }
        for (RawEID e : S.edges) {
            if (!localE.insert(e).second || !RE.E.a[e].alive) {
                throw runtime_error("state dump invalid edge ownership");
            }
            ++edgeOwnerCnt[e];
        }
        for (OccID occ : S.hostedOcc) {
            if (!localOcc.insert(occ).second || !RE.occ.a[occ].alive) {
                throw runtime_error("state dump invalid hosted occurrence");
            }
            ++occHostCnt[occ];
            assert_occ_patch_consistent(RE, occ);
            if (RE.occ.get(occ).hostSkel != sid) {
                throw runtime_error("state dump host skeleton mismatch");
            }
        }
    }

    for (u32 vid = 0; vid < RE.V.a.size(); ++vid) {
        if (RE.V.a[vid].alive && vertexOwnerCnt[vid] != 1) {
            throw runtime_error("state dump vertex owner count mismatch");
        }
    }
    for (u32 eid = 0; eid < RE.E.a.size(); ++eid) {
        if (RE.E.a[eid].alive && edgeOwnerCnt[eid] != 1) {
            throw runtime_error("state dump edge owner count mismatch");
        }
    }

    unordered_map<OccID, int> occOrigCnt;
    for (const auto& entry : RE.occOfOrig) {
        unordered_set<OccID> uniq(entry.second.begin(), entry.second.end());
        if (uniq.size() != entry.second.size()) {
            throw runtime_error("state dump duplicate occOfOrig entry");
        }
        for (OccID occ : entry.second) {
            if (!RE.occ.a[occ].alive || RE.occ.get(occ).orig != entry.first) {
                throw runtime_error("state dump occOfOrig mismatch");
            }
            ++occOrigCnt[occ];
        }
    }
    for (u32 occ = 0; occ < RE.occ.a.size(); ++occ) {
        if (!RE.occ.a[occ].alive) {
            continue;
        }
        const RawOccRecord& O = RE.occ.get(occ);
        if (occHostCnt[occ] != 1 || occOrigCnt[occ] != 1) {
            throw runtime_error("state dump occurrence count mismatch");
        }
        if (O.hostSkel >= RE.skel.a.size() || !RE.skel.a[O.hostSkel].alive) {
            throw runtime_error("state dump dead host skeleton");
        }
    }
}

bool remove_value(vector<u32>& values, u32 target) {
    const auto it = remove(values.begin(), values.end(), target);
    if (it == values.end()) {
        return false;
    }
    values.erase(it, values.end());
    return true;
}

bool remove_occ_id(vector<OccID>& values, OccID target) {
    const auto it = remove(values.begin(), values.end(), target);
    if (it == values.end()) {
        return false;
    }
    values.erase(it, values.end());
    return true;
}

void retire_edge(RawEngine& RE, RawEID eid) {
    if (eid >= RE.E.a.size() || !RE.E.a[eid].alive) {
        return;
    }
    const RawEdge edge = RE.E.a[eid].val;
    remove_value(RE.V.a[edge.a].val.adj, eid);
    remove_value(RE.V.a[edge.b].val.adj, eid);
    for (u32 sid = 0; sid < RE.skel.a.size(); ++sid) {
        if (!RE.skel.a[sid].alive) {
            continue;
        }
        remove_value(RE.skel.a[sid].val.edges, eid);
    }
    for (u32 occ = 0; occ < RE.occ.a.size(); ++occ) {
        if (!RE.occ.a[occ].alive) {
            continue;
        }
        remove_value(RE.occ.a[occ].val.corePatchEdges, eid);
    }
    RE.E.retire(eid);
}

void retire_vertex(RawEngine& RE, RawVID vid) {
    if (vid >= RE.V.a.size() || !RE.V.a[vid].alive) {
        return;
    }
    vector<RawEID> incident = RE.V.a[vid].val.adj;
    for (RawEID eid : incident) {
        retire_edge(RE, eid);
    }
    for (u32 sid = 0; sid < RE.skel.a.size(); ++sid) {
        if (!RE.skel.a[sid].alive) {
            continue;
        }
        remove_value(RE.skel.a[sid].val.verts, vid);
    }
    RE.V.retire(vid);
}

bool vertex_is_essential_for_invocation(const LiveStateDump& dump, RawVID vid) {
    if (vid >= dump.engine.V.a.size() || !dump.engine.V.a[vid].alive) {
        return true;
    }
    const RawVertex& v = dump.engine.V.a[vid].val;
    if (v.kind != RawVertexKind::REAL) {
        return true;
    }
    const PrimitiveInvocation& inv = dump.invocation;
    if (v.orig == inv.aOrig || v.orig == inv.bOrig) {
        return true;
    }
    for (const BoundaryMapEntry& entry : inv.boundaryMap) {
        if (v.orig == entry.childOrig || v.orig == entry.parentOrig) {
            return true;
        }
    }
    return false;
}

void prune_isolated_real_vertices(LiveStateDump& dump) {
    bool changed = true;
    while (changed) {
        changed = false;
        for (u32 vid = 0; vid < dump.engine.V.a.size(); ++vid) {
            if (!dump.engine.V.a[vid].alive) {
                continue;
            }
            const RawVertex& v = dump.engine.V.a[vid].val;
            if (v.kind != RawVertexKind::REAL || !v.adj.empty() || vertex_is_essential_for_invocation(dump, vid)) {
                continue;
            }
            retire_vertex(dump.engine, vid);
            changed = true;
        }
    }
}

bool remove_occurrence(LiveStateDump& dump, OccID occ) {
    if (occ >= dump.engine.occ.a.size() || !dump.engine.occ.a[occ].alive) {
        return false;
    }
    if (occ == dump.invocation.occ) {
        return false;
    }
    const RawOccRecord rec = dump.engine.occ.a[occ].val;
    for (u32 id = 0; id < dump.engine.occ.a.size(); ++id) {
        if (!dump.engine.occ.a[id].alive) {
            continue;
        }
        remove_occ_id(dump.engine.occ.a[id].val.allocNbr, occ);
    }
    remove_occ_id(dump.invocation.keepOcc, occ);

    vector<RawEID> coreEdges = rec.corePatchEdges;
    for (RawEID eid : coreEdges) {
        retire_edge(dump.engine, eid);
    }
    retire_vertex(dump.engine, rec.centerV);
    if (rec.hostSkel < dump.engine.skel.a.size() && dump.engine.skel.a[rec.hostSkel].alive) {
        remove_occ_id(dump.engine.skel.a[rec.hostSkel].val.hostedOcc, occ);
    }
    dump.engine.occ.retire(occ);
    prune_isolated_real_vertices(dump);
    rebuild_engine_indexes(dump.engine);
    return true;
}

bool remove_real_vertex(LiveStateDump& dump, RawVID vid) {
    if (vid >= dump.engine.V.a.size() || !dump.engine.V.a[vid].alive) {
        return false;
    }
    if (vertex_is_essential_for_invocation(dump, vid)) {
        return false;
    }
    if (dump.engine.V.a[vid].val.kind != RawVertexKind::REAL) {
        return false;
    }
    retire_vertex(dump.engine, vid);
    prune_isolated_real_vertices(dump);
    rebuild_engine_indexes(dump.engine);
    return true;
}

bool remove_edge_candidate(LiveStateDump& dump, RawEID eid) {
    if (eid >= dump.engine.E.a.size() || !dump.engine.E.a[eid].alive) {
        return false;
    }
    retire_edge(dump.engine, eid);
    prune_isolated_real_vertices(dump);
    rebuild_engine_indexes(dump.engine);
    return true;
}

filesystem::path write_candidate_dump(const TestOptions& options, const LiveStateDump& dump, const string& stem) {
    const filesystem::path dir = artifact_subdir(options, "reduced");
    const filesystem::path out = dir / (stem + ".txt");
    save_state_dump(out, dump);
    enforce_artifact_retention(options);
    return out;
}

FailureSignature replay_child_state_failure(
    const TestOptions& options,
    const LiveStateDump& dump,
    PrimitiveKind primitive,
    bool oracle
) {
    if (options.executablePath.empty()) {
        throw runtime_error("missing executable path for state reducer");
    }

    const filesystem::path candidate = write_candidate_dump(options, dump, "reducer_candidate_state");
    const filesystem::path logPath =
        artifact_subdir(options, "logs") /
        (current_failure_stem() + "_replay_state_" + primitive_name_string(primitive) + "_" +
         to_string(++g_dumpCounter) + ".log");
    ostringstream cmd;
    cmd << shell_quote(options.executablePath)
        << " --case replay_state"
        << " --state-file " << shell_quote(candidate.string())
        << " --primitive " << primitive_name_string(primitive)
        << " --artifact-dir " << shell_quote(resolve_artifact_dir(options).string());
    if (oracle) {
        cmd << " --oracle primitive";
    }
    cmd << " > " << shell_quote(logPath.string()) << " 2>&1";
    const int exitCode = std::system(cmd.str().c_str());
    return classify_child_replay_result(exitCode, read_text_file(logPath));
}

filesystem::path write_candidate_planner_dump(
    const TestOptions& options,
    const PlannerStateDump& dump,
    const string& stem
) {
    const filesystem::path dir = artifact_subdir(options, "reduced_planner");
    const filesystem::path out = dir / (stem + ".txt");
    save_planner_state_dump(out, dump);
    enforce_artifact_retention(options);
    return out;
}

FailureSignature replay_child_planner_failure(const TestOptions& options, const PlannerStateDump& dump, bool oracle) {
    if (options.executablePath.empty()) {
        throw runtime_error("missing executable path for planner state reducer");
    }

    const filesystem::path candidate = write_candidate_planner_dump(options, dump, "reducer_candidate_planner_state");
    const filesystem::path logPath =
        artifact_subdir(options, "logs") /
        (current_failure_stem() + "_replay_planner_" + to_string(++g_dumpCounter) + ".log");
    ostringstream cmd;
    cmd << shell_quote(options.executablePath)
        << " --case replay_planner_state"
        << " --state-file " << shell_quote(candidate.string())
        << " --artifact-dir " << shell_quote(resolve_artifact_dir(options).string());
    if (oracle) {
        cmd << " --oracle planner";
    }
    if (dump.traceLevel != TraceLevel::NONE || trace_enabled(options)) {
        cmd << " --dump-trace";
    }
    cmd << " > " << shell_quote(logPath.string()) << " 2>&1";
    const int exitCode = std::system(cmd.str().c_str());
    return classify_child_replay_result(exitCode, read_text_file(logPath));
}

size_t live_entity_metric(const LiveStateDump& dump) {
    size_t total = 0;
    for (const auto& slot : dump.engine.V.a) {
        total += slot.alive ? 1U : 0U;
    }
    for (const auto& slot : dump.engine.E.a) {
        total += slot.alive ? 1U : 0U;
    }
    for (const auto& slot : dump.engine.skel.a) {
        total += slot.alive ? 1U : 0U;
    }
    for (const auto& slot : dump.engine.occ.a) {
        total += slot.alive ? 1U : 0U;
    }
    total += dump.invocation.boundaryMap.size();
    total += dump.invocation.keepOcc.size();
    total += dump.invocation.throughBranches.size();
    total += dump.invocation.sequence.size();
    return total;
}

bool accept_candidate(
    const TestOptions& options,
    LiveStateDump& best,
    const LiveStateDump& candidate,
    const FailureSignature& targetFailure,
    PrimitiveKind primitive,
    bool oracle
) {
    if (live_entity_metric(candidate) >= live_entity_metric(best)) {
        return false;
    }
    const FailureSignature candidateFailure = replay_child_state_failure(options, candidate, primitive, oracle);
    if (!candidateFailure.same_failure(targetFailure)) {
        return false;
    }
    best = candidate;
    return true;
}

OccID new_occ(RawEngine& RE, Vertex orig) {
    const OccID occ = RE.occ.alloc(make_occ_record(orig));
    RE.occOfOrig[orig].push_back(occ);
    return occ;
}

RawSkelID new_skeleton(RawEngine& RE) {
    return RE.skel.alloc(RawSkeleton{});
}

void finalize_state(RawEngine& RE) {
    rebuild_engine_indexes(RE);
    assert_engine_bookkeeping_sane_local(RE);
}

LiveStateDump planner_state_as_live_dump(const PlannerStateDump& dump) {
    LiveStateDump live;
    live.engine = dump.engine;
    live.invocation.occ = dump.targetOcc;
    live.invocation.keepOcc = dump.keepOcc;
    return live;
}

void copy_live_dump_back_to_planner(PlannerStateDump& dst, const LiveStateDump& live) {
    dst.engine = live.engine;
    dst.keepOcc = live.invocation.keepOcc;
}

bool remove_planner_occurrence(PlannerStateDump& dump, OccID occ) {
    LiveStateDump live = planner_state_as_live_dump(dump);
    if (!remove_occurrence(live, occ)) {
        return false;
    }
    copy_live_dump_back_to_planner(dump, live);
    return true;
}

bool remove_planner_real_vertex(PlannerStateDump& dump, RawVID vid) {
    LiveStateDump live = planner_state_as_live_dump(dump);
    if (!remove_real_vertex(live, vid)) {
        return false;
    }
    copy_live_dump_back_to_planner(dump, live);
    return true;
}

bool remove_planner_edge_candidate(PlannerStateDump& dump, RawEID eid) {
    LiveStateDump live = planner_state_as_live_dump(dump);
    if (!remove_edge_candidate(live, eid)) {
        return false;
    }
    copy_live_dump_back_to_planner(dump, live);
    return true;
}

array<size_t, 5> planner_state_metric(const PlannerStateDump& dump) {
    return {{
        live_entity_metric(planner_state_as_live_dump(dump)),
        dump.keepOcc.size(),
        dump.initialQueue.size(),
        dump.tracePrefixLength,
        (dump.stepBudget == 0U ? numeric_limits<size_t>::max() / 4U : dump.stepBudget),
    }};
}

bool accept_planner_candidate(
    const TestOptions& options,
    PlannerStateDump& best,
    const PlannerStateDump& candidate,
    const FailureSignature& targetFailure,
    bool oracle
) {
    if (planner_state_metric(candidate) >= planner_state_metric(best)) {
        return false;
    }
    const FailureSignature candidateFailure = replay_child_planner_failure(options, candidate, oracle);
    if (!candidateFailure.same_failure(targetFailure)) {
        return false;
    }
    best = candidate;
    return true;
}

vector<size_t> numeric_shrink_candidates(size_t value, size_t floorValue) {
    vector<size_t> out;
    if (value <= floorValue) {
        return out;
    }

    out.push_back(floorValue);
    if (floorValue + 1U < value) {
        out.push_back(floorValue + 1U);
    }
    for (size_t probe = 1U; probe < value; probe *= 2U) {
        if (probe >= floorValue) {
            out.push_back(probe);
        }
        if (probe > value / 2U) {
            break;
        }
    }
    out.push_back(value / 2U);
    if (value > floorValue + 1U) {
        out.push_back(value - 1U);
    }
    sort(out.begin(), out.end());
    out.erase(unique(out.begin(), out.end()), out.end());
    out.erase(remove_if(out.begin(), out.end(), [&](size_t x) {
        return x < floorValue || x >= value;
    }), out.end());
    return out;
}

bool shrink_planner_step_budget(
    const TestOptions& options,
    PlannerStateDump& best,
    const FailureSignature& targetFailure,
    bool oracle
) {
    if (best.stepBudget <= 1U) {
        return false;
    }
    for (size_t candidateBudget : numeric_shrink_candidates(best.stepBudget, 1U)) {
        PlannerStateDump candidate = best;
        candidate.stepBudget = candidateBudget;
        if (accept_planner_candidate(options, best, candidate, targetFailure, oracle)) {
            return true;
        }
    }
    return false;
}

bool shrink_planner_trace_prefix(
    const TestOptions& options,
    PlannerStateDump& best,
    const FailureSignature& targetFailure,
    bool oracle
) {
    if (best.tracePrefixLength == 0U) {
        return false;
    }
    for (size_t candidatePrefix : numeric_shrink_candidates(best.tracePrefixLength, 0U)) {
        PlannerStateDump candidate = best;
        candidate.tracePrefixLength = candidatePrefix;
        if (accept_planner_candidate(options, best, candidate, targetFailure, oracle)) {
            return true;
        }
    }
    return false;
}

bool shrink_planner_initial_queue(
    const TestOptions& options,
    PlannerStateDump& best,
    const FailureSignature& targetFailure,
    bool oracle
) {
    while (!best.initialQueue.empty()) {
        PlannerStateDump candidate = best;
        candidate.initialQueue.pop_back();
        if (!accept_planner_candidate(options, best, candidate, targetFailure, oracle)) {
            break;
        }
        return true;
    }

    for (size_t i = 0; i < best.initialQueue.size(); ++i) {
        while (!best.initialQueue[i].bm.empty()) {
            PlannerStateDump candidate = best;
            candidate.initialQueue[i].bm.pop_back();
            if (!accept_planner_candidate(options, best, candidate, targetFailure, oracle)) {
                break;
            }
            return true;
        }
    }
    return false;
}

vector<PlannerTraceEntry> collect_planner_trace_for_dump(const PlannerStateDump& dump) {
    RawEngine RE = dump.engine;
    RawUpdateCtx U;
    RawPlannerCtx ctx;
    ctx.targetOcc = dump.targetOcc;
    for (OccID occ : dump.keepOcc) {
        ctx.keepOcc.insert(occ);
    }

    vector<PlannerTraceEntry> trace;
    PlannerPhase activePhase = PlannerPhase::NONE;
    try {
        execute_planner_capture(
            RE,
            ctx,
            U,
            planner_run_options(dump.stepBudget, active_test_options()),
            dump.initialQueue.empty() ? nullptr : &dump.initialQueue,
            trace,
            &activePhase
        );
    } catch (...) {
    }
    return trace;
}

} // namespace

filesystem::path resolve_artifact_dir(const TestOptions& options) {
    const filesystem::path dir = options.artifactDir.has_value()
        ? filesystem::absolute(*options.artifactDir)
        : default_artifact_dir(options);
    filesystem::create_directories(dir);
    return dir;
}

filesystem::path artifact_subdir(const TestOptions& options, const string& leaf) {
    const filesystem::path dir = resolve_artifact_dir(options) / leaf;
    filesystem::create_directories(dir);
    return dir;
}

void enforce_artifact_retention(const TestOptions& options) {
    apply_artifact_retention(options);
}

void write_state_dump_stream(ostream& ofs, const LiveStateDump& dump) {
    ofs << "raw_state_v1\n";
    ofs << "primitive " << primitive_name_string(dump.invocation.primitive) << '\n';
    ofs << "sid " << dump.invocation.sid << '\n';
    ofs << "occ " << dump.invocation.occ << '\n';
    ofs << "left_sid " << dump.invocation.leftSid << '\n';
    ofs << "right_sid " << dump.invocation.rightSid << '\n';
    ofs << "parent_sid " << dump.invocation.parentSid << '\n';
    ofs << "child_sid " << dump.invocation.childSid << '\n';
    ofs << "a_orig " << dump.invocation.aOrig << '\n';
    ofs << "b_orig " << dump.invocation.bOrig << '\n';

    ofs << "boundary_map " << dump.invocation.boundaryMap.size() << '\n';
    for (const BoundaryMapEntry& entry : dump.invocation.boundaryMap) {
        ofs << "bm " << entry.childOrig << ' ' << entry.parentOrig << '\n';
    }

    ofs << "keep_occ ";
    write_counted_vector(ofs, dump.invocation.keepOcc);
    ofs << '\n';
    ofs << "through_branches ";
    write_counted_vector(ofs, dump.invocation.throughBranches);
    ofs << '\n';
    ofs << "sequence " << dump.invocation.sequence.size();
    for (PrimitiveKind primitive : dump.invocation.sequence) {
        ofs << ' ' << primitive_name_string(primitive);
    }
    ofs << '\n';

    size_t vertexCount = 0;
    for (const auto& slot : dump.engine.V.a) {
        vertexCount += slot.alive ? 1U : 0U;
    }
    ofs << "vertices " << vertexCount << '\n';
    for (u32 id = 0; id < dump.engine.V.a.size(); ++id) {
        if (!dump.engine.V.a[id].alive) {
            continue;
        }
        const RawVertex& v = dump.engine.V.a[id].val;
        ofs << "v " << id << ' ' << static_cast<int>(v.kind) << ' ' << v.orig << ' ' << v.occ << '\n';
    }

    size_t edgeCount = 0;
    for (const auto& slot : dump.engine.E.a) {
        edgeCount += slot.alive ? 1U : 0U;
    }
    ofs << "edges " << edgeCount << '\n';
    for (u32 id = 0; id < dump.engine.E.a.size(); ++id) {
        if (!dump.engine.E.a[id].alive) {
            continue;
        }
        const RawEdge& e = dump.engine.E.a[id].val;
        ofs << "e " << id << ' ' << e.a << ' ' << e.b << ' ' << static_cast<int>(e.kind)
            << ' ' << e.br << ' ' << static_cast<int>(e.side) << '\n';
    }

    size_t skeletonCount = 0;
    for (const auto& slot : dump.engine.skel.a) {
        skeletonCount += slot.alive ? 1U : 0U;
    }
    ofs << "skeletons " << skeletonCount << '\n';
    for (u32 id = 0; id < dump.engine.skel.a.size(); ++id) {
        if (!dump.engine.skel.a[id].alive) {
            continue;
        }
        const RawSkeleton& s = dump.engine.skel.a[id].val;
        ofs << "s " << id << ' ';
        write_counted_vector(ofs, s.verts);
        ofs << ' ';
        write_counted_vector(ofs, s.edges);
        ofs << ' ';
        write_counted_vector(ofs, s.hostedOcc);
        ofs << '\n';
    }

    size_t occCount = 0;
    for (const auto& slot : dump.engine.occ.a) {
        occCount += slot.alive ? 1U : 0U;
    }
    ofs << "occurrences " << occCount << '\n';
    for (u32 id = 0; id < dump.engine.occ.a.size(); ++id) {
        if (!dump.engine.occ.a[id].alive) {
            continue;
        }
        const RawOccRecord& occ = dump.engine.occ.a[id].val;
        ofs << "o " << id << ' ' << occ.orig << ' ' << occ.hostSkel << ' ' << occ.centerV << ' ';
        write_counted_vector(ofs, occ.allocNbr);
        ofs << ' ';
        write_counted_vector(ofs, occ.corePatchEdges);
        ofs << '\n';
    }

    ofs << "end\n";
}

void save_state_dump(const filesystem::path& path, const LiveStateDump& dump) {
    ofstream ofs(path);
    if (!ofs) {
        throw runtime_error("failed to write state dump: " + path.string());
    }
    write_state_dump_stream(ofs, dump);
}

LiveStateDump load_state_dump(const filesystem::path& path) {
    ifstream ifs(path);
    if (!ifs) {
        throw runtime_error("failed to open state dump: " + path.string());
    }

    string magic;
    if (!(ifs >> magic) || magic != "raw_state_v1") {
        throw runtime_error("invalid state dump magic");
    }

    LiveStateDump dump;
    string primitiveText;
    require_tag(ifs, "primitive");
    ifs >> primitiveText;
    const optional<PrimitiveKind> primitive = parse_primitive_kind(primitiveText);
    if (!primitive.has_value()) {
        throw runtime_error("invalid primitive in state dump");
    }
    dump.invocation.primitive = *primitive;

    require_tag(ifs, "sid");
    ifs >> dump.invocation.sid;
    require_tag(ifs, "occ");
    ifs >> dump.invocation.occ;
    require_tag(ifs, "left_sid");
    ifs >> dump.invocation.leftSid;
    require_tag(ifs, "right_sid");
    ifs >> dump.invocation.rightSid;
    require_tag(ifs, "parent_sid");
    ifs >> dump.invocation.parentSid;
    require_tag(ifs, "child_sid");
    ifs >> dump.invocation.childSid;
    require_tag(ifs, "a_orig");
    ifs >> dump.invocation.aOrig;
    require_tag(ifs, "b_orig");
    ifs >> dump.invocation.bOrig;

    size_t boundaryCount = 0;
    require_tag(ifs, "boundary_map");
    ifs >> boundaryCount;
    dump.invocation.boundaryMap.resize(boundaryCount);
    for (size_t i = 0; i < boundaryCount; ++i) {
        require_tag(ifs, "bm");
        ifs >> dump.invocation.boundaryMap[i].childOrig >> dump.invocation.boundaryMap[i].parentOrig;
    }

    require_tag(ifs, "keep_occ");
    dump.invocation.keepOcc = read_counted_vector<OccID>(ifs);
    require_tag(ifs, "through_branches");
    dump.invocation.throughBranches = read_counted_vector<u32>(ifs);
    require_tag(ifs, "sequence");
    size_t seqCount = 0;
    ifs >> seqCount;
    dump.invocation.sequence.reserve(seqCount);
    for (size_t i = 0; i < seqCount; ++i) {
        string name;
        ifs >> name;
        const optional<PrimitiveKind> item = parse_primitive_kind(name);
        if (!item.has_value()) {
            throw runtime_error("invalid sequence primitive in state dump");
        }
        dump.invocation.sequence.push_back(*item);
    }

    size_t vertexCount = 0;
    require_tag(ifs, "vertices");
    ifs >> vertexCount;
    vector<VertexRec> vertices(vertexCount);
    for (size_t i = 0; i < vertexCount; ++i) {
        int kind = 0;
        require_tag(ifs, "v");
        ifs >> vertices[i].id >> kind >> vertices[i].orig >> vertices[i].occ;
        vertices[i].kind = static_cast<RawVertexKind>(kind);
    }

    size_t edgeCount = 0;
    require_tag(ifs, "edges");
    ifs >> edgeCount;
    vector<EdgeRec> edges(edgeCount);
    for (size_t i = 0; i < edgeCount; ++i) {
        int kind = 0;
        int side = 0;
        require_tag(ifs, "e");
        ifs >> edges[i].id >> edges[i].a >> edges[i].b >> kind >> edges[i].br >> side;
        edges[i].kind = static_cast<RawEdgeKind>(kind);
        edges[i].side = static_cast<u8>(side);
    }

    size_t skeletonCount = 0;
    require_tag(ifs, "skeletons");
    ifs >> skeletonCount;
    vector<SkeletonRec> skeletons(skeletonCount);
    for (size_t i = 0; i < skeletonCount; ++i) {
        require_tag(ifs, "s");
        ifs >> skeletons[i].id;
        skeletons[i].verts = read_counted_vector<RawVID>(ifs);
        skeletons[i].edges = read_counted_vector<RawEID>(ifs);
        skeletons[i].hostedOcc = read_counted_vector<OccID>(ifs);
    }

    size_t occCount = 0;
    require_tag(ifs, "occurrences");
    ifs >> occCount;
    vector<OccRec> occs(occCount);
    for (size_t i = 0; i < occCount; ++i) {
        require_tag(ifs, "o");
        ifs >> occs[i].id >> occs[i].orig >> occs[i].hostSkel >> occs[i].centerV;
        occs[i].allocNbr = read_counted_vector<OccID>(ifs);
        occs[i].corePatchEdges = read_counted_vector<RawEID>(ifs);
    }

    require_tag(ifs, "end");

    u32 maxVertex = 0;
    u32 maxEdge = 0;
    u32 maxSkeleton = 0;
    u32 maxOcc = 0;
    for (const VertexRec& rec : vertices) {
        maxVertex = max(maxVertex, rec.id);
    }
    for (const EdgeRec& rec : edges) {
        maxEdge = max(maxEdge, rec.id);
    }
    for (const SkeletonRec& rec : skeletons) {
        maxSkeleton = max(maxSkeleton, rec.id);
    }
    for (const OccRec& rec : occs) {
        maxOcc = max(maxOcc, rec.id);
    }

    dump.engine.V.a.resize(vertices.empty() ? 0U : maxVertex + 1U);
    dump.engine.E.a.resize(edges.empty() ? 0U : maxEdge + 1U);
    dump.engine.skel.a.resize(skeletons.empty() ? 0U : maxSkeleton + 1U);
    dump.engine.occ.a.resize(occs.empty() ? 0U : maxOcc + 1U);

    for (VertexRec rec : vertices) {
        dump.engine.V.a[rec.id].alive = true;
        dump.engine.V.a[rec.id].val = RawVertex{rec.kind, rec.orig, rec.occ, {}};
    }
    for (EdgeRec rec : edges) {
        dump.engine.E.a[rec.id].alive = true;
        dump.engine.E.a[rec.id].val = RawEdge{rec.a, rec.b, rec.kind, rec.br, rec.side};
    }
    for (SkeletonRec rec : skeletons) {
        dump.engine.skel.a[rec.id].alive = true;
        dump.engine.skel.a[rec.id].val = RawSkeleton{rec.verts, rec.edges, rec.hostedOcc};
    }
    for (OccRec rec : occs) {
        dump.engine.occ.a[rec.id].alive = true;
        dump.engine.occ.a[rec.id].val = RawOccRecord{
            rec.orig,
            rec.hostSkel,
            rec.centerV,
            rec.allocNbr,
            rec.corePatchEdges,
        };
    }

    rebuild_engine_indexes(dump.engine);
    assert_engine_bookkeeping_sane_local(dump.engine);
    return dump;
}

void write_planner_state_dump_stream(ostream& ofs, const PlannerStateDump& dump) {
    ofs << "planner_state_v2\n";
    ofs << "case " << dump.caseName << '\n';
    ofs << "seed " << (dump.seed.has_value() ? to_string(*dump.seed) : string("none")) << '\n';
    ofs << "iter " << dump.iter << '\n';
    ofs << "target_occ " << dump.targetOcc << '\n';
    ofs << "keep_occ ";
    write_counted_vector(ofs, dump.keepOcc);
    ofs << '\n';
    ofs << "step_budget " << dump.stepBudget << '\n';
    ofs << "trace_prefix_length " << dump.tracePrefixLength << '\n';
    ofs << "trace_level " << trace_level_name_string(dump.traceLevel) << '\n';
    ofs << "initial_queue " << dump.initialQueue.size() << '\n';
    for (const UpdJob& job : dump.initialQueue) {
        write_upd_job(ofs, job);
    }
    ofs << "raw_begin\n";

    LiveStateDump rawDump;
    rawDump.engine = dump.engine;
    write_state_dump_stream(ofs, rawDump);
}

void save_planner_state_dump(const filesystem::path& path, const PlannerStateDump& dump) {
    ofstream ofs(path);
    if (!ofs) {
        throw runtime_error("failed to write planner state dump: " + path.string());
    }
    write_planner_state_dump_stream(ofs, dump);
}

string serialize_planner_state_dump(const PlannerStateDump& dump) {
    ostringstream oss;
    write_planner_state_dump_stream(oss, dump);
    return oss.str();
}

PlannerStateDump load_planner_state_dump(const filesystem::path& path) {
    ifstream ifs(path);
    if (!ifs) {
        throw runtime_error("failed to open planner state dump: " + path.string());
    }

    string magic;
    if (!(ifs >> magic) || (magic != "planner_state_v1" && magic != "planner_state_v2")) {
        throw runtime_error("invalid planner state dump magic");
    }
    const bool isV2 = (magic == "planner_state_v2");

    PlannerStateDump dump;
    dump.traceLevel = TraceLevel::SUMMARY;
    require_tag(ifs, "case");
    ifs >> dump.caseName;
    require_tag(ifs, "seed");
    string seedText;
    ifs >> seedText;
    if (seedText != "none") {
        dump.seed = static_cast<u32>(stoul(seedText));
    }
    require_tag(ifs, "iter");
    ifs >> dump.iter;
    require_tag(ifs, "target_occ");
    ifs >> dump.targetOcc;
    require_tag(ifs, "keep_occ");
    dump.keepOcc = read_counted_vector<OccID>(ifs);
    require_tag(ifs, "step_budget");
    ifs >> dump.stepBudget;
    if (isV2) {
        require_tag(ifs, "trace_prefix_length");
        ifs >> dump.tracePrefixLength;
        require_tag(ifs, "trace_level");
        string traceLevel;
        ifs >> traceLevel;
        if (traceLevel == "none") {
            dump.traceLevel = TraceLevel::NONE;
        } else if (traceLevel == "full") {
            dump.traceLevel = TraceLevel::FULL;
        } else {
            dump.traceLevel = TraceLevel::SUMMARY;
        }
        require_tag(ifs, "initial_queue");
        size_t queueCount = 0;
        ifs >> queueCount;
        dump.initialQueue.reserve(queueCount);
        for (size_t i = 0; i < queueCount; ++i) {
            dump.initialQueue.push_back(read_upd_job(ifs));
        }
    } else {
        require_tag(ifs, "dump_trace");
        int dumpTrace = 0;
        ifs >> dumpTrace;
        dump.traceLevel = (dumpTrace != 0 ? TraceLevel::FULL : TraceLevel::SUMMARY);
    }
    require_tag(ifs, "raw_begin");

    ostringstream rawText;
    rawText << ifs.rdbuf();
    const filesystem::path rawPath = path.string() + ".rawtmp";
    ofstream rawOfs(rawPath);
    rawOfs << rawText.str();
    rawOfs.close();
    dump.engine = load_state_dump(rawPath).engine;
    filesystem::remove(rawPath);
    return dump;
}

void set_pending_dump_path(const optional<filesystem::path>& path) {
    g_pendingDumpPath = path;
}

optional<filesystem::path> pending_dump_path() {
    return g_pendingDumpPath;
}

PrimitiveCallDumpGuard::PrimitiveCallDumpGuard(
    const TestOptions& options,
    const RawEngine& engine,
    const PrimitiveInvocation& invocation,
    const string& label
) {
    if (!options.dumpOnFail) {
        return;
    }
    const filesystem::path dir = artifact_subdir(options, "counterexamples");
    const filesystem::path out =
        dir / (current_failure_stem() + "_primitive_" + label + "_" + to_string(++g_dumpCounter) + ".txt");
    save_state_dump(out, LiveStateDump{invocation, engine});
    enforce_artifact_retention(options);
    path_ = out;
    set_pending_dump_path(out);
}

PrimitiveCallDumpGuard::~PrimitiveCallDumpGuard() {
    if (!path_.has_value()) {
        return;
    }
    if (!success_) {
        return;
    }
    filesystem::remove(*path_);
    if (g_pendingDumpPath.has_value() && *g_pendingDumpPath == *path_) {
        g_pendingDumpPath.reset();
    }
}

const optional<filesystem::path>& PrimitiveCallDumpGuard::path() const {
    return path_;
}

void PrimitiveCallDumpGuard::mark_success() {
    success_ = true;
}

PlannerCallDumpGuard::PlannerCallDumpGuard(const TestOptions& options, const PlannerStateDump& dump, const string& label) {
    if (!options.dumpOnFail) {
        return;
    }
    const filesystem::path dir = artifact_subdir(options, "counterexamples");
    const filesystem::path out =
        dir / (current_failure_stem() + "_planner_" + label + "_" + to_string(++g_dumpCounter) + ".txt");
    save_planner_state_dump(out, dump);
    enforce_artifact_retention(options);
    path_ = out;
    set_pending_dump_path(out);
}

PlannerCallDumpGuard::~PlannerCallDumpGuard() {
    if (!path_.has_value() || !success_) {
        return;
    }
    filesystem::remove(*path_);
    if (g_pendingDumpPath.has_value() && *g_pendingDumpPath == *path_) {
        g_pendingDumpPath.reset();
    }
}

const optional<filesystem::path>& PlannerCallDumpGuard::path() const {
    return path_;
}

void PlannerCallDumpGuard::mark_success() {
    success_ = true;
}

FailureSignature classify_message(
    const string& message,
    bool plannerOracle,
    FailureStage stage,
    PrimitiveKind primitiveKind,
    PlannerPhase plannerPhase,
    OccID targetOcc,
    const RawEngine* state
) {
    FailureSignature sig;
    sig.stage = stage;
    sig.primitiveKind = primitiveKind;
    sig.plannerPhase = plannerPhase;
    sig.targetOcc = targetOcc;
    sig.detail = message;

    if (contains_text(message, "step budget exceeded")) {
        sig.failureClass = FailureClass::STEP_BUDGET_EXCEEDED;
        sig.mismatchKind = FailureMismatchKind::STEP_BUDGET;
        return sig;
    }
    if (contains_text(message, "AddressSanitizer") ||
        contains_text(message, "UndefinedBehaviorSanitizer") ||
        contains_text(message, "runtime error:")) {
        sig.failureClass = FailureClass::SANITIZER_FAILURE;
        sig.mismatchKind = FailureMismatchKind::SANITIZER;
        return sig;
    }
    if (contains_text(message, "oracle mismatch")) {
        sig.failureClass =
            plannerOracle ? FailureClass::PLANNER_ORACLE_MISMATCH : FailureClass::PRIMITIVE_ORACLE_MISMATCH;
        return sig;
    }
    if (contains_text(message, "state dump") ||
        contains_text(message, "validator") ||
        contains_text(message, "ownership") ||
        contains_text(message, "host skeleton") ||
        contains_text(message, "occurrence count mismatch")) {
        sig.failureClass = FailureClass::VALIDATOR_FAILURE;
        sig.mismatchKind = FailureMismatchKind::VALIDATION;
        if (state != nullptr) {
            sig.canonicalStateHash = hash_engine_state(*state);
        }
        return sig;
    }
    sig.failureClass = FailureClass::CRASH;
    sig.mismatchKind = FailureMismatchKind::CRASH;
    return sig;
}

string render_trace_artifact(const vector<PlannerTraceEntry>& trace, TraceLevel traceLevel, size_t prefixLength) {
    if (traceLevel == TraceLevel::NONE) {
        return {};
    }

    const size_t limit = (prefixLength == 0U ? trace.size() : min(prefixLength, trace.size()));
    if (traceLevel == TraceLevel::FULL) {
        const auto prefixEnd =
            trace.begin() + static_cast<vector<PlannerTraceEntry>::difference_type>(limit);
        vector<PlannerTraceEntry> prefix(trace.begin(), prefixEnd);
        return describe_planner_trace(prefix);
    }

    ostringstream oss;
    oss << "trace_count=" << trace.size() << '\n';
    oss << "trace_prefix_count=" << limit << '\n';
    oss << "trace_prefix_hash=" << hash_planner_trace_prefix(trace, limit) << '\n';
    const size_t summaryLimit = min<size_t>(limit, 16U);
    for (size_t i = 0; i < summaryLimit; ++i) {
        oss << i << ": " << planner_primitive_name(trace[i].primitive)
            << " hash=" << trace[i].hash
            << " " << trace[i].summary << '\n';
    }
    return oss.str();
}

filesystem::path save_trace_artifact(
    const TestOptions& options,
    const filesystem::path& dir,
    const string& stem,
    const vector<PlannerTraceEntry>& trace,
    TraceLevel traceLevel,
    size_t prefixLength
) {
    const filesystem::path out = dir / (stem + ".log");
    ofstream ofs(out);
    if (!ofs) {
        throw runtime_error("failed to write trace artifact: " + out.string());
    }
    ofs << render_trace_artifact(trace, traceLevel, prefixLength);
    maybe_compress_artifact(options, out);
    enforce_artifact_retention(options);
    return out;
}

FailureSignature make_primitive_oracle_failure(
    PrimitiveKind primitive,
    OccID targetOcc,
    FailureMismatchKind mismatchKind,
    const string& canonicalStateHash,
    const string& oracleHash,
    const string& detail
) {
    FailureSignature sig;
    sig.failureClass = FailureClass::PRIMITIVE_ORACLE_MISMATCH;
    sig.stage = FailureStage::REPLAY;
    sig.primitiveKind = primitive;
    sig.targetOcc = targetOcc;
    sig.mismatchKind = mismatchKind;
    sig.canonicalStateHash = canonicalStateHash;
    sig.oracleHash = oracleHash;
    sig.detail = detail;
    return sig;
}

FailureSignature make_planner_oracle_failure(
    OccID targetOcc,
    FailureMismatchKind mismatchKind,
    const string& canonicalStateHash,
    const string& oracleHash,
    const string& tracePrefixHash,
    const string& detail
) {
    FailureSignature sig;
    sig.failureClass = FailureClass::PLANNER_ORACLE_MISMATCH;
    sig.stage = FailureStage::REPLAY;
    sig.targetOcc = targetOcc;
    sig.mismatchKind = mismatchKind;
    sig.canonicalStateHash = canonicalStateHash;
    sig.oracleHash = oracleHash;
    sig.tracePrefixHash = tracePrefixHash;
    sig.detail = detail;
    return sig;
}

FailureSignature make_primitive_validation_failure(
    PrimitiveKind primitive,
    OccID targetOcc,
    const RawEngine& state,
    const string& detail
) {
    FailureSignature sig;
    sig.failureClass = FailureClass::VALIDATOR_FAILURE;
    sig.stage = FailureStage::REPLAY;
    sig.primitiveKind = primitive;
    sig.targetOcc = targetOcc;
    sig.mismatchKind = FailureMismatchKind::VALIDATION;
    sig.canonicalStateHash = hash_engine_state(state);
    sig.detail = detail;
    return sig;
}

FailureSignature make_planner_validation_failure(
    OccID targetOcc,
    PlannerPhase plannerPhase,
    const RawEngine& state,
    const vector<PlannerTraceEntry>& trace,
    size_t tracePrefixLength,
    const string& detail
) {
    FailureSignature sig;
    sig.failureClass = FailureClass::VALIDATOR_FAILURE;
    sig.stage = FailureStage::REPLAY;
    sig.plannerPhase = plannerPhase;
    sig.targetOcc = targetOcc;
    sig.mismatchKind = FailureMismatchKind::VALIDATION;
    sig.canonicalStateHash = hash_engine_state(state);
    sig.tracePrefixHash = hash_planner_trace_prefix(trace, tracePrefixLength);
    sig.detail = detail;
    return sig;
}

bool replay_state_dump(
    const TestOptions&,
    const LiveStateDump& dump,
    PrimitiveKind primitive,
    bool oracle,
    FailureSignature* failure
) {
    RawEngine RE = dump.engine;
    try {
        RawUpdateCtx U;
        assert_engine_bookkeeping_sane_local(RE);

        if (primitive == PrimitiveKind::ISOLATE) {
            const IsolatePrepared prep = prepare_isolate_neighborhood(RE, dump.invocation.sid, dump.invocation.occ);
            if (oracle) {
                const IsolatePreparedSignature actualSig = capture_isolate_signature(prep);
                const IsolatePreparedSignature expectedSig = capture_isolate_signature(
                    reference_prepare_isolate_neighborhood(RE, dump.invocation.sid, dump.invocation.occ)
                );
                if (!(expectedSig == actualSig)) {
                    if (failure != nullptr) {
                        *failure = make_primitive_oracle_failure(
                            primitive,
                            dump.invocation.occ,
                            FailureMismatchKind::PREPARE,
                            hash_engine_state(RE),
                            hash_isolate_signature(expectedSig),
                            string("prepare_isolate_neighborhood oracle mismatch\nexpected=") +
                                describe_signature(expectedSig) +
                                "\nactual=" + describe_signature(actualSig)
                        );
                    }
                    return false;
                }
            }
            assert_engine_bookkeeping_sane_local(RE);
            return true;
        }

        if (primitive == PrimitiveKind::SPLIT) {
            const RawEngine before = RE;
            const SplitSeparationPairResult result = split_separation_pair(
                RE,
                dump.invocation.sid,
                dump.invocation.aOrig,
                dump.invocation.bOrig,
                U
            );
            assert_engine_bookkeeping_sane_local(RE);
            if (oracle) {
                RawEngine expected = before;
                const SplitSeparationPairResult refResult = reference_split_separation_pair(
                    expected,
                    dump.invocation.sid,
                    dump.invocation.aOrig,
                    dump.invocation.bOrig
                );
                const SplitResultSignature expectedResult = capture_split_result_signature(expected, refResult);
                const SplitResultSignature actualResult = capture_split_result_signature(RE, result);
                if (!(expectedResult == actualResult)) {
                    if (failure != nullptr) {
                        *failure = make_primitive_oracle_failure(
                            primitive,
                            dump.invocation.occ,
                            FailureMismatchKind::SPLIT_RESULT,
                            hash_engine_state(RE),
                            hash_split_result_signature(expectedResult),
                            string("split_separation_pair result mismatch\nexpected=") +
                                describe_signature(expectedResult) +
                                "\nactual=" + describe_signature(actualResult)
                        );
                    }
                    return false;
                }

                const EngineStateSignature expectedState = capture_engine_state_signature(expected);
                const EngineStateSignature actualState = capture_engine_state_signature(RE);
                if (!(expectedState == actualState)) {
                    if (failure != nullptr) {
                        *failure = make_primitive_oracle_failure(
                            primitive,
                            dump.invocation.occ,
                            FailureMismatchKind::SPLIT_STATE,
                            hash_engine_state_signature(actualState),
                            hash_engine_state_signature(expectedState),
                            string("split_separation_pair state mismatch\nexpected=") +
                                describe_signature(expectedState) +
                                "\nactual=" + describe_signature(actualState)
                        );
                    }
                    return false;
                }
            }
            return true;
        }

        if (primitive == PrimitiveKind::JOIN) {
            const RawEngine before = RE;
            const JoinSeparationPairResult result = join_separation_pair(
                RE,
                dump.invocation.leftSid,
                dump.invocation.rightSid,
                dump.invocation.aOrig,
                dump.invocation.bOrig,
                U
            );
            assert_engine_bookkeeping_sane_local(RE);
            if (oracle) {
                RawEngine expected = before;
                const JoinSeparationPairResult refResult = reference_join_separation_pair(
                    expected,
                    dump.invocation.leftSid,
                    dump.invocation.rightSid,
                    dump.invocation.aOrig,
                    dump.invocation.bOrig
                );
                const MergeResultSignature expectedResult = capture_join_result_signature(expected, refResult);
                const MergeResultSignature actualResult = capture_join_result_signature(RE, result);
                if (!(expectedResult == actualResult)) {
                    if (failure != nullptr) {
                        *failure = make_primitive_oracle_failure(
                            primitive,
                            dump.invocation.occ,
                            FailureMismatchKind::JOIN_RESULT,
                            hash_engine_state(RE),
                            hash_merge_result_signature(expectedResult),
                            string("join_separation_pair result mismatch\nexpected=") +
                                describe_signature(expectedResult) +
                                "\nactual=" + describe_signature(actualResult)
                        );
                    }
                    return false;
                }

                const EngineStateSignature expectedState = capture_engine_state_signature(expected);
                const EngineStateSignature actualState = capture_engine_state_signature(RE);
                if (!(expectedState == actualState)) {
                    if (failure != nullptr) {
                        *failure = make_primitive_oracle_failure(
                            primitive,
                            dump.invocation.occ,
                            FailureMismatchKind::JOIN_STATE,
                            hash_engine_state_signature(actualState),
                            hash_engine_state_signature(expectedState),
                            string("join_separation_pair state mismatch\nexpected=") +
                                describe_signature(expectedState) +
                                "\nactual=" + describe_signature(actualState)
                        );
                    }
                    return false;
                }
            }
            return true;
        }

        const RawEngine before = RE;
        const IntegrateResult result = integrate_skeleton(
            RE,
            dump.invocation.parentSid,
            dump.invocation.childSid,
            dump.invocation.boundaryMap,
            U
        );
        assert_engine_bookkeeping_sane_local(RE);
        if (oracle) {
            RawEngine expected = before;
            const IntegrateResult refResult = reference_integrate_skeleton(
                expected,
                dump.invocation.parentSid,
                dump.invocation.childSid,
                dump.invocation.boundaryMap
            );
            const MergeResultSignature expectedResult = capture_integrate_result_signature(expected, refResult);
            const MergeResultSignature actualResult = capture_integrate_result_signature(RE, result);
            if (!(expectedResult == actualResult)) {
                if (failure != nullptr) {
                    *failure = make_primitive_oracle_failure(
                        primitive,
                        dump.invocation.occ,
                        FailureMismatchKind::INTEGRATE_RESULT,
                        hash_engine_state(RE),
                        hash_merge_result_signature(expectedResult),
                        string("integrate_skeleton result mismatch\nexpected=") +
                            describe_signature(expectedResult) +
                            "\nactual=" + describe_signature(actualResult)
                    );
                }
                return false;
            }

            const EngineStateSignature expectedState = capture_engine_state_signature(expected);
            const EngineStateSignature actualState = capture_engine_state_signature(RE);
            if (!(expectedState == actualState)) {
                if (failure != nullptr) {
                    *failure = make_primitive_oracle_failure(
                        primitive,
                        dump.invocation.occ,
                        FailureMismatchKind::INTEGRATE_STATE,
                        hash_engine_state_signature(actualState),
                        hash_engine_state_signature(expectedState),
                        string("integrate_skeleton state mismatch\nexpected=") +
                            describe_signature(expectedState) +
                            "\nactual=" + describe_signature(actualState)
                    );
                }
                return false;
            }
        }
        return true;
    } catch (const exception& ex) {
        if (failure != nullptr) {
            FailureSignature sig = classify_message(
                ex.what(),
                false,
                FailureStage::REPLAY,
                primitive,
                PlannerPhase::NONE,
                dump.invocation.occ,
                &RE
            );
            if (sig.failureClass == FailureClass::VALIDATOR_FAILURE) {
                sig = make_primitive_validation_failure(primitive, dump.invocation.occ, RE, ex.what());
            }
            *failure = sig;
        }
        return false;
    }
}

bool replay_planner_state_dump(
    const TestOptions& options,
    const PlannerStateDump& dump,
    bool oracle,
    FailureSignature* failure
) {
    if (dump.caseName.rfind("split_choice_", 0) == 0) {
        return replay_split_choice_oracle_dump(options, dump, failure);
    }

    RawEngine RE = dump.engine;
    vector<PlannerTraceEntry> trace;
    PlannerPhase activePhase = PlannerPhase::NONE;

    auto maybe_save_failure_trace = [&](const string& stemSuffix) {
        const TraceLevel traceLevel =
            options.dumpTrace ? TraceLevel::FULL :
            (dump.traceLevel != TraceLevel::SUMMARY || !trace_enabled(options) ? dump.traceLevel : options.traceLevel);
        if (traceLevel == TraceLevel::NONE) {
            return;
        }
        (void)save_trace_artifact(
            options,
            artifact_subdir(options, "traces"),
            current_failure_stem() + "_" + stemSuffix,
            trace,
            traceLevel,
            dump.tracePrefixLength
        );
    };

    try {
        RawUpdateCtx U;
        assert_engine_bookkeeping_sane_local(RE);

        RawPlannerCtx ctx;
        ctx.targetOcc = dump.targetOcc;
        for (OccID occ : dump.keepOcc) {
            ctx.keepOcc.insert(occ);
        }
        const RawUpdaterRunOptions runOptions = planner_run_options(dump.stepBudget, active_test_options());

        execute_planner_capture(
            RE,
            ctx,
            U,
            runOptions,
            dump.initialQueue.empty() ? nullptr : &dump.initialQueue,
            trace,
            &activePhase
        );
        assert_engine_bookkeeping_sane_local(RE);

        if (oracle) {
            const PlannerOracleComparison comparison = compare_planner_oracle(
                dump.engine,
                RE,
                ctx,
                runOptions,
                trace,
                dump.initialQueue.empty() ? nullptr : &dump.initialQueue
            );
            if (!comparison.matches) {
                maybe_save_failure_trace("replay_planner_failure");

                string actualHash;
                string expectedHash;
                switch (comparison.mismatchKind) {
                    case FailureMismatchKind::FINAL_STATE:
                        actualHash = hash_engine_state_signature(comparison.actual.finalState);
                        expectedHash = hash_engine_state_signature(comparison.expected.finalState);
                        break;
                    case FailureMismatchKind::STOP_CONDITION:
                        actualHash = stable_hash_text(
                            comparison.actual.stopConditionSatisfied ? "stop_condition=1" : "stop_condition=0"
                        );
                        expectedHash = stable_hash_text(
                            comparison.expected.stopConditionSatisfied ? "stop_condition=1" : "stop_condition=0"
                        );
                        break;
                    case FailureMismatchKind::TARGET_PREPARE:
                        actualHash = hash_isolate_signature(comparison.actual.targetPrepare);
                        expectedHash = hash_isolate_signature(comparison.expected.targetPrepare);
                        break;
                    default:
                        break;
                }

                if (failure != nullptr) {
                    ostringstream oss;
                    oss << "planner_oracle_mismatch"
                        << "\ncase=" << dump.caseName
                        << "\nseed=" << (dump.seed.has_value() ? to_string(*dump.seed) : string("none"))
                        << "\niter=" << dump.iter
                        << "\ntargetOcc=" << dump.targetOcc
                        << "\nstepBudget=" << dump.stepBudget
                        << "\nexpected=" << describe_planner_signature(comparison.expected)
                        << "\nactual=" << describe_planner_signature(comparison.actual);
                    *failure = make_planner_oracle_failure(
                        dump.targetOcc,
                        comparison.mismatchKind,
                        actualHash,
                        expectedHash,
                        hash_planner_trace_prefix(trace, dump.tracePrefixLength),
                        oss.str()
                    );
                }
                return false;
            }
        }
        return true;
    } catch (const exception& ex) {
        maybe_save_failure_trace("replay_planner_exception");
        if (failure != nullptr) {
            FailureSignature sig = classify_message(
                ex.what(),
                true,
                FailureStage::REPLAY,
                PrimitiveKind::NONE,
                activePhase,
                dump.targetOcc,
                &RE
            );
            sig.tracePrefixHash = hash_planner_trace_prefix(trace, dump.tracePrefixLength);
            if (sig.failureClass == FailureClass::VALIDATOR_FAILURE) {
                sig = make_planner_validation_failure(
                    dump.targetOcc,
                    activePhase,
                    RE,
                    trace,
                    dump.tracePrefixLength,
                    ex.what()
                );
            }
            *failure = sig;
        }
        return false;
    }
}

filesystem::path reduce_state_dump_file(
    const TestOptions& options,
    const filesystem::path& stateFile,
    PrimitiveKind primitive,
    bool oracle
) {
    LiveStateDump best = load_state_dump(stateFile);
    const FailureSignature targetFailure = replay_child_state_failure(options, best, primitive, oracle);
    if (targetFailure.empty()) {
        throw runtime_error("state file does not reproduce a failure");
    }

    bool changed = true;
    while (changed) {
        changed = false;

        while (!best.invocation.sequence.empty()) {
            LiveStateDump candidate = best;
            candidate.invocation.sequence.pop_back();
            if (!accept_candidate(options, best, candidate, targetFailure, primitive, oracle)) {
                break;
            }
            changed = true;
        }

        while (!best.invocation.throughBranches.empty()) {
            LiveStateDump candidate = best;
            candidate.invocation.throughBranches.pop_back();
            if (!accept_candidate(options, best, candidate, targetFailure, primitive, oracle)) {
                break;
            }
            changed = true;
        }

        while (!best.invocation.keepOcc.empty()) {
            LiveStateDump candidate = best;
            candidate.invocation.keepOcc.pop_back();
            if (!accept_candidate(options, best, candidate, targetFailure, primitive, oracle)) {
                break;
            }
            changed = true;
        }

        while (!best.invocation.boundaryMap.empty()) {
            LiveStateDump candidate = best;
            candidate.invocation.boundaryMap.pop_back();
            if (!accept_candidate(options, best, candidate, targetFailure, primitive, oracle)) {
                break;
            }
            changed = true;
        }

        vector<OccID> occs = collect_live_occurrences(best.engine);
        for (OccID occ : occs) {
            LiveStateDump candidate = best;
            if (!remove_occurrence(candidate, occ)) {
                continue;
            }
            if (accept_candidate(options, best, candidate, targetFailure, primitive, oracle)) {
                changed = true;
                break;
            }
        }
        if (changed) {
            continue;
        }

        for (u32 vid = 0; vid < best.engine.V.a.size(); ++vid) {
            LiveStateDump candidate = best;
            if (!remove_real_vertex(candidate, vid)) {
                continue;
            }
            if (accept_candidate(options, best, candidate, targetFailure, primitive, oracle)) {
                changed = true;
                break;
            }
        }
        if (changed) {
            continue;
        }

        for (u32 eid = 0; eid < best.engine.E.a.size(); ++eid) {
            LiveStateDump candidate = best;
            if (!remove_edge_candidate(candidate, eid)) {
                continue;
            }
            if (accept_candidate(options, best, candidate, targetFailure, primitive, oracle)) {
                changed = true;
                break;
            }
        }
    }

    const filesystem::path dir = artifact_subdir(options, "reduced");
    const filesystem::path out =
        dir / (current_failure_stem() + "_reduced_state_" + primitive_name_string(primitive) + ".txt");
    save_state_dump(out, best);
    return out;
}

filesystem::path reduce_planner_state_dump_file(
    const TestOptions& options,
    const filesystem::path& stateFile,
    bool oracle
) {
    PlannerStateDump best = load_planner_state_dump(stateFile);
    const FailureSignature targetFailure = replay_child_planner_failure(options, best, oracle);
    if (targetFailure.empty()) {
        throw runtime_error("planner state file does not reproduce a failure");
    }

    bool changed = true;
    while (changed) {
        changed = false;

        while (!best.keepOcc.empty()) {
            PlannerStateDump candidate = best;
            candidate.keepOcc.pop_back();
            if (!accept_planner_candidate(options, best, candidate, targetFailure, oracle)) {
                break;
            }
            changed = true;
        }
        if (changed) {
            continue;
        }

        if (shrink_planner_initial_queue(options, best, targetFailure, oracle)) {
            changed = true;
            continue;
        }

        if (shrink_planner_trace_prefix(options, best, targetFailure, oracle)) {
            changed = true;
            continue;
        }

        if (shrink_planner_step_budget(options, best, targetFailure, oracle)) {
            changed = true;
            continue;
        }

        vector<OccID> occs = collect_live_occurrences(best.engine);
        for (OccID occ : occs) {
            PlannerStateDump candidate = best;
            if (!remove_planner_occurrence(candidate, occ)) {
                continue;
            }
            if (accept_planner_candidate(options, best, candidate, targetFailure, oracle)) {
                changed = true;
                break;
            }
        }
        if (changed) {
            continue;
        }

        for (u32 vid = 0; vid < best.engine.V.a.size(); ++vid) {
            PlannerStateDump candidate = best;
            if (!remove_planner_real_vertex(candidate, vid)) {
                continue;
            }
            if (accept_planner_candidate(options, best, candidate, targetFailure, oracle)) {
                changed = true;
                break;
            }
        }
        if (changed) {
            continue;
        }

        for (u32 eid = 0; eid < best.engine.E.a.size(); ++eid) {
            PlannerStateDump candidate = best;
            if (!remove_planner_edge_candidate(candidate, eid)) {
                continue;
            }
            if (accept_planner_candidate(options, best, candidate, targetFailure, oracle)) {
                changed = true;
                break;
            }
        }
    }

    const filesystem::path dir = artifact_subdir(options, "reduced_planner");
    const filesystem::path out = dir / (current_failure_stem() + "_reduced_planner_state.txt");
    save_planner_state_dump(out, best);
    enforce_artifact_retention(options);

    if (best.traceLevel != TraceLevel::NONE || trace_enabled(options)) {
        const TraceLevel traceLevel = options.dumpTrace ? TraceLevel::FULL : best.traceLevel;
        const vector<PlannerTraceEntry> trace = collect_planner_trace_for_dump(best);
        (void)save_trace_artifact(
            options,
            dir,
            current_failure_stem() + "_reduced_planner_trace",
            trace,
            traceLevel,
            best.tracePrefixLength
        );
    }
    return out;
}

LiveStateDump make_reducer_smoke_state() {
    RawEngine RE;
    RawUpdateCtx U;

    const OccID occ31 = new_occ(RE, 9);
    const OccID occ44 = new_occ(RE, 14);
    const RawSkelID parentSid = new_skeleton(RE);
    const RawSkelID childSid = new_skeleton(RE);

    {
        RawSkeletonBuilder B;
        B.V = {
            make_builder_vertex(RawVertexKind::REAL, 6, 0),
            make_builder_vertex(RawVertexKind::REAL, 8, 0),
            make_builder_vertex(RawVertexKind::REAL, 2, 0),
            make_builder_vertex(RawVertexKind::OCC_CENTER, 9, occ31),
        };
        B.E.push_back(make_builder_edge(3, 2, RawEdgeKind::REAL_PORT));
        B.E.push_back(make_builder_edge(2, 0, RawEdgeKind::CORE_REAL));
        B.E.push_back(make_builder_edge(0, 1, RawEdgeKind::CORE_REAL));
        B.allocNbr[occ31] = {};
        B.corePatchLocalEids[occ31] = {1, 2};
        commit_skeleton(RE, parentSid, std::move(B), U);
    }

    {
        RawSkeletonBuilder B;
        B.V = {
            make_builder_vertex(RawVertexKind::REAL, 6, 0),
            make_builder_vertex(RawVertexKind::REAL, 8, 0),
            make_builder_vertex(RawVertexKind::REAL, 11, 0),
            make_builder_vertex(RawVertexKind::OCC_CENTER, 14, occ44),
        };
        B.E.push_back(make_builder_edge(3, 2, RawEdgeKind::REAL_PORT));
        B.E.push_back(make_builder_edge(2, 0, RawEdgeKind::CORE_REAL));
        B.E.push_back(make_builder_edge(2, 1, RawEdgeKind::CORE_REAL));
        B.allocNbr[occ44] = {};
        B.corePatchLocalEids[occ44] = {1, 2};
        commit_skeleton(RE, childSid, std::move(B), U);
    }

    finalize_state(RE);

    LiveStateDump dump;
    dump.engine = RE;
    dump.invocation.primitive = PrimitiveKind::INTEGRATE;
    dump.invocation.occ = occ31;
    dump.invocation.parentSid = parentSid;
    dump.invocation.childSid = childSid;
    dump.invocation.boundaryMap = {
        BoundaryMapEntry{6, 6},
        BoundaryMapEntry{6, 8},
    };
    dump.invocation.keepOcc = {occ31};
    dump.invocation.throughBranches = {0, 1};
    dump.invocation.sequence = {PrimitiveKind::INTEGRATE, PrimitiveKind::JOIN};
    return dump;
}

PlannerStateDump make_planner_reducer_smoke_state() {
    RawEngine RE;
    RawUpdateCtx U;

    const OccID occ31 = new_occ(RE, 9);
    const OccID occ44 = new_occ(RE, 12);
    const RawSkelID sid = new_skeleton(RE);

    RawSkeletonBuilder B;
    B.V = {
        make_builder_vertex(RawVertexKind::REAL, 2, 0),
        make_builder_vertex(RawVertexKind::REAL, 8, 0),
        make_builder_vertex(RawVertexKind::OCC_CENTER, 9, occ31),
        make_builder_vertex(RawVertexKind::REAL, 6, 0),
        make_builder_vertex(RawVertexKind::OCC_CENTER, 12, occ44),
        make_builder_vertex(RawVertexKind::REAL, 10, 0),
    };
    B.E.push_back(make_builder_edge(2, 3, RawEdgeKind::REAL_PORT));
    B.E.push_back(make_builder_edge(0, 3, RawEdgeKind::CORE_REAL));
    B.E.push_back(make_builder_edge(3, 1, RawEdgeKind::CORE_REAL));
    B.E.push_back(make_builder_edge(4, 5, RawEdgeKind::REAL_PORT));
    B.E.push_back(make_builder_edge(0, 5, RawEdgeKind::CORE_REAL));
    B.E.push_back(make_builder_edge(5, 1, RawEdgeKind::CORE_REAL));
    B.E.push_back(make_builder_edge(0, 1, RawEdgeKind::CORE_REAL));
    B.allocNbr[occ31] = {};
    B.allocNbr[occ44] = {};
    B.corePatchLocalEids[occ31] = {1, 2};
    B.corePatchLocalEids[occ44] = {4, 5};
    commit_skeleton(RE, sid, std::move(B), U);
    finalize_state(RE);

    PlannerStateDump dump;
    dump.engine = RE;
    dump.caseName = "planner_reducer_smoke";
    dump.targetOcc = occ31;
    dump.keepOcc = {occ31};
    dump.stepBudget = 4;
    dump.tracePrefixLength = 8;
    dump.traceLevel = TraceLevel::SUMMARY;
    for (int i = 0; i < 12; ++i) {
        UpdJob job;
        job.kind = UpdJobKind::ENSURE_SOLE;
        job.occ = occ31;
        dump.initialQueue.push_back(job);
    }
    return dump;
}
