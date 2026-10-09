<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 11f42cde349a -->
# `pluss::ai::ice::Source::valid_batches`

[Back to `pluss::ai::ice::Source`](../Source.md)

## Signature

```cpp
[[nodiscard]] virtual const std::vector<pluss::ai::ice::data::Batch>& pluss::ai::ice::Source::valid_batches() const = 0;
```

## What It Does

Returns every validation batch, populated by [`prepare`](prepare.md).
`Coach::learn`'s own loop runs this once per validated epoch. Optional
for both built-in sources: [`TensorSource`](../TensorSource.md) only
populates it when its four/five-argument constructor is given
`x_valid`/`y_valid`; [`DataSetSource`](../DataSetSource.md) only when
constructed with a `valid` `DataSet`.

## Parameters

None.

## Returns

`const std::vector<data::Batch>&` -- empty before
[`prepare`](prepare.md), or if this source has no validation data.

## Errors

None.

## Practical Examples

Three examples: present only when `x_valid`/`y_valid` are given, always
exactly one unshuffled batch regardless of the training `BatchPlan`,
and the analogous optional `valid` `DataSet` on `DataSetSource`.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  const pluss::Series x_train = pluss::make_series({0.0f, 0.0f, 1.0f, 0.0f}, {2, 2});
  const pluss::Series y_train = pluss::make_series({0.0f, 1.0f}, {2, 1});
  const pluss::Series x_val = pluss::make_series({1.0f, 1.0f}, {1, 2});
  const pluss::Series y_val = pluss::make_series({2.0f}, {1, 1});

  ice::TensorSource with_valid(x_train, y_train, x_val, y_val, ice::BatchPlan{});
  with_valid.prepare();
  std::cout << "with x_valid given: valid_batches().size() = " << with_valid.valid_batches().size() << "\n";

  ice::TensorSource without_valid(x_train, y_train, std::nullopt, std::nullopt, ice::BatchPlan{});
  without_valid.prepare();
  std::cout << "without x_valid:    valid_batches().size() = " << without_valid.valid_batches().size() << "\n";

  return 0;
}
```

Output

```text
with x_valid given: valid_batches().size() = 1
without x_valid:    valid_batches().size() = 0
```

### Example 2

The training `BatchPlan` asks for 2-row batches, but `valid_batches()`
is always one unshuffled batch holding every validation row --
`TensorSource` never applies the training `BatchPlan` to validation
data.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  const pluss::Series x_train = pluss::make_series({0.0f, 0.0f}, {1, 2});
  const pluss::Series y_train = pluss::make_series({0.0f}, {1, 1});
  const pluss::Series x_val = pluss::make_series({0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {3, 2});
  const pluss::Series y_val = pluss::make_series({0.0f, 1.0f, 2.0f}, {3, 1});

  ice::TensorSource source(x_train, y_train, x_val, y_val, ice::BatchPlan{.batch_size = 2});
  source.prepare();
  std::cout << "valid_batches().size() = " << source.valid_batches().size() << "\n";
  std::cout << "valid_batches()[0].size() = " << source.valid_batches()[0].size() << "\n";

  return 0;
}
```

Output

```text
valid_batches().size() = 1
valid_batches()[0].size() = 3
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

  // Same optionality on DataSetSource -- the second constructor
  // argument is std::nullopt by default.
  ice::DataSetSource source(train);
  source.prepare();
  std::cout << "valid_batches().size() = " << source.valid_batches().size() << "\n";

  return 0;
}
```

Output

```text
valid_batches().size() = 0
```

## See Also

- [`prepare`](prepare.md) -- what populates this.
- [`learn_batches`](learn_batches.md) -- the one accessor that is never
  optional.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="transfer_batch.html">
    <span class="label">Previous</span>
    <span class="title">transfer_batch</span>
  </a>
</div>
<!-- apiref:nav:end -->
