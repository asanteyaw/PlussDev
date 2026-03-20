# CodeN CPU Performance Execution Tracker

## Program Goal
Execute the CodeN CPU performance blueprint in controlled, test-first patches.

## Success Definition
- Correctness parity with reference behavior on all covered cases.
- Measurable speedups on targeted operations.
- No API breakage for existing users.

## Status Legend
- `[ ]` Not started
- `[/]` In progress
- `[x]` Completed
- `[!]` Blocked

## Patch 01: Baseline + Harness
- Status: `[ ]`
- Goal: Establish trustworthy baselines and regression corpus.
- Tasks:
  - `[ ]` Add microbench for `Note::sort`.
  - `[ ]` Add microbench for `Note::distinct`.
  - `[ ]` Add microbench for `Note::tally`/count-by path.
  - `[ ]` Add microbench for `Note::take(indices)`.
  - `[ ]` Add correctness corpus with nulls, repeated labels, mixed lengths, UTF-8.
  - `[ ]` Store baseline numbers in docs/artifacts.
- Gate:
  - `[ ]` Baseline report committed and reproducible.

## Patch 02: Public `Note::argsort`
- Status: `[ ]`
- Goal: Provide permutation primitive (`Series<int64>`) for Note.
- Tasks:
  - `[ ]` Add API surface `Note::argsort(...)`.
  - `[ ]` Add dispatch wiring.
  - `[ ]` Add tests for ordering and stability.
  - `[ ]` Add tests for null behavior policy.
- Gate:
  - `[ ]` New API passes tests and docs mention behavior.

## Patch 03: Fast `Note::take(indices)`
- Status: `[ ]`
- Goal: Fast gather-by-permutation kernel for spans.
- Tasks:
  - `[ ]` Implement optimized gather for `OffLen`.
  - `[ ]` Avoid unnecessary string materialization.
  - `[ ]` Add bounds and error-path tests.
  - `[ ]` Benchmark vs previous path.
- Gate:
  - `[ ]` `take` shows measurable speedup on benchmark corpus.

## Patch 04: Compare/Equality/Hash Kernels
- Status: `[ ]`
- Goal: Reusable span-aware kernels for hot ops.
- Tasks:
  - `[ ]` Implement lexical compare kernel.
  - `[ ]` Implement equality kernel.
  - `[ ]` Implement hash kernel with collision-safe validation path.
  - `[ ]` Add unit tests across UTF-8 and edge cases.
- Gate:
  - `[ ]` Kernel outputs match reference behavior exactly.

## Patch 05: `sort` Delegates to `argsort + take`
- Status: `[ ]`
- Goal: Single canonical sort pipeline.
- Tasks:
  - `[ ]` Refactor `Note::sort` to use `argsort + take`.
  - `[ ]` Preserve stability and null ordering semantics.
  - `[ ]` Remove duplicated legacy sort logic where safe.
  - `[ ]` Add regression tests for deterministic ordering.
- Gate:
  - `[ ]` Functional parity confirmed and perf non-regressive.

## Patch 06: Encoded Fast Path
- Status: `[ ]`
- Goal: Integer-code acceleration for repetitive text.
- Tasks:
  - `[ ]` Add optional dictionary/interned code representation.
  - `[ ]` Add heuristic to enable encoded path only when beneficial.
  - `[ ]` Keep original string outputs unchanged.
  - `[ ]` Add parity tests under encoded and non-encoded routes.
- Gate:
  - `[ ]` Encoded mode speedup proven on repetitive categorical datasets.

## Patch 07: `distinct` Optimization
- Status: `[ ]`
- Goal: Accelerate `distinct` with code/hash route.
- Tasks:
  - `[ ]` Wire `distinct` through encoded/hash kernels.
  - `[ ]` Validate collision-safe exactness.
  - `[ ]` Add tests for null retention/drop policies.
  - `[ ]` Benchmark against baseline.
- Gate:
  - `[ ]` `distinct` speedup achieved with no semantic drift.

## Patch 08: `tally` / Count Optimization
- Status: `[ ]`
- Goal: Faster frequency counting for Note columns.
- Tasks:
  - `[ ]` Implement counting over codes/hashes.
  - `[ ]` Preserve deterministic output ordering policy.
  - `[ ]` Validate top-N behavior and null handling.
  - `[ ]` Benchmark on large repetitive columns.
- Gate:
  - `[ ]` `tally` speedup achieved and output parity confirmed.

## Patch 09: Parallel Lane Execution
- Status: `[ ]`
- Goal: Scale hot ops on multi-core CPU for large workloads.
- Tasks:
  - `[ ]` Add parallel lane implementation for large inputs.
  - `[ ]` Add adaptive thresholds for parallel enablement.
  - `[ ]` Verify deterministic outputs under parallel execution.
  - `[ ]` Benchmark small vs large input behavior.
- Gate:
  - `[ ]` Large input speedup without small-input regressions.

## Patch 10: Semantic Lock + Fallback Policy
- Status: `[ ]`
- Goal: Safe rollout controls.
- Tasks:
  - `[ ]` Add runtime/config flag to force reference path.
  - `[ ]` Add optimized-vs-reference parity test suite.
  - `[ ]` Add failure diagnostics for mismatch reporting.
- Gate:
  - `[ ]` Forced reference mode is available and tested.

## Patch 11: Profile-Guided Tuning
- Status: `[ ]`
- Goal: Convert functional gains into robust production gains.
- Tasks:
  - `[ ]` Capture CPU profiles for hot datasets.
  - `[ ]` Tune allocation patterns and thresholds.
  - `[ ]` Tune branch behavior in compare/hash paths.
  - `[ ]` Re-run benchmark matrix and record final deltas.
- Gate:
  - `[ ]` Final benchmark report meets or exceeds target ranges.

## Patch 12: Docs + User Guidance
- Status: `[ ]`
- Goal: Ship-ready documentation and expectations.
- Tasks:
  - `[ ]` Document `Note::argsort` and sort semantics.
  - `[ ]` Document null ordering and stability guarantees.
  - `[ ]` Add performance guidance and examples.
  - `[ ]` Add fallback/config policy docs.
- Gate:
  - `[ ]` User docs and API references updated.

## Cross-Patch Quality Gates
- `[ ]` All existing CodeN tests pass after every patch.
- `[ ]` Added tests are deterministic and repeatable.
- `[ ]` No public API breaks without explicit migration notice.
- `[ ]` Benchmark report updated at each milestone.

## Milestone Checklist
- Milestone A (Patches 01-05): `[ ]`
- Milestone B (Patches 06-09): `[ ]`
- Milestone C (Patches 10-12): `[ ]`

## Expectation Management
- Target is to make CodeN much closer to Series-class CPU performance for key text workloads.
- Exact parity with numeric Series throughput is not guaranteed for all string-heavy cases because string comparison and collation costs are fundamentally higher.
