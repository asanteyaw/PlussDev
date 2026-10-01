<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c1df21ce74ca -->

# `pluss::loader::Dataset::batch_dim`

[Back to `pluss::loader::Dataset`](../Dataset.md)

## Signature

```cpp
virtual std::int64_t pluss::loader::Dataset::batch_dim() const noexcept;
```

## What It Does

Names which axis samples run along, for every field
[`materialize_fields()`](materialize_fields.md) returns and for
[`size()`](size.md)'s own count. Defaults to `0` (the conventional
leading batch axis). `DataLoader(const Dataset&, options)` reads this
once at construction and uses it as the `batch_dim` every subsequent
`index_select`-based batch slice runs along -- overriding
`DataLoaderOptions::batch_dim` with whatever the dataset itself reports,
so a dataset whose own layout genuinely differs from axis `0` does not
need the caller to separately remember to set it.

## Parameters

None.

## Returns

`std::int64_t` -- the sample axis. `0` by default.

## Errors

None. `batch_dim()` is `noexcept` and has no preconditions.

## Practical Examples

Three examples: the default value, overriding it for data whose samples
run along a non-leading axis, and confirming a `DataLoader` constructed
from such a dataset actually uses it.

### Example 1

`Dataset`'s own default -- axis `0`.

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>
#include <vector>

class ScalarDataset final : public pluss::loader::Dataset {
public:
  explicit ScalarDataset(std::vector<double> values) : values_(std::move(values)) {}
  std::int64_t size() const override { return static_cast<std::int64_t>(values_.size()); }
  pluss::loader::DataBatch get(std::int64_t index) const override {
    pluss::loader::DataBatch batch;
    batch.fields.push_back(pluss::make_series(values_[static_cast<std::size_t>(index)]));
    return batch;
  }
  std::vector<pluss::Series> materialize_fields() const override {
    return {pluss::make_series(values_, {static_cast<std::int64_t>(values_.size())})};
  }

private:
  std::vector<double> values_;
};

int main() {
  ScalarDataset dataset({1.0, 2.0, 3.0});
  std::cout << "batch_dim() = " << dataset.batch_dim() << " (default)\n";
  return 0;
}
```

Output

```text
batch_dim() = 0 (default)
```

### Example 2

A dataset holding a 2-feature-by-4-sample matrix, samples along axis 1 --
`batch_dim()` is overridden to `1`, and `size()` correctly reports the
count along that axis rather than axis 0's feature count.

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

class ColumnSamplesDataset final : public pluss::loader::Dataset {
public:
  explicit ColumnSamplesDataset(pluss::Series data) : data_(std::move(data)) {}

  std::int64_t size() const override { return data_.sizes()[1]; }

  pluss::loader::DataBatch get(std::int64_t index) const override {
    pluss::loader::DataBatch batch;
    batch.fields.push_back(data_.select(1, index));
    return batch;
  }

  std::vector<pluss::Series> materialize_fields() const override { return {data_}; }
  std::int64_t batch_dim() const noexcept override { return 1; }

private:
  pluss::Series data_;
};

int main() {
  const pluss::Series data = pluss::make_series({1.0, 2.0, 3.0, 4.0, 10.0, 20.0, 30.0, 40.0}, {2, 4});
  ColumnSamplesDataset dataset(data);
  std::cout << "data.sizes() = [" << data.sizes()[0] << ", " << data.sizes()[1] << "]\n";
  std::cout << "batch_dim() = " << dataset.batch_dim() << ", size() = " << dataset.size() << "\n";
  return 0;
}
```

Output

```text
data.sizes() = [2, 4]
batch_dim() = 1, size() = 4
```

### Example 3

A `DataLoader` constructed from the same `batch_dim() == 1` dataset slices
its batches along axis 1 too -- each batch keeps both feature rows,
slicing only the sample axis.

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

class ColumnSamplesDataset final : public pluss::loader::Dataset {
public:
  explicit ColumnSamplesDataset(pluss::Series data) : data_(std::move(data)) {}
  std::int64_t size() const override { return data_.sizes()[1]; }
  pluss::loader::DataBatch get(std::int64_t index) const override {
    pluss::loader::DataBatch batch;
    batch.fields.push_back(data_.select(1, index));
    return batch;
  }
  std::vector<pluss::Series> materialize_fields() const override { return {data_}; }
  std::int64_t batch_dim() const noexcept override { return 1; }

private:
  pluss::Series data_;
};

int main() {
  const pluss::Series data = pluss::make_series({1.0, 2.0, 3.0, 4.0, 10.0, 20.0, 30.0, 40.0}, {2, 4});
  ColumnSamplesDataset dataset(data);

  pluss::loader::DataLoaderOptions options{};
  options.batch_size = 2;
  pluss::loader::DataLoader data_loader(dataset, options);
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
- [`materialize_fields`](materialize_fields.md) -- every returned field
  shares this axis as its sample dimension.
- [`DataLoaderOptions::batch_dim`](../DataLoaderOptions.md) -- the
  equivalent option when constructing a `DataLoader` directly from
  `Series` fields, without a `Dataset` at all.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="get.html">
    <span class="label">Next</span>
    <span class="title">get</span>
  </a>
</div>
<!-- apiref:nav:end -->
