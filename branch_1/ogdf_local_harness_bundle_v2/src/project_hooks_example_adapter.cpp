#include "harness/project_hooks.hpp"

namespace harness {

// Example adapter: replace each TODO with your project's actual implementation.
// This file is intentionally NOT built by default.
class ExampleProjectHarnessOps final : public IHarnessOps {
public:
    const char *name() const override { return "example-project-adapter"; }

    bool validateRawSpqrDecomp(const CompactGraph &, const RawSpqrDecomp &, std::string &why) override {
        why = "TODO: wire validateRawSpqrDecomp from your codebase";
        return false;
    }
    bool materializeMiniCore(const CompactGraph &, const RawSpqrDecomp &, StaticMiniCore &, std::string &why) override {
        why = "TODO: wire materializeMiniCore(H, raw, mini, why)";
        return false;
    }
    void normalizeWholeMiniCore(StaticMiniCore &) override {
        // TODO: wire normalizeWholeMiniCore(mini)
    }
    bool checkMiniOwnershipConsistency(const StaticMiniCore &, const CompactGraph &, std::string &why) override {
        why = "TODO: wire checkOwnershipConsistency(mini, H, why)";
        return false;
    }
    bool checkMiniReducedInvariant(const StaticMiniCore &, const CompactGraph &, std::string &why) override {
        why = "TODO: wire checkReducedInvariant(mini, H, why)";
        return false;
    }
    bool buildDummyActualCoreEnvelope(const CompactGraph &, DummyActualEnvelope &, std::string &why) override {
        why = "TODO: wire buildDummyActualCoreEnvelope(H, env, why)";
        return false;
    }
    bool chooseKeepMiniNode(const StaticMiniCore &, int &keep, std::string &why) override {
        keep = -1;
        why = "TODO: wire chooseKeepMiniNode(mini, keep, why)";
        return false;
    }
    bool graftMiniCoreIntoPlace(ReducedSPQRCore &, NodeId, const CompactGraph &, const StaticMiniCore &, int, std::queue<NodeId> &, GraftTrace *, std::string &why) override {
        why = "TODO: wire graftMiniCoreIntoPlace(core, oldR, H, mini, keep, q, trace, why)";
        return false;
    }
    bool rebuildActualMetadata(ReducedSPQRCore &, std::string &why) override {
        why = "TODO: wire rebuildActualMetadata(core, why)";
        return false;
    }
    bool checkActualReducedInvariant(const ReducedSPQRCore &, const std::unordered_set<NodeId> *, std::string &why) override {
        why = "TODO: wire checkReducedInvariantActual(core, allowedStubNodes, why)";
        return false;
    }
    ExplicitBlockGraph materializeWholeCoreExplicit(const ReducedSPQRCore &) override {
        return {};
    }
    ExplicitBlockGraph materializeCompactRealProjection(const CompactGraph &) override {
        return {};
    }
    bool checkEquivalentExplicitGraphs(const ExplicitBlockGraph &, const ExplicitBlockGraph &, std::string &why) override {
        why = "TODO: wire checkEquivalentExplicitGraphs(got, exp, why)";
        return false;
    }
    bool checkDummyProxyRewire(const DummyActualEnvelope &, const StaticMiniCore &, const GraftTrace &, std::string &why) override {
        why = "TODO: wire checkDummyProxyRewire(env, mini, trace, why)";
        return false;
    }
};

} // namespace harness
