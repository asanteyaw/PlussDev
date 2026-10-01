# `pluss::ai::nn::optim::StepLaneTelemetry`

A plain report struct: one instance per parameter group, describing which
update lane that group's most recent [`step()`](Optimizer/step.md) call
requested versus what it actually resolved to and ran, plus enough
platform/dtype/support context to explain why. Never constructed directly
by calling code -- every instance comes from
[`Optimizer::last_step_lane_telemetry()`](Optimizer/last_step_lane_telemetry.md),
which returns one per group from the most recent `step()`.

## Signature

```cpp
nn::optim::StepLaneTelemetry::StepLaneTelemetry() = default;
```

A plain aggregate -- every field below defaults to a harmless zero/`false`/
`Auto`/`Loop` value, but a default-constructed instance is never meaningful
on its own; every real instance comes from
[`last_step_lane_telemetry()`](Optimizer/last_step_lane_telemetry.md).

## State

### `group_index`

`std::size_t group_index`
: Which parameter group this entry describes -- matches the group's
  position in the optimizer's own construction order (single-group
  construction always produces `group_index == 0`; per-group construction
  via a `Group` list produces one entry per group, in list order).

### `requested`

`UpdateLane requested`
: That group's own [`ExecutionOptions`](ExecutionOptions.md)`.lane` value
  at the time `step()` ran -- the request, unresolved.

### `resolved`

`UpdateLane resolved`
: The lane that actually ran. Equal to `requested` unless `requested ==
  Auto` (resolved to whichever supported lane is fastest) or the
  requested lane was unsupported and `strict == false` (resolved to
  `Loop`).

### `platform`

`pluss::Platform platform`
: The platform of this group's "anchor" parameter -- the first active
  (defined, with a defined gradient) parameter found in the group.
  Support flags below (`batched_supported`, ...) are evaluated against
  this platform/dtype pair.

### `dtype`

`pluss::ScalarType dtype`
: The dtype of this group's anchor parameter -- see `platform` above.

### `active_parameters`

`std::size_t active_parameters`
: How many parameters in this group had both a defined value and a
  defined gradient on this step -- a parameter with no gradient this
  round (see [`Optimizer::clear`](Optimizer/clear.md) for why that can
  happen) is not counted.

### `batch_shape_compatible`

`bool batch_shape_compatible`
: Whether every active parameter in the group shares the anchor's
  platform, dtype, and exact shape -- the precondition `Batched`/`Fused`
  both need before platform support is even checked.

### `foreach_supported`

`bool foreach_supported`
: Whether the `Foreach` lane could have run for this group, given
  `batch_shape_compatible` and the anchor's platform/dtype. Currently
  always agrees with `batched_supported` (`Foreach` routes through the
  same vectorized path as `Batched`).

### `batched_supported`

`bool batched_supported`
: Whether the `Batched` lane could have run for this group -- see
  `foreach_supported` above.

### `fused_supported`

`bool fused_supported`
: Whether the `Fused` lane could have run for this group.

### `capturable_supported`

`bool capturable_supported`
: Whether this group's anchor platform supports
  [`ExecutionOptions`](ExecutionOptions.md)`.capturable` execution.

## Methods

None -- `StepLaneTelemetry` is a plain data report; every field is public
and read directly, see `## State` above.

## Practical Examples

One example: reading every field off a single-group optimizer's telemetry
after a step, with `lane` left at its default `Auto`.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;
  namespace optim = nn::optim;
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;

  const Config f32 = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  Series a = pluss::make_series({5.0F, 1.0F, 2.0F}, {3}, f32);
  Series b = pluss::make_series({-3.0F, 0.5F, 1.5F}, {3}, f32);
  a.tracks_slope_(true);
  b.tracks_slope_(true);

  optim::SGD optimizer(std::vector<Series*>{&a, &b}, optim::SGDOptions{.learning_rate = 0.1});

  const Series loss = a.mul(a).sum().add(b.mul(b).sum());
  loss.reverse();
  optimizer.step();

  const auto& entry = optimizer.last_step_lane_telemetry().front();
  std::cout << "group_index = " << entry.group_index << "\n";
  std::cout << "active_parameters = " << entry.active_parameters << "\n";
  std::cout << "batch_shape_compatible = " << std::boolalpha << entry.batch_shape_compatible << "\n";
  std::cout << "batched_supported = " << entry.batched_supported << "\n";
  std::cout << "fused_supported = " << entry.fused_supported << "\n";
  std::cout << "capturable_supported = " << entry.capturable_supported << "\n";

  return 0;
}
```

Output

```text
group_index = 0
active_parameters = 2
batch_shape_compatible = true
batched_supported = true
fused_supported = true
capturable_supported = true
```

## Changelog

- **0.1** — the eleven-field per-group lane report: request vs. resolution,
  platform/dtype anchor, and every lane's support flag.

## See Also

- [`Optimizer::last_step_lane_telemetry`](Optimizer/last_step_lane_telemetry.md)
  -- how this telemetry is retrieved, with `requested`-versus-`resolved`
  examples across `Auto`, an explicit lane, and multiple groups.
- [`ExecutionOptions`](ExecutionOptions.md) -- the options struct
  `requested` reflects.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
