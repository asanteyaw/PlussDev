<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3bf2086ebd5f -->

# `pluss::loader::DataLoader::batch_size`

[Back to `pluss::loader::DataLoader`](../DataLoader.md)

## Signature

```cpp
std::int64_t pluss::loader::DataLoader::batch_size() const noexcept;
```

## What It Does

Returns `options().batch_size` directly -- how many samples each batch
targets (the last batch in an epoch may hold fewer, unless `drop_last`
is set). A plain, `noexcept` convenience so a caller does not need to go
through [`options()`](../DataLoader.md#state) just to read this one
field back.

## Parameters

None.

## Returns

`std::int64_t` -- the configured batch size.

## Errors

None. `batch_size` is `noexcept` and has no preconditions.

## Practical Examples

Three examples: the default batch size, an explicit override at
construction, and a changed value read back after
[`set_options`](set_options.md).

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0}, {3, 1});
  loader::DataLoader data_loader({x});

  std::cout << "batch_size() (default) = " << data_loader.batch_size() << "\n";

  return 0;
}
```

Output

```text
batch_size() (default) = 32
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}, {5, 1});
  loader::DataLoaderOptions options{};
  options.batch_size = 2;
  loader::DataLoader data_loader({x}, options);

  std::cout << "batch_size() = " << data_loader.batch_size() << "\n";

  return 0;
}
```

Output

```text
batch_size() = 2
```

### Example 3

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}, {5, 1});
  loader::DataLoader data_loader({x});

  loader::DataLoaderOptions new_options{};
  new_options.batch_size = 3;
  data_loader.set_options(new_options);

  std::cout << "batch_size() after set_options = " << data_loader.batch_size() << "\n";

  return 0;
}
```

Output

```text
batch_size() after set_options = 3
```

## See Also

- [`batch_count`](batch_count.md) -- how many batches this size produces
  over `sample_count()` samples.
- [`DataLoaderOptions::batch_size`](../DataLoaderOptions.md) -- the
  underlying field, including its default.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="batch_count.html">
    <span class="label">Previous</span>
    <span class="title">batch_count</span>
  </a>
  <a class="next-link" href="begin.html">
    <span class="label">Next</span>
    <span class="title">begin</span>
  </a>
</div>
<!-- apiref:nav:end -->
