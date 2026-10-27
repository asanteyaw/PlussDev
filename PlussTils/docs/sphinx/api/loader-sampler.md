# `pluss::loader::Sampler and its implementations`

[Back to Loaders](loaders.md) · [Loader API Reference](../api_reference.md)

## Signature

```cpp
#include <pluss/tils/aux/sampler.h>

virtual std::vector<std::int64_t> Sampler::sample(std::int64_t size, std::uint64_t epoch_seed) const = 0;

SequentialSampler::SequentialSampler();
explicit RandomSampler::RandomSampler(bool replacement = false, std::int64_t num_samples = -1);
explicit SubsetRandomSampler::SubsetRandomSampler(std::vector<std::int64_t> indices);
explicit WeightedRandomSampler::WeightedRandomSampler(std::vector<double> weights, std::int64_t num_samples, bool replacement = true);
```

`SequentialSampler`, `RandomSampler`, `SubsetRandomSampler`, and
`WeightedRandomSampler` each derive from `Sampler` and implement `sample`
with their own strategy; none adds any member beyond its own constructor.

## What It Does

A `Sampler` decides the order `DataLoader` visits a dataset's indices in, for one epoch. `sample` returns the whole order up front, as a plain vector, rather than producing one index at a time; the caller passes `epoch_seed`, already offset for the current epoch, so the same sampler instance gives a different but reproducible order each epoch. Set one on `DataLoaderOptions::sampler`; it is mutually exclusive with the plain `shuffle` flag.

`SequentialSampler` visits every index in order, ignoring the seed. `RandomSampler` shuffles every index; with `replacement` left false, it visits each index exactly once and `num_samples` may not exceed the dataset size; with it true, `num_samples` may be any positive count, and the same index can repeat. `SubsetRandomSampler` shuffles a fixed list of indices you supply, visiting only those. `WeightedRandomSampler` picks indices with probability proportional to a weight per index; `num_samples` is independent of the dataset size, and without replacement it cannot exceed the number of indices with a nonzero weight.

## Parameters

| Parameter | Meaning |
|---|---|
| `size` | The dataset's sample count, passed by the loader. |
| `epoch_seed` | A seed already offset for the current epoch. |
| `replacement` | Whether an index may be visited more than once. |
| `num_samples` | How many indices to produce. `-1` on `RandomSampler` means exactly one pass. |
| `indices` | The fixed list `SubsetRandomSampler` shuffles. |
| `weights` | One weight per index, for `WeightedRandomSampler`. |

## Returns

`sample` returns the full visiting order for one epoch, as a `std::vector<std::int64_t>`.

## Errors

None from the samplers themselves. The loader refuses a sampler set together with `shuffle`.

## Practical Examples

### Example 1

Example 1 visits every index in order with SequentialSampler.

```cpp
// SequentialSampler visits every index in order, ignoring the seed entirely. It is the sampler a
// loader with shuffle left off would behave like, if you wanted one as an explicit object.
#include <pluss/tils/aux/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  std::cout << std::boolalpha;
  namespace loader = pluss::loader;
  const loader::SequentialSampler sampler;
  const auto order = sampler.sample(4, /*epoch_seed=*/999);
  std::cout << "order: ";
  for (auto index : order) std::cout << index << ' ';
  std::cout << '\n';
  std::cout << "in order: " << (order == std::vector<std::int64_t>{0, 1, 2, 3}) << '\n';
  return 0;
}
```

Output

```text
order: 0 1 2 3 
in order: true
```

The order is 0, 1, 2, 3, ignoring the seed entirely.

### Example 2

Example 2 shuffles without replacement with RandomSampler.

```cpp
// RandomSampler without replacement visits every index exactly once, in a shuffled order. The
// same seed, passed through DataLoaderOptions, gives the same order every time.
#include <pluss/tils/aux/data_loader.h>
#include <pluss/pluss.h>

#include <algorithm>
#include <iostream>

int main() {
  std::cout << std::boolalpha;
  namespace loader = pluss::loader;
  const loader::RandomSampler sampler(/*replacement=*/false);
  const auto order = sampler.sample(5, /*epoch_seed=*/7);
  std::cout << "visits: " << order.size() << '\n';
  auto sorted = order;
  std::sort(sorted.begin(), sorted.end());
  std::cout << "every index once: " << (sorted == std::vector<std::int64_t>{0, 1, 2, 3, 4}) << '\n';
  std::cout << "same seed, same order: " << (sampler.sample(5, 7) == order) << '\n';
  return 0;
}
```

Output

```text
visits: 5
every index once: true
same seed, same order: true
```

Every index is visited exactly once, and the same seed gives the same order.

### Example 3

Example 3 shuffles with replacement and more samples than the dataset size.

```cpp
// With replacement, num_samples may exceed the dataset size, and the same index can appear more
// than once.
#include <pluss/tils/aux/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;
  const loader::RandomSampler sampler(/*replacement=*/true, /*num_samples=*/8);
  const auto order = sampler.sample(3, /*epoch_seed=*/1);
  std::cout << "visits: " << order.size() << '\n';
  bool in_range = true;
  for (auto index : order) in_range = in_range && index >= 0 && index < 3;
  std::cout << "every index in range: " << in_range << '\n';
  return 0;
}
```

Output

```text
visits: 8
every index in range: 1
```

Eight samples come from a dataset of three indices, and every one is in range.

### Example 4

Example 4 uses SubsetRandomSampler, and WeightedRandomSampler favoring one index.

```cpp
// WeightedRandomSampler picks indices with probability proportional to their weight. A weight of
// zero is never picked, and the sample count is independent of the dataset size.
#include <pluss/tils/aux/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  std::cout << std::boolalpha;
  namespace loader = pluss::loader;
  const loader::WeightedRandomSampler sampler(std::vector<double>{0.0, 1.0, 0.0}, /*num_samples=*/10);
  const auto order = sampler.sample(3, /*epoch_seed=*/3);
  std::cout << "visits: " << order.size() << '\n';
  bool only_index_one = true;
  for (auto index : order) only_index_one = only_index_one && index == 1;
  std::cout << "only the weighted index is picked: " << only_index_one << '\n';
  return 0;
}
```

Output

```text
visits: 10
only the weighted index is picked: true
```

Every sample picks the one index with a nonzero weight.

### Example 5

Example 5 uses WeightedRandomSampler without replacement.

```cpp
// Without replacement, the sampler cannot deliver more samples than there are indices to draw, and
// each index is picked at most once.
#include <pluss/tils/aux/data_loader.h>
#include <pluss/pluss.h>

#include <algorithm>
#include <iostream>

int main() {
  std::cout << std::boolalpha;
  namespace loader = pluss::loader;
  const loader::WeightedRandomSampler sampler(std::vector<double>{1.0, 1.0, 1.0, 1.0}, /*num_samples=*/4, /*replacement=*/false);
  const auto order = sampler.sample(4, /*epoch_seed=*/5);
  std::cout << "visits: " << order.size() << '\n';
  auto sorted = order;
  std::sort(sorted.begin(), sorted.end());
  std::cout << "every index at most once: " << (sorted == std::vector<std::int64_t>{0, 1, 2, 3}) << '\n';
  return 0;
}
```

Output

```text
visits: 4
every index at most once: true
```

Exactly as many samples as indices are produced, and each index is picked at most once.

### Example 6

Example 6 sets a sampler on a loader directly, and shows it rejected alongside shuffle.

```cpp
#include <pluss/tils/aux/data_loader.h>
#include <pluss/pluss.h>

#include <algorithm>
#include <iostream>
#include <memory>
#include <vector>

namespace {

void print_values(const std::vector<double>& values) {
  for (std::size_t i = 0; i < values.size(); ++i) std::cout << (i ? ", " : "") << values[i];
}

std::vector<double> visited(pluss::loader::DataLoader& loader) {
  std::vector<double> out;
  for (auto batch : loader) {
    const auto values = batch.x().reshape({batch.x().numel()}).pull<double>();
    out.insert(out.end(), values.begin(), values.end());
  }
  return out;
}

} // namespace

int main() {
  std::cout << std::boolalpha;
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{0, 1, 2, 3, 4, 5, 6, 7}, {8}, cfg);

  // A sequential sampler visits every sample in index order.
  loader::DataLoaderOptions sequential{};
  sequential.batch_size = 4;
  sequential.sampler = std::make_shared<loader::SequentialSampler>();
  loader::DataLoader in_order({x}, sequential);
  std::cout << "sequential: ";
  print_values(visited(in_order));
  std::cout << '\n';

  // A subset sampler visits only the listed indices, in a random order.
  loader::DataLoaderOptions subset{};
  subset.batch_size = 2;
  subset.sampler = std::make_shared<loader::SubsetRandomSampler>(std::vector<std::int64_t>{4, 5, 6});
  loader::DataLoader only_subset({x}, subset);
  auto picked = visited(only_subset);
  std::sort(picked.begin(), picked.end());
  std::cout << "subset visits exactly 4, 5, 6: " << (picked == std::vector<double>{4, 5, 6}) << '\n';

  // A sampler and the shuffle flag are mutually exclusive. The loader rejects the pair.
  loader::DataLoaderOptions both{};
  both.shuffle = true;
  both.sampler = std::make_shared<loader::SequentialSampler>();
  bool rejected = false;
  try {
    loader::DataLoader conflicting({x}, both);
  } catch (const std::exception&) {
    rejected = true;
  }
  std::cout << "shuffle and sampler together rejected: " << rejected << '\n';
}
```

Output

```text
sequential: 0, 1, 2, 3, 4, 5, 6, 7
subset visits exactly 4, 5, 6: true
shuffle and sampler together rejected: true
```

A sequential sampler and a subset sampler each drive the loader's order, and setting a sampler together with shuffle is refused.

## See Also

- [`DataLoaderOptions`](loader-data-loader-options.md)
