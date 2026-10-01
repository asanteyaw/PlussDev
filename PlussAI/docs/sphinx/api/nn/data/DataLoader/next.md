<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 39087a04668f -->

# `pluss::loader::DataLoader::next`

[Back to `pluss::loader::DataLoader`](../DataLoader.md)

## Signature

```cpp
std::optional<pluss::loader::DataBatch> pluss::loader::DataLoader::next();
```

## What It Does

Returns the next batch in the current epoch, advancing an internal
cursor, or `std::nullopt` once every batch in the epoch has been
returned. The manual, pull-one-batch-at-a-time alternative to range-for
iteration ([`begin`](begin.md)/[`end`](end.md)) -- useful when a loop
needs to interleave batch consumption with control flow a plain
range-for does not fit (an early-exit condition checked mid-epoch,
multiple loaders driven in lockstep, ...). When `num_workers > 0`,
`next()` reaching the end of the epoch also stops background worker
threads unless `persistent_workers` is set.

## Parameters

None.

## Returns

`std::optional<DataBatch>` -- the next batch, or `std::nullopt` when the
epoch is exhausted. Does not automatically start a new epoch; call
[`reset`](reset.md) (or [`set_epoch`](set_epoch.md)) explicitly to begin
another one.

## Errors

- Propagates any exception a worker thread encountered while computing a
  batch (when `num_workers > 0`), on the call that would have returned
  that batch.
- Propagates whatever `collate_fn` (if set) throws.

## Practical Examples

Three examples: pulling every batch in a `while` loop, confirming
`next()` returns `std::nullopt` once exhausted, and interleaving `next()`
with a real training step.

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

  while (auto batch = data_loader.next()) {
    std::cout << "next(): " << batch->x() << "\n";
  }

  return 0;
}
```

Output

```text
next(): pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[1],
  [2]]
next(): pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[3],
  [4]]
next(): pluss::Series(shape=[1,1], strides=[1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[5]]
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series x = pluss::make_series({1.0, 2.0}, {2, 1});
  loader::DataLoaderOptions options{};
  options.batch_size = 2;
  loader::DataLoader data_loader({x}, options);

  std::cout << "first next(): has_value() = " << std::boolalpha << data_loader.next().has_value() << "\n";
  std::cout << "second next(): has_value() = " << data_loader.next().has_value() << "\n";

  return 0;
}
```

Output

```text
first next(): has_value() = true
second next(): has_value() = false
```

### Example 3

A real training loop, `next()` interleaved directly with an optimizer
step.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;
  namespace loader = pluss::loader;
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;

  const Config f32 = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  const Series x = pluss::make_series({1.0F, 2.0F, 3.0F, 4.0F}, {4, 1}, f32);
  const Series y = pluss::make_series({2.0F, 4.0F, 6.0F, 8.0F}, {4, 1}, f32);

  loader::DataLoaderOptions options{};
  options.batch_size = 2;
  loader::DataLoader data_loader({x, y}, options);

  nn::Fusion model(1, 1, /*bias=*/false, f32);
  nn::optim::SGD optimizer(*model, nn::optim::SGDOptions{.learning_rate = 0.05});

  while (auto batch = data_loader.next()) {
    const Series loss = optimizer.step([&]() -> Series {
      const Series prediction = model.forward(batch->x());
      return nn::losses::MSE(prediction, batch->y());
    });
    std::cout << "batch loss = " << loss.item<float>() << "\n";
  }

  return 0;
}
```

Output

```text
batch loss = 9.29151
batch loss = 26.1324
```

## See Also

- [`begin`](begin.md) / [`end`](end.md) -- the range-for alternative.
- [`reset`](reset.md) / [`set_epoch`](set_epoch.md) -- start a new epoch
  after `next()` returns `std::nullopt`.
- [`DataBatch`](../DataBatch.md) -- the type wrapped in the returned
  `optional`.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="epoch.html">
    <span class="label">Previous</span>
    <span class="title">epoch</span>
  </a>
  <a class="next-link" href="reset.html">
    <span class="label">Next</span>
    <span class="title">reset</span>
  </a>
</div>
<!-- apiref:nav:end -->
