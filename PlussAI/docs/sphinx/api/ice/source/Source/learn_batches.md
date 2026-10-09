<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 737192ad65c2 -->
# `pluss::ai::ice::Source::learn_batches`

[Back to `pluss::ai::ice::Source`](../Source.md)

## Signature

```cpp
[[nodiscard]] virtual const std::vector<pluss::ai::ice::data::Batch>& pluss::ai::ice::Source::learn_batches() const = 0;
```

## What It Does

Returns every training batch, populated by [`prepare`](prepare.md).
`Coach::learn`'s own training loop iterates this once per epoch.
Whether it is ever non-empty depends entirely on how the concrete
`Source` was constructed -- for [`TensorSource`](../TensorSource.md),
only the two four/five-argument training constructors populate it; the
infer-only and test-only constructors never do.

## Parameters

None.

## Returns

`const std::vector<data::Batch>&` -- a reference into the source's own
storage, valid as long as the source itself is alive. Empty before
[`prepare`](prepare.md) runs, or if this source was never given
training data at all.

## Errors

None.

## Practical Examples

Three examples: batches split per `BatchPlan::batch_size`, the
infer-only constructor leaving this empty, and
[`DataSetSource`](../DataSetSource.md)'s own training `DataSet`
populating it unconditionally (unlike `valid`/`test`/`infer`, which are
optional there).

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

  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 1.0f, 0.0f}, {2, 2});

  // The infer-only constructor never populates learn_batches().
  ice::TensorSource source(x, ice::BatchPlan{});
  source.prepare();
  std::cout << "learn_batches().size() = " << source.learn_batches().size() << "\n";
  std::cout << "infer_batches().size() = " << source.infer_batches().size() << "\n";

  return 0;
}
```

Output

```text
learn_batches().size() = 0
infer_batches().size() = 1
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

  // The training DataSet is the only required argument -- learn_batches()
  // is always populated, unlike valid/test/infer which are optional.
  ice::DataSetSource source(train);
  source.prepare();
  std::cout << "learn_batches().size() = " << source.learn_batches().size() << "\n";
  std::cout << "valid_batches().size() = " << source.valid_batches().size() << "\n";

  return 0;
}
```

Output

```text
learn_batches().size() = 1
valid_batches().size() = 0
```

## See Also

- [`prepare`](prepare.md) -- what populates this.
- [`valid_batches`](valid_batches.md) / [`test_batches`](test_batches.md) /
  [`infer_batches`](infer_batches.md) -- the sibling accessors for the
  other three stages.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="infer_batches.html">
    <span class="label">Previous</span>
    <span class="title">infer_batches</span>
  </a>
  <a class="next-link" href="prepare.html">
    <span class="label">Next</span>
    <span class="title">prepare</span>
  </a>
</div>
<!-- apiref:nav:end -->
