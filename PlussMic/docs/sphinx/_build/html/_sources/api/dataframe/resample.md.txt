# DataFrame::resample

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::resample(std::string_view time_feature, pluss::RangeView<std::string_view> value_features, std::string_view freq, std::string_view op = "mean", std::int64_t step = 1, bool render = false) -> DataFrameView
```

## What It Does

This is part of the time-aware surface and expects timeline or time-index semantics.

## Parameters

**time_feature**
    Name of the Timeline or TimeIndex feature that defines the resampling, alignment, or calendar axis.

**value_features**
    Feature name(s) to aggregate inside each resampling bin. Leave empty when you want PlussMic to infer compatible value features.

**freq**
    Bin or calendar frequency such as `"1h"`, `"D"`, `"W"`, or `"M"`.

**op**
    Bin reduction to compute in each time bucket.

    Supported values:
    - `count`: Count observed values within each bucket.
    - `size`: Count rows within each bucket, including missing values.
    - `sum`: Compute the numeric sum within each bucket.
    - `mean`: Compute the arithmetic mean within each bucket.
    - `min`: Compute the minimum within each bucket.
    - `max`: Compute the maximum within each bucket.
    - `first`: Take the first observed value within each bucket.
    - `last`: Take the last observed value within each bucket.

**step**
    Positive frequency multiplier. For example, `6` with hourly frequency means six-hour bins.

**render**
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

## Returns

`DataFrameView`
    Value produced by this call.

## Examples

Example 1. Choose whether the returned object should auto-render in display contexts.

```cpp
auto df = pluss::table::DataFrame({
    {"ts", pluss::table::Thread(pluss::make_timeline({
        "2024-01-01T09:10:00Z",
        "2024-01-01T09:40:00Z",
        "2024-01-01T10:05:00Z",
    }))},
    {"load", pluss::table::Thread(pluss::make_series({10.0, 20.0, 15.0}))},
});

auto out = df.resample("ts", {"load"}, "1h", "mean", 1, false);
```

### Output

```text
ts                   load
2024-01-01T09:00:00Z  15.0
2024-01-01T10:00:00Z  15.0
```

## Practical Behavior

- `resample(...)` groups rows into temporal bins and aggregates one value per bin and selected feature.
- `count` ignores missing values, while `size` counts rows whether the value is missing or not.
- For fixed frequencies, bins are anchored from the start of the local day rather than from the first observation timestamp.
- Numeric reductions such as `mean`, `sum`, `min`, `max`, `first`, and `last` require numeric Series features.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="replace.html">
    <span class="label">Previous</span>
    <span class="title">replace</span>
  </a>
  <a class="next-link" href="reset_cases.html">
    <span class="label">Next</span>
    <span class="title">reset_cases</span>
  </a>
</div>
