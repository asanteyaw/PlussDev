<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f39ec86327ba -->
# `pluss::ai::classic::SpatialIndex::query_k`

Finds each of `query_rows`' own `k` nearest indexed points — an external
batch query against the index built by [`build`](build.md). **Never
excludes self-matches**: if a query row happens to equal an indexed
point exactly, it is returned as its own nearest neighbor at distance
`0`, just like any other point would be. See
[`query_self_k`](query_self_k.md) for the self-vs-self form that does
exclude exact self-matches.

## Signature

```cpp
classic::StatusOr<classic::KNearestResult> classic::SpatialIndex::query_k(
    const pluss::Series& query_rows,
    std::int64_t k) const;
```

## What It Does

For every row in `query_rows`, walks the KD-tree built by
[`build`](build.md) and returns the `k` indexed points closest to it,
sorted nearest-first, along with their distances -- an ordinary
nearest-neighbor lookup against a separate batch of query points, distinct
from the indexed points themselves.

## Parameters

`query_rows` — `pluss::Series`
: A rank-2 `[n_queries, dim]` `Series`, one query point per row. Must be
  defined, and `dim` must match [`dim()`](../SpatialIndex.md#state)
  exactly.

`k` — `std::int64_t`
: How many nearest neighbors to find per query row. Must be `> 0`.
  **Silently clamped** to [`size()`](../SpatialIndex.md#state) if
  larger — asking for more neighbors than exist in the index is never an
  error, you just get every indexed point back. See
  [Example 3](#example-3).

## Returns

A `StatusOr<KNearestResult>`:

```cpp
pluss::Series classic::KNearestResult::indices;     // Int64,   [n_queries, effective_k]
pluss::Series classic::KNearestResult::distances2;  // Float64, [n_queries, effective_k]
```

`effective_k` is `k` after the clamp above. Row `i` of both fields
describes query row `i`'s own nearest neighbors, **sorted ascending by
distance** — column `0` is always the single nearest match. `distances2`
holds *squared* Euclidean distance (no `sqrt` applied) — take the square
root yourself if you need the actual distance; comparing or ranking
squared distances gives identical results to comparing real distances,
so most callers never need to. `indices` holds row positions into the point set
[`build`](build.md) was given — usable directly to index back into the
original `Series` `build` was called with.

## Errors

- `query_rows` undefined — `Status::invalid_argument`.
- `query_rows` not rank-2, or its column count does not equal `dim()` —
  `Status::shape_mismatch`.
- `k <= 0` — `Status::invalid_argument`.

## Practical Examples

A basic query verified by hand against brute-force pairwise distance,
`distances2`'s ascending order and squared-not-real-distance convention
made explicit, `k` clamped when it exceeds `size()`, multiple query rows
handled independently in one call, a query row that exactly matches an
indexed point returning itself at distance `0`, and an error roundup.

### Example 1

Four 2-D points; a query near one corner — the single nearest neighbor
and its exact squared distance, verified by hand.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series rows = pluss::make_series(
      {0.0, 0.0, 10.0, 0.0, 0.0, 10.0, 10.0, 10.0}, {8}, f64).reshape({4, 2});
  const cls::SpatialIndex index = cls::SpatialIndex::build(rows).value();

  // query (1, 1): nearest is (0, 0), squared distance 1^2 + 1^2 = 2.
  const pluss::Series query = pluss::make_series({1.0, 1.0}, {2}, f64).reshape({1, 2});
  const cls::KNearestResult result = index.query_k(query, 1).value();
  std::cout << "indices (expect [[0]]) = " << result.indices << "\n";
  std::cout << "distances2 (expect [[2]]) = " << result.distances2 << "\n";

  return 0;
}
```

Output

```text
indices (expect [[0]]) = pluss::Series(shape=[1,1], strides=[1,1], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=[[0]]
distances2 (expect [[2]]) = pluss::Series(shape=[1,1], strides=[1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[2]]
```

### Example 2

`k = 3` from the same query point — every returned distance is squared,
and the row is sorted ascending.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series rows = pluss::make_series(
      {0.0, 0.0, 10.0, 0.0, 0.0, 10.0, 10.0, 10.0}, {8}, f64).reshape({4, 2});
  const cls::SpatialIndex index = cls::SpatialIndex::build(rows).value();

  const pluss::Series query = pluss::make_series({1.0, 1.0}, {2}, f64).reshape({1, 2});
  const cls::KNearestResult result = index.query_k(query, 3).value();
  std::cout << "indices    = " << result.indices << "\n";
  std::cout << "distances2 (ascending: 2, then two ties at 82) = " << result.distances2 << "\n";

  return 0;
}
```

Output

```text
indices    = pluss::Series(shape=[1,3], strides=[3,1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[[0, 1, 2]]
distances2 (ascending: 2, then two ties at 82) = pluss::Series(shape=[1,3], strides=[3,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[2, 82, 82]]
```

### Example 3

`k` larger than `size()` is silently clamped — never an error.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series rows = pluss::make_series({0.0, 1.0, 2.0}, {3}, f64).reshape({3, 1});
  const cls::SpatialIndex index = cls::SpatialIndex::build(rows).value();

  const pluss::Series query = pluss::make_series({0.5}, {1}, f64).reshape({1, 1});
  const cls::KNearestResult result = index.query_k(query, 100).value();
  std::cout << "requested k=100, size()=3 -> effective width = " << result.indices.sizes()[1] << "\n";

  return 0;
}
```

Output

```text
requested k=100, size()=3 -> effective width = 3
```

### Example 4

Three independent query rows in one call — each row's own nearest
neighbor, unaffected by the other queries.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series rows = pluss::make_series(
      {0.0, 0.0, 10.0, 0.0, 0.0, 10.0, 10.0, 10.0}, {8}, f64).reshape({4, 2});
  const cls::SpatialIndex index = cls::SpatialIndex::build(rows).value();

  const pluss::Series queries = pluss::make_series(
      {0.1, 0.1, 9.9, 0.1, 0.1, 9.9}, {6}, f64).reshape({3, 2});
  const cls::KNearestResult result = index.query_k(queries, 1).value();
  std::cout << "indices (expect [[0],[1],[2]]) = " << result.indices << "\n";

  return 0;
}
```

Output

```text
indices (expect [[0],[1],[2]]) = pluss::Series(shape=[3,1], strides=[1,1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[[0],
  [1],
  [2]]
```

### Example 5

A query row that exactly matches an indexed point returns itself as its
own nearest neighbor, at distance `0` — `query_k` never excludes
self-matches.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series rows = pluss::make_series(
      {0.0, 0.0, 10.0, 10.0, 20.0, 20.0}, {6}, f64).reshape({3, 2});
  const cls::SpatialIndex index = cls::SpatialIndex::build(rows).value();

  // query row 1 (10, 10) is exactly indexed point 1.
  const pluss::Series query = pluss::make_series({10.0, 10.0}, {2}, f64).reshape({1, 2});
  const cls::KNearestResult result = index.query_k(query, 1).value();
  std::cout << "indices (expect [[1]], itself)     = " << result.indices << "\n";
  std::cout << "distances2 (expect [[0]])          = " << result.distances2 << "\n";

  return 0;
}
```

Output

```text
indices (expect [[1]], itself)     = pluss::Series(shape=[1,1], strides=[1,1], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=[[1]]
distances2 (expect [[0]])          = pluss::Series(shape=[1,1], strides=[1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[0]]
```

### Example 6

An error roundup: undefined `query_rows`, a dimensionality mismatch, and
`k <= 0`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series rows = pluss::make_series({0.0, 0.0, 1.0, 1.0}, {4}, f64).reshape({2, 2});
  const cls::SpatialIndex index = cls::SpatialIndex::build(rows).value();

  std::cout << "undefined query_rows ok = " << std::boolalpha << index.query_k(pluss::series::undefined(), 1).ok() << "\n";

  const pluss::Series wrong_dim = pluss::make_series({0.0, 0.0, 0.0}, {3}, f64).reshape({1, 3});
  std::cout << "dim mismatch ok = " << index.query_k(wrong_dim, 1).ok() << "\n";

  const pluss::Series ok_dim = pluss::make_series({0.5, 0.5}, {2}, f64).reshape({1, 2});
  std::cout << "k=0 ok = " << index.query_k(ok_dim, 0).ok() << "\n";

  return 0;
}
```

Output

```text
undefined query_rows ok = false
dim mismatch ok = false
k=0 ok = false
```

## See Also

- [`build`](build.md) — constructs the index this method queries.
- [`query_self_k`](query_self_k.md) — the self-vs-self form, which does
  exclude exact self-matches.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="build.html">
    <span class="label">Previous</span>
    <span class="title">build</span>
  </a>
  <a class="next-link" href="query_self_k.html">
    <span class="label">Next</span>
    <span class="title">query_self_k</span>
  </a>
</div>
<!-- apiref:nav:end -->
