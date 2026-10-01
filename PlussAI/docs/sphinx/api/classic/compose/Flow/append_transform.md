<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3ef368a0598d -->
# `pluss::ai::classic::Flow::append_transform`

`append_transform` is the general primitive underneath both
[`add_function_transformer`](add_function_transformer.md) and
[`add_feature_transformer`](add_feature_transformer.md): attach any
`TransformerBase` — a `prep::` type built directly, or a hand-authored
subclass — as an opaque [`Flow`](../Flow.md) stage, bypassing `TransformKind`
entirely. Reach for it directly when neither convenience wrapper fits (a
subclass of your own), and for the two wrappers' own implementation, which is
exactly this call plus a `std::make_unique`.

An appended stage always sees the **whole** batch — there is no
`feature_route` parameter here, unlike every `add_*` method covered under
[`add_stage`](add_stage.md)/[`add_named_transform`](add_named_transform.md).
Route a subset of columns to it yourself first (build the routing into a
[`prep::FeatureTransformer`](add_feature_transformer.md) and append that
instead), or give it a callable that only touches the columns it cares about.

## Signature

```cpp
classic::Status classic::Flow::append_transform(std::unique_ptr<classic::TransformerBase> transformer);
```

## What It Does

Takes ownership of any `TransformerBase` -- whatever concrete type it
actually is -- and appends it to the `Flow` as an opaque stage that sees
the entire batch every time it runs. This is the lowest-level attachment
primitive in this family; both
[`add_function_transformer`](add_function_transformer.md) and
[`add_feature_transformer`](add_feature_transformer.md) are thin
convenience wrappers that construct their specific transformer type and
call this directly.

## Parameters

`transformer` — `std::unique_ptr<TransformerBase>`
: Ownership transfers to the `Flow`. Any concrete `TransformerBase` works —
  every [`prep`](../../prep.md) type (`std::make_unique<prep::StandardScaler>()`,
  …), a `prep::FunctionTransformer`/`prep::FeatureTransformer`, or your own
  subclass.

## Returns

A `Status`. `.ok()` on success — the stage is appended,
[`stage_count()`](../Flow.md#state) increases by one, and
[`has_opaque_transforms()`](../Flow.md#state) becomes `true`. A matching
empty placeholder is pushed onto [`transform_specs()`](../TransformSpec.md)
at the same index, purely to keep that list index-aligned with the internal
transformer list — see Example 3.

## Errors

- `Status::invalid_argument` — `transformer` is null.

## Practical Examples

### Example 1

Attaching a `prep::StandardScaler` directly — the same type
[`add_standard_scaler`](add_stage.md) would add, but as an opaque, unrouted
stage instead of a named one.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::Flow flow{};
  const cls::Status s = flow.append_transform(std::make_unique<cls::prep::StandardScaler>());
  std::cout << "ok = " << std::boolalpha << s.ok() << "\n";
  std::cout << "stage_count() = " << flow.stage_count() << "\n";
  std::cout << "has_opaque_transforms() = " << flow.has_opaque_transforms() << "\n";

  const pluss::Series x = pluss::make_series({1.0, 10.0, 2.0, 20.0, 3.0, 30.0}, {6}, f64).reshape({3, 2});
  cls::FeatureBatch batch{};
  batch.x = x;
  const pluss::Series mapped = flow.learn_map(batch).value();
  std::cout << "mapped = " << mapped << "\n";

  return 0;
}
```

Output

```text
ok = true
stage_count() = 1
has_opaque_transforms() = true
mapped = pluss::Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[-1.2247449, -1.2247449],
  [0, 0],
  [1.2247449, 1.2247449]]
```

### Example 2

A null transformer.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow{};
  const cls::Status s = flow.append_transform(nullptr);
  std::cout << "ok = " << std::boolalpha << s.ok() << "\n";
  std::cout << "message = \"" << s.message << "\"\n";
  std::cout << "stage_count() = " << flow.stage_count() << "\n";

  return 0;
}
```

Output

```text
ok = false
message = "Flow::append_transform requires a non-null transformer."
stage_count() = 0
```

### Example 3

Mixing a named stage, an opaque `append_transform` stage, and another named
stage: `transform_specs()` carries an empty placeholder at the opaque stage's
index, keeping every index aligned with the underlying transformer list so a
later stage's route is never misapplied to the wrong transformer.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow{};
  flow.add_standard_scaler({}, {0});                                    // named, index 0
  flow.append_transform(std::make_unique<cls::prep::OneHotEncoder>());  // opaque, index 1
  flow.add_one_hot_encoder({}, {2});                                    // named, index 2

  std::cout << "stage_count() = " << flow.stage_count() << "\n";
  const std::vector<cls::TransformSpec>& specs = flow.transform_specs();
  for (std::size_t i = 0; i < specs.size(); ++i) {
    std::cout << "spec[" << i << "]: route size = " << specs[i].feature_route.size() << "\n";
  }
  std::cout << "has_opaque_transforms() = " << std::boolalpha << flow.has_opaque_transforms() << "\n";

  return 0;
}
```

Output

```text
stage_count() = 3
spec[0]: route size = 1
spec[1]: route size = 0
spec[2]: route size = 1
has_opaque_transforms() = true
```

## See Also

- [`add_function_transformer`](add_function_transformer.md) /
  [`add_feature_transformer`](add_feature_transformer.md) — the two typed
  convenience wrappers over this same call.
- [`clear_transforms`](clear_transforms.md) — remove every stage, opaque or
  named.
- [`clone`](clone.md) / [`save_blueprint`](save_blueprint.md) — the two
  operations `has_opaque_transforms()` blocks.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="add_stage.html">
    <span class="label">Previous</span>
    <span class="title">add_stage</span>
  </a>
  <a class="next-link" href="assess.html">
    <span class="label">Next</span>
    <span class="title">assess</span>
  </a>
</div>
<!-- apiref:nav:end -->
