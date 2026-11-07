# `pluss::loader::DataLoader::iteration`

[Back to `DataLoader`](../loader-data-loader.md) · [Loader API Reference](../../api_reference.md)

```cpp
std::optional<batch_type> next();
batch_type batch_at(std::size_t batch_index) const;
iterator begin();
iterator end();
```

There are three ways to pull batches out of a loader. `next` pulls one batch and advances the loader's own cursor, returning `std::nullopt` once every batch has been delivered for the current epoch. `batch_at` reads one batch by its index, without touching the cursor. `begin`/`end` give an input iterator, so a loader can be used directly in a range-based for loop; `begin` resets the cursor and, if `reshuffle_each_epoch` is set, starts the next epoch and reshuffles.

## Parameters

| Parameter | Meaning |
|---|---|
| `batch_index` | The batch's position, for `batch_at`. Refused if it is not less than `batch_count()`. |

## Returns

`next` returns the next batch, or nothing once the epoch is exhausted. `batch_at` returns that one batch. `begin`/`end` return an `iterator` that dereferences to a batch.

## Errors

`batch_at` refuses an index that is not less than `batch_count()`.

## Practical Examples

### Example 1

Example 1 pulls batches one at a time with next, until it returns nothing.

```cpp
// next() pulls one batch and advances the loader's own cursor, so repeated calls walk through the
// epoch without a range-for. It returns nullopt once every batch has been delivered.
#include <pluss/tils/kit/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  std::cout << std::boolalpha;
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{0, 1, 2, 3, 4}, {5}, cfg);
  loader::DataLoaderOptions options{};
  options.batch_size = 2;
  loader::DataLoader data({x}, options);

  std::size_t pulled = 0;
  while (const auto batch = data.next()) {
    ++pulled;
    std::cout << "batch " << pulled << " rows: " << batch->x().sizes()[0] << '\n';
  }
  std::cout << "batches pulled: " << pulled << '\n';
  std::cout << "next past the end: " << data.next().has_value() << '\n';
  return 0;
}
```

Output

```text
batch 1 rows: 2
batch 2 rows: 2
batch 3 rows: 1
batches pulled: 3
next past the end: false
```

Each call advances the cursor, and the final call past the end returns nothing.

### Example 2

Example 2 reads a loader in a range-based for loop.

```cpp
#include <pluss/tils/kit/data_loader.h>
#include <pluss/pluss.h>

#include <algorithm>
#include <iostream>
#include <vector>

namespace {

// The order of samples for one full pass, read through the batches the loader delivers.
std::vector<double> one_pass(pluss::loader::DataLoader& loader) {
  std::vector<double> order;
  for (auto batch : loader) {
    const auto values = batch.x().reshape({batch.x().numel()}).pull<double>();
    order.insert(order.end(), values.begin(), values.end());
  }
  return order;
}

} // namespace

int main() {
  std::cout << std::boolalpha;
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{0, 1, 2, 3, 4, 5, 6, 7}, {8}, cfg);

  loader::DataLoaderOptions options{};
  options.batch_size = 4;
  options.shuffle = true;
  options.seed = 42;

  // Two loaders built from the same seed produce the same first pass.
  loader::DataLoader first(std::vector<pluss::Series>{x}, options);
  loader::DataLoader second(std::vector<pluss::Series>{x}, options);
  const auto first_order = one_pass(first);
  std::cout << "same seed, same order: " << (first_order == one_pass(second)) << '\n';

  // A shuffled pass still visits every sample exactly once.
  auto sorted = first_order;
  std::sort(sorted.begin(), sorted.end());
  std::cout << "every sample once: " << (sorted == std::vector<double>{0, 1, 2, 3, 4, 5, 6, 7}) << '\n';

  // Moving to a new epoch reshuffles by default. Returning to epoch zero replays it.
  first.set_epoch(1);
  const auto epoch_one = one_pass(first);
  std::cout << "epoch one differs from epoch zero: " << (epoch_one != first_order) << '\n';
  first.set_epoch(0);
  std::cout << "epoch zero replays: " << (one_pass(first) == first_order) << '\n';
}
```

Output

```text
same seed, same order: true
every sample once: true
epoch one differs from epoch zero: true
epoch zero replays: true
```

The loop visits every sample exactly once per pass, in the order begin and the iterator produce.

### Example 3

Example 3 reads batches by index, comparing a loader that keeps a partial batch with one that drops it.

```cpp
#include <pluss/tils/kit/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

int main() {
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{0, 1, 2, 3, 4, 5, 6}, {7}, cfg);

  // Keeping the partial batch: seven samples in batches of three give 3, 3, 1.
  loader::DataLoaderOptions keep{};
  keep.batch_size = 3;
  keep.drop_last = false;
  loader::DataLoader kept({x}, keep);

  // Dropping it: the one leftover sample is never delivered.
  loader::DataLoaderOptions drop{};
  drop.batch_size = 3;
  drop.drop_last = true;
  loader::DataLoader dropped({x}, drop);

  std::cout << "kept batches: " << kept.batch_count() << '\n';
  std::cout << "dropped batches: " << dropped.batch_count() << '\n';
  std::cout << "last kept batch rows: " << kept.batch_at(kept.batch_count() - 1).x().sizes()[0] << '\n';
}
```

Output

```text
kept batches: 3
dropped batches: 2
last kept batch rows: 1
```

The kept loader has one more batch than the dropped one, and its last batch is the single leftover sample.

### Example 4

Example 4 reads an index past the end.

```cpp
#include <pluss/tils/kit/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

namespace {

// Returns true if the callable throws. The message itself is not printed, so this
// example's output does not depend on library wording.
template <class Fn>
bool throws(Fn fn) {
  try {
    fn();
  } catch (const std::exception&) {
    return true;
  }
  return false;
}

} // namespace

int main() {
  std::cout << std::boolalpha;
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{0, 1, 2, 3}, {4}, cfg);

  // Prefetching depth is a worker setting. Without workers it is an error, not a no-op.
  loader::DataLoaderOptions prefetch_without_workers{};
  prefetch_without_workers.prefetch_factor = 4;
  std::cout << "prefetch without workers rejected: "
            << throws([&] { loader::DataLoader bad({x}, prefetch_without_workers); }) << '\n';

  // Reading past the last batch is an error. Batch indices start at zero.
  loader::DataLoader two_batches({x}, [] {
    loader::DataLoaderOptions o{};
    o.batch_size = 2;
    return o;
  }());
  std::cout << "batch count: " << two_batches.batch_count() << '\n';
  std::cout << "reading batch two rejected: "
            << throws([&] { (void)two_batches.batch_at(two_batches.batch_count()); }) << '\n';

  // A zero batch size cannot describe any batch.
  loader::DataLoaderOptions zero{};
  zero.batch_size = 0;
  std::cout << "zero batch size rejected: "
            << throws([&] { loader::DataLoader bad({x}, zero); }) << '\n';
}
```

Output

```text
prefetch without workers rejected: true
batch count: 2
reading batch two rejected: true
zero batch size rejected: true
```

The out-of-range index is refused, among the other refusals this program checks.

## See Also

- [`DataLoader`](../loader-data-loader.md)
- [Reset and epoch](reset-and-epoch.md)
