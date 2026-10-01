# `pluss::loader::DataSplit`

A plain result struct: the train/val/test field `Series` produced by one
[`split_fields`](split_fields.md) call. Never constructed directly by
calling code -- every instance comes back from `split_fields`.

## Signature

```cpp
pluss::loader::DataSplit::DataSplit() = default;
```

A plain aggregate -- every field below defaults to an empty vector, but a
default-constructed instance is never meaningful on its own; every real
instance comes from [`split_fields`](split_fields.md).

## State

### `train_fields`

`std::vector<Series> train_fields`
: The training-part field `Series`, one per input field, in the same
  order as the fields passed to `split_fields`.

### `val_fields`

`std::vector<Series> val_fields`
: The validation-part field `Series`, same order and count as
  `train_fields`. Can be empty when `DataSplitOptions::val_ratio` is
  `0.0`.

### `test_fields`

`std::vector<Series> test_fields`
: Whatever remains after `train_fields`/`val_fields` are taken --
  `sample_count - train_count - val_count` samples' worth, same order
  and count as `train_fields`.

## Methods

None -- `DataSplit` is a plain data result; every field is public and
read directly, see `## State` above.

## Practical Examples

One example: a single-field split read back field by field.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series x = pluss::make_series(
      {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0}, {10, 1});

  loader::DataSplitOptions options{};
  options.train_ratio = 0.6;
  options.val_ratio = 0.2;
  options.shuffle = false;
  loader::DataSplit split = loader::split_fields({x}, options);

  std::cout << "train_fields[0]:\n" << split.train_fields[0] << "\n";
  std::cout << "val_fields[0]:\n" << split.val_fields[0] << "\n";
  std::cout << "test_fields[0]:\n" << split.test_fields[0] << "\n";

  return 0;
}
```

Output

```text
train_fields[0]:
pluss::Series(shape=[6,1], strides=[1,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[0],
  [1],
  [2],
  [3],
  [4],
  [5]]
val_fields[0]:
pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[6],
  [7]]
test_fields[0]:
pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[8],
  [9]]
```

## Changelog

- **0.1** — the three-field train/val/test result of `split_fields`.

## See Also

- [`split_fields`](split_fields.md) -- produces this result.
- [`DataSplitOptions`](DataSplitOptions.md) -- controls the ratios/
  shuffle/seed this split is computed with.
- [`split_loaders`](split_loaders.md) -- `split_fields` plus a
  [`DataLoader`](DataLoader.md) per part.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
