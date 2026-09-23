# `pluss::ai::classic::neighbors`

`neighbors` owns exactly one type:
[`SpatialIndex`](neighbors/SpatialIndex.md), an exact k-nearest-neighbor
index over a fixed point set, backed by a median-split KD-tree. Build it
once over any `[n, dim]` point set, then query it as many times as you
need — with an external batch of query rows
([`query_k`](neighbors/SpatialIndex/query_k.md)) or every indexed point
against every other one in a single call
([`query_self_k`](neighbors/SpatialIndex/query_self_k.md)). See
[`Introduction`](neighbors/Introduction.md) for where this class is used
internally elsewhere in this reference, and why its results are always
exact, never approximate, regardless of dimensionality.

## The Surface

| Method | What it gives you | Reach for it when |
| --- | --- | --- |
| [`SpatialIndex::build`](neighbors/SpatialIndex/build.md) | a queryable index over a fixed point set | you have one point set and expect to query it more than once |
| [`SpatialIndex::query_k`](neighbors/SpatialIndex/query_k.md) | each of an external batch's own `k` nearest indexed points | finding nearest neighbors for new points against the index |
| [`SpatialIndex::query_self_k`](neighbors/SpatialIndex/query_self_k.md) | every indexed point's `k` nearest *other* indexed points | finding each point's own neighbors within the same set |

## How To Call It

```cpp
namespace cls = pluss::ai::classic;

const cls::SpatialIndex index = cls::SpatialIndex::build(points).value();
const cls::KNearestResult neighbors = index.query_k(query_points, /*k=*/5).value();
// neighbors.indices[i] / neighbors.distances2[i] -- query row i's 5 nearest
// indexed points, sorted ascending by squared distance.
```

## Related Pages

- [`balance`](balance.md) — [`plan_balance`](balance/plan_balance.md),
  this family's own internal consumer for `BlendNeighbors` synthesis and
  cleaning.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

neighbors/Introduction
neighbors/SpatialIndex
```
<!-- apiref:toctree:end -->
