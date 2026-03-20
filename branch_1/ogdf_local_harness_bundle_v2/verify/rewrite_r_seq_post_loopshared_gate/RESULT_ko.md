# rewrite-r-seq post-loopshared gate

- correctness: green
- gate decision: stop here
- reason: dominant remaining trigger is not `RFT_COMPACT_EMPTY_AFTER_DELETE`
- dominant remaining trigger: `RFT_COMPACT_BUILD_FAIL`

## Current Totals
- `rewriteSeqCalls=111100`
- `rewriteSeqSucceededCases=111100`
- `rewriteSeqFailedCases=0`
- `seqFallbackCaseCount=18034`
- `seqRewriteWholeCoreFallbackCount=27698`
- `seqLoopPlusEdgeSharedHandledCount=9968`
- `seqTooSmallOtherHandledCount=9968`
- `rewriteFallbackSpecialCaseCount=86747`

## Remaining Trigger Order
- `RFT_COMPACT_BUILD_FAIL=6969`
- `RFT_X_INCIDENT_VIRTUAL_UNSUPPORTED=6247`
- `RFT_COMPACT_TOO_SMALL_UNHANDLED=6212`
- `RFT_GRAFT_REWIRE_FAIL=4818`
- `RFT_COMPACT_EMPTY_AFTER_DELETE=3452`

## Decision
- `RFT_COMPACT_EMPTY_AFTER_DELETE` is not dominant.
- Per gate rule, do not start `EMPTY_AFTER_DELETE` subtype instrumentation in this turn.
- Next target should be `RFT_COMPACT_BUILD_FAIL`.

## Suggested Next Plan
- Split `RFT_COMPACT_BUILD_FAIL` by concrete local compact build failure cause.
- Prioritize `SELF_LOOP`, `X_NOT_PRESENT_IN_R`, `OWNER_NOT_R`, and residual builder failures separately.
