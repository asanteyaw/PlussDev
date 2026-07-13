# DataFrame::tz_convert

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::tz_convert(pluss::RangeView<std::string_view> features, std::string_view tz_name) -> DataFrame
```

## What It Does

This is part of the time-aware surface and expects timeline or time-index semantics.

## Parameters

**features**
    Feature name(s) that select the target DataFrame columns for the operation.

**tz_name**
    Destination timezone name such as `"UTC"` or `"America/Edmonton"`.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"ts", pluss::table::Thread(pluss::make_timeline({
        "2024-01-01 09:10:00",
        "2024-01-01 09:40:00",
        "2024-01-01 10:05:00",
    }))},
    {"load", pluss::table::Thread(pluss::make_series({10.0, 20.0, 15.0}))},
});

auto localized = df.tz_localize({"ts"}, "UTC");
auto out = localized.tz_convert({"ts"}, "UTC");
```

### Output

```text
ts
2024-01-01T09:10:00Z
2024-01-01T09:40:00Z
2024-01-01T10:05:00Z
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ts.html">
    <span class="label">Previous</span>
    <span class="title">ts</span>
  </a>
  <a class="next-link" href="tz_localize.html">
    <span class="label">Next</span>
    <span class="title">tz_localize</span>
  </a>
</div>
