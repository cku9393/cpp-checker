#include "harness/dump.hpp"
#include "harness/project_static_adapter.hpp"
#include <iomanip>
#include <sstream>

namespace harness {

static void dumpCanonicalExplicitGraph(const CanonicalExplicitGraph &graph,
                                       std::ostream &os) {
    os << "canonicalVertices:";
    for (int v : graph.vertices) os << ' ' << v;
    os << "\n";
    os << "canonicalEdges:\n";
    for (const auto &[edgeId, u, v] : graph.edges) {
        os << "  (" << edgeId << "," << u << "," << v << ")\n";
    }
}

static void dumpCompactPrecheckSummary(const CompactPrecheckSummary &summary,
                                       std::ostream &os) {
    os << "precheckSummary:\n";
    os << "  edgeCount=" << summary.edgeCount << "\n";
    os << "  vertexCount=" << summary.vertexCount << "\n";
    os << "  selfLoopCount=" << summary.selfLoopCount << "\n";
    os << "  connectedComponentCount=" << summary.connectedComponentCount << "\n";
    os << "  connected=" << (summary.connected ? 1 : 0) << "\n";
    os << "  biconnected=" << (summary.biconnected ? 1 : 0) << "\n";
    os << "  spqrReady=" << (summary.spqrReady ? 1 : 0) << "\n";
    os << "  spqrReadyWhy=" << summary.spqrReadyWhy << "\n";
    os << "  tooSmallSubtype=" << summary.tooSmallSubtype << "\n";
    os << "  notBiconnectedSubtype=" << summary.notBiconnectedSubtype << "\n";
    os << "  selfLoopSubtype=" << summary.selfLoopSubtype << "\n";
}

void dumpCompactGraph(const CompactGraph &H, std::ostream &os) {
    os << "=== CompactGraph ===\n";
    os << "block=" << H.block << " ownerR=" << H.ownerR << " deletedX=" << H.deletedX << "\n";
    os << "origOfCv:";
    for (int i = 0; i < (int)H.origOfCv.size(); ++i) os << " [" << i << "->" << H.origOfCv[i] << "]";
    os << "\n";
    for (const auto &e : H.edges) {
        os << "edge#" << e.id << " kind=" << (e.kind == CompactEdgeKind::REAL ? "REAL" : "PROXY")
           << " cv=(" << e.a << "," << e.b << ")";
        if (e.a >= 0 && e.a < (int)H.origOfCv.size() && e.b >= 0 && e.b < (int)H.origOfCv.size()) {
            os << " orig=(" << H.origOfCv[e.a] << "," << H.origOfCv[e.b] << ")";
        }
        if (e.kind == CompactEdgeKind::REAL) {
            os << " realEdge=" << e.realEdge;
        } else {
            os << " oldArc=" << e.oldArc << " outsideNode=" << e.outsideNode << " oldSlotInU=" << e.oldSlotInU;
            os << " sideAgg=(" << e.sideAgg.edgeCnt << "," << e.sideAgg.vertexCnt << "," << e.sideAgg.watchedCnt << ")";
        }
        os << "\n";
    }
}

void dumpRawSpqrDecomp(const RawSpqrDecomp &raw, std::ostream &os) {
    os << "=== RawSpqrDecomp ===\n";
    os << "valid=" << raw.valid << " error=" << (int)raw.error << "\n";
    for (int u = 0; u < (int)raw.nodes.size(); ++u) {
        const auto &N = raw.nodes[u];
        if (!N.alive) continue;
        os << "node " << u << " type=" << (N.type == SPQRType::S_NODE ? 'S' : N.type == SPQRType::P_NODE ? 'P' : 'R') << "\n";
        for (int s = 0; s < (int)N.slots.size(); ++s) {
            const auto &sl = N.slots[s];
            if (!sl.alive) continue;
            os << "  slot " << s << " kind=" << (sl.kind == RawSlotKind::INPUT_EDGE ? "INPUT" : "TREE")
               << " poles=(" << sl.poleA << "," << sl.poleB << ")";
            if (sl.kind == RawSlotKind::INPUT_EDGE) os << " input=" << sl.inputEdgeId;
            else os << " tree=" << sl.treeEdgeId;
            os << "\n";
        }
        if (!N.cycleSlots.empty()) {
            os << "  cycleSlots:";
            for (int x : N.cycleSlots) os << ' ' << x;
            os << "\n";
        }
        if (N.pShape) {
            os << "  pShape poles=(" << N.pShape->poleA << "," << N.pShape->poleB << ")\n";
        }
        if (N.rShape) {
            os << "  rShape skelVertices:";
            for (auto v : N.rShape->skelVertices) os << ' ' << v;
            os << "\n";
        }
    }
    for (int t = 0; t < (int)raw.treeEdges.size(); ++t) {
        const auto &E = raw.treeEdges[t];
        if (!E.alive) continue;
        os << "treeEdge " << t << " (" << E.a << ':' << E.slotInA << ") <-> (" << E.b << ':' << E.slotInB
           << ") poles=(" << E.poleA << "," << E.poleB << ")\n";
    }
}

void dumpStaticMiniCore(const StaticMiniCore &mini, std::ostream &os) {
    os << "valid=" << mini.valid << " kind=" << (mini.kind == CoreKind::TINY ? "TINY" : "REDUCED_SPQR") << "\n";
    for (int u = 0; u < (int)mini.nodes.size(); ++u) {
        const auto &N = mini.nodes[u];
        if (!N.alive) continue;
        os << "node " << u << " type=" << (N.type == SPQRType::S_NODE ? 'S' : N.type == SPQRType::P_NODE ? 'P' : 'R')
           << " localAgg=(" << N.localAgg.edgeCnt << "," << N.localAgg.vertexCnt << "," << N.localAgg.watchedCnt << ")"
           << " payloadAgg=(" << N.payloadAgg.edgeCnt << "," << N.payloadAgg.vertexCnt << "," << N.payloadAgg.watchedCnt << ")\n";
        for (int s = 0; s < (int)N.slots.size(); ++s) {
            const auto &sl = N.slots[s];
            if (!sl.alive) continue;
            os << "  slot " << s << " kind=";
            if (sl.kind == MiniSlotKind::REAL_INPUT) os << "REAL_INPUT";
            else if (sl.kind == MiniSlotKind::PROXY_INPUT) os << "PROXY_INPUT";
            else os << "INTERNAL_VIRTUAL";
            os << " poles=(" << sl.poleA << "," << sl.poleB << ")";
            if (sl.kind == MiniSlotKind::REAL_INPUT) os << " input=" << sl.inputEdgeId << " realEdge=" << sl.realEdge;
            else if (sl.kind == MiniSlotKind::PROXY_INPUT) os << " input=" << sl.inputEdgeId;
            else os << " arc=" << sl.miniArcId;
            os << "\n";
        }
    }
}

void dumpActualCore(const ReducedSPQRCore &C, std::ostream &os) {
    os << "blockId=" << C.blockId << " root=" << C.root
       << " totalAgg=(" << C.totalAgg.edgeCnt << "," << C.totalAgg.vertexCnt << "," << C.totalAgg.watchedCnt << ")\n";
    for (NodeId u = 0; u < (NodeId)C.nodes.size(); ++u) {
        const auto &N = C.nodes[u];
        if (!N.alive) continue;
        os << "node " << u << " type=" << (N.type == SPQRType::S_NODE ? 'S' : N.type == SPQRType::P_NODE ? 'P' : 'R')
           << " localAgg=(" << N.localAgg.edgeCnt << "," << N.localAgg.vertexCnt << "," << N.localAgg.watchedCnt << ")"
           << " subAgg=(" << N.subAgg.edgeCnt << "," << N.subAgg.vertexCnt << "," << N.subAgg.watchedCnt << ")\n";
        os << "  realEdgesHere:";
        for (auto e : N.realEdgesHere) os << ' ' << e;
        os << "\n";
        for (int s = 0; s < (int)N.slots.size(); ++s) {
            const auto &sl = N.slots[s];
            if (!sl.alive) continue;
            os << "  slot " << s << " kind=" << (sl.isVirtual ? "VIRTUAL" : "REAL")
               << " poles=(" << sl.poleA << "," << sl.poleB << ")";
            if (sl.isVirtual) os << " arcId=" << sl.arcId;
            else os << " realEdge=" << sl.realEdge;
            os << "\n";
        }
    }
}

void dumpOgdfRawCrashReplayBundle(const OgdfRawCrashReplayBundle &bundle,
                                  const std::string &path) {
    std::ofstream os(path);
    os << "caseName=" << bundle.caseName << "\n";
    os << "manifestPath=" << bundle.manifestPath << "\n";
    os << "seed=" << bundle.seed << "\n";
    os << "tcIndex=" << bundle.tcIndex << "\n";
    os << "targetStep=";
    if (bundle.targetStep.has_value()) os << *bundle.targetStep;
    else os << "null";
    os << "\n";
    os << "topLevelOk=" << (bundle.topLevelOk ? 1 : 0) << "\n";
    os << "requestedSource=" << ogdfRawCrashReplaySourceKindName(bundle.requestedSource)
       << "\n";
    os << "stopBeforeOgdf=" << (bundle.stopBeforeOgdf ? 1 : 0) << "\n";
    os << "runChild=" << (bundle.runChild ? 1 : 0) << "\n";
    os << "sourceSide=" << bundle.sourceSide << "\n";
    os << "callSiteTag=" << bundle.callSiteTag << "\n";
    os << "phaseTag=" << bundle.phaseTag << "\n";
    os << "dispatchKind=" << bundle.dispatchKind << "\n";
    os << "directRawAllowed=" << (bundle.directRawAllowed ? 1 : 0) << "\n";
    os << "directRawBlockedReason=" << bundle.directRawBlockedReason << "\n";
    os << "usedSharedDispatchPath=" << (bundle.usedSharedDispatchPath ? 1 : 0) << "\n";
    os << "usedWholeCoreFallback=" << (bundle.usedWholeCoreFallback ? 1 : 0) << "\n";
    os << "stepIndex=" << bundle.stepIndex << "\n";
    os << "chosenR=" << bundle.chosenR << "\n";
    os << "chosenX=" << bundle.chosenX << "\n";
    os << "compactGraphCanonicalSummary=" << bundle.compactGraphCanonicalSummary << "\n";
    os << "compactGraphDumpPath=" << bundle.compactGraphDumpPath << "\n";
    os << "childExitCode=" << bundle.childExitCode << "\n";
    os << "childSignal=" << bundle.childSignal << "\n";
    os << "crashed=" << (bundle.crashed ? 1 : 0) << "\n";
    os << "crashWhy=" << bundle.crashWhy << "\n";
    os << "lldbBacktracePath=" << bundle.lldbBacktracePath << "\n";
    os << "notes=" << bundle.notes << "\n";
    dumpCompactPrecheckSummary(bundle.precheckSummary, os);
    dumpCompactGraph(bundle.compactGraphRaw, os);
}

static void dumpReplayLiveArcSummary(const GraftTrace::ReplayLiveArcSummary &arc,
                                     std::ostream &os) {
    os << "    arcId=" << arc.arcId
       << " otherNode=" << arc.otherNode
       << " slotInNode=" << arc.slotInNode
       << " slotInOther=" << arc.slotInOther
       << " poles=(" << arc.poleA << "," << arc.poleB << ")\n";
}

static void dumpReplaySlotSnapshot(const GraftTrace::ReplaySlotSnapshot &slot,
                                   std::ostream &os) {
    os << "    slot " << slot.slotId
       << " alive=" << (slot.alive ? 1 : 0)
       << " kind=" << (slot.isVirtual ? "VIRTUAL" : "REAL")
       << " poles=(" << slot.poleA << "," << slot.poleB << ")";
    if (slot.isVirtual) os << " arcId=" << slot.arcId;
    else os << " realEdge=" << slot.realEdge;
    os << "\n";
}

static void dumpReplayNodeSnapshot(const GraftTrace::ReplayNodeSnapshot &node,
                                   std::ostream &os) {
    os << "  node " << node.nodeId
       << " alive=" << (node.alive ? 1 : 0)
       << " type="
       << (node.type == SPQRType::S_NODE ? 'S' :
           node.type == SPQRType::P_NODE ? 'P' : 'R')
       << "\n";
    os << "    adjArcs:";
    for (auto arcId : node.adjArcs) os << ' ' << arcId;
    os << "\n";
    os << "    realEdgesHere:";
    for (auto edgeId : node.realEdgesHere) os << ' ' << edgeId;
    os << "\n";
    os << "    slots:\n";
    for (const auto &slot : node.slots) {
        dumpReplaySlotSnapshot(slot, os);
    }
    os << "    neighboringLiveArcs:\n";
    for (const auto &arc : node.neighboringLiveArcs) {
        dumpReplayLiveArcSummary(arc, os);
    }
}

static void dumpReplayNodeSnapshotPhases(
    const std::vector<GraftTrace::ReplayNodeSnapshotPhase> &phases,
    const char *label,
    std::ostream &os) {
    os << label << ":\n";
    for (const auto &phase : phases) {
        os << " phase=" << replaySnapshotPhaseName(phase.phase) << "\n";
        for (const auto &node : phase.nodes) {
            dumpReplayNodeSnapshot(node, os);
        }
    }
}

static void dumpSolverBaselineReplayPhaseSnapshots(
    const std::vector<SolverBaselineReplayPhaseSnapshot> &phases,
    const char *label,
    std::ostream &os) {
    os << label << ":\n";
    for (const auto &phase : phases) {
        os << " phase=" << solverBaselineReplayPhaseName(phase.phase)
           << " aliveNodeCount=" << phase.aliveNodeCount
           << " currentRoot=" << phase.currentRoot
           << " currentExplicitEdgeCount=" << phase.currentExplicitEdgeCount
           << "\n";
        for (const auto &node : phase.nodes) {
            dumpReplayNodeSnapshot(node, os);
        }
    }
}

void dumpGraftTrace(const GraftTrace &T, std::ostream &os) {
    os << "=== GraftTrace ===\nactualOfMini:";
    for (int i = 0; i < (int)T.actualOfMini.size(); ++i) os << " [" << i << "->" << T.actualOfMini[i] << "]";
    os << "\nactualNodes:";
    for (auto x : T.actualNodes) os << ' ' << x;
    os << "\nactualSlotOfMiniSlot:\n";
    for (int m = 0; m < (int)T.actualSlotOfMiniSlot.size(); ++m) {
        os << "  mini " << m << ':';
        for (int s = 0; s < (int)T.actualSlotOfMiniSlot[m].size(); ++s) {
            os << " [" << s << "->" << T.actualSlotOfMiniSlot[m][s] << "]";
        }
        os << "\n";
    }
    os << "rewiredProxyEdges:\n";
    for (const auto &rw : T.rewiredProxyEdges) {
        os << "  input=" << rw.inputEdgeId
           << " oldArc=" << rw.oldArc
           << " actualNode=" << rw.actualNode
           << " actualSlot=" << rw.actualSlot << "\n";
    }
    os << "resolvedProxyEndpoints:\n";
    for (const auto &rp : T.resolvedProxyEndpoints) {
        os << "  input=" << rp.inputEdgeId
           << " originalOldArc=" << rp.originalOldArc
           << " resolvedArc=" << rp.resolvedArc
           << " oldNode=" << rp.oldNode
           << " originalOutsideNode=" << rp.originalOutsideNode
           << " resolvedOutsideNode=" << rp.resolvedOutsideNode
           << " outsideNode=" << rp.outsideNode
           << " resolvedOldSlot=" << rp.resolvedOldSlot
           << " poles=(" << rp.poleA << "," << rp.poleB << ")"
           << " weakPolesOnly=" << (rp.repairUsedWeakPolesOnly ? 1 : 0)
           << " repairOutcome=" << proxyArcRepairOutcomeName(rp.repairOutcome)
           << " firstBadPhase=" << proxyArcLifecyclePhaseName(rp.firstBadPhase)
           << " firstBadWhy=" << rp.firstBadWhy
           << " phaseHistory=";
        for (size_t i = 0; i < rp.phaseHistory.size(); ++i) {
            if (i) os << ',';
            os << proxyArcLifecyclePhaseName(rp.phaseHistory[i]);
        }
        os << "\n";
    }
    os << "preservedProxyArcs:\n";
    for (const auto &pp : T.preservedProxyArcs) {
        os << "  input=" << pp.inputEdgeId
           << " oldArc=" << pp.oldArc
           << " oldNode=" << pp.oldNode
           << " outsideNode=" << pp.outsideNode
           << " resolvedOldSlot=" << pp.resolvedOldSlot
           << " poles=(" << pp.poleA << "," << pp.poleB << ")"
           << " newSlot=" << pp.newSlot
           << " finalNode=" << pp.finalNode
           << " crossNodeRewire=" << (pp.crossNodeRewire ? 1 : 0)
           << " sameNodeRehome=" << (pp.sameNodeRehome ? 1 : 0)
           << "\n";
    }
    os << "affectedAdjRepairNodes:";
    for (auto nodeId : T.affectedAdjRepairNodes) os << ' ' << nodeId;
    os << "\naffectedNodesAfterInPlaceApply:";
    for (auto nodeId : T.affectedNodesAfterInPlaceApply) os << ' ' << nodeId;
    os << "\noldNodeAdjArcsBeforeRepair:";
    for (auto arcId : T.oldNodeAdjArcsBeforeRepair) os << ' ' << arcId;
    os << "\noldNodeAdjArcsAfterRepair:";
    for (auto arcId : T.oldNodeAdjArcsAfterRepair) os << ' ' << arcId;
    os << "\nfirstBadAdjNode=" << T.firstBadAdjNode;
    os << "\nexpectedAdj:";
    for (auto arcId : T.expectedAdj) os << ' ' << arcId;
    os << "\nactualAdj:";
    for (auto arcId : T.actualAdj) os << ' ' << arcId;
    os << "\nsameTypeSPCleanupSeedNodes:";
    for (auto nodeId : T.sameTypeSPCleanupSeedNodes) os << ' ' << nodeId;
    os << "\nsameTypeSPCleanupMergeCount=" << T.sameTypeSPCleanupMergeCount;
    os << "\nsameTypeSPCleanupMergedPairs:\n";
    for (const auto &merge : T.sameTypeSPCleanupMergedPairs) {
        os << "  (" << merge.u << "," << merge.v << ") -> keep " << merge.keep << "\n";
    }
    os << "\n";
    os << "graftRewireSubtype=" << graftRewireBailoutSubtypeName(T.graftRewireSubtype)
       << " graftOtherSubtype=" << graftOtherSubtypeName(T.graftOtherSubtype)
       << " postcheckSubtype=" << graftPostcheckSubtypeName(T.postcheckSubtype)
       << " preCleanupPostcheckSubtype="
       << graftPostcheckSubtypeName(T.preCleanupPostcheckSubtype)
       << " postCleanupPostcheckSubtype="
       << graftPostcheckSubtypeName(T.postCleanupPostcheckSubtype)
       << " deferredSameTypeSP=" << (T.deferredSameTypeSP ? 1 : 0)
       << " preservedProxyArcsCount=" << T.preservedProxyArcsCount
       << " inPlaceLoopSharedApplied=" << (T.inPlaceLoopSharedApplied ? 1 : 0)
       << " loopInputEdgeId=" << T.loopInputEdgeId
       << " realInputEdgeId=" << T.realInputEdgeId
       << " loopSharedCutVertex=" << T.loopSharedCutVertex
       << " loopSharedChildNode=" << T.loopSharedChildNode
       << " sameNodeRehomeAttempted=" << (T.sameNodeRehomeAttempted ? 1 : 0)
       << " sameNodeRehomeSucceeded=" << (T.sameNodeRehomeSucceeded ? 1 : 0)
       << " failingPreservedInputEdge=" << T.failingPreservedInputEdge
       << " failingPreservedOldArc=" << T.failingPreservedOldArc
       << " failingPreservedOldSlot=" << T.failingPreservedOldSlot
       << " failingNewSlot=" << T.failingNewSlot
       << " failingInputEdge=" << T.failingInputEdge
       << " failingOldArc=" << T.failingOldArc
       << " failingOwnerMini=" << T.failingOwnerMini
       << " failingOwnerMiniSlot=" << T.failingOwnerMiniSlot << "\n";
    if (!T.graftOtherWhy.empty()) {
        os << "graftOtherWhy=" << T.graftOtherWhy << "\n";
    }
    if (!T.postcheckWhyDetailed.empty()) {
        os << "postcheckWhyDetailed=" << T.postcheckWhyDetailed << "\n";
    }
    os << "weakRepairEntered=" << (T.weakRepairEntered ? 1 : 0)
       << " weakRepairGateSubtype=" << weakRepairGateSubtypeName(T.weakRepairGateSubtype)
       << " weakRepairCandidateSubtype="
       << weakRepairCandidateSubtypeName(T.weakRepairCandidateSubtype)
       << " weakRepairCommitOutcome="
       << weakRepairCommitOutcomeName(T.weakRepairCommitOutcome)
       << " weakRepairInputEdgeId=" << T.weakRepairInputEdgeId
       << " weakRepairOriginalOldArc=" << T.weakRepairOriginalOldArc
       << " weakRepairResolvedArc=" << T.weakRepairResolvedArc
       << " weakRepairOriginalOutsideNode=" << T.weakRepairOriginalOutsideNode
       << " weakRepairResolvedOutsideNode=" << T.weakRepairResolvedOutsideNode << "\n";
    dumpReplayNodeSnapshotPhases(T.oldNodeSnapshotsByPhase, "oldNodeSnapshotsByPhase", os);
    dumpReplayNodeSnapshotPhases(T.affectedNodeSnapshotsByPhase,
                                 "affectedNodeSnapshotsByPhase",
                                 os);
}

void dumpExplicitBlockGraph(const ExplicitBlockGraph &G, std::ostream &os) {
    os << "vertices:";
    for (auto v : G.vertices) os << ' ' << v;
    os << "\nedges:\n";
    for (const auto &e : G.edges) {
        os << "  id=" << e.id << " (" << e.u << "," << e.v << ")\n";
    }
}

void dumpSolverOutput(const SolverOutput &out, std::ostream &os) {
    os << "valid=" << (out.valid ? 1 : 0) << "\n";
    os << "actualInvariantOk=" << (out.actualInvariantOk ? 1 : 0) << "\n";
    if (out.baselineKind.has_value()) {
        os << "baselineKind=" << compareBaselineKindName(*out.baselineKind) << "\n";
    }
    os << "debugTag=" << out.debugTag << "\n";
    os << "why=" << out.why << "\n";
    os << "parent:";
    for (int value : out.parent) os << ' ' << value;
    os << "\n";
    dumpExplicitBlockGraph(out.explicitGraph, os);
    dumpCanonicalExplicitGraph(out.canonicalExplicitGraph, os);
}

void dumpSolverCompareBundle(const SolverCompareBundle &bundle,
                             const std::string &path) {
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    std::ofstream ofs(path);
    ofs << "=== SolverCompareBundle ===\n";
    ofs << "inputCaseId=" << bundle.inputCaseId << "\n";
    ofs << "seed=" << bundle.seed << "\n";
    ofs << "tc=" << bundle.tc << "\n";
    ofs << "oracleHandoffPolicy="
        << oracleHandoffPolicyName(bundle.oracleHandoffPolicy) << "\n";
    if (bundle.targetStep.has_value()) {
        ofs << "targetStep=" << *bundle.targetStep << "\n";
    }
    ofs << "legacyWhy=" << bundle.legacyWhy << "\n";
    ofs << "oracleWhy=" << bundle.oracleWhy << "\n";
    ofs << "rewriteSeqWhy=" << bundle.rewriteSeqWhy << "\n";
    if (bundle.legacyVsRewriteEquivalent.has_value()) {
        ofs << "legacyVsRewriteEquivalent="
            << (*bundle.legacyVsRewriteEquivalent ? 1 : 0) << "\n";
    }
    if (bundle.legacyVsRewriteRawExplicitEquivalent.has_value()) {
        ofs << "legacyVsRewriteRawExplicitEquivalent="
            << (*bundle.legacyVsRewriteRawExplicitEquivalent ? 1 : 0) << "\n";
    }
    if (bundle.legacyVsRewriteCanonicalExplicitEquivalent.has_value()) {
        ofs << "legacyVsRewriteCanonicalExplicitEquivalent="
            << (*bundle.legacyVsRewriteCanonicalExplicitEquivalent ? 1 : 0) << "\n";
    }
    if (bundle.oracleVsRewriteEquivalent.has_value()) {
        ofs << "oracleVsRewriteEquivalent="
            << (*bundle.oracleVsRewriteEquivalent ? 1 : 0) << "\n";
    }
    if (bundle.oracleVsRewriteRawExplicitEquivalent.has_value()) {
        ofs << "oracleVsRewriteRawExplicitEquivalent="
            << (*bundle.oracleVsRewriteRawExplicitEquivalent ? 1 : 0) << "\n";
    }
    if (bundle.oracleVsRewriteCanonicalExplicitEquivalent.has_value()) {
        ofs << "oracleVsRewriteCanonicalExplicitEquivalent="
            << (*bundle.oracleVsRewriteCanonicalExplicitEquivalent ? 1 : 0) << "\n";
    }
    if (bundle.legacyVsOracleEquivalent.has_value()) {
        ofs << "legacyVsOracleEquivalent="
            << (*bundle.legacyVsOracleEquivalent ? 1 : 0) << "\n";
    }
    if (bundle.legacyVsOracleRawExplicitEquivalent.has_value()) {
        ofs << "legacyVsOracleRawExplicitEquivalent="
            << (*bundle.legacyVsOracleRawExplicitEquivalent ? 1 : 0) << "\n";
    }
    if (bundle.legacyVsOracleCanonicalExplicitEquivalent.has_value()) {
        ofs << "legacyVsOracleCanonicalExplicitEquivalent="
            << (*bundle.legacyVsOracleCanonicalExplicitEquivalent ? 1 : 0) << "\n";
    }
    if (bundle.deleteVsRewriteCanonicalEqual.has_value()) {
        ofs << "deleteVsRewriteCanonicalEqual="
            << (*bundle.deleteVsRewriteCanonicalEqual ? 1 : 0) << "\n";
    }
    if (bundle.normalizeVsRewriteCanonicalEqual.has_value()) {
        ofs << "normalizeVsRewriteCanonicalEqual="
            << (*bundle.normalizeVsRewriteCanonicalEqual ? 1 : 0) << "\n";
    }
    if (bundle.deleteVsNormalizeCanonicalEqual.has_value()) {
        ofs << "deleteVsNormalizeCanonicalEqual="
            << (*bundle.deleteVsNormalizeCanonicalEqual ? 1 : 0) << "\n";
    }
    ofs << "firstMismatchDescription=" << bundle.firstMismatchDescription << "\n";
    ofs << "legacyElapsedMs=" << bundle.legacyElapsedMs << "\n";
    ofs << "oracleElapsedMs=" << bundle.oracleElapsedMs << "\n";
    ofs << "rewriteSeqElapsedMs=" << bundle.rewriteSeqElapsedMs << "\n\n";

    ofs << "=== InputExplicit ===\n";
    dumpExplicitBlockGraph(bundle.inputExplicit, ofs);
    ofs << "\n";
    if (bundle.legacyCanonicalExplicit.has_value()) {
        ofs << "=== LegacyCanonicalExplicit ===\n";
        dumpCanonicalExplicitGraph(*bundle.legacyCanonicalExplicit, ofs);
        ofs << "\n";
    }
    if (bundle.oracleCanonicalExplicit.has_value()) {
        ofs << "=== OracleCanonicalExplicit ===\n";
        dumpCanonicalExplicitGraph(*bundle.oracleCanonicalExplicit, ofs);
        ofs << "\n";
    }
    if (bundle.rewriteSeqCanonicalExplicit.has_value()) {
        ofs << "=== RewriteSeqCanonicalExplicit ===\n";
        dumpCanonicalExplicitGraph(*bundle.rewriteSeqCanonicalExplicit, ofs);
        ofs << "\n";
    }
    if (bundle.oracleDeletePolicyFinalExplicit.has_value()) {
        ofs << "=== OracleDeletePolicyFinalExplicit ===\n";
        dumpCanonicalExplicitGraph(*bundle.oracleDeletePolicyFinalExplicit, ofs);
        ofs << "\n";
    }
    if (bundle.oracleNormalizePolicyFinalExplicit.has_value()) {
        ofs << "=== OracleNormalizePolicyFinalExplicit ===\n";
        dumpCanonicalExplicitGraph(*bundle.oracleNormalizePolicyFinalExplicit, ofs);
        ofs << "\n";
    }
    if (bundle.rewriteFinalExplicit.has_value()) {
        ofs << "=== RewriteFinalExplicit ===\n";
        dumpCanonicalExplicitGraph(*bundle.rewriteFinalExplicit, ofs);
        ofs << "\n";
    }

    if (bundle.legacyOutput.has_value()) {
        ofs << "=== LegacyOutput ===\n";
        dumpSolverOutput(*bundle.legacyOutput, ofs);
        ofs << "\n";
    }
    if (bundle.oracleOutput.has_value()) {
        ofs << "=== OracleOutput ===\n";
        dumpSolverOutput(*bundle.oracleOutput, ofs);
        ofs << "\n";
    }
    if (bundle.rewriteSeqOutput.has_value()) {
        ofs << "=== RewriteSeqOutput ===\n";
        dumpSolverOutput(*bundle.rewriteSeqOutput, ofs);
        ofs << "\n";
    }
}

static void dumpSemanticStepTrace(const SemanticStepTrace &trace, std::ostream &os);

static void dumpSemanticReplayResult(const SemanticReplayResult &result,
                                     const char *label,
                                     std::ostream &os) {
    os << "=== " << label << " ===\n";
    os << "ok=" << (result.ok ? 1 : 0) << "\n";
    os << "terminatedStep=" << result.terminatedStep << "\n";
    os << "why=" << result.why << "\n";
    os << "stepCount=" << result.steps.size() << "\n";
    os << "=== FinalExplicitRaw ===\n";
    dumpExplicitBlockGraph(result.finalExplicitRaw, os);
    os << "=== FinalExplicitCanonical ===\n";
    dumpCanonicalExplicitGraph(result.finalExplicitCanonical, os);
    os << "steps:\n";
    for (const auto &step : result.steps) {
        dumpSemanticStepTrace(step, os);
        os << "\n";
    }
}

static void dumpFinalCoreStepTrace(const FinalCoreStepTrace &trace, std::ostream &os) {
    os << "stepIndex=" << trace.stepIndex << "\n";
    os << "chosenR=" << trace.chosenR << "\n";
    os << "chosenX=" << trace.chosenX << "\n";
    os << "terminated=" << (trace.terminated ? 1 : 0) << "\n";
    os << "terminateReason=" << trace.terminateReason << "\n";
    os << "=== CanonicalExplicitBefore ===\n";
    dumpCanonicalExplicitGraph(trace.canonicalExplicitBefore, os);
    os << "=== CanonicalExplicitAfterNormalize ===\n";
    dumpCanonicalExplicitGraph(trace.canonicalExplicitAfterNormalize, os);
}

static void dumpRewriteTargetSnapshot(const RewriteTargetSnapshot &snapshot,
                                      const char *label,
                                      std::ostream &os) {
    os << "=== " << label << " ===\n";
    os << "stepIndex=" << snapshot.stepIndex << "\n";
    os << "aliveRNodes=[";
    for (size_t i = 0; i < snapshot.aliveRNodes.size(); ++i) {
        if (i != 0) os << ',';
        os << snapshot.aliveRNodes[i];
    }
    os << "]\n";
    os << "hasNextTarget=" << (snapshot.hasNextTarget ? 1 : 0) << "\n";
    os << "chosenR=" << snapshot.chosenR << "\n";
    os << "chosenX=" << snapshot.chosenX << "\n";
    os << "noTargetReason=" << snapshot.noTargetReason << "\n";
    os << "candidates:\n";
    for (const auto &candidate : snapshot.candidates) {
        os << "  rNode=" << candidate.rNode
           << " x=" << candidate.x
           << " scorePrimary=" << candidate.scorePrimary
           << " scoreSecondary=" << candidate.scoreSecondary
           << " sourceTag=" << candidate.sourceTag << "\n";
    }
}

static void dumpSemanticTargetSnapshot(const SemanticTargetSnapshot &snapshot,
                                       const char *label,
                                       std::ostream &os) {
    os << "=== " << label << " ===\n";
    os << "stepIndex=" << snapshot.stepIndex << "\n";
    os << "side=" << snapshot.side << "\n";
    os << "=== CanonicalExplicitAfterNormalize ===\n";
    dumpCanonicalExplicitGraph(snapshot.canonicalExplicitAfterNormalize, os);
    os << "aliveRNodes=[";
    for (size_t i = 0; i < snapshot.aliveRNodes.size(); ++i) {
        if (i != 0) os << ',';
        os << snapshot.aliveRNodes[i];
    }
    os << "]\n";
    os << "hasNextTarget=" << (snapshot.hasNextTarget ? 1 : 0) << "\n";
    os << "chosenR=" << snapshot.chosenR << "\n";
    os << "chosenX=" << snapshot.chosenX << "\n";
    os << "noTargetReason=" << snapshot.noTargetReason << "\n";
    os << "candidates:\n";
    for (const auto &candidate : snapshot.candidates) {
        os << "  rNode=" << candidate.rNode
           << " x=" << candidate.x
           << " scorePrimary=" << candidate.scorePrimary
           << " scoreSecondary=" << candidate.scoreSecondary
           << " sourceTag=" << candidate.sourceTag << "\n";
    }
}

static void dumpTransitionReplaySnapshot(const TransitionReplaySnapshot &snapshot,
                                         const char *label,
                                         std::ostream &os) {
    os << "=== " << label << " ===\n";
    os << "sourceStep=" << snapshot.sourceStep << "\n";
    os << "chosenR=" << snapshot.chosenR << "\n";
    os << "chosenX=" << snapshot.chosenX << "\n";
    os << "terminated=" << (snapshot.terminated ? 1 : 0) << "\n";
    os << "terminateReason=" << snapshot.terminateReason << "\n";
    os << "ok=" << (snapshot.ok ? 1 : 0) << "\n";
    os << "why=" << snapshot.why << "\n";
    os << "explicitBefore:\n";
    dumpCanonicalExplicitGraph(snapshot.explicitBefore, os);
    os << "explicitAfterDelete:\n";
    dumpCanonicalExplicitGraph(snapshot.explicitAfterDelete, os);
    os << "explicitAfterStep:\n";
    dumpCanonicalExplicitGraph(snapshot.explicitAfterStep, os);
}

static void dumpStepHandoffSnapshot(const StepHandoffSnapshot &snapshot,
                                    const char *label,
                                    std::ostream &os) {
    os << "=== " << label << " ===\n";
    os << "stepIndex=" << snapshot.stepIndex << "\n";
    os << "side=" << snapshot.side << "\n";
    os << "chosenR=" << snapshot.chosenR << "\n";
    os << "chosenX=" << snapshot.chosenX << "\n";
    os << "terminated=" << (snapshot.terminated ? 1 : 0) << "\n";
    os << "terminateReason=" << snapshot.terminateReason << "\n";
    os << "nextInputSourceKind="
       << stepHandoffSourceKindName(snapshot.nextInputSourceKind) << "\n";
    os << "explicitBefore:\n";
    dumpCanonicalExplicitGraph(snapshot.explicitBefore, os);
    os << "explicitAfterDelete:\n";
    dumpCanonicalExplicitGraph(snapshot.explicitAfterDelete, os);
    os << "explicitAfterNormalize:\n";
    dumpCanonicalExplicitGraph(snapshot.explicitAfterNormalize, os);
    os << "nextInputExplicit:\n";
    dumpCanonicalExplicitGraph(snapshot.nextInputExplicit, os);
}

static void dumpStepTransitionSnapshot(const StepTransitionSnapshot &snapshot,
                                       const char *label,
                                       std::ostream &os) {
    os << "=== " << label << " ===\n";
    os << "side=" << snapshot.side << "\n";
    os << "sourceStep=" << snapshot.sourceStep << "\n";
    os << "chosenR=" << snapshot.chosenR << "\n";
    os << "chosenX=" << snapshot.chosenX << "\n";
    os << "terminatedAfterStep=" << (snapshot.terminatedAfterStep ? 1 : 0) << "\n";
    os << "terminateReason=" << snapshot.terminateReason << "\n";
    os << "nextInputSourceTag=" << snapshot.nextInputSourceTag << "\n";
    os << "explicitBeforeStep:\n";
    dumpCanonicalExplicitGraph(snapshot.explicitBeforeStep, os);
    os << "explicitAfterDelete:\n";
    dumpCanonicalExplicitGraph(snapshot.explicitAfterDelete, os);
    os << "explicitAfterNormalize:\n";
    dumpCanonicalExplicitGraph(snapshot.explicitAfterNormalize, os);
    os << "nextInputExplicit:\n";
    dumpCanonicalExplicitGraph(snapshot.nextInputExplicit, os);
    os << "aliveRNodesForNextStep=[";
    for (size_t i = 0; i < snapshot.aliveRNodesForNextStep.size(); ++i) {
        if (i != 0) os << ',';
        os << snapshot.aliveRNodesForNextStep[i];
    }
    os << "]\n";
    os << "nextStepCandidates:\n";
    for (const auto &candidate : snapshot.nextStepCandidates) {
        os << "  rNode=" << candidate.rNode
           << " x=" << candidate.x
           << " scorePrimary=" << candidate.scorePrimary
           << " scoreSecondary=" << candidate.scoreSecondary
           << " sourceTag=" << candidate.sourceTag << "\n";
    }
}

static void dumpFinalCoreSignature(const FinalCoreSignature &sig,
                                   const char *label,
                                   std::ostream &os) {
    os << "=== " << label << " ===\n";
    os << "aliveNodeCount=" << sig.aliveNodeCount << "\n";
    os << "aliveArcCount=" << sig.aliveArcCount << "\n";
    os << "root=" << sig.root << "\n";
    dumpCanonicalExplicitGraph(sig.canonicalExplicit, os);
    os << "nodeSummaries:\n";
    for (const auto &summary : sig.nodeSummaries) {
        os << "  " << summary << "\n";
    }
}

static void dumpCoreShapeSnapshot(const CoreShapeSnapshot &snapshot,
                                  const char *label,
                                  std::ostream &os) {
    os << "=== " << label << " ===\n";
    os << "stepIndex=" << snapshot.stepIndex << "\n";
    os << "side=" << snapshot.side << "\n";
    os << "root=" << snapshot.root << "\n";
    os << "aliveNodeCount=" << snapshot.aliveNodeCount << "\n";
    os << "aliveArcCount=" << snapshot.aliveArcCount << "\n";
    os << "rNodeCount=" << snapshot.rNodeCount << "\n";
    os << "sNodeCount=" << snapshot.sNodeCount << "\n";
    os << "pNodeCount=" << snapshot.pNodeCount << "\n";
    os << "aliveRNodes=[";
    for (size_t i = 0; i < snapshot.aliveRNodes.size(); ++i) {
        if (i != 0) os << ',';
        os << snapshot.aliveRNodes[i];
    }
    os << "]\n";
    dumpCanonicalExplicitGraph(snapshot.canonicalExplicit, os);
    dumpRewriteTargetSnapshot(snapshot.targetSnapshot, "TargetSnapshot", os);
    os << "nodeSummaries:\n";
    for (const auto &summary : snapshot.nodeSummaries) {
        os << "  " << summary << "\n";
    }
}

static void dumpBuilderStageSnapshot(const BuilderStageSnapshot &snapshot,
                                     const char *label,
                                     std::ostream &os) {
    os << "=== " << label << " ===\n";
    os << "stage=" << snapshot.stage << "\n";
    os << "root=" << snapshot.root << "\n";
    os << "aliveNodeCount=" << snapshot.aliveNodeCount << "\n";
    os << "aliveArcCount=" << snapshot.aliveArcCount << "\n";
    os << "rNodeCount=" << snapshot.rNodeCount << "\n";
    os << "sNodeCount=" << snapshot.sNodeCount << "\n";
    os << "pNodeCount=" << snapshot.pNodeCount << "\n";
    dumpCanonicalExplicitGraph(snapshot.canonicalExplicit, os);
    os << "nodeSummaries:\n";
    for (const auto &summary : snapshot.nodeSummaries) {
        os << "  " << summary << "\n";
    }
}

static void dumpMaterializeCoreSubphaseSnapshot(
    const MaterializeCoreSubphaseSnapshot &snapshot,
    const char *label,
    std::ostream &os) {
    os << "=== " << label << " ===\n";
    os << "subphase=" << snapshot.subphase << "\n";
    os << "aliveNodeCount=" << snapshot.aliveNodeCount << "\n";
    os << "aliveArcCount=" << snapshot.aliveArcCount << "\n";
    os << "rNodeCount=" << snapshot.rNodeCount << "\n";
    os << "sNodeCount=" << snapshot.sNodeCount << "\n";
    os << "pNodeCount=" << snapshot.pNodeCount << "\n";
    dumpCanonicalExplicitGraph(snapshot.canonicalExplicit, os);
    os << "nodeSummaries:\n";
    for (const auto &summary : snapshot.nodeSummaries) {
        os << "  " << summary << "\n";
    }
}

static void dumpRawReplaySnapshot(const RawReplaySnapshot &snapshot,
                                  const char *label,
                                  std::ostream &os) {
    os << "=== " << label << " ===\n";
    os << "ok=" << (snapshot.ok ? 1 : 0) << "\n";
    os << "why=" << snapshot.why << "\n";
    os << "compactGraphCanonicalSummary=" << snapshot.compactGraphCanonicalSummary << "\n";
    os << "rawNodeCount=" << snapshot.rawNodeCount << "\n";
    os << "rawTreeEdgeCount=" << snapshot.rawTreeEdgeCount << "\n";
    os << "rawRNodeCount=" << snapshot.rawRNodeCount << "\n";
    os << "rawSNodeCount=" << snapshot.rawSNodeCount << "\n";
    os << "rawPNodeCount=" << snapshot.rawPNodeCount << "\n";
    os << "inputExplicit:\n";
    dumpCanonicalExplicitGraph(snapshot.inputExplicit, os);
    dumpCompactPrecheckSummary(snapshot.precheckSummary, os);
    dumpCompactGraph(snapshot.compactGraph, os);
    os << "compactEdgeSummaries:\n";
    for (const auto &summary : snapshot.compactEdgeSummaries) {
        os << "  " << summary << "\n";
    }
    os << "rawNodeSummaries:\n";
    for (const auto &summary : snapshot.rawNodeSummaries) {
        os << "  " << summary << "\n";
    }
    os << "rawTreeEdgeSummaries:\n";
    for (const auto &summary : snapshot.rawTreeEdgeSummaries) {
        os << "  " << summary << "\n";
    }
}

void dumpSolverFinalCoreReplayBundle(const SolverFinalCoreReplayBundle &bundle,
                                     const std::string &path) {
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    std::ofstream ofs(path);
    ofs << "=== SolverFinalCoreReplayBundle ===\n";
    ofs << "caseName=" << bundle.caseName << "\n";
    ofs << "manifestPath=" << bundle.manifestPath << "\n";
    ofs << "seed=" << bundle.seed << "\n";
    ofs << "tc=" << bundle.tc << "\n";
    if (bundle.targetStep.has_value()) {
        ofs << "targetStep=" << *bundle.targetStep << "\n";
    }
    ofs << "topLevelOk=" << (bundle.topLevelOk ? 1 : 0) << "\n";
    ofs << "finalCoreSeamKind=" << finalCoreSeamKindName(bundle.finalCoreSeamKind) << "\n";
    ofs << "firstDivergenceStep=" << bundle.firstDivergenceStep << "\n";
    ofs << "finalCoreSeamWhy=" << bundle.finalCoreSeamWhy << "\n";
    ofs << "targetSearchComparedStep=" << bundle.targetSearchComparedStep << "\n";
    ofs << "targetSearchSeamKind="
        << targetSearchSeamKindName(bundle.targetSearchSeamKind) << "\n";
    ofs << "targetSearchSeamWhy=" << bundle.targetSearchSeamWhy << "\n";
    ofs << "solverWhy=" << bundle.solverWhy << "\n";
    ofs << "replayWhy=" << bundle.replayWhy << "\n";
    ofs << "solverStats.success=" << (bundle.solverStats.success ? 1 : 0) << "\n";
    ofs << "solverStats.reachedFixpoint=" << (bundle.solverStats.reachedFixpoint ? 1 : 0)
        << "\n";
    ofs << "solverStats.hadSequenceFallback="
        << (bundle.solverStats.hadSequenceFallback ? 1 : 0) << "\n";
    ofs << "solverStats.maxStepReached="
        << (bundle.solverStats.maxStepReached ? 1 : 0) << "\n";
    ofs << "solverStats.completedSteps=" << bundle.solverStats.completedSteps << "\n\n";
    ofs << "solverStats.solverShadowResyncAttemptCount="
        << bundle.solverStats.solverShadowResyncAttemptCount << "\n";
    ofs << "solverStats.solverShadowResyncAppliedCount="
        << bundle.solverStats.solverShadowResyncAppliedCount << "\n";
    ofs << "solverStats.solverShadowResyncNoopCount="
        << bundle.solverStats.solverShadowResyncNoopCount << "\n";
    ofs << "solverStats.solverShadowResyncNoTargetToHasTargetCount="
        << bundle.solverStats.solverShadowResyncNoTargetToHasTargetCount << "\n";
    ofs << "solverStats.solverShadowResyncAliveRSetDifferCount="
        << bundle.solverStats.solverShadowResyncAliveRSetDifferCount << "\n\n";

    dumpFinalCoreSignature(bundle.solverFinalCoreSignature, "SolverFinalCoreSignature", ofs);
    ofs << "\n";
    dumpFinalCoreSignature(bundle.replayFinalCoreSignature, "ReplayFinalCoreSignature", ofs);
    ofs << "\n";
    if (bundle.targetSearchComparedStep >= 0) {
        for (const auto &snapshot : bundle.solverPostStepTargetSnapshots) {
            if (snapshot.stepIndex == bundle.targetSearchComparedStep) {
                dumpRewriteTargetSnapshot(
                    snapshot, "SolverPostStepTargetSnapshotCompared", ofs);
                ofs << "\n";
                break;
            }
        }
        for (const auto &snapshot : bundle.replayPostStepTargetSnapshots) {
            if (snapshot.stepIndex == bundle.targetSearchComparedStep) {
                dumpRewriteTargetSnapshot(
                    snapshot, "ReplayPostStepTargetSnapshotCompared", ofs);
                ofs << "\n";
                break;
            }
        }
    }
    ofs << "\nsolverTrace:\n";
    for (const auto &step : bundle.solverTrace) {
        dumpFinalCoreStepTrace(step, ofs);
        ofs << "\n";
    }
    ofs << "replayTrace:\n";
    for (const auto &step : bundle.replayTrace) {
        dumpFinalCoreStepTrace(step, ofs);
        ofs << "\n";
    }
    ofs << "solverPostStepTargetSnapshots:\n";
    for (const auto &snapshot : bundle.solverPostStepTargetSnapshots) {
        dumpRewriteTargetSnapshot(snapshot, "SolverPostStepTargetSnapshot", ofs);
        ofs << "\n";
    }
    ofs << "replayPostStepTargetSnapshots:\n";
    for (const auto &snapshot : bundle.replayPostStepTargetSnapshots) {
        dumpRewriteTargetSnapshot(snapshot, "ReplayPostStepTargetSnapshot", ofs);
        ofs << "\n";
    }
}

void dumpSolverShapeReplayBundle(const SolverShapeReplayBundle &bundle,
                                 const std::string &path) {
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    std::ofstream ofs(path);
    ofs << "=== SolverShapeReplayBundle ===\n";
    ofs << "caseName=" << bundle.caseName << "\n";
    ofs << "manifestPath=" << bundle.manifestPath << "\n";
    ofs << "seed=" << bundle.seed << "\n";
    ofs << "tc=" << bundle.tc << "\n";
    if (bundle.targetStep.has_value()) {
        ofs << "targetStep=" << *bundle.targetStep << "\n";
    }
    ofs << "topLevelOk=" << (bundle.topLevelOk ? 1 : 0) << "\n";
    ofs << "seamStepIndex=" << bundle.seamStepIndex << "\n";
    ofs << "coreShapeSeamKind=" << coreShapeSeamKindName(bundle.coreShapeSeamKind) << "\n";
    ofs << "coreShapeSeamWhy=" << bundle.coreShapeSeamWhy << "\n";
    ofs << "solverWhy=" << bundle.solverWhy << "\n";
    ofs << "replayWhy=" << bundle.replayWhy << "\n";
    ofs << "shadowWhy=" << bundle.shadowWhy << "\n\n";
    dumpCoreShapeSnapshot(bundle.solverShapeSnapshot, "SolverShapeSnapshot", ofs);
    ofs << "\n";
    dumpCoreShapeSnapshot(bundle.replayShapeSnapshot, "ReplayShapeSnapshot", ofs);
    ofs << "\n";
    dumpCoreShapeSnapshot(bundle.shadowShapeSnapshot, "ShadowShapeSnapshot", ofs);
    ofs << "\n";
}

void dumpExplicitCoreBuilderReplayBundle(const ExplicitCoreBuilderReplayBundle &bundle,
                                         const std::string &path) {
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    std::ofstream ofs(path);
    ofs << "=== ExplicitCoreBuilderReplayBundle ===\n";
    ofs << "caseName=" << bundle.caseName << "\n";
    ofs << "manifestPath=" << bundle.manifestPath << "\n";
    ofs << "seed=" << bundle.seed << "\n";
    ofs << "tc=" << bundle.tc << "\n";
    if (bundle.targetStep.has_value()) {
        ofs << "targetStep=" << *bundle.targetStep << "\n";
    }
    ofs << "sourceStep=" << bundle.sourceStep << "\n";
    ofs << "sourceKind=" << bundle.sourceKind << "\n";
    ofs << "sourceSide=" << bundle.sourceSide << "\n";
    ofs << "topLevelOk=" << (bundle.topLevelOk ? 1 : 0) << "\n";
    ofs << "builderPipelineSeamKind="
        << builderPipelineSeamKindName(bundle.builderPipelineSeamKind) << "\n";
    ofs << "builderPipelineSeamWhy=" << bundle.builderPipelineSeamWhy << "\n";
    ofs << "sourceWhy=" << bundle.sourceWhy << "\n";
    ofs << "builderWhy=" << bundle.builderWhy << "\n\n";
    ofs << "=== SharedInputExplicit ===\n";
    dumpExplicitBlockGraph(bundle.inputExplicit, ofs);
    ofs << "\n";
    if (bundle.sourceKind == "handoff") {
        dumpStepTransitionSnapshot(bundle.solverStep1Snapshot, "SolverStep1Snapshot", ofs);
        ofs << "\n";
        dumpStepTransitionSnapshot(bundle.rewriteStep1Snapshot, "RewriteStep1Snapshot", ofs);
        ofs << "\n";
        dumpCoreShapeSnapshot(bundle.solverLiveCore, "SolverLiveCore", ofs);
        ofs << "\n";
        dumpCoreShapeSnapshot(bundle.replayLiveCore, "ReplayLiveCore", ofs);
        ofs << "\n";
    } else {
        dumpCoreShapeSnapshot(bundle.sourceLiveCore, "SourceLiveCore", ofs);
        ofs << "\n";
    }
    dumpBuilderStageSnapshot(bundle.rawSnapshot, "RawStageSnapshot", ofs);
    ofs << "\n";
    dumpBuilderStageSnapshot(bundle.miniBeforeSnapshot,
                             "MiniBeforeNormalizeSnapshot",
                             ofs);
    ofs << "\n";
    dumpBuilderStageSnapshot(bundle.miniAfterSnapshot,
                             "MiniAfterNormalizeSnapshot",
                             ofs);
    ofs << "\n";
    dumpBuilderStageSnapshot(bundle.coreAfterMaterializeSnapshot,
                             "CoreAfterMaterializeSnapshot",
                             ofs);
    ofs << "\n";
    dumpBuilderStageSnapshot(bundle.coreAfterNormalizeSnapshot,
                             "CoreAfterNormalizeSnapshot",
                             ofs);
    ofs << "\n";
}

void dumpHandoffRawReplayBundle(const HandoffRawReplayBundle &bundle,
                                const std::string &path) {
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    std::ofstream ofs(path);
    ofs << "=== HandoffRawReplayBundle ===\n";
    ofs << "caseName=" << bundle.caseName << "\n";
    ofs << "manifestPath=" << bundle.manifestPath << "\n";
    ofs << "seed=" << bundle.seed << "\n";
    ofs << "tc=" << bundle.tc << "\n";
    if (bundle.targetStep.has_value()) {
        ofs << "targetStep=" << *bundle.targetStep << "\n";
    }
    ofs << "sourceStep=" << bundle.sourceStep << "\n";
    ofs << "topLevelOk=" << (bundle.topLevelOk ? 1 : 0) << "\n";
    ofs << "rawReplaySeamKind=" << rawReplaySeamKindName(bundle.rawReplaySeamKind) << "\n";
    ofs << "rawReplaySeamWhy=" << bundle.rawReplaySeamWhy << "\n";
    ofs << "sourceWhy=" << bundle.sourceWhy << "\n";
    ofs << "rawWhy=" << bundle.rawWhy << "\n\n";
    ofs << "=== SharedHandoffExplicit ===\n";
    dumpExplicitBlockGraph(bundle.sharedHandoffExplicit, ofs);
    ofs << "\n";
    dumpCoreShapeSnapshot(bundle.sourceLiveCoreSnapshot, "SourceLiveCoreSnapshot", ofs);
    ofs << "\n";
    dumpRawReplaySnapshot(bundle.rawReplaySnapshot, "RawReplaySnapshot", ofs);
    ofs << "\n";
}

void dumpMaterializeCoreReplayBundle(const MaterializeCoreReplayBundle &bundle,
                                     const std::string &path) {
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    std::ofstream ofs(path);
    ofs << "=== MaterializeCoreReplayBundle ===\n";
    ofs << "caseName=" << bundle.caseName << "\n";
    ofs << "manifestPath=" << bundle.manifestPath << "\n";
    ofs << "seed=" << bundle.seed << "\n";
    ofs << "tc=" << bundle.tc << "\n";
    if (bundle.targetStep.has_value()) {
        ofs << "targetStep=" << *bundle.targetStep << "\n";
    }
    ofs << "sourceStep=" << bundle.sourceStep << "\n";
    ofs << "sourceSide=" << bundle.sourceSide << "\n";
    ofs << "topLevelOk=" << (bundle.topLevelOk ? 1 : 0) << "\n";
    ofs << "seamKind=" << coreMaterializeSubphaseSeamKindName(bundle.seamKind) << "\n";
    ofs << "seamWhy=" << bundle.seamWhy << "\n";
    ofs << "sourceWhy=" << bundle.sourceWhy << "\n";
    ofs << "builderWhy=" << bundle.builderWhy << "\n\n";
    ofs << "=== InputExplicit ===\n";
    dumpExplicitBlockGraph(bundle.inputExplicit, ofs);
    ofs << "\n";
    dumpCoreShapeSnapshot(bundle.sourceLiveCore, "SourceLiveCore", ofs);
    ofs << "\n";
    dumpBuilderStageSnapshot(bundle.miniAfterSnapshot, "MiniAfterNormalizeSnapshot", ofs);
    ofs << "\n";
    dumpMaterializeCoreSubphaseSnapshot(bundle.allocateSnapshot, "AllocateSnapshot", ofs);
    ofs << "\n";
    dumpMaterializeCoreSubphaseSnapshot(bundle.installSlotsSnapshot,
                                        "InstallSlotsSnapshot",
                                        ofs);
    ofs << "\n";
    dumpMaterializeCoreSubphaseSnapshot(bundle.connectArcsSnapshot,
                                        "ConnectArcsSnapshot",
                                        ofs);
    ofs << "\n";
    dumpMaterializeCoreSubphaseSnapshot(bundle.postMetadataSnapshot,
                                        "PostMetadataSnapshot",
                                        ofs);
    ofs << "\n";
    dumpMaterializeCoreSubphaseSnapshot(bundle.finalNormalizeSnapshot,
                                        "FinalNormalizeSnapshot",
                                        ofs);
    ofs << "\n";
}

void dumpSolverCompareReplayBundle(const SolverCompareReplayBundle &bundle,
                                   const std::string &path) {
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    std::ofstream ofs(path);
    ofs << "=== SolverCompareReplayBundle ===\n";
    ofs << "caseName=" << bundle.caseName << "\n";
    ofs << "manifestPath=" << bundle.manifestPath << "\n";
    ofs << "seed=" << bundle.seed << "\n";
    ofs << "tc=" << bundle.tc << "\n";
    if (bundle.targetStep.has_value()) {
        ofs << "targetStep=" << *bundle.targetStep << "\n";
    }
    ofs << "topLevelOk=" << (bundle.topLevelOk ? 1 : 0) << "\n";
    ofs << "compareAssemblySeamKind="
        << compareAssemblySeamKindName(bundle.compareAssemblySeamKind) << "\n";
    ofs << "compareAssemblyWhy=" << bundle.compareAssemblyWhy << "\n";
    ofs << "firstMismatchDescription=" << bundle.firstMismatchDescription << "\n";
    ofs << "oracleReplayTerminatedStep=" << bundle.oracleReplayTerminatedStep << "\n";
    ofs << "rewriteReplayTerminatedStep=" << bundle.rewriteReplayTerminatedStep << "\n";
    ofs << "oracleSolverWhy=" << bundle.oracleSolverWhy << "\n";
    ofs << "rewriteSolverWhy=" << bundle.rewriteSolverWhy << "\n";
    ofs << "oracleReplayWhy=" << bundle.oracleReplayWhy << "\n";
    ofs << "rewriteReplayWhy=" << bundle.rewriteReplayWhy << "\n";
    ofs << "rewriteSolverOutputDebugTag=" << bundle.rewriteSolverOutputDebugTag << "\n";
    ofs << "rewriteTerminalAssemblyWhy=" << bundle.rewriteTerminalAssemblyWhy << "\n";
    if (bundle.oracleSolverVsReplayEqualRaw.has_value()) {
        ofs << "oracleSolverVsReplayEqualRaw="
            << (*bundle.oracleSolverVsReplayEqualRaw ? 1 : 0) << "\n";
    }
    if (bundle.oracleSolverVsReplayEqualCanonical.has_value()) {
        ofs << "oracleSolverVsReplayEqualCanonical="
            << (*bundle.oracleSolverVsReplayEqualCanonical ? 1 : 0) << "\n";
    }
    if (bundle.rewriteSolverVsReplayEqualRaw.has_value()) {
        ofs << "rewriteSolverVsReplayEqualRaw="
            << (*bundle.rewriteSolverVsReplayEqualRaw ? 1 : 0) << "\n";
    }
    if (bundle.rewriteSolverVsReplayEqualCanonical.has_value()) {
        ofs << "rewriteSolverVsReplayEqualCanonical="
            << (*bundle.rewriteSolverVsReplayEqualCanonical ? 1 : 0) << "\n";
    }
    if (bundle.oracleVsRewriteEqualRaw.has_value()) {
        ofs << "oracleVsRewriteEqualRaw="
            << (*bundle.oracleVsRewriteEqualRaw ? 1 : 0) << "\n";
    }
    if (bundle.oracleVsRewriteEqualCanonical.has_value()) {
        ofs << "oracleVsRewriteEqualCanonical="
            << (*bundle.oracleVsRewriteEqualCanonical ? 1 : 0) << "\n";
    }
    ofs << "\n=== InputExplicit ===\n";
    dumpExplicitBlockGraph(bundle.inputExplicit, ofs);
    ofs << "\n";

    if (bundle.oracleSolverOutput.has_value()) {
        ofs << "=== OracleSolverOutput ===\n";
        dumpSolverOutput(*bundle.oracleSolverOutput, ofs);
        ofs << "\n";
    }
    if (bundle.oracleSolverOutputCanonical.has_value()) {
        ofs << "=== OracleSolverOutputCanonical ===\n";
        dumpCanonicalExplicitGraph(*bundle.oracleSolverOutputCanonical, ofs);
        ofs << "\n";
    }
    if (bundle.oracleReplayFinalRaw.has_value()) {
        ofs << "=== OracleReplayFinalRaw ===\n";
        dumpExplicitBlockGraph(*bundle.oracleReplayFinalRaw, ofs);
        ofs << "\n";
    }
    if (bundle.oracleReplayFinalCanonical.has_value()) {
        ofs << "=== OracleReplayFinalCanonical ===\n";
        dumpCanonicalExplicitGraph(*bundle.oracleReplayFinalCanonical, ofs);
        ofs << "\n";
    }

    if (bundle.rewriteSolverOutput.has_value()) {
        ofs << "=== RewriteSolverOutput ===\n";
        dumpSolverOutput(*bundle.rewriteSolverOutput, ofs);
        ofs << "\n";
    }
    if (bundle.rewriteSolverOutputCanonical.has_value()) {
        ofs << "=== RewriteSolverOutputCanonical ===\n";
        dumpCanonicalExplicitGraph(*bundle.rewriteSolverOutputCanonical, ofs);
        ofs << "\n";
    }
    if (bundle.rewriteReplayFinalRaw.has_value()) {
        ofs << "=== RewriteReplayFinalRaw ===\n";
        dumpExplicitBlockGraph(*bundle.rewriteReplayFinalRaw, ofs);
        ofs << "\n";
    }
    if (bundle.rewriteReplayFinalCanonical.has_value()) {
        ofs << "=== RewriteReplayFinalCanonical ===\n";
        dumpCanonicalExplicitGraph(*bundle.rewriteReplayFinalCanonical, ofs);
        ofs << "\n";
    }

    if (bundle.oracleReplay.has_value()) {
        dumpSemanticReplayResult(*bundle.oracleReplay, "OracleReplay", ofs);
        ofs << "\n";
    }
    if (bundle.rewriteReplay.has_value()) {
        dumpSemanticReplayResult(*bundle.rewriteReplay, "RewriteReplay", ofs);
        ofs << "\n";
    }
}

void dumpSolverBaselineReplayBundle(const SolverBaselineReplayBundle &bundle,
                                    const std::string &path) {
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    std::ofstream ofs(path);
    ofs << "=== SolverBaselineReplayBundle ===\n";
    ofs << "caseName=" << bundle.caseName << "\n";
    ofs << "manifestPath=" << bundle.manifestPath << "\n";
    ofs << "seed=" << bundle.seed << "\n";
    ofs << "tc=" << bundle.tc << "\n";
    if (bundle.targetStep.has_value()) {
        ofs << "targetStep=" << *bundle.targetStep << "\n";
    }
    if (bundle.stepIndex.has_value()) {
        ofs << "stepIndex=" << *bundle.stepIndex << "\n";
    }
    if (bundle.sequenceLengthSoFar.has_value()) {
        ofs << "sequenceLengthSoFar=" << *bundle.sequenceLengthSoFar << "\n";
    }
    if (bundle.chosenR.has_value()) {
        ofs << "chosenR=" << *bundle.chosenR << "\n";
    }
    if (bundle.chosenX.has_value()) {
        ofs << "chosenX=" << *bundle.chosenX << "\n";
    }
    ofs << "baselineStage=" << solverBaselineStageName(bundle.baselineStage) << "\n";
    ofs << "debugTag=" << bundle.debugTag << "\n";
    if (bundle.actualInvariantOk.has_value()) {
        ofs << "actualInvariantOk=" << (*bundle.actualInvariantOk ? 1 : 0) << "\n";
    }
    ofs << "actualInvariantWhy=" << bundle.actualInvariantWhy << "\n";
    ofs << "actualInvariantDetailedSubtype=" << bundle.actualInvariantDetailedSubtype
        << "\n";
    if (bundle.oracleEquivalentOk.has_value()) {
        ofs << "oracleEquivalentOk=" << (*bundle.oracleEquivalentOk ? 1 : 0) << "\n";
    }
    ofs << "oracleWhy=" << bundle.oracleWhy << "\n";
    if (bundle.firstFailingNodeId.has_value()) {
        ofs << "firstFailingNodeId=" << *bundle.firstFailingNodeId << "\n";
    }
    ofs << "firstFailingInvariantKind="
        << solverBaselineInvariantKindName(bundle.firstFailingInvariantKind) << "\n";
    ofs << "sameTypeSPPresent=" << (bundle.sameTypeSPPresent ? 1 : 0) << "\n";
    ofs << "adjacencyMismatchPresent=" << (bundle.adjacencyMismatchPresent ? 1 : 0)
        << "\n";
    ofs << "deadRelayCandidateNodes:";
    for (NodeId nodeId : bundle.deadRelayCandidateNodes) ofs << ' ' << nodeId;
    ofs << "\n\n";

    ofs << "=== ExplicitInput ===\n";
    dumpExplicitBlockGraph(bundle.explicitInput, ofs);
    ofs << "\n";

    if (bundle.explicitBefore.has_value()) {
        ofs << "=== ExplicitBefore ===\n";
        dumpExplicitBlockGraph(*bundle.explicitBefore, ofs);
        ofs << "\n";
    }
    if (bundle.explicitAfter.has_value()) {
        ofs << "=== ExplicitAfter ===\n";
        dumpExplicitBlockGraph(*bundle.explicitAfter, ofs);
        ofs << "\n";
    }
    if (bundle.actualBeforeRewrite.has_value()) {
        ofs << "=== ActualBeforeRewrite ===\n";
        dumpActualCore(*bundle.actualBeforeRewrite, ofs);
        ofs << "\n";
    }
    if (bundle.actualAfterRewrite.has_value()) {
        ofs << "=== ActualAfterRewrite ===\n";
        dumpActualCore(*bundle.actualAfterRewrite, ofs);
        ofs << "\n";
    }
    if (bundle.actualAfterNormalize.has_value()) {
        ofs << "=== ActualAfterNormalize ===\n";
        dumpActualCore(*bundle.actualAfterNormalize, ofs);
        ofs << "\n";
    }

    ofs << "stepSnapshots:\n";
    for (const auto &step : bundle.stepSnapshots) {
        ofs << " stepIndex=" << step.stepIndex
            << " sequenceLengthSoFar=" << step.sequenceLengthSoFar
            << " aliveNodeCount=" << step.aliveNodeCount
            << " currentRoot=" << step.currentRoot
            << " currentExplicitEdgeCount=" << step.currentExplicitEdgeCount << "\n";
        if (step.chosenR.has_value()) ofs << "  chosenR=" << *step.chosenR << "\n";
        if (step.chosenX.has_value()) ofs << "  chosenX=" << *step.chosenX << "\n";
        ofs << "  actualInvariantOk=" << (step.actualInvariantOk ? 1 : 0) << "\n";
        ofs << "  actualInvariantWhy=" << step.actualInvariantWhy << "\n";
        ofs << "  actualInvariantDetailedSubtype="
            << step.actualInvariantDetailedSubtype << "\n";
        if (step.firstFailingNodeId.has_value()) {
            ofs << "  firstFailingNodeId=" << *step.firstFailingNodeId << "\n";
        }
        ofs << "  firstFailingInvariantKind="
            << solverBaselineInvariantKindName(step.firstFailingInvariantKind) << "\n";
        ofs << "  sameTypeSPPresent=" << (step.sameTypeSPPresent ? 1 : 0) << "\n";
        ofs << "  adjacencyMismatchPresent="
            << (step.adjacencyMismatchPresent ? 1 : 0) << "\n";
        ofs << "  deadRelayCandidateNodes:";
        for (NodeId nodeId : step.deadRelayCandidateNodes) ofs << ' ' << nodeId;
        ofs << "\n";
        dumpSolverBaselineReplayPhaseSnapshots(step.phaseSnapshots,
                                               "  phaseSnapshots",
                                               ofs);
    }
}

static void dumpSemanticStepTrace(const SemanticStepTrace &trace, std::ostream &os) {
    os << "stepIndex=" << trace.stepIndex << "\n";
    os << "side=" << trace.side << "\n";
    os << "chosenR=" << trace.chosenR << "\n";
    os << "chosenX=" << trace.chosenX << "\n";
    os << "stepOk=" << (trace.stepOk ? 1 : 0) << "\n";
    os << "terminated=" << (trace.terminated ? 1 : 0) << "\n";
    os << "terminateReason=" << trace.terminateReason << "\n";
    os << "actualInvariantOk=" << (trace.actualInvariantOk ? 1 : 0) << "\n";
    os << "actualInvariantWhy=" << trace.actualInvariantWhy << "\n";
    os << "debugTag=" << trace.debugTag << "\n";
    os << "=== ExplicitBefore ===\n";
    dumpExplicitBlockGraph(trace.explicitBefore, os);
    os << "=== CanonicalExplicitBefore ===\n";
    dumpCanonicalExplicitGraph(trace.canonicalExplicitBefore, os);
    os << "=== ExplicitAfterDelete ===\n";
    dumpExplicitBlockGraph(trace.explicitAfterDelete, os);
    os << "=== CanonicalExplicitAfterDelete ===\n";
    dumpCanonicalExplicitGraph(trace.canonicalExplicitAfterDelete, os);
    os << "=== ExplicitAfterNormalize ===\n";
    dumpExplicitBlockGraph(trace.explicitAfterNormalize, os);
    os << "=== CanonicalExplicitAfterNormalize ===\n";
    dumpCanonicalExplicitGraph(trace.canonicalExplicitAfterNormalize, os);
}

void dumpSolverSemanticReplayBundle(const SolverSemanticReplayBundle &bundle,
                                    const std::string &path) {
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    std::ofstream ofs(path);
    ofs << "=== SolverSemanticReplayBundle ===\n";
    ofs << "caseName=" << bundle.caseName << "\n";
    ofs << "manifestPath=" << bundle.manifestPath << "\n";
    ofs << "stopPolicy=" << semanticReplayStopPolicyName(bundle.stopPolicy) << "\n";
    ofs << "topLevelOk=" << (bundle.topLevelOk ? 1 : 0) << "\n";
    ofs << "divergenceKind=" << semanticDivergenceKindName(bundle.divergenceKind) << "\n";
    ofs << "divergenceStepIndex=" << bundle.divergenceStepIndex << "\n";
    ofs << "divergenceWhy=" << bundle.divergenceWhy << "\n";
    ofs << "rawFirstDivergenceKind="
        << semanticDivergenceKindName(bundle.rawFirstDivergenceKind) << "\n";
    ofs << "rawFirstDivergenceStep=" << bundle.rawFirstDivergenceStep << "\n";
    ofs << "rawFirstDivergenceWhy=" << bundle.rawFirstDivergenceWhy << "\n";
    ofs << "canonicalFirstDivergenceKind="
        << canonicalDivergenceKindName(bundle.canonicalFirstDivergenceKind) << "\n";
    ofs << "canonicalFirstDivergenceStep=" << bundle.canonicalFirstDivergenceStep << "\n";
    ofs << "canonicalFirstDivergenceWhy=" << bundle.canonicalFirstDivergenceWhy << "\n";
    ofs << "oracleTerminatedStep=" << bundle.oracleTerminatedStep << "\n";
    ofs << "rewriteTerminatedStep=" << bundle.rewriteTerminatedStep << "\n";
    ofs << "firstOracleWhy=" << bundle.firstOracleWhy << "\n";
    ofs << "firstRewriteWhy=" << bundle.firstRewriteWhy << "\n\n";
    ofs << "canonicalEquivalent=" << (bundle.canonicalEquivalent ? 1 : 0) << "\n";
    ofs << "canonicalWhy=" << bundle.canonicalWhy << "\n";
    ofs << "finalCanonicalEquivalent=" << (bundle.finalCanonicalEquivalent ? 1 : 0)
        << "\n";
    ofs << "finalRawEquivalent=" << (bundle.finalRawEquivalent ? 1 : 0) << "\n\n";

    ofs << "=== ExplicitInput ===\n";
    dumpExplicitBlockGraph(bundle.explicitInput, ofs);
    ofs << "\n";
    if (bundle.oracleCanonicalExplicit.has_value()) {
        ofs << "=== OracleCanonicalExplicit ===\n";
        dumpCanonicalExplicitGraph(*bundle.oracleCanonicalExplicit, ofs);
        ofs << "\n";
    }
    if (bundle.rewriteCanonicalExplicit.has_value()) {
        ofs << "=== RewriteCanonicalExplicit ===\n";
        dumpCanonicalExplicitGraph(*bundle.rewriteCanonicalExplicit, ofs);
        ofs << "\n";
    }

    ofs << "oracleTrace:\n";
    for (const auto &trace : bundle.oracleTrace) {
        dumpSemanticStepTrace(trace, ofs);
        ofs << "\n";
    }
    ofs << "rewriteTrace:\n";
    for (const auto &trace : bundle.rewriteTrace) {
        dumpSemanticStepTrace(trace, ofs);
        ofs << "\n";
    }
}

void dumpSolverSemanticTargetReplayBundle(
    const SolverSemanticTargetReplayBundle &bundle,
    const std::string &path) {
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    std::ofstream ofs(path);
    ofs << "=== SolverSemanticTargetReplayBundle ===\n";
    ofs << "caseName=" << bundle.caseName << "\n";
    ofs << "manifestPath=" << bundle.manifestPath << "\n";
    ofs << "seed=" << bundle.seed << "\n";
    ofs << "tc=" << bundle.tc << "\n";
    if (bundle.targetStep.has_value()) {
        ofs << "targetStep=" << *bundle.targetStep << "\n";
    }
    ofs << "topLevelOk=" << (bundle.topLevelOk ? 1 : 0) << "\n";
    ofs << "semanticTargetSeamKind="
        << semanticTargetSeamKindName(bundle.semanticTargetSeamKind) << "\n";
    ofs << "divergenceStepIndex=" << bundle.divergenceStepIndex << "\n";
    ofs << "semanticTargetSeamWhy=" << bundle.semanticTargetSeamWhy << "\n";
    ofs << "oracleWhy=" << bundle.oracleWhy << "\n";
    ofs << "rewriteWhy=" << bundle.rewriteWhy << "\n";
    ofs << "shadowWhy=" << bundle.shadowWhy << "\n\n";
    ofs << "=== ExplicitInput ===\n";
    dumpExplicitBlockGraph(bundle.explicitInput, ofs);
    ofs << "\n";
    dumpSemanticTargetSnapshot(bundle.oracleTargetSnapshot, "OracleTargetSnapshot", ofs);
    ofs << "\n";
    dumpSemanticTargetSnapshot(bundle.rewriteTargetSnapshot, "RewriteTargetSnapshot", ofs);
    ofs << "\n";
    dumpSemanticTargetSnapshot(bundle.shadowTargetSnapshot, "ShadowTargetSnapshot", ofs);
    ofs << "\n";
    ofs << "oracleSemanticTrace:\n";
    for (const auto &trace : bundle.oracleSemanticTrace) {
        dumpSemanticStepTrace(trace, ofs);
        ofs << "\n";
    }
    ofs << "rewriteSemanticTrace:\n";
    for (const auto &trace : bundle.rewriteSemanticTrace) {
        dumpSemanticStepTrace(trace, ofs);
        ofs << "\n";
    }
}

void dumpSolverSemanticTransitionReplayBundle(
    const SolverSemanticTransitionReplayBundle &bundle,
    const std::string &path) {
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    std::ofstream ofs(path);
    ofs << "=== SolverSemanticTransitionReplayBundle ===\n";
    ofs << "caseName=" << bundle.caseName << "\n";
    ofs << "manifestPath=" << bundle.manifestPath << "\n";
    ofs << "seed=" << bundle.seed << "\n";
    ofs << "tc=" << bundle.tc << "\n";
    if (bundle.targetStep.has_value()) {
        ofs << "targetStep=" << *bundle.targetStep << "\n";
    }
    ofs << "sourceStep=" << bundle.sourceStep << "\n";
    ofs << "topLevelOk=" << (bundle.topLevelOk ? 1 : 0) << "\n";
    ofs << "transitionSeamKind="
        << transitionSeamKindName(bundle.transitionSeamKind) << "\n";
    ofs << "transitionSeamWhy=" << bundle.transitionSeamWhy << "\n";
    ofs << "sharedExplicitWhy=" << bundle.sharedExplicitWhy << "\n";
    ofs << "oracleWhy=" << bundle.oracleWhy << "\n";
    ofs << "rewriteWhy=" << bundle.rewriteWhy << "\n";
    ofs << "shadowWhy=" << bundle.shadowWhy << "\n\n";
    ofs << "=== SharedExplicitBefore ===\n";
    dumpExplicitBlockGraph(bundle.sharedExplicitBefore, ofs);
    ofs << "\n";
    ofs << "=== SharedExplicitBeforeCanonical ===\n";
    dumpCanonicalExplicitGraph(bundle.sharedExplicitBeforeCanonical, ofs);
    ofs << "\n";
    dumpTransitionReplaySnapshot(
        bundle.oracleTransitionSnapshot, "OracleTransitionSnapshot", ofs);
    ofs << "\n";
    dumpTransitionReplaySnapshot(
        bundle.rewriteTransitionSnapshot, "RewriteTransitionSnapshot", ofs);
    ofs << "\n";
    ofs << "=== ShadowDeleteExplicit ===\n";
    dumpCanonicalExplicitGraph(bundle.shadowDeleteExplicit, ofs);
}

void dumpSolverHandoffReplayBundle(const SolverHandoffReplayBundle &bundle,
                                   const std::string &path) {
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    std::ofstream ofs(path);
    ofs << "=== SolverHandoffReplayBundle ===\n";
    ofs << "caseName=" << bundle.caseName << "\n";
    ofs << "manifestPath=" << bundle.manifestPath << "\n";
    ofs << "seed=" << bundle.seed << "\n";
    ofs << "tc=" << bundle.tc << "\n";
    if (bundle.targetStep.has_value()) {
        ofs << "targetStep=" << *bundle.targetStep << "\n";
    }
    ofs << "topLevelOk=" << (bundle.topLevelOk ? 1 : 0) << "\n";
    ofs << "handoffStepIndex=" << bundle.handoffStepIndex << "\n";
    ofs << "handoffSeamKind=" << handoffSeamKindName(bundle.handoffSeamKind) << "\n";
    ofs << "handoffSeamWhy=" << bundle.handoffSeamWhy << "\n";
    ofs << "oracleNextMatchesTransitionShared="
        << (bundle.oracleNextMatchesTransitionShared ? 1 : 0) << "\n";
    ofs << "rewriteNextMatchesTransitionShared="
        << (bundle.rewriteNextMatchesTransitionShared ? 1 : 0) << "\n";
    ofs << "transitionSharedWhy=" << bundle.transitionSharedWhy << "\n";
    ofs << "oracleWhy=" << bundle.oracleWhy << "\n";
    ofs << "rewriteWhy=" << bundle.rewriteWhy << "\n\n";
    ofs << "=== TransitionSharedExplicit ===\n";
    dumpExplicitBlockGraph(bundle.transitionSharedExplicit, ofs);
    ofs << "\n";
    ofs << "=== TransitionSharedExplicitCanonical ===\n";
    dumpCanonicalExplicitGraph(bundle.transitionSharedExplicitCanonical, ofs);
    ofs << "\n";
    dumpStepHandoffSnapshot(bundle.oracleHandoffSnapshot, "OracleHandoffSnapshot", ofs);
    ofs << "\n";
    dumpStepHandoffSnapshot(bundle.rewriteHandoffSnapshot, "RewriteHandoffSnapshot", ofs);
    ofs << "\n";
    ofs << "oracleHandoffTrace:\n";
    for (const auto &snapshot : bundle.oracleHandoffTrace) {
        dumpStepHandoffSnapshot(snapshot, "OracleTraceStep", ofs);
        ofs << "\n";
    }
    ofs << "rewriteHandoffTrace:\n";
    for (const auto &snapshot : bundle.rewriteHandoffTrace) {
        dumpStepHandoffSnapshot(snapshot, "RewriteTraceStep", ofs);
        ofs << "\n";
    }
}

void dumpSolverHandoffPolicyReplayBundle(
    const SolverHandoffPolicyReplayBundle &bundle,
    const std::string &path) {
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    std::ofstream ofs(path);
    ofs << "=== SolverHandoffPolicyReplayBundle ===\n";
    ofs << "caseName=" << bundle.caseName << "\n";
    ofs << "manifestPath=" << bundle.manifestPath << "\n";
    ofs << "seed=" << bundle.seed << "\n";
    ofs << "tc=" << bundle.tc << "\n";
    if (bundle.targetStep.has_value()) {
        ofs << "targetStep=" << *bundle.targetStep << "\n";
    }
    ofs << "topLevelOk=" << (bundle.topLevelOk ? 1 : 0) << "\n";
    ofs << "oracleHandoffPolicy="
        << oracleHandoffPolicyName(bundle.oracleHandoffPolicy) << "\n";
    ofs << "deleteVsRewriteCanonicalEqual="
        << (bundle.deleteVsRewriteCanonicalEqual ? 1 : 0) << "\n";
    ofs << "normalizeVsRewriteCanonicalEqual="
        << (bundle.normalizeVsRewriteCanonicalEqual ? 1 : 0) << "\n";
    ofs << "deleteVsNormalizeCanonicalEqual="
        << (bundle.deleteVsNormalizeCanonicalEqual ? 1 : 0) << "\n";
    ofs << "oracleDeleteWhy=" << bundle.oracleDeleteWhy << "\n";
    ofs << "oracleNormalizeWhy=" << bundle.oracleNormalizeWhy << "\n";
    ofs << "rewriteWhy=" << bundle.rewriteWhy << "\n\n";
    ofs << "=== OracleDeletePolicyFinalExplicit ===\n";
    dumpCanonicalExplicitGraph(bundle.oracleDeletePolicyFinalExplicit, ofs);
    ofs << "\n=== OracleNormalizePolicyFinalExplicit ===\n";
    dumpCanonicalExplicitGraph(bundle.oracleNormalizePolicyFinalExplicit, ofs);
    ofs << "\n=== RewriteFinalExplicit ===\n";
    dumpCanonicalExplicitGraph(bundle.rewriteFinalExplicit, ofs);
    ofs << "\noracleDeleteTrace:\n";
    for (const auto &snapshot : bundle.oracleDeleteTrace) {
        dumpStepHandoffSnapshot(snapshot, "OracleDeleteTraceStep", ofs);
        ofs << "\n";
    }
    ofs << "oracleNormalizeTrace:\n";
    for (const auto &snapshot : bundle.oracleNormalizeTrace) {
        dumpStepHandoffSnapshot(snapshot, "OracleNormalizeTraceStep", ofs);
        ofs << "\n";
    }
    ofs << "rewriteTrace:\n";
    for (const auto &snapshot : bundle.rewriteTrace) {
        dumpStepHandoffSnapshot(snapshot, "RewriteTraceStep", ofs);
        ofs << "\n";
    }
}

void dumpSolverStepTransitionReplayBundle(
    const SolverStepTransitionReplayBundle &bundle,
    const std::string &path) {
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    std::ofstream ofs(path);
    ofs << "=== SolverStepTransitionReplayBundle ===\n";
    ofs << "caseName=" << bundle.caseName << "\n";
    ofs << "manifestPath=" << bundle.manifestPath << "\n";
    ofs << "seed=" << bundle.seed << "\n";
    ofs << "tc=" << bundle.tc << "\n";
    if (bundle.targetStep.has_value()) {
        ofs << "targetStep=" << *bundle.targetStep << "\n";
    }
    ofs << "topLevelOk=" << (bundle.topLevelOk ? 1 : 0) << "\n";
    ofs << "sourceStep=" << bundle.sourceStep << "\n";
    ofs << "stepTransitionSeamKind="
        << stepTransitionSeamKindName(bundle.stepTransitionSeamKind) << "\n";
    ofs << "stepTransitionSeamWhy=" << bundle.stepTransitionSeamWhy << "\n";
    ofs << "solverWhy=" << bundle.solverWhy << "\n";
    ofs << "rewriteWhy=" << bundle.rewriteWhy << "\n";
    ofs << "shadowWhy=" << bundle.shadowWhy << "\n\n";
    dumpStepTransitionSnapshot(bundle.solverStep1Snapshot, "SolverStep1Snapshot", ofs);
    ofs << "\n";
    dumpStepTransitionSnapshot(bundle.rewriteStep1Snapshot, "RewriteStep1Snapshot", ofs);
    if (bundle.hasShadowStep2Snapshot) {
        ofs << "\n";
        dumpStepTransitionSnapshot(bundle.shadowStep2Snapshot, "ShadowStep2Snapshot", ofs);
    }
}

void dumpHarnessBundle(const HarnessBundle &B, const std::string &path) {
    std::ofstream ofs(path);
    ofs << "=== HarnessBundle ===\n";
    ofs << "seed=" << B.seed << "\n";
    ofs << "tc=" << B.tc << "\n";
    ofs << "backend=" << B.backendName << "\n";
    ofs << "stage=" << stageName(B.stage) << "\n";
    ofs << "where=" << B.where << "\n";
    ofs << "why=" << B.why << "\n\n";
    if (B.targetTcIndex.has_value()) ofs << "targetTcIndex=" << *B.targetTcIndex << "\n";
    if (B.targetStep.has_value()) ofs << "targetStep=" << *B.targetStep << "\n";
    if (B.stepIndex.has_value()) ofs << "stepIndex=" << *B.stepIndex << "\n";
    if (B.sequenceLengthSoFar.has_value()) {
        ofs << "sequenceLengthSoFar=" << *B.sequenceLengthSoFar << "\n";
    }
    if (B.chosenR.has_value()) ofs << "chosenR=" << *B.chosenR << "\n";
    if (B.chosenX.has_value()) ofs << "chosenX=" << *B.chosenX << "\n";
    if (B.postcheckSubtype) {
        ofs << "postcheckSubtype=" << graftPostcheckSubtypeName(*B.postcheckSubtype) << "\n";
    }
    if (B.normalizeOk) {
        ofs << "normalizeOk=" << (*B.normalizeOk ? 1 : 0) << "\n";
        ofs << "normalizeWhy=" << B.normalizeWhy << "\n";
    }
    if (B.actualInvariantOk) {
        ofs << "actualInvariantOk=" << (*B.actualInvariantOk ? 1 : 0) << "\n";
        ofs << "actualInvariantWhy=" << B.actualInvariantWhy << "\n";
    }
    if (B.oracleBuildOk) {
        ofs << "oracleBuildOk=" << (*B.oracleBuildOk ? 1 : 0) << "\n";
    }
    if (B.oracleEquivalentOk) {
        ofs << "oracleEquivalentOk=" << (*B.oracleEquivalentOk ? 1 : 0) << "\n";
    }
    if (!B.oracleWhy.empty()) {
        ofs << "oracleWhy=" << B.oracleWhy << "\n";
    }
    if (B.targetTcIndex.has_value() || B.targetStep.has_value() ||
        B.stepIndex.has_value() || B.sequenceLengthSoFar.has_value() ||
        B.chosenR.has_value() || B.chosenX.has_value() || B.postcheckSubtype ||
        B.normalizeOk || B.actualInvariantOk || B.oracleBuildOk || B.oracleEquivalentOk ||
        !B.oracleWhy.empty()) {
        ofs << "\n";
    }
    if (B.explicitInput) { ofs << "=== ExplicitInput ===\n"; dumpExplicitBlockGraph(*B.explicitInput, ofs); ofs << "\n"; }
    if (B.compact) { dumpCompactGraph(*B.compact, ofs); ofs << "\n"; }
    if (B.raw) { dumpRawSpqrDecomp(*B.raw, ofs); ofs << "\n"; }
    if (B.miniBeforeNormalize) { ofs << "=== MiniBeforeNormalize ===\n"; dumpStaticMiniCore(*B.miniBeforeNormalize, ofs); ofs << "\n"; }
    if (B.miniAfterNormalize) { ofs << "=== MiniAfterNormalize ===\n"; dumpStaticMiniCore(*B.miniAfterNormalize, ofs); ofs << "\n"; }
    if (B.actualBeforeGraft) { ofs << "=== ActualBeforeGraft ===\n"; dumpActualCore(*B.actualBeforeGraft, ofs); ofs << "\n"; }
    if (B.actualAfterGraft) { ofs << "=== ActualAfterGraft ===\n"; dumpActualCore(*B.actualAfterGraft, ofs); ofs << "\n"; }
    if (B.actualBeforeRewrite) { ofs << "=== ActualBeforeRewrite ===\n"; dumpActualCore(*B.actualBeforeRewrite, ofs); ofs << "\n"; }
    if (B.actualAfterRewrite) { ofs << "=== ActualAfterRewrite ===\n"; dumpActualCore(*B.actualAfterRewrite, ofs); ofs << "\n"; }
    if (B.actualAfterNormalize) { ofs << "=== ActualAfterNormalize ===\n"; dumpActualCore(*B.actualAfterNormalize, ofs); ofs << "\n"; }
    if (B.trace) { dumpGraftTrace(*B.trace, ofs); ofs << "\n"; }
    if (!B.oldNodeSnapshotsByPhase.empty()) {
        dumpReplayNodeSnapshotPhases(B.oldNodeSnapshotsByPhase,
                                     "bundle.oldNodeSnapshotsByPhase",
                                     ofs);
        ofs << "\n";
    }
    if (!B.affectedNodeSnapshotsByPhase.empty()) {
        dumpReplayNodeSnapshotPhases(B.affectedNodeSnapshotsByPhase,
                                     "bundle.affectedNodeSnapshotsByPhase",
                                     ofs);
        ofs << "\n";
    }
    if (B.explicitBefore) { ofs << "=== ExplicitBefore ===\n"; dumpExplicitBlockGraph(*B.explicitBefore, ofs); ofs << "\n"; }
    if (B.explicitAfter) { ofs << "=== ExplicitAfter ===\n"; dumpExplicitBlockGraph(*B.explicitAfter, ofs); ofs << "\n"; }
    if (B.explicitAfterNormalize) { ofs << "=== ExplicitAfterNormalize ===\n"; dumpExplicitBlockGraph(*B.explicitAfterNormalize, ofs); ofs << "\n"; }
    if (B.explicitExpected) { ofs << "=== ExplicitExpected ===\n"; dumpExplicitBlockGraph(*B.explicitExpected, ofs); ofs << "\n"; }
    if (B.explicitGot) { ofs << "=== ExplicitGot ===\n"; dumpExplicitBlockGraph(*B.explicitGot, ofs); ofs << "\n"; }
}

} // namespace harness
