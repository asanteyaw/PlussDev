<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 890f846a9487 -->

# `pluss::loader::DataLoader::epoch`

[Back to `pluss::loader::DataLoader`](../DataLoader.md)

## Signature

```cpp
std::uint64_t pluss::loader::DataLoader::epoch() const noexcept;
```

## What It Does

Returns the current epoch number -- `0` for a freshly constructed
loader, incremented by [`reset`](reset.md) (and, when
`reshuffle_each_epoch` is on, by every [`begin`](begin.md) call too), or
set directly via [`set_epoch`](set_epoch.md). Feeds into the order a
[`Sampler`](../Sampler.md)/`shuffle` draws: `options.seed + epoch` is the
actual seed each epoch's shuffle uses, so the same epoch number always
reproduces the same order for a given `seed`.

## Parameters

None.

## Returns

`std::uint64_t` -- the current epoch number.

## Errors

None. `epoch` is `noexcept` and has no preconditions.

## Practical Examples

Three examples: the starting value on a fresh loader, incrementing via
`reset()`, and reading it back after `set_epoch`.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0}, {3, 1});
  loader::DataLoader data_loader({x});

  std::cout << "epoch() on a fresh loader = " << data_loader.epoch() << "\n";

  return 0;
}
```

Output

```text
epoch() on a fresh loader = 0
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0}, {3, 1});
  loader::DataLoader data_loader({x});

  data_loader.reset();
  data_loader.reset();
  std::cout << "epoch() after two reset() calls = " << data_loader.epoch() << "\n";

  return 0;
}
```

Output

```text
epoch() after two reset() calls = 2
```

### Example 3

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0}, {3, 1});
  loader::DataLoader data_loader({x});

  data_loader.set_epoch(42);
  std::cout << "epoch() after set_epoch(42) = " << data_loader.epoch() << "\n";

  return 0;
}
```

Output

```text
epoch() after set_epoch(42) = 42
```

## See Also

- [`reset`](reset.md) -- increments `epoch` by one.
- [`set_epoch`](set_epoch.md) -- sets `epoch` to a specific value
  directly.
- [`DataLoaderOptions::seed`](../DataLoaderOptions.md) -- combined with
  `epoch` to seed each epoch's shuffle.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="end.html">
    <span class="label">Previous</span>
    <span class="title">end</span>
  </a>
  <a class="next-link" href="next.html">
    <span class="label">Next</span>
    <span class="title">next</span>
  </a>
</div>
<!-- apiref:nav:end -->
