# `pluss::ai::classic::SpatialIndex` Introduction

## Role

`SpatialIndex` wraps a fixed set of points in a KD-tree so repeated
nearest-neighbor queries against that same set are fast, without ever
giving up exactness — every result is identical to what a brute-force
pairwise-distance scan would find. You obtain one from
[`build`](build.md), keep it around for as many queries as you need
against that fixed point set, and query it with either
[`query_k`](query_k.md) (an external batch of query rows) or
[`query_self_k`](query_self_k.md) (every indexed point queried against
every *other* indexed point, in one call).

## Lifecycle

An index is immutable once built — there is no method to add, remove, or
update a point afterward; rebuild a fresh `SpatialIndex` (via
[`build`](build.md)) whenever the underlying point set changes.

1. **`build(rows)`** — copies `rows` into the index's own storage and
   constructs the tree. Returns a `StatusOr<SpatialIndex>`; check `.ok()`
   before querying.
2. **`query_k(query_rows, k)`** / **`query_self_k(k)`** — any number of
   times, in any order, against the same built index. Both return a
   [`KNearestResult`](query_k.md#returns) — the same type either way.

## Exact, Not Approximate

Some KD-tree-based nearest-neighbor libraries trade correctness for
speed in high dimensions. This one never does: pruning (skipping a
branch of the tree once its own splitting hyperplane is already farther
than the current k-th best distance found so far) only ever *skips work
that provably cannot contain a better answer* — it never skips a branch
that might. As dimensionality grows, that pruning naturally weakens (the
"curse of dimensionality"), so query *cost* can degrade toward brute-force
in high-dimensional spaces — but query *correctness* never does, at any
dimensionality.

## Two Query Shapes, One Result Type

[`query_k`](query_k.md) and [`query_self_k`](query_self_k.md) both
return a [`KNearestResult`](query_k.md#returns) and differ only in where
the query points come from and how self-matches are handled:

| | [`query_k`](query_k.md) | [`query_self_k`](query_self_k.md) |
| --- | --- | --- |
| Query points | An external `Series` you supply | Every indexed point, against the index itself |
| Self-matches | Never excluded — if a query row happens to equal an indexed point, it matches itself at distance `0` | Always excluded — each point's own exact index is skipped, but a genuine duplicate point elsewhere in the index is not |
| `k` clamped to | `size()` | `size() - 1` |

## Related Pages

- [`balance`](../../balance.md) — [`plan_balance`](../../balance/plan_balance.md),
  which uses `query_self_k` internally for `BlendNeighbors` synthesis and
  cleaning's own neighbor searches, on CPU.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="build.html">
    <span class="label">Next</span>
    <span class="title">build</span>
  </a>
</div>
<!-- apiref:nav:end -->
