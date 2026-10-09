# `pluss::ai::ice::DataSetSource`

A [`Source`](Source.md) over [`data::DataSet`](../data/DataSet.md)
train/valid/test/infer parts, with separate
[`BatchPlan`](BatchPlan.md)s for the learn loop and evaluation. `final`,
not intended as a further base class. What
[`data::DataSet::source()`](../data/DataSet/source.md)/
[`data::TimeSeriesDataSet::source()`](../data/TimeSeriesDataSet/source.md)
construct internally; construct one directly for explicit control over
which `DataSet` plays which role, or distinct `BatchPlan`s for training
versus evaluation.

## Signature

```cpp
pluss::ai::ice::DataSetSource::DataSetSource(
    pluss::ai::ice::data::DataSet train,
    std::optional<pluss::ai::ice::data::DataSet> valid = std::nullopt,
    std::optional<pluss::ai::ice::data::DataSet> test = std::nullopt,
    std::optional<pluss::ai::ice::data::DataSet> infer = std::nullopt,
    pluss::ai::ice::BatchPlan learn_plan = {},
    pluss::ai::ice::BatchPlan eval_plan = {});
```

## Construction

```cpp
ice::DataSetSource learn_only(train);                                        // valid/test/infer absent
ice::DataSetSource with_valid(train, valid_set);                             // only valid given
ice::DataSetSource full(train, valid_set, test_set, infer_set, learn_plan, eval_plan);
```

`train` is the only required argument -- its own
[`learn_batches`](Source/learn_batches.md) is always populated.
`valid`/`test`/`infer` are each independently optional; `learn_plan`
governs training batches, and `eval_plan` governs validation/test/
infer batches alike (falling back to one unshuffled batch for
validation/test specifically when `eval_plan.batch_size == 0`, the
default).

## State

Same shape as [`TensorSource`](TensorSource.md)'s own table, but keyed
on which optional `DataSet` was given rather than which constructor was
called -- `train` always populates
[`learn_batches`](Source/learn_batches.md);
[`valid_batches`](Source/valid_batches.md)/
[`test_batches`](Source/test_batches.md)/
[`infer_batches`](Source/infer_batches.md) are populated only when
their respective `DataSet` argument is present.

## Methods

Every [`Source`](Source.md) method is overridden here -- not
re-documented on this page; see
[`Source`'s own method pages](Source.md) for what each one means
generically. `do_transfer_batch` copies every field from the host
[`data::Batch`](../data/Batch.md) across unchanged (casting dtype/
platform per [`SourceContext`](SourceContext.md)), additionally
setting `inputs` from the `"features"` field when present and
reshaping a 1-D cross-section target to `[rows, 1]` -- distinct from
[`TensorSource`](TensorSource.md)'s own conversion, which only ever
reads a fixed `"features"`/target pair.

## Practical Examples

Three examples: the training-only shape, adding a validation
`DataSet`, and a separate test `DataSet`.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  ice::data::Batch raw(ice::data::Layout::CrossSection);
  raw.set("features", pluss::make_series({0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {3, 2}));
  raw.set_target(pluss::make_series({0.0f, 1.0f, 2.0f}, {3, 1}));
  const ice::data::DataSet train = ice::data::DataSet::from_batch(raw);

  ice::DataSetSource source(train);
  source.prepare();
  std::cout << "learn_batches().size() = " << source.learn_batches().size() << "\n";
  std::cout << "valid_batches().size() = " << source.valid_batches().size() << "\n";
  std::cout << "test_batches().size() = " << source.test_batches().size() << "\n";
  std::cout << "infer_batches().size() = " << source.infer_batches().size() << "\n";

  return 0;
}
```

Output

```text
learn_batches().size() = 1
valid_batches().size() = 0
test_batches().size() = 0
infer_batches().size() = 0
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  ice::data::Batch raw(ice::data::Layout::CrossSection);
  raw.set("features", pluss::make_series({0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {3, 2}));
  raw.set_target(pluss::make_series({0.0f, 1.0f, 2.0f}, {3, 1}));
  const ice::data::DataSet train = ice::data::DataSet::from_batch(raw);
  const ice::data::DataSet valid = ice::data::DataSet::from_batch(raw);

  ice::DataSetSource source(train, valid);
  source.prepare();
  std::cout << "valid_batches().size() = " << source.valid_batches().size() << "\n";

  return 0;
}
```

Output

```text
valid_batches().size() = 1
```

### Example 3

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  ice::data::Batch raw(ice::data::Layout::CrossSection);
  raw.set("features", pluss::make_series({0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {3, 2}));
  raw.set_target(pluss::make_series({0.0f, 1.0f, 2.0f}, {3, 1}));
  const ice::data::DataSet train = ice::data::DataSet::from_batch(raw);
  const ice::data::DataSet test = ice::data::DataSet::from_batch(raw);

  ice::DataSetSource source(train, std::nullopt, test);
  source.prepare();
  std::cout << "test_batches().size() = " << source.test_batches().size() << "\n";

  return 0;
}
```

Output

```text
test_batches().size() = 1
```

## Changelog

- **0.1** -- `DataSetSource`: four independently-optional `DataSet`
  roles, with separate learn/eval `BatchPlan`s.

## See Also

- [`Source`](Source.md) -- the base class, with the full method
  surface.
- [`TensorSource`](TensorSource.md) -- the raw-`Series` counterpart.
- [`data::DataSet::source`](../data/DataSet/source.md) -- builds one
  of these internally.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
