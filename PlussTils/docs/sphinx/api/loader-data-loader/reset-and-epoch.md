# `pluss::loader::DataLoader::reset-and-epoch`

[Back to `DataLoader`](../loader-data-loader.md) · [Loader API Reference](../../api_reference.md)

```cpp
DataLoader& set_epoch(std::uint64_t epoch, bool reshuffle = true);
DataLoader& reset(bool reshuffle = true);
```

`set_epoch` moves to a chosen epoch and rewinds the cursor to the start; with `reshuffle` left at its default, a shuffled loader recomputes its order from the new epoch's seed. `reset` is the same operation, except it always moves to the next epoch rather than a chosen one, which is what `begin` calls automatically when `reshuffle_each_epoch` is set.

## Parameters

| Parameter | Meaning |
|---|---|
| `epoch` | The epoch to move to, for `set_epoch`. |
| `reshuffle` | Whether to recompute the shuffle order for the new epoch. Default true. |

## Returns

The loader, so the call chains.

## Errors

None.

## Practical Examples

### Example 1

Example 1 moves between epochs with set_epoch and checks that the shuffle order is reproducible per epoch.

```cpp
#include <pluss/tils/aux/data_loader.h>
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

A later epoch shuffles differently from the first, and returning to epoch zero replays the exact same order.

### Example 2

Example 2 advances the epoch with reset.

```cpp
// reset() moves to the next epoch and rewinds the cursor, the same way begin() does automatically
// when reshuffle_each_epoch is on. set_epoch jumps to a chosen epoch instead of the next one.
#include <pluss/tils/aux/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{0, 1, 2, 3}, {4}, cfg);
  loader::DataLoaderOptions options{};
  options.batch_size = 4;
  loader::DataLoader data({x}, options);

  std::cout << "epoch before reset: " << data.epoch() << '\n';
  data.reset();
  std::cout << "epoch after reset: " << data.epoch() << '\n';
  data.reset();
  std::cout << "epoch after a second reset: " << data.epoch() << '\n';
  return 0;
}
```

Output

```text
epoch before reset: 0
epoch after reset: 1
epoch after a second reset: 2
```

Each call to reset moves the epoch forward by one, from zero to one to two.

## See Also

- [`DataLoader`](../loader-data-loader.md)
- [Iteration](iteration.md)
