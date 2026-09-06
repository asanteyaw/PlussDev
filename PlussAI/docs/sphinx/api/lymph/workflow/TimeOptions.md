<!-- pluss-hand-authored -->

# `lymph::workflow::TimeOptions`

[Back to `lymph::workflow::Workflow`](Workflow.md)

`TimeOptions` names the time role — index column, forecast horizon, gap, and
lookback — that a time-aware [`Workflow`](Workflow.md#configuration) or
[`Payload`](Payload.md) carries. Setting it (`Workflow::time(...)` /
`Payload::time(...)`) does not itself change how `learn`/`infer` run; it is
read back by [`selection::validate::time`](../selection/validate/time.md),
which fills in any of its own unset chronological-split fields from here —
one place to declare the leakage-aware contract instead of repeating it at
every validation call site.

| Field | Meaning | Default |
|---|---|---|
| `index` | The time column's name | `""` (required) |
| `observed_at` | Reserved: observation-time column, for a future observed-vs-target split | `""` |
| `target_at` | Reserved: pairs with `observed_at` | `""` |
| `horizon` | How far ahead a fold's test window reaches | `0` |
| `gap` | Rows skipped between train and test windows | `0` |
| `lookback` | Cap on training rows per fold (`0` = no cap) | `0` |
| `require_ordered` | Reserved: whether the index must already be sorted | `true` |
| `reject_missing_index` | Whether an empty `index` is a hard error | `true` |

`horizon`, `gap`, and `lookback` are plain row counts — set them through the
[`steps(n)` / `days(n)`](#guidance) helpers below rather than a bare integer,
so `time_options.horizon = workflow::steps(3);` reads as "3 rows" at the call
site instead of an unlabeled `3`.

## Fields

### `index`

`std::string index = ""`

The time column's name. Required (see `reject_missing_index` below) — this is
what `Payload::time_feature()` / a time-aware `Workflow` reads back.

### `observed_at`

`std::string observed_at = ""`

Optional secondary time column naming when observation time and target time
differ (e.g. a label only known some horizon after the row was observed).
Not yet consumed by `selection::validate::time` — reserved for a future
observed-vs-target split.

### `target_at`

`std::string target_at = ""`

Pairs with `observed_at`; same reserved status.

### `horizon`

`TimeOffset horizon = 0`

How far ahead a chronological fold's test window reaches, in row-steps.
Cannot be negative. Read by `selection::validate::time` to fill
`CrossValOptions::time_test_size` when that field is left at its own default.

### `gap`

`TimeOffset gap = 0`

Rows skipped between a fold's training window and its test window — models a
forecast lead time. Cannot be negative. Fills
`CrossValOptions::time_gap`.

### `lookback`

`TimeOffset lookback = 0`

Cap on training rows per fold (`0` = expanding window, no cap; a positive
value makes it a sliding window). Cannot be negative. Fills
`CrossValOptions::time_max_train_size`.

### `require_ordered`

`bool require_ordered = true`

Whether the time index must already be sorted ascending. Reserved for a
future explicit-sort path; the current chronological split assumes an
already-ordered index regardless of this flag.

### `reject_missing_index`

`bool reject_missing_index = true`

When true (the default), `validate_time_options(...)` throws if `index` is
empty — a time-aware contract with no named time column is treated as a
configuration mistake, not a silent no-op. Set `false` only for a caller that
builds `TimeOptions` incrementally and validates once the index is filled in.

## Guidance

`steps(n)` and `days(n)` are both plain `TimeOffset` (row-index) wrappers
today — the chronological-split engine underneath is sample-index based, not
calendar-aware, so the two currently produce identical values. Use whichever
name matches the caller's intent (`steps(3)` for "3 rows," `days(1)` for "one
calendar day, assuming one row per day") — the distinction is there so
existing call sites do not need to change once calendar-aware timestamps
land.

`validate_time_options(options, where)` is called automatically by
`Workflow::time(...)` and `Payload::time(TimeOptions)` — a caller does not
normally need to call it directly; it is exposed because
`selection::validate::time` also calls it on the merged, fallback-filled
options before running a chronological split.

## Practical Example

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;

  workflow::TimeOptions time_options{};
  time_options.index = "observed_at";
  time_options.horizon = workflow::steps(3);
  time_options.gap = workflow::days(1);
  time_options.lookback = workflow::steps(30);

  workflow::validate_time_options(time_options, "example");
  std::cout << "has_time_index = " << workflow::has_time_index(time_options) << "\n";

  workflow::Workflow wf = workflow::make(lymph::models::logistic_regression());
  wf.time(time_options);
  std::cout << "time_aware = " << wf.time_aware() << "\n";
  std::cout << "time_options().index = " << wf.time_options()->index
            << " horizon=" << wf.time_options()->horizon
            << " gap=" << wf.time_options()->gap
            << " lookback=" << wf.time_options()->lookback << "\n";

  workflow::TimeOptions invalid{};
  invalid.index = "observed_at";
  invalid.horizon = -1;
  try {
    workflow::validate_time_options(invalid, "example(invalid)");
    std::cout << "validate_time_options: no error (unexpected)\n";
  } catch (const std::exception& e) {
    std::cout << "validate_time_options threw: " << e.what() << "\n";
  }

  return 0;
}
```

Output

```text
has_time_index = 1
time_aware = 1
time_options().index = observed_at horizon=3 gap=1 lookback=30
validate_time_options threw: example(invalid): horizon cannot be negative.
```

## See Also

- [`Workflow::time`](Workflow.md#configuration) / [`Payload::time`](Payload.md#configuration)
  — the two setters that accept `TimeOptions`.
- [`selection::validate::time`](../selection/validate/time.md) — reads back
  any unset chronological-split field from here.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
