<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b724cc44d12b -->
# `pluss::ai::ice::Coach::infer`

[Back to `pluss::ai::ice::Coach`](../Coach.md)

## Signature

```cpp
[[nodiscard]] Series pluss::ai::ice::Coach::infer(const Model& model, Source& source) const;
[[nodiscard]] Series pluss::ai::ice::Coach::infer(const Model& model, const Series& x, std::int64_t chunk_size = 0) const;
[[nodiscard]] Series pluss::ai::ice::Coach::infer(const Model& model, const pluss::table::DataFrame& df,
                                                  std::string_view target = {}, std::int64_t chunk_size = 0) const;
```

## What It Does

Runs `model` over every row of its input and returns the concatenated
predictions. `model` must already satisfy `model.ready()`. Like
[`learn`](learn.md), the `Source&` overload is the primitive: the
`Series` overload builds a
[`TensorSource`](../../source/TensorSource.md) with `chunk_size` as
that source's own `BatchPlan::batch_size` (`0` meaning one pass over
every row) and forwards to it; the `DataFrame` overload extracts a
feature matrix (optionally re-applying fitted standardization), then
calls the `Series` overload. `on_run_begin`/`on_batch_begin`/
`on_batch_end`/`on_run_end` all fire through this call, same as
[`learn`](learn.md) -- see
[`Callback`'s own Introduction](../../callback/Callback/Introduction.md)
for the shorter `infer`/`assess` hook shape.

## Parameters

`const Model& model`
: The trained model -- `const`, never mutated by inference itself
  (though `Coach` does briefly toggle `is_training()` internally and
  restore it afterward).

`Source& source`
: A pluggable batch feed, iterated once over `infer_batches()`.

`const Series& x`
: Raw input features.

`std::int64_t chunk_size`
: Rows per batch; `0` (the default) means every row in one batch.
  Chunking changes nothing about the result -- inference has no
  batch-dependent randomness -- only how much device memory a single
  forward pass needs at once.

`const pluss::table::DataFrame& df`
: A frame whose columns (other than `target`, if named) become
  features.

`std::string_view target`
: The column to exclude from the feature matrix -- name the same
  column `learn` was trained against, even though its values are not
  read here. Left empty (the default), every column becomes a feature,
  which only works when `df` genuinely has no target column at all
  (see Example 3's failure case otherwise).

## Returns

`Series` -- every batch's predictions concatenated along axis 0, in
input row order regardless of `chunk_size`.

## Errors

- Throws if `model.ready()` is `false`.
- Throws whatever [`Model::infer_step`](../../model/Model/infer_step.md)
  throws for a shape mismatch between the input and the model's
  parameters -- see Example 3.

## Practical Examples

Three examples: the `Series`/`chunk_size` overload, the `Source&`
primitive reconstructing exactly what a given `chunk_size` builds
internally, and the `DataFrame` overload's `target` parameter, both
naming it correctly and the shape-mismatch failure from leaving it out
on a frame that still has the column.

### Example 1

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
  options.execution.max_epochs = 100;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.0;

  const ice::Coach coach(options);
  coach.learn(model, x, y);

  const pluss::Series predictions = coach.infer(model, x);
  std::cout << "predictions:\n" << predictions << "\n";

  return 0;
}
```

Output

```text
predictions:
pluss::Series(shape=[4,1], strides=[1,1], dtype=float32, platform=cpu, numel=4, contiguous=true)
data=[[0.0092697404],
  [1.0043123],
  [1.0074358],
  [2.0024784]]
```

### Example 2

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
  options.execution.max_epochs = 100;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.0;

  const ice::Coach coach(options);
  coach.learn(model, x, y);

  // chunk_size becomes the infer Source's own BatchPlan::batch_size --
  // reconstructing it by hand produces the identical predictions, chunked
  // in pieces of 2 rows instead of one pass over all 4.
  ice::TensorSource source(x, ice::BatchPlan{.batch_size = 2});
  const pluss::Series via_source = coach.infer(model, source);
  const pluss::Series via_chunk_size = coach.infer(model, x, /*chunk_size=*/2);

  const double diff = via_source.sub(via_chunk_size).abs().amax().item<double>();
  std::cout << "diff = " << diff << "\n";

  return 0;
}
```

Output

```text
diff = 0
```

### Example 3

Naming the real target column excludes it from the feature matrix,
reproducing exactly the shape `learn` trained on. Leaving `target`
empty on the same frame leaves all three columns (including `"label"`)
as features -- a shape the model was never trained on, so the forward
pass throws.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f32 = pluss::Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  const pluss::Series x0 = pluss::make_series({0.0f, 0.0f, 1.0f, 1.0f}, {4}, f32);
  const pluss::Series x1 = pluss::make_series({0.0f, 1.0f, 0.0f, 1.0f}, {4}, f32);
  const pluss::Series label = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4}, f32);

  const DataFrame df({
      {"x0", Slate(x0)},
      {"x1", Slate(x1)},
      {"label", Slate(label)},
  });

  ice::Outline model(nn::Fusion(2, 1));
  ice::TrainOptions options{};
  options.execution.max_epochs = 100;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.0;

  const ice::Coach coach(options);
  coach.learn(model, df, "label");

  const pluss::Series with_target_excluded = coach.infer(model, df, "label");
  std::cout << "with target excluded:\n" << with_target_excluded << "\n";

  bool threw = false;
  try {
    const pluss::Series with_target_included = coach.infer(model, df);
    static_cast<void>(with_target_included);
  } catch (const std::exception&) {
    threw = true;
  }
  std::cout << "infer(model, df) with no target name threw: " << threw << "\n";

  return 0;
}
```

Output

```text
with target excluded:
pluss::Series(shape=[4,1], strides=[1,1], dtype=float32, platform=cpu, numel=4, contiguous=true)
data=[[0.0092697404],
  [1.0043123],
  [1.0074358],
  [2.0024784]]
infer(model, df) with no target name threw: 1
```

## See Also

- [`learn`](learn.md) -- the same `Source&`-primitive shape.
- [`assess`](assess.md) -- runs inference and compares against ground
  truth in one call.
- [`TrainOptions::standardize`](../../options/TrainOptions.md) -- when
  set, the `DataFrame` overload re-applies the fitted standardization
  automatically.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="callbacks.html">
    <span class="label">Previous</span>
    <span class="title">callbacks</span>
  </a>
  <a class="next-link" href="learn.html">
    <span class="label">Next</span>
    <span class="title">learn</span>
  </a>
</div>
<!-- apiref:nav:end -->
