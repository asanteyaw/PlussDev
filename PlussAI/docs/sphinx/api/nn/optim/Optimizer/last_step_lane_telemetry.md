<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 98be12d27a06 -->

# `pluss::ai::nn::optim::Optimizer::last_step_lane_telemetry`

[Back to `pluss::ai::nn::optim::Optimizer`](../Optimizer.md)

## Signature

```cpp
const std::vector<pluss::ai::nn::optim::StepLaneTelemetry>&
nn::optim::Optimizer::last_step_lane_telemetry() const noexcept;
```

## What It Does

Returns one [`StepLaneTelemetry`](../StepLaneTelemetry.md) entry per
parameter group from the most recent [`step`](step.md) call -- which
update lane [`ExecutionOptions`](../ExecutionOptions.md)`.lane` actually
requested for that group, which lane the optimizer resolved to run
(possibly different, when `lane == Auto` or a requested lane fell back),
and why (platform/dtype/shape support flags). This is read-only
introspection with no effect on the next `step()` -- it exists to answer
"what actually ran" after the fact, for logging, testing, or debugging a
lane that unexpectedly resolved to `Loop` instead of a faster batched
path. It is overwritten (not accumulated) on every `step()` call and
starts empty before the first one.

## Parameters

None.

## Returns

`const std::vector<StepLaneTelemetry>&` -- one entry per parameter group,
in group-index order, from the most recent `step()` call. Empty before the
first `step()`.

## Errors

None. `last_step_lane_telemetry` is `noexcept` and has no preconditions.

## Practical Examples

Three examples: `lane = Auto` resolving to `Batched` on CPU, an explicit
`lane = Loop` request reported as-is even though batching was available,
and a multi-group optimizer producing one telemetry entry per group.

### Example 1

With `ExecutionOptions.lane` left at its default (`Auto`), two
same-shape, same-platform, same-dtype parameters in one group resolve to
the `Batched` lane on CPU -- both `batched_supported` and
`fused_supported` report `true` for this group.

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

  const auto lane_name = [](optim::UpdateLane lane) {
    switch (lane) {
      case optim::UpdateLane::Auto: return "Auto";
      case optim::UpdateLane::Loop: return "Loop";
      case optim::UpdateLane::Foreach: return "Foreach";
      case optim::UpdateLane::Batched: return "Batched";
      case optim::UpdateLane::Fused: return "Fused";
    }
    return "?";
  };

  optim::SGDOptions options{};
  options.learning_rate = 0.1;
  optim::SGD optimizer(std::vector<Series*>{&a, &b}, options);

  const Series loss = a.mul(a).sum().add(b.mul(b).sum());
  loss.reverse();
  optimizer.step();

  const auto& entry = optimizer.last_step_lane_telemetry().front();
  std::cout << "requested = " << lane_name(entry.requested) << ", resolved = " << lane_name(entry.resolved) << "\n";
  std::cout << "active_parameters = " << entry.active_parameters
             << ", batched_supported = " << std::boolalpha << entry.batched_supported
             << ", fused_supported = " << entry.fused_supported << "\n";

  return 0;
}
```

Output

```text
requested = Auto, resolved = Batched
active_parameters = 2, batched_supported = true, fused_supported = true
```

### Example 2

Forcing `lane = Loop` explicitly: `resolved` matches the request exactly,
even though `batched_supported` still reports `true` -- the scalar loop ran
because it was asked for, not because batching was unavailable.

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

  const auto lane_name = [](optim::UpdateLane lane) {
    switch (lane) {
      case optim::UpdateLane::Auto: return "Auto";
      case optim::UpdateLane::Loop: return "Loop";
      case optim::UpdateLane::Foreach: return "Foreach";
      case optim::UpdateLane::Batched: return "Batched";
      case optim::UpdateLane::Fused: return "Fused";
    }
    return "?";
  };

  optim::SGDOptions options{};
  options.learning_rate = 0.1;
  options.execution.lane = optim::UpdateLane::Loop;
  optim::SGD optimizer(std::vector<Series*>{&a, &b}, options);

  const Series loss = a.mul(a).sum().add(b.mul(b).sum());
  loss.reverse();
  optimizer.step();

  const auto& entry = optimizer.last_step_lane_telemetry().front();
  std::cout << "requested = " << lane_name(entry.requested) << ", resolved = " << lane_name(entry.resolved) << "\n";
  std::cout << "batched_supported = " << std::boolalpha << entry.batched_supported
             << " (still true -- Loop was requested explicitly, not forced by an unsupported combination)\n";

  return 0;
}
```

Output

```text
requested = Loop, resolved = Loop
batched_supported = true (still true -- Loop was requested explicitly, not forced by an unsupported combination)
```

### Example 3

Two parameter groups, each with its own `SGDOptions` (a fast group and a
slow, explicitly-looped group) via [`SGDGroup`](../SGDGroup.md) --
`last_step_lane_telemetry()` reports one entry per group, in group order.

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
  Series a = pluss::make_series({5.0F, 1.0F}, {2}, f32);
  Series b = pluss::make_series({-3.0F, 0.5F}, {2}, f32);
  a.tracks_slope_(true);
  b.tracks_slope_(true);

  optim::SGDOptions fast{};
  fast.learning_rate = 0.1;
  optim::SGDOptions slow{};
  slow.learning_rate = 0.01;
  slow.execution.lane = optim::UpdateLane::Loop;

  std::vector<optim::SGDGroup> groups{
      optim::SGDGroup{{&a}, fast},
      optim::SGDGroup{{&b}, slow},
  };
  optim::SGD optimizer(groups);

  const Series loss = a.mul(a).sum().add(b.mul(b).sum());
  loss.reverse();
  optimizer.step();

  for (const auto& entry : optimizer.last_step_lane_telemetry()) {
    std::cout << "group " << entry.group_index << ": active_parameters = " << entry.active_parameters << "\n";
  }

  return 0;
}
```

Output

```text
group 0: active_parameters = 1
group 1: active_parameters = 1
```

## See Also

- [`StepLaneTelemetry`](../StepLaneTelemetry.md) -- the full field list
  each entry carries.
- [`ExecutionOptions`](../ExecutionOptions.md) -- the options struct
  `requested`/`resolved` reflect.
- [`step`](step.md) -- populates this telemetry as part of every call.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="clear.html">
    <span class="label">Previous</span>
    <span class="title">clear</span>
  </a>
  <a class="next-link" href="parameters.html">
    <span class="label">Next</span>
    <span class="title">parameters</span>
  </a>
</div>
<!-- apiref:nav:end -->
