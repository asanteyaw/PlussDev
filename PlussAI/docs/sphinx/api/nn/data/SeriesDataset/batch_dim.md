<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 06b3ba5ebf39 -->

# `pluss::loader::SeriesDataset::batch_dim`

[Back to `pluss::loader::SeriesDataset`](../SeriesDataset.md)

## Signature

```cpp
std::int64_t pluss::loader::SeriesDataset::batch_dim() const noexcept override;
```

## What It Does

Returns the axis samples run along, exactly as given to the constructor
(or `0`, the default). Overrides
[`Dataset::batch_dim`](../Dataset/batch_dim.md); unlike the base
interface, this is set once at construction, not implemented per
subclass.

## Parameters

None.

## Returns

`std::int64_t` -- the sample axis, fixed at construction.

## Errors

None. `batch_dim` is `noexcept` and has no preconditions.

## Practical Examples

Three examples: the default value, an explicit non-default value passed
to the constructor, and confirming a `DataLoader` built from it slices
along the same axis.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4, 1});
  loader::SeriesDataset dataset({x});

  std::cout << "batch_dim() = " << dataset.batch_dim() << " (default)\n";

  return 0;
}
```

Output

```text
batch_dim() = 0 (default)
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series data = pluss::make_series({1.0, 2.0, 3.0, 4.0, 10.0, 20.0, 30.0, 40.0}, {2, 4});
  loader::SeriesDataset dataset({data}, /*batch_dim=*/1);

  std::cout << "batch_dim() = " << dataset.batch_dim() << "\n";

  return 0;
}
```

Output

```text
batch_dim() = 1
```

### Example 3

A `DataLoader` constructed from a `batch_dim = 1` dataset slices batches
along axis 1 -- both feature rows stay intact in each batch.

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series data = pluss::make_series({1.0, 2.0, 3.0, 4.0, 10.0, 20.0, 30.0, 40.0}, {2, 4});
  loader::SeriesDataset dataset({data}, /*batch_dim=*/1);

  loader::DataLoaderOptions options{};
  options.batch_size = 2;
  loader::DataLoader data_loader(dataset, options);
  for (auto batch : data_loader) {
    std::cout << "batch: " << batch.x() << "\n";
  }

  return 0;
}
```

Output

```text
batch: pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, 2],
  [10, 20]]
batch: pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[3, 4],
  [30, 40]]
```

## See Also

- [`size`](size.md) -- counted along this same axis.
- [`get`](get.md) / [`materialize_fields`](materialize_fields.md) -- both
  slice/report along this axis.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="fields.html">
    <span class="label">Next</span>
    <span class="title">fields</span>
  </a>
</div>
<!-- apiref:nav:end -->
