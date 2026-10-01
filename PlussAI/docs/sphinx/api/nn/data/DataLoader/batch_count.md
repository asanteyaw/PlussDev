<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 13369d1ce701 -->

# `pluss::loader::DataLoader::batch_count`

[Back to `pluss::loader::DataLoader`](../DataLoader.md)

## Signature

```cpp
std::size_t pluss::loader::DataLoader::batch_count() const noexcept;
```

## What It Does

Returns how many batches one epoch produces -- `ceil(sample_count() /
batch_size())`, or the floor of that division when `drop_last` is set
(discarding a final partial batch instead of keeping it short). The
valid range for [`batch_at`](batch_at.md)'s own index argument, and what
[`end`](end.md)'s iterator is positioned at.

## Parameters

None.

## Returns

`std::size_t` -- the batch count for the current order (reflects the
dataset's `sample_count()` unless a [`Sampler`](../Sampler.md) with a
non-default `num_samples` is in effect, in which case it reflects the
sampler's own order length instead).

## Errors

None. `batch_count` is `noexcept` and has no preconditions.

## Practical Examples

Three examples: an evenly-divisible sample count, a remainder producing
one extra partial batch, and `drop_last` discarding that partial batch
instead.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4, 1});
  loader::DataLoaderOptions options{};
  options.batch_size = 2;
  loader::DataLoader data_loader({x}, options);

  std::cout << "batch_count() = " << data_loader.batch_count() << "\n";

  return 0;
}
```

Output

```text
batch_count() = 2
```

### Example 2

Five samples at `batch_size = 2` -- two full batches plus one partial
batch of the remaining sample.

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

  std::cout << "batch_count() = " << data_loader.batch_count() << "\n";

  return 0;
}
```

Output

```text
batch_count() = 3
```

### Example 3

The same five samples with `drop_last = true` -- the partial batch is
discarded, leaving only the two full ones.

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}, {5, 1});
  loader::DataLoaderOptions options{};
  options.batch_size = 2;
  options.drop_last = true;
  loader::DataLoader data_loader({x}, options);

  std::cout << "batch_count() = " << data_loader.batch_count() << "\n";
  for (auto batch : data_loader) {
    std::cout << "batch: " << batch.x() << "\n";
  }

  return 0;
}
```

Output

```text
batch_count() = 2
batch: pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[1],
  [2]]
batch: pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[3],
  [4]]
```

## See Also

- [`batch_at`](batch_at.md) -- valid indices are `[0, batch_count())`.
- [`sample_count`](sample_count.md) / [`batch_size`](batch_size.md) -- what
  `batch_count` is derived from.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="batch_at.html">
    <span class="label">Previous</span>
    <span class="title">batch_at</span>
  </a>
  <a class="next-link" href="batch_size.html">
    <span class="label">Next</span>
    <span class="title">batch_size</span>
  </a>
</div>
<!-- apiref:nav:end -->
