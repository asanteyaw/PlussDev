<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 808fa45e96d1 -->

# `pluss::loader::DataLoader::set_epoch`

[Back to `pluss::loader::DataLoader`](../DataLoader.md)

## Signature

```cpp
pluss::loader::DataLoader& pluss::loader::DataLoader::set_epoch(std::uint64_t epoch, bool reshuffle = true);
```

## What It Does

Sets [`epoch()`](epoch.md) to a specific value directly, rather than
incrementing it by one the way [`reset`](reset.md) does -- rewinding the
cursor and, when `reshuffle` is `true` and `shuffle`/`sampler` is set,
drawing that exact epoch's order. Reach for this over `reset` when
resuming training from a checkpoint at a known epoch number, so the
resumed run's shuffled order matches what that epoch would have produced
from a fresh start (since the order is a deterministic function of
`options.seed + epoch`).

## Parameters

`std::uint64_t epoch`
: The epoch number to set. Not required to be `current_epoch + 1` -- any
  value is accepted, including going backward.

`bool reshuffle = true`
: Whether to draw the order for that specific epoch. `false` rewinds the
  cursor and sets the epoch number, but leaves whatever order was
  already in place.

## Returns

`DataLoader&` -- a reference to `*this`, for chaining.

## Errors

None from `set_epoch` itself.

## Practical Examples

Three examples: setting a specific epoch directly, confirming the same
epoch number always reproduces the identical order, and resuming
"training" at a known epoch after simulating several already-completed
ones.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0}, {3, 1});
  loader::DataLoader data_loader({x});

  data_loader.set_epoch(7);
  std::cout << "epoch() after set_epoch(7) = " << data_loader.epoch() << "\n";

  return 0;
}
```

Output

```text
epoch() after set_epoch(7) = 7
```

### Example 2

The same epoch number, set twice (with an unrelated epoch in between),
draws the identical order both times -- the order is a pure function of
`options.seed + epoch`, not of how the loader got there.

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}, {5, 1});
  loader::DataLoaderOptions options{};
  options.batch_size = 5;
  options.shuffle = true;
  options.seed = 10;
  loader::DataLoader data_loader({x}, options);

  data_loader.set_epoch(3);
  const auto first = data_loader.next();
  std::cout << "epoch 3, first time: " << first->x() << "\n";

  data_loader.set_epoch(9);
  data_loader.set_epoch(3);
  const auto second = data_loader.next();
  std::cout << "epoch 3, second time: " << second->x() << "\n";

  return 0;
}
```

Output

```text
epoch 3, first time: pluss::Series(shape=[5,1], strides=[1,1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[[4],
  [5],
  [2],
  [1],
  [3]]
epoch 3, second time: pluss::Series(shape=[5,1], strides=[1,1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[[4],
  [5],
  [2],
  [1],
  [3]]
```

### Example 3

Resuming at a known epoch after several already-completed ones --
`set_epoch` jumps directly there without needing to replay every
intervening epoch.

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0}, {3, 1});
  loader::DataLoaderOptions options{};
  options.shuffle = true;
  loader::DataLoader trained_loader({x}, options);

  // Simulate training through epochs 0-4.
  for (std::uint64_t e = 0; e < 5; ++e) {
    trained_loader.set_epoch(e);
  }
  std::cout << "after simulated training, epoch() = " << trained_loader.epoch() << "\n";

  // A freshly constructed loader, resumed directly at epoch 5.
  loader::DataLoader resumed_loader({x}, options);
  resumed_loader.set_epoch(5);
  std::cout << "resumed_loader.epoch() = " << resumed_loader.epoch() << "\n";

  return 0;
}
```

Output

```text
after simulated training, epoch() = 4
resumed_loader.epoch() = 5
```

## See Also

- [`epoch`](epoch.md) -- the value `set_epoch` sets.
- [`reset`](reset.md) -- the increment-by-one alternative.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sample_count.html">
    <span class="label">Previous</span>
    <span class="title">sample_count</span>
  </a>
  <a class="next-link" href="set_options.html">
    <span class="label">Next</span>
    <span class="title">set_options</span>
  </a>
</div>
<!-- apiref:nav:end -->
