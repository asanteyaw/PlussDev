# `pluss::ai::ice::TensorSource`

A [`Source`](Source.md) over raw train/val, infer-only, or test-only
`Series` tensor pairs -- the implementation every
[`Coach::learn`/`infer`/`assess`](../coach/Coach.md) overload that
takes `Series` directly builds internally (see each method's own page
for the exact construction). `final`, not intended as a further base
class; reach for a hand-written [`Source`](Source.md) subclass instead
when the data does not fit a flat `Series` pair.

## Signature

```cpp
pluss::ai::ice::TensorSource::TensorSource(
    Series x_train, Series y_train,
    std::optional<Series> x_valid, std::optional<Series> y_valid,
    pluss::ai::ice::BatchPlan learn_plan);
explicit pluss::ai::ice::TensorSource::TensorSource(
    Series x_infer, pluss::ai::ice::BatchPlan infer_plan = {});
pluss::ai::ice::TensorSource::TensorSource(Series x_test, Series y_test);
```

Three constructors, each populating exactly one shape of batches --
see `## State` below for which.

## Construction

```cpp
ice::TensorSource training(x_train, y_train, x_val, y_val, ice::BatchPlan{.batch_size = 32});
ice::TensorSource training_no_valid(x_train, y_train, std::nullopt, std::nullopt, ice::BatchPlan{});
ice::TensorSource inference(x_infer, ice::BatchPlan{.batch_size = 64});
ice::TensorSource testing(x_test, y_test);
```

`x_valid`/`y_valid` must both be present or both absent (constructing
with only one set throws once [`prepare`](Source/prepare.md) runs).
`learn_plan` governs [`learn_batches`](Source/learn_batches.md) (and
`infer_plan` governs [`infer_batches`](Source/infer_batches.md)) --
[`valid_batches`](Source/valid_batches.md)/
[`test_batches`](Source/test_batches.md) always use a single,
unshuffled batch regardless of either plan.

## State

Which `*_batches()` accessor ends up populated depends entirely on
which constructor was used:

| Constructor | `learn_batches` | `valid_batches` | `test_batches` | `infer_batches` |
| --- | --- | --- | --- | --- |
| Training (`x_train`, `y_train`, ...) | yes | if `x_valid` given | no | no |
| Infer-only (`x_infer`, ...) | no | no | no | yes |
| Test-only (`x_test`, `y_test`) | no | no | yes | no |

## Methods

Every [`Source`](Source.md) method is overridden here with
`TensorSource`'s own batching logic described above -- not
re-documented on this page; see
[`Source`'s own method pages](Source.md) for what each one means
generically. `do_transfer_batch` reads the `"features"` field
[`TensorSource`](TensorSource.md) materializes internally and sets it
as `inputs`, plus `target` as `targets` when present -- the same
conversion [`transfer_batch`'s own page](Source/transfer_batch.md)
documents against a hand-written `Source`.

## Practical Examples

Three examples, one per constructor.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {3, 2});
  const pluss::Series y = pluss::make_series({0.0f, 1.0f, 2.0f}, {3, 1});

  ice::TensorSource source(x, y, std::nullopt, std::nullopt, ice::BatchPlan{.batch_size = 2});
  source.prepare();

  std::cout << "learn_batches().size() = " << source.learn_batches().size() << "\n";
  std::cout << "learn_batches()[0].size() = " << source.learn_batches()[0].size() << "\n";
  std::cout << "learn_batches()[1].size() = " << source.learn_batches()[1].size() << "\n";

  return 0;
}
```

Output

```text
learn_batches().size() = 2
learn_batches()[0].size() = 2
learn_batches()[1].size() = 1
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {3, 2});

  ice::TensorSource source(x, ice::BatchPlan{.batch_size = 2});
  source.prepare();
  std::cout << "infer_batches().size() = " << source.infer_batches().size() << "\n";
  std::cout << "infer_batches()[0].size() = " << source.infer_batches()[0].size() << "\n";
  std::cout << "infer_batches()[1].size() = " << source.infer_batches()[1].size() << "\n";

  return 0;
}
```

Output

```text
infer_batches().size() = 2
infer_batches()[0].size() = 2
infer_batches()[1].size() = 1
```

### Example 3

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  const pluss::Series x_test = pluss::make_series({0.0f, 0.0f, 1.0f, 0.0f}, {2, 2});
  const pluss::Series y_test = pluss::make_series({0.0f, 1.0f}, {2, 1});

  ice::TensorSource source(x_test, y_test);
  source.prepare();
  std::cout << "test_batches().size() = " << source.test_batches().size() << "\n";
  std::cout << "learn_batches().size() = " << source.learn_batches().size() << "\n";
  std::cout << "infer_batches().size() = " << source.infer_batches().size() << "\n";

  return 0;
}
```

Output

```text
test_batches().size() = 1
learn_batches().size() = 0
infer_batches().size() = 0
```

## Changelog

- **0.1** -- `TensorSource`: three constructors over raw `Series`
  pairs, one per batch-stage shape.

## See Also

- [`Source`](Source.md) -- the base class, with the full method
  surface.
- [`DataSetSource`](DataSetSource.md) -- the panel/windowed-data
  counterpart.
- [`Coach::learn`](../coach/Coach/learn.md) -- builds this internally
  for every `Series`/`DataFrame` overload.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
