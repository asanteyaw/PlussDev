# `pluss::loader::Dataset and SeriesDataset`

[Back to Loaders](loaders.md) · [Loader API Reference](../api_reference.md)

## Signature

```cpp
#include <pluss/tils/aux/data_loader.h>

virtual std::int64_t Dataset::size() const = 0;
virtual DataBatch Dataset::get(std::int64_t index) const = 0;
virtual std::vector<Series> Dataset::materialize_fields() const;
virtual std::int64_t Dataset::batch_dim() const noexcept;

explicit SeriesDataset::SeriesDataset(std::vector<Series> fields, std::int64_t batch_dim = 0);
SeriesDataset::SeriesDataset(std::initializer_list<Series> fields, std::int64_t batch_dim = 0);
const std::vector<Series>& SeriesDataset::fields() const noexcept;
```

`SeriesDataset` derives from `Dataset`. `size` and `get` are pure virtual and
must be overridden; `materialize_fields` and `batch_dim` have defaults, so a
subclass only needs to override them when it has a faster whole-dataset path.

## What It Does

`Dataset` is the map-style interface a `DataLoader` reads from: `size` reports the sample count, and `get` returns one sample as a `DataBatch`. Override `materialize_fields` when the whole dataset can be produced as a set of batch-dimension-aligned fields in one call; a `DataLoader` prefers this path when it is available, because it avoids calling `get` once per sample. `batch_dim` names which axis of those fields holds the samples; it defaults to the first axis.

`SeriesDataset` is the built-in `Dataset` that wraps a list of fields you already have as `Series` values. It is what a `DataLoader` builds internally when you construct it from a plain list of fields, so building one directly gives you the same object with its `fields` and `batch_dim` available on their own.

## Parameters

| Parameter | Meaning |
|---|---|
| `fields` | The fields, all sharing the same sample count on `batch_dim`. |
| `batch_dim` | The axis that holds the samples. Defaults to 0. |
| `index` | The sample's position, for `get`. |

## Returns

`get` returns one sample as a `DataBatch`. `materialize_fields` returns the dataset as its original fields. `fields()` on a `SeriesDataset` returns the fields it was built from.

## Errors

`SeriesDataset` requires at least one field, each with rank at least one, and every field must share the same sample count on `batch_dim`.

## Practical Examples

### Example 1

Example 1 builds a SeriesDataset directly and reads its size, fields, and one sample.

```cpp
// SeriesDataset wraps a list of fields as a map-style Dataset, the same way the loader's own
// constructor does internally when you pass it a plain vector of fields. Building one directly
// gives access to the fields and the batch dimension on their own.
#include <pluss/tils/aux/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{0, 1, 2, 3, 4}, {5}, cfg);
  const pluss::Series y = pluss::make_series(std::vector<double>{0, 1, 0, 1, 0}, {5}, cfg);

  const loader::SeriesDataset dataset({x, y});
  std::cout << "size: " << dataset.size() << '\n';
  std::cout << "field count: " << dataset.fields().size() << '\n';
  std::cout << "batch_dim: " << dataset.batch_dim() << '\n';

  const loader::DataBatch sample = dataset.get(2);
  std::cout << "sample 2 x: " << sample.x().item<double>() << '\n';
  std::cout << "sample 2 y: " << sample.y().item<double>() << '\n';
  return 0;
}
```

Output

```text
size: 5
field count: 2
batch_dim: 0
sample 2 x: 2
sample 2 y: 0
```

The dataset reports its sample count and field count, and get returns the sample at the given index.

### Example 2

Example 2 reads the whole dataset back through materialize_fields.

```cpp
// materialize_fields returns the whole dataset as its original fields, which is what a DataLoader
// built from this dataset reads in one shot, rather than calling get() once per sample.
#include <pluss/tils/aux/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{10, 20, 30}, {3}, cfg);
  const loader::SeriesDataset dataset({x});
  const auto materialized = dataset.materialize_fields();
  std::cout << "materialized field count: " << materialized.size() << '\n';
  std::cout << "values survive: " << (materialized[0].to_vec<double>() == std::vector<double>{10, 20, 30}) << '\n';
  return 0;
}
```

Output

```text
materialized field count: 1
values survive: 1
```

The materialized fields are the same values the dataset was built from.

### Example 3

Example 3 writes a custom Dataset that computes each sample on demand, instead of storing a table.

```cpp
// A custom Dataset computes each sample on demand, rather than storing a table. The loader
// materializes it once, by calling get() for every index, when no faster path is overridden.
#include <pluss/tils/aux/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>

namespace {
class SquaresDataset final : public pluss::loader::Dataset {
public:
  explicit SquaresDataset(std::int64_t count) : count_(count) {}
  [[nodiscard]] std::int64_t size() const override { return count_; }
  [[nodiscard]] pluss::loader::DataBatch get(std::int64_t index) const override {
    const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
    pluss::loader::DataBatch sample{};
    sample.fields.push_back(pluss::make_series(std::vector<double>{static_cast<double>(index * index)}, {1}, cfg));
    return sample;
  }

private:
  std::int64_t count_;
};
} // namespace

int main() {
  const SquaresDataset dataset(4);
  std::cout << "size: " << dataset.size() << '\n';
  std::cout << "sample 3: " << dataset.get(3).x().item<double>() << '\n';
  return 0;
}
```

Output

```text
size: 4
sample 3: 9
```

size and get both work as declared, and get computes its answer rather than reading it from storage.

## See Also

- [`DataLoader`](loader-data-loader.md) -- reads a Dataset to produce batches.
- [`DataBatch`](loader-data-batch.md) -- what `get` returns.
