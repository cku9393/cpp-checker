#include "raw_engine/raw_engine.hpp"

#include <stdexcept>

using namespace std;

namespace {

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
        q.push_front(move(*it));
    }
}

bool valid_split_pair_for_support(
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
    int compCnt = 0;

    for (RawVID start : S.verts) {
        if (is_sep(start) || comp.count(start) != 0U) {
            continue;
        }

        const int cid = compCnt++;
        touchA.push_back(false);
        touchB.push_back(false);
        deque<RawVID> dq;
        dq.push_back(start);
        comp[start] = cid;

        while (!dq.empty()) {
            const RawVID u = dq.front();
            dq.pop_front();
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
            return false;
        }
        if (supportComp == -1) {
            supportComp = it->second;
        } else if (supportComp != it->second) {
            return false;
        }
    }
    if (supportComp == -1) {
        return false;
    }

    for (int cid = 0; cid < compCnt; ++cid) {
        if (!touchA[static_cast<size_t>(cid)] || !touchB[static_cast<size_t>(cid)]) {
            return false;
        }
    }

    return compCnt >= 2;
}

optional<pair<Vertex, Vertex>> discover_split_pair_from_support(const RawEngine& RE, OccID occ) {
    const RawOccRecord& O = RE.occ.get(occ);
    const RawSkeleton& S = RE.skel.get(O.hostSkel);
    if (S.hostedOcc.size() != 1U || S.hostedOcc[0] != occ) {
        return nullopt;
    }

    const vector<RawVID> support = collect_support_vertices(RE, occ);
    vector<pair<Vertex, RawVID>> realVs;
    for (RawVID v : S.verts) {
        const RawVertex& RV = RE.V.get(v);
        if (RV.kind == RawVertexKind::REAL) {
            realVs.push_back({RV.orig, v});
        }
    }
    sort(realVs.begin(), realVs.end(), [](const auto& x, const auto& y) { return x.first < y.first; });

    for (u32 i = 0; i < static_cast<u32>(realVs.size()); ++i) {
        for (u32 j = i + 1; j < static_cast<u32>(realVs.size()); ++j) {
            if (valid_split_pair_for_support(RE, S, realVs[i].second, realVs[j].second, support)) {
                return pair<Vertex, Vertex>{realVs[i].first, realVs[j].first};
            }
        }
    }
    return nullopt;
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
        prepend_jobs(q, move(jobs));
    };

    hooks.afterJoin = [ctx](const JoinSeparationPairResult&, deque<UpdJob>& q) {
        enqueue_ensure_sole_once_back(q, ctx.targetOcc);
    };

    hooks.afterIntegrate = [ctx](const IntegrateResult&, deque<UpdJob>& q) {
        enqueue_ensure_sole_once_back(q, ctx.targetOcc);
    };

    return hooks;
}

void run_raw_local_updater(
    RawEngine& RE,
    OccID targetOcc,
    RawUpdateCtx& U,
    RawUpdaterHooks* hooks,
    const RawUpdaterRunOptions& runOptions
) {
    deque<UpdJob> q;
    q.push_back(make_ensure_sole_job(targetOcc));

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
                    isolate_vertex(RE, O.hostSkel, job.occ, U);
                    q.push_front(make_ensure_sole_job(job.occ));
                    break;
                }

                const optional<pair<Vertex, Vertex>> sep = discover_split_pair_from_support(RE, job.occ);
                if (!sep.has_value()) {
                    break;
                }

                const auto [a, b] = *sep;
                const SplitSeparationPairResult res = split_separation_pair(RE, O.hostSkel, a, b, U);
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
                    enqueue_ensure_sole_once_back(q, targetOcc);
                }
                break;
            }

            case UpdJobKind::INTEGRATE_CHILD: {
                const IntegrateResult res = integrate_skeleton(RE, job.parentSid, job.childSid, job.bm, U);
                if (hooks != nullptr && hooks->afterIntegrate) {
                    hooks->afterIntegrate(res, q);
                } else {
                    enqueue_ensure_sole_once_back(q, targetOcc);
                }
                break;
            }
        }
    }
}
