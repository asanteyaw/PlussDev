<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0e36cfcca6a5 -->
# `pluss::ai::classic::SpatialIndex::query_self_k`

Self-vs-self k-nearest-neighbor: for each of the indexed points, finds
its `k` nearest *other* points within the same index, in one call — no
external query batch needed. Each point's own exact match to itself is
**always excluded**, by index identity, not by distance — so a genuine
duplicate point elsewhere in the index is never excluded, only the
literal same row position is. Contrast [`query_k`](query_k.md), which
never excludes anything, including a query row that happens to equal an
indexed point.

## Signature

```cpp
classic::StatusOr<classic::KNearestResult> classic::SpatialIndex::query_self_k(
    std::int64_t k) const;
```

## What It Does

For every point already in the index, finds its `k` nearest *other*
indexed points, in one call, with no separate query batch needed -- each
point's own row is excluded from its own result by identity, not by
distance, so a genuine coincident duplicate elsewhere in the index still
counts as a valid neighbor. This is the shape to reach for when the
question is "what is near each of my existing points," as opposed to
[`query_k`](query_k.md)'s "what is near each of these new points."

## Parameters

`k` — `std::int64_t`
: How many nearest *other* points to find per indexed point. Must be
  `> 0`. **Silently clamped** to `size() - 1` if larger — since every
  point excludes itself, at most `size() - 1` other points ever exist to
  find.

## Returns

A `StatusOr<KNearestResult>` — identical shape and meaning to
[`query_k`'s own return](query_k.md#returns): `indices`/`distances2`,
both `[size(), effective_k]`, sorted ascending by squared distance per
row. Row `i` describes indexed point `i`'s own nearest other points —
`indices[i]` never contains `i` itself.

## Errors

- `k <= 0` — `Status::invalid_argument`.
- `size() == 1` (a single-point index — no other point exists to find at
  any `k`) — `Status::invalid_argument`.

## Practical Examples

A basic self-query proving no point ever lists itself, a genuine
duplicate point proven **not** excluded (only the literal self-index
is), `k` clamped to `size() - 1`, the direct contrast with `query_k` on
the exact same data, and the single-point error case.

### Example 1

Four points — every point's own index is absent from its own neighbor
list.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series rows = pluss::make_series(
      {0.0, 0.0, 1.0, 0.0, 5.0, 5.0, 6.0, 5.0}, {8}, f64).reshape({4, 2});
  const cls::SpatialIndex index = cls::SpatialIndex::build(rows).value();

  const cls::KNearestResult result = index.query_self_k(1).value();
  std::cout << "indices (each row's own index never appears: [[1],[0],[3],[2]]) = " << result.indices << "\n";

  return 0;
}
```

Output

```text
indices (each row's own index never appears: [[1],[0],[3],[2]]) = pluss::Series(shape=[4,1], strides=[1,1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[[1],
  [0],
  [3],
  [2]]
```

### Example 2

Two indexed points are exact duplicates — each still correctly lists the
other as its own nearest neighbor, at distance `0`. Only the literal
same row position is excluded, never a distance-`0` match to a different
row.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // rows 0 and 1 are identical; row 2 is far away.
  const pluss::Series rows = pluss::make_series(
      {3.0, 3.0, 3.0, 3.0, 100.0, 100.0}, {6}, f64).reshape({3, 2});
  const cls::SpatialIndex index = cls::SpatialIndex::build(rows).value();

  const cls::KNearestResult result = index.query_self_k(1).value();
  std::cout << "indices (row 0 -> 1, row 1 -> 0, row 2 -> one of the tied pair) = " << result.indices << "\n";
  std::cout << "distances2 (rows 0 and 1 each find the other at 0)          = " << result.distances2 << "\n";

  return 0;
}
```

Output

```text
indices (row 0 -> 1, row 1 -> 0, row 2 -> one of the tied pair) = pluss::Series(shape=[3,1], strides=[1,1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[[1],
  [0],
  [1]]
distances2 (rows 0 and 1 each find the other at 0)          = pluss::Series(shape=[3,1], strides=[1,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[0],
  [0],
  [18818]]
```

### Example 3

`k` larger than `size() - 1` is silently clamped.

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

  const cls::KNearestResult result = index.query_self_k(100).value();
  std::cout << "requested k=100, size()-1=2 -> effective width = " << result.indices.sizes()[1] << "\n";

  return 0;
}
```

Output

```text
requested k=100, size()-1=2 -> effective width = 2
```

### Example 4

The direct contrast with [`query_k`](query_k.md) on the same data: query
every indexed point against itself via `query_k` (self-matches
included), versus `query_self_k` (self-matches excluded).

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series rows = pluss::make_series(
      {0.0, 0.0, 1.0, 0.0, 5.0, 5.0}, {6}, f64).reshape({3, 2});
  const cls::SpatialIndex index = cls::SpatialIndex::build(rows).value();

  const cls::KNearestResult via_query_k = index.query_k(rows, 1).value();
  std::cout << "query_k(rows, 1) indices (each row matches ITSELF)      = " << via_query_k.indices << "\n";

  const cls::KNearestResult via_self_k = index.query_self_k(1).value();
  std::cout << "query_self_k(1) indices (each row's own index excluded) = " << via_self_k.indices << "\n";

  return 0;
}
```

Output

```text
query_k(rows, 1) indices (each row matches ITSELF)      = pluss::Series(shape=[3,1], strides=[1,1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[[0],
  [1],
  [2]]
query_self_k(1) indices (each row's own index excluded) = pluss::Series(shape=[3,1], strides=[1,1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[[1],
  [0],
  [1]]
```

### Example 5

A single-point index has no other point to find at any `k`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series rows = pluss::make_series({0.0, 0.0}, {2}, f64).reshape({1, 2});
  const cls::SpatialIndex index = cls::SpatialIndex::build(rows).value();

  const cls::StatusOr<cls::KNearestResult> rejected = index.query_self_k(1);
  std::cout << "ok = " << std::boolalpha << rejected.ok() << "\n";
  std::cout << "message = \"" << rejected.status().message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "SpatialIndex::query_k: not enough other points in the index to satisfy k."
```

## See Also

- [`query_k`](query_k.md) — the external-query form, which never
  excludes self-matches.
- [`build`](build.md) — constructs the index this method queries.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="query_k.html">
    <span class="label">Previous</span>
    <span class="title">query_k</span>
  </a>
</div>
<!-- apiref:nav:end -->
