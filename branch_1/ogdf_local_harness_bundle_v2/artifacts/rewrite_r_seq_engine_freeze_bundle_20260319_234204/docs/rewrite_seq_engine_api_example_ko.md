# Rewrite Sequence Engine API Example

```cpp
#include "harness/project_static_adapter.hpp"

harness::ReducedSPQRCore core = buildInitialCoreSomehow();
harness::RewriteSeqStats seqStats;
std::string why;

const bool ok = harness::runRewriteSequenceToFixpoint(core, seqStats, why);
if (!ok) {
    std::cerr << "rewrite sequence failed at stage="
              << harness::stageName(seqStats.failureStage)
              << " where=" << seqStats.failureWhere
              << " why=" << why << "\n";
    return;
}

std::cout << "rewrite sequence reached fixpoint after "
          << seqStats.completedSteps << " step(s)\n";
```

## Notes

- 이 API는 이미 build된 `ReducedSPQRCore`를 받아 fixpoint까지 rewrite + normalize를 반복 적용한다.
- 현재 `rewrite-r-seq` runner와 같은 deterministic target 선택 규칙을 사용한다.
- 실패 시 `why`와 `RewriteSeqStats.failure*` 필드를 함께 본다.
- solver 통합 전까지는 `USE_REWRITE_SEQ_ENGINE` feature flag 뒤에 두는 것이 맞다.
