<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 7b69f873311b -->

# `pluss::loader::Dataset::size`

[Back to `pluss::loader::Dataset`](../Dataset.md)

## Signature

```cpp
virtual std::int64_t pluss::loader::Dataset::size() const = 0;
```

## What It Does

Reports how many samples this dataset holds, counted along whatever axis
[`batch_dim()`](batch_dim.md) names. Pure virtual -- every subclass must
implement it. `DataLoader` reads it (indirectly, via each field's own
shape after `materialize_fields()`) to know how many batches a full epoch
contains; a directly-held `Dataset&`/`Dataset*` calling code holds can
also call it standalone, with no `DataLoader` involved at all.

## Parameters

None.

## Returns

`std::int64_t` -- the sample count. Must stay consistent with
`materialize_fields()`'s own field shapes along `batch_dim()` for
`DataLoader` construction to succeed.

## Errors

None from `size()` itself -- a subclass's own implementation may throw if
it has its own preconditions, but the base contract imposes none.

## Practical Examples

Three examples: `size()` on a small custom dataset, `size()` counted
along a non-default `batch_dim`, and `size()` read back through a
constructed `DataLoader`.

### Example 1

A custom `Dataset` over five in-memory scalars -- `size()` simply reports
how many.

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
  ScalarDataset dataset({10.0, 20.0, 30.0, 40.0, 50.0});
  std::cout << "size() = " << dataset.size() << "\n";
  return 0;
}
```

Output

```text
size() = 5
```

### Example 2

A dataset whose samples run along axis 1 (`batch_dim() == 1`, columns of
a 2-feature-by-4-sample matrix) -- `size()` reports `4`, the count along
that axis, not the total element count or the feature count.

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
  std::cout << "size() = " << dataset.size() << " (batch_dim=" << dataset.batch_dim() << ")\n";
  return 0;
}
```

Output

```text
size() = 4 (batch_dim=1)
```

### Example 3

`DataLoader::batch_count()` depends on `size()` indirectly (through the
materialized fields' own shape) -- five samples at `batch_size = 2`
produce three batches (two full, one partial), matching `ceil(5 / 2)`.

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
  ScalarDataset dataset({10.0, 20.0, 30.0, 40.0, 50.0});
  pluss::loader::DataLoaderOptions options{};
  options.batch_size = 2;
  pluss::loader::DataLoader data_loader(dataset, options);
  std::cout << "dataset.size() = " << dataset.size() << ", data_loader.batch_count() = " << data_loader.batch_count() << "\n";
  return 0;
}
```

Output

```text
dataset.size() = 5, data_loader.batch_count() = 3
```

## See Also

- [`batch_dim`](batch_dim.md) -- which axis `size()` counts along.
- [`get`](get.md) -- valid indices are `[0, size())`.
- [`DataLoader::sample_count`](../DataLoader/sample_count.md) -- the same
  count, read back from a constructed `DataLoader`.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="materialize_fields.html">
    <span class="label">Previous</span>
    <span class="title">materialize_fields</span>
  </a>
</div>
<!-- apiref:nav:end -->
