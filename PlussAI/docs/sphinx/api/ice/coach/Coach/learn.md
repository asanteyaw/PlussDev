<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: efb9ecd26656 -->
# `pluss::ai::ice::Coach::learn`

[Back to `pluss::ai::ice::Coach`](../Coach.md)

## Signature

```cpp
void pluss::ai::ice::Coach::learn(Model& model, Source& source) const;
void pluss::ai::ice::Coach::learn(Model& model, const Series& x_train, const Series& y_train) const;
void pluss::ai::ice::Coach::learn(Model& model, const Series& x_train, const Series& y_train,
                                  const Series& x_val, const Series& y_val) const;
void pluss::ai::ice::Coach::learn(Model& model, const pluss::table::DataFrame& df, std::string_view target) const;
```

## What It Does

Runs the full training loop against `model`, using this `Coach`'s own
[`options()`](options.md) and [`callbacks()`](callbacks.md). The
`Source&` overload is the primitive every other overload is built from
-- each of the other three constructs a
[`TensorSource`](../../source/TensorSource.md) and forwards to it:

- `(x_train, y_train)` builds a `TensorSource` from `x_train`/`y_train`
  alone, or splits them into train/validation parts first when
  [`ValidationPolicy::split_ratio`](../../options/ValidationPolicy.md)
  is positive (the default, `0.1`).
- `(x_train, y_train, x_val, y_val)` builds a `TensorSource` from all
  four directly -- `split_ratio` is never consulted, since the
  validation set is already explicit.
- `(df, target)` extracts a feature matrix and the `target` column from
  `df` (fitting [`TrainOptions::standardize`](../../options/TrainOptions.md)
  first, if set), then calls the `(x_train, y_train)` overload.

Every overload uses `batch_size`/`shuffle`/`seed` from `options()` to
build its `Source`'s own [`BatchPlan`](../../source/BatchPlan.md) --
see Example 2 for the `Source&` overload reconstructing exactly what
`(x_train, y_train)` builds internally.

## Parameters

`Model& model`
: The model to train -- mutated in place. Must satisfy `model.ready()`
  afterward.

`Source& source`
: A pluggable batch feed -- see [`Source`](../../source/Source.md) for
  the abstraction, or reach for
  [`DataSetSource`](../../source/DataSetSource.md) over panel/windowed
  data built through [`data::DataSet`](../../data/DataSet.md).

`const Series& x_train`, `const Series& y_train`
: Training features/targets, consumed per `options().validation.split_ratio`
  as described above.

`const Series& x_val`, `const Series& y_val`
: An explicit validation set, bypassing the split entirely.

`const pluss::table::DataFrame& df`, `std::string_view target`
: A frame whose numeric columns (other than `target`) become features;
  `target` becomes `y_train`.

## Returns

`void` -- training happens entirely for its side effect on `model`.

## Errors

- Throws if `source` (or the `Source` any other overload builds)
  produces no learn batches.
- Throws whatever [`Model::learn_step`](../../model/Model/learn_step.md)/
  [`valid_step`](../../model/Model/valid_step.md) throws for a
  malformed batch.
- See [`Callback::on_exception`](../../callback/Callback/on_exception.md)
  for what happens to a run-time failure mid-loop.

## Practical Examples

Four examples, one per overload: the common `(x_train, y_train)` case,
the `Source&` primitive reconstructing exactly what that case builds
internally, the four-Series overload with an explicit validation set,
and the `DataFrame` overload.

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

  std::cout << "ready() = " << model.ready() << "\n";

  return 0;
}
```

Output

```text
ready() = 1
```

### Example 2

With `split_ratio == 0.0`, `learn(model, x, y)` constructs exactly the
`TensorSource` built by hand here -- confirmed by comparing the exact
`(batch_index, rows)` trace each run produces, independent of the two
models' own (separately randomized) starting weights.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>
#include <memory>
#include <string>

namespace {

class TraceBatchesCallback final : public pluss::ai::ice::Callback {
 public:
  std::string trace{};
  void on_batch_begin(pluss::ai::ice::RunState& state) override {
    trace += "batch_index=" + std::to_string(state.batch_index) +
              " rows=" + std::to_string(state.rows) + ";";
  }
};

} // namespace

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;

  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});
  const pluss::Series y = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4, 1});

  ice::TrainOptions options{};
  options.execution.max_epochs = 1;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.0;
  options.batch_size = 2;
  options.shuffle = false;

  ice::Outline model_via_source(nn::Fusion(2, 1));
  ice::TensorSource source(
      x, y, std::nullopt, std::nullopt,
      ice::BatchPlan{.batch_size = options.batch_size, .shuffle = options.shuffle, .seed = options.seed});
  const auto trace_via_source = std::make_shared<TraceBatchesCallback>();
  ice::Coach source_coach(options);
  source_coach.add_callback(trace_via_source);
  source_coach.learn(model_via_source, source);

  ice::Outline model_via_pair(nn::Fusion(2, 1));
  const auto trace_via_pair = std::make_shared<TraceBatchesCallback>();
  ice::Coach pair_coach(options);
  pair_coach.add_callback(trace_via_pair);
  pair_coach.learn(model_via_pair, x, y);

  std::cout << "trace matches: " << (trace_via_source->trace == trace_via_pair->trace) << "\n";
  std::cout << "trace = " << trace_via_pair->trace << "\n";

  return 0;
}
```

Output

```text
trace matches: 1
trace = batch_index=0 rows=2;batch_index=1 rows=2;
```

### Example 3

The four-Series overload supplies the validation set directly --
`validation.split_ratio` is never consulted.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>
#include <memory>

namespace {

class CountValidBatchesCallback final : public pluss::ai::ice::Callback {
 public:
  int valid_count = 0;
  void on_batch_begin(pluss::ai::ice::RunState& state) override {
    if (state.stage == pluss::ai::ice::StepStage::Valid) ++valid_count;
  }
};

} // namespace

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;

  ice::Outline model(nn::Fusion(2, 1));
  const pluss::Series x_train = pluss::make_series({0.0f, 0.0f, 1.0f, 0.0f}, {2, 2});
  const pluss::Series y_train = pluss::make_series({0.0f, 1.0f}, {2, 1});
  const pluss::Series x_val = pluss::make_series({0.0f, 1.0f, 1.0f, 1.0f}, {2, 2});
  const pluss::Series y_val = pluss::make_series({1.0f, 2.0f}, {2, 1});

  ice::TrainOptions options{};
  options.execution.max_epochs = 1;
  options.learning_rate = 0.1;
  // validation.split_ratio is irrelevant here -- the four-Series overload
  // supplies the validation set explicitly rather than splitting x_train.

  const auto counter = std::make_shared<CountValidBatchesCallback>();
  ice::Coach coach(options);
  coach.add_callback(counter);
  coach.learn(model, x_train, y_train, x_val, y_val);

  std::cout << "ready() = " << model.ready() << "\n";
  std::cout << "validation batches ran = " << counter->valid_count << "\n";

  return 0;
}
```

Output

```text
ready() = 1
validation batches ran = 1
```

### Example 4

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

  std::cout << "ready() = " << model.ready() << "\n";

  return 0;
}
```

Output

```text
ready() = 1
```

## See Also

- [`infer`](infer.md) / [`assess`](assess.md) -- the matching overload
  families for the other two phases.
- [`Source`](../../source/Source.md) /
  [`TensorSource`](../../source/TensorSource.md) -- what every overload
  ultimately runs through.
- [`Model::learn`](../../model/Model/learn.md) -- the convenience wrapper that
  runs through a fresh `Coach`; caller-supplied callbacks require this explicit
  surface.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="infer.html">
    <span class="label">Previous</span>
    <span class="title">infer</span>
  </a>
  <a class="next-link" href="options.html">
    <span class="label">Next</span>
    <span class="title">options</span>
  </a>
</div>
<!-- apiref:nav:end -->
