<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b2d86a6cb68d -->
# `pluss::ai::classic::Flow::add_function_transformer`

`add_function_transformer` wraps a plain `Series -> Series` callable as a
[`Flow`](../Flow.md) stage — for a one-off transform that does not warrant its
own [`prep`](../../prep.md) type. Internally it builds a
`prep::FunctionTransformer` and forwards to
[`append_transform`](append_transform.md), so it shares that method's
consequences: the stage is **opaque** (it does its own internal routing on
the whole batch, ignoring `feature_route`), and adding one sets
[`has_opaque_transforms()`](../Flow.md#state) — which in turn blocks
[`clone()`](clone.md) and blueprint round-tripping.

## Signature

```cpp
classic::Status classic::Flow::add_function_transformer(classic::prep::FunctionTransformer transformer);
```

## Parameters

`transformer` — `prep::FunctionTransformer`, by value (moved in)
: Constructed from a `SeriesMapFn` (`std::function<StatusOr<Series>(const
  Series&)>`) — the callable receives the stage's entire input `Series` (every
  feature column, every row) and must return one of the same sample count. By
  default (`FunctionTransformerOptions::preserve_sample_count = true`) a
  callable that changes the row count fails at `map()` time, not here — see
  Errors.

## Returns

A `Status`. `.ok()` is `true` whenever `transformer` holds a valid callable
(mirrors [`append_transform`](append_transform.md)'s own contract) — the
stage is appended and both [`stage_count()`](../Flow.md#state) and
[`has_opaque_transforms()`](../Flow.md#state) update immediately. The
callable itself is not invoked until [`map`](map.md) / [`learn_map`](learn_map.md).

## Errors

- `Status::invalid_argument` — `transformer` was default-constructed with no
  callable (mirrors `append_transform`'s null-pointer check).
- Failures from the callable itself surface later, from
  [`map`](map.md)/[`learn_map`](learn_map.md), not from this call:
  `Status::shape_mismatch` if the callable returns a feature count `map()`
  cannot reconcile, or changes the sample count while
  `preserve_sample_count` is still its default `true`.

## Practical Examples

### Example 1

Wrapping a doubling callable, then running it through
[`Flow::map`](map.md) on a transform-only flow.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::prep::FunctionTransformer doubler(
      [](const pluss::Series& x) -> cls::StatusOr<pluss::Series> { return cls::StatusOr<pluss::Series>(x.mul(2.0)); });

  cls::Flow flow{};
  const cls::Status s = flow.add_function_transformer(std::move(doubler));
  std::cout << "ok = " << std::boolalpha << s.ok() << "\n";
  std::cout << "stage_count() = " << flow.stage_count() << "\n";
  std::cout << "has_opaque_transforms() = " << flow.has_opaque_transforms() << "\n";

  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64).reshape({2, 2});
  cls::FeatureBatch batch{};
  batch.x = x;
  const pluss::Series mapped = flow.map(batch).value();
  std::cout << "mapped = " << mapped << "\n";

  return 0;
}
```

Output

```text
ok = true
stage_count() = 1
has_opaque_transforms() = true
mapped = pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[2, 4],
  [6, 8]]
```

### Example 2

A callable that changes the sample count is accepted here — the shape
contract is only enforced once `map()` actually runs it.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // Drops the last row -- an invalid transform, since row count must be
  // preserved by default (FunctionTransformerOptions::preserve_sample_count).
  cls::prep::FunctionTransformer drops_a_row([](const pluss::Series& x) -> cls::StatusOr<pluss::Series> {
    return cls::StatusOr<pluss::Series>(x.slice(0, 0, 1));
  });

  cls::Flow flow{};
  const cls::Status add_status = flow.add_function_transformer(std::move(drops_a_row));
  std::cout << "add ok = " << std::boolalpha << add_status.ok() << "\n";

  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64).reshape({2, 2});
  cls::FeatureBatch batch{};
  batch.x = x;
  const cls::StatusOr<pluss::Series> mapped = flow.map(batch);
  std::cout << "map ok = " << mapped.ok() << "\n";
  std::cout << "map message = \"" << mapped.status().message << "\"\n";

  return 0;
}
```

Output

```text
add ok = true
map ok = false
map message = "FunctionTransformer::map callable changed sample count."
```

### Example 3

The practical consequence of the opaque stage this creates:
[`clone()`](clone.md) refuses to run once
[`has_opaque_transforms()`](../Flow.md#state) is `true`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::prep::FunctionTransformer identity(
      [](const pluss::Series& x) -> cls::StatusOr<pluss::Series> { return cls::StatusOr<pluss::Series>(x); });

  cls::Flow flow{};
  flow.add_function_transformer(std::move(identity));
  std::cout << "has_opaque_transforms() = " << std::boolalpha << flow.has_opaque_transforms() << "\n";

  const cls::StatusOr<cls::Flow> cloned = flow.clone();
  std::cout << "clone ok = " << cloned.ok() << "\n";
  std::cout << "clone message = \"" << cloned.status().message << "\"\n";

  return 0;
}
```

Output

```text
has_opaque_transforms() = true
clone ok = false
clone message = "Flow::clone does not support opaque append_transform stages."
```

## See Also

- [`add_feature_transformer`](add_feature_transformer.md) — the same
  opaque-stage mechanism, for a pre-built sub-pipeline instead of a bare
  callable.
- [`append_transform`](append_transform.md) — the general primitive both of
  these forward to.
- [`clone`](clone.md) / [`save_blueprint`](save_blueprint.md) — the two
  operations an opaque stage blocks.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="add_feature_transformer.html">
    <span class="label">Previous</span>
    <span class="title">add_feature_transformer</span>
  </a>
  <a class="next-link" href="add_named_transform.html">
    <span class="label">Next</span>
    <span class="title">add_named_transform</span>
  </a>
</div>
<!-- apiref:nav:end -->
