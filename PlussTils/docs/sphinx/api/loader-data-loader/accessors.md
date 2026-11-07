# `pluss::loader::DataLoader::accessors`

[Back to `DataLoader`](../loader-data-loader.md) · [Loader API Reference](../../api_reference.md)

```cpp
bool empty() const noexcept;
std::size_t field_count() const noexcept;
std::int64_t sample_count() const noexcept;
std::size_t batch_count() const noexcept;
std::int64_t batch_size() const noexcept;
const DataLoaderOptions& options() const noexcept;
std::uint64_t epoch() const noexcept;
```

These members read back what a loader was built with and where it currently stands, without changing anything. `empty` is true for a default-constructed loader or one with zero samples. `batch_count` is computed from the sample count, the batch size, and `drop_last`; `epoch` is the epoch `set_epoch` or `reset` last moved to.

## Parameters

These members take no parameters.

## Returns

Each returns the value its name describes.

## Errors

None.

## Practical Examples

### Example 1

Example 1 reads every accessor on a loader built from two fields.

```cpp
// empty, field_count, sample_count, batch_size, and options read back what the loader was built
// with, without changing anything.
#include <pluss/tils/auxiliary/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  std::cout << std::boolalpha;
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{0, 1, 2, 3, 4, 5, 6}, {7}, cfg);
  const pluss::Series y = pluss::make_series(std::vector<double>{0, 1, 0, 1, 0, 1, 0}, {7}, cfg);
  loader::DataLoaderOptions options{};
  options.batch_size = 3;
  loader::DataLoader data({x, y}, options);

  std::cout << "empty: " << data.empty() << '\n';
  std::cout << "field_count: " << data.field_count() << '\n';
  std::cout << "sample_count: " << data.sample_count() << '\n';
  std::cout << "batch_size: " << data.batch_size() << '\n';
  std::cout << "options().batch_size matches: " << (data.options().batch_size == 3) << '\n';

  const loader::DataLoader empty_loader;
  std::cout << "default-constructed loader is empty: " << empty_loader.empty() << '\n';
  return 0;
}
```

Output

```text
empty: false
field_count: 2
sample_count: 7
batch_size: 3
options().batch_size matches: true
default-constructed loader is empty: true
```

The field count, sample count, and batch size each match what the loader was built with, and a default-constructed loader is empty.

### Example 2

Example 2 reads epoch after set_epoch moves to a chosen epoch.

```cpp
// epoch() reads back the epoch set_epoch moved to, so a caller can tell which pass through the data
// a loader is currently on.
#include <pluss/tils/auxiliary/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{0, 1, 2, 3}, {4}, cfg);
  loader::DataLoaderOptions options{};
  options.batch_size = 4;
  options.shuffle = true;
  options.seed = 1;
  loader::DataLoader data({x}, options);

  std::cout << "epoch at construction: " << data.epoch() << '\n';
  data.set_epoch(5);
  std::cout << "epoch after set_epoch(5): " << data.epoch() << '\n';
  return 0;
}
```

Output

```text
epoch at construction: 0
epoch after set_epoch(5): 5
```

epoch starts at zero and reads back the value set_epoch was given.

### Example 3

Example 3 reads sample_count and batch_count on the loader from the first example.

```cpp
#include <pluss/tils/auxiliary/data_loader.h>
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

Seven samples in batches of three give three batches, the last one smaller.

## See Also

- [`DataLoader`](../loader-data-loader.md)
- [Reset and epoch](reset-and-epoch.md)
