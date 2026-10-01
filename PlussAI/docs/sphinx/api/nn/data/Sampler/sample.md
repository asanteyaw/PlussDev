<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b51f3a0336e0 -->

# `pluss::loader::Sampler::sample`

[Back to `pluss::loader::Sampler`](../Sampler.md)

## Signature

```cpp
virtual std::vector<std::int64_t> pluss::loader::Sampler::sample(std::int64_t size, std::uint64_t epoch_seed) const = 0;
```

## What It Does

Returns the full index order for one epoch over a dataset of `size`
samples. Pure virtual -- every concrete `Sampler` implements it.
`DataLoader` calls this once per epoch (when `sampler` is set), using
`options.seed + epoch` as `epoch_seed` -- the same reused `Sampler`
instance produces a different (but reproducible, for the same seed and
epoch) order every epoch, without needing to be reconstructed.

## Parameters

`std::int64_t size`
: The dataset's sample count.

`std::uint64_t epoch_seed`
: The seed for this specific epoch's order -- already offset by the
  caller (`DataLoader` passes `options.seed + epoch`), so calling
  `sample()` directly with a fixed `epoch_seed` reproduces exactly what a
  given epoch would draw.

## Returns

`std::vector<std::int64_t>` -- one entry per position in the epoch,
each a valid index into `[0, size)`. May be shorter or longer than
`size` (a `RandomSampler` with an explicit `num_samples`, or a
`SubsetRandomSampler` over a fixed subset).

## Errors

None from the base contract; a concrete subclass may throw for its own
preconditions (e.g. `RandomSampler` without `replacement` requires
`num_samples <= size`).

## Practical Examples

Three examples: the same call across two different concrete samplers to
show the interface is uniform, `sample()` called twice with the same
`epoch_seed` producing the identical order, and different `epoch_seed`
values producing different orders.

### Example 1

`SequentialSampler` and `RandomSampler` both satisfy the same `sample`
signature -- calling code that only knows it holds a `Sampler` reference
does not need to know which.

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

void print_order(const pluss::loader::Sampler& sampler, std::int64_t size, std::uint64_t epoch_seed) {
  const auto order = sampler.sample(size, epoch_seed);
  for (auto i : order) std::cout << i << " ";
  std::cout << "\n";
}

int main() {
  namespace loader = pluss::loader;

  loader::SequentialSampler seq;
  loader::RandomSampler rand;

  std::cout << "SequentialSampler: ";
  print_order(seq, 5, 0);
  std::cout << "RandomSampler: ";
  print_order(rand, 5, 42);

  return 0;
}
```

Output

```text
SequentialSampler: 0 1 2 3 4 
RandomSampler: 0 3 4 2 1 
```

### Example 2

The same `epoch_seed` reproduces the identical order -- `sample()` is a
pure function of its two arguments, not internal mutable state.

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  loader::RandomSampler rand;
  const auto order1 = rand.sample(5, 42);
  const auto order2 = rand.sample(5, 42);

  std::cout << "order1 == order2: " << std::boolalpha << (order1 == order2) << "\n";

  return 0;
}
```

Output

```text
order1 == order2: true
```

### Example 3

Different `epoch_seed` values (as `DataLoader` supplies across
successive epochs, via `options.seed + epoch`) produce different orders.

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  loader::RandomSampler rand;
  const auto epoch0 = rand.sample(5, /*epoch_seed=*/100);
  const auto epoch1 = rand.sample(5, /*epoch_seed=*/101);

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
epoch 0: 3 0 1 2 4 
epoch 1: 4 1 3 2 0 
```

## See Also

- [`DataLoaderOptions::sampler`](../DataLoaderOptions.md) -- where a
  `Sampler` gets wired into `DataLoader`, and how `epoch_seed` is derived.
- [`SequentialSampler`](../SequentialSampler.md) /
  [`RandomSampler`](../RandomSampler.md) /
  [`SubsetRandomSampler`](../SubsetRandomSampler.md) /
  [`WeightedRandomSampler`](../WeightedRandomSampler.md) -- the four
  concrete implementations.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
</div>
<!-- apiref:nav:end -->
