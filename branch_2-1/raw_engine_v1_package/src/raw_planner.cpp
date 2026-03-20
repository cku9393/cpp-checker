#include "raw_engine/raw_engine.hpp"

#include <algorithm>
#include <deque>
#include <numeric>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

using namespace std;

namespace {

struct SplitPairRank {
    int supportRealCount = 0;
    int supportEdgeCount = 0;
    int componentCount = 0;
    vector<pair<int, int>> componentShapes;
};

struct SplitCandidate {
    pair<Vertex, Vertex> pair;
    RawVID aV = NIL_U32;
    RawVID bV = NIL_U32;
    SplitPairRank rank;
};

struct ProductionSplitChoiceSignature {
    bool failed = false;
    bool stopConditionSatisfied = false;
    string targetIsolateKey;
    string finalStateKey;
    string tracePrefixSummary;
    string failureKey;
};

struct ProductionExactCanonicalKey {
    bool skipped = false;
    string key;
};

struct EvaluatedSplitCandidate {
    SplitCandidate candidate;
    ProductionSplitChoiceSignature fastSignature;
    ProductionSplitChoiceSignature exactSignature;
    bool exactAvailable = false;
};

SplitChoicePolicyStats g_splitChoicePolicyStats;
size_t g_splitChoicePolicyDepth = 0;

constexpr size_t kProductionExactCanonicalOrigCap = 8U;
constexpr u64 kProductionExactCanonicalPermutationCap = 5000000ULL;

bool split_pair_rank_less(const SplitPairRank& lhs, const SplitPairRank& rhs) {
    return tie(lhs.supportRealCount, lhs.supportEdgeCount, lhs.componentCount, lhs.componentShapes) <
           tie(rhs.supportRealCount, rhs.supportEdgeCount, rhs.componentCount, rhs.componentShapes);
}

void merge_split_choice_policy_stats(SplitChoicePolicyStats& dst, const SplitChoicePolicyStats& src) {
    dst.candidateCount += src.candidateCount;
    dst.evalCount += src.evalCount;
    dst.tieCount += src.tieCount;
    dst.multiclassCount += src.multiclassCount;
    dst.fallbackCount += src.fallbackCount;
    for (const auto& [classCount, seen] : src.equivClassCountHistogram) {
        dst.equivClassCountHistogram[classCount] += seen;
    }
}

bool live_occurrence_exists(const RawEngine& RE, OccID occ) {
    return occ < RE.occ.a.size() && RE.occ.a[occ].alive;
}

pair<Vertex, Vertex> ordered_pair(Vertex aOrig, Vertex bOrig) {
    if (aOrig > bOrig) {
        swap(aOrig, bOrig);
    }
    return {aOrig, bOrig};
}

string join_tokens(const vector<string>& values) {
    ostringstream oss;
    for (size_t i = 0; i < values.size(); ++i) {
        if (i != 0U) {
            oss << ',';
        }
        oss << values[i];
    }
    return oss.str();
}

vector<Vertex> collect_live_origs(const RawEngine& RE) {
    unordered_set<Vertex> seen;
    for (u32 vid = 0; vid < RE.V.a.size(); ++vid) {
        if (!RE.V.a[vid].alive) {
            continue;
        }
        const RawVertex& vertex = RE.V.get(vid);
        if (vertex.kind == RawVertexKind::REAL) {
            seen.insert(vertex.orig);
        }
    }
    for (u32 occ = 0; occ < RE.occ.a.size(); ++occ) {
        if (!RE.occ.a[occ].alive) {
            continue;
        }
        seen.insert(RE.occ.get(occ).orig);
    }

    vector<Vertex> out(seen.begin(), seen.end());
    sort(out.begin(), out.end());
    return out;
}

string refined_occurrence_feature(
    const RawEngine& RE,
    const unordered_map<Vertex, size_t>& origIndex,
    const vector<string>& origTokens,
    OccID occ
) {
    const RawOccRecord& O = RE.occ.get(occ);

    vector<string> allocNbr;
    allocNbr.reserve(O.allocNbr.size());
    for (OccID nbr : O.allocNbr) {
        allocNbr.push_back(origTokens[origIndex.at(RE.occ.get(nbr).orig)]);
    }
    sort(allocNbr.begin(), allocNbr.end());

    vector<string> ports;
    for (RawEID eid : RE.V.get(O.centerV).adj) {
        const RawEdge& edge = RE.E.get(eid);
        const RawVID attach = other_end(edge, O.centerV);
        const string attachToken = origTokens[origIndex.at(RE.V.get(attach).orig)];
        ostringstream oss;
        oss << static_cast<int>(edge.kind)
            << ':' << attachToken
            << ':' << edge.br
            << ':' << static_cast<int>(edge.side);
        ports.push_back(oss.str());
    }
    sort(ports.begin(), ports.end());

    vector<string> corePatch;
    corePatch.reserve(O.corePatchEdges.size());
    for (RawEID eid : O.corePatchEdges) {
        const RawEdge& edge = RE.E.get(eid);
        string aToken = origTokens[origIndex.at(RE.V.get(edge.a).orig)];
        string bToken = origTokens[origIndex.at(RE.V.get(edge.b).orig)];
        if (aToken > bToken) {
            swap(aToken, bToken);
        }
        corePatch.push_back(aToken + "-" + bToken);
    }
    sort(corePatch.begin(), corePatch.end());

    ostringstream oss;
    oss << "alloc=[" << join_tokens(allocNbr)
        << "]ports=[" << join_tokens(ports)
        << "]core=[" << join_tokens(corePatch) << ']';
    return oss.str();
}

unordered_map<Vertex, string> build_refined_orig_tokens(const RawEngine& RE) {
    const vector<Vertex> origs = collect_live_origs(RE);
    unordered_map<Vertex, string> result;
    if (origs.empty()) {
        return result;
    }

    unordered_map<Vertex, size_t> origIndex;
    for (size_t i = 0; i < origs.size(); ++i) {
        origIndex.emplace(origs[i], i);
    }

    vector<string> tokens(origs.size());
    vector<string> base(origs.size());
    for (u32 vid = 0; vid < RE.V.a.size(); ++vid) {
        if (!RE.V.a[vid].alive) {
            continue;
        }
        const RawVertex& vertex = RE.V.get(vid);
        if (vertex.kind != RawVertexKind::REAL) {
            continue;
        }
        base[origIndex.at(vertex.orig)] += 'R';
    }
    for (u32 occ = 0; occ < RE.occ.a.size(); ++occ) {
        if (!RE.occ.a[occ].alive) {
            continue;
        }
        base[origIndex.at(RE.occ.get(occ).orig)] += 'O';
    }
    for (size_t i = 0; i < base.size(); ++i) {
        tokens[i] = "b:" + base[i];
    }

    for (size_t round = 0; round < origs.size() + 2U; ++round) {
        vector<vector<string>> realAdj(tokens.size());
        for (u32 eid = 0; eid < RE.E.a.size(); ++eid) {
            if (!RE.E.a[eid].alive) {
                continue;
            }
            const RawEdge& edge = RE.E.get(eid);
            const RawVertex& aVertex = RE.V.get(edge.a);
            const RawVertex& bVertex = RE.V.get(edge.b);
            if (aVertex.kind != RawVertexKind::REAL || bVertex.kind != RawVertexKind::REAL) {
                continue;
            }
            const string aLabel = to_string(static_cast<int>(edge.kind)) + ":" + to_string(edge.br) + ":" +
                to_string(static_cast<int>(edge.side)) + ":" + tokens[origIndex.at(bVertex.orig)];
            const string bLabel = to_string(static_cast<int>(edge.kind)) + ":" + to_string(edge.br) + ":" +
                to_string(static_cast<int>(edge.side)) + ":" + tokens[origIndex.at(aVertex.orig)];
            realAdj[origIndex.at(aVertex.orig)].push_back(aLabel);
            realAdj[origIndex.at(bVertex.orig)].push_back(bLabel);
        }

        vector<vector<string>> occFeatures(tokens.size());
        for (u32 occ = 0; occ < RE.occ.a.size(); ++occ) {
            if (!RE.occ.a[occ].alive) {
                continue;
            }
            const RawOccRecord& O = RE.occ.get(occ);
            occFeatures[origIndex.at(O.orig)].push_back(
                refined_occurrence_feature(RE, origIndex, tokens, occ)
            );
        }

        vector<string> detailed(tokens.size());
        for (size_t i = 0; i < tokens.size(); ++i) {
            sort(realAdj[i].begin(), realAdj[i].end());
            sort(occFeatures[i].begin(), occFeatures[i].end());
            detailed[i] = tokens[i] + "|real=[" + join_tokens(realAdj[i]) + "]|occ=[" + join_tokens(occFeatures[i]) + "]";
        }

        vector<string> compressed(tokens.size());
        vector<string> uniqueDetails = detailed;
        sort(uniqueDetails.begin(), uniqueDetails.end());
        uniqueDetails.erase(std::unique(uniqueDetails.begin(), uniqueDetails.end()), uniqueDetails.end());
        unordered_map<string, string> color;
        for (size_t i = 0; i < uniqueDetails.size(); ++i) {
            color.emplace(uniqueDetails[i], "c" + to_string(i));
        }
        for (size_t i = 0; i < detailed.size(); ++i) {
            compressed[i] = color.at(detailed[i]);
        }

        if (compressed == tokens) {
            break;
        }
        tokens = std::move(compressed);
    }

    for (size_t i = 0; i < origs.size(); ++i) {
        result.emplace(origs[i], tokens[i]);
    }
    return result;
}

vector<Vertex> collect_live_real_origs_exact(const RawEngine& RE) {
    unordered_set<Vertex> seen;
    vector<Vertex> origs;
    for (const auto& slot : RE.V.a) {
        if (!slot.alive || slot.val.kind != RawVertexKind::REAL) {
            continue;
        }
        if (seen.insert(slot.val.orig).second) {
            origs.push_back(slot.val.orig);
        }
    }
    sort(origs.begin(), origs.end());
    return origs;
}

vector<OccID> collect_live_occurrences_exact(const RawEngine& RE) {
    vector<OccID> occs;
    for (u32 occ = 0; occ < RE.occ.a.size(); ++occ) {
        if (RE.occ.a[occ].alive) {
            occs.push_back(occ);
        }
    }
    sort(occs.begin(), occs.end());
    return occs;
}

u64 capped_factorial(size_t n, u64 cap) {
    u64 value = 1ULL;
    for (size_t i = 2U; i <= n; ++i) {
        if (value > cap / static_cast<u64>(i)) {
            return cap + 1ULL;
        }
        value *= static_cast<u64>(i);
    }
    return value;
}

bool production_exact_key_supported(const RawEngine& RE) {
    const vector<Vertex> origs = collect_live_real_origs_exact(RE);
    if (origs.size() > kProductionExactCanonicalOrigCap) {
        return false;
    }
    const vector<OccID> occs = collect_live_occurrences_exact(RE);
    const u64 origPermutations = capped_factorial(origs.size(), kProductionExactCanonicalPermutationCap);
    if (origPermutations > kProductionExactCanonicalPermutationCap) {
        return false;
    }
    const u64 remainingCap = max<u64>(1ULL, kProductionExactCanonicalPermutationCap / max<u64>(1ULL, origPermutations));
    const u64 occPermutations = capped_factorial(occs.size(), remainingCap);
    return occPermutations <= remainingCap;
}

string exact_mapped_orig_token(const unordered_map<Vertex, size_t>& origMap, Vertex orig) {
    return to_string(origMap.at(orig));
}

string exact_mapped_occ_token(const unordered_map<OccID, size_t>& occMap, OccID occ) {
    return string("o") + to_string(occMap.at(occ));
}

string exact_occurrence_descriptor(
    const RawEngine& RE,
    const unordered_map<Vertex, size_t>& origMap,
    const unordered_map<OccID, size_t>& occMap,
    OccID occ
) {
    const RawOccRecord& O = RE.occ.get(occ);

    vector<string> allocNbr;
    allocNbr.reserve(O.allocNbr.size());
    for (OccID nbr : O.allocNbr) {
        allocNbr.push_back(exact_mapped_occ_token(occMap, nbr));
    }
    sort(allocNbr.begin(), allocNbr.end());

    vector<string> ports;
    ports.reserve(RE.V.get(O.centerV).adj.size());
    for (RawEID eid : RE.V.get(O.centerV).adj) {
        const RawEdge& edge = RE.E.get(eid);
        const RawVID attach = other_end(edge, O.centerV);
        ostringstream oss;
        oss << static_cast<int>(edge.kind)
            << ':'
            << exact_mapped_orig_token(origMap, RE.V.get(attach).orig)
            << ':'
            << edge.br
            << ':'
            << static_cast<int>(edge.side);
        ports.push_back(oss.str());
    }
    sort(ports.begin(), ports.end());

    vector<string> corePatch;
    corePatch.reserve(O.corePatchEdges.size());
    for (RawEID eid : O.corePatchEdges) {
        const RawEdge& edge = RE.E.get(eid);
        string aToken = exact_mapped_orig_token(origMap, RE.V.get(edge.a).orig);
        string bToken = exact_mapped_orig_token(origMap, RE.V.get(edge.b).orig);
        if (aToken > bToken) {
            swap(aToken, bToken);
        }
        corePatch.push_back(aToken + "-" + bToken);
    }
    sort(corePatch.begin(), corePatch.end());

    ostringstream oss;
    oss << "occ=" << exact_mapped_occ_token(occMap, occ)
        << ";alloc=[" << join_tokens(allocNbr)
        << "];ports=[" << join_tokens(ports)
        << "];core=[" << join_tokens(corePatch) << ']';
    return oss.str();
}

string exact_vertex_token(
    const RawEngine& RE,
    const unordered_map<Vertex, size_t>& origMap,
    const unordered_map<OccID, size_t>& occMap,
    RawVID vid
) {
    const RawVertex& vertex = RE.V.get(vid);
    if (vertex.kind == RawVertexKind::REAL) {
        return string("R:") + exact_mapped_orig_token(origMap, vertex.orig);
    }
    return string("C:") + exact_mapped_occ_token(occMap, vertex.occ);
}

string exact_edge_token(
    const RawEngine& RE,
    const unordered_map<Vertex, size_t>& origMap,
    const unordered_map<OccID, size_t>& occMap,
    RawEID eid
) {
    const RawEdge& edge = RE.E.get(eid);
    string aToken = exact_vertex_token(RE, origMap, occMap, edge.a);
    string bToken = exact_vertex_token(RE, origMap, occMap, edge.b);
    if (aToken > bToken) {
        swap(aToken, bToken);
    }

    ostringstream oss;
    oss << static_cast<int>(edge.kind)
        << ':'
        << aToken
        << ':'
        << bToken
        << ':'
        << edge.br
        << ':'
        << static_cast<int>(edge.side);
    return oss.str();
}

string exact_state_descriptor(
    const RawEngine& RE,
    const unordered_map<Vertex, size_t>& origMap,
    const unordered_map<OccID, size_t>& occMap
) {
    vector<string> skeletons;
    for (u32 sid = 0; sid < RE.skel.a.size(); ++sid) {
        if (!RE.skel.a[sid].alive) {
            continue;
        }
        const RawSkeleton& S = RE.skel.get(sid);
        vector<string> vertices;
        vector<string> edges;
        vector<string> hostedOcc;
        vertices.reserve(S.verts.size());
        edges.reserve(S.edges.size());
        hostedOcc.reserve(S.hostedOcc.size());

        for (RawVID vid : S.verts) {
            vertices.push_back(exact_vertex_token(RE, origMap, occMap, vid));
        }
        for (RawEID eid : S.edges) {
            edges.push_back(exact_edge_token(RE, origMap, occMap, eid));
        }
        for (OccID occ : S.hostedOcc) {
            hostedOcc.push_back(exact_mapped_occ_token(occMap, occ));
        }
        sort(vertices.begin(), vertices.end());
        sort(edges.begin(), edges.end());
        sort(hostedOcc.begin(), hostedOcc.end());

        skeletons.push_back(
            "V=[" + join_tokens(vertices) + "]E=[" + join_tokens(edges) + "]H=[" + join_tokens(hostedOcc) + ']'
        );
    }
    sort(skeletons.begin(), skeletons.end());

    vector<string> occurrences;
    occurrences.reserve(occMap.size());
    for (const auto& [occ, _] : occMap) {
        occurrences.push_back(exact_occurrence_descriptor(RE, origMap, occMap, occ));
    }
    sort(occurrences.begin(), occurrences.end());

    ostringstream oss;
    oss << "skeletons=[" << join_tokens(skeletons) << "]occ=[" << join_tokens(occurrences) << ']';
    return oss.str();
}

string exact_isolate_descriptor(
    const IsolatePrepared& prep,
    OccID targetOcc,
    const unordered_map<Vertex, size_t>& origMap,
    const unordered_map<OccID, size_t>& occMap
) {
    vector<string> allocNbr;
    allocNbr.reserve(prep.allocNbr.size());
    for (OccID occ : prep.allocNbr) {
        allocNbr.push_back(exact_mapped_occ_token(occMap, occ));
    }
    sort(allocNbr.begin(), allocNbr.end());

    vector<string> ports;
    ports.reserve(prep.ports.size());
    for (const IsoPort& port : prep.ports) {
        ostringstream oss;
        oss << static_cast<int>(port.kind)
            << ':'
            << exact_mapped_orig_token(origMap, port.attachOrig)
            << ':'
            << port.br
            << ':'
            << static_cast<int>(port.side);
        ports.push_back(oss.str());
    }
    sort(ports.begin(), ports.end());

    vector<string> coreVerts;
    coreVerts.reserve(prep.core.orig.size());
    for (Vertex orig : prep.core.orig) {
        coreVerts.push_back(exact_mapped_orig_token(origMap, orig));
    }
    sort(coreVerts.begin(), coreVerts.end());

    vector<string> coreEdges;
    coreEdges.reserve(prep.core.edges.size());
    for (const TinyEdge& edge : prep.core.edges) {
        string aToken = exact_mapped_orig_token(origMap, prep.core.orig[edge.a]);
        string bToken = exact_mapped_orig_token(origMap, prep.core.orig[edge.b]);
        if (aToken > bToken) {
            swap(aToken, bToken);
        }
        coreEdges.push_back(aToken + "-" + bToken);
    }
    sort(coreEdges.begin(), coreEdges.end());

    ostringstream oss;
    oss << "target=" << exact_mapped_occ_token(occMap, targetOcc)
        << ";alloc=[" << join_tokens(allocNbr)
        << "];ports=[" << join_tokens(ports)
        << "];coreV=[" << join_tokens(coreVerts)
        << "];coreE=[" << join_tokens(coreEdges) << ']';
    return oss.str();
}

template <class SignatureFn>
ProductionExactCanonicalKey compute_production_exact_key(
    const RawEngine& RE,
    SignatureFn&& makeDescriptor
) {
    ProductionExactCanonicalKey result;
    if (!production_exact_key_supported(RE)) {
        result.skipped = true;
        return result;
    }

    const vector<Vertex> origs = collect_live_real_origs_exact(RE);
    const vector<OccID> occs = collect_live_occurrences_exact(RE);
    vector<size_t> origPerm(origs.size());
    iota(origPerm.begin(), origPerm.end(), 0U);
    vector<size_t> occPerm(occs.size());
    iota(occPerm.begin(), occPerm.end(), 0U);

    bool haveBest = false;
    do {
        unordered_map<Vertex, size_t> origMap;
        origMap.reserve(origs.size());
        for (size_t pos = 0; pos < origPerm.size(); ++pos) {
            origMap.emplace(origs[origPerm[pos]], pos);
        }

        do {
            unordered_map<OccID, size_t> occMap;
            occMap.reserve(occs.size());
            for (size_t pos = 0; pos < occPerm.size(); ++pos) {
                occMap.emplace(occs[occPerm[pos]], pos);
            }

            const string key = makeDescriptor(origMap, occMap);
            if (!haveBest || key < result.key) {
                result.key = key;
                haveBest = true;
            }
        } while (next_permutation(occPerm.begin(), occPerm.end()));
        iota(occPerm.begin(), occPerm.end(), 0U);
    } while (next_permutation(origPerm.begin(), origPerm.end()));

    return result;
}

ProductionExactCanonicalKey compute_exact_state_key(const RawEngine& RE) {
    return compute_production_exact_key(
        RE,
        [&](const unordered_map<Vertex, size_t>& origMap, const unordered_map<OccID, size_t>& occMap) {
            return exact_state_descriptor(RE, origMap, occMap);
        }
    );
}

ProductionExactCanonicalKey compute_exact_isolate_key(const RawEngine& RE, const IsolatePrepared& prep, OccID targetOcc) {
    return compute_production_exact_key(
        RE,
        [&](const unordered_map<Vertex, size_t>& origMap, const unordered_map<OccID, size_t>& occMap) {
            return exact_isolate_descriptor(prep, targetOcc, origMap, occMap);
        }
    );
}

UpdJob make_ensure_sole_job(OccID occ) {
    UpdJob job;
    job.kind = UpdJobKind::ENSURE_SOLE;
    job.occ = occ;
    return job;
}

UpdJob make_join_pair_job(RawSkelID leftSid, RawSkelID rightSid, Vertex aOrig, Vertex bOrig) {
    UpdJob job;
    job.kind = UpdJobKind::JOIN_PAIR;
    job.leftSid = leftSid;
    job.rightSid = rightSid;
    job.aOrig = aOrig;
    job.bOrig = bOrig;
    return job;
}

UpdJob make_integrate_child_job(RawSkelID parentSid, RawSkelID childSid, Vertex aOrig, Vertex bOrig) {
    UpdJob job;
    job.kind = UpdJobKind::INTEGRATE_CHILD;
    job.parentSid = parentSid;
    job.childSid = childSid;
    job.bm.push_back(BoundaryMapEntry{aOrig, aOrig});
    job.bm.push_back(BoundaryMapEntry{bOrig, bOrig});
    return job;
}

} // namespace

bool queue_has_ensure_sole(const deque<UpdJob>& q, OccID occ) {
    for (const UpdJob& j : q) {
        if (j.kind == UpdJobKind::ENSURE_SOLE && j.occ == occ) {
            return true;
        }
    }
    return false;
}

void enqueue_ensure_sole_once_back(deque<UpdJob>& q, OccID occ) {
    if (!queue_has_ensure_sole(q, occ)) {
        q.push_back(make_ensure_sole_job(occ));
    }
}

void prepend_jobs(deque<UpdJob>& q, vector<UpdJob> jobs) {
    for (auto it = jobs.rbegin(); it != jobs.rend(); ++it) {
        q.push_front(std::move(*it));
    }
}

optional<SplitPairRank> analyze_split_pair_for_support(
    const RawEngine& RE,
    const RawSkeleton& S,
    RawVID aV,
    RawVID bV,
    const vector<RawVID>& support
) {
    const auto is_sep = [&](RawVID v) { return v == aV || v == bV; };
    unordered_map<RawVID, int> comp;
    vector<char> touchA;
    vector<char> touchB;
    vector<int> realCount;
    vector<int> edgeCount;
    int compCnt = 0;

    for (RawVID start : S.verts) {
        if (is_sep(start) || comp.count(start) != 0U) {
            continue;
        }

        const int cid = compCnt++;
        touchA.push_back(false);
        touchB.push_back(false);
        realCount.push_back(0);
        edgeCount.push_back(0);
        deque<RawVID> dq;
        dq.push_back(start);
        comp[start] = cid;

        while (!dq.empty()) {
            const RawVID u = dq.front();
            dq.pop_front();
            if (RE.V.get(u).kind == RawVertexKind::REAL) {
                ++realCount[static_cast<size_t>(cid)];
            }
            for (RawEID eid : RE.V.get(u).adj) {
                const RawEdge& e = RE.E.get(eid);
                const RawVID v = other_end(e, u);
                if (v == aV) {
                    touchA[static_cast<size_t>(cid)] = true;
                    continue;
                }
                if (v == bV) {
                    touchB[static_cast<size_t>(cid)] = true;
                    continue;
                }
                if (comp.count(v) == 0U) {
                    comp[v] = cid;
                    dq.push_back(v);
                }
            }
        }
    }

    int supportComp = -1;
    for (RawVID v : support) {
        if (is_sep(v)) {
            continue;
        }

        const auto it = comp.find(v);
        if (it == comp.end()) {
            return nullopt;
        }
        if (supportComp == -1) {
            supportComp = it->second;
        } else if (supportComp != it->second) {
            return nullopt;
        }
    }
    if (supportComp == -1) {
        return nullopt;
    }

    for (RawEID eid : S.edges) {
        const RawEdge& e = RE.E.get(eid);
        if (is_sep(e.a) || is_sep(e.b)) {
            continue;
        }
        const auto itA = comp.find(e.a);
        const auto itB = comp.find(e.b);
        if (itA == comp.end() || itB == comp.end() || itA->second != itB->second) {
            return nullopt;
        }
        ++edgeCount[static_cast<size_t>(itA->second)];
    }

    for (int cid = 0; cid < compCnt; ++cid) {
        if (!touchA[static_cast<size_t>(cid)] || !touchB[static_cast<size_t>(cid)]) {
            return nullopt;
        }
    }

    if (compCnt < 2) {
        return nullopt;
    }

    SplitPairRank rank;
    rank.supportRealCount = realCount[static_cast<size_t>(supportComp)];
    rank.supportEdgeCount = edgeCount[static_cast<size_t>(supportComp)];
    rank.componentCount = compCnt;
    for (int cid = 0; cid < compCnt; ++cid) {
        rank.componentShapes.push_back({
            realCount[static_cast<size_t>(cid)],
            edgeCount[static_cast<size_t>(cid)],
        });
    }
    sort(rank.componentShapes.begin(), rank.componentShapes.end());
    return rank;
}

bool valid_split_pair_for_support(
    const RawEngine& RE,
    const RawSkeleton& S,
    RawVID aV,
    RawVID bV,
    const vector<RawVID>& support
) {
    return analyze_split_pair_for_support(RE, S, aV, bV, support).has_value();
}

string canonical_occurrence_token(
    const RawEngine& RE,
    const unordered_map<Vertex, string>& origTokens,
    OccID occ
) {
    const RawOccRecord& O = RE.occ.get(occ);

    vector<string> allocNbr;
    allocNbr.reserve(O.allocNbr.size());
    for (OccID nbr : O.allocNbr) {
        allocNbr.push_back(origTokens.at(RE.occ.get(nbr).orig));
    }
    sort(allocNbr.begin(), allocNbr.end());

    vector<string> ports;
    for (RawEID eid : RE.V.get(O.centerV).adj) {
        const RawEdge& edge = RE.E.get(eid);
        const RawVID attach = other_end(edge, O.centerV);
        ostringstream oss;
        oss << static_cast<int>(edge.kind)
            << ':' << origTokens.at(RE.V.get(attach).orig)
            << ':' << edge.br
            << ':' << static_cast<int>(edge.side);
        ports.push_back(oss.str());
    }
    sort(ports.begin(), ports.end());

    vector<string> corePatch;
    for (RawEID eid : O.corePatchEdges) {
        const RawEdge& edge = RE.E.get(eid);
        string aToken = origTokens.at(RE.V.get(edge.a).orig);
        string bToken = origTokens.at(RE.V.get(edge.b).orig);
        if (aToken > bToken) {
            swap(aToken, bToken);
        }
        corePatch.push_back(aToken + "-" + bToken);
    }
    sort(corePatch.begin(), corePatch.end());

    ostringstream oss;
    oss << "orig=" << origTokens.at(O.orig)
        << ";alloc=[" << join_tokens(allocNbr)
        << "];ports=[" << join_tokens(ports)
        << "];core=[" << join_tokens(corePatch) << ']';
    return oss.str();
}

string canonical_vertex_token(
    const RawEngine& RE,
    const unordered_map<Vertex, string>& origTokens,
    const unordered_map<OccID, string>& occTokens,
    RawVID vid
) {
    const RawVertex& vertex = RE.V.get(vid);
    if (vertex.kind == RawVertexKind::REAL) {
        return "R:" + origTokens.at(vertex.orig);
    }
    return "C:" + occTokens.at(vertex.occ);
}

string canonical_edge_token(
    const RawEngine& RE,
    const unordered_map<Vertex, string>& origTokens,
    const unordered_map<OccID, string>& occTokens,
    RawEID eid
) {
    const RawEdge& edge = RE.E.get(eid);
    string aToken = canonical_vertex_token(RE, origTokens, occTokens, edge.a);
    string bToken = canonical_vertex_token(RE, origTokens, occTokens, edge.b);
    if (aToken > bToken) {
        swap(aToken, bToken);
    }
    ostringstream oss;
    oss << static_cast<int>(edge.kind)
        << ':' << aToken
        << ':' << bToken
        << ':' << edge.br
        << ':' << static_cast<int>(edge.side);
    return oss.str();
}

string canonical_state_key(const RawEngine& RE) {
    const unordered_map<Vertex, string> origTokens = build_refined_orig_tokens(RE);
    unordered_map<OccID, string> occTokens;
    for (u32 occ = 0; occ < RE.occ.a.size(); ++occ) {
        if (!RE.occ.a[occ].alive) {
            continue;
        }
        occTokens.emplace(occ, canonical_occurrence_token(RE, origTokens, occ));
    }

    vector<string> skeletons;
    for (u32 sid = 0; sid < RE.skel.a.size(); ++sid) {
        if (!RE.skel.a[sid].alive) {
            continue;
        }
        const RawSkeleton& S = RE.skel.get(sid);
        vector<string> vertices;
        vector<string> edges;
        vector<string> hostedOcc;
        for (RawVID vid : S.verts) {
            vertices.push_back(canonical_vertex_token(RE, origTokens, occTokens, vid));
        }
        for (RawEID eid : S.edges) {
            edges.push_back(canonical_edge_token(RE, origTokens, occTokens, eid));
        }
        for (OccID occ : S.hostedOcc) {
            hostedOcc.push_back(occTokens.at(occ));
        }
        sort(vertices.begin(), vertices.end());
        sort(edges.begin(), edges.end());
        sort(hostedOcc.begin(), hostedOcc.end());
        skeletons.push_back(
            "V=[" + join_tokens(vertices) + "]E=[" + join_tokens(edges) + "]H=[" + join_tokens(hostedOcc) + ']'
        );
    }
    sort(skeletons.begin(), skeletons.end());

    vector<string> occurrences;
    for (const auto& [_, token] : occTokens) {
        occurrences.push_back(token);
    }
    sort(occurrences.begin(), occurrences.end());

    ostringstream oss;
    oss << "skeletons=[" << join_tokens(skeletons) << "]occ=[" << join_tokens(occurrences) << ']';
    return oss.str();
}

string canonical_isolate_key(const RawEngine& RE, const IsolatePrepared& prep) {
    const unordered_map<Vertex, string> origTokens = build_refined_orig_tokens(RE);

    vector<string> allocNbr;
    allocNbr.reserve(prep.allocNbr.size());
    for (OccID nbr : prep.allocNbr) {
        allocNbr.push_back(origTokens.at(RE.occ.get(nbr).orig));
    }
    sort(allocNbr.begin(), allocNbr.end());

    vector<string> ports;
    for (const IsoPort& port : prep.ports) {
        ostringstream oss;
        oss << static_cast<int>(port.kind)
            << ':' << origTokens.at(port.attachOrig)
            << ':' << port.br
            << ':' << static_cast<int>(port.side);
        ports.push_back(oss.str());
    }
    sort(ports.begin(), ports.end());

    vector<string> coreEdges;
    for (const TinyEdge& edge : prep.core.edges) {
        string aToken = origTokens.at(prep.core.orig[edge.a]);
        string bToken = origTokens.at(prep.core.orig[edge.b]);
        if (aToken > bToken) {
            swap(aToken, bToken);
        }
        coreEdges.push_back(aToken + "-" + bToken);
    }
    sort(coreEdges.begin(), coreEdges.end());

    vector<string> coreVerts;
    coreVerts.reserve(prep.core.orig.size());
    for (Vertex orig : prep.core.orig) {
        coreVerts.push_back(origTokens.at(orig));
    }
    sort(coreVerts.begin(), coreVerts.end());

    ostringstream oss;
    oss << "orig=" << origTokens.at(prep.orig)
        << ";alloc=[" << join_tokens(allocNbr)
        << "];ports=[" << join_tokens(ports)
        << "];coreV=[" << join_tokens(coreVerts)
        << "];coreE=[" << join_tokens(coreEdges) << ']';
    return oss.str();
}

vector<SplitCandidate> enumerate_split_candidates(const RawEngine& RE, OccID occ) {
    vector<SplitCandidate> out;
    if (!live_occurrence_exists(RE, occ)) {
        return out;
    }

    const RawOccRecord& O = RE.occ.get(occ);
    if (!RE.skel.a[O.hostSkel].alive) {
        return out;
    }
    const RawSkeleton& S = RE.skel.get(O.hostSkel);
    if (S.hostedOcc.size() != 1U || S.hostedOcc[0] != occ) {
        return out;
    }

    const vector<RawVID> support = collect_support_vertices(RE, occ);
    vector<pair<Vertex, RawVID>> realVertices;
    for (RawVID vid : S.verts) {
        const RawVertex& vertex = RE.V.get(vid);
        if (vertex.kind == RawVertexKind::REAL) {
            realVertices.push_back({vertex.orig, vid});
        }
    }

    for (size_t i = 0; i < realVertices.size(); ++i) {
        for (size_t j = i + 1U; j < realVertices.size(); ++j) {
            const optional<SplitPairRank> rank =
                analyze_split_pair_for_support(RE, S, realVertices[i].second, realVertices[j].second, support);
            if (!rank.has_value()) {
                continue;
            }
            SplitCandidate candidate;
            candidate.pair = ordered_pair(realVertices[i].first, realVertices[j].first);
            candidate.aV = realVertices[i].second;
            candidate.bV = realVertices[j].second;
            candidate.rank = *rank;
            out.push_back(std::move(candidate));
        }
    }
    return out;
}

bool has_split_candidate_for_support(const RawEngine& RE, OccID occ) {
    return !enumerate_split_candidates(RE, occ).empty();
}

string split_choice_signature_key(const ProductionSplitChoiceSignature& signature) {
    ostringstream oss;
    oss << "failed=" << (signature.failed ? 1 : 0)
        << ";stop=" << (signature.stopConditionSatisfied ? 1 : 0)
        << ";target=" << signature.targetIsolateKey
        << ";final=" << signature.finalStateKey
        << ";trace=" << signature.tracePrefixSummary
        << ";detail=" << signature.failureKey;
    return oss.str();
}

bool split_choice_signature_less(const ProductionSplitChoiceSignature& lhs, const ProductionSplitChoiceSignature& rhs) {
    return tie(
               lhs.failed,
               lhs.stopConditionSatisfied,
               lhs.targetIsolateKey,
               lhs.finalStateKey,
               lhs.tracePrefixSummary,
               lhs.failureKey
           ) <
           tie(
               rhs.failed,
               rhs.stopConditionSatisfied,
               rhs.targetIsolateKey,
               rhs.finalStateKey,
               rhs.tracePrefixSummary,
               rhs.failureKey
           );
}

string split_candidate_order_key(const RawEngine& RE, const SplitCandidate& candidate) {
    const unordered_map<Vertex, string> origTokens = build_refined_orig_tokens(RE);
    string aToken = origTokens.at(candidate.pair.first);
    string bToken = origTokens.at(candidate.pair.second);
    if (aToken > bToken) {
        swap(aToken, bToken);
    }
    ostringstream oss;
    oss << candidate.rank.supportRealCount
        << ':' << candidate.rank.supportEdgeCount
        << ':' << candidate.rank.componentCount
        << ':' << aToken
        << ':' << bToken;
    for (const auto& shape : candidate.rank.componentShapes) {
        oss << ':' << shape.first << '/' << shape.second;
    }
    return oss.str();
}

bool planner_stop_condition_satisfied_without_policy(const RawEngine& RE, OccID targetOcc) {
    if (!live_occurrence_exists(RE, targetOcc)) {
        return false;
    }
    const RawOccRecord& O = RE.occ.get(targetOcc);
    const RawSkeleton& S = RE.skel.get(O.hostSkel);
    if (S.hostedOcc.size() != 1U || S.hostedOcc[0] != targetOcc) {
        return false;
    }
    if (has_split_candidate_for_support(RE, targetOcc)) {
        return false;
    }
    assert_occ_patch_consistent(RE, targetOcc);
    return true;
}

bool child_contains_occ(const SplitChildInfo& c, OccID occ) {
    for (OccID x : c.hostedOcc) {
        if (x == occ) {
            return true;
        }
    }
    return false;
}

bool child_intersects_keep_occ(const SplitChildInfo& c, const RawPlannerCtx& ctx) {
    if (ctx.keepOcc.empty()) {
        return child_contains_occ(c, ctx.targetOcc);
    }
    for (OccID x : c.hostedOcc) {
        if (ctx.keepOcc.count(x) != 0U) {
            return true;
        }
    }
    return false;
}

int choose_anchor_child(const SplitSeparationPairResult& res, const RawPlannerCtx& ctx) {
    for (int i = 0; i < static_cast<int>(res.child.size()); ++i) {
        if (child_contains_occ(res.child[static_cast<size_t>(i)], ctx.targetOcc)) {
            return i;
        }
    }
    for (int i = 0; i < static_cast<int>(res.child.size()); ++i) {
        if (child_intersects_keep_occ(res.child[static_cast<size_t>(i)], ctx)) {
            return i;
        }
    }
    for (int i = 0; i < static_cast<int>(res.child.size()); ++i) {
        if (!res.child[static_cast<size_t>(i)].boundaryOnly) {
            return i;
        }
    }
    return res.child.empty() ? -1 : 0;
}

RawUpdaterHooks make_basic_hooks_for_target(const RawPlannerCtx& ctx) {
    RawUpdaterHooks hooks;

    hooks.afterSplit = [ctx](const SplitSeparationPairResult& res, deque<UpdJob>& q) {
        if (res.child.empty()) {
            return;
        }

        const int anchorIdx = choose_anchor_child(res, ctx);
        assert(anchorIdx >= 0);
        const RawSkelID anchorSid = res.child[static_cast<size_t>(anchorIdx)].sid;
        vector<UpdJob> jobs;

        for (int i = 0; i < static_cast<int>(res.child.size()); ++i) {
            if (i == anchorIdx) {
                continue;
            }
            const SplitChildInfo& c = res.child[static_cast<size_t>(i)];
            if (c.boundaryOnly) {
                jobs.push_back(make_integrate_child_job(anchorSid, c.sid, res.aOrig, res.bOrig));
            }
        }

        for (int i = 0; i < static_cast<int>(res.child.size()); ++i) {
            if (i == anchorIdx) {
                continue;
            }
            const SplitChildInfo& c = res.child[static_cast<size_t>(i)];
            if (!c.boundaryOnly && child_intersects_keep_occ(c, ctx)) {
                jobs.push_back(make_join_pair_job(anchorSid, c.sid, res.aOrig, res.bOrig));
            }
        }

        jobs.push_back(make_ensure_sole_job(ctx.targetOcc));
        prepend_jobs(q, std::move(jobs));
    };

    hooks.afterJoin = [ctx](const JoinSeparationPairResult&, deque<UpdJob>& q) {
        enqueue_ensure_sole_once_back(q, ctx.targetOcc);
    };

    hooks.afterIntegrate = [ctx](const IntegrateResult&, deque<UpdJob>& q) {
        enqueue_ensure_sole_once_back(q, ctx.targetOcc);
    };

    return hooks;
}

void reset_split_choice_policy_stats() {
    g_splitChoicePolicyStats = {};
}

const SplitChoicePolicyStats& split_choice_policy_stats() {
    return g_splitChoicePolicyStats;
}

string planner_fast_canonical_state_key(const RawEngine& RE) {
    return canonical_state_key(RE);
}

string planner_fast_canonical_isolate_key(const RawEngine& RE, const IsolatePrepared& prep) {
    return canonical_isolate_key(RE, prep);
}

void run_raw_local_updater_queue(
    RawEngine& RE,
    const RawPlannerCtx& ctx,
    RawUpdateCtx& U,
    deque<UpdJob>& q,
    RawUpdaterHooks* hooks,
    const RawUpdaterRunOptions& runOptions
) {
    size_t steps = 0;
    while (!q.empty()) {
        ++steps;
        if (runOptions.stepBudget != 0U && steps > runOptions.stepBudget) {
            throw runtime_error("raw updater step budget exceeded");
        }

        UpdJob job = std::move(q.front());
        q.pop_front();

        switch (job.kind) {
            case UpdJobKind::ENSURE_SOLE: {
                const RawOccRecord& O = RE.occ.get(job.occ);
                const RawSkeleton& S = RE.skel.get(O.hostSkel);

                if (S.hostedOcc.size() != 1U || S.hostedOcc[0] != job.occ) {
                    isolate_vertex(RE, O.hostSkel, job.occ, U);
                    q.push_front(make_ensure_sole_job(job.occ));
                    break;
                }

                const optional<pair<Vertex, Vertex>> sep =
                    discover_split_pair_from_support(RE, job.occ, &ctx, runOptions);
                if (!sep.has_value()) {
                    break;
                }

                const SplitSeparationPairResult res =
                    split_separation_pair(RE, O.hostSkel, sep->first, sep->second, U);
                if (hooks != nullptr && hooks->afterSplit) {
                    hooks->afterSplit(res, q);
                } else {
                    q.push_front(make_ensure_sole_job(job.occ));
                }
                break;
            }

            case UpdJobKind::JOIN_PAIR: {
                const JoinSeparationPairResult res = join_separation_pair(
                    RE,
                    job.leftSid,
                    job.rightSid,
                    job.aOrig,
                    job.bOrig,
                    U
                );
                if (hooks != nullptr && hooks->afterJoin) {
                    hooks->afterJoin(res, q);
                } else {
                    enqueue_ensure_sole_once_back(q, ctx.targetOcc);
                }
                break;
            }

            case UpdJobKind::INTEGRATE_CHILD: {
                const IntegrateResult res = integrate_skeleton(RE, job.parentSid, job.childSid, job.bm, U);
                if (hooks != nullptr && hooks->afterIntegrate) {
                    hooks->afterIntegrate(res, q);
                } else {
                    enqueue_ensure_sole_once_back(q, ctx.targetOcc);
                }
                break;
            }
        }
    }
}

EvaluatedSplitCandidate evaluate_split_candidate(
    const RawEngine& RE,
    OccID occ,
    const RawPlannerCtx& ctx,
    const RawUpdaterRunOptions& runOptions,
    const SplitCandidate& candidate
) {
    EvaluatedSplitCandidate evaluated;
    evaluated.candidate = candidate;
    try {
        RawEngine working = RE;
        RawUpdateCtx update;
        const RawSkelID sid = working.occ.get(occ).hostSkel;
        const SplitSeparationPairResult result =
            split_separation_pair(working, sid, candidate.pair.first, candidate.pair.second, update);

        deque<UpdJob> q;
        RawUpdaterHooks hooks = make_basic_hooks_for_target(ctx);
        hooks.afterSplit(result, q);
        run_raw_local_updater_queue(working, ctx, update, q, &hooks, runOptions);

        evaluated.fastSignature.finalStateKey = canonical_state_key(working);
        evaluated.fastSignature.stopConditionSatisfied =
            planner_stop_condition_satisfied_without_policy(working, ctx.targetOcc);
        if (!live_occurrence_exists(working, ctx.targetOcc)) {
            evaluated.fastSignature.failed = true;
            evaluated.fastSignature.failureKey = "target_occurrence_disappeared";
            evaluated.exactSignature = evaluated.fastSignature;
            evaluated.exactAvailable = true;
            return evaluated;
        }

        const RawOccRecord& targetOcc = working.occ.get(ctx.targetOcc);
        const IsolatePrepared targetPrepare =
            prepare_isolate_neighborhood(working, targetOcc.hostSkel, ctx.targetOcc);
        evaluated.fastSignature.targetIsolateKey = canonical_isolate_key(
            working,
            targetPrepare
        );

        const ProductionExactCanonicalKey exactStateKey = compute_exact_state_key(working);
        const ProductionExactCanonicalKey exactIsolateKey =
            compute_exact_isolate_key(working, targetPrepare, ctx.targetOcc);
        if (!exactStateKey.skipped && !exactIsolateKey.skipped) {
            evaluated.exactSignature.failed = false;
            evaluated.exactSignature.stopConditionSatisfied = evaluated.fastSignature.stopConditionSatisfied;
            evaluated.exactSignature.targetIsolateKey = exactIsolateKey.key;
            evaluated.exactSignature.finalStateKey = exactStateKey.key;
            evaluated.exactAvailable = true;
        }
    } catch (const exception& ex) {
        evaluated.fastSignature.failed = true;
        evaluated.fastSignature.failureKey = ex.what();
        evaluated.exactSignature = evaluated.fastSignature;
        evaluated.exactAvailable = true;
    }
    return evaluated;
}

optional<pair<Vertex, Vertex>> discover_split_pair_from_support(
    const RawEngine& RE,
    OccID occ,
    const RawPlannerCtx* plannerCtx,
    const RawUpdaterRunOptions& runOptions,
    SplitChoicePolicyStats* eventStats
) {
    if (!live_occurrence_exists(RE, occ)) {
        return nullopt;
    }

    const RawOccRecord& O = RE.occ.get(occ);
    const RawSkeleton& S = RE.skel.get(O.hostSkel);
    if (S.hostedOcc.size() != 1U || S.hostedOcc[0] != occ) {
        return nullopt;
    }

    RawPlannerCtx fallbackCtx;
    fallbackCtx.targetOcc = occ;
    const RawPlannerCtx& ctx = (plannerCtx != nullptr ? *plannerCtx : fallbackCtx);

    SplitChoicePolicyStats localStats;
    const vector<SplitCandidate> candidates = enumerate_split_candidates(RE, occ);
    localStats.candidateCount += candidates.size();
    if (candidates.size() >= 2U) {
        ++localStats.tieCount;
    }

    struct DepthGuard {
        bool topLevel = false;
        DepthGuard() {
            topLevel = (g_splitChoicePolicyDepth == 0U);
            ++g_splitChoicePolicyDepth;
        }
        ~DepthGuard() {
            --g_splitChoicePolicyDepth;
        }
    } depthGuard;

    auto flush_stats = [&](const SplitChoicePolicyStats& stats) {
        if (eventStats != nullptr) {
            merge_split_choice_policy_stats(*eventStats, stats);
        }
        if (depthGuard.topLevel) {
            merge_split_choice_policy_stats(g_splitChoicePolicyStats, stats);
        }
    };

    if (candidates.empty()) {
        flush_stats(localStats);
        return nullopt;
    }
    if (candidates.size() == 1U) {
        localStats.equivClassCountHistogram[1U] += 1U;
        flush_stats(localStats);
        return candidates.front().pair;
    }

    if (runOptions.maxSplitChoiceEval != 0U && candidates.size() > runOptions.maxSplitChoiceEval) {
        ++localStats.fallbackCount;
        const SplitCandidate* bestCandidate = nullptr;
        string bestKey;
        for (const SplitCandidate& candidate : candidates) {
            const string key = split_candidate_order_key(RE, candidate);
            if (bestCandidate == nullptr ||
                split_pair_rank_less(candidate.rank, bestCandidate->rank) ||
                (!split_pair_rank_less(bestCandidate->rank, candidate.rank) && key < bestKey)) {
                bestCandidate = &candidate;
                bestKey = key;
            }
        }
        flush_stats(localStats);
        return bestCandidate == nullptr ? nullopt : optional<pair<Vertex, Vertex>>(bestCandidate->pair);
    }

    vector<EvaluatedSplitCandidate> evaluated;
    evaluated.reserve(candidates.size());
    bool exactFallback = false;
    for (const SplitCandidate& candidate : candidates) {
        EvaluatedSplitCandidate entry = evaluate_split_candidate(RE, occ, ctx, runOptions, candidate);
        if (runOptions.splitChoicePolicy == RawSplitChoicePolicyMode::EXACT_SHADOW && !entry.exactAvailable) {
            exactFallback = true;
        }
        evaluated.push_back(std::move(entry));
    }
    localStats.evalCount += evaluated.size();

    const bool useExactPolicy =
        runOptions.splitChoicePolicy == RawSplitChoicePolicyMode::EXACT_SHADOW && !exactFallback;
    if (runOptions.splitChoicePolicy == RawSplitChoicePolicyMode::EXACT_SHADOW && exactFallback) {
        ++localStats.fallbackCount;
    }

    unordered_map<string, size_t> classCounts;
    for (const EvaluatedSplitCandidate& entry : evaluated) {
        ++classCounts[split_choice_signature_key(useExactPolicy ? entry.exactSignature : entry.fastSignature)];
    }
    localStats.equivClassCountHistogram[classCounts.size()] += 1U;
    if (classCounts.size() > 1U) {
        ++localStats.multiclassCount;
    }

    const EvaluatedSplitCandidate* best = nullptr;
    string bestPairKey;
    for (const EvaluatedSplitCandidate& entry : evaluated) {
        const string pairKey = split_candidate_order_key(RE, entry.candidate);
        if (best == nullptr ||
            split_choice_signature_less(
                useExactPolicy ? entry.exactSignature : entry.fastSignature,
                useExactPolicy ? best->exactSignature : best->fastSignature
            ) ||
            (!split_choice_signature_less(
                useExactPolicy ? best->exactSignature : best->fastSignature,
                useExactPolicy ? entry.exactSignature : entry.fastSignature
            ) && pairKey < bestPairKey)) {
            best = &entry;
            bestPairKey = pairKey;
        }
    }

    flush_stats(localStats);
    return best == nullptr ? nullopt : optional<pair<Vertex, Vertex>>(best->candidate.pair);
}

optional<pair<Vertex, Vertex>> discover_split_pair_from_support(const RawEngine& RE, OccID occ) {
    return discover_split_pair_from_support(RE, occ, nullptr, RawUpdaterRunOptions{}, nullptr);
}

void run_raw_local_updater(
    RawEngine& RE,
    OccID targetOcc,
    RawUpdateCtx& U,
    RawUpdaterHooks* hooks,
    const RawUpdaterRunOptions& runOptions
) {
    reset_split_choice_policy_stats();
    deque<UpdJob> q;
    q.push_back(make_ensure_sole_job(targetOcc));
    RawPlannerCtx ctx;
    ctx.targetOcc = targetOcc;
    run_raw_local_updater_queue(RE, ctx, U, q, hooks, runOptions);
}
