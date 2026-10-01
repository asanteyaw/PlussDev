<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2648150facc3 -->
# `pluss::ai::classic::Flow::set_stage_route`

`set_stage_route` re-routes an existing [`Flow`](../Flow.md) stage to a
different set of columns, without removing and re-adding it. Because the
route is part of what a fit depends on, this **resets**
[`trained()`](../Flow.md#state) — contrast with
[`set_stage_key`](set_stage_key.md), which does not.

## Signature

```cpp
classic::Status classic::Flow::set_stage_route(
    std::int64_t stage_index, /* feature indices */ feature_route);
```

## What It Does

Rewrites the stage at `stage_index`'s `feature_route` to a new set of
columns, in place, without removing and re-adding the stage. Because the
route is part of what a fit actually depends on, this resets `trained()`
-- unlike [`set_stage_key`](set_stage_key.md), whose renaming has no
bearing on fitted state.

## Parameters

`stage_index` — `std::int64_t`, 0-based
: Which stage to re-route — an index into [`transform_specs()`](../TransformSpec.md).

`feature_route`
: The new 0-based column indices, as a braced list or any range of
  `std::int64_t`. An empty route means "every column" — the same convention
  every `add_*` method uses.

## Returns

A `Status`. `.ok()` on success — the stage's
[`TransformSpec::feature_route`](../TransformSpec.md#state) updates and
[`trained()`](../Flow.md#state) becomes `false`.

## Errors

- `Status::invalid_argument` — `stage_index` is negative or `>=`
  [`stage_count()`](../Flow.md#state).

## Practical Examples

### Example 1

Widening a stage's route from one column to two.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow{};
  flow.add_standard_scaler({}, {0});
  std::cout << "route size before = " << flow.transform_specs().front().feature_route.size() << "\n";

  const cls::Status s = flow.set_stage_route(0, {0, 1});
  std::cout << "ok = " << std::boolalpha << s.ok() << "\n";
  std::cout << "route size after = " << flow.transform_specs().front().feature_route.size() << "\n";

  return 0;
}
```

Output

```text
route size before = 1
ok = true
route size after = 2
```

### Example 2

An out-of-range index, and an empty route clearing back to "every column".

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow{};
  flow.add_standard_scaler({}, {0});

  const cls::Status out_of_range = flow.set_stage_route(3, {0});
  std::cout << "out-of-range ok = " << std::boolalpha << out_of_range.ok() << "\n";
  std::cout << "out-of-range message = \"" << out_of_range.message << "\"\n";

  const cls::Status cleared = flow.set_stage_route(0, {});
  std::cout << "clear-route ok = " << cleared.ok() << "\n";
  std::cout << "route.empty() after clearing = " << flow.transform_specs().front().feature_route.empty() << "\n";

  return 0;
}
```

Output

```text
out-of-range ok = false
out-of-range message = "Flow::set_stage_route stage index out of range."
clear-route ok = true
route.empty() after clearing = true
```

### Example 3

Re-routing resets `trained()` — a fit flow becomes untrained again the
moment its routing changes.

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

  const pluss::Series x = pluss::make_series({1.0, 2.0, 2.0, 4.0, 3.0, 6.0}, {6}, f64).reshape({3, 2});
  cls::FeatureBatch batch{};
  batch.x = x;
  const cls::StatusOr<pluss::Series> mapped = flow.learn_map(batch);
  std::cout << "learn_map ok = " << std::boolalpha << mapped.ok() << "\n";
  std::cout << "trained() after learn_map() = " << flow.trained() << "\n";

  flow.set_stage_route(0, {0, 1});
  std::cout << "trained() after set_stage_route() = " << flow.trained() << "\n";

  return 0;
}
```

Output

```text
learn_map ok = true
trained() after learn_map() = true
trained() after set_stage_route() = false
```

## See Also

- [`set_stage_remainder`](set_stage_remainder.md) — the other route-adjacent
  mutator, also resets `trained()`.
- [`set_stage_key`](set_stage_key.md) — the one stage mutator that does not.
- [`TransformSpec`](../TransformSpec.md) — the `feature_route` field this
  changes.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="set_stage_remainder.html">
    <span class="label">Previous</span>
    <span class="title">set_stage_remainder</span>
  </a>
</div>
<!-- apiref:nav:end -->
