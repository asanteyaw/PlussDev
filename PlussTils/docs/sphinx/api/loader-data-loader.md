# `pluss::loader::DataLoader`

[Back to Loaders](loaders.md) · [Loader API Reference](../api_reference.md)

```cpp
#include <pluss/tils/aux/data_loader.h>

class DataLoader {
public:
  using batch_type = DataBatch;

  DataLoader() = default;
  explicit DataLoader(const Dataset& dataset, DataLoaderOptions options = {});
  explicit DataLoader(const SeriesDataset& dataset, DataLoaderOptions options = {});
  explicit DataLoader(std::vector<Series> fields, DataLoaderOptions options = {});
  DataLoader(std::initializer_list<Series> fields, DataLoaderOptions options = {});

  DataLoader& set_options(DataLoaderOptions options);
};
```

`DataLoader` turns a dataset into a sequence of batches, in the order its options describe: sequential or shuffled, with or without a custom sampler, with or without background workers. Build one from a list of fields, from a `SeriesDataset`, or from any `Dataset` subclass; all four constructors validate the options and compute the batch plan immediately, so a bad configuration is refused at construction, not on first use.

`DataLoader` is move-only: a copy would leave two loaders racing over the same background worker threads. `set_options` replaces the configuration after construction and recomputes the batch plan the same way the constructor does.

## Parameters

| Parameter | Meaning |
|---|---|
| `dataset` / `fields` | The data to read: a `Dataset`, a `SeriesDataset`, or a plain list of fields. |
| `options` | The batching, shuffling, transfer, and worker configuration. See [`DataLoaderOptions`](loader-data-loader-options.md). |

## Returns

A `DataLoader` ready to produce batches. `set_options` returns the loader, so the call chains.

## Errors

A batch size that is not positive, a `shuffle` and `sampler` set together, or a `prefetch_factor` given without `num_workers`, are all refused at construction or at `set_options`.

## Practical Examples

### Example 1

Example 1 builds a loader from a list of fields and reads every batch by index.

```cpp
#include <pluss/tils/aux/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

int main() {
  namespace loader = pluss::loader;

  // Seven samples with two fields. Field x holds a row of two values per sample;
  // field y holds one label per sample. Both fields share the same batch dimension.
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{
      0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13}, {7, 2}, cfg);
  const pluss::Series y = pluss::make_series(std::vector<double>{0, 1, 0, 1, 0, 1, 0}, {7}, cfg);

  loader::DataLoaderOptions options{};
  options.batch_size = 3;
  loader::DataLoader batches_source({x, y}, options);

  std::cout << "samples: " << batches_source.sample_count() << '\n';
  std::cout << "batches: " << batches_source.batch_count() << '\n';

  // Sequential order visits samples 0..6 in turn. The last batch is smaller.
  for (std::size_t i = 0; i < batches_source.batch_count(); ++i) {
    const auto batch = batches_source.batch_at(i);
    std::cout << "batch " << i << " rows: " << batch.x().sizes()[0]
              << ", label sum: " << batch.y().sum().item<double>() << '\n';
  }
}
```

Output

```text
samples: 7
batches: 3
batch 0 rows: 3, label sum: 1
batch 1 rows: 3, label sum: 2
batch 2 rows: 1, label sum: 0
```

The loader reports its sample and batch counts, and each batch holds the rows and the label sum for its slice.

### Example 2

Example 2 builds a loader from a custom Dataset.

```cpp
#include <pluss/tils/aux/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

namespace {

// A map-style dataset that computes each sample on demand instead of storing a table.
// Sample i has feature i and label i modulo 2.
class SquaresDataset final : public pluss::loader::Dataset {
public:
  explicit SquaresDataset(std::int64_t count) : count_(count) {}

  [[nodiscard]] std::int64_t size() const override { return count_; }

  [[nodiscard]] pluss::loader::DataBatch get(std::int64_t index) const override {
    const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
    pluss::loader::DataBatch sample{};
    sample.fields.push_back(pluss::make_series(std::vector<double>{static_cast<double>(index * index)}, {1}, cfg));
    sample.fields.push_back(pluss::make_series(std::vector<double>{static_cast<double>(index % 2)}, {1}, cfg));
    return sample;
  }

private:
  std::int64_t count_;
};

} // namespace

int main() {
  namespace loader = pluss::loader;
  const SquaresDataset dataset(5);

  // Without a collate function, the loader stacks samples into batches.
  loader::DataLoaderOptions options{};
  options.batch_size = 2;
  loader::DataLoader plain(dataset, options);
  std::cout << "samples: " << plain.sample_count() << '\n';
  std::cout << "batches: " << plain.batch_count() << '\n';
  std::cout << "last batch squares: " << plain.batch_at(2).x().sum().item<double>() << '\n';

  // A collate function receives each assembled batch and may transform it. Here it
  // keeps only the labels, so every delivered batch has one field.
  loader::DataLoaderOptions collated = options;
  collated.collate_fn = [](loader::DataBatch batch) {
    loader::DataBatch out{};
    out.fields.push_back(batch.y());
    return out;
  };
  loader::DataLoader labels(dataset, collated);
  std::cout << "collated fields per batch: " << labels.batch_at(0).size() << '\n';
  std::cout << "label sum in batch 0: " << labels.batch_at(0).x().sum().item<double>() << '\n';
}
```

Output

```text
samples: 5
batches: 3
last batch squares: 16
collated fields per batch: 1
label sum in batch 0: 1
```

The loader reads the dataset the same way it reads a plain field list, computing samples through the dataset's own get.

### Example 3

Example 3 replaces the configuration after construction with set_options.

```cpp
// set_options replaces the whole configuration after construction and rebuilds the batch plan, the
// same validation and ordering the constructor runs.
#include <pluss/tils/aux/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{0, 1, 2, 3, 4, 5}, {6}, cfg);

  loader::DataLoaderOptions start{};
  start.batch_size = 2;
  loader::DataLoader data({x}, start);
  std::cout << "batches with size 2: " << data.batch_count() << '\n';

  loader::DataLoaderOptions changed{};
  changed.batch_size = 3;
  data.set_options(changed);
  std::cout << "batches with size 3: " << data.batch_count() << '\n';
  std::cout << "options reflects the change: " << (data.options().batch_size == 3) << '\n';
  return 0;
}
```

Output

```text
batches with size 2: 3
batches with size 3: 2
options reflects the change: 1
```

The batch count changes to match the new batch size, and options() reflects the change.

## See Also

- [Accessors](loader-data-loader/accessors.md)
- [Iteration](loader-data-loader/iteration.md)
- [Reset and epoch](loader-data-loader/reset-and-epoch.md)
- [`DataLoaderOptions`](loader-data-loader-options.md)
- [`Dataset`](loader-dataset.md)
- [`DataBatch`](loader-data-batch.md)
