<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 238789e5305c -->

# `pluss::loader::DataLoader::reset`

[Back to `pluss::loader::DataLoader`](../DataLoader.md)

## Signature

```cpp
pluss::loader::DataLoader& pluss::loader::DataLoader::reset(bool reshuffle = true);
```

## What It Does

Advances to a new epoch: increments [`epoch()`](epoch.md), rewinds the
cursor [`next`](next.md) reads from back to the start, and -- when
`reshuffle` is `true` and `shuffle` or a `sampler` is set -- draws a
fresh order for the new epoch. Called internally by
[`begin`](begin.md) whenever `reshuffle_each_epoch` is `true` (the
default); call it directly only when driving epochs by hand via
[`next`](next.md) rather than range-for.

## Parameters

`bool reshuffle = true`
: Whether to draw a new order for the new epoch (when `shuffle`/`sampler`
  is set). `false` advances the epoch counter and resets the cursor, but
  keeps whatever order was already in place.

## Returns

`DataLoader&` -- a reference to `*this`, for chaining.

## Errors

None from `reset` itself.

## Practical Examples

Three examples: manually resetting between two `next()`-driven epochs,
`reshuffle = false` keeping the prior order, and confirming `epoch()`
advances by exactly one per call.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0}, {3, 1});
  loader::DataLoaderOptions options{};
  options.batch_size = 1;
  loader::DataLoader data_loader({x}, options);

  while (data_loader.next()) {}
  std::cout << "epoch before reset() = " << data_loader.epoch() << "\n";

  data_loader.reset();
  std::cout << "epoch after reset() = " << data_loader.epoch() << "\n";
  while (auto batch = data_loader.next()) {
    std::cout << "batch: " << batch->x() << "\n";
  }

  return 0;
}
```

Output

```text
epoch before reset() = 0
epoch after reset() = 1
batch: pluss::Series(shape=[1,1], strides=[1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[1]]
batch: pluss::Series(shape=[1,1], strides=[1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[2]]
batch: pluss::Series(shape=[1,1], strides=[1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[3]]
```

### Example 2

`reshuffle = false` -- the order from before `reset()` carries over
unchanged into the new epoch.

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
  options.seed = 3;
  loader::DataLoader data_loader({x}, options);

  auto first = data_loader.next();
  std::cout << "epoch 0: " << first->x() << "\n";

  data_loader.reset(/*reshuffle=*/false);
  auto second = data_loader.next();
  std::cout << "epoch 1 (reshuffle=false): " << second->x() << "\n";

  return 0;
}
```

Output

```text
epoch 0: pluss::Series(shape=[5,1], strides=[1,1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[[4],
  [5],
  [1],
  [2],
  [3]]
epoch 1 (reshuffle=false): pluss::Series(shape=[5,1], strides=[1,1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[[4],
  [5],
  [1],
  [2],
  [3]]
```

### Example 3

`epoch()` advances by exactly one per `reset()` call, regardless of how
many batches were actually consumed beforehand.

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0}, {3, 1});
  loader::DataLoader data_loader({x});

  for (int i = 0; i < 3; ++i) {
    data_loader.reset();
    std::cout << "epoch() = " << data_loader.epoch() << "\n";
  }

  return 0;
}
```

Output

```text
epoch() = 1
epoch() = 2
epoch() = 3
```

## See Also

- [`epoch`](epoch.md) -- what `reset` advances.
- [`set_epoch`](set_epoch.md) -- set a specific epoch number directly,
  instead of incrementing by one.
- [`begin`](begin.md) -- calls `reset` internally when
  `reshuffle_each_epoch` is `true`.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="next.html">
    <span class="label">Previous</span>
    <span class="title">next</span>
  </a>
  <a class="next-link" href="sample_count.html">
    <span class="label">Next</span>
    <span class="title">sample_count</span>
  </a>
</div>
<!-- apiref:nav:end -->
