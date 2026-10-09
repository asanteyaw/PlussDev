<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: bcfcc0bc7fcc -->
# `pluss::ai::ice::Source::test_batches`

[Back to `pluss::ai::ice::Source`](../Source.md)

## Signature

```cpp
[[nodiscard]] virtual const std::vector<pluss::ai::ice::data::Batch>& pluss::ai::ice::Source::test_batches() const = 0;
```

## What It Does

Returns every test batch, populated by [`prepare`](prepare.md).
[`Coach::assess`](../../coach/Coach/assess.md) is what actually reads
this. On [`TensorSource`](../TensorSource.md) it is populated only by
the dedicated two-`Series` constructor
(`TensorSource(x_test, y_test)`) -- distinct from the four/five-argument
training constructor's `x_valid`/`y_valid`, which populate
[`valid_batches`](valid_batches.md) instead. On
[`DataSetSource`](../DataSetSource.md), only when constructed with a
`test` `DataSet`.

## Parameters

None.

## Returns

`const std::vector<data::Batch>&` -- empty before
[`prepare`](prepare.md), or if this source has no test data.

## Errors

None.

## Practical Examples

Three examples: populated by the dedicated two-`Series` constructor
and nothing else, confirming it is distinct from `valid_batches`, and
the `DataSetSource` equivalent.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  const pluss::Series x_test = pluss::make_series({0.0f, 0.0f, 1.0f, 0.0f}, {2, 2});
  const pluss::Series y_test = pluss::make_series({0.0f, 1.0f}, {2, 1});

  // The dedicated two-Series constructor populates test_batches() only --
  // learn_batches()/valid_batches()/infer_batches() all stay empty.
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

### Example 2

The four/five-argument training constructor's `x_valid`/`y_valid`
populate [`valid_batches`](valid_batches.md), never `test_batches` --
the two accessors are populated by two entirely different constructors,
never both by the same `TensorSource`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  const pluss::Series x_train = pluss::make_series({0.0f, 0.0f}, {1, 2});
  const pluss::Series y_train = pluss::make_series({0.0f}, {1, 1});
  const pluss::Series x_val = pluss::make_series({1.0f, 1.0f}, {1, 2});
  const pluss::Series y_val = pluss::make_series({2.0f}, {1, 1});

  ice::TensorSource source(x_train, y_train, x_val, y_val, ice::BatchPlan{});
  source.prepare();
  std::cout << "valid_batches().size() = " << source.valid_batches().size() << "\n";
  std::cout << "test_batches().size() = " << source.test_batches().size() << "\n";

  return 0;
}
```

Output

```text
valid_batches().size() = 1
test_batches().size() = 0
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

## See Also

- [`prepare`](prepare.md) -- what populates this.
- [`valid_batches`](valid_batches.md) -- the sibling accessor
  `TensorSource` populates from a completely different constructor.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="prepare.html">
    <span class="label">Previous</span>
    <span class="title">prepare</span>
  </a>
  <a class="next-link" href="transfer_batch.html">
    <span class="label">Next</span>
    <span class="title">transfer_batch</span>
  </a>
</div>
<!-- apiref:nav:end -->
