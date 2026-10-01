# `pluss::loader::WeightedRandomSampler`

Draws indices proportional to per-index weights -- the standard fix for
class imbalance: give minority-class samples a larger weight, and they
turn up proportionally more often across an epoch than their raw count
in the dataset would produce.

## Signature

```cpp
explicit pluss::loader::WeightedRandomSampler::WeightedRandomSampler(
    std::vector<double> weights, std::int64_t num_samples, bool replacement = true);

std::vector<std::int64_t> pluss::loader::WeightedRandomSampler::sample(
    std::int64_t size, std::uint64_t epoch_seed) const override;
```

## Construction

```cpp
pluss::loader::WeightedRandomSampler sampler({1.0, 1.0, 5.0, 1.0}, /*num_samples=*/4);
```

## Parameters

`std::vector<double> weights`
: One weight per dataset index -- larger means more likely to be drawn.
  Must be non-empty; must have exactly `size` entries at `sample()` time
  (throws otherwise). Weights need not sum to `1.0`; they are used as
  relative proportions.

`std::int64_t num_samples`
: How many indices to draw per epoch. Must be `> 0`.

`bool replacement = true`
: When `true` (the default), each draw is independent -- the same index
  can be drawn more than once. When `false`, drawn indices are removed
  from the pool as they are picked, so `num_samples` must not exceed
  `weights.size()`.

## State

`weights`, `num_samples`, and `replacement`, fixed for the object's
lifetime.

## Methods

`sample(size, epoch_seed)` -- `vector<int64_t>`
: Draws `num_samples` indices, weighted by `weights`. `size` must equal
  `weights.size()` (throws otherwise). See
  [`Sampler::sample`](Sampler/sample.md) for the shared contract.

## Practical Examples

Three examples: a weight concentrated entirely on one index, sampling
without replacement, and a realistic class-imbalance weighting.

### Example 1

Weight `1.0` on index `2`, `0.0` everywhere else -- every draw picks
index `2`.

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  loader::WeightedRandomSampler sampler({0.0, 0.0, 1.0, 0.0}, /*num_samples=*/4, /*replacement=*/true);
  const auto order = sampler.sample(4, /*epoch_seed=*/1);

  for (auto i : order) std::cout << i << " ";
  std::cout << "\n";

  return 0;
}
```

Output

```text
2 2 2 2 
```

### Example 2

Equal weights, `replacement = false` -- every draw is distinct, so this
reduces to an ordinary shuffle of a 3-of-4 subset.

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  loader::WeightedRandomSampler sampler({1.0, 1.0, 1.0, 1.0}, /*num_samples=*/3, /*replacement=*/false);
  const auto order = sampler.sample(4, /*epoch_seed=*/5);

  for (auto i : order) std::cout << i << " ";
  std::cout << "\n";

  return 0;
}
```

Output

```text
2 0 1 
```

### Example 3

A minority class (index `0`, weight `9.0`) drawn far more often across a
20-sample epoch than its single raw occurrence in a 4-index dataset would
suggest -- the mechanism behind training-time class-imbalance correction.

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>
#include <array>

int main() {
  namespace loader = pluss::loader;

  // Index 0 is the minority class, weighted 9x every other index.
  loader::WeightedRandomSampler sampler({9.0, 1.0, 1.0, 1.0}, /*num_samples=*/20, /*replacement=*/true);
  const auto order = sampler.sample(4, /*epoch_seed=*/3);

  std::array<int, 4> counts{};
  for (auto i : order) ++counts[static_cast<std::size_t>(i)];
  std::cout << "index 0 drawn " << counts[0] << "/20 times\n";

  return 0;
}
```

Output

```text
index 0 drawn 17/20 times
```

Index `0` carries `9` of the total `12` weight (`9 + 1 + 1 + 1`) -- a `75%`
expected share, close to the `85%` actually drawn here over 20 samples.

## Changelog

- **0.1** — weighted sampling, with or without replacement.

## See Also

- [`Sampler`](Sampler.md) -- the interface this implements.
- [`RandomSampler`](RandomSampler.md) -- uniform (unweighted) sampling.
- [`DataLoaderOptions::sampler`](DataLoaderOptions.md) -- where this gets
  wired in.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
