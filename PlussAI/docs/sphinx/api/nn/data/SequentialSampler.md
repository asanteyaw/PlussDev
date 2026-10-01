# `pluss::loader::SequentialSampler`

The plain `0..size-1` order, unshuffled -- the [`Sampler`](Sampler.md)
equivalent of `DataLoaderOptions::shuffle = false`. Reach for this
directly only when a `Sampler` is required by some other API surface but
the actual order should stay sequential (for instance, alongside a
[`WeightedRandomSampler`](WeightedRandomSampler.md)-driven training run
that also needs an unshuffled evaluation pass over the same dataset).

## Signature

```cpp
pluss::loader::SequentialSampler::SequentialSampler() = default;

std::vector<std::int64_t> pluss::loader::SequentialSampler::sample(
    std::int64_t size, std::uint64_t epoch_seed) const override;
```

## Construction

```cpp
pluss::loader::SequentialSampler sampler;
```

Takes no arguments -- there is nothing to configure.

## State

None.

## Methods

`sample(size, epoch_seed)` -- `vector<int64_t>`
: Returns `[0, 1, ..., size - 1]`, ignoring `epoch_seed` entirely --
  every epoch produces the identical order. See
  [`Sampler::sample`](Sampler/sample.md) for the shared contract.

## Practical Examples

Two examples: the plain sequential order, and confirming `epoch_seed`
has no effect on it.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  loader::SequentialSampler sampler;
  const auto order = sampler.sample(5, /*epoch_seed=*/0);

  for (auto i : order) std::cout << i << " ";
  std::cout << "\n";

  return 0;
}
```

Output

```text
0 1 2 3 4 
```

### Example 2

Two calls with different `epoch_seed` values produce the identical
order -- unlike every other concrete `Sampler` in this family,
`SequentialSampler` never reads `epoch_seed` at all.

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  loader::SequentialSampler sampler;
  const auto order_a = sampler.sample(5, 1);
  const auto order_b = sampler.sample(5, 999);

  std::cout << "order_a == order_b: " << std::boolalpha << (order_a == order_b) << "\n";

  return 0;
}
```

Output

```text
order_a == order_b: true
```

## Changelog

- **0.1** — the unshuffled `[0, size)` order.

## See Also

- [`Sampler`](Sampler.md) -- the interface this implements.
- [`DataLoaderOptions::shuffle`](DataLoaderOptions.md) -- the plain-bool
  equivalent this `Sampler` matches when set to `false`.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
