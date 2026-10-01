<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 037acff3d09b -->

# `pluss::loader::DataLoader::sample_count`

[Back to `pluss::loader::DataLoader`](../DataLoader.md)

## Signature

```cpp
std::int64_t pluss::loader::DataLoader::sample_count() const noexcept;
```

## What It Does

Returns the total sample count -- how many individual samples this
loader batches over, independent of `batch_size`. Fixed at construction
(or [`set_options`](set_options.md)); never changes as a result of
iterating, [`reset`](reset.md), or [`set_epoch`](set_epoch.md).

## Parameters

None.

## Returns

`std::int64_t` -- the total sample count.

## Errors

None. `sample_count` is `noexcept` and has no preconditions.

## Practical Examples

Three examples: the sample count from raw fields, from a `Dataset`, and
confirming it stays fixed across a `reset()` call.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}, {5, 1});
  loader::DataLoader data_loader({x});

  std::cout << "sample_count() = " << data_loader.sample_count() << "\n";

  return 0;
}
```

Output

```text
sample_count() = 5
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}, {5, 1});
  loader::SeriesDataset dataset({x});
  loader::DataLoader data_loader(dataset);

  std::cout << "sample_count() = " << data_loader.sample_count() << "\n";

  return 0;
}
```

Output

```text
sample_count() = 5
```

### Example 3

`sample_count()` stays fixed across `reset()` -- only `batch_size` (via
[`set_options`](set_options.md)) changes how many *batches* that same
sample count produces.

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0}, {3, 1});
  loader::DataLoader data_loader({x});

  std::cout << "sample_count() before reset = " << data_loader.sample_count() << "\n";
  data_loader.reset();
  std::cout << "sample_count() after reset = " << data_loader.sample_count() << "\n";

  return 0;
}
```

Output

```text
sample_count() before reset = 3
sample_count() after reset = 3
```

## See Also

- [`batch_count`](batch_count.md) / [`batch_size`](batch_size.md) -- how
  `sample_count` is divided into batches.
- [`Dataset::size`](../Dataset/size.md) -- the same count, from the
  dataset's own side.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="reset.html">
    <span class="label">Previous</span>
    <span class="title">reset</span>
  </a>
  <a class="next-link" href="set_epoch.html">
    <span class="label">Next</span>
    <span class="title">set_epoch</span>
  </a>
</div>
<!-- apiref:nav:end -->
