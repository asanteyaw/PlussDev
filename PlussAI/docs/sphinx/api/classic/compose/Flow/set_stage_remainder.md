<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 98cea720f38e -->
# `pluss::ai::classic::Flow::set_stage_remainder`

`set_stage_remainder` changes an existing [`Flow`](../Flow.md) stage's
`RouteRemainderPolicy` after the fact — the one control the twenty typed
methods under [`add_stage`](add_stage.md) do not expose directly (they always
add a stage with `PassThrough`). Like
[`set_stage_route`](set_stage_route.md), this **resets**
[`trained()`](../Flow.md#state).

## Signature

```cpp
classic::Status classic::Flow::set_stage_remainder(
    std::int64_t stage_index, classic::RouteRemainderPolicy remainder);
```

## Parameters

`stage_index` — `std::int64_t`, 0-based
: Which stage to change — an index into [`transform_specs()`](../TransformSpec.md).

`remainder` — `RouteRemainderPolicy`
: `PassThrough` (columns the stage's route does not name continue unchanged)
  or `Drop` (they are removed from the flow's output entirely) — see
  [`TransformSpec::remainder`](../TransformSpec.md#state).

## Returns

A `Status`. `.ok()` on success — the stage's
[`TransformSpec::remainder`](../TransformSpec.md#state) updates and
[`trained()`](../Flow.md#state) becomes `false`.

## Errors

- `Status::invalid_argument` — `stage_index` is negative or `>=`
  [`stage_count()`](../Flow.md#state).

## Practical Examples

### Example 1

Switching a stage from the default `PassThrough` to `Drop`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow{};
  flow.add_standard_scaler({}, {0});
  std::cout << "remainder before == PassThrough: " << std::boolalpha
            << (flow.transform_specs().front().remainder == cls::RouteRemainderPolicy::PassThrough) << "\n";

  const cls::Status s = flow.set_stage_remainder(0, cls::RouteRemainderPolicy::Drop);
  std::cout << "ok = " << s.ok() << "\n";
  std::cout << "remainder after == Drop: "
            << (flow.transform_specs().front().remainder == cls::RouteRemainderPolicy::Drop) << "\n";

  return 0;
}
```

Output

```text
remainder before == PassThrough: true
ok = true
remainder after == Drop: true
```

### Example 2

An out-of-range index.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow{};
  flow.add_standard_scaler({}, {0});

  const cls::Status s = flow.set_stage_remainder(1, cls::RouteRemainderPolicy::Drop);
  std::cout << "ok = " << std::boolalpha << s.ok() << "\n";
  std::cout << "message = \"" << s.message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "Flow::set_stage_remainder stage index out of range."
```

### Example 3

Switching to `Drop` changes the mapped output's column count (column 1,
outside the route, disappears), and resets `trained()` like
[`set_stage_route`](set_stage_route.md).

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::Flow flow{};
  flow.add_standard_scaler({}, {0});  // column 1 passes through by default

  const pluss::Series x = pluss::make_series({1.0, 10.0, 2.0, 20.0, 3.0, 30.0}, {6}, f64).reshape({3, 2});
  cls::FeatureBatch batch{};
  batch.x = x;
  const pluss::Series pass_through = flow.learn_map(batch).value();
  std::cout << "PassThrough mapped = " << pass_through << "\n";

  flow.set_stage_remainder(0, cls::RouteRemainderPolicy::Drop);
  std::cout << "trained() after set_stage_remainder() = " << std::boolalpha << flow.trained() << "\n";
  const pluss::Series dropped = flow.learn_map(batch).value();
  std::cout << "Drop mapped = " << dropped << "\n";

  return 0;
}
```

Output

```text
PassThrough mapped = pluss::Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[-1.2247449, 10],
  [0, 20],
  [1.2247449, 30]]
trained() after set_stage_remainder() = false
Drop mapped = pluss::Series(shape=[3,1], strides=[1,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[-1.2247449],
  [0],
  [1.2247449]]
```

## See Also

- [`set_stage_route`](set_stage_route.md) — the other route-adjacent mutator,
  also resets `trained()`.
- [`set_stage_key`](set_stage_key.md) — the one stage mutator that does not.
- [`add_named_transform`](add_named_transform.md) /
  [`add_column_stage`](add_column_stage.md) — the two `add_*` forms that can
  set `remainder` at construction instead.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="set_stage_key.html">
    <span class="label">Previous</span>
    <span class="title">set_stage_key</span>
  </a>
  <a class="next-link" href="set_stage_route.html">
    <span class="label">Next</span>
    <span class="title">set_stage_route</span>
  </a>
</div>
<!-- apiref:nav:end -->
