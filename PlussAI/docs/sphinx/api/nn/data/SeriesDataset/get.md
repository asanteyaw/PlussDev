<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3688a6c94b6e -->

# `pluss::loader::SeriesDataset::get`

[Back to `pluss::loader::SeriesDataset`](../SeriesDataset.md)

## Signature

```cpp
pluss::loader::DataBatch pluss::loader::SeriesDataset::get(std::int64_t index) const override;
```

## What It Does

Returns sample `index` from every field, via `index_select` along
`batch_dim`. Overrides [`Dataset::get`](../Dataset/get.md); like the base
contract, this is a direct single-sample accessor --
[`DataLoader`](../DataLoader.md) never calls it, it always batches
through [`materialize_fields()`](materialize_fields.md) instead.

## Parameters

`std::int64_t index`
: The sample index, in `[0, size())`.

## Returns

`DataBatch` -- one `Series` per field, each a single-sample slice.

## Errors

- `index` outside `[0, size())` throws.

## Practical Examples

Three examples: a baseline in-bounds call across two fields, an
out-of-bounds call, and a non-default `batch_dim`.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4, 1});
  const pluss::Series y = pluss::make_series({0.0, 1.0, 1.0, 0.0}, {4});
  loader::SeriesDataset dataset({x, y});

  std::cout << "get(1).x() = " << dataset.get(1).x() << "\n";
  std::cout << "get(1).y() = " << dataset.get(1).y() << "\n";

  return 0;
}
```

Output

```text
get(1).x() = pluss::Series(shape=[1,1], strides=[1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[2]]
get(1).y() = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1]
```

### Example 2

An out-of-bounds index throws.

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>
#include <string>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4, 1});
  loader::SeriesDataset dataset({x});

  try {
    dataset.get(10);
  } catch (const std::exception& e) {
    const std::string message(e.what());
    std::cout << "caught: " << message.substr(0, message.find('\n')) << "\n";
  }

  return 0;
}
```

Output

```text
caught: pluss::SeriesDataset::get: index out of range
```

### Example 3

Samples along axis 1 -- `get(2)` selects column 2 from both rows,
returning a `[2, 1]` slice (both features, one sample).

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series data = pluss::make_series({1.0, 2.0, 3.0, 4.0, 10.0, 20.0, 30.0, 40.0}, {2, 4});
  loader::SeriesDataset dataset({data}, /*batch_dim=*/1);

  std::cout << "get(2).x() = " << dataset.get(2).x() << "\n";

  return 0;
}
```

Output

```text
get(2).x() = pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[3],
  [30]]
```

## See Also

- [`size`](size.md) -- the valid index range.
- [`batch_dim`](batch_dim.md) -- which axis `get` slices along.
- [`materialize_fields`](materialize_fields.md) -- what `DataLoader`
  actually batches through.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fields.html">
    <span class="label">Previous</span>
    <span class="title">fields</span>
  </a>
  <a class="next-link" href="materialize_fields.html">
    <span class="label">Next</span>
    <span class="title">materialize_fields</span>
  </a>
</div>
<!-- apiref:nav:end -->
