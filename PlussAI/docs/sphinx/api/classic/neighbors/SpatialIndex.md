# `pluss::ai::classic::SpatialIndex`

An exact k-nearest-neighbor index over a fixed point set, backed by a
median-split KD-tree. "Exact" is load-bearing: every query returns the
same k-nearest set a brute-force pairwise-distance scan would, just found
faster on average — this is a real speed optimization, never an
approximation, regardless of how many dimensions the points have. Reach
for it directly when you need repeated nearest-neighbor queries against
the same fixed point set; [`plan_balance`](../balance/plan_balance.md#example-7)
uses it internally for `BlendNeighbors` synthesis and cleaning's own
neighbor searches.

## Signature

```cpp
classic::SpatialIndex::SpatialIndex() = default;

static classic::StatusOr<classic::SpatialIndex> classic::SpatialIndex::build(
    const pluss::Series& rows);

classic::StatusOr<classic::KNearestResult> classic::SpatialIndex::query_k(
    const pluss::Series& query_rows,
    std::int64_t k) const;
classic::StatusOr<classic::KNearestResult> classic::SpatialIndex::query_self_k(
    std::int64_t k) const;

std::int64_t classic::SpatialIndex::size() const noexcept;
std::int64_t classic::SpatialIndex::dim() const noexcept;
```

## Construction

A default-constructed `SpatialIndex()` is empty and not useful on its
own — every real instance comes from [`build`](SpatialIndex/build.md),
which copies the given points into its own host-memory storage and never
aliases the source `Series` afterward (mutating or dropping the source
after `build` returns has no effect on the index). `build` can fail —
see its own page for every validation.

## State

`size()` — `std::int64_t`
: The number of indexed points — `0` for a default-constructed index,
  otherwise exactly the row count [`build`](SpatialIndex/build.md) was
  given.

`dim()` — `std::int64_t`
: The dimensionality every point (and every query row) must have — `0`
  for a default-constructed index, otherwise the column count `build`
  was given.

**CPU-only by construction**: the tree is a real host-memory recursive
structure, not a `Series`-vectorized computation, so building and
querying always run on the CPU regardless of the input `Series`'
platform — the *index* is a CPU-only structure, but query results (see
[`KNearestResult`](SpatialIndex/query_k.md#returns)) are still returned
on the same platform the indexed `rows` were built from.

## Methods

| Method | Returns | Covered on |
| --- | --- | --- |
| `build(rows)` | `StatusOr<SpatialIndex>` | [`build`](SpatialIndex/build.md) |
| `query_k(query_rows, k)` | `StatusOr<KNearestResult>` | [`query_k`](SpatialIndex/query_k.md) |
| `query_self_k(k)` | `StatusOr<KNearestResult>` | [`query_self_k`](SpatialIndex/query_self_k.md) |

## Changelog

- **0.1** — `build`, `query_k`, `query_self_k`, `size`, `dim`.

## See Also

- [`balance`](../balance.md) — [`plan_balance`](../balance/plan_balance.md),
  this class's own internal consumer for `BlendNeighbors` synthesis and
  cleaning.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

SpatialIndex/Introduction
SpatialIndex/build
SpatialIndex/query_k
SpatialIndex/query_self_k
```
<!-- apiref:toctree:end -->
