# `pluss::ai::ice::SourceContext`

A plain, read-only context struct: what
[`Source::transfer_batch`](Source/transfer_batch.md) (and the
`do_transfer_batch` extension point it calls) receives alongside a raw
`data::Batch`, telling it which stage is running and where the
device-resident result should end up. Never constructed by calling
code in the ordinary `learn`/`infer`/`assess` flow --
[`Coach`](../coach/Coach.md) builds one per batch internally; a caller
only constructs one by hand when calling `transfer_batch` directly,
outside that loop.

## Signature

```cpp
pluss::ai::ice::SourceContext::SourceContext() = default;
```

A plain aggregate -- every field below defaults to a harmless empty/
sentinel value (`recipe = nullptr`, `stage = StepStage::Infer`,
`target_platform = pluss::pCPU`, `target_dtype = ScalarType::Float32`),
but a default-constructed instance only matters as a starting point for
setting the fields a direct `transfer_batch` call needs.

## State

### `recipe`

`const TrainOptions* recipe = nullptr`
: The [`TrainOptions`](../options/TrainOptions.md) this batch is being
  transferred under -- `nullptr` for `infer`/`assess`, which have no
  recipe (see [`Coach::infer`](../coach/Coach/infer.md)/
  [`assess`](../coach/Coach/assess.md)'s own `SourceContext` construction).

### `stage`

`StepStage stage = StepStage::Infer`
: Which phase this batch belongs to -- see
  [`StepContext::stage`](../step/StepContext.md) for the same enum's
  full meaning.

### `target_platform`

`Platform target_platform = pluss::pCPU`
: The platform the transferred batch's tensors should end up on --
  [`resolve_platform`](../platform/resolve_platform.md)'s result for
  the run.

### `target_dtype`

`ScalarType target_dtype = ScalarType::Float32`
: The floating-point dtype the transferred batch's tensors should end
  up in -- only applied to fields that are already a floating dtype;
  an integer (for example categorical codes) field is left untouched.
  See [`TensorSource::do_transfer_batch`](TensorSource.md)/
  [`DataSetSource::do_transfer_batch`](DataSetSource.md) for exactly
  which fields this reaches.

## Methods

None -- `SourceContext` is a plain read-only context; every field is
public and read directly, see `## State` above.

## Practical Examples

One example: constructing one by hand and passing it to
[`transfer_batch`](Source/transfer_batch.md) directly, confirming
`target_dtype` actually casts the transferred batch.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  using pluss::ScalarType;

  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 1.0f, 0.0f}, {2, 2});
  const pluss::Series y = pluss::make_series({0.0f, 1.0f}, {2, 1});

  ice::TensorSource source(x, y, std::nullopt, std::nullopt, ice::BatchPlan{});
  source.prepare();

  ice::SourceContext ctx{};
  ctx.target_platform = pluss::pCPU;
  ctx.target_dtype = ScalarType::Float64;

  const ice::Batch transferred = source.transfer_batch(source.learn_batches().front(), ctx);
  std::cout << "has_inputs() = " << transferred.has_inputs() << "\n";
  std::cout << "has_targets() = " << transferred.has_targets() << "\n";
  std::cout << "inputs dtype == Float64: " << (transferred.inputs().scalar_type() == ScalarType::Float64) << "\n";

  return 0;
}
```

Output

```text
has_inputs() = 1
has_targets() = 1
inputs dtype == Float64: 1
```

## Changelog

- **0.1** -- `recipe`/`stage`/`target_platform`/`target_dtype`, the
  per-batch bookkeeping passed into `Source::transfer_batch`.

## Related Pages

- [`Source::transfer_batch`](Source/transfer_batch.md) -- the public
  entry point this context is passed into.
- [`StepContext`](../step/StepContext.md) -- the analogous read-only
  context for a `Model`'s own step methods, one layer up.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
