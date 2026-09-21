<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4e870e09135d -->
# `pluss::ai::classic::Flow::clear_transforms`

`clear_transforms` removes every transform stage from a [`Flow`](../Flow.md)
— named or opaque — and resets the flags those stages set. Use it to rebuild
a flow's preprocessing from scratch while keeping a Classification or
Regression model spec it already has — a `Transform`-task model is the one
exception, since it runs as the last transform stage rather than a separate
slot: clearing the stage list clears it too (see
[`set_model`](set_model.md#returns)).

## Signature

```cpp
void classic::Flow::clear_transforms() noexcept;
```

## Parameters

None.

## Returns

`void`. Unlike almost every other mutating `Flow` method, `clear_transforms`
cannot fail — there is no invalid state to reject.

## Errors

None — `noexcept`, always succeeds.

## Practical Examples

### Example 1

Clearing a mix of a named and an opaque stage resets
[`has_opaque_transforms()`](../Flow.md#state) along with
[`stage_count()`](../Flow.md#state).

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow{};
  flow.add_standard_scaler({}, {0});
  flow.append_transform(std::make_unique<cls::prep::OneHotEncoder>());
  std::cout << "before: stage_count() = " << flow.stage_count()
            << ", has_opaque_transforms() = " << std::boolalpha << flow.has_opaque_transforms() << "\n";

  flow.clear_transforms();
  std::cout << "after: stage_count() = " << flow.stage_count()
            << ", has_opaque_transforms() = " << flow.has_opaque_transforms() << "\n";

  return 0;
}
```

Output

```text
before: stage_count() = 2, has_opaque_transforms() = true
after: stage_count() = 0, has_opaque_transforms() = false
```

### Example 2

Resets [`trained()`](../Flow.md#state) on a transform-only flow that had
already been fit — [`learn_map`](learn_map.md) leaves a model-less flow
`trained()`; clearing its only stage un-trains it too.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::Flow flow{};
  flow.add_standard_scaler({}, {0});

  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0}, {3}, f64).reshape({3, 1});
  cls::FeatureBatch batch{};
  batch.x = x;
  const cls::StatusOr<pluss::Series> mapped = flow.learn_map(batch);
  std::cout << "learn_map ok = " << std::boolalpha << mapped.ok() << "\n";
  std::cout << "trained() after learn_map() = " << flow.trained() << "\n";

  flow.clear_transforms();
  std::cout << "trained() after clear_transforms() = " << flow.trained() << "\n";
  std::cout << "stage_count() after clear_transforms() = " << flow.stage_count() << "\n";

  return 0;
}
```

Output

```text
learn_map ok = true
trained() after learn_map() = true
trained() after clear_transforms() = false
stage_count() after clear_transforms() = 0
```

### Example 3

Safe to call on a flow with no stages yet, and leaves an attached model
untouched — only [`clear_model`](clear_model.md) affects the model.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  cls::Flow flow =
      cls::make_flow(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear}).move_value();

  flow.clear_transforms();  // no stages yet -- a no-op
  std::cout << "stage_count() = " << flow.stage_count() << "\n";
  std::cout << "has_model() (untouched) = " << std::boolalpha << flow.has_model() << "\n";

  return 0;
}
```

Output

```text
stage_count() = 0
has_model() (untouched) = true
```

## See Also

- [`append_transform`](append_transform.md) / [`add_stage`](add_stage.md) /
  [`add_named_transform`](add_named_transform.md) — what populates the list
  this clears.
- [`clear_model`](clear_model.md) — the narrower counterpart: clears a
  Classification/Regression/Transform model without touching any other
  transform stage.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="clear_model.html">
    <span class="label">Previous</span>
    <span class="title">clear_model</span>
  </a>
  <a class="next-link" href="clone.html">
    <span class="label">Next</span>
    <span class="title">clone</span>
  </a>
</div>
<!-- apiref:nav:end -->
