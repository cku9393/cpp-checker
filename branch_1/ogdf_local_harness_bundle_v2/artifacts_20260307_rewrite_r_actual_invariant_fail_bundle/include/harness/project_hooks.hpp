#pragma once
#include "harness/types.hpp"

namespace harness {

class IHarnessOps {
public:
    virtual ~IHarnessOps() = default;
    virtual const char *name() const = 0;

    virtual bool validateRawSpqrDecomp(const CompactGraph &H,
                                       const RawSpqrDecomp &raw,
                                       std::string &why) = 0;

    virtual bool materializeMiniCore(const CompactGraph &H,
                                     const RawSpqrDecomp &raw,
                                     StaticMiniCore &mini,
                                     std::string &why) = 0;

    virtual void normalizeWholeMiniCore(StaticMiniCore &mini) = 0;

    virtual bool checkMiniOwnershipConsistency(const StaticMiniCore &mini,
                                               const CompactGraph &H,
                                               std::string &why) = 0;
    virtual bool checkMiniReducedInvariant(const StaticMiniCore &mini,
                                           const CompactGraph &H,
                                           std::string &why) = 0;

    virtual bool buildDummyActualCoreEnvelope(const CompactGraph &H,
                                              DummyActualEnvelope &env,
                                              std::string &why) = 0;

    virtual bool chooseKeepMiniNode(const StaticMiniCore &mini,
                                    int &keep,
                                    std::string &why) = 0;

    virtual bool graftMiniCoreIntoPlace(ReducedSPQRCore &core,
                                        NodeId oldR,
                                        const CompactGraph &H,
                                        const StaticMiniCore &mini,
                                        int keep,
                                        std::queue<NodeId> &q,
                                        GraftTrace *trace,
                                        std::string &why) = 0;

    virtual bool rebuildActualMetadata(ReducedSPQRCore &core,
                                       std::string &why) = 0;

    virtual bool checkActualReducedInvariant(const ReducedSPQRCore &core,
                                             const std::unordered_set<NodeId> *allowedStubNodes,
                                             std::string &why) = 0;

    virtual ExplicitBlockGraph materializeWholeCoreExplicit(const ReducedSPQRCore &core) = 0;
    virtual ExplicitBlockGraph materializeCompactRealProjection(const CompactGraph &H) = 0;

    virtual bool checkEquivalentExplicitGraphs(const ExplicitBlockGraph &got,
                                               const ExplicitBlockGraph &exp,
                                               std::string &why) = 0;

    virtual bool checkDummyProxyRewire(const DummyActualEnvelope &env,
                                       const StaticMiniCore &mini,
                                       const GraftTrace &trace,
                                       std::string &why) = 0;

    virtual bool rewriteRFallback(ReducedSPQRCore &core,
                                  NodeId rNode,
                                  VertexId x,
                                  std::string &why) = 0;

    virtual bool normalizeTouchedRegion(ReducedSPQRCore &core,
                                        std::string &why) = 0;
};

class StubHarnessOps final : public IHarnessOps {
public:
    const char *name() const override { return "stub-hooks"; }

    bool validateRawSpqrDecomp(const CompactGraph &, const RawSpqrDecomp &, std::string &why) override;
    bool materializeMiniCore(const CompactGraph &, const RawSpqrDecomp &, StaticMiniCore &, std::string &why) override;
    void normalizeWholeMiniCore(StaticMiniCore &mini) override;
    bool checkMiniOwnershipConsistency(const StaticMiniCore &, const CompactGraph &, std::string &why) override;
    bool checkMiniReducedInvariant(const StaticMiniCore &, const CompactGraph &, std::string &why) override;
    bool buildDummyActualCoreEnvelope(const CompactGraph &, DummyActualEnvelope &, std::string &why) override;
    bool chooseKeepMiniNode(const StaticMiniCore &, int &keep, std::string &why) override;
    bool graftMiniCoreIntoPlace(ReducedSPQRCore &, NodeId, const CompactGraph &, const StaticMiniCore &, int, std::queue<NodeId> &, GraftTrace *, std::string &why) override;
    bool rebuildActualMetadata(ReducedSPQRCore &, std::string &why) override;
    bool checkActualReducedInvariant(const ReducedSPQRCore &, const std::unordered_set<NodeId> *, std::string &why) override;
    ExplicitBlockGraph materializeWholeCoreExplicit(const ReducedSPQRCore &) override;
    ExplicitBlockGraph materializeCompactRealProjection(const CompactGraph &) override;
    bool checkEquivalentExplicitGraphs(const ExplicitBlockGraph &, const ExplicitBlockGraph &, std::string &why) override;
    bool checkDummyProxyRewire(const DummyActualEnvelope &, const StaticMiniCore &, const GraftTrace &, std::string &why) override;
    bool rewriteRFallback(ReducedSPQRCore &, NodeId, VertexId, std::string &why) override;
    bool normalizeTouchedRegion(ReducedSPQRCore &, std::string &why) override;
};

class ProjectHarnessOps final : public IHarnessOps {
public:
    const char *name() const override { return "project-hooks"; }

    bool validateRawSpqrDecomp(const CompactGraph &, const RawSpqrDecomp &, std::string &why) override;
    bool materializeMiniCore(const CompactGraph &, const RawSpqrDecomp &, StaticMiniCore &, std::string &why) override;
    void normalizeWholeMiniCore(StaticMiniCore &mini) override;
    bool checkMiniOwnershipConsistency(const StaticMiniCore &, const CompactGraph &, std::string &why) override;
    bool checkMiniReducedInvariant(const StaticMiniCore &, const CompactGraph &, std::string &why) override;
    bool buildDummyActualCoreEnvelope(const CompactGraph &, DummyActualEnvelope &, std::string &why) override;
    bool chooseKeepMiniNode(const StaticMiniCore &, int &keep, std::string &why) override;
    bool graftMiniCoreIntoPlace(ReducedSPQRCore &, NodeId, const CompactGraph &, const StaticMiniCore &, int, std::queue<NodeId> &, GraftTrace *, std::string &why) override;
    bool rebuildActualMetadata(ReducedSPQRCore &, std::string &why) override;
    bool checkActualReducedInvariant(const ReducedSPQRCore &, const std::unordered_set<NodeId> *, std::string &why) override;
    ExplicitBlockGraph materializeWholeCoreExplicit(const ReducedSPQRCore &) override;
    ExplicitBlockGraph materializeCompactRealProjection(const CompactGraph &) override;
    bool checkEquivalentExplicitGraphs(const ExplicitBlockGraph &, const ExplicitBlockGraph &, std::string &why) override;
    bool checkDummyProxyRewire(const DummyActualEnvelope &, const StaticMiniCore &, const GraftTrace &, std::string &why) override;
    bool rewriteRFallback(ReducedSPQRCore &, NodeId, VertexId, std::string &why) override;
    bool normalizeTouchedRegion(ReducedSPQRCore &, std::string &why) override;
};

} // namespace harness
