<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6387efdfdc52 -->
# `pluss::ai::ice::Source::prepare`

[Back to `pluss::ai::ice::Source`](../Source.md)

## Signature

```cpp
virtual void pluss::ai::ice::Source::prepare() = 0;
```

## What It Does

Materializes every batch this source will hand out -- after this
returns, [`learn_batches`](learn_batches.md)/
[`valid_batches`](valid_batches.md)/[`test_batches`](test_batches.md)/
[`infer_batches`](infer_batches.md) are populated (whichever of them
this source actually has data for). `Coach` calls this exactly once,
at the start of every [`learn`](../../coach/Coach/learn.md)/
[`infer`](../../coach/Coach/infer.md)/
[`assess`](../../coach/Coach/assess.md) call, before reading any
batches -- a caller driving a `Source` directly needs to call it too.
Both built-in implementations
([`TensorSource`](../TensorSource.md)/[`DataSetSource`](../DataSetSource.md))
honor an idempotency contract worth preserving in any hand-written
subclass: a second call is a cheap no-op, not a rebuild.

## Parameters

None.

## Returns

`void`.

## Errors

Implementation-defined; both built-in sources throw if asked to
materialize an empty input (see
[`TensorSource`](../TensorSource.md)/[`DataSetSource`](../DataSetSource.md)'s
own pages).

## Practical Examples

Three examples: batches appearing only after `prepare()` runs, a
second call changing nothing, and confirming `Coach::learn` calls it
automatically so a caller handing it a `Source` directly never has to.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 1.0f, 0.0f}, {2, 2});
  const pluss::Series y = pluss::make_series({0.0f, 1.0f}, {2, 1});

  ice::TensorSource source(x, y, std::nullopt, std::nullopt, ice::BatchPlan{});
  std::cout << "before prepare(): learn_batches().size() = " << source.learn_batches().size() << "\n";

  source.prepare();
  std::cout << "after prepare():  learn_batches().size() = " << source.learn_batches().size() << "\n";

  return 0;
}
```

Output

```text
before prepare(): learn_batches().size() = 0
after prepare():  learn_batches().size() = 1
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 1.0f, 0.0f}, {2, 2});
  const pluss::Series y = pluss::make_series({0.0f, 1.0f}, {2, 1});

  ice::TensorSource source(x, y, std::nullopt, std::nullopt, ice::BatchPlan{});
  source.prepare();
  std::cout << "after first prepare():  learn_batches().size() = " << source.learn_batches().size() << "\n";

  source.prepare();
  source.prepare();
  std::cout << "after two more prepare(): learn_batches().size() = " << source.learn_batches().size() << "\n";

  return 0;
}
```

Output

```text
after first prepare():  learn_batches().size() = 1
after two more prepare(): learn_batches().size() = 1
```

### Example 3

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;

  ice::Outline model(nn::Fusion(2, 1));
  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});
  const pluss::Series y = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4, 1});

  ice::TrainOptions options{};
  options.execution.max_epochs = 1;
  options.validation.split_ratio = 0.0;

  // Never calling source.prepare() ourselves -- Coach::learn calls it.
  ice::TensorSource source(x, y, std::nullopt, std::nullopt, ice::BatchPlan{});
  ice::Coach coach(options);
  coach.learn(model, source);

  std::cout << "ready() = " << model.ready() << "\n";

  return 0;
}
```

Output

```text
ready() = 1
```

## See Also

- [`learn_batches`](learn_batches.md) / [`valid_batches`](valid_batches.md) /
  [`test_batches`](test_batches.md) / [`infer_batches`](infer_batches.md) --
  what this populates.
- [`TensorSource`](../TensorSource.md) / [`DataSetSource`](../DataSetSource.md) --
  the two built-in implementations.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="learn_batches.html">
    <span class="label">Previous</span>
    <span class="title">learn_batches</span>
  </a>
  <a class="next-link" href="test_batches.html">
    <span class="label">Next</span>
    <span class="title">test_batches</span>
  </a>
</div>
<!-- apiref:nav:end -->
