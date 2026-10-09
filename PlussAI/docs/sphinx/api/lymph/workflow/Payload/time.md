<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d2ebfc056743 -->

# `lymph::workflow::Payload::time`

[Back to `lymph::workflow::Payload`](../Payload.md)

`time` attaches the time role — the column (and, optionally, the
leakage-window contract) that chronological cross-validation reads. Three
overloads: a bare column name, a full [`TimeOptions`](../TimeOptions.md), or
an already-separate `Series`. `time_feature()` and `time_options()` read the
attachment back.

## Signature

```cpp
lymph::workflow::Payload& lymph::workflow::Payload::time(std::string feature_name);
lymph::workflow::Payload& lymph::workflow::Payload::time(lymph::workflow::TimeOptions options);
lymph::workflow::Payload& lymph::workflow::Payload::time(pluss::Series values);

bool lymph::workflow::Payload::has_time() const;
std::string lymph::workflow::Payload::time_feature() const;
const std::optional<lymph::workflow::TimeOptions>& lymph::workflow::Payload::time_options() const;
const pluss::Series& lymph::workflow::Payload::time() const;
```

## What It Does

`time` sets the time role of a payload: the column that chronological cross-validation orders rows by, and optionally a leakage window. It takes a column name, a full time options struct, or a separate series. The getters return what was attached.

## Parameters

`std::string feature_name`
: The time column's name. The simplest form — `time_feature()` returns it and
  `time_options()` stays empty (no horizon/gap/lookback contract).

`lymph::workflow::TimeOptions options`
: The full contract: `index`, `horizon`, `gap`, `lookback`. Validated
  immediately — see Errors. `time_feature()` returns `options.index` and
  `time_options()` returns the whole struct. Read by
  [`selection::validate::time`](../../selection/validate/time.md) to fill any
  of its own unset window fields.

`pluss::Series values`
: An already-separate time vector, one value per row. `has_time()` becomes
  `true` and `time()` returns it; `time_feature()` stays empty.

## Returns

The same `Payload` by reference. `time_feature()` is filled by the name and
`TimeOptions` forms; `time_options()` only by the `TimeOptions` form; the
`Series` accessor `time()` only by the `Series` form.

## Errors

- `time(TimeOptions)` validates on the spot and throws for a negative
  `horizon` / `gap` / `lookback`, or an empty `index` when
  `reject_missing_index` is left at its default `true` (Example 3).
- `time(std::string)` throws if the name is empty.

## Practical Examples

### Example 1

`time(name)` — just the column. No window contract attached.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Slate(pluss::make_series({1.0, 1.4, 1.8, 2.2}, {4}, f64))},
      {"balance", Slate(pluss::make_series({0.6, 1.0, 1.3, 1.6}, {4}, f64))},
      {"default", Slate(pluss::make_series({0, 0, 1, 1}, {4}, f64))},
      {"observed_at", Slate(pluss::make_timeline({
          "2026-01-01T00:00:00Z", "2026-01-02T00:00:00Z",
          "2026-01-03T00:00:00Z", "2026-01-04T00:00:00Z"}))},
  });

  const workflow::Payload payload = workflow::payload::make(raw)
                                        .features({"income", "balance"})
                                        .label("default")
                                        .time("observed_at");

  std::cout << "has_time()               = " << std::boolalpha << payload.has_time() << "\n";
  std::cout << "time_feature()           = " << payload.time_feature() << "\n";
  std::cout << "time_options().has_value = " << payload.time_options().has_value() << "\n";

  return 0;
}
```

Output

```text
has_time()               = true
time_feature()           = observed_at
time_options().has_value = false
```

### Example 2

`time(TimeOptions)` — the leakage-aware contract. `time_options()` reads the
whole struct back.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Slate(pluss::make_series({1.0, 1.4, 1.8, 2.2}, {4}, f64))},
      {"balance", Slate(pluss::make_series({0.6, 1.0, 1.3, 1.6}, {4}, f64))},
      {"default", Slate(pluss::make_series({0, 0, 1, 1}, {4}, f64))},
      {"observed_at", Slate(pluss::make_timeline({
          "2026-01-01T00:00:00Z", "2026-01-02T00:00:00Z",
          "2026-01-03T00:00:00Z", "2026-01-04T00:00:00Z"}))},
  });

  workflow::TimeOptions time_options{};
  time_options.index = "observed_at";
  time_options.horizon = workflow::steps(2);
  time_options.gap = workflow::steps(1);

  const workflow::Payload payload = workflow::payload::make(raw)
                                        .features({"income", "balance"})
                                        .label("default")
                                        .time(time_options);

  std::cout << "time_feature()             = " << payload.time_feature() << "\n";
  std::cout << "time_options().has_value() = " << std::boolalpha
            << payload.time_options().has_value() << "\n";
  std::cout << "time_options()->horizon    = " << payload.time_options()->horizon << "\n";
  std::cout << "time_options()->gap        = " << payload.time_options()->gap << "\n";

  return 0;
}
```

Output

```text
time_feature()             = observed_at
time_options().has_value() = true
time_options()->horizon    = 2
time_options()->gap        = 1
```

### Example 3

`time(Series)` — the vector form; and the immediate validation of a bad
`TimeOptions`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Slate(pluss::make_series({1.0, 1.4, 1.8, 2.2}, {4}, f64))},
      {"balance", Slate(pluss::make_series({0.6, 1.0, 1.3, 1.6}, {4}, f64))},
  });
  const pluss::Series t = pluss::make_series({0.0, 1.0, 2.0, 3.0}, {4}, f64);

  const workflow::Payload payload =
      workflow::payload::make(raw).features({"income", "balance"}).time(t);
  std::cout << "has_time()      = " << std::boolalpha << payload.has_time() << "\n";
  std::cout << "time().defined()= " << payload.time().defined() << "\n";

  workflow::TimeOptions invalid{};
  invalid.index = "observed_at";
  invalid.horizon = -1;
  try {
    (void)workflow::payload::make(raw).features({"income", "balance"}).time(invalid);
    std::cout << "time(invalid): no error (unexpected)\n";
  } catch (const std::exception& e) {
    std::cout << "time(invalid) threw: " << e.what() << "\n";
  }

  return 0;
}
```

Output

```text
has_time()      = true
time().defined()= true
time(invalid) threw: lymph::workflow::Payload::time: horizon cannot be negative.
```

## See Also

- [`TimeOptions`](../TimeOptions.md) — the full field list the middle
  overload takes.
- [`selection::validate::time`](../../selection/validate/time.md) — the
  consumer that reads `time_options()` back.
- [`Workflow::time`](../Workflow.md#configuration) — the same contract on a
  `Workflow` directly.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="roles.html">
    <span class="label">Previous</span>
    <span class="title">roles</span>
  </a>
  <a class="next-link" href="weights.html">
    <span class="label">Next</span>
    <span class="title">weights</span>
  </a>
</div>
<!-- apiref:nav:end -->
