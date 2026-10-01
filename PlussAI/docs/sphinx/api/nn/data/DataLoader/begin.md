<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: fe8914671e45 -->

# `pluss::loader::DataLoader::begin`

[Back to `pluss::loader::DataLoader`](../DataLoader.md)

## Signature

```cpp
pluss::loader::DataLoader::iterator pluss::loader::DataLoader::begin();
```

## What It Does

Starts a fresh pass over the loader -- called implicitly by range-for
(`for (auto batch : data_loader)`), rarely by hand. When
`DataLoaderOptions::reshuffle_each_epoch` is `true` (the default), `begin`
calls [`reset`](reset.md) internally first, which reshuffles the order
(when `shuffle` or a `sampler` is set) and advances
[`epoch`](epoch.md) -- so every range-for pass over the same loader draws
a genuinely different order, without the caller needing to call `reset`
themselves. When `num_workers > 0`, `begin` also (re)starts the
background prefetch pool.

## Parameters

None.

## Returns

`DataLoader::iterator` -- an input iterator; dereferencing it calls the
same internal batch-fetch path [`next`](next.md) uses.

## Errors

None from `begin` itself; errors surface when the returned iterator is
dereferenced or incremented, the same as [`next`](next.md)'s own error
cases.

## Practical Examples

Three examples: two range-for passes drawing different shuffled orders
by default, the same two passes producing the identical order with
`reshuffle_each_epoch = false`, and manually calling `begin`/`end` to
drive an equivalent loop without range-for syntax.

### Example 1

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

  for (int pass = 0; pass < 2; ++pass) {
    for (auto batch : data_loader) {
      std::cout << "pass " << pass << ": " << batch.x() << "\n";
    }
  }
  std::cout << "epoch() after two range-for passes = " << data_loader.epoch() << "\n";

  return 0;
}
```

Output

```text
pass 0: pluss::Series(shape=[5,1], strides=[1,1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[[5],
  [4],
  [1],
  [3],
  [2]]
pass 1: pluss::Series(shape=[5,1], strides=[1,1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[[1],
  [2],
  [5],
  [4],
  [3]]
epoch() after two range-for passes = 2
```

### Example 2

`reshuffle_each_epoch = false` -- `begin` no longer calls `reset`
internally, so both passes draw the exact same order.

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
  options.reshuffle_each_epoch = false;
  loader::DataLoader data_loader({x}, options);

  for (int pass = 0; pass < 2; ++pass) {
    for (auto batch : data_loader) {
      std::cout << "pass " << pass << ": " << batch.x() << "\n";
    }
  }

  return 0;
}
```

Output

```text
pass 0: pluss::Series(shape=[5,1], strides=[1,1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[[4],
  [5],
  [1],
  [2],
  [3]]
pass 1: pluss::Series(shape=[5,1], strides=[1,1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[[4],
  [5],
  [1],
  [2],
  [3]]
```

### Example 3

Calling `begin`/`end` directly, without range-for syntax -- exactly what
the range-for loop in Examples 1-2 expands to underneath.

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

  for (auto it = data_loader.begin(); it != data_loader.end(); ++it) {
    std::cout << "manual iterator: " << (*it).x() << "\n";
  }

  return 0;
}
```

Output

```text
manual iterator: pluss::Series(shape=[1,1], strides=[1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[1]]
manual iterator: pluss::Series(shape=[1,1], strides=[1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[2]]
manual iterator: pluss::Series(shape=[1,1], strides=[1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[3]]
```

## See Also

- [`end`](end.md) -- the matching end-of-range sentinel.
- [`reset`](reset.md) -- what `begin` calls internally when
  `reshuffle_each_epoch` is `true`.
- [`next`](next.md) -- the manual, one-batch-at-a-time alternative.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="batch_size.html">
    <span class="label">Previous</span>
    <span class="title">batch_size</span>
  </a>
  <a class="next-link" href="end.html">
    <span class="label">Next</span>
    <span class="title">end</span>
  </a>
</div>
<!-- apiref:nav:end -->
