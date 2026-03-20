#include "exact_canonicalizer.hpp"

#include <algorithm>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;

namespace {

vector<Vertex> collect_live_real_origs(const RawEngine& RE) {
    unordered_set<Vertex> seen;
    vector<Vertex> origs;
    for (const auto& slot : RE.V.a) {
        if (!slot.alive) {
            continue;
        }
        if (slot.val.kind != RawVertexKind::REAL) {
            continue;
        }
        if (seen.insert(slot.val.orig).second) {
            origs.push_back(slot.val.orig);
        }
    }
    sort(origs.begin(), origs.end());
    return origs;
}

vector<OccID> collect_live_occurrences_local(const RawEngine& RE) {
    vector<OccID> occs;
    for (u32 occ = 0; occ < RE.occ.a.size(); ++occ) {
        if (RE.occ.a[occ].alive) {
            occs.push_back(occ);
        }
    }
    sort(occs.begin(), occs.end());
    return occs;
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

string mapped_orig_token(const unordered_map<Vertex, size_t>& origMap, Vertex orig) {
    if (orig == NIL_U32) {
        return "nil";
    }
    const auto it = origMap.find(orig);
    if (it != origMap.end()) {
        return to_string(it->second);
    }
    return string("stale:") + to_string(orig);
}

string mapped_occ_token(const unordered_map<OccID, size_t>& occMap, OccID occ) {
    return string("o") + to_string(occMap.at(occ));
}

OccID lookup_occ_by_orig(const RawEngine& RE, Vertex occOrig) {
    const auto it = RE.occOfOrig.find(occOrig);
    if (it == RE.occOfOrig.end() || it->second.empty()) {
        throw out_of_range("missing occurrence for orig");
    }
    return it->second.front();
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
        allocNbr.push_back(mapped_occ_token(occMap, nbr));
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
            << mapped_orig_token(origMap, RE.V.get(attach).orig)
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
        string aToken = mapped_orig_token(origMap, RE.V.get(edge.a).orig);
        string bToken = mapped_orig_token(origMap, RE.V.get(edge.b).orig);
        if (aToken > bToken) {
            swap(aToken, bToken);
        }
        corePatch.push_back(aToken + "-" + bToken);
    }
    sort(corePatch.begin(), corePatch.end());

    ostringstream oss;
    oss << "occ=" << mapped_occ_token(occMap, occ)
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
    const RawVertex& V = RE.V.get(vid);
    if (V.kind == RawVertexKind::REAL) {
        return string("R:") + mapped_orig_token(origMap, V.orig);
    }
    return string("C:") + mapped_occ_token(occMap, V.occ);
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

unordered_map<RawSkelID, string> exact_skeleton_descriptors(
    const RawEngine& RE,
    const unordered_map<Vertex, size_t>& origMap,
    const unordered_map<OccID, size_t>& occMap,
    vector<string>* sortedDescriptors
) {
    unordered_map<RawSkelID, string> bySid;
    vector<string> descriptors;
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
            hostedOcc.push_back(mapped_occ_token(occMap, occ));
        }
        sort(vertices.begin(), vertices.end());
        sort(edges.begin(), edges.end());
        sort(hostedOcc.begin(), hostedOcc.end());

        const string descriptor =
            "V=[" + join_tokens(vertices) + "]E=[" + join_tokens(edges) + "]H=[" + join_tokens(hostedOcc) + ']';
        bySid.emplace(sid, descriptor);
        descriptors.push_back(descriptor);
    }
    sort(descriptors.begin(), descriptors.end());
    if (sortedDescriptors != nullptr) {
        *sortedDescriptors = std::move(descriptors);
    }
    return bySid;
}

string exact_state_descriptor(
    const RawEngine& RE,
    const unordered_map<Vertex, size_t>& origMap,
    const unordered_map<OccID, size_t>& occMap
) {
    vector<string> skeletons;
    const unordered_map<RawSkelID, string> skelBySid =
        exact_skeleton_descriptors(RE, origMap, occMap, &skeletons);
    (void)skelBySid;

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

string exact_explorer_descriptor(
    const ExhaustiveScenario& scenario,
    const unordered_map<Vertex, size_t>& origMap,
    const unordered_map<OccID, size_t>& occMap
) {
    vector<string> skeletons;
    const unordered_map<RawSkelID, string> skelBySid =
        exact_skeleton_descriptors(scenario.RE, origMap, occMap, &skeletons);

    vector<string> keep;
    for (OccID occ : scenario.ctx.keepOcc) {
        keep.push_back(mapped_occ_token(occMap, occ));
    }
    sort(keep.begin(), keep.end());

    vector<string> queue;
    queue.reserve(scenario.initialQueue.size());
    for (const UpdJob& job : scenario.initialQueue) {
        ostringstream oss;
        oss << static_cast<int>(job.kind);
        switch (job.kind) {
            case UpdJobKind::ENSURE_SOLE:
                oss << ':' << mapped_occ_token(occMap, job.occ);
                break;
            case UpdJobKind::JOIN_PAIR:
                oss << ':' << skelBySid.at(job.leftSid)
                    << ':' << skelBySid.at(job.rightSid)
                    << ':' << mapped_orig_token(origMap, job.aOrig)
                    << ':' << mapped_orig_token(origMap, job.bOrig);
                break;
            case UpdJobKind::INTEGRATE_CHILD:
                oss << ':' << skelBySid.at(job.parentSid)
                    << ':' << skelBySid.at(job.childSid)
                    << ':' << mapped_orig_token(origMap, job.aOrig)
                    << ':' << mapped_orig_token(origMap, job.bOrig);
                for (const BoundaryMapEntry& entry : job.bm) {
                    oss << ':'
                        << mapped_orig_token(origMap, entry.childOrig)
                        << "->"
                        << mapped_orig_token(origMap, entry.parentOrig);
                }
                break;
        }
        queue.push_back(oss.str());
    }

    ostringstream oss;
    oss << "family=" << exhaustive_family_name(scenario.family)
        << ";target=" << mapped_occ_token(occMap, scenario.ctx.targetOcc)
        << ";keep=[" << join_tokens(keep)
        << "];queue=[" << join_tokens(queue)
        << "];state=" << exact_state_descriptor(scenario.RE, origMap, occMap);
    return oss.str();
}

string exact_isolate_descriptor(
    const RawEngine& RE,
    const IsolatePrepared& prep,
    const unordered_map<Vertex, size_t>& origMap,
    const unordered_map<OccID, size_t>& occMap
) {
    vector<string> allocNbr;
    allocNbr.reserve(prep.allocNbr.size());
    for (OccID occ : prep.allocNbr) {
        allocNbr.push_back(mapped_occ_token(occMap, occ));
    }
    sort(allocNbr.begin(), allocNbr.end());

    vector<string> ports;
    ports.reserve(prep.ports.size());
    for (const IsoPort& port : prep.ports) {
        ostringstream oss;
        oss << static_cast<int>(port.kind)
            << ':'
            << mapped_orig_token(origMap, port.attachOrig)
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
        coreVerts.push_back(mapped_orig_token(origMap, orig));
    }
    sort(coreVerts.begin(), coreVerts.end());

    vector<string> coreEdges;
    coreEdges.reserve(prep.core.edges.size());
    for (const TinyEdge& edge : prep.core.edges) {
        string aToken = mapped_orig_token(origMap, prep.core.orig[edge.a]);
        string bToken = mapped_orig_token(origMap, prep.core.orig[edge.b]);
        if (aToken > bToken) {
            swap(aToken, bToken);
        }
        coreEdges.push_back(aToken + "-" + bToken);
    }
    sort(coreEdges.begin(), coreEdges.end());

    ostringstream oss;
    oss << "target=" << mapped_occ_token(occMap, lookup_occ_by_orig(RE, prep.orig))
        << ";alloc=[" << join_tokens(allocNbr)
        << "];ports=[" << join_tokens(ports)
        << "];coreV=[" << join_tokens(coreVerts)
        << "];coreE=[" << join_tokens(coreEdges) << ']';
    return oss.str();
}

template <class DescriptorFn>
ExactCanonicalKey compute_exact_key_impl(
    const RawEngine& RE,
    size_t cap,
    DescriptorFn&& describe
) {
    ExactCanonicalKey result;
    const vector<Vertex> origs = collect_live_real_origs(RE);
    const vector<OccID> occs = collect_live_occurrences_local(RE);
    result.origCount = origs.size();
    result.occurrenceCount = occs.size();
    if (cap == 0U || origs.size() > cap) {
        result.skipped = true;
        return result;
    }

    vector<size_t> origPerm(origs.size());
    iota(origPerm.begin(), origPerm.end(), 0U);
    vector<size_t> occPerm(occs.size());
    iota(occPerm.begin(), occPerm.end(), 0U);

    bool haveBest = false;
    do {
        unordered_map<Vertex, size_t> origMap;
        for (size_t pos = 0; pos < origPerm.size(); ++pos) {
            origMap.emplace(origs[origPerm[pos]], pos);
        }

        do {
            unordered_map<OccID, size_t> occMap;
            for (size_t pos = 0; pos < occPerm.size(); ++pos) {
                occMap.emplace(occs[occPerm[pos]], pos);
            }

            const string descriptor = describe(origMap, occMap);
            if (!haveBest || descriptor < result.key) {
                result.key = descriptor;
                haveBest = true;
            }
            ++result.permutationCount;
        } while (next_permutation(occPerm.begin(), occPerm.end()));
        iota(occPerm.begin(), occPerm.end(), 0U);
    } while (next_permutation(origPerm.begin(), origPerm.end()));

    return result;
}

} // namespace

ExactCanonicalKey compute_exact_state_canonical_key(const RawEngine& RE, size_t cap) {
    return compute_exact_key_impl(
        RE,
        cap,
        [&](const unordered_map<Vertex, size_t>& origMap, const unordered_map<OccID, size_t>& occMap) {
            return exact_state_descriptor(RE, origMap, occMap);
        }
    );
}

ExactCanonicalKey compute_exact_explorer_canonical_key(const ExhaustiveScenario& scenario, size_t cap) {
    return compute_exact_key_impl(
        scenario.RE,
        cap,
        [&](const unordered_map<Vertex, size_t>& origMap, const unordered_map<OccID, size_t>& occMap) {
            return exact_explorer_descriptor(scenario, origMap, occMap);
        }
    );
}

ExactCanonicalKey compute_exact_isolate_canonical_key(
    const RawEngine& RE,
    const IsolatePrepared& prep,
    size_t cap
) {
    return compute_exact_key_impl(
        RE,
        cap,
        [&](const unordered_map<Vertex, size_t>& origMap, const unordered_map<OccID, size_t>& occMap) {
            return exact_isolate_descriptor(RE, prep, origMap, occMap);
        }
    );
}

ExactCanonicalKey compute_exact_split_choice_canonical_key(
    const RawEngine& RE,
    const IsolatePrepared* prep,
    bool stopConditionSatisfied,
    const string& failureKey,
    size_t cap
) {
    return compute_exact_key_impl(
        RE,
        cap,
        [&](const unordered_map<Vertex, size_t>& origMap, const unordered_map<OccID, size_t>& occMap) {
            ostringstream oss;
            oss << "failed=" << (failureKey.empty() ? 0 : 1)
                << ";stop=" << (stopConditionSatisfied ? 1 : 0)
                << ";target=";
            if (prep != nullptr) {
                oss << exact_isolate_descriptor(RE, *prep, origMap, occMap);
            }
            oss << ";final=" << exact_state_descriptor(RE, origMap, occMap)
                << ";detail=" << failureKey;
            return oss.str();
        }
    );
}

bool should_sample_exact_canonical(const TestOptions& options, size_t ordinal) {
    if (options.exactCanonicalCap == 0U) {
        return false;
    }
    return ordinal % options.exactCanonicalSampleRate == 0U;
}
