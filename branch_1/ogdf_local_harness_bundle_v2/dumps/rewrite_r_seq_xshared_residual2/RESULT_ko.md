# rewrite-r-seq xshared residual2 result

- correctness: green
- behavior change: none
- aggregate strategy: s1_r100 + full_s1_10_r1000(seed1..10) + full_s1_20_r5000(seed1..20), excluding duplicate s1_r1000

## Current optimized baseline
- rewriteSeqCalls: 110100
- seqFallbackCaseCount: 2059
- seqRewriteWholeCoreFallbackCount: 3977

## Dominant remaining trigger
- current dominant remaining trigger: RFT_X_INCIDENT_VIRTUAL_UNSUPPORTED
- trigger count: 1356
- representative residual subtype: XSR_HAFTER_LOOP_SHARED (1235)

## Loopshared residual split
- dominant input subtype: XLSI_PROXY_LOOP_REAL_EDGE (1235)
- dominant bailout: XLSB_GRAFT_FAIL (1235)
- dominant combo: XSR_HAFTER_LOOP_SHARED + XLSI_PROXY_LOOP_REAL_EDGE + XLSB_GRAFT_FAIL = 1235
- residual side branch still present: XSR_HAFTER_SPQR_READY = 121

## Decision
- next safe fix target: loopshared graft 안정화
- rationale: XSR_HAFTER_LOOP_SHARED is entirely concentrated in XLSI_PROXY_LOOP_REAL_EDGE and fails at XLSB_GRAFT_FAIL
- this stage changed only instrumentation and preserved green correctness

## Sample dumps
- input sample: dumps/rewrite_r_seq_xshared_residual2/seq_xshared_loopshared_input_XLSI_PROXY_LOOP_REAL_EDGE_seed1_tc56_step1.txt
- bailout sample: dumps/rewrite_r_seq_xshared_residual2/seq_xshared_loopshared_bailout_XLSB_GRAFT_FAIL_seed1_tc56_step1.txt
