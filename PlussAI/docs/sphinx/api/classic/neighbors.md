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

Index six points, then query two points for their three nearest neighbors. The program prints the query count and the neighbor count per query.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>
#include <pluss/mic.h>

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <vector>

namespace cls = pluss::ai::classic;

cls::LabeledBatch fixture_batch(const pluss::Config& cfg) {
  const pluss::Series x = pluss::make_series(std::vector<double>{
      0.0, 0.1, 0.2, 0.0, 0.1, 0.3, 0.3, 0.2,
      1.0, 1.1, 1.2, 0.9, 1.1, 1.3, 1.4, 1.0}, {8, 2}, cfg);
  const pluss::Series y = pluss::make_series(std::vector<double>{0, 0, 0, 0, 1, 1, 1, 1}, {8}, cfg);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;
  return batch;
}

int main() {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series points = pluss::make_series(std::vector<double>{
      0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 5.0, 5.0, 6.0, 5.0, 5.0, 6.0}, {6, 2}, cfg);
  const pluss::Series queries = pluss::make_series(std::vector<double>{0.1, 0.1, 5.1, 5.1}, {2, 2}, cfg);
  const cls::SpatialIndex index = cls::SpatialIndex::build(points).value();
  const cls::KNearestResult neighbors = index.query_k(queries, /*k=*/3).value();
  std::cout << "query rows = " << neighbors.indices.sizes()[0] << "\n";
  std::cout << "neighbors per query = " << neighbors.indices.sizes()[1] << "\n";
  return 0;
}
```

Output

```text
query rows = 2
neighbors per query = 3
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
