# Next Scope Candidate Inventory 90

## purpose

This inventory defines the next-scope candidates after support8 closure and family-chain lower-layer closure.

## candidates

| candidate key | current status | can start immediately | recommended priority | reason |
| --- | --- | --- | --- | --- |
| `shell16_readiness` | not attempted; optional shell16 ledger exists | preflight only | 2 | concrete extension path, but needs artifact/doc/runtime contract before scan |
| `higher_support_necessity` | not implemented and not currently justified | no | 4 | support9+ should follow a theoretical bound or shell16 result, not precede it |
| `broader_general_gap_theorem` | ready for bridge formalization, not proof attempt | yes | 1 | determines whether shell16 or higher support is actually needed |
| `BOJ_solver_bridge` | bridge notes exist; solver not ready | bridge formalization only | 3 | useful after theorem bridge shape is clearer; no solver implementation yet |
| `archive_wide_history_provenance_cleanup` | nonblocking archival caveat | yes, low urgency | 5 | improves history hygiene but does not advance next theorem scope |

## inventory conclusion

The next exact target should be `general_gap_bridge_formalization`. It is the only candidate that can decide the dependency order among shell16, higher-support, and BOJ bridge work without opening a new expensive scan prematurely.
