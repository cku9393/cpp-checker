#pragma once

#include <string>
#include <vector>

#include "state_dump.hpp"

struct PortSignature {
    int kind = 0;
    Vertex attachOrig = NIL_U32;
    BridgeRef br = 0;
    u8 side = 0;

    bool operator==(const PortSignature& rhs) const;
    bool operator<(const PortSignature& rhs) const;
};

struct TinyGraphSignature {
    std::vector<Vertex> verts;
    std::vector<std::pair<Vertex, Vertex>> edges;

    bool operator==(const TinyGraphSignature& rhs) const;
};

struct IsolatePreparedSignature {
    Vertex orig = NIL_U32;
    std::vector<OccID> allocNbr;
    std::vector<PortSignature> ports;
    TinyGraphSignature core;

    bool operator==(const IsolatePreparedSignature& rhs) const;
};

struct CanonicalSkeletonSignature {
    std::vector<std::string> vertices;
    std::vector<std::string> edges;
    std::vector<OccID> hostedOcc;

    bool operator==(const CanonicalSkeletonSignature& rhs) const;
    bool operator<(const CanonicalSkeletonSignature& rhs) const;
};

struct CanonicalOccurrenceSignature {
    OccID occ = 0;
    Vertex orig = NIL_U32;
    std::string hostSkel;
    std::string centerV;
    std::vector<OccID> allocNbr;
    std::vector<std::string> corePatchEdges;

    bool operator==(const CanonicalOccurrenceSignature& rhs) const;
    bool operator<(const CanonicalOccurrenceSignature& rhs) const;
};

struct EngineStateSignature {
    std::vector<CanonicalSkeletonSignature> skeletons;
    std::vector<CanonicalOccurrenceSignature> occurrences;

    bool operator==(const EngineStateSignature& rhs) const;
};

struct SplitChildSignature {
    bool boundaryOnly = false;
    std::vector<OccID> hostedOcc;
    CanonicalSkeletonSignature graph;

    bool operator==(const SplitChildSignature& rhs) const;
    bool operator<(const SplitChildSignature& rhs) const;
};

struct SplitResultSignature {
    Vertex aOrig = NIL_U32;
    Vertex bOrig = NIL_U32;
    std::vector<SplitChildSignature> child;

    bool operator==(const SplitResultSignature& rhs) const;
};

struct MergeResultSignature {
    CanonicalSkeletonSignature mergedSkeleton;
    std::vector<CanonicalOccurrenceSignature> hostedOccurrence;

    bool operator==(const MergeResultSignature& rhs) const;
};

IsolatePreparedSignature capture_isolate_signature(const IsolatePrepared& prep);
SplitResultSignature capture_split_result_signature(const RawEngine& RE, const SplitSeparationPairResult& result);
MergeResultSignature capture_join_result_signature(const RawEngine& RE, const JoinSeparationPairResult& result);
MergeResultSignature capture_integrate_result_signature(const RawEngine& RE, const IntegrateResult& result);
EngineStateSignature capture_engine_state_signature(const RawEngine& RE);

IsolatePrepared reference_prepare_isolate_neighborhood(const RawEngine& before, RawSkelID sid, OccID occ);
IsolateVertexResult reference_isolate_vertex(RawEngine& before, RawSkelID sid, OccID occ);
SplitSeparationPairResult reference_split_separation_pair(RawEngine& before, RawSkelID sid, Vertex saOrig, Vertex sbOrig);
JoinSeparationPairResult reference_join_separation_pair(
    RawEngine& before,
    RawSkelID leftSid,
    RawSkelID rightSid,
    Vertex saOrig,
    Vertex sbOrig
);
IntegrateResult reference_integrate_skeleton(
    RawEngine& before,
    RawSkelID parentSid,
    RawSkelID childSid,
    const std::vector<BoundaryMapEntry>& bm
);

std::string describe_signature(const IsolatePreparedSignature& sig);
std::string describe_signature(const SplitResultSignature& sig);
std::string describe_signature(const MergeResultSignature& sig);
std::string describe_signature(const EngineStateSignature& sig);

bool check_prepare_isolate_oracle(
    const RawEngine& before,
    RawSkelID sid,
    OccID occ,
    const IsolatePrepared& actual,
    std::string* failure
);

bool check_split_oracle(
    const RawEngine& before,
    const RawEngine& after,
    const PrimitiveInvocation& invocation,
    const SplitSeparationPairResult& actual,
    std::string* failure
);

bool check_join_oracle(
    const RawEngine& before,
    const RawEngine& after,
    const PrimitiveInvocation& invocation,
    const JoinSeparationPairResult& actual,
    std::string* failure
);

bool check_integrate_oracle(
    const RawEngine& before,
    const RawEngine& after,
    const PrimitiveInvocation& invocation,
    const IntegrateResult& actual,
    std::string* failure
);
