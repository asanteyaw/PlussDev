# CodeN CPU Performance Blueprint

## Objective
Bring CodeN (`Note`) CPU performance closer to CodeM (`Series`) for high-impact workflows while preserving current semantics and API stability.

## Guiding Doctrine
- Do not rewrite CodeN wholesale.
- Keep current arena + `OffLen` span storage model.
- Add missing low-level primitives and kernelized execution paths incrementally.
- Preserve correctness first, then optimize.

## Scope
- In scope:
  - Sorting, indexing, grouping/counting primitives for `Note`.
  - CPU kernelization and optional parallelization for large lanes.
  - Encoded fast path (dictionary/interned codes) for repeated strings.
  - Strict semantic parity (null, stability, UTF-8 behavior).
- Out of scope (for this plan):
  - GPU backend for CodeN.
  - Full redesign of Note storage format.
  - Locale-dependent collation as default behavior.

## Performance Goals
- `Note::sort` and `argsort` throughput: 3x-10x on large 1D text columns.
- `distinct`/`tally` throughput: 2x-8x on repetitive categorical text.
- Keep memory growth bounded and predictable.
- No behavioral regressions in existing APIs.

## Correctness Goals
- Preserve null ordering rules.
- Preserve stable sort behavior.
- Preserve UTF-8 correctness.
- Preserve existing default lexical ordering semantics.
- Keep fallback/reference path available during rollout.

## Patch Plan (12 patches)

### Patch 01: Baseline + Harness
- Deliverables:
  - Add focused CodeN microbenchmarks for `sort`, `distinct`, `tally`, `take`.
  - Add regression corpus with nulls, repeated labels, mixed lengths, UTF-8.
- Goal:
  - Establish reliable baseline and perf/correctness gates.

### Patch 02: `Note::argsort` Public Surface
- Deliverables:
  - Add `Note::argsort(...) -> Series<int64>` API.
  - Dispatch wiring and tests.
- Goal:
  - Enable permutation-first flows (`take(argsort(...))`) similar to Series.

### Patch 03: Fast `Note::take(indices)`
- Deliverables:
  - Dedicated gather-by-permutation kernel for `OffLen` spans.
  - Minimize allocations/copies and branch overhead.
- Goal:
  - Make sort pipelines practical and reusable.

### Patch 04: Vectorized Compare/Eq/Hash Kernels
- Deliverables:
  - Span-aware kernels for lexical compare/equality/hash.
  - Reusable kernel interfaces for `sort`, `distinct`, `tally`.
- Goal:
  - Remove repeated scalar string conversions/comparisons in hot loops.

### Patch 05: `sort` Delegates to `argsort + take`
- Deliverables:
  - Refactor `Note::sort` to compute permutation once, then gather.
  - Preserve stable behavior and current ordering semantics.
- Goal:
  - Centralize sorting logic and reduce duplicated code paths.

### Patch 06: Encoded Fast Path (Optional Codes)
- Deliverables:
  - Optional dictionary/interned code representation.
  - Fast-path branch for repeated strings.
- Goal:
  - Convert expensive string operations into integer operations when beneficial.

### Patch 07: `distinct` on Codes/Hashes
- Deliverables:
  - `distinct` optimized via code/hash route with collision-safe fallback.
  - Exactness guarantees.
- Goal:
  - Speed up common categorical workflows.

### Patch 08: `tally` / `count_by` Optimized Path
- Deliverables:
  - Frequency counting via code/hash bins.
  - Stable deterministic output ordering policy retained.
- Goal:
  - Accelerate value-count style operations.

### Patch 09: Parallel Lane Execution
- Deliverables:
  - Optional multithreaded lane execution for large workloads.
  - Tunables: thresholds, chunking, and fallback to single-thread.
- Goal:
  - Improve CPU scaling on large text columns without hurting small inputs.

### Patch 10: Semantic Lock + Fallback Policy
- Deliverables:
  - Feature flag/runtime policy to force reference path.
  - Golden tests proving parity between optimized and reference implementations.
- Goal:
  - Safe rollout with immediate correctness fallback.

### Patch 11: Profiling + Tuning Pass
- Deliverables:
  - Profile-guided threshold tuning.
  - Allocation and cache-locality optimizations.
- Goal:
  - Convert functional speedups into robust real-world gains.

### Patch 12: Documentation + Migration Notes
- Deliverables:
  - API docs for `argsort`, behavior notes, complexity notes.
  - Performance guidance and best practices.
- Goal:
  - Make optimized usage discoverable and stable for users.

## Milestones
- Milestone A (Patches 01-05):
  - Core primitives complete (`argsort`, fast `take`, delegated `sort`).
  - Semantic parity validated.
- Milestone B (Patches 06-09):
  - Encoded fast path + optimized `distinct`/`tally` + optional parallelization.
- Milestone C (Patches 10-12):
  - Rollout controls, final tuning, docs.

## Risks and Mitigations
- Risk: semantic drift from reference behavior.
  - Mitigation: parity tests and forced-reference mode.
- Risk: hash/coding collisions affecting correctness.
  - Mitigation: collision-safe verification fallback.
- Risk: parallel overhead on small inputs.
  - Mitigation: adaptive thresholds and single-thread fallback.
- Risk: memory blow-up from code dictionaries.
  - Mitigation: bounded caches and heuristic enablement.

## Acceptance Criteria
- Correctness:
  - All existing CodeN tests pass.
  - New parity tests pass across null/UTF-8/repeated-label cases.
- Performance:
  - Meets target speedups for `sort`, `distinct`, `tally` on benchmark corpus.
- Safety:
  - Reference fallback path remains functional and test-covered.

## Notes on Timeline and Ownership
- Recommended cadence:
  - One patch at a time, each with tests + benchmark delta.
- Suggested review checkpoints:
  - After Patch 05 (core API usable),
  - After Patch 09 (major perf features),
  - After Patch 12 (ship-ready docs + policy).

## Execution Tracker
- Detailed execution checklist:
  - `docs/CODEN_CPU_PERFORMANCE_EXECUTION_TRACKER.md`
