# `pluss::loader::DataLoaderSplit`

A plain result struct: the train/val/test [`DataLoader`](DataLoader.md)
trio produced by one [`split_loaders`](split_loaders.md) call -- exactly
[`split_fields`](split_fields.md)'s own three parts, each already
wrapped in its own `DataLoader`. Never constructed directly by calling
code -- every instance comes back from `split_loaders`.

## Signature

```cpp
pluss::loader::DataLoaderSplit::DataLoaderSplit() = default;
```

A plain aggregate of three default-constructed, empty `DataLoader`s --
never meaningful on its own; every real instance comes from
[`split_loaders`](split_loaders.md). Like `DataLoader` itself,
`DataLoaderSplit` is move-only (copy is implicitly deleted, since
`DataLoader` is).

## State

### `train`

`DataLoader train`
: A `DataLoader` over `DataSplit::train_fields`, constructed with
  whichever `train_options` was passed to `split_loaders`.

### `val`

`DataLoader val`
: A `DataLoader` over `DataSplit::val_fields`, constructed with
  whichever `val_options` was passed to `split_loaders`. Empty (zero
  samples) when `DataSplitOptions::val_ratio` was `0.0`.

### `test`

`DataLoader test`
: A `DataLoader` over `DataSplit::test_fields`, constructed with
  whichever `test_options` was passed to `split_loaders`.

## Methods

None -- `DataLoaderSplit` is a plain data result; every field is public
and read directly, see `## State` above.

## Practical Examples

One example: a split with a small `batch_size` on the training loader,
iterated to show each part's own batching.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series x = pluss::make_series(
      {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0}, {10, 1});

  loader::DataSplitOptions split_options{};
  split_options.train_ratio = 0.6;
  split_options.val_ratio = 0.2;
  split_options.shuffle = false;

  loader::DataLoaderOptions train_options{};
  train_options.batch_size = 2;

  loader::DataLoaderSplit loaders = loader::split_loaders({x}, split_options, train_options);

  std::cout << "train.sample_count() = " << loaders.train.sample_count() << "\n";
  std::cout << "train.batch_count() = " << loaders.train.batch_count() << "\n";
  std::cout << "val.sample_count() = " << loaders.val.sample_count() << "\n";
  std::cout << "test.sample_count() = " << loaders.test.sample_count() << "\n";

  return 0;
}
```

Output

```text
train.sample_count() = 6
train.batch_count() = 3
val.sample_count() = 2
test.sample_count() = 2
```

## Changelog

- **0.1** — the three-`DataLoader` train/val/test result of
  `split_loaders`.

## See Also

- [`split_loaders`](split_loaders.md) -- produces this result.
- [`DataSplit`](DataSplit.md) -- the underlying field-level split, before
  each part is wrapped in a `DataLoader`.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
