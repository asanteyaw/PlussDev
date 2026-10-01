<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 59b23a0878fe -->

# `pluss::loader::DataLoader::batch_at`

[Back to `pluss::loader::DataLoader`](../DataLoader.md)

## Signature

```cpp
pluss::loader::DataBatch pluss::loader::DataLoader::batch_at(std::size_t batch_index) const;
```

## What It Does

Computes and returns one specific batch by position, synchronously on
the calling thread -- always, regardless of `num_workers`; `batch_at`
never goes through the threaded prefetch path
[`next`](next.md)/range-for iteration uses. Useful for random access into
a specific batch (evaluation code that always wants "the first batch", or
a debugging session inspecting one batch in isolation) without walking
through every prior batch first.

## Parameters

`std::size_t batch_index`
: The batch position, in `[0, batch_count())`.

## Returns

`DataBatch` -- the batch at that position, computed fresh on every call
(not cached).

## Errors

- `batch_index >= batch_count()` throws.

## Practical Examples

Three examples: fetching a specific batch directly, an out-of-range
index, and confirming `batch_at` is synchronous even when the loader is
otherwise configured with worker threads.

### Example 1

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

  std::cout << "batch_at(1): " << data_loader.batch_at(1).x() << "\n";

  return 0;
}
```

Output

```text
batch_at(1): pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[3],
  [4]]
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>
#include <string>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0}, {3, 1});
  loader::DataLoaderOptions options{};
  options.batch_size = 2;
  loader::DataLoader data_loader({x}, options);

  std::cout << "batch_count() = " << data_loader.batch_count() << "\n";
  try {
    data_loader.batch_at(5);
  } catch (const std::exception& e) {
    const std::string message(e.what());
    std::cout << "caught: " << message.substr(0, message.find('\n')) << "\n";
  }

  return 0;
}
```

Output

```text
batch_count() = 2
caught: pluss::loader::DataLoader::batch_at: batch index out of range
```

### Example 3

`batch_at(0)` called twice in a row on a `num_workers > 0` loader is
still synchronous and independent -- neither call touches the background
prefetch queue `next()`/range-for iteration would use.

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4, 1});
  loader::DataLoaderOptions options{};
  options.batch_size = 2;
  options.num_workers = 2;
  loader::DataLoader data_loader({x}, options);

  std::cout << "batch_at(0) call 1: " << data_loader.batch_at(0).x() << "\n";
  std::cout << "batch_at(0) call 2: " << data_loader.batch_at(0).x() << "\n";

  return 0;
}
```

Output

```text
batch_at(0) call 1: pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[1],
  [2]]
batch_at(0) call 2: pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[1],
  [2]]
```

## See Also

- [`batch_count`](batch_count.md) -- the valid range for `batch_index`.
- [`next`](next.md) / [`begin`](begin.md) -- the sequential, threaded-prefetch-aware alternative.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="batch_count.html">
    <span class="label">Next</span>
    <span class="title">batch_count</span>
  </a>
</div>
<!-- apiref:nav:end -->
