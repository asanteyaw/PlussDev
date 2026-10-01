# `pluss::loader::RandomSampler`

A full shuffle of `[0, size)`, matching
`DataLoaderOptions::shuffle = true`'s own behavior exactly when
constructed with its defaults -- plus two things a plain bool cannot
express: sampling *with replacement*, and an explicit `num_samples`
different from the dataset's own size (fewer samples per epoch, or --
with replacement -- more).

## Signature

```cpp
explicit pluss::loader::RandomSampler::RandomSampler(bool replacement = false, std::int64_t num_samples = -1);

std::vector<std::int64_t> pluss::loader::RandomSampler::sample(
    std::int64_t size, std::uint64_t epoch_seed) const override;
```

## Construction

```cpp
pluss::loader::RandomSampler default_sampler;                       // one full shuffled pass
pluss::loader::RandomSampler with_replacement(/*replacement=*/true);
pluss::loader::RandomSampler fewer_samples(/*replacement=*/false, /*num_samples=*/100);
```

## Parameters

`bool replacement = false`
: When `true`, each drawn index is independent -- the same index can
  appear more than once in one epoch's order, and `num_samples` may
  exceed `size`. When `false` (the default), the order is a permutation
  of a subset (or all) of `[0, size)` -- every index appears at most
  once.

`std::int64_t num_samples = -1`
: How many indices to draw. `-1` (the default) means "exactly one pass
  over the dataset" -- `size` samples. Any non-negative value overrides
  that; without replacement, it must not exceed `size` (throws
  otherwise).

## State

The two constructor arguments, fixed for the object's lifetime.

## Methods

`sample(size, epoch_seed)` -- `vector<int64_t>`
: Returns the shuffled (and possibly resized/resampled) order. See
  [`Sampler::sample`](Sampler/sample.md) for the shared contract.

## Practical Examples

Three examples: the default (one full shuffled pass), sampling with
replacement, and the error `num_samples` without replacement throws when
it exceeds `size`.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  loader::RandomSampler sampler;
  const auto order = sampler.sample(5, /*epoch_seed=*/42);

  for (auto i : order) std::cout << i << " ";
  std::cout << "\n";

  return 0;
}
```

Output

```text
0 3 4 2 1 
```

### Example 2

`replacement = true` with `num_samples = 8` over a 3-sample dataset --
more draws than the dataset holds, with repeats.

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  loader::RandomSampler sampler(/*replacement=*/true, /*num_samples=*/8);
  const auto order = sampler.sample(3, /*epoch_seed=*/7);

  for (auto i : order) std::cout << i << " ";
  std::cout << "\n";

  return 0;
}
```

Output

```text
2 2 2 1 0 1 2 1 
```

### Example 3

Without replacement, `num_samples` greater than `size` throws -- there
are not enough distinct indices to draw that many.

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>
#include <string>

int main() {
  namespace loader = pluss::loader;

  loader::RandomSampler sampler(/*replacement=*/false, /*num_samples=*/10);
  try {
    sampler.sample(5, 0);
  } catch (const std::exception& e) {
    const std::string message(e.what());
    std::cout << "caught: " << message.substr(0, message.find('\n')) << "\n";
  }

  return 0;
}
```

Output

```text
caught: pluss::tils::RandomSampler: num_samples must be <= size when replacement=false
```

## Changelog

- **0.1** — shuffled sampling, with optional replacement and an explicit
  `num_samples`.

## See Also

- [`Sampler`](Sampler.md) -- the interface this implements.
- [`DataLoaderOptions::shuffle`](DataLoaderOptions.md) -- the plain-bool
  equivalent this `Sampler` matches with its defaults.
- [`SubsetRandomSampler`](SubsetRandomSampler.md) -- a fixed subset,
  shuffled, rather than the full `[0, size)` range.
- [`WeightedRandomSampler`](WeightedRandomSampler.md) -- non-uniform
  sampling.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
