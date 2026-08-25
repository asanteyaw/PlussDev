# `lymph::validate::time`

## Summary

`validate::time(...)` creates a time-ordered validation plan.

Use it when:

- ordering matters
- future rows must never leak into past training windows
- you want a workflow-facing chronological validation surface

## What It Means

`validate::time(...)` is the public entrypoint for chronological validation.

Use it whenever row order is part of the problem definition itself, not merely a
presentation detail.

This includes cases such as:

- forecasting
- behavioral modeling over time
- transaction drift analysis
- out-of-time model review

The returned object is still a reusable `lymph::validate::Plan`, but its semantics
are fundamentally different from ordinary cross-validation:

- training windows must precede test windows
- future information must not leak backward
- shuffling is not allowed

## High-Value Time Toggles

Time plans usually revolve around three parameters:

### `time_gap`

Adds a buffer between the train block and the test block.

Use it when near-neighbor temporal leakage is a real risk.

### `time_test_size`

Sets the size of each test window.

Use it to encode the actual business horizon rather than leaving the horizon
implicit.

### `time_max_train_size`

Caps the size of the training window.

This is how you move from expanding-history behavior toward a rolling-window regime.

## Signature

```cpp
lymph::ValidationPlan lymph::validate::time(
    lymph::ValidationOptions options = {});
std::vector<lymph::TimeSeriesSplit> lymph::ValidationPlan::splits(
    const pluss::table::DataFrame& frame,
    std::string_view time_feature) const;
lymph::TimeSeriesSplit lymph::ValidationPlan::last(
    const pluss::table::DataFrame& frame,
    std::string_view time_feature) const;
```

## Practical Examples

These examples show two important chronological behaviors:

1. extracting the final out-of-time block
2. tightening the window with a gap and shorter lookback

### Example 1

This example creates a chronological plan and asks for the final time block
directly.

```cpp
#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const DataFrame frame({
      {"income", Thread(pluss::make_series({1.0, 1.4, 1.8, 2.2, 3.8, 4.2, 4.6, 5.0}))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, 0.6, 0.9, 1.8, 2.1, 2.4, 2.7}))},
      {"late", Thread(pluss::make_series({0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0}))},
      {"target", Thread(pluss::make_series({0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0}))},
      {"when", Thread(pluss::make_timeline({
          "2026-01-01T00:00:00Z",
          "2026-01-02T00:00:00Z",
          "2026-01-03T00:00:00Z",
          "2026-01-04T00:00:00Z",
          "2026-01-05T00:00:00Z",
          "2026-01-06T00:00:00Z",
          "2026-01-07T00:00:00Z",
          "2026-01-08T00:00:00Z",
      }))},
  });

  lymph::ValidationOptions opts{};
  opts.folds = 3;
  opts.shuffle = false;
  opts.time_gap = 0;
  opts.time_test_size = 2;
  opts.time_max_train_size = 4;

  const lymph::ValidationPlan plan = lymph::validate::time(opts);
  const lymph::prep::TimeSeriesSplit final_block = plan.last(frame, "when");

  std::cout << "== time_plan_last ==\n";
  std::cout << "train_rows=" << final_block.train_set.ncases()
            << " test_rows=" << final_block.test_set.ncases() << "\n";
  std::cout << "train_last_when=" << final_block.train_set.features("when").tail(1)
            << " test_first_when=" << final_block.test_set.features("when").head(1) << "\n";
}
```

Output

```text
== time_plan_last ==
train_rows=4 test_rows=2
train_last_when=5  2026-01-06
Name: when, ptype: Timeline
 test_first_when=6  2026-01-07
Name: when, ptype: Timeline
```

This is exactly why the time builder exists: the chronological contract is visible
and auditable instead of being hidden inside manual sorting and slicing.

### Example 2

This follow-up example keeps the same `frame` and base `opts` shape, but changes
one-step gap and shortens the lookback. That changes where the train window ends
before the final test block begins.

```cpp
lymph::ValidationOptions gap_opts = opts;
gap_opts.time_gap = 1;
gap_opts.time_max_train_size = 3;

const lymph::ValidationPlan gap_plan = lymph::validate::time(gap_opts);
const lymph::prep::TimeSeriesSplit gap_block = gap_plan.last(frame, "when");

std::cout << "== time_plan_gap_last ==\n";
std::cout << "train_rows=" << gap_block.train_set.ncases()
          << " test_rows=" << gap_block.test_set.ncases() << "\n";
std::cout << "train_last_when=" << gap_block.train_set.features("when").tail(1)
          << " test_first_when=" << gap_block.test_set.features("when").head(1) << "\n";
```

Output

```text
== time_plan_gap_last ==
train_rows=3 test_rows=2
train_last_when=4  2026-01-05
Name: when, ptype: Timeline
 test_first_when=6  2026-01-07
Name: when, ptype: Timeline
```

This second example is the real teaching point for `time_gap` and `lookback`:

- the train block got shorter
- the final train timestamp moved farther away from the test block
- the validation contract became stricter against leakage

## Guardrails

Time validation is where APIs most often lie to users through accidental leakage.

That is why `validate::time(...)` should be treated as a semantic choice, not just
another split flavor. If the task is time-aware, the plan should say so openly.

## Related Pages

- [ValidationOptions](../metrics/ValidationOptions.md)
- [Plan](Plan.md)
