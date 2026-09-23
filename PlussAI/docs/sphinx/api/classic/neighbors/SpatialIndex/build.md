<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 81bbafacd0e1 -->
# `pluss::ai::classic::SpatialIndex::build`

Constructs a `SpatialIndex` over a fixed point set, copying every point
into the index's own host-memory storage and building the KD-tree once.
The only way to obtain a real `SpatialIndex` — the default constructor
alone gives you an empty, unqueryable one.

## Signature

```cpp
static classic::StatusOr<classic::SpatialIndex> classic::SpatialIndex::build(
    const pluss::Series& rows);
```

## Parameters

`rows` — `pluss::Series`
: A rank-2 `[n, dim]` `Series`, one point per row. Must be defined,
  `n > 0`, `dim > 0`. Densified through a `Float64`, CPU copy
  internally regardless of `rows`' own dtype or platform — the index
  never aliases `rows` afterward, so mutating or letting `rows` go out
  of scope after `build` returns has no effect on the built index. See
  [Example 2](#example-2).

## Returns

A `StatusOr<SpatialIndex>`. On success, `size() == rows.sizes()[0]` and
`dim() == rows.sizes()[1]`.

## Errors

- `rows` undefined — `Status::invalid_argument`.
- `rows` not rank-2 — `Status::shape_mismatch`.
- `rows.sizes()[0] <= 0` or `rows.sizes()[1] <= 0` —
  `Status::shape_mismatch`.

## Practical Examples

A basic build with `size()`/`dim()` verified, the index proven to own an
independent copy of the source data, a single-dimension (`dim = 1`)
point set, and an error roundup.

### Example 1

Five 2-D points.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series rows = pluss::make_series(
      {0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 5.0, 5.0, 5.0, 6.0}, {10}, f64).reshape({5, 2});
  const cls::SpatialIndex index = cls::SpatialIndex::build(rows).value();
  std::cout << "size() = " << index.size() << "\n";
  std::cout << "dim()  = " << index.dim() << "\n";

  return 0;
}
```

Output

```text
size() = 5
dim()  = 2
```

### Example 2

The index owns an independent copy — mutating the source `Series`
variable after `build` returns has no effect on queries against the
already-built index.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  pluss::Series rows = pluss::make_series({0.0, 0.0, 10.0, 10.0}, {4}, f64).reshape({2, 2});
  const cls::SpatialIndex index = cls::SpatialIndex::build(rows).value();

  // Reassign the local variable entirely -- the index was already built
  // from the original data and cannot see this change.
  rows = pluss::make_series({999.0, 999.0, 999.0, 999.0}, {4}, f64).reshape({2, 2});

  const pluss::Series query = pluss::make_series({0.1, 0.1}, {2}, f64).reshape({1, 2});
  const cls::KNearestResult result = index.query_k(query, 1).value();
  std::cout << "nearest index (still 0, the original first point) = " << result.indices << "\n";

  return 0;
}
```

Output

```text
nearest index (still 0, the original first point) = pluss::Series(shape=[1,1], strides=[1,1], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=[[0]]
```

### Example 3

A single-dimension point set (`dim = 1`) — the KD-tree degenerates to a
plain sorted structure along one axis, but the interface is unchanged.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series rows = pluss::make_series({0.0, 5.0, 10.0, 15.0}, {4}, f64).reshape({4, 1});
  const cls::SpatialIndex index = cls::SpatialIndex::build(rows).value();
  std::cout << "size() = " << index.size() << ", dim() = " << index.dim() << "\n";

  return 0;
}
```

Output

```text
size() = 4, dim() = 1
```

### Example 4

An error roundup: undefined `rows`, rank-1 `rows`, zero rows, and zero
columns.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  std::cout << "undefined ok = " << std::boolalpha << cls::SpatialIndex::build(pluss::Series::undefined()).ok() << "\n";

  const pluss::Series rank1 = pluss::make_series({1.0, 2.0, 3.0}, {3}, f64);
  std::cout << "rank-1 ok = " << cls::SpatialIndex::build(rank1).ok() << "\n";

  const pluss::Series zero_rows = pluss::make_series(std::vector<double>{}, {0}, f64).reshape({0, 2});
  std::cout << "zero rows ok = " << cls::SpatialIndex::build(zero_rows).ok() << "\n";

  return 0;
}
```

Output

```text
undefined ok = false
rank-1 ok = false
zero rows ok = false
```

## See Also

- [`query_k`](query_k.md) / [`query_self_k`](query_self_k.md) — the two
  ways to query the built index.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="query_k.html">
    <span class="label">Next</span>
    <span class="title">query_k</span>
  </a>
</div>
<!-- apiref:nav:end -->
