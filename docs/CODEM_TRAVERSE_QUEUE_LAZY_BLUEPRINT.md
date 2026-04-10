# CodeM Traverse Queue Lazy Engine Blueprint

Status: Draft (implementation not started)
Owners: CodeM core maintainers
Scope: `Series::traverse.queue()` full lazy execution engine

## 1. Objective

Build a production-grade lazy query engine for `Series::traverse` that:

1. Defers execution until `eval()`.
2. Preserves correctness under mutation and aliasing.
3. Supports backend-aware execution (CPU/CUDA/MPS).
4. Applies optimizer passes before execution.
5. Keeps API policy aligned with `RangeView::*` usage.

This blueprint is for a full system, not a deferred-wrapper shim.

## 2. Non-goals

1. Do not expose partial `.queue()` user API before the full engine is ready.
2. Do not introduce free-function `pluss::*` query entrypoints for existing `Series` ops.
3. Do not silently fall back to eager behavior when lazy guards are violated.
4. Do not introduce scalar-host nested loops in hot indexing/write paths.

## 3. User-facing API contract

### 3.1 Surface

Target usage shape:

```cpp
auto plan = s.traverse.queue()
  .on(pluss::msharp::RangeView::Of<pluss::codem::ExtractSpec>(specs))
  .find_btw(0.0, 1.0, true)
  .find_not(s.isnan());

auto m = plan.eval();
auto out = m.select_from(other);
```

### 3.2 RangeView policy

1. Canonical API parameters for selector lists use `RangeView::Of<ExtractSpec>`.
2. `initializer_list` shims are only allowed if needed to disambiguate with scalar overloads.
3. No `std::vector`-only canonical overloads on queue DSL methods.

### 3.3 Type model

1. `SeriesTraverseQueueBuilder` (build phase).
2. `SeriesTraversePlan` (compiled immutable plan).
3. `TraverseMatch` (execution result, existing type, possibly extended metadata).

## 4. Semantics and correctness

### 4.1 Lazy capture semantics

Plan captures:

1. Source handle(s): referenced `SeriesImpl` identities.
2. Shape/dtype/platform guards.
3. Version guards (mutation counters).
4. Normalized query IR.

Execution rule:

1. `eval()` must validate guards before running.
2. Guard mismatch must fail with explicit diagnostic.
3. No implicit rebind, no silent eager fallback.

### 4.2 Mutation and alias safety

Add `version_counter` to `SeriesImpl` and increment on all in-place writes:

1. `copy_`, `index_put_`, `extract_put_`, `traverse_put_`, scatter variants, etc.
2. Any op that mutates shared storage view.

Plan stores source versions at compile time:

1. `source_version_snapshot`.
2. `alias_set_id` (storage identity guard).

Guard failure message must identify:

1. Which source changed.
2. Expected vs current version.
3. Suggested action (`rebuild plan`).

### 4.3 View contract

1. `.on(...)` path is traverse-slice semantics (view-representable only).
2. Non-view selectors in `.on(...)` are rejected at compile time of plan build.
3. `find_*` predicates can materialize masks/coords as execution artifacts.

## 5. IR design

### 5.1 Core IR objects

1. `QueryIR`
2. `IRNode`
3. `PredicateExpr`
4. `EvalGuardSet`

### 5.2 Node kinds

1. `OnSelectorNode` (normalized selector list)
2. `MaskInputNode` (external mask series)
3. `UnaryPredicateNode` (`isnan`, `not`, etc.)
4. `BinaryPredicateNode` (`eq`, `ge`, `le`, `between`, `and`, `or`)
5. `FindNode` (`find_all`, `find_first`)
6. `MaterializeCoordsNode` (`nonzero` equivalent lowering)

### 5.3 Canonicalization rules

1. Rewrite `find_btw(lo, hi, inclusive)` into primitive predicate nodes.
2. Fold double negation.
3. Constant-fold scalar predicates where possible.
4. Normalize predicate dtype to bool and platform to source platform.

## 6. Optimizer pipeline

Run before backend lowering:

1. Predicate simplification pass.
2. Mask fusion pass (merge chained boolean ops into single expression tree).
3. Shape-check hoisting pass.
4. Common subexpression elimination for repeated mask terms.
5. Short-circuit pruning for trivially-all-true/all-false masks.

Optional (phase 2):

1. Cost-based pass to pick fused-kernel or staged execution.

## 7. Backend lowering

Lowering target is a backend execution plan:

1. CPU plan
2. CUDA plan
3. MPS plan

### 7.1 CPU

1. Use existing vectorized kernels and `SeriesIterator` where possible.
2. Avoid host scalar/nested loops in mask+index hot paths.
3. Reuse optimized `index/index_put/select/nonzero` primitives.

### 7.2 CUDA

1. Lower to native CUDA masking/indexing routes where available.
2. Respect strict/native route flags.
3. No implicit CPU bridge when strict mode is enabled.

### 7.3 MPS

1. Use current supported paths.
2. Enforce guardrails where unsupported mixed indexing exists.
3. Explicit fail instead of hidden route changes.

## 8. Execution and caching

### 8.1 Compile cache key

Cache by:

1. IR fingerprint.
2. Source platform/dtype/rank.
3. Backend capability flags.

### 8.2 Cache invalidation

1. Source version mismatch invalidates executable plan use.
2. Backend capability change invalidates lowered plan.

### 8.3 Thread safety

1. Plan objects immutable after compile.
2. Shared cache guarded by lock-free map or coarse mutex in phase 1.

## 9. Autograd behavior

### 9.1 Differentiability policy

1. Predicates and coordinates are non-differentiable.
2. `select_from(other)` is differentiable w.r.t selected values in `other`.
3. No gradients through match coordinates.

### 9.2 Tape integration

1. Lazy eval should record executed primitive ops, not opaque black-box node.
2. Preserve existing backward behavior for `index/select/gather` paths.
3. Fail clearly where current backend/backward limitations apply.

## 10. Diagnostics and observability

Add debug surfaces:

1. `plan.describe()` textual IR.
2. `plan.explain()` optimized vs original graph summary.
3. `plan.guard_report()` version/shape/platform guard details.
4. Optional tracing counters (`nodes_fused`, `guards_failed`, `cache_hits`).

## 11. Phased implementation gates

### Phase A: Foundations

Deliver:

1. `SeriesImpl` version counter.
2. Guard infrastructure.
3. IR data structures and serializer.

Gate:

1. Mutation guard tests pass.
2. No behavior regressions in existing indexing/traverse tests.

### Phase B: Builder and compiler

Deliver:

1. Internal queue DSL builder.
2. IR normalization/canonicalization.
3. Static validation (`on(...)` view-only checks).

Gate:

1. Builder parity tests for all current `find_*` forms.
2. Invalid selectors fail with deterministic errors.

### Phase C: CPU executor + optimizer

Deliver:

1. Full CPU lazy execution path.
2. Optimizer passes (initial set).
3. Cache + diagnostics.

Gate:

1. Numeric parity with eager traverse.
2. Perf non-regression for one-off eager path.
3. Repeated-plan benchmark wins.

### Phase D: CUDA/MPS lowering

Deliver:

1. Backend-specific lowering and route control integration.
2. Strict-mode behavior guarantees.

Gate:

1. Backend parity tests.
2. No silent CPU fallback in strict modes.

### Phase E: Public API release

Deliver:

1. Public `.traverse.queue()` API enabled.
2. Docs: API reference + user guide + troubleshooting.

Gate:

1. Full test matrix green.
2. Stability soak on representative workloads.

## 12. Test plan

### 12.1 Correctness matrix

1. `find_all/find_if/find_eq/find_neq/find_ge/find_le/find_btw/find_not/find_and/find_or/find_first`
2. With and without `.on(...)`
3. Different dtypes/platforms/ranks
4. Alias/view mutation guard cases

### 12.2 Parity checks

1. Eager traverse vs lazy queue results.
2. `select_from` parity for repeated indices and broadcast tails.

### 12.3 Performance checks

1. One-off query overhead ceiling.
2. Repeat-eval speedup thresholds.
3. Memory footprint under large masks.

## 13. Rollout policy

1. Feature flag until phase E gate is met.
2. No public docs advertising `.queue()` before full readiness.
3. Maintain compatibility of eager `traverse` APIs throughout rollout.

## 14. Open decisions

1. Guard policy on mismatch:
   - hard error (recommended), or
   - optional rebuild mode via explicit API (`plan.recompile()`).
2. Cache lifetime policy:
   - process-global cache vs context-scoped cache.
3. IR serialization format for audit/replay:
   - text JSON-like vs binary compact form.

## 15. Immediate next implementation tasks

1. Add `version_counter` to `SeriesImpl` and increment hooks on all in-place mutators.
2. Add internal `TraverseQueueIR` headers and unit tests.
3. Add builder prototype behind compile-time flag without public API exposure.
4. Add eager-vs-IR parity golden tests for current `traverse` examples.
