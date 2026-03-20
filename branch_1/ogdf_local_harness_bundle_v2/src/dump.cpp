#include "harness/dump.hpp"
#include "harness/project_static_adapter.hpp"
#include <iomanip>
#include <sstream>

namespace harness {

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
