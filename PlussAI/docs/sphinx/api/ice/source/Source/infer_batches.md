<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e3af7d38e8b2 -->
# `pluss::ai::ice::Source::infer_batches`

[Back to `pluss::ai::ice::Source`](../Source.md)

## Signature

```cpp
[[nodiscard]] virtual const std::vector<pluss::ai::ice::data::Batch>& pluss::ai::ice::Source::infer_batches() const = 0;
```

## What It Does

Returns every inference batch, populated by [`prepare`](prepare.md).
[`Coach::infer`](../../coach/Coach/infer.md) is what reads this. On
[`TensorSource`](../TensorSource.md), only the single-`Series`
constructor (`TensorSource(x_infer, infer_plan)`) populates it --
`chunk_size` from
[`Coach::infer(model, x, chunk_size)`](../../coach/Coach/infer.md)
becomes that constructor's `BatchPlan::batch_size` directly.

## Parameters

None.

## Returns

`const std::vector<data::Batch>&` -- empty before
[`prepare`](prepare.md), or if this source has no inference data.

## Errors

None.

## Practical Examples

Three examples: `BatchPlan::batch_size` splitting the input into
multiple chunks, confirming it stays empty on a training-only source,
and the `DataSetSource` equivalent.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {3, 2});

  // chunk_size (the infer-constructor's BatchPlan::batch_size) governs
  // infer_batches() the same way learn_plan governs learn_batches().
  ice::TensorSource source(x, ice::BatchPlan{.batch_size = 2});
  source.prepare();
  std::cout << "infer_batches().size() = " << source.infer_batches().size() << "\n";
  std::cout << "infer_batches()[0].size() = " << source.infer_batches()[0].size() << "\n";
  std::cout << "infer_batches()[1].size() = " << source.infer_batches()[1].size() << "\n";
  std::cout << "learn_batches().size() = " << source.learn_batches().size() << "\n";

  return 0;
}
```

Output

```text
infer_batches().size() = 2
infer_batches()[0].size() = 2
infer_batches()[1].size() = 1
learn_batches().size() = 0
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  const pluss::Series x_train = pluss::make_series({0.0f, 0.0f, 1.0f, 0.0f}, {2, 2});
  const pluss::Series y_train = pluss::make_series({0.0f, 1.0f}, {2, 1});

  // The training constructor never populates infer_batches().
  ice::TensorSource source(x_train, y_train, std::nullopt, std::nullopt, ice::BatchPlan{});
  source.prepare();
  std::cout << "infer_batches().size() = " << source.infer_batches().size() << "\n";

  return 0;
}
```

Output

```text
infer_batches().size() = 0
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
  const ice::data::DataSet infer_set = ice::data::DataSet::from_batch(raw);

  ice::DataSetSource source(train, std::nullopt, std::nullopt, infer_set);
  source.prepare();
  std::cout << "infer_batches().size() = " << source.infer_batches().size() << "\n";

  return 0;
}
```

Output

```text
infer_batches().size() = 1
```

## See Also

- [`prepare`](prepare.md) -- what populates this.
- [`Coach::infer`](../../coach/Coach/infer.md) -- `chunk_size` maps
  straight onto this accessor's `BatchPlan::batch_size`.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="learn_batches.html">
    <span class="label">Next</span>
    <span class="title">learn_batches</span>
  </a>
</div>
<!-- apiref:nav:end -->
