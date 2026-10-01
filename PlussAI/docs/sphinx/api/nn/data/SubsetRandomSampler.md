# `pluss::loader::SubsetRandomSampler`

A shuffle of a fixed, caller-supplied index subset -- every epoch draws
from exactly the same indices (a train split held out in advance, say),
re-shuffled each time. Unlike [`RandomSampler`](RandomSampler.md), the
subset itself is never re-derived from `size` -- it is whatever was
passed to the constructor, verbatim.

## Signature

```cpp
explicit pluss::loader::SubsetRandomSampler::SubsetRandomSampler(std::vector<std::int64_t> indices);

std::vector<std::int64_t> pluss::loader::SubsetRandomSampler::sample(
    std::int64_t size, std::uint64_t epoch_seed) const override;
```

## Construction

```cpp
pluss::loader::SubsetRandomSampler sampler({0, 2, 4, 7});
```

## Parameters

`std::vector<std::int64_t> indices`
: The fixed index subset every epoch draws from, in whatever order given
  -- `sample()` shuffles a copy of this list each call, never the
  original order.

## State

The index subset, fixed for the object's lifetime.

## Methods

`sample(size, epoch_seed)` -- `vector<int64_t>`
: Returns the constructor's `indices`, shuffled -- `size` is accepted (to
  satisfy [`Sampler`](Sampler.md)'s shared signature) but otherwise
  ignored; the subset never depends on it.

## Practical Examples

Three examples: a fixed subset shuffled once, the same subset reshuffled
under a different `epoch_seed`, and constructing one from a real
[`split_fields`](split_fields.md)-style held-out index list.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  loader::SubsetRandomSampler sampler({1, 3});
  const auto order = sampler.sample(/*size=*/10, /*epoch_seed=*/1);

  for (auto i : order) std::cout << i << " ";
  std::cout << "\n";

  return 0;
}
```

Output

```text
1 3 
```

### Example 2

A larger subset, reshuffled under two different `epoch_seed` values --
the same three indices, different order each time.

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  loader::SubsetRandomSampler sampler({2, 5, 8});
  const auto epoch0 = sampler.sample(10, 10);
  const auto epoch1 = sampler.sample(10, 20);

  std::cout << "epoch 0: ";
  for (auto i : epoch0) std::cout << i << " ";
  std::cout << "\nepoch 1: ";
  for (auto i : epoch1) std::cout << i << " ";
  std::cout << "\n";

  return 0;
}
```

Output

```text
epoch 0: 8 5 2 
epoch 1: 5 8 2 
```

### Example 3

Wired directly into a [`DataLoader`](DataLoader.md) via
[`DataLoaderOptions::sampler`](DataLoaderOptions.md) -- training only on
samples `{0, 2, 4}` out of a 5-sample dataset.

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>
#include <memory>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series x = pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}, {5, 1});

  loader::DataLoaderOptions options{};
  options.batch_size = 5;
  options.sampler = std::make_shared<loader::SubsetRandomSampler>(std::vector<std::int64_t>{0, 2, 4});

  loader::DataLoader data_loader({x}, options);
  for (auto batch : data_loader) {
    std::cout << "batch: " << batch.x() << "\n";
  }

  return 0;
}
```

Output

```text
batch: pluss::Series(shape=[3,1], strides=[1,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[10],
  [30],
  [50]]
```

## Changelog

- **0.1** — a fixed index subset, shuffled each epoch.

## See Also

- [`Sampler`](Sampler.md) -- the interface this implements.
- [`RandomSampler`](RandomSampler.md) -- shuffles the *full* `[0, size)`
  range rather than a fixed subset.
- [`DataLoaderOptions::sampler`](DataLoaderOptions.md) -- where this gets
  wired in.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
